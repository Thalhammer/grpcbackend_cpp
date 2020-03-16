#pragma once
#include "../attribute.h"
#include "router.h"
#include <map>
#include <string>

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			class route_params : public attribute {
				std::map<std::string, std::string> params;
				std::shared_ptr<const router::routing_info> info;
				std::shared_ptr<const router::route_entry> entry;
				friend class router;
			public:
				const std::string& get_selected_route() const;

				bool has_param(const std::string& key) const;
				const std::string& get_param(const std::string& key) const;
			};
		}
	}
}