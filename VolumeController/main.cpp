#include "VolumeController.h"
#include <conio.h>
int main()
{

	xiaochufuji::VolumeController vc;
	auto x = vc.getAllAudioSessions();
	for (auto ite : x)
	{
		std::cout << ite << std::endl;
	}
	auto a = vc.getAllAudioDevices();
	for (auto ite : a)
	{
		std::cout << ite << std::endl;
	}
	int processId = 3292;	// your audio process id

	vc.setDeviceVolume(a[0], 60);
	vc.triggerMuteDeviceVolume(a[0]);
	vc.adjustDeviceVolume(a[0], -10);

	vc.setSessionVolume(processId, 10);
	vc.triggerMuteSessionVolume(processId);
	vc.adjustSessionVolume(processId, 50);

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
			default:
				std::cout << " other key pressed: " << (int)ch << std::endl;
				break;
			}
			std::cout << "key pressed: " << (int)ch << std::endl;
		}
		else {
			std::cout << "Non-arrow key pressed: " << (int)ch << std::endl;
		}

	}
}