# grpcbackend_cpp
Provides a library to help developing a service for mod_grpcbackend.
It makes webservices in C++ dead simple and supports every protocol provided by apache.

A simple example:
```c++
#include "server.h"

using namespace thalhammer::grpcbackend;

int main(int argc, const char** argv) {
	std::cout.sync_with_stdio(true);
	server mserver;

	mserver.get_logger().set_loglevel(thalhammer::loglevel::TRACE);
	mserver.get_router()
		.set_debug_mode(true)
		.log_requests(mserver.get_logger())
		.mime_detector()
		.serve_dir("/", "./public/", true);

	mserver.get_builder().AddListeningPort("0.0.0.0:50051", ::grpc::InsecureServerCredentials());
	mserver.start_server();

	std::string line;
	while (std::getline(std::cin, line)) {
		if (line == "quit" || line == "shutdown" || line == "q")
			break;
	}

	mserver.shutdown_server();

	return 0;
}
```
