#pragma once
#include "../connection.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
            /* Helper class to override some methods of a response object while forwarding all the others */
			class connection_forward: public connection {
                connection_ptr orig;
			protected:
            public:
                explicit connection_forward(connection_ptr con) : orig(con) {}
                connection_ptr get_original_connection() const { return orig; }

				virtual const std::string& get_client_ip() const override { return orig->get_client_ip(); }
				virtual uint16_t get_client_port() const override { return orig->get_client_port(); }

				// Serverinfo
				virtual const std::string& get_server_ip() const override { return orig->get_server_ip(); }
				virtual uint16_t get_server_port() const override { return orig->get_server_port(); }
				virtual bool is_encrypted() const override { return orig->is_encrypted(); }

				// Request info
				virtual const std::string& get_method() const override { return orig->get_method(); }
				virtual const std::string& get_resource() const override { return orig->get_resource(); }
				virtual const std::string& get_protocol() const override { return orig->get_protocol(); }
				virtual const std::multimap<std::string, std::string>& get_headers() const override { return orig->get_headers(); }

				// Only valid until first write
				virtual void set_status(int code, const std::string& message = "") override { return orig->set_status(code, message); }
				virtual void set_header(const std::string& key, const std::string& value, bool replace = false) override { return orig->set_header(key, value, replace); }

				virtual bool is_headers_done() const override { return orig->is_headers_done(); }
				virtual bool is_body_read() const override { return orig->is_body_read(); }
				virtual bool is_finished() const override { return orig->is_finished(); }

				virtual std::map<std::type_index, std::shared_ptr<attribute>>& get_attributes() override {
					return orig->get_attributes();
				}
				virtual const std::map<std::type_index, std::shared_ptr<attribute>>& get_attributes() const override {
					return orig->get_attributes();
				}

				virtual void read_body(std::function<void(std::shared_ptr<connection>, bool, std::string)> cb) override { return orig->read_body(cb); }
				virtual void skip_body(std::function<void(std::shared_ptr<connection>, bool)> cb) override { return orig->skip_body(cb); }
				virtual void send_body(std::string body, std::function<void(std::shared_ptr<connection>, bool)> cb, bool can_buffer=false) override { return orig->send_body(body, cb, can_buffer); }
				virtual void send_body(std::istream& body, std::function<void(std::shared_ptr<connection>, bool)> cb) override { return orig->send_body(body, cb); }
				virtual void end(std::function<void(std::shared_ptr<connection>, bool)> cb = [](std::shared_ptr<connection>, bool){}) override { return orig->end(cb); }
				virtual void end(std::string body, std::function<void(std::shared_ptr<connection>, bool)> cb) override { return orig->end(body, cb); }
			};
		}
	}
}