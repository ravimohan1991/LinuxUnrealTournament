// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#undef TEXT
#undef ERROR_SUCCESS
#undef ERROR_IO_PENDING
#undef E_NOTIMPL
#undef E_FAIL

#include "CUESDK.h"
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmsystem.h>
#include <Avrt.h>

#include "Core.h"
#include "UnrealTournament.h"

#include "CorsairRGB.generated.h"

const int NumSpectrums = 12;

UCLASS(Blueprintable, Meta = (ChildCanTick))
class ACorsairRGB : public AActor
{
	GENERATED_UCLASS_BODY()
	
};

class FCaptureAudioWorker : public FRunnable
{
	FCaptureAudioWorker(struct FCorsairRGB* InCorsairRGB);

	~FCaptureAudioWorker()
	{
		delete Thread;
		Thread = nullptr;
	}

	uint32 Run();

	void InitAudioLoopback();
	void StopAudioLoopback();
	void ReadAudioLoopback();

private:
	IMMDevice* MMDevice;
	IAudioClient* AudioClient;
	IAudioCaptureClient* AudioCaptureClient;
	REFERENCE_TIME hnsDefaultDevicePeriod;
	WAVEFORMATEX* WFX;
	bool bCapturingAudio;
	int32 AudioBlockAlign;

	FRunnableThread* Thread;
	static FCaptureAudioWorker* Runnable;
	struct FCorsairRGB* CorsairRGB;

public:
	bool bStopCapture;

	void WaitForCompletion()
	{
		Thread->WaitForCompletion();
	}

	static FCaptureAudioWorker* RunWorkerThread(struct FCorsairRGB* InCorsairRGB)
	{
		if (Runnable)
		{
			delete Runnable;
			Runnable = nullptr;
		}

		Runnable = new FCaptureAudioWorker(InCorsairRGB);

		return Runnable;
	}
};

struct FCorsairRGB : FTickableGameObject, IModuleInterface
{
	FCorsairRGB();
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableInEditor() const override { return true; }
	virtual bool IsTickableWhenPaused() const override { return true; }

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	// Put a real stat id here
	virtual TStatId GetStatId() const
	{
		return TStatId();
	}

	float DeltaTimeAccumulator;
	float FrameTimeMinimum;
	int FlashSpeed;

	uint64 LastFrameCounter;

	bool bIsFlashingForEnd;
	bool bInitialized;
	bool bCorsairSDKEnabled;
	
	bool bPlayingIdleColors;
	bool bStopCapture;

	float SpectrumInfo[NumSpectrums];

	FCaptureAudioWorker* AudioWorker;
	void CaptureAudio();
	void StopAudioCapture();
	void UpdateKeyboardToSpectrum();

	void UpdateIdleColors(float DeltaTime);
};