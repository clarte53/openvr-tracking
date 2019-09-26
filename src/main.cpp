#include "openvr.h"

#include <future>
#include <iostream>

bool exitProcess()
{
	std::string line;

	std::getline(std::cin, line);

	return line.empty();
}

int main(int argc, const char* argv[])
{
	const unsigned int default_frequency = 10; // In milliseconds

	int parameter_frequency = (argc > 1 ? atoi(argv[1]) : 0);

	unsigned int frequency = (parameter_frequency > 0 ? parameter_frequency : 0);

	vr::EVRInitError error;

	vr::IVRSystem* vr_system = vr::VR_Init(&error, vr::EVRApplicationType::VRApplication_Background);

	if(error == vr::EVRInitError::VRInitError_None)
	{
		vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];

		std::future<bool> future = std::async(exitProcess);

		while(future.wait_for(std::chrono::milliseconds(frequency)) != std::future_status::ready || !future.get())
		{
			vr_system->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseStanding, 0, poses, vr::k_unMaxTrackedDeviceCount);

			vr::HmdMatrix34_t matrix = poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;

			std::cout << matrix.m[0][0] << std::endl;
		}

		vr::VR_Shutdown();
	}

	return 0;
}
