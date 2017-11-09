#pragma once
#include <exception>

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			class notfound_exception : public std::runtime_error {
				public:
				notfound_exception() 
					: std::runtime_error("Not found")
				{}
			};
		}
	}
}