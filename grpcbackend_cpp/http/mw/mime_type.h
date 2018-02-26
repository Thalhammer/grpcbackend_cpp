#pragma once
#include "../mime.h"
#include "../middleware.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			namespace mw {
				class mime_type : public middleware {
					mime _mime;
				public:
					explicit mime_type(const std::string& filename = "mime.types");
					explicit mime_type(std::istream& type_stream);
					// Geerbt über middleware
					virtual void handle_request(request & req, response & resp, std::function<void(request&, response&)>& next) override;
				};
			}

		}
	}
}