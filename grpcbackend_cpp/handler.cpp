#include "handler.h"
#include <queue>
#include <ttl/logger.h>

namespace thalhammer {
	namespace grpcbackend {
		namespace {
		class handler_streambuf : public std::streambuf {
			::grpc::ServerReaderWriter< ::thalhammer::http::HandleResponse, ::thalhammer::http::HandleRequest>* _stream;
			::thalhammer::http::HandleRequest req;
			std::string _out_buf;
		public:
			explicit handler_streambuf(::grpc::ServerReaderWriter< ::thalhammer::http::HandleResponse, ::thalhammer::http::HandleRequest>* stream)
				:_stream(stream), _out_buf(4096, 0x00)
			{
				setg(nullptr, nullptr, nullptr);
				setp(&_out_buf[0], &_out_buf[_out_buf.size()]);
			}

			~handler_streambuf() {
				this->sync();
			}

			virtual std::streambuf::int_type underflow() override
			{
				if (_stream->Read(&req)) {
					auto* str = req.mutable_request()->mutable_content();
					setg((char*)str->data(), (char*)str->data(), (char*)str->data() + str->size());
					return traits_type::to_int_type(*gptr());
				}
				else {
					return traits_type::eof();
				}
			}

			virtual std::streambuf::int_type overflow(std::streambuf::int_type value)
			{
				int write = pptr() - pbase();
				if (write)
				{
					auto buf_size = _out_buf.size();
					_out_buf.resize(write);
					::thalhammer::http::HandleResponse resp;
					resp.mutable_response()->set_allocated_content(&_out_buf);
					if (!_stream->Write(resp)) {
						resp.mutable_response()->release_content();
						return traits_type::eof();
					}
					// Prevent destructor from deleting _out_buf
					resp.mutable_response()->release_content();
					_out_buf.resize(buf_size);
				}

				setp(&_out_buf[0], &_out_buf[_out_buf.size()]);
				if (!traits_type::eq_int_type(value, traits_type::eof())) sputc(value);
				return traits_type::not_eof(value);
			}

			virtual int sync()
			{
				std::streambuf::int_type result = this->overflow(traits_type::eof());
				return traits_type::eq_int_type(result, traits_type::eof()) ? -1 : 0;
			}

			::grpc::ServerReaderWriter< ::thalhammer::http::HandleResponse, ::thalhammer::http::HandleRequest>* get_stream()
			{
				return _stream;
			}
		};

		class handler_interface : public thalhammer::grpcbackend::http::request, public thalhammer::grpcbackend::http::response {
			::thalhammer::http::HandleRequest _initial_req;
			std::multimap<std::string, std::string> _req_headers;
			::thalhammer::http::HandleResponse _initial_resp;
			bool _initial_sent;
			handler_streambuf _streambuf;
			std::istream _istream;
			std::ostream _ostream;
		public:
			explicit handler_interface(::grpc::ServerReaderWriter< ::thalhammer::http::HandleResponse, ::thalhammer::http::HandleRequest>* stream);

			void send_headers();

			// Geerbt über request
			virtual const std::string & get_client_ip() const override { return _initial_req.client().remote_ip(); }
			virtual uint16_t get_client_port() const override { return _initial_req.client().remote_port(); }
			virtual const std::string & get_server_ip() const override { return _initial_req.client().local_ip(); }
			virtual uint16_t get_server_port() const override { return _initial_req.client().local_port(); }
			virtual bool is_encrypted() const override { return _initial_req.client().encrypted(); }
			virtual const std::string & get_method() const override { return _initial_req.request().method(); }
			virtual const std::string & get_resource() const override { return _initial_req.request().resource(); }
			virtual const std::string & get_protocol() const override { return _initial_req.request().protocol(); }
			virtual const std::multimap<std::string, std::string>& get_headers() const override { return _req_headers; }
			virtual std::istream & get_istream() override { return _istream; }

			// Geerbt über response
			virtual void set_status(int code, const std::string & message = "") override;
			virtual void set_header(const std::string & key, const std::string & value, bool replace = false) override;
			virtual std::ostream & get_ostream() override;
		};
		}

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
				service.get_logger()(thalhammer::loglevel::TRACE, "websocket") << "write ok:" << (ok ?"true":"false");
				response_queue.pop();
				if(!this->done && !response_queue.empty()) {
					stream.Write(*response_queue.front(), new cont_function_t([that](bool ok){
						that->send_cb(ok);
					}));
				}
				if(done) {
					this->con_handler.on_disconnect(that);
					if(this->done.exchange(true)) return;
					stream.Finish(::grpc::Status::OK, new cont_function_t([that = this->shared_from_this()](bool ok) mutable {
						that->service.get_logger()(thalhammer::loglevel::TRACE, "websocket") <<"finish (server) called";
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
				service.get_logger()(thalhammer::loglevel::TRACE, "websocket") <<"con create 0x" << std::hex << this;
			}

			virtual ~ws_connection() {
				service.get_logger()(thalhammer::loglevel::TRACE, "websocket") << "con destroy 0x" << std::hex << this;
			}
			
			void start() {
				auto that = this->shared_from_this();
				service.RequestHandleWebSocket(&ctx, &stream, server_cq, server_cq, new cont_function_t([that](bool ok) mutable {
					if (!ok)
						return;
					// Add new connection
					auto con = std::make_shared<ws_connection>(that->server_cq, that->service, that->con_handler);
					con->start();
					// Read Request data
					auto msg = std::make_shared<::thalhammer::http::HandleWebSocketRequest>();
					that->stream.Read(msg.get(), new cont_function_t([that, msg](bool ok) mutable {
						if (!ok)
							return;
						that->req.CopyFrom(msg->request());
						for (int i = 0; i < that->req.headers_size(); i++) {
							auto& hdr = that->req.headers()[i];
							that->req_headers.insert({ string::to_lower_copy(hdr.key()), hdr.value() });
						}
						that->on_connect(std::shared_ptr<const ::thalhammer::http::WebSocketRequest>(msg, &msg->request()));
					}));
				}));
			}
			
			void on_connect(std::shared_ptr<const ::thalhammer::http::WebSocketRequest> req) {
				auto that = this->shared_from_this();
				auto msg = std::make_shared<::thalhammer::http::HandleWebSocketRequest>();
				stream.Read(msg.get(), new cont_function_t([that, msg](bool ok) mutable {
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
					stream.Finish(::grpc::Status::OK, new cont_function_t([that = this->shared_from_this()](bool ok) mutable {
						that->service.get_logger()(thalhammer::loglevel::TRACE, "websocket") <<"finish (client) called";
					}));
				}
				else if(!this->done){
					this->con_handler.on_message(this->shared_from_this(), msg->type() == ::thalhammer::http::WebSocketMessage_Type::WebSocketMessage_Type_BINARY, msg->content());
					auto that = this->shared_from_this();
					auto req = std::make_shared<::thalhammer::http::HandleWebSocketRequest>();
					stream.Read(req.get(), new cont_function_t([that, req](bool ok) mutable {
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
				auto key = string::to_lower_copy(pkey);
				if (replace) {
					resp_headers.erase(key);
				}
				resp_headers.insert({ key, value });
			}
		};

		handler::handler(http::router & route, websocket::con_handler& ws_handler, thalhammer::logger& logger)
			:_route(route), _ws_handler(ws_handler), _logger(logger)
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
					catch (const std::exception& e) {}
					delete cont_fn;
				}
			}
		}

		::grpc::Status handler::Handle(::grpc::ServerContext * context, ::grpc::ServerReaderWriter<::thalhammer::http::HandleResponse, ::thalhammer::http::HandleRequest>* stream)
		{
			try {
				handler_interface iface(stream);
				_route.handle_request(iface, iface);
				// Send headers if ostream was not requested
				iface.send_headers();
				return ::grpc::Status::OK;
			}
			catch (const std::exception& e) {
				return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
			}
		}

		handler_interface::handler_interface(::grpc::ServerReaderWriter<::thalhammer::http::HandleResponse, ::thalhammer::http::HandleRequest>* stream)
			: _initial_sent(false), _streambuf(stream), _istream(&_streambuf), _ostream(&_streambuf)
		{
			this->set_status(200);
			if (!stream->Read(&_initial_req)) {
				throw std::runtime_error("Failed to read initial packet");
			}
			for (int i = 0; i < _initial_req.request().headers_size(); i++) {
				auto& hdr = _initial_req.request().headers()[i];
				_req_headers.insert({ string::to_lower_copy(hdr.key()), hdr.value() });
			}
		}

		void handler_interface::send_headers()
		{
			if (!_initial_sent) {
				_initial_sent = true;
				if (!_streambuf.get_stream()->Write(_initial_resp)) {
					throw std::runtime_error("Failed to send headers");
				}
			}
		}

		void handler_interface::set_status(int code, const std::string & message)
		{
			_initial_resp.mutable_response()->set_status_code(code);
			if (!message.empty())
				_initial_resp.mutable_response()->set_status_message(message);
		}

		void handler_interface::set_header(const std::string & pkey, const std::string & value, bool replace)
		{
			auto key = string::to_lower_copy(pkey);
			if (replace) {
				for (int i = 0; i < _initial_resp.response().headers_size(); i++)
				{
					auto* hdr = _initial_resp.mutable_response()->mutable_headers()->Mutable(i);
					if (hdr->key() == key)
					{
						hdr->set_value(value);
						return;
					}
				}
			}
			auto hdr = _initial_resp.mutable_response()->add_headers();
			hdr->set_key(key);
			hdr->set_value(value);
		}

		std::ostream & handler_interface::get_ostream()
		{
			// Send initial header response
			send_headers();
			return _ostream;
		}
	}
}