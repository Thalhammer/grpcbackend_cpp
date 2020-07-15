#pragma once
#include <grpcbackend/http/connection.h>
#include <ttl/string_util.h>

struct dummy_connection : thalhammer::grpcbackend::http::connection {
	// Response info
	int status_code;
	std::string status_message;
	std::multimap<std::string, std::string> resp_headers;
	std::string output;

	// Request info
	std::string client_ip = "127.0.0.1";
	uint16_t client_port = 2374;
	std::string server_ip = "127.0.0.1";
	uint16_t server_port = 80;
	bool encrypted = false;
	std::string method = "GET";
	std::string resource = "/";
	std::string protocol = "HTTP/1.0";
	std::multimap<std::string, std::string> req_headers;
	std::string post_data;

	std::map<std::type_index, std::shared_ptr<thalhammer::grpcbackend::attribute>> attributes;

	enum {
		START,
		HEADERS_DONE,
		BODY_READ,
		FINISHED
	} state;


	// Request methods
	const std::string& get_client_ip() const { return client_ip; }
	uint16_t get_client_port() const { return client_port; }
	const std::string& get_server_ip() const { return server_ip; }
	uint16_t get_server_port() const { return server_port; }
	bool is_encrypted() const { return encrypted; }
	const std::string& get_method() const { return method; }
	const std::string& get_resource() const { return resource; }
	const std::string& get_protocol() const { return protocol; }
	const std::multimap<std::string, std::string>& get_headers() const { return req_headers; }

	// Geerbt über response
	void set_status(int code, const std::string & message = "") override {
		status_code = code;
		status_message = message;
	}

	void set_header(const std::string & key, const std::string & value, bool replace = false) override {
		auto k = ttl::string::to_lower_copy(key);
		if (replace)
			resp_headers.erase(k);
		resp_headers.insert({ k, value });
	}
	
	bool is_headers_done() const { return state >= HEADERS_DONE; }
	bool is_body_read() const { return state >= BODY_READ; }
	bool is_finished() const { return state >= FINISHED; }

	// TODO
	void read_body(std::function<void(std::shared_ptr<connection>, bool, std::string)> cb) {}
	void read_body_full(std::function<void(std::shared_ptr<connection>, bool, std::string)> cb) {}
	void skip_body(std::function<void(std::shared_ptr<connection>, bool)> cb) {}
	void send_body(std::string body, std::function<void(std::shared_ptr<connection>, bool)> cb, bool can_buffer=false) {}
	void send_body(std::istream& body, std::function<void(std::shared_ptr<connection>, bool)> cb) {}
	void end(std::function<void(std::shared_ptr<connection>, bool)> cb = [](std::shared_ptr<connection>, bool){}) {}
	void end(std::string body, std::function<void(std::shared_ptr<connection>, bool)> cb = [](std::shared_ptr<connection>, bool){}) {}


	std::map<std::type_index, std::shared_ptr<thalhammer::grpcbackend::attribute>>& get_attributes() { return attributes; }
	const std::map<std::type_index, std::shared_ptr<thalhammer::grpcbackend::attribute>>& get_attributes() const { return attributes; }


	static std::shared_ptr<dummy_connection> make_get(const std::string& uri, bool ssl = false) {
		auto res = std::make_shared<dummy_connection>();
		res->server_port = ssl ? 443 : 80;
		res->encrypted = ssl;
		res->method = "GET";
		res->resource = uri;
		return res;
	}

	static std::shared_ptr<dummy_connection> make_post(const std::string& uri, const std::string& data, bool ssl = false) {
		auto res = std::make_shared<dummy_connection>();
		res->server_port = ssl ? 443 : 80;
		res->encrypted = ssl;
		res->method = "POST";
		res->resource = uri;
		res->post_data = data;
		return res;
	}

};