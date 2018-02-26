#pragma once
#include "../response.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
            /* Helper class to override some methods of a response object while forwarding all the others */
			class response_forward: public response {
                response& orig;
            public:
                explicit response_forward(response& o) : orig(o) {}
                response& get_original_response() const { return orig; }
				virtual void set_status(int code, const std::string& message = "") { orig.set_status(code, message); }
				virtual void set_header(const std::string& key, const std::string& value, bool replace = false) { orig.set_header(key, value, replace); }
				virtual std::ostream& get_ostream() { return orig.get_ostream(); }
			};
		}
	}
}