#include "openvr.h"

#include <chrono>
#include <future>
#include <iomanip>
#include <iostream>

bool exitProcess()
{
	std::string line;

	std::getline(std::cin, line);

	return line.empty();
}

void print(const void* data, char* bytes, size_t size)
{
	memcpy(bytes, data, size);

	for(size_t i = 0; i < size; ++i)
	{
		std::cout << std::setfill('0') << std::setw(2) << (0xFF & bytes[i]);
	}
}

int main(int argc, const char* argv[])
{
	const unsigned int default_frequency = 10; // In milliseconds
	const size_t long_size = sizeof(long long);
	const size_t float_size = sizeof(float);

	char long_bytes[long_size];
	char float_bytes[float_size];

	int parameter_frequency = (argc > 1 ? atoi(argv[1]) : 0);

	unsigned int frequency = (parameter_frequency > 0 ? parameter_frequency : 0);

	vr::EVRInitError error;

	vr::IVRSystem* vr_system = vr::VR_Init(&error, vr::EVRApplicationType::VRApplication_Background);

	if(error == vr::EVRInitError::VRInitError_None)
	{
		vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];

		std::future<bool> future = std::async(exitProcess);

		std::cout << std::hex;

		while(future.wait_for(std::chrono::milliseconds(frequency)) != std::future_status::ready || !future.get())
		{
			const auto time = std::chrono::system_clock::now();

			vr_system->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseStanding, 0, poses, vr::k_unMaxTrackedDeviceCount);

			const unsigned long long timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();

			print(&timestamp, long_bytes, long_size);

			vr::HmdMatrix34_t matrix = poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;

			for(size_t i = 0; i < 3; ++i)
			{
				for(size_t j = 0; j < 4; ++j)
				{
					print(&matrix.m[i][j], float_bytes, float_size);
				}
			}

			std::cout << std::endl;
		}

		vr::VR_Shutdown();
	}

	return 0;
}
