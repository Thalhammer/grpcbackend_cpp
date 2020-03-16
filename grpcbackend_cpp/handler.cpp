#include "handler.h"
#include <queue>
#include <ttl/logger.h>
#include "http/router.h"

namespace thalhammer {
	namespace grpcbackend {
		typedef std::function<void(bool)> cont_function_t;

		class ws_connection : public websocket::connection, public std::enable_shared_from_this<ws_connection> {
			handler& service;
			websocket::con_handler& con_handler;
			::grpc::ServerContext ctx;
			::grpc::ServerAsyncReaderWriter< ::thalhammer::http::HandleWebSocketResponse, ::thalhammer::http::HandleWebSocketRequest> stream;
			::grpc::ServerCompletionQueue* server_cq;
			std::atomic<bool> done;

			::thalhammer::http::WebSocketRequest req;
			std::multimap<std::string, std::string> req_headers;
			std::multimap<std::string, std::string> resp_headers;

			std::mutex response_queue_lck;
			std::queue<std::shared_ptr<const ::thalhammer::http::HandleWebSocketResponse>> response_queue;

			void send_cb(bool ok) {
				auto that = this->shared_from_this();
				std::unique_lock<std::mutex> lck(this->response_queue_lck);
				bool done = response_queue.front()->message().type() == thalhammer::http::WebSocketMessage::CLOSE;
				service.get_logger()(ttl::loglevel::TRACE, "websocket") << "write ok:" << (ok ?"true":"false");
				response_queue.pop();
				if(!this->done && !response_queue.empty()) {
					stream.Write(*response_queue.front(), new cont_function_t([that](bool ok){
						that->send_cb(ok);
					}));
				}
				if(done) {
					this->con_handler.on_disconnect(that);
					if(this->done.exchange(true)) return;
					stream.Finish(::grpc::Status::OK, new cont_function_t([that = this->shared_from_this()](bool ok) {
						that->service.get_logger()(ttl::loglevel::TRACE, "websocket") <<"finish (server) called";
					}));
				}
			}

			void queue_response(std::shared_ptr<const ::thalhammer::http::HandleWebSocketResponse> msg) {
				std::unique_lock<std::mutex> lck(this->response_queue_lck);
				response_queue.emplace(std::move(msg));
				if(response_queue.size() == 1) {
					auto that = this->shared_from_this();
					stream.Write(*response_queue.front(), new cont_function_t([that](bool ok){
						that->send_cb(ok);
					}));
				}
			}
		public:
			ws_connection(::grpc::ServerCompletionQueue* cq, handler& pservice, websocket::con_handler& pcon_handler)
				: service(pservice), con_handler(pcon_handler), stream(&ctx), server_cq(cq), done(false)
			{
				service.get_logger()(ttl::loglevel::TRACE, "websocket") <<"con create 0x" << std::hex << this;
			}

			virtual ~ws_connection() {
				service.get_logger()(ttl::loglevel::TRACE, "websocket") << "con destroy 0x" << std::hex << this;
			}
			
			void start() {
				auto that = this->shared_from_this();
				service.RequestHandleWebSocket(&ctx, &stream, server_cq, server_cq, new cont_function_t([that](bool ok) {
					if (!ok)
						return;
					// Add new connection
					auto con = std::make_shared<ws_connection>(that->server_cq, that->service, that->con_handler);
					con->start();
					// Read Request data
					auto msg = std::make_shared<::thalhammer::http::HandleWebSocketRequest>();
					that->stream.Read(msg.get(), new cont_function_t([that, msg](bool ok) {
						if (!ok)
							return;
						that->req.CopyFrom(msg->request());
						for (int i = 0; i < that->req.headers_size(); i++) {
							auto hdr = that->req.headers().data()[i];
							that->req_headers.insert({ ttl::string::to_lower_copy(hdr->key()), hdr->value() });
						}
						that->on_connect(std::shared_ptr<const ::thalhammer::http::WebSocketRequest>(msg, &msg->request()));
					}));
				}));
			}
			
			void on_connect(std::shared_ptr<const ::thalhammer::http::WebSocketRequest> req) {
				auto that = this->shared_from_this();
				auto msg = std::make_shared<::thalhammer::http::HandleWebSocketRequest>();
				stream.Read(msg.get(), new cont_function_t([that, msg](bool ok) {
					if (!ok)
						return;
					that->on_message(std::shared_ptr<const ::thalhammer::http::WebSocketMessage>(msg, &msg->message()));
				}));
				that->con_handler.on_connect(that);
				auto msg2 = std::make_shared<::thalhammer::http::HandleWebSocketResponse>();
				auto ptr = msg2->mutable_response();
				for (auto& entry : resp_headers) {
					auto hdr = ptr->add_headers();
					hdr->set_key(entry.first);
					hdr->set_value(entry.second);
				}
				this->queue_response(msg2);
			}

			void on_message(std::shared_ptr<const ::thalhammer::http::WebSocketMessage> msg) {
				if (msg->type() == ::thalhammer::http::WebSocketMessage_Type::WebSocketMessage_Type_CLOSE) {
					this->con_handler.on_disconnect(this->shared_from_this());
					if(this->done.exchange(true)) return;
					stream.Finish(::grpc::Status::OK, new cont_function_t([that = this->shared_from_this()](bool ok) {
						that->service.get_logger()(ttl::loglevel::TRACE, "websocket") <<"finish (client) called";
					}));
				}
				else if(!this->done){
					this->con_handler.on_message(this->shared_from_this(), msg->type() == ::thalhammer::http::WebSocketMessage_Type::WebSocketMessage_Type_BINARY, msg->content());
					auto that = this->shared_from_this();
					auto req = std::make_shared<::thalhammer::http::HandleWebSocketRequest>();
					stream.Read(req.get(), new cont_function_t([that, req](bool ok) {
						if (!ok)
							return;
						that->on_message(std::shared_ptr<const ::thalhammer::http::WebSocketMessage>(req, &req->message()));
					}));
				}
			}

			virtual void send_message(bool bin, const std::string& buf) override {
				if(this->done) return;
				auto msg = std::make_shared<::thalhammer::http::HandleWebSocketResponse>();
				auto ptr = msg->mutable_message();
				ptr->set_content(buf);
				ptr->set_type(bin ? ::thalhammer::http::WebSocketMessage_Type::WebSocketMessage_Type_BINARY : ::thalhammer::http::WebSocketMessage_Type::WebSocketMessage_Type_TEXT);
				this->queue_response(msg);
			}

			virtual void close() override {
				if(this->done) return;
				auto msg = std::make_shared<::thalhammer::http::HandleWebSocketResponse>();
				msg->mutable_message()->set_type(::thalhammer::http::WebSocketMessage_Type::WebSocketMessage_Type_CLOSE);
				this->queue_response(msg);
			}

			// Clientinfo
			virtual const std::string& get_client_ip() const override { return req.client().remote_ip(); }
			virtual uint16_t get_client_port() const override { return req.client().remote_port(); }

			// Serverinfo
			virtual const std::string& get_server_ip() const override { return req.client().local_ip(); }
			virtual uint16_t get_server_port() const override { return req.client().local_port(); }
			virtual bool is_encrypted() const override { return req.client().encrypted(); }

			// Request info
			virtual const std::string& get_resource() const override { return req.resource(); }
			virtual const std::string& get_protocol() const override { return req.protocol(); }
			virtual const std::multimap<std::string, std::string>& get_headers() const override { return req_headers; }

			// Only valid during on_connect
			virtual void set_header(const std::string& pkey, const std::string& value, bool replace = false) override {
				auto key = ttl::string::to_lower_copy(pkey);
				if (replace) {
					resp_headers.erase(key);
				}
				resp_headers.insert({ key, value });
			}
		};

		class http_connection : public http::connection, public std::enable_shared_from_this<http_connection> {
			std::map<std::type_index, std::shared_ptr<attribute>> m_attributes;

			handler& m_service;
			http::con_handler& m_handler;
			::grpc::ServerContext m_ctx;
			::grpc::ServerAsyncReaderWriter< ::thalhammer::http::HandleResponse, ::thalhammer::http::HandleRequest> m_stream;
			::grpc::ServerCompletionQueue* m_server_cq;

			::thalhammer::http::HandleRequest m_initial_req;
			::thalhammer::http::HandleRequest m_req;
			::thalhammer::http::HandleResponse m_resp;
			std::multimap<std::string, std::string> req_headers;

			std::mutex m_read_lck;
			std::mutex m_write_lck;
			std::atomic<bool> m_read_done;
			std::atomic<bool> m_headers_sent;
			std::atomic<bool> m_finished;
			std::atomic<bool> m_started;
		public:
			http_connection(::grpc::ServerCompletionQueue* cq, handler& pservice, http::con_handler& handler)
				: m_service(pservice), m_handler(handler), m_stream(&m_ctx), m_server_cq(cq), m_read_done(false), m_headers_sent(false), m_finished(false), m_started(false)
			{
				m_service.get_logger()(ttl::loglevel::TRACE, "http") <<"con create 0x" << std::hex << this;
			}

			virtual ~http_connection() {
				if(m_started) {
					if(!m_read_done)
						m_service.get_logger()(ttl::loglevel::WARN, "http") << "con dropped without reading content 0x" << std::hex << this;
					if(!m_headers_sent)
						m_service.get_logger()(ttl::loglevel::WARN, "http") << "con dropped without sending headers 0x" << std::hex << this;
					if(!m_finished)
						m_service.get_logger()(ttl::loglevel::WARN, "http") << "con dropped without finishing 0x" << std::hex << this;
				}
				m_service.get_logger()(ttl::loglevel::TRACE, "http") << "con destroy 0x" << std::hex << this;
			}
			
			void start() {
				auto that = this->shared_from_this();
				m_service.RequestHandle(&m_ctx, &m_stream, m_server_cq, m_server_cq, new cont_function_t([that](bool ok) {
					if (!ok)
						return;
					that->m_started = true;
					// Add new connection
					auto con = std::make_shared<http_connection>(that->m_server_cq, that->m_service, that->m_handler);
					con->start();
					// Read Request data
					that->m_stream.Read(&that->m_initial_req, new cont_function_t([that](bool ok) {
						if (!ok)
							return;
						for (int i = 0; i < that->m_initial_req.request().headers_size(); i++) {
							auto hdr = that->m_initial_req.request().headers().data()[i];
							that->req_headers.insert({ ttl::string::to_lower_copy(hdr->key()), hdr->value() });
						}
						try {
							that->m_handler.on_request(that);
						} catch(const std::exception& e) {
							that->m_service.get_logger()(ttl::loglevel::ERR, "http") <<"exception processing http request: " << e.what();
						}
					}));
				}));
			}

			// Clientinfo
			virtual const std::string& get_client_ip() const override { return m_initial_req.client().remote_ip(); }
			virtual uint16_t get_client_port() const override { return m_initial_req.client().remote_port(); }

			// Serverinfo
			virtual const std::string& get_server_ip() const override { return m_initial_req.client().local_ip(); }
			virtual uint16_t get_server_port() const override { return m_initial_req.client().local_port(); }
			virtual bool is_encrypted() const override { return m_initial_req.client().encrypted(); }

			// Request info
			virtual const std::string& get_method() const override { return m_initial_req.request().method(); }
			virtual const std::string& get_resource() const override { return m_initial_req.request().resource(); }
			virtual const std::string& get_protocol() const override { return m_initial_req.request().protocol(); }
			virtual const std::multimap<std::string, std::string>& get_headers() const override { return req_headers; }

			// Only valid until first write
			virtual void set_status(int code, const std::string& message = "") override {
				m_resp.mutable_response()->set_status_code(code);
				if(!message.empty())
					m_resp.mutable_response()->set_status_message(message);
			}

			virtual void set_header(const std::string& pkey, const std::string& value, bool replace = false) override {
				auto key = ttl::string::to_lower_copy(pkey);
				auto hdrs = m_resp.mutable_response()->mutable_headers();
				if(replace) {
					for(auto& e : *hdrs) {
						if(e.key() == key) {
							e.set_value(value);
							return;
						}
					}
				}
				auto hdr = hdrs->Add();
				hdr->set_key(key);
				hdr->set_value(value);
			}

			virtual bool is_headers_done() const override {
				return m_headers_sent;
			}
			virtual bool is_body_read() const override {
				return m_read_done;
			}
			virtual bool is_finished() const override {
				return m_finished;
			}

			virtual void read_body(std::function<void(http::connection_ptr, bool, std::string)> cb) override {
				std::unique_lock<std::mutex> lck(m_read_lck);
				auto that = this->shared_from_this();
				if(m_read_done) {
					lck.unlock();
					try {
						if(cb)
							cb(that, false, "");
					} catch(...) {
						http::router::handle_exception(that, std::current_exception());
					}
				}

				m_stream.Read(&m_req, new cont_function_t([that, cb](bool ok) mutable {
					std::string msg;
					if(ok) msg = std::move(*(that->m_req.mutable_request()->mutable_content()));
					that->m_read_done = !ok;
					that->m_read_lck.unlock();
					try {
						if(cb)
							cb(that, ok, std::move(msg));
					} catch(...) {
						http::router::handle_exception(that, std::current_exception());
					}
				}));
				lck.release();
			}

			static void skip_helper(std::shared_ptr<connection> con, bool ok, std::string, std::function<void(std::shared_ptr<connection>, bool)> cb) {
				if(ok) {
					con->read_body(std::bind(skip_helper, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, cb));
				} else {
					try {
						if(cb)
							cb(con, ok);
					} catch(...) {
						http::router::handle_exception(con, std::current_exception());
					}
				}
			}

			virtual void skip_body(std::function<void(std::shared_ptr<connection>, bool)> cb) override {
				this->read_body(std::bind(skip_helper, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, cb));
			}

			virtual void send_body(std::string body, std::function<void(std::shared_ptr<connection>, bool)> cb, bool can_buffer) override {
				std::unique_lock<std::mutex> lck(m_write_lck);
				auto that = this->shared_from_this();
				if(!m_read_done) {
					lck.unlock();
					try {
						if(cb)
							cb(that, false);
					} catch(...) {
						http::router::handle_exception(that, std::current_exception());
					}
				}
				if(m_headers_sent) {
					m_resp.Clear();
				}
				m_resp.mutable_response()->set_content(std::move(body));

				grpc::WriteOptions opts;
				if(can_buffer && m_headers_sent) opts.set_buffer_hint();

				m_stream.Write(m_resp, opts, new cont_function_t([that, cb](bool ok) mutable {
					that->m_headers_sent = true;
					that->m_write_lck.unlock();
					try {
						if(cb)
							cb(that, ok);
					} catch(...) {
						http::router::handle_exception(that, std::current_exception());
					}
				}));
				lck.release();
			}

			/**
			 * Note on buffer size:
			 * More experimentation is needed:
			 * * Filesize impact
			 * * Concurrency
			 * Without buffer_hint:
			 * 65536=> 780MB/s
			 * 32768=> 600MB/s
			 * 16384=> 380MB/s
			 * 8192 => 170MB/s
			 * 4096 => 140MB/s
			 * 2048 => 76MB/s
			 * 
			 * With buffer_hint:
			 * 65536=> 750MB/s
			 * 32768=> 680MB/s
			 * 16384=> 520MB/s
			 * 8129 => 370MB/s
			 * 4096 => 245MB/s
			 * 2048 => 157MB/s
			 */

			virtual void send_body(std::istream& body, std::function<void(std::shared_ptr<connection>, bool)> cb) override {
				struct helper {
					std::istream& body;
					std::function<void(std::shared_ptr<connection>, bool)> cb;
					std::string buf;
					void handle_send(std::shared_ptr<connection> con, bool ok) {
						if(!body || !ok) {
							try {
								if(cb)
									cb(con, ok);
							} catch(...) {
								http::router::handle_exception(con, std::current_exception());
							}
							delete this;
						} else {
							buf.resize(65536);
							auto r = body.read(const_cast<char*>(buf.data()), buf.size()).gcount();
							buf.resize(r);
							con->send_body(buf, std::bind(&helper::handle_send, this, std::placeholders::_1, std::placeholders::_2), false);
						}
					}
				};
				auto h = new helper{body, cb};
				h->handle_send(this->shared_from_this(), true);
			}

			virtual void end(std::function<void(std::shared_ptr<connection>, bool)> cb) override {
				return end("", cb);
			}

			virtual void end(std::string body, std::function<void(std::shared_ptr<connection>, bool)> cb) override {
				std::unique_lock<std::mutex> lck(m_write_lck);
				auto that = this->shared_from_this();
				if(!m_read_done) {
					lck.unlock();
					try {
						if(cb)
							cb(that, false);
					} catch(...) {
						http::router::handle_exception(that, std::current_exception());
					}
				}
				if(m_headers_sent && body.empty()) {
					m_finished = true;
					m_stream.Finish(grpc::Status::OK, new cont_function_t([that, cb](bool ok) mutable {
						that->m_write_lck.unlock();
						try {
							if(cb)
								cb(that, ok);
						} catch(...) {
							http::router::handle_exception(that, std::current_exception());
						}
					}));
				} else {
					m_finished = true;
					m_resp.mutable_response()->set_content(std::move(body));
					m_stream.WriteAndFinish(m_resp, {}, grpc::Status::OK, new cont_function_t([that, cb](bool ok) mutable {
						that->m_headers_sent = true;
						that->m_write_lck.unlock();
						try {
							if(cb)
								cb(that, ok);
						} catch(...) {
							http::router::handle_exception(that, std::current_exception());
						}
					}));
				}
				lck.release();
			}

			virtual std::map<std::type_index, std::shared_ptr<attribute>>& get_attributes() override { return m_attributes; }
			virtual const std::map<std::type_index, std::shared_ptr<attribute>>& get_attributes() const override { return m_attributes; }
		};

		handler::handler(websocket::con_handler& ws_handler, http::con_handler& http_handler, ttl::logger& logger)
			:_ws_handler(ws_handler), _http_handler(http_handler), _logger(logger)
		{
		}

		handler::~handler()
		{
		}

		void handler::async_task(::grpc::ServerCompletionQueue* cq)
		{
			{
				auto con = std::make_shared<ws_connection>(cq, *this, _ws_handler);
				con->start();
			}
			{
				auto con = std::make_shared<http_connection>(cq, *this, _http_handler);
				con->start();
			}
			while (true) {
				void* tag;
				bool ok = false;
				if(!cq->Next(&tag, &ok)) break;
				cont_function_t* cont_fn = (cont_function_t*)tag;
				if (cont_fn != nullptr) {
					try {
						if ((*cont_fn))
							(*cont_fn)(ok);
					}
					catch (const std::exception& e) {
						get_logger()(ttl::loglevel::TRACE, "handler") <<"exception processing callback: " << e.what();
					}
					delete cont_fn;
				}
			}
		}
	}
}