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
	bool provide_all;
	vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];

	std::stringstream ss;
	const size_t long_size = sizeof(long long);
	const size_t float_size = sizeof(float);
	char* long_bytes;
	char* float_bytes;
	std::mutex mtx;
	std::condition_variable new_message;

public:
	Message(bool all) : provide_all(all) {
		ss << std::hex;
		long_bytes = new char[long_size];
		float_bytes = new char[float_size];
	}

	~Message() {
		delete[] long_bytes;
		delete[] float_bytes;
	}

	void set(vr::IVRSystem* vr_system) {
		// get system time
		const auto time = std::chrono::system_clock::now();

		// collect data
		vr_system->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseStanding, 0, poses, vr::k_unMaxTrackedDeviceCount);

		// extract timestamp
		const unsigned long long timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();

		// extract head pose
		vr::HmdMatrix34_t head_matrix = poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
		std::vector<vr::TrackedDeviceIndex_t> positions;
		std::vector<std::tuple<unsigned long long, unsigned long long>> buttons;
		std::vector<std::string> ids;

		if (provide_all) {
			for (vr::TrackedDeviceIndex_t idx = 1; idx < vr::k_unMaxTrackedDeviceCount; ++idx) {
				if (vr_system->IsTrackedDeviceConnected(idx) && poses[idx].bPoseIsValid) {
					char buffer[1024];
					vr_system->GetStringTrackedDeviceProperty(idx, vr::ETrackedDeviceProperty::Prop_SerialNumber_String, buffer, 1024);
					std::string serial(buffer);

					vr::ETrackedDeviceClass trackedDeviceClass = vr_system->GetTrackedDeviceClass(idx);
					std::string device_class;
					if (trackedDeviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller) {
						vr::ETrackedControllerRole role = vr_system->GetControllerRoleForTrackedDeviceIndex(idx);
						if (role == vr::ETrackedControllerRole::TrackedControllerRole_LeftHand) {
							device_class = "CL";
						} else if(role == vr::ETrackedControllerRole::TrackedControllerRole_RightHand) {
							device_class = "CR";
						} else {
							device_class = "C?";
						}
					} else if (trackedDeviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker) {
						device_class = "T";
					}

					vr::VRControllerState_t controllerState;
					unsigned long long button_pressed = 0, button_touched = 0;
					if (vr_system->GetControllerState(idx, &controllerState, sizeof(controllerState))) {
						button_pressed = controllerState.ulButtonPressed;
						button_touched = controllerState.ulButtonTouched;
					}

					vr_system->GetStringTrackedDeviceProperty(idx, vr::ETrackedDeviceProperty::Prop_ControllerType_String, buffer, 1024);
					std::string type(buffer);

					if (type != "" && device_class != "") {
						positions.push_back(idx);
						ids.push_back("[" + device_class + "#" + type + "#" + serial + "]");
						buttons.push_back(std::make_tuple(button_pressed, button_touched));
					}
				}
			}
		}

		// Start section with stringstream locked
		{
			std::unique_lock<std::mutex> lk(mtx);

			//Reset buffer content
			ss.str("");
			ss.clear();

			//Construct message
			print_to_stream(&timestamp, long_bytes, long_size);
			print_matrix_to_stream(head_matrix);
			for (int i = 0; i < ids.size(); ++i) {
				//Info
				ss << ids[i];
				unsigned long long but;
				//press
				but = std::get<0>(buttons[i]);
				print_to_stream(&but, long_bytes, long_size);
				//touch
				but = std::get<1>(buttons[i]);
				print_to_stream(&but, long_bytes, long_size);
				//matrix
				vr::TrackedDeviceIndex_t idx = positions[i];
				vr::HmdMatrix34_t matrix = poses[idx].mDeviceToAbsoluteTracking;
				print_matrix_to_stream(matrix);
			}

			ss << std::endl;
			new_message.notify_all();
		}
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
	void print_matrix_to_stream(vr::HmdMatrix34_t& matrix) {
		for (size_t i = 0; i < 3; ++i) {
			for (size_t j = 0; j < 4; ++j) {
				print_to_stream(&matrix.m[i][j], float_bytes, float_size);
			}
		}
	}

	void print_to_stream(const void* data, char* bytes, size_t size)
	{
		memcpy(bytes, data, size);

		for (size_t i = 0; i < size; ++i)
		{
			ss << std::setfill('0') << std::setw(2) << (0xFF & bytes[i]);
		}
	}
};


void CollectMessage(Message* msg, unsigned int frequency) {
	vr::EVRInitError error;

	vr::IVRSystem* vr_system = vr::VR_Init(&error, vr::EVRApplicationType::VRApplication_Background);

	if (error == vr::EVRInitError::VRInitError_None)
	{
		std::future<bool> future = std::async(exitProcess);

		while (future.wait_for(std::chrono::milliseconds(frequency)) != std::future_status::ready || !future.get())
		{
			msg->set(vr_system);
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

	int parameter_frequency = (argc > 1 ? atoi(argv[1]) : 0);
	int socket_port = 0;
	bool all = false;
	if (argc > 2) {
		for (int i = 2; i < argc; ++i) {
			std::string arg(argv[i]);
			if (arg.length() > 0 && arg[0] == '-') {
				if (arg == "-all") { all = true; }
			} else {
				socket_port = atoi(argv[i]);
			}
		}
	}

	unsigned int frequency = (parameter_frequency > default_frequency ? parameter_frequency : default_frequency);

	// create collect
	Message msg(all);
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
