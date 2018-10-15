#pragma once
#include <http/request.h>

struct dummy_request : public thalhammer::grpcbackend::http::request {
	std::string client_ip = "127.0.0.1";
	uint16_t client_port = 2374;
	std::string server_ip = "127.0.0.1";
	uint16_t server_port = 80;
	bool encrypted = false;
	std::string method = "GET";
	std::string resource = "/";
	std::string protocol = "HTTP/1.0";
	std::multimap<std::string, std::string> headers;
	std::string post_data;
	std::unique_ptr<std::istringstream> post_body;

	static std::unique_ptr<dummy_request> make_get(const std::string& uri, bool ssl = false) {
		auto res = std::make_unique<dummy_request>();
		res->server_port = ssl ? 443 : 80;
		res->encrypted = ssl;
		res->method = "GET";
		res->resource = uri;
		return res;
	}

	static std::unique_ptr<dummy_request> make_post(const std::string& uri, const std::string& data, bool ssl = false) {
		auto res = std::make_unique<dummy_request>();
		res->server_port = ssl ? 443 : 80;
		res->encrypted = ssl;
		res->method = "POST";
		res->resource = uri;
		res->post_data = data;
		return res;
	}

	// Geerbt �ber request
	virtual const std::string & get_client_ip() const override
	{
		return client_ip;
	}
	virtual uint16_t get_client_port() const override
	{
		return client_port;
	}
	virtual const std::string & get_server_ip() const override
	{
		return server_ip;
	}
	virtual uint16_t get_server_port() const override
	{
		return server_port;
	}
	virtual bool is_encrypted() const override
	{
		return encrypted;
	}
	virtual const std::string & get_method() const override
	{
		return method;
	}
	virtual const std::string & get_resource() const override
	{
		return resource;
	}
	virtual const std::string & get_protocol() const override
	{
		return protocol;
	}
	virtual const std::multimap<std::string, std::string>& get_headers() const override
	{
		return headers;
	}
	virtual std::istream & get_istream() override
	{
		if (post_body)
			return *post_body;
		post_body = std::make_unique<std::istringstream>(post_data);
		return *post_body;
	}
};