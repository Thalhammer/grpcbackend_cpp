#pragma once
#include "../attribute.h"
#include <memory>
#include <map>
#include <typeindex>
#include <string>
#include "../uri.h"
#include <ttl/string_util.h>
#include <functional>

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			class connection
			{
				std::unique_ptr<uri> _parsed_uri;
			protected:
			public:
				virtual ~connection() {}

				// Clientinfo
				virtual const std::string& get_client_ip() const = 0;
				virtual uint16_t get_client_port() const = 0;

				// Serverinfo
				virtual const std::string& get_server_ip() const = 0;
				virtual uint16_t get_server_port() const = 0;
				virtual bool is_encrypted() const = 0;

				// Request info
				virtual const std::string& get_method() const = 0;
				virtual const std::string& get_resource() const = 0;
				virtual const std::string& get_protocol() const = 0;
				virtual const std::multimap<std::string, std::string>& get_headers() const = 0;

				// Only valid until first write
				virtual void set_status(int code, const std::string& message = "") = 0;
				virtual void set_header(const std::string& key, const std::string& value, bool replace = false) = 0;

				virtual bool is_headers_done() const = 0;
				virtual bool is_body_read() const = 0;
				virtual bool is_finished() const = 0;

				virtual void read_body(std::function<void(std::shared_ptr<connection>, bool, std::string)> cb) = 0;
				virtual void read_body_full(std::function<void(std::shared_ptr<connection>, bool, std::string)> cb) = 0;
				virtual void skip_body(std::function<void(std::shared_ptr<connection>, bool)> cb) = 0;
				virtual void send_body(std::string body, std::function<void(std::shared_ptr<connection>, bool)> cb, bool can_buffer=false) = 0;
				virtual void send_body(std::istream& body, std::function<void(std::shared_ptr<connection>, bool)> cb) = 0;
				virtual void end(std::function<void(std::shared_ptr<connection>, bool)> cb = [](std::shared_ptr<connection>, bool){}) = 0;
				virtual void end(std::string body, std::function<void(std::shared_ptr<connection>, bool)> cb = [](std::shared_ptr<connection>, bool){}) = 0;

				/** Helper methods **/

				const uri& get_parsed_uri() {
					if (!_parsed_uri) {
						_parsed_uri = std::make_unique<uri>(get_resource());
					}
					return *_parsed_uri;
				}

				bool has_header(const std::string& key) const {
					return get_headers().count(ttl::string::to_lower_copy(key)) != 0;
				}

				const std::string& get_header(const std::string& key) const {
					static const std::string empty = "";
					auto k = ttl::string::to_lower_copy(key);
					auto& headers = get_headers();
					if (headers.count(k) != 0)
						return headers.find(k)->second;
					else return empty;
				}

				virtual std::map<std::type_index, std::shared_ptr<attribute>>& get_attributes() = 0;
				virtual const std::map<std::type_index, std::shared_ptr<attribute>>& get_attributes() const = 0;

				template<typename T>
				typename std::enable_if<std::is_base_of<attribute, T>::value, void>::type
					set_attribute(std::shared_ptr<T> attr) {
					get_attributes()[typeid(T)] = attr;
				}

				template<typename T>
				typename std::enable_if<std::is_base_of<attribute, T>::value, bool>::type
					has_attribute() const {
					return get_attributes().count(typeid(T)) != 0;
				}

				template<typename T>
				typename std::enable_if<std::is_base_of<attribute, T>::value, std::shared_ptr<T>>::type
					get_attribute() const {
					if (!has_attribute<T>())
						return nullptr;
					return std::dynamic_pointer_cast<T>(get_attributes().at(typeid(T)));
				}
			};
			typedef std::shared_ptr<connection> connection_ptr;
		}
	}
}