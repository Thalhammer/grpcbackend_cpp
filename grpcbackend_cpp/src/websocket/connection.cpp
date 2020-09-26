#include <grpcbackend/websocket/connection.h>
#include <ttl/string_util.h>

namespace thalhammer {
	namespace grpcbackend {
		namespace websocket {
            const uri& connection::get_parsed_uri() const {
				if (!_parsed_uri) {
					_parsed_uri.emplace(get_resource());
				}
				return _parsed_uri.value();
			}

			bool connection::has_header(const std::string& key) const {
				return get_headers().count(ttl::string::to_lower_copy(key)) != 0;
			}

			const std::string& connection::get_header(const std::string& key) const {
				static const std::string empty = "";
				auto k = ttl::string::to_lower_copy(key);
				auto& headers = get_headers();
				if (headers.count(k) != 0)
					return headers.find(k)->second;
				else return empty;
			}
		}
	}
}