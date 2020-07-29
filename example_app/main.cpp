#include <grpcbackend/server.h>
#include <grpcbackend/http/router.h>
#include <grpcbackend/websocket/hub.h>
#include <grpcbackend/filesystem.h>
#include <grpcbackend/util/json_rpc.h>
#include <grpc++/grpc++.h>
#include <unistd.h>
#include <ttl/module.h>

#include "out/rpc_api.proto.h"

using namespace thalhammer::grpcbackend;

int main(int argc, const char** argv) try {
	(void)argc; (void) argv;
	std::cout.sync_with_stdio(true);
	server mserver;

	class wshandler : public websocket::con_handler, public test::TestService {
		server& _s;
		util::json_rpc _rpc;
	public:
		wshandler(server& s) : _s(s) {
			(void)s;
			this->register_service(_rpc);
		}
		void on_connect(websocket::connection_ptr con) override
		{ (void)con; }
		void on_message(websocket::connection_ptr con, bool bin, const std::string& msg) override {
			(void)bin;
			_s.get_logger()(ttl::loglevel::INFO, "ws") << "Message " << msg;
			_rpc.handle_message(msg, [con](const std::string& str){ con->send_message(false, str); });
		}
		void on_disconnect(websocket::connection_ptr con) override {
			_s.get_logger()(ttl::loglevel::INFO, "ws") << "Closed connection from " << con->get_client_ip() << ":" << con->get_client_port();
		}

		void Test(std::shared_ptr<thalhammer::grpcbackend::util::json_rpc_query<test::TestRequest,test::TestResponse>> context) override {
			test::TestResponse resp;
			resp.response = context->get_request().request;
			return context->respond(resp);
		}
	};
	class defaulthandler : public websocket::con_handler {
		server& _s;
	public:
		defaulthandler(server& s) : _s(s) {}
		virtual void on_connect(websocket::connection_ptr con) override
		{
			_s.get_wshub().set_connection_group(con, "default");
			_s.get_logger()(ttl::loglevel::INFO, "ws") << "Opened connection from " << con->get_client_ip() << ":" << con->get_client_port();
		}
		virtual void on_message(websocket::connection_ptr con, bool bin, const std::string& msg) override {
			(void)con; (void)bin; (void)msg;
		}
		virtual void on_disconnect(websocket::connection_ptr con) override {
			(void)con;
		}
	};

	mserver.get_logger().set_loglevel(ttl::loglevel::TRACE);
	mserver.get_router()
		.set_debug_mode(true)
		.log_requests(mserver.get_logger())
		.mime_detector()
		.serve_dir("/", "./public/");
		//.serve_dir("/", "./public/", true, std::make_shared<zipfilesystem>(thalhammer::module::current_module().get_filename()));
	mserver.get_wshub()
		.set_default_handler(std::make_shared<defaulthandler>(mserver))
		.add_group("default", std::make_shared<wshandler>(mserver));

	mserver.get_builder().AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());

	mserver.start_server();

	mserver.get_logger()(ttl::loglevel::INFO, "main") << "Init done!";

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