#pragma once
#include <grpcbackend/attribute_host.h>
#include <grpcbackend/uri.h>
#include <string>
#include <functional>
#include <optional>

namespace thalhammer {
	namespace grpcbackend {
		namespace websocket {
			class connection : public attribute_host
			{
				attribute_map _attributes;
				mutable std::optional<uri> _parsed_uri;
			protected:
				virtual attribute_map& get_attributes() override { return _attributes; }
			public:
				virtual ~connection() {}
				virtual void send_message(bool bin, const std::string& msg) = 0;
				virtual void send_message(bool bin, const std::string& msg, std::function<void()> cb) = 0;
				virtual void close() = 0;

				// Clientinfo
				virtual const std::string& get_client_ip() const = 0;
				virtual uint16_t get_client_port() const = 0;

				// Serverinfo
				virtual const std::string& get_server_ip() const = 0;
				virtual uint16_t get_server_port() const = 0;
				virtual bool is_encrypted() const = 0;

				// Request info
				virtual const std::string& get_resource() const = 0;
				virtual const std::string& get_protocol() const = 0;
				virtual const std::multimap<std::string, std::string>& get_headers() const = 0;

				// Only valid during on_connect
				virtual void set_header(const std::string& key, const std::string& value, bool replace = false) = 0;

				const uri& get_parsed_uri() const;
				bool has_header(const std::string& key) const;
				const std::string& get_header(const std::string& key) const;

				virtual const attribute_map& get_attributes() const override { return _attributes; }
			};
			typedef std::shared_ptr<connection> connection_ptr;
		}
	}
}