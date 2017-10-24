#pragma once
#include "../request.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
            /* Helper class to override some methods of a request object while forwarding all the others */
			class request_forward: public request {
                request& orig;
			public:
				request_forward(request& porig)
					: orig(porig)
				{}
                // Clientinfo
				virtual const std::string& get_client_ip() const { return orig.get_client_ip(); }
				virtual uint16_t get_client_port() const { return orig.get_client_port(); }

				// Serverinfo
				virtual const std::string& get_server_ip() const { return orig.get_server_ip(); }
				virtual uint16_t get_server_port() const { return orig.get_server_port(); }
				virtual bool is_encrypted() const { return orig.is_encrypted(); }

				// Request info
				virtual const std::string& get_method() const { return orig.get_method(); }
				virtual const std::string& get_resource() const { return orig.get_resource(); }
				virtual const std::string& get_protocol() const { return orig.get_protocol(); }
				virtual const std::multimap<std::string, std::string>& get_headers() const { return orig.get_headers(); }
				virtual std::istream& get_istream() { return orig.get_istream(); }

				const uri& get_parsed_uri() { return orig.get_parsed_uri(); }

				bool is_get() const { return orig.is_get(); }

				bool is_post() const { return orig.is_post(); }

				bool has_header(const std::string& key) const { return orig.has_header(key); }

				const std::string& get_header(const std::string& key) const { return orig.get_header(key); }

				template<typename T>
				typename std::enable_if<std::is_base_of<attribute, T>::value, void>::type
					set_attribute(std::shared_ptr<T> attr) {
					orig.set_attribute<T>(attr);
				}

				template<typename T>
				typename std::enable_if<std::is_base_of<attribute, T>::value, bool>::type
					has_attribute() const { return orig.has_attribute<T>(); }

				template<typename T>
				typename std::enable_if<std::is_base_of<attribute, T>::value, std::shared_ptr<T>>::type
					get_attribute() const { return orig.get_attribute<T>(); }

				const std::map<std::type_index, std::shared_ptr<attribute>>& get_attributes() const { return orig.get_attributes(); }
			};
		}
	}
}