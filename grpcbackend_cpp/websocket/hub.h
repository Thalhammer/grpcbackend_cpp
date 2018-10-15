#pragma once
#include "con_handler.h"
#include <map>
#include <string>
#include <mutex>
#include <set>
#include <ttl/logger.h>

namespace thalhammer {
	namespace grpcbackend {
		namespace websocket {
			class hub : public con_handler
			{
				struct group {
					std::mutex cons_lck;
					std::set<connection_ptr> cons;

					std::shared_ptr<con_handler> handler;
					std::string name;
				};
				std::unordered_map<std::string, std::shared_ptr<group>> _groups;

				struct group_attribute : public attribute {
					std::weak_ptr<hub::group> group;
				};

				std::shared_ptr<con_handler> _default_handler;
				ttl::logger& _logger;
			public:
				explicit hub(ttl::logger& logger);
				~hub();

				hub& close_all();

				void on_connect(connection_ptr con) override;
				void on_message(connection_ptr con, bool bin, const std::string& msg) override;
				void on_disconnect(connection_ptr con) override;

				/* Set the default handler used for on_connect and if connection is not assigned to a group. */
				hub& set_default_handler(std::shared_ptr<con_handler> handler);

				/* Set the group of a connection. */
				hub& set_connection_group(const connection_ptr& con, const std::string& gname);
				/* Get the name of the group of con or empty string if no group was set for con. */
				std::string get_connection_group(const connection_ptr& con);
				/* Add a new group. */
				hub& add_group(const std::string& name, std::shared_ptr<con_handler> handler);
				/* Broadcast a message to all connections in a specific group. */
				hub& broadcast(const std::string& gname, bool bin, const std::string& msg);
				/* Broadcast a message to all connections in the same group as con. */
				hub& broadcast(const connection_ptr& con, bool bin, const std::string& msg);
				/* Close connections of a specific group */
				hub& close_group(const std::string& gname);
			};
		}
	}
}