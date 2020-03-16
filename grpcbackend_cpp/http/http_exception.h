#pragma once
#include <stdexcept>

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			class http_exception : public std::runtime_error {
				int code;
				std::string message;
				public:
				http_exception(int c, std::string msg) 
					: std::runtime_error(std::to_string(c) + " " + msg), code(c), message(std::move(msg))
				{}

				int get_code() const { return code; }
				const std::string& get_message() const { return message; }
			};
			class notfound_exception : public http_exception {
				public:
				notfound_exception() 
					: http_exception(404, "Not found")
				{}
			};
			class servererror_exception : public http_exception {
				public:
				servererror_exception() 
					: http_exception(500, "Servererror")
				{}
			};
		}
	}
}