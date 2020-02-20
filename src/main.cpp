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
	std::condition_variable new_message;

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
		std::unique_lock<std::mutex> lk(mtx);

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
		new_message.notify_all();
	}

	void signal() {
		std::unique_lock<std::mutex> lk(mtx);
		ss.str("");
		ss.clear();
		new_message.notify_all();
	}

	void print(std::ostream& stream) {
		std::unique_lock<std::mutex> lk(mtx);
		new_message.wait(lk);
		stream << ss.str();
		stream.flush();
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

void CollectMessage(Message* msg, unsigned int frequency) {
	vr::EVRInitError error;

	vr::IVRSystem* vr_system = vr::VR_Init(&error, vr::EVRApplicationType::VRApplication_Background);

	if (error == vr::EVRInitError::VRInitError_None)
	{
		vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];

		std::future<bool> future = std::async(exitProcess);

		while (future.wait_for(std::chrono::milliseconds(frequency)) != std::future_status::ready || !future.get())
		{
			const auto time = std::chrono::system_clock::now();

			vr_system->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseStanding, 0, poses, vr::k_unMaxTrackedDeviceCount);

			const unsigned long long timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();
			vr::HmdMatrix34_t matrix = poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;

			msg->set(timestamp, matrix);
		}

		vr::VR_Shutdown();
	}
}


class CoutMessagePrinter {
	Message& msg;
	bool stop;
public:
	CoutMessagePrinter(Message& message) : msg(message), stop(false) {}

	void setStop() {
		stop = true;
	}

	void run() {
		while (!stop) {
			msg.print(std::cout);
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

	// create collect
	std::thread th_collect(CollectMessage, &msg, frequency);

	// create consumers
	CoutMessagePrinter printer(msg);
	std::thread th_print(&CoutMessagePrinter::run, &printer);

	// Wait collect thread finished
	th_collect.join();

	// Stop consumers
	printer.setStop();

	// Signal message to free consumers
	msg.signal();

	// join consumers
	th_print.join();

	return 0;
}
