/*
 *  Generated by grpcbackend_jsonrpc on protobuf 3.11.2
 */
#include <grpcbackend/util/json_rpc.h>
namespace thalhammer {
namespace grpcbackend {
namespace util {
	template<typename TReq, typename TRes>
	class json_rpc_query {
		std::shared_ptr<thalhammer::grpcbackend::util::json_rpc_call_context> m_context;
		TReq m_request;
	public:
		json_rpc_query(std::shared_ptr<thalhammer::grpcbackend::util::json_rpc_call_context> ctx)
			: m_context(ctx)
		{
			m_request.from_json(m_context->get_params());
		}
		void respond(const TRes& result) {
			m_context->respond(result.to_json());
		}
		void error(int code, const std::string& message, picojson::value data = {}) {
			m_context->error(code, message, data);
		}
	
		const picojson::value& get_id() const noexcept { return m_context->get_id(); }
		bool is_notification() const noexcept { return m_context->is_notification(); }
		bool is_batched() const noexcept { return m_context->is_batched(); }
		const std::string& get_method() const noexcept { return m_context->get_method(); }
		const TReq& get_request() const noexcept { return m_request; }
	
		bool prefer_sync() const noexcept { return m_context->prefer_sync(); }
		std::shared_ptr<void> get_context() const noexcept { return m_context->get_context(); }
	};
}
}
}

namespace test {

enum class TestRequest_SubMessage_TestEnum : int {
	TEST = 0,
	TEST2 = 1,
};

struct TestRequest_SubMessage {
	typedef TestRequest_SubMessage_TestEnum TestEnum;

	int32_t test;
	TestEnum _enum;
	std::string bfield;

	picojson::value to_json() const;
	void from_json(const picojson::value& val);
};

struct TestRequest {
	typedef TestRequest_SubMessage SubMessage;

	std::string request;
	SubMessage msg;

	picojson::value to_json() const;
	void from_json(const picojson::value& val);
};

struct TestResponse {

	std::string response;

	picojson::value to_json() const;
	void from_json(const picojson::value& val);
};

class TestService {
protected:
	void register_service(thalhammer::grpcbackend::util::json_rpc& rpc);

	virtual void Test(std::shared_ptr<thalhammer::grpcbackend::util::json_rpc_query<TestRequest,TestResponse>> context) = 0;
};

} /* namespace test */

