#pragma once
#include <http/response.h>
#include <ttl/string_util.h>

struct dummy_response : thalhammer::grpcbackend::http::response {
	int status_code;
	std::string status_message;
	std::multimap<std::string, std::string> headers;
	std::ostringstream output;

	// Geerbt über response
	virtual void set_status(int code, const std::string & message = "") override {
		status_code = code;
		status_message = message;
	}

	virtual void set_header(const std::string & key, const std::string & value, bool replace = false) override {
		auto k = thalhammer::string::to_lower_copy(key);
		if (replace)
			headers.erase(k);
		headers.insert({ k, value });
	}

	virtual std::ostream & get_ostream() override {
		return output;
	}
};