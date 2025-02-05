#include "VolumeController.h"
#include <conio.h>
int main()
{

	xiaochufuji::VolumeController vc;
	auto x = vc.getAllAudioSessions();
	for (auto ite : x)
	{
		std::cout << "process id: " << ite << std::endl;
	}
	auto a = vc.getAllAudioDevices();
	for (auto ite : a)
	{
		std::cout << ite << std::endl;
	}
	int processId = 19208;	// your audio process id
	if (a.size() < 1) return 0;
	std::cout << a[0] << std::endl;
	xiaochufuji::VolumeController::setDeviceVolume(a[0], 7);
	xiaochufuji::VolumeController::muteDeviceVolume(a[0]);
	xiaochufuji::VolumeController::triggerMuteDeviceVolume(a[0]);
	xiaochufuji::VolumeController::adjustDeviceVolume(a[0], -1);

	xiaochufuji::VolumeController::setSessionVolume(processId, 10);
	xiaochufuji::VolumeController::muteSessionVolume(processId);
	xiaochufuji::VolumeController::triggerMuteSessionVolume(processId);
	xiaochufuji::VolumeController::adjustSessionVolume(processId, 50);

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
			std::cout << "current volume: " <<  xiaochufuji::VolumeController::getSessionVolume(processId) << std::endl;
		}
		else {
			std::cout << "Non-arrow key pressed: " << (int)ch << std::endl;
		}
	}
}