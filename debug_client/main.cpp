#include <grpc++/grpc++.h>
#include "handler.grpc.pb.h"
#include <thread>
#include <atomic>

void testHTTPRequest(std::unique_ptr<thalhammer::http::Handler::Stub>& stub) {
	::grpc::ClientContext context;
	auto stream = stub->Handle(&context);
	{
		::thalhammer::http::HandleRequest req;
		auto* client = req.mutable_client();
		client->set_local_port(80);
		client->set_local_ip("127.0.0.1");
		client->set_remote_port((uint16_t)rand());
		client->set_remote_ip("127.0.0.1");
		client->set_encrypted(true);
		auto* request = req.mutable_request();
		request->set_method("GET");
		request->set_protocol("HTTP/1.1");
		request->set_resource("/");
		
		for(int i = 0; i < 4; i++) {
			auto* header = request->add_headers();
			header->set_key("Header-" +std::to_string(i));
			header->set_value("Content " + std::to_string(i));
		}

		if(!stream->Write(req)) {
			std::cerr << "Failed to write initial packet";
			exit(-1);
		}
	}

	/*bool failed = false;
	read_body([&stream, &failed](const char* data, size_t len){
		::thalhammer::http::HandleRequest req;
		req.mutable_request()->set_content(data, len);
		if(!stream->Write(req))
			failed = true;
		return !failed;
	}, r);*/

	if(!stream->WritesDone()) {
		std::cerr << "Failed to finish writing" << std::endl;
		exit(-1);
	}

	::thalhammer::http::HandleResponse resp;
	if(!stream->Read(&resp)) {
		std::cerr << "Failed to read initial response" << std::endl;
		exit(-1);
	} else {
		auto& response = resp.response();
		std::cout << response.status_code() << " " << response.status_message() << std::endl;
		for(auto& header : response.headers()) {
			std::cout << header.key() << ": " << header.value() << std::endl;
		}

		std::cout << std::endl << std::endl;
		if(!response.content().empty())
		{
			auto& content = response.content();
			std::cout << content << std::endl;
		}
	}

	while(stream->Read(&resp)) {
		auto& content = resp.response().content();
		std::cout << content << std::endl;
	}

	::grpc::Status s = stream->Finish();
	if(!s.ok()) {
		std::cerr << "Failed to execute rpc: " << s.error_message() << std::endl;
		exit(-1);
	}
}

void testWSRequest(std::unique_ptr<thalhammer::http::Handler::Stub>& stub) {
	::grpc::ClientContext context;
	auto stream = stub->HandleWebSocket(&context);

	{
		::thalhammer::http::HandleWebSocketRequest req;
		auto* client = req.mutable_request()->mutable_client();
		client->set_local_port(80);
		client->set_local_ip("127.0.0.1");
		client->set_remote_port((uint16_t)rand());
		client->set_remote_ip("127.0.0.1");
		client->set_encrypted(true);
		auto* request = req.mutable_request();
		request->set_method("GET");
		request->set_protocol("HTTP/1.1");
		request->set_resource("/");
		
		for(int i = 0; i < 4; i++) {
			auto* header = request->add_headers();
			header->set_key("Header-" +std::to_string(i));
			header->set_value("Content " + std::to_string(i));
		}

		if(!stream->Write(req)) {
			std::cerr << "Failed to write initial packet";
			exit(-1);
		}
	}

	{
		::thalhammer::http::HandleWebSocketResponse resp;
		if(!stream->Read(&resp)) {
			std::cerr << "Failed to read initial response" << std::endl;
			exit(-1);
		} else {
			auto& response = resp.response();
			for(auto& header : response.headers()) {
				std::cout << header.key() << ": " << header.value() << std::endl;
			}
		}
	}

	std::atomic<bool> recv_shutdown(false);
	auto recv_thread = std::thread([&](){
		try {
			::thalhammer::http::HandleWebSocketResponse resp;
			while(!recv_shutdown && stream->Read(&resp)) {
				auto& msg = resp.message();
				switch(msg.type()) {
					case ::thalhammer::http::WebSocketMessage::TEXT:
						std::cout<<"Textmsg:" << msg.content() << std::endl; break;
					case ::thalhammer::http::WebSocketMessage::BINARY:
						std::cout<<"Binmsg: " << msg.content().size() << " bytes" << std::endl; break;
					case ::thalhammer::http::WebSocketMessage::CLOSE:
						recv_shutdown = true;
						std::cout << "Close received" << std::endl;
						break;
					default:
						break;
				}
			}
		} catch(...) {
			std::cerr <<  "Exception in read thread" << std::endl;
		}
	});

	recv_thread.join();
	::grpc::Status s = stream->Finish();
	if(!s.ok()) {
		std::cerr << "Failed to execute rpc: " << s.error_message() << std::endl;
		exit(-1);
	}
}

int main() {
	auto channel = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
	auto stub = ::thalhammer::http::Handler::NewStub(channel);

	testHTTPRequest(stub);
	testWSRequest(stub);
}