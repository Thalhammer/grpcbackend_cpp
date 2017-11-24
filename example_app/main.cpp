#include "server.h"

using namespace thalhammer::grpcbackend;

int main(int argc, const char** argv) try {
	std::cout.sync_with_stdio(true);
	server mserver;

	class wshandler : public websocket::con_handler {
		server& _s;
	public:
		wshandler(server& s) : _s(s) {}
		virtual void on_connect(websocket::connection_ptr con) override
		{}
		virtual void on_message(websocket::connection_ptr con, bool bin, const std::string& msg) override {
			_s.get_logger()(thalhammer::loglevel::INFO, "ws") << "Message" << msg;
			_s.get_wshub().broadcast(con, bin, msg);
			//con->send_message(bin, msg);
		}
		virtual void on_disconnect(websocket::connection_ptr con) override {
			_s.get_logger()(thalhammer::loglevel::INFO, "ws") << "Closed connection from " << con->get_client_ip() << ":" << con->get_client_port();
		}
	};
	class defaulthandler : public websocket::con_handler {
		server& _s;
	public:
		defaulthandler(server& s) : _s(s) {}
		virtual void on_connect(websocket::connection_ptr con) override
		{
			_s.get_wshub().set_connection_group(con, "default");
		}
		virtual void on_message(websocket::connection_ptr con, bool bin, const std::string& msg) override {
		}
		virtual void on_disconnect(websocket::connection_ptr con) override {
		}
	};

	mserver.get_logger().set_loglevel(thalhammer::loglevel::TRACE);
	mserver.get_router()
		.set_debug_mode(true)
		.log_requests(mserver.get_logger())
		.mime_detector()
		.serve_dir("/", "./public/", true);
	mserver.get_wshub()
		.set_default_handler(std::make_shared<defaulthandler>(mserver))
		.add_group("default", std::make_shared<wshandler>(mserver));

	mserver.get_builder().AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());

	mserver.start_server();

	std::string line;
	while (std::getline(std::cin, line)) {
		if (line == "quit" || line == "shutdown" || line == "q")
			break;
	}

	mserver.shutdown_server();

	return 0;
}
catch (const std::exception& e) {
	std::cerr << "Fatal error:" << e.what() << std::endl;
	return -1;
}