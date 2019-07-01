// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Delegate.h"
#include "Runtime/Core/Public/Features/IModularFeature.h"

DECLARE_MULTICAST_DELEGATE(FVoiceChatReconnected);
DECLARE_MULTICAST_DELEGATE(FVoiceChatChannelJoinFailed);

class UTVoiceChatFeature : public IModularFeature
{
public:
	virtual void Connect() = 0;
	virtual void Disconnect() = 0;

	virtual void LoginUsingToken(const FString& PlayerName, const FString& Token) = 0;
	virtual void Logout(const FString& PlayerName) = 0;
	virtual void JoinChannelUsingToken(const FString& PlayerName, const FString& Channel, const FString& Token) = 0;
	virtual void LeaveChannel(const FString& PlayerName, const FString& Channel) = 0;

	virtual FDelegateHandle RegisterReconnectedDelegate_Handle(const FVoiceChatReconnected::FDelegate& Delegate) = 0;
	virtual void UnregisterReconnectedDelegate_Handle(FDelegateHandle Handle) = 0;

	virtual FDelegateHandle RegisterChannelJoinFailedDelegate_Handle(const FVoiceChatChannelJoinFailed::FDelegate& Delegate) = 0;
	virtual void UnregisterChannelJoinFailedDelegate_Handle(FDelegateHandle Handle) = 0;

	virtual void SetPlaybackVolume(float InVolume) = 0;
	virtual void SetRecordVolume(float InVolume) = 0;

	virtual void SetAudioInputDeviceMuted(bool bIsMuted) = 0;

	virtual void MutePlayer(const FString& PlayerName) = 0;
	virtual void UnMutePlayer(const FString& PlayerName) = 0;
	virtual bool IsPlayerMuted(const FString& PlayerName) = 0;

	virtual bool IsUsingCustomInputDevice() = 0;
	virtual bool IsUsingCustomOutputDevice() = 0;

	virtual void GetAvailableCustomInputDevices(TArray<FString>& CustomInputDevices) = 0;
	virtual void GetAvailableCustomOutputDevices(TArray<FString>& CustomOutputDevices) = 0;

	virtual void SetCustomInputDevice(const FString& CustomInputDevice) = 0;
	virtual void SetCustomOutputDevice(const FString& CustomOutputDevice) = 0;

	virtual bool IsConnected() = 0;
	virtual bool IsLoggedIn() = 0;
	virtual FString GetChannelName() = 0;
};