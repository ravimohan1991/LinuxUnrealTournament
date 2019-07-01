// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTPlayerController.h"
#include "UTGameState.h"
#include "UTArmor.h"
#include "UTTimedPowerup.h"
#include "Misc/App.h"
#include "kiss_fft.h"
#include "tools/kiss_fftr.h"

#include "CorsairRGB.h"

// Somehow this got #define'd to GetTickCount()
#undef GetCurrentTime

int32 GCorsairSpectrumAnalyzer = 0;
static FAutoConsoleVariableRef GCorsairSpectrumAnalyzerCVar(
	TEXT("Corsair.Spectrum"),
	GCorsairSpectrumAnalyzer,
	TEXT("If 1, show spectrum analyzer on Corsair RGB")
	);

DEFINE_LOG_CATEGORY_STATIC(LogUTKBLightShow, Log, All);

ACorsairRGB::ACorsairRGB(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FCorsairRGB::FCorsairRGB()
{
	FrameTimeMinimum = 0.03f;
	DeltaTimeAccumulator = 0;
	bIsFlashingForEnd = false;
	bInitialized = false;
	bCorsairSDKEnabled = false;
	bPlayingIdleColors = false;
	AudioWorker = nullptr;
	
	FlashSpeed = 100;
}

IMPLEMENT_MODULE(FCorsairRGB, CorsairRGB)

void FCorsairRGB::StartupModule()
{
	CorsairPerformProtocolHandshake();
	CorsairError LastError = CorsairGetLastError();
	if (LastError != CE_Success)
	{
		if (LastError != CE_ServerNotFound)
		{
			UE_LOG(LogUTKBLightShow, Warning, TEXT("Failed to enable Corsair RGB"));
		}
	}
	else
	{
		if (CorsairRequestControl(CAM_ExclusiveLightingControl))
		{
			bCorsairSDKEnabled = true;
		}
	}
}

void FCorsairRGB::ShutdownModule()
{
	if (bCorsairSDKEnabled)
	{
		CorsairReleaseControl(CAM_ExclusiveLightingControl);
	}
}

void FCorsairRGB::Tick(float DeltaTime)
{
	if (GIsEditor)
	{
		return;
	}
	
	if (!bCorsairSDKEnabled)
	{
		return;
	}

	UUTGameUserSettings* UserSettings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
	if (UserSettings && !UserSettings->IsKeyboardLightingEnabled())
	{
		if (bCorsairSDKEnabled)
		{
			CorsairReleaseControl(CAM_ExclusiveLightingControl);
			bCorsairSDKEnabled = false;
			StopAudioCapture();
		}

		return;
	}
	
	// Avoid double ticking
	if (LastFrameCounter > 0 && LastFrameCounter == GFrameCounter)
	{
		return;
	}

	LastFrameCounter = GFrameCounter;

	// We may be going 120hz, don't spam the device
	DeltaTimeAccumulator += DeltaTime;
	if (DeltaTimeAccumulator < FrameTimeMinimum)
	{
		return;
	}
	DeltaTimeAccumulator = 0;

	if (GCorsairSpectrumAnalyzer)
	{
		if (AudioWorker == nullptr)
		{
			CaptureAudio();
		}
		UpdateKeyboardToSpectrum();
		return;
	}
	else
	{
		StopAudioCapture();
	}

	AUTPlayerController* UTPC = nullptr;
	AUTGameState* GS = nullptr;
	const TIndirectArray<FWorldContext>& AllWorlds = GEngine->GetWorldContexts();
	for (const FWorldContext& Context : AllWorlds)
	{
		UWorld* World = Context.World();
		if (World && World->WorldType == EWorldType::Game)
		{
			UTPC = Cast<AUTPlayerController>(GEngine->GetFirstLocalPlayerController(World));
			if (UTPC)
			{
				UUTLocalPlayer* UTLP = Cast<UUTLocalPlayer>(UTPC->GetLocalPlayer());
				if (UTLP == nullptr || UTLP->IsMenuGame())
				{
					UTPC = nullptr;
					continue;
				}

				GS = World->GetGameState<AUTGameState>();
				break;
			}
		}
	}

	if (!UTPC || !GS)
	{
		UpdateIdleColors(DeltaTime);
		return;
	}

	if (GS->HasMatchEnded())
	{
		UpdateIdleColors(DeltaTime);
		return;
	}
	else if (GS->HasMatchStarted())
	{
		bPlayingIdleColors = false;

		// Spectators and DM players are all on 255
		int32 TeamNum = UTPC->GetTeamNum();
		
		int R = 0;
		int G = 0;
		int B = 0;
		if (TeamNum == 0)
		{
			R = 255;
		}
		else if (TeamNum == 1)
		{
			B = 255;
		}
		
		TArray<CorsairLedColor> ColorsToSend;

		CorsairLedPositions* ledPositions = CorsairGetLedPositions();
		if (ledPositions)
		{
			ColorsToSend.Reserve(ledPositions->numberOfLed * 2);
		}

		TArray<CorsairLedId> UsedKeys;
		if (UTPC->GetUTCharacter() && !UTPC->GetUTCharacter()->IsDead())
		{
			// Find WASD and other variants like ESDF or IJKL
			TArray<FKey> Keys;
			UTPC->ResolveKeybindToFKey(TEXT("MoveForward"), Keys, false, true);
			for (int i = 0; i < Keys.Num(); i++)
			{
				FString KeyName = Keys[i].ToString();
				if (KeyName.Len() == 1)
				{
					auto KeyNameAnsi = StringCast<ANSICHAR>(*KeyName);
					CorsairLedId ledId = CorsairGetLedIdForKeyName(KeyNameAnsi.Get()[0]);
					if (ledId != CLI_Invalid)
					{
						ColorsToSend.Add(CorsairLedColor{ ledId, 0, 255, 0 });
						UsedKeys.Add(ledId);
					}
				}
			}
			UTPC->ResolveKeybindToFKey(TEXT("MoveBackward"), Keys, false, true);
			for (int i = 0; i < Keys.Num(); i++)
			{
				FString KeyName = Keys[i].ToString();
				if (KeyName.Len() == 1)
				{
					auto KeyNameAnsi = StringCast<ANSICHAR>(*KeyName);
					CorsairLedId ledId = CorsairGetLedIdForKeyName(KeyNameAnsi.Get()[0]);
					if (ledId != CLI_Invalid)
					{
						ColorsToSend.Add(CorsairLedColor{ ledId, 0, 255, 0 });
						UsedKeys.Add(ledId);
					}
				}
			}
			UTPC->ResolveKeybindToFKey(TEXT("MoveLeft"), Keys, false, true);
			for (int i = 0; i < Keys.Num(); i++)
			{
				FString KeyName = Keys[i].ToString();
				if (KeyName.Len() == 1)
				{
					auto KeyNameAnsi = StringCast<ANSICHAR>(*KeyName);
					CorsairLedId ledId = CorsairGetLedIdForKeyName(KeyNameAnsi.Get()[0]);
					if (ledId != CLI_Invalid)
					{
						ColorsToSend.Add(CorsairLedColor{ ledId, 0, 255, 0 });
						UsedKeys.Add(ledId);
					}
				}
			}
			UTPC->ResolveKeybindToFKey(TEXT("MoveRight"), Keys, false, true);
			for (int i = 0; i < Keys.Num(); i++)
			{
				FString KeyName = Keys[i].ToString();
				if (KeyName.Len() == 1)
				{
					auto KeyNameAnsi = StringCast<ANSICHAR>(*KeyName);
					CorsairLedId ledId = CorsairGetLedIdForKeyName(KeyNameAnsi.Get()[0]);
					if (ledId != CLI_Invalid)
					{
						ColorsToSend.Add(CorsairLedColor{ ledId, 0, 255, 0 });
						UsedKeys.Add(ledId);
					}
				}
			}
			UTPC->ResolveKeybindToFKey(TEXT("TurnRate"), Keys, false, true);
			for (int i = 0; i < Keys.Num(); i++)
			{
				FString KeyName = Keys[i].ToString();
				if (KeyName.Len() == 1)
				{
					auto KeyNameAnsi = StringCast<ANSICHAR>(*KeyName);
					CorsairLedId ledId = CorsairGetLedIdForKeyName(KeyNameAnsi.Get()[0]);
					if (ledId != CLI_Invalid)
					{
						ColorsToSend.Add(CorsairLedColor{ ledId, 0, 255, 0 });
						UsedKeys.Add(ledId);
					}
				}
			}


			bool bFoundWeapon = false;
			for (int32 i = 0; i < 10; i++)
			{
				bFoundWeapon = false;

				int32 CurrentWeaponGroup = -1;
				if (UTPC->GetUTCharacter()->GetWeapon())
				{
					CurrentWeaponGroup = UTPC->GetUTCharacter()->GetWeapon()->DefaultGroup;
				}

				for (TInventoryIterator<AUTWeapon> It(UTPC->GetUTCharacter()); It; ++It)
				{
					AUTWeapon* Weap = *It;
					if (Weap->HasAnyAmmo())
					{
						if (Weap->DefaultGroup == (i + 1))
						{
							bFoundWeapon = true;
							break;
						}
					}
				}
				
				if (bFoundWeapon)
				{
					switch (i)
					{
					case 0:
					default:
						ColorsToSend.Add(CorsairLedColor{ CorsairLedId(CLK_1 + i), 128, 128, 128 });
						break;
					case 1:
						ColorsToSend.Add(CorsairLedColor{ CorsairLedId(CLK_1 + i), 255, 255, 255 });
						break;
					case 2:
						ColorsToSend.Add(CorsairLedColor{ CorsairLedId(CLK_1 + i), 0, 255, 0 });
						break;
					case 3:
						ColorsToSend.Add(CorsairLedColor{ CorsairLedId(CLK_1 + i), 128, 0, 128 });
						break;
					case 4:
						ColorsToSend.Add(CorsairLedColor{ CorsairLedId(CLK_1 + i), 0, 255, 255 });
						break;
					case 5:
						ColorsToSend.Add(CorsairLedColor{ CorsairLedId(CLK_1 + i), 0, 0, 255 });
						break;
					case 6:
						ColorsToSend.Add(CorsairLedColor{ CorsairLedId(CLK_1 + i), 255, 255, 0 });
						break;
					case 7:
						ColorsToSend.Add(CorsairLedColor{ CorsairLedId(CLK_1 + i), 255, 0, 0 });
						break;
					case 8:
						ColorsToSend.Add(CorsairLedColor{ CorsairLedId(CLK_1 + i), 255, 255, 255 });
						break;
					}
				}
				else
				{
					ColorsToSend.Add(CorsairLedColor{ CorsairLedId(CLK_1 + i), 0, 0, 0 });
				}

				UsedKeys.Add(CorsairLedId(CLK_1 + i));
			}
		}

		if (ledPositions)
		{
			for (int i = 0; i < ledPositions->numberOfLed; i++)
			{
				CorsairLedId ledId = ledPositions->pLedPosition[i].ledId;
				if (!UsedKeys.Contains(ledId))
				{
					ColorsToSend.Add(CorsairLedColor{ ledId, R, G, B });
				}
			}

			CorsairSetLedsColorsAsync(ColorsToSend.Num(), ColorsToSend.GetData(), nullptr, nullptr);
		}

	}
	else
	{
		UpdateIdleColors(DeltaTime);
	}

}

void FCorsairRGB::UpdateIdleColors(float DeltaTime)
{
	if (!bPlayingIdleColors)
	{
		TArray<CorsairLedColor> ColorsToSend;

		CorsairLedPositions* ledPositions = CorsairGetLedPositions();
		if (ledPositions)
		{
			ColorsToSend.Reserve(ledPositions->numberOfLed * 2);
			
			for (int i = 0; i < ledPositions->numberOfLed; i++)
			{
				CorsairLedId ledId = ledPositions->pLedPosition[i].ledId;
				ColorsToSend.Add(CorsairLedColor{ ledId, 255, 255, 255 });
			}
		}

		CorsairSetLedsColorsAsync(ColorsToSend.Num(), ColorsToSend.GetData(), nullptr, nullptr);

		bPlayingIdleColors = true;
	}
}

FCaptureAudioWorker::FCaptureAudioWorker(FCorsairRGB* InCorsairRGB)
{
	CorsairRGB = InCorsairRGB;

	bStopCapture = false;
	
	bCapturingAudio = false;
	MMDevice = nullptr;
	AudioClient = nullptr;
	AudioCaptureClient = nullptr;
	WFX = nullptr;

	// This needs to be last or you get race conditions with ::Run()
	Thread = FRunnableThread::Create(this, TEXT("FCaptureAudioWorker"), 0, TPri_Highest);
}

FCaptureAudioWorker* FCaptureAudioWorker::Runnable = nullptr;

uint32 FCaptureAudioWorker::Run()
{
	CoInitialize(NULL);

	DWORD nTaskIndex = 0;
	HANDLE hTask = AvSetMmThreadCharacteristics(L"Capture", &nTaskIndex);

	InitAudioLoopback();

	ReadAudioLoopback();

	StopAudioLoopback();

	AvRevertMmThreadCharacteristics(hTask);

	CoUninitialize();

	return 0;
}

void FCaptureAudioWorker::InitAudioLoopback()
{
	// Based on http://blogs.msdn.com/b/matthew_van_eerde/archive/2014/11/05/draining-the-wasapi-capture-buffer-fully.aspx

	IMMDeviceEnumerator* DeviceEnumerator = nullptr;
	const CLSID CLSID_MMDeviceEnumeratorCorsair = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumeratorCorsair = __uuidof(IMMDeviceEnumerator);
	HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumeratorCorsair, NULL, CLSCTX_ALL, IID_IMMDeviceEnumeratorCorsair, (void**)&DeviceEnumerator);
	if (hr == S_OK)
	{
		hr = DeviceEnumerator->GetDefaultAudioEndpoint(EDataFlow::eRender, ERole::eConsole, &MMDevice);
		if (hr == S_OK && MMDevice != nullptr)
		{
			const IID IID_IAudioClientCorsair = __uuidof(IAudioClient);
			hr = MMDevice->Activate(IID_IAudioClientCorsair, CLSCTX_ALL, nullptr, (void**)&AudioClient);
			if (hr == S_OK && AudioClient)
			{
				hr = AudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, NULL);

				// this must be free'd with CoTaskMemFreeLater
				hr = AudioClient->GetMixFormat(&WFX);
				if (hr == S_OK)
				{
					// May need to create a silent audio stream to work around an issue from 2008, hopefully not anymore
					// https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/c7ba0a04-46ce-43ff-ad15-ce8932c00171/loopback-recording-causes-digital-stuttering?forum=windowspro-audiodevelopment

					// Adjust the desired WFX for capture to 16 bit PCM, not sure if necessary, makes writing to wave file for testing nice though
					if (WFX->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
					{
						WAVEFORMATEXTENSIBLE* WFEX = (WAVEFORMATEXTENSIBLE*)WFX;
						if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, WFEX->SubFormat))
						{
							WFEX->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
							WFX->wBitsPerSample = 16;
							WFEX->Samples.wValidBitsPerSample = WFX->wBitsPerSample;
							WFX->nBlockAlign = WFX->nChannels * WFX->wBitsPerSample / 8;
							WFX->nAvgBytesPerSec = WFX->nBlockAlign * WFX->nSamplesPerSec;
						}
					}
					else if (WFX->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
					{
						WFX->wFormatTag = WAVE_FORMAT_PCM;
						WFX->wBitsPerSample = 16;
						WFX->nBlockAlign = WFX->nChannels * WFX->wBitsPerSample / 8;
						WFX->nAvgBytesPerSec = WFX->nBlockAlign * WFX->nSamplesPerSec;
					}

					AudioBlockAlign = WFX->nBlockAlign;
					
					// nanoseconds, value taken from windows sample
					const int32 REFTIMES_PER_SEC = 10000000;

					// Audio capture
					REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
					hr = AudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, hnsRequestedDuration, 0, WFX, 0);
					if (hr == S_OK)
					{
						const IID IID_IAudioCaptureClientCorsair = __uuidof(IAudioCaptureClient);
						hr = AudioClient->GetService(IID_IAudioCaptureClientCorsair, (void**)&AudioCaptureClient);
						if (hr == S_OK && AudioCaptureClient)
						{
							hr = AudioClient->Start();
							if (hr == S_OK)
							{
								bCapturingAudio = true;
							}
						}
					}
				}
			}
		}
		DeviceEnumerator->Release();
		DeviceEnumerator = nullptr;
	}
}

void FCaptureAudioWorker::ReadAudioLoopback()
{
#if PLATFORM_64BITS
	// create a periodic waitable timer
	HANDLE hWakeUp = CreateWaitableTimer(NULL, FALSE, NULL);
	// set the waitable timer
	LARGE_INTEGER liFirstFire;
	liFirstFire.QuadPart = -hnsDefaultDevicePeriod / 2; // negative means relative time
	LONG lTimeBetweenFires = (LONG)hnsDefaultDevicePeriod / 2 / (10 * 1000); // convert to milliseconds
	BOOL bOK = SetWaitableTimer(hWakeUp, &liFirstFire, lTimeBetweenFires, NULL, NULL, FALSE);

	HANDLE waitArray[1] = { hWakeUp };
	DWORD dwWaitResult;

	const int32 FFTDimension = 1024;
	const int32 FrequencyDimension = FFTDimension / 2 + 1;
	kiss_fftr_cfg cfg = kiss_fftr_alloc(FFTDimension, 0, 0, 0);
	kiss_fft_scalar AudioBuffer[FFTDimension];
	kiss_fft_cpx FFTOutput[FrequencyDimension];
	float FrequencyMagnitudes[FrequencyDimension];

	int32 AudioBufferIter = 0;

	while (!bStopCapture && bCapturingAudio)
	{
		UINT32 nNextPacketSize;
		for (HRESULT hr = AudioCaptureClient->GetNextPacketSize(&nNextPacketSize); AudioBufferIter < FFTDimension && hr == S_OK && nNextPacketSize > 0; hr = AudioCaptureClient->GetNextPacketSize(&nNextPacketSize))
		{
			BYTE* Data;
			UINT32 NumFramesRead;
			DWORD Flags;
			UINT64 QPCPosition;
			HRESULT hrGetBuffer = AudioCaptureClient->GetBuffer(&Data, &NumFramesRead, &Flags, nullptr, &QPCPosition);
			if (hrGetBuffer == S_OK)
			{
				uint32 DataSizeInBytes = NumFramesRead * AudioBlockAlign;
					
				// Assuming 16 bit stereo or mono, no idea what to do in surround
				if (WFX->nChannels == 2)
				{
					for (UINT32 i = 0; i < NumFramesRead && AudioBufferIter < FFTDimension; i++)
					{
						short SoundValue = ((short*)Data)[i * 2] + ((short*)Data)[i * 2 + 1];
						AudioBuffer[AudioBufferIter] = SoundValue;
						AudioBufferIter++;
					}
				}
				else if (WFX->nChannels == 1)
				{
					for (UINT32 i = 0; i < NumFramesRead && AudioBufferIter < FFTDimension; i++)
					{
						short SoundValue = ((short*)Data)[i];
						AudioBuffer[AudioBufferIter] = SoundValue;
						AudioBufferIter++;
					}
				}
								
				HRESULT hrReleaseBuffer = AudioCaptureClient->ReleaseBuffer(NumFramesRead);
			}
		}

		if (AudioBufferIter == FFTDimension)
		{
			AudioBufferIter = 0;

			// remove average from the buffer
			if (1)
			{
				float Average = 0;
				for (int i = 0; i < FFTDimension; i++)
				{
					Average += AudioBuffer[i];
				}
				Average /= (float)FFTDimension;

				for (int i = 0; i < FFTDimension; i++)
				{
					AudioBuffer[i] -= Average;
				}
			}

			kiss_fftr(cfg, AudioBuffer, FFTOutput);

			for (int i = 0; i < FrequencyDimension; i++)
			{
				FrequencyMagnitudes[i] = FMath::Sqrt(FFTOutput[i].r * FFTOutput[i].r + FFTOutput[i].i * FFTOutput[i].i);
			}

			// Compute average value across frequency magnitudes to get the spectrum in 10 buckets
			const int32 TopEndToRemove = 0;
			const int32 SliceSize = (FrequencyDimension - TopEndToRemove) / NumSpectrums;
			for (int i = 0; i < NumSpectrums; i++)
			{
				float Average = 0;
				for (int j = 0; j < SliceSize; j++)
				{
					Average += FrequencyMagnitudes[i*SliceSize + j];
				}

				// Unsafe copy across threads, should be ok just in this instance
				CorsairRGB->SpectrumInfo[i] = 10.0f * FMath::LogX(10.0f, Average / SliceSize) - 10.0f;
			}
		}

		dwWaitResult = WaitForMultipleObjects(ARRAYSIZE(waitArray), waitArray, FALSE, INFINITE);
	}

	KISS_FFT_FREE(cfg);
#endif
}

void FCorsairRGB::CaptureAudio()
{
	AudioWorker = FCaptureAudioWorker::RunWorkerThread(this);
}

void FCorsairRGB::StopAudioCapture()
{
	if (AudioWorker)
	{
		AudioWorker->bStopCapture = true;
		AudioWorker = nullptr;
	}
}

void FCorsairRGB::UpdateKeyboardToSpectrum()
{
	TArray<CorsairLedColor> ColorsToSend;
	
	float Range1 = 15.0f;
	float Range2 = 30.0f;
	float Range3 = 40.0f;
	float Range4 = 45.0f;

	CorsairLedId LedIds[NumSpectrums][4];

	LedIds[0][0] = CLK_GraveAccentAndTilde;
	LedIds[0][1] = CLK_Tab;
	LedIds[0][2] = CLK_CapsLock;
	LedIds[0][3] = CLK_LeftShift;

	LedIds[1][0] = CLK_1;
	LedIds[1][1] = CLK_Q;
	LedIds[1][2] = CLK_A;
	LedIds[1][3] = CLK_Z;

	LedIds[2][0] = CLK_2;
	LedIds[2][1] = CLK_W;
	LedIds[2][2] = CLK_S;
	LedIds[2][3] = CLK_X;

	LedIds[3][0] = CLK_3;
	LedIds[3][1] = CLK_E;
	LedIds[3][2] = CLK_D;
	LedIds[3][3] = CLK_C;

	LedIds[4][0] = CLK_4;
	LedIds[4][1] = CLK_R;
	LedIds[4][2] = CLK_F;
	LedIds[4][3] = CLK_V;

	LedIds[5][0] = CLK_5;
	LedIds[5][1] = CLK_T;
	LedIds[5][2] = CLK_G;
	LedIds[5][3] = CLK_B;
	
	LedIds[6][0] = CLK_6;
	LedIds[6][1] = CLK_Y;
	LedIds[6][2] = CLK_H;
	LedIds[6][3] = CLK_N;

	LedIds[7][0] = CLK_7;
	LedIds[7][1] = CLK_U;
	LedIds[7][2] = CLK_J;
	LedIds[7][3] = CLK_M;

	LedIds[8][0] = CLK_8;
	LedIds[8][1] = CLK_I;
	LedIds[8][2] = CLK_K;
	LedIds[8][3] = CLK_CommaAndLessThan;

	LedIds[9][0] = CLK_9;
	LedIds[9][1] = CLK_O;
	LedIds[9][2] = CLK_L;
	LedIds[9][3] = CLK_PeriodAndBiggerThan;

	LedIds[10][0] = CLK_0;
	LedIds[10][1] = CLK_P;
	LedIds[10][2] = CLK_SemicolonAndColon;
	LedIds[10][3] = CLK_SlashAndQuestionMark;

	LedIds[11][0] = CLK_MinusAndUnderscore;
	LedIds[11][1] = CLK_BracketLeft;
	LedIds[11][2] = CLK_ApostropheAndDoubleQuote;
	LedIds[11][3] = CLK_RightShift;
	
	for (int i = 0; i < NumSpectrums; i++)
	{
		if (SpectrumInfo[i] > Range4)
		{
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][0], 255, 0, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][1], 0, 255, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][2], 255, 255, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][3], 255, 255, 255 });
		}
		else if (SpectrumInfo[i] > Range3)
		{
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][0], 0, 0, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][1], 0, 255, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][2], 255, 255, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][3], 255, 255, 255 });
		}
		else if (SpectrumInfo[i] > Range2)
		{
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][0], 0, 0, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][1], 0, 0, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][2], 255, 255, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][3], 255, 255, 255 });
		}
		else if (SpectrumInfo[i] > Range1)
		{
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][0], 0, 0, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][1], 0, 0, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][2], 0, 0, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][3], 255, 255, 255 });
		}
		else
		{
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][0], 0, 0, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][1], 0, 0, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][2], 0, 0, 0 });
			ColorsToSend.Add(CorsairLedColor{ LedIds[i][3], 0, 0, 0 });
		}
	}

	if (ColorsToSend.Num() > 0)
	{
		CorsairSetLedsColorsAsync(ColorsToSend.Num(), ColorsToSend.GetData(), nullptr, nullptr);
	}
}

void FCaptureAudioWorker::StopAudioLoopback()
{
	if (WFX)
	{
		// Should be done with WFX now
		CoTaskMemFree(WFX);
		WFX = nullptr;
	}

	if (AudioClient)
	{
		AudioClient->Stop();
	}

	if (AudioCaptureClient)
	{
		AudioCaptureClient->Release();
		AudioCaptureClient = nullptr;
	}

	if (AudioClient)
	{
		AudioClient->Release();
		AudioClient = nullptr;
	}

	if (MMDevice)
	{
		MMDevice->Release();
		MMDevice = nullptr;
	}
}