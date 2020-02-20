#include "openvr.h"
#include "sockpp/tcp_acceptor.h"

#include <chrono>
#include <future>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <list>

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

	std::string wait_and_getcopy() {
		std::unique_lock<std::mutex> lk(mtx);
		new_message.wait(lk);
		return ss.str();
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


class MessageProvider {
protected:
	Message& msg;
	bool stop;
public:
	MessageProvider(Message& message) : msg(message), stop(false) {}
	virtual ~MessageProvider() {}

	virtual void setStop() {
		stop = true;
	}

	virtual void run() = 0;
};


class CoutMessagePrinter: public MessageProvider {
public:
	CoutMessagePrinter(Message& message) : MessageProvider(message) {}
	virtual ~CoutMessagePrinter() {}

	virtual void run() final {
		while (!stop) {
			std::string message = msg.wait_and_getcopy();
			std::cout << message;
			std::cout.flush();
		}
	}
};


class ServerMessageProvider: public MessageProvider {
	int socket_port = 10000;
	sockpp::tcp_acceptor acc;

public:
	ServerMessageProvider(Message& message, int port): MessageProvider(message), socket_port(port), acc(port) {}
	virtual ~ServerMessageProvider() {}

	virtual void setStop() final {
		MessageProvider::setStop();
		acc.close();
		acc.destroy();
	}

	virtual void run() final {
		if (!acc) {
			std::cerr << "Error creating the acceptor: " << acc.last_error_str() << std::endl;
			return;
		}
		std::cout << "Awaiting connections on port " << socket_port << "..." << std::endl;

		while (!stop) {
			sockpp::inet_address peer;
			sockpp::tcp_socket sock = acc.accept(&peer);
			if (!stop) {
				std::cout << "Received a connection request from " << peer << std::endl;
				if (!sock) {
					std::cerr << "Error accepting incoming connection: " << acc.last_error_str() << std::endl;
				}
				else {
					std::thread thr(&ServerMessageProvider::run_write_message, this, std::move(sock));
					thr.detach();
				}
			}
		}
	}

private:
	void run_write_message(sockpp::tcp_socket sock) {
		while (!stop) {
			std::string message = msg.wait_and_getcopy();
			ssize_t sz = sock.write(message);
			if (sz != message.size()) {
				std::cerr << sock.last_error_str() << std::endl;
				break;
			}
		}
	}
};


int main(int argc, const char* argv[])
{
	sockpp::socket_initializer sockInit;

	const unsigned int default_frequency = 10; // In milliseconds
	Message msg;

	int parameter_frequency = (argc > 1 ? atoi(argv[1]) : 0);
	int socket_port = (argc > 2 ? atoi(argv[2]) : 0);

	unsigned int frequency = (parameter_frequency > 0 ? parameter_frequency : 0);

	// create collect
	std::thread th_collect(CollectMessage, &msg, frequency);

	MessageProvider* provider = nullptr;

	if (socket_port == 0) {
		provider = new CoutMessagePrinter(msg);
	} else {
		provider = new ServerMessageProvider(msg, socket_port);
	}

	std::thread th_print(&MessageProvider::run, provider);

	// Wait collect thread finished
	th_collect.join();

	// Stop consumers
	provider->setStop();

	// Signal message to free consumers
	msg.signal();

	// join consumers
	th_print.join();

	return 0;
}
