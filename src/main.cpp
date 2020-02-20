#include "openvr.h"
#include "sockpp/acceptor.h"

#include <chrono>
#include <future>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

bool exitProcess()
{
	std::string line;

	std::getline(std::cin, line);

	return line.empty();
}

class Message {
	std::stringstream ss;
	const size_t long_size = sizeof(long long);
	const size_t float_size = sizeof(float);
	char* long_bytes;
	char* float_bytes;
	std::mutex mtx;

public:
	Message() {
		ss << std::hex;
		long_bytes = new char[long_size];
		float_bytes = new char[float_size];
	}

	~Message() {
		delete[] long_bytes;
		delete[] float_bytes;
	}

	void set(unsigned long long timestamp, vr::HmdMatrix34_t& matrix) {
		std::lock_guard<std::mutex> lk(mtx);

		//Reset buffer content
		ss.str("");
		ss.clear();
		
		//Construct message
		print_to_stream(&timestamp, long_bytes, long_size, ss);
		for (size_t i = 0; i < 3; ++i)
		{
			for (size_t j = 0; j < 4; ++j)
			{
				print_to_stream(&matrix.m[i][j], float_bytes, float_size, ss);
			}
		}
		ss<<std::endl;
	}

	void print() {
		std::lock_guard<std::mutex> lk(mtx);
		std::cout << ss.str();
		std::cout.flush();
	}

private:
	static void print_to_stream(const void* data, char* bytes, size_t size, std::ostream& stream)
	{
		memcpy(bytes, data, size);

		for (size_t i = 0; i < size; ++i)
		{
			stream << std::setfill('0') << std::setw(2) << (0xFF & bytes[i]);
		}
	}
};

int main(int argc, const char* argv[])
{
	const unsigned int default_frequency = 10; // In milliseconds
	Message msg;

	int parameter_frequency = (argc > 1 ? atoi(argv[1]) : 0);
	int socket_port = (argc > 2 ? atoi(argv[2]) : 0);

	unsigned int frequency = (parameter_frequency > 0 ? parameter_frequency : 0);

	vr::EVRInitError error;

	vr::IVRSystem* vr_system = vr::VR_Init(&error, vr::EVRApplicationType::VRApplication_Background);

	if(error == vr::EVRInitError::VRInitError_None)
	{
		vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];

		std::future<bool> future = std::async(exitProcess);

		while(future.wait_for(std::chrono::milliseconds(frequency)) != std::future_status::ready || !future.get())
		{
			const auto time = std::chrono::system_clock::now();

			vr_system->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseStanding, 0, poses, vr::k_unMaxTrackedDeviceCount);

			const unsigned long long timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();
			vr::HmdMatrix34_t matrix = poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;

			msg.set(timestamp, matrix);
			msg.print();
		}

		vr::VR_Shutdown();
	}

	return 0;
}
