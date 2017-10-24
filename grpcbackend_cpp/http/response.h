#pragma once
#include <string>
#include <map>

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			class response {
			public:
				virtual void set_status(int code, const std::string& message = "") = 0;
				virtual void set_header(const std::string& key, const std::string& value, bool replace = false) = 0;
				virtual std::ostream& get_ostream() = 0;
			};
		}
	}
}