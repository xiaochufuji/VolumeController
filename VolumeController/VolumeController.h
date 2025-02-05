#pragma once
#include "StringAdapter.h"
#include <vector>
#include <map>
#include <shlobj.h>
#include <atlbase.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <iostream>
#include <set>
namespace xiaochufuji
{
	union VolumeStucture {
		unsigned int get;
		unsigned int set;
		int adjust;
		BOOL isMute;
	};
	class VolumeController
	{
		using DeviceCallbackType = bool(*)(const CComPtr<IMMDevice>& device, VolumeStucture& info, bool cancelMute);
		using SessionCallbackType = bool(*)(const CComPtr<ISimpleAudioVolume> &session, VolumeStucture& info, bool cancelMute);
	public:
		explicit VolumeController();
		~VolumeController();
	public:
		// device control
		std::vector<std::string> getAllAudioDevices();
		static int getDeviceVolume(const std::string& deviceName);
		static BOOL getDeviceMute(const std::string& deviceName);
		static bool setDeviceVolume(const std::string& deviceName, UINT32 volumeLevel, bool cancelMute = true);
		static bool adjustDeviceVolume(const std::string& deviceName, UINT32 volumeLevel, bool cancelMute = true);
		static bool muteDeviceVolume(const std::string& deviceName, bool isMute = true);
		static bool triggerMuteDeviceVolume(const std::string& deviceName);
		// session control
		std::vector<DWORD> getAllAudioSessions();
		static int getSessionVolume(DWORD processId);
		static BOOL getSessionMute(DWORD processId);
		static bool setSessionVolume(DWORD processId, UINT32 volumeLevel, bool cancelMute = true);
		static bool adjustSessionVolume(DWORD processId, UINT32 volumeLevel, bool cancelMute = true);
		static bool muteSessionVolume(DWORD processId, bool isMute = true);
		static bool triggerMuteSessionVolume(DWORD processId);
	private:
		std::vector<DWORD> set2Vector();
		void clearDevice();
		void clearSession();
		void enumerateAudioSession();
		static bool findDevice(const std::string& findDeviceName, DeviceCallbackType callback, VolumeStucture& info, bool cancelMute = true);
		static bool findSession(DWORD processId, SessionCallbackType callback, VolumeStucture& info, bool cancelMute = true);

	private:
		// device
		std::vector<std::string> m_audioDevices;
		// session
		std::set<DWORD> m_audioSessions;																		// All audio session's process id
		std::map<std::string, std::vector<DWORD>> m_deviceSessionsMap;											// Audio which device is managing. { deviceName --> session list }
		std::vector<std::pair<std::string, CComPtr<IAudioSessionEnumerator>>> m_audioSessionEnumeratorList;		// it store all device's name and device's enumerator that can get it's all sessions
	};
}