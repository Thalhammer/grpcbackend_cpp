#pragma once
#include "../attribute.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			class route_params : public attribute {
				std::map<std::string, std::string> params;
				std::string selected_route;
			public:
				virtual ~route_params();

				const std::string& get_selected_route() const;
				void set_selected_route(const std::string& route);

				bool has_param(const std::string& key) const;
				const std::string& get_param(const std::string& key) const;
				void set_param(const std::string& key, const std::string& val);
			};
		}
	}
}