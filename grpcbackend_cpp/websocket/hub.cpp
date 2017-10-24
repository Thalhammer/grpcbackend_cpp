#include "hub.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace websocket {

			void hub::on_connect(connection_ptr con)
			{
				if(_default_handler)
					_default_handler->on_connect(con);
			}

			void hub::on_message(connection_ptr con, bool bin, const std::string& msg)
			{
				auto attr = con->get_attribute<group_attribute>();
				if(attr) {
					auto group = attr->group.lock();
					if(group) {
						group->handler->on_message(con, bin, msg);
						return;
					}
				}
				if(_default_handler)
					_default_handler->on_disconnect(con);
			}

			void hub::on_disconnect(connection_ptr con)
			{
				auto attr = con->get_attribute<group_attribute>();
				if(attr) {
					auto group = attr->group.lock();
					if(group) {
						group->handler->on_disconnect(con);
						{
							std::unique_lock<std::mutex> lck(group->cons_lck);
							group->cons.erase(con);
						}
						attr->group.reset();
						return;
					}
				}
				if(_default_handler)
					_default_handler->on_disconnect(con);
			}

			hub& hub::set_default_handler(std::shared_ptr<con_handler> handler)
			{
				_default_handler = handler;
				return *this;
			}

			hub& hub::set_connection_group(const connection_ptr& con, const std::string& gname)
			{
				if(_groups.count(gname)==0)
					return *this;
				auto attr = con->get_attribute<group_attribute>();
				if(!attr) {
					attr = std::make_shared<group_attribute>();
					con->set_attribute(attr);
				}
				auto g = _groups.at(gname);
				attr->group = g;
				{
					std::unique_lock<std::mutex> lck(g->cons_lck);
					g->cons.insert(con);
				}
				return *this;
			}

			std::string hub::get_connection_group(const connection_ptr& con)
			{
				auto attr = con->get_attribute<group_attribute>();
				if(attr) {
					auto g = attr->group.lock();
					if(g)
						return g->name;
				}
				return "";
			}

			hub& hub::add_group(const std::string& name, std::shared_ptr<con_handler> handler)
			{
				if(!handler) {
					handler = _default_handler;
				}
				if(_groups.count(name)==0) {
					auto g = std::make_shared<group>();
					g->name = name;
					g->handler = handler;
					_groups[name] = g;
				}
				return *this;
			}

			hub& hub::broadcast(const std::string& gname, bool bin, const std::string& msg)
			{
				if(_groups.count(gname)!=0) {
					auto g = _groups[gname];
					if(g) {
						std::unique_lock<std::mutex> lck(g->cons_lck);
						for(auto& e : g->cons) {
							e->send_message(bin, msg);
						}
					}
				}
				return *this;
			}

			hub& hub::broadcast(const connection_ptr& con, bool bin, const std::string& msg)
			{
				auto attr = con->get_attribute<group_attribute>();
				if(attr) {
					auto g = attr->group.lock();
					if(g){
						std::unique_lock<std::mutex> lck(g->cons_lck);
						for(auto& e : g->cons) {
							e->send_message(bin, msg);
						}
					}
				}
				return *this;
			}
		}
	}
}