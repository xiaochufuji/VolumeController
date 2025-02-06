#include "VolumeController.h"
#include <windows.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h> 
#include <cmath>
#include "PolicyConfig.h"
// Kahan 
static float kahanAdd(float a, float b) {
	static float c = 0.0f;
	float y = b - c;
	float t = a + y;
	c = (t - a) - y;  
	return t; 
}

xiaochufuji::VolumeController::VolumeController()
{
	std::locale::global(std::locale("zh_CN.UTF-8"));
	HRESULT hr = CoInitialize(nullptr);
	if (FAILED(hr)) return;
}

xiaochufuji::VolumeController::~VolumeController()
{
	clearDevice();
	clearSession();
	CoUninitialize();
}
std::vector<std::string> xiaochufuji::VolumeController::getAllAudioDevices()
{
	clearDevice();

	CComPtr<IMMDeviceEnumerator> deviceEnumerator;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&deviceEnumerator));
	if (FAILED(hr)) return {};

	CComPtr<IMMDeviceCollection> deviceCollection;
	hr = deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
	if (FAILED(hr)) return {};

	UINT deviceCount = 0;
	hr = deviceCollection->GetCount(&deviceCount);
	if (FAILED(hr)) return {};

	for (UINT i = 0; i < deviceCount; ++i) {
		CComPtr<IMMDevice> device;
		hr = deviceCollection->Item(i, &device);
		if (SUCCEEDED(hr)) {
			// get device's friendly name
			CComPtr<IPropertyStore> propertyStore;
			hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
			std::wstring deviceName;
			if (SUCCEEDED(hr)) {
				PROPVARIANT varName;
				PropVariantInit(&varName);
				if (SUCCEEDED(propertyStore->GetValue(PKEY_Device_FriendlyName, &varName))) {
					deviceName = varName.pwszVal;
					PropVariantClear(&varName);
				}
			}
			m_audioDevices.emplace_back(WStringToString(deviceName));
		}
	}
	return m_audioDevices;
}

int xiaochufuji::VolumeController::getDeviceVolume(const std::string& deviceName)
{
	auto callback = [](const CComPtr<IMMDevice>& device, VolumeStucture& info, bool cancelMute) ->bool {
		CComPtr<IAudioEndpointVolume> pEndpointVolume;
		HRESULT hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pEndpointVolume);
		if (FAILED(hr)) return false;

		float getValue;
		hr = pEndpointVolume->GetMasterVolumeLevelScalar(&getValue);
		if (SUCCEEDED(hr))
		{
			info.get = static_cast<UINT32>(std::round(getValue * 100));
			return true;
		}
		return false;
		};
	VolumeStucture vs{};
	vs.get = 0;
	if (findDevice(deviceName, callback, vs)) return vs.get;
	return  -1;
}

BOOL xiaochufuji::VolumeController::getDeviceMute(const std::string& deviceName)
{
	auto callback = [](const CComPtr<IMMDevice>& device, VolumeStucture& info, bool cancelMute) ->bool {
		CComPtr<IAudioEndpointVolume> pEndpointVolume;
		HRESULT hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pEndpointVolume);
		if (FAILED(hr)) return false;

		BOOL isMute{ -1 };
		hr = pEndpointVolume->GetMute(&isMute);
		if (SUCCEEDED(hr))
		{
			info.isMute = isMute;
			return true;
		}
		return false;
		};
	VolumeStucture vs{};
	vs.isMute =  -1 ;
	if (findDevice(deviceName, callback, vs)) return vs.isMute;
	return  -1;
}

bool xiaochufuji::VolumeController::setDeviceVolume(const std::string& deviceName, UINT32 volumeLevel, bool cancelMute)
{
	auto callback = [](const CComPtr<IMMDevice>& device, VolumeStucture& info, bool cancelMute) ->bool {
		CComPtr<IAudioEndpointVolume> pEndpointVolume;
		HRESULT hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pEndpointVolume);
		if (FAILED(hr)) return false;

		const float epsilon = 1e-4f;
		float setValue = static_cast<float>(info.set) / 100;
		if (setValue < 0.0f + epsilon) setValue = 0.0f;
		if (setValue > 1.0f - epsilon) setValue = 1.0f;
		if (cancelMute) hr = pEndpointVolume->SetMute(FALSE, NULL);
		if (SUCCEEDED(hr))
		{
			hr = pEndpointVolume->SetMasterVolumeLevelScalar(setValue, NULL);
			if (SUCCEEDED(hr)) return true;
		}
		return false;
		};
	VolumeStucture vs{};
	vs.set = volumeLevel;
	return findDevice(deviceName, callback, vs);
}

bool xiaochufuji::VolumeController::adjustDeviceVolume(const std::string& deviceName, UINT32 volumeLevel, bool cancelMute)
{
	auto callback = [](const CComPtr<IMMDevice>& device, VolumeStucture& info, bool cancelMute) ->bool {

		CComPtr<IAudioEndpointVolume> pEndpointVolume;
		HRESULT hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pEndpointVolume);
		if (FAILED(hr)) return false;

		const float epsilon = 1e-4f;
		do
		{
			float currentValue = -1.0f;
			hr = pEndpointVolume->GetMasterVolumeLevelScalar(&currentValue);
			if (FAILED(hr)) break;

			float newValue = kahanAdd(static_cast<float>(info.adjust) / 100, currentValue);
			if (newValue < 0.0f + epsilon) newValue = 0.0f;
			if (newValue > 1.0f - epsilon) newValue = 1.0f;
			if (cancelMute) hr = pEndpointVolume->SetMute(FALSE, NULL);
			if (SUCCEEDED(hr))
			{
				hr = pEndpointVolume->SetMasterVolumeLevelScalar(newValue, NULL);
				if (SUCCEEDED(hr)) return true;
			}
			return false;
		} while (0);
		return false;
		};
	VolumeStucture vs{};
	vs.adjust = volumeLevel;
	return findDevice(deviceName, callback, vs);
}

bool xiaochufuji::VolumeController::muteDeviceVolume(const std::string& deviceName, bool isMute)
{
	auto callback = [](const CComPtr<IMMDevice>& device, VolumeStucture& info, bool cancelMute) ->bool {
		CComPtr<IAudioEndpointVolume> pEndpointVolume;
		HRESULT hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pEndpointVolume);
		if (FAILED(hr)) return false;

		hr = pEndpointVolume->SetMute(info.isMute, NULL);
		if (SUCCEEDED(hr)) return true;
		return false;
		};
	VolumeStucture vs{};
	vs.isMute = isMute ? TRUE : FALSE;
	return findDevice(deviceName, callback, vs);
}

bool xiaochufuji::VolumeController::triggerMuteDeviceVolume(const std::string& deviceName)
{
	auto callback = [](const CComPtr<IMMDevice>& device, VolumeStucture& info, bool cancelMute) ->bool {
		CComPtr<IAudioEndpointVolume> pEndpointVolume;
		HRESULT hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pEndpointVolume);
		if (FAILED(hr)) return false;

		do
		{
			BOOL isMute{ -1 };
			hr = pEndpointVolume->GetMute(&isMute);
			if (FAILED(hr)) break;
			hr = pEndpointVolume->SetMute(!isMute, NULL);
			if (FAILED(hr)) break;
			return true;
		} while (0);
		return false;
		};
	VolumeStucture vs{};
	return findDevice(deviceName, callback, vs);
}

std::vector<DWORD> xiaochufuji::VolumeController::getAllAudioSessions()
{
	clearSession();

	// get device enumerator
	CComPtr<IMMDeviceEnumerator> deviceEnumerator;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&deviceEnumerator));
	if (FAILED(hr))	return {};

	// enumerate all device
	CComPtr<IMMDeviceCollection> deviceCollection;
	hr = deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
	if (FAILED(hr))	return {};

	// calculate the device's count
	UINT deviceCount = 0;
	hr = deviceCollection->GetCount(&deviceCount);
	if (FAILED(hr))	return {};

	// traverse all device, to get all session
	for (UINT i = 0; i < deviceCount; ++i) {
		CComPtr<IMMDevice> device;
		hr = deviceCollection->Item(i, &device);
		if (FAILED(hr))	continue;

		// get device's friendly name
		CComPtr<IPropertyStore> propertyStore;
		hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
		std::wstring deviceName;
		if (SUCCEEDED(hr)) {
			PROPVARIANT varName;
			PropVariantInit(&varName);
			if (SUCCEEDED(propertyStore->GetValue(PKEY_Device_FriendlyName, &varName))) {
				deviceName = varName.pwszVal;
				PropVariantClear(&varName);
			}
		}

		// get the session manager under which device
		CComPtr<IAudioSessionManager2> sessionManager;
		hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&sessionManager);
		if (FAILED(hr))	continue;

		// get the audio session enumerator
		CComPtr<IAudioSessionEnumerator> sessionEnumerator = nullptr;
		hr = sessionManager->GetSessionEnumerator(&sessionEnumerator);
		if (FAILED(hr))	continue;
		m_audioSessionEnumeratorList.emplace_back(std::make_pair(WStringToString(deviceName), sessionEnumerator));
	}
	enumerateAudioSession();
	return set2Vector();
}

int xiaochufuji::VolumeController::getSessionVolume(DWORD processId)
{
	auto callback = [](const CComPtr<ISimpleAudioVolume>& session, VolumeStucture& info, bool cancelMute) ->bool {
		CComPtr<ISimpleAudioVolume> pAudioVolume;
		HRESULT hr = session->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
		if (FAILED(hr)) return false;

		float getValue;
		hr = pAudioVolume->GetMasterVolume(&getValue);
		if (SUCCEEDED(hr))
		{
			info.get = static_cast<UINT32>(std::round(getValue * 100));
			return true;
		}
		return false;
		};
	VolumeStucture vs{};
	vs.get = 0;
	if (findSession(processId, callback, vs)) return vs.get;
	return  -1;
}

BOOL xiaochufuji::VolumeController::getSessionMute(DWORD processId)
{
	auto callback = [](const CComPtr<ISimpleAudioVolume>& session, VolumeStucture& info, bool cancelMute) ->bool {
		CComPtr<ISimpleAudioVolume> pAudioVolume;
		HRESULT hr = session->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
		if (FAILED(hr)) return false;

		BOOL isMute{ -1 };
		hr = pAudioVolume->GetMute(&isMute);
		if (SUCCEEDED(hr))
		{
			info.isMute = isMute;
			return true;
		}
		return false;
		};
	VolumeStucture vs{};
	vs.isMute = -1;
	if (findSession(processId, callback, vs)) return vs.isMute;
	return  -1;
}

bool xiaochufuji::VolumeController::setSessionVolume(DWORD processId, UINT32 volumeLevel, bool cancelMute)
{
	auto callback = [](const CComPtr<ISimpleAudioVolume>& session, VolumeStucture& info, bool cancelMute) ->bool {
		CComPtr<ISimpleAudioVolume> pAudioVolume;
		HRESULT hr = session->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
		if (FAILED(hr)) return false;

		const float epsilon = 1e-4f;
		float setValue = static_cast<float>(info.set) / 100;
		if (setValue < 0.0f + epsilon) setValue = 0.0f;
		if (setValue > 1.0f - epsilon) setValue = 1.0f;
		if (cancelMute) hr = pAudioVolume->SetMute(FALSE, NULL);
		if (SUCCEEDED(hr))
		{
			hr = pAudioVolume->SetMasterVolume(setValue, NULL);
			if (SUCCEEDED(hr)) return true;
		}
		return false;
		};
	VolumeStucture vs{};
	vs.set = volumeLevel;
	return findSession(processId, callback, vs);
}

bool xiaochufuji::VolumeController::adjustSessionVolume(DWORD processId, UINT32 volumeLevel, bool cancelMute)
{
	auto callback = [](const CComPtr<ISimpleAudioVolume>& session, VolumeStucture& info, bool cancelMute) ->bool {
		CComPtr<ISimpleAudioVolume> pAudioVolume;
		HRESULT hr = session->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
		if (FAILED(hr)) return false;

		const float epsilon = 1e-4f;
		do
		{
			float currentValue = -1.0f;
			hr = pAudioVolume->GetMasterVolume(&currentValue);
			if (FAILED(hr)) break;

			float newValue = kahanAdd(static_cast<float>(info.adjust) / 100, currentValue);
			if (newValue < 0.0f + epsilon) newValue = 0.0f;
			if (newValue > 1.0f - epsilon) newValue = 1.0f;
			if (cancelMute) hr = pAudioVolume->SetMute(FALSE, NULL);
			if (SUCCEEDED(hr))
			{
				hr = pAudioVolume->SetMasterVolume(newValue, NULL);
				if (SUCCEEDED(hr)) return true;
			}
			return false;
		} while (0);
		return false;
		};
	VolumeStucture vs{};
	vs.adjust = volumeLevel;
	return findSession(processId, callback, vs);
}

bool xiaochufuji::VolumeController::muteSessionVolume(DWORD processId, bool isMute)
{
	auto callback = [](const CComPtr<ISimpleAudioVolume>& session, VolumeStucture& info, bool cancelMute) ->bool {
		CComPtr<ISimpleAudioVolume> pAudioVolume;
		HRESULT hr = session->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
		if (FAILED(hr)) return false;

		hr = pAudioVolume->SetMute(info.isMute, NULL);
		if (SUCCEEDED(hr)) return true;
		return false;
		};
	VolumeStucture vs{};
	vs.isMute = isMute ? TRUE : FALSE;
	return findSession(processId, callback, vs);
}

bool xiaochufuji::VolumeController::triggerMuteSessionVolume(DWORD processId)
{
	auto callback = [](const CComPtr<ISimpleAudioVolume>& session, VolumeStucture& info, bool cancelMute) ->bool {
		CComPtr<ISimpleAudioVolume> pAudioVolume;
		HRESULT hr = session->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
		if (FAILED(hr)) return false;

		do
		{
			BOOL isMute{ FALSE };
			hr = pAudioVolume->GetMute(&isMute);
			if (FAILED(hr)) break;
			hr = pAudioVolume->SetMute(!isMute, NULL);
			if (FAILED(hr)) break;
			return true;
		} while (0);
		return false;
		};
	VolumeStucture vs{};
	return findSession(processId, callback, vs);
}

bool xiaochufuji::VolumeController::switchDevice(const std::string& deviceName)
{
	CComPtr<IMMDeviceEnumerator> deviceEnumerator = nullptr;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
	if (FAILED(hr)) return false;

	CComPtr<IMMDeviceCollection> deviceCollection = nullptr;
	hr = deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
	if (FAILED(hr)) return false;

	UINT count;
	deviceCollection->GetCount(&count);

	for (UINT i = 0; i < count; i++) {
		CComPtr<IMMDevice> device = nullptr;
		hr = deviceCollection->Item(i, &device);
		if (FAILED(hr)) continue;

		LPWSTR deviceId = nullptr;
		device->GetId(&deviceId);
		CComPtr<IPropertyStore> propertyStore;
		device->OpenPropertyStore(STGM_READ, &propertyStore);

		PROPVARIANT nameProp;
		PropVariantInit(&nameProp);
		hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &nameProp);

		if (SUCCEEDED(hr)) {
			if (WStringToString(nameProp.pwszVal) == deviceName)
			{
				setDefaultAudioPlaybackDevice(deviceId);
			}
		}
		PropVariantClear(&nameProp);
		CoTaskMemFree(deviceId);
	}
}

std::vector<DWORD> xiaochufuji::VolumeController::set2Vector()
{
	std::vector<DWORD> retVal;
	for (const auto& ite : m_audioSessions)
	{
		retVal.emplace_back(ite);
	}
	return retVal;
}

void xiaochufuji::VolumeController::clearDevice()
{
	m_audioDevices.clear();
}

void xiaochufuji::VolumeController::clearSession()
{
	m_audioSessions.clear();m_deviceSessionsMap.clear();m_audioSessionEnumeratorList.clear();
}

void xiaochufuji::VolumeController::enumerateAudioSession()
{
	for (const auto& enumerator : m_audioSessionEnumeratorList)
	{
		auto sessionEnumerator = enumerator.second;
		HRESULT hr;
		int sessionCount = 0;
		hr = sessionEnumerator->GetCount(&sessionCount);
		if (FAILED(hr)) {
			continue;
		}
		for (int j = 0; j < sessionCount; ++j) {
			CComPtr<IAudioSessionControl> sessionControl;
			hr = sessionEnumerator->GetSession(j, &sessionControl);
			if (FAILED(hr)) {
				continue;
			}

			// get the seesion's process id
			CComPtr<IAudioSessionControl2> sessionControl2;
			hr = sessionControl->QueryInterface(IID_PPV_ARGS(&sessionControl2));
			if (SUCCEEDED(hr)) {
				DWORD processId = 0;
				hr = sessionControl2->GetProcessId(&processId);
				if (SUCCEEDED(hr)) {
					m_audioSessions.insert(processId);
					m_deviceSessionsMap[enumerator.first].emplace_back(processId);
				}
			}
		}
	}
}

bool xiaochufuji::VolumeController::findDevice(const std::string& findDeviceName, DeviceCallbackType callback, VolumeStucture& info, bool cancelMute)
{
	CComPtr<IMMDeviceEnumerator> deviceEnumerator;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&deviceEnumerator));
	if (FAILED(hr)) return {};

	CComPtr<IMMDeviceCollection> deviceCollection;
	hr = deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
	if (FAILED(hr)) return {};

	UINT deviceCount = 0;
	hr = deviceCollection->GetCount(&deviceCount);
	if (FAILED(hr)) return {};

	for (UINT i = 0; i < deviceCount; ++i) {
		CComPtr<IMMDevice> device;
		hr = deviceCollection->Item(i, &device);
		if (SUCCEEDED(hr)) {
			// get device's friendly name
			CComPtr<IPropertyStore> propertyStore;
			hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
			std::wstring deviceName;
			if (SUCCEEDED(hr)) {
				PROPVARIANT varName;
				PropVariantInit(&varName);
				if (SUCCEEDED(propertyStore->GetValue(PKEY_Device_FriendlyName, &varName))) 
				{
					deviceName = varName.pwszVal;
					PropVariantClear(&varName);
					if (findDeviceName == WStringToString(deviceName))
						return callback(device, info, cancelMute);
				}
			}
		}
	}
	return false;
}

bool xiaochufuji::VolumeController::findSession(DWORD processId, SessionCallbackType callback, VolumeStucture& info, bool cancelMute)
{
	CComPtr<IMMDeviceEnumerator> enumerator;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
	if (FAILED(hr)) return false;

	CComPtr<IMMDeviceCollection> deviceCollection;
	hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
	if (FAILED(hr)) return false;

	UINT deviceCount = 0;
	hr = deviceCollection->GetCount(&deviceCount);
	if (FAILED(hr)) return false;

	bool globalFlag = false;
	for (UINT i = 0; i < deviceCount; ++i) {
		CComPtr<IMMDevice> device;
		hr = deviceCollection->Item(i, &device);
		if (FAILED(hr)) return false;

		CComPtr<IAudioSessionManager2> audioSessionManager;
		hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&audioSessionManager);
		if (FAILED(hr)) return false;

		CComPtr<IAudioSessionEnumerator> sessionEnumerator;
		hr = audioSessionManager->GetSessionEnumerator(&sessionEnumerator);
		if (FAILED(hr)) continue;

		int sessionCount = 0;
		hr = sessionEnumerator->GetCount(&sessionCount);
		if (FAILED(hr)) continue;

		for (int j = 0; j < sessionCount; ++j) {
			CComPtr<IAudioSessionControl> sessionControl;
			hr = sessionEnumerator->GetSession(j, &sessionControl);
			if (FAILED(hr)) continue;

			CComPtr<IAudioSessionControl2> sessionControl2;
			hr = sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2);
			if (FAILED(hr)) continue;

			DWORD sessionProcessId = 0;
			hr = sessionControl2->GetProcessId(&sessionProcessId);
			if (FAILED(hr)) continue;

			if (sessionProcessId == processId) {
				CComPtr<ISimpleAudioVolume> audioVolume;
				hr = sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&audioVolume);
				if (FAILED(hr)) continue;
				if(callback(audioVolume, info, cancelMute)) globalFlag = true;	// one modify success is global success
			}
		}
	}
	return globalFlag;
}

bool xiaochufuji::VolumeController::setDefaultAudioPlaybackDevice(LPCWSTR deviceId)
{
	CComPtr<IPolicyConfigVista> pPolicyConfig;
	ERole reserved = eConsole;

	HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient),
		NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID*)&pPolicyConfig);
	if (SUCCEEDED(hr))
	{
		hr = pPolicyConfig->SetDefaultEndpoint(deviceId, reserved);
	}
	return hr;
}
