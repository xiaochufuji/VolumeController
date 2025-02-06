#include "VolumeController.h"
#include <conio.h>
#include <fstream>
int main()
{

	xiaochufuji::VolumeController vc;
	auto session = vc.getAllAudioSessions();
	std::ofstream ofs("./1.txt");
	for (auto ite : x)
	{
		std::cout << "process id: " << ite << std::endl;
		
	}
	auto devices = vc.getAllAudioDevices();
	for (auto ite : devices)
	{
		std::cout << ite << std::endl;
		ofs.write(ite.c_str(), ite.length());
	}
	int processId = 19208;	// your audio process id get from the pre-loop
	if (devices.size() < 1) return 0;
	std::cout << "control: " << devices[0] << std::endl;
	xiaochufuji::VolumeController::setDeviceVolume(devices[0], 7);
	xiaochufuji::VolumeController::muteDeviceVolume(devices[0]);
	xiaochufuji::VolumeController::triggerMuteDeviceVolume(devices[0]);
	xiaochufuji::VolumeController::adjustDeviceVolume(devices[0], -1);

	xiaochufuji::VolumeController::setSessionVolume(processId, 10);
	xiaochufuji::VolumeController::muteSessionVolume(processId);
	xiaochufuji::VolumeController::triggerMuteSessionVolume(processId);
	xiaochufuji::VolumeController::adjustSessionVolume(processId, 50);
	std::string currentDevice = devices[0];
	while (true) {
		char ch = _getch(); 
		if (ch > 0 && ch < 224) { 
			switch (ch) {
			case 'w': 
				xiaochufuji::VolumeController::adjustSessionVolume(processId, 1);
				break;
			case 's': 
				xiaochufuji::VolumeController::adjustSessionVolume(processId, -1);
				break;
			case 'a':
				xiaochufuji::VolumeController::triggerMuteSessionVolume(processId);
				break;
			case 'd':
				xiaochufuji::VolumeController::setSessionVolume(processId, 50);
				break;
			case 72:
				xiaochufuji::VolumeController::adjustDeviceVolume(currentDevice, 1);
				break;
			case 80:
				xiaochufuji::VolumeController::adjustDeviceVolume(currentDevice, -1);
				break;
			case 75:
				xiaochufuji::VolumeController::triggerMuteDeviceVolume(currentDevice);
				break;
			case 77:
				xiaochufuji::VolumeController::setDeviceVolume(currentDevice, 50);
				break;
			case '0':
				if (devices.empty()) break;
				currentDevice = devices[0];
				xiaochufuji::VolumeController::switchDevice(devices[0]);
				break;
			case '1':
				if (devices.size() < 2) break;
				currentDevice = devices[1];
				xiaochufuji::VolumeController::switchDevice(devices[1]);
				break;
			case '2':
				if (devices.size() < 3) break;
				currentDevice = devices[2];
				xiaochufuji::VolumeController::switchDevice(devices[2]);
				break;
			default:
				std::cout << " other key pressed: " << (int)ch << std::endl;
				break;
			}
			std::cout << "current device volume: " << xiaochufuji::VolumeController::getDeviceVolume(currentDevice) << "    \t";
			std::cout << "current session volume: " <<  xiaochufuji::VolumeController::getSessionVolume(processId) <<  "    \r";

		}
	}
}