#pragma once
#include <grpcbackend/attribute_host.h>
#include <grpcbackend/uri.h>
#include <string>
#include <functional>
#include <optional>

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			class connection : public attribute_host
			{
				mutable std::optional<uri> _parsed_uri;
			protected:
			public:
				virtual ~connection() {}
				virtual attribute_map& get_attributes() = 0;
				virtual const attribute_map& get_attributes() const = 0;

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
				const uri& get_parsed_uri() const;
				bool has_header(const std::string& key) const;
				const std::string& get_header(const std::string& key) const;

				bool is_get() const { return get_method() == "GET"; }
				bool is_post() const { return get_method() == "POST"; }
			};
			typedef std::shared_ptr<connection> connection_ptr;
		}
	}
}