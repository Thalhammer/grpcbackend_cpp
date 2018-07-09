#include <gtest/gtest.h>
#include "dummy_request.h"
#include "dummy_response.h"
#include <http/router.h>
#include <http/route_params.h>

using namespace thalhammer::grpcbackend;

struct dummy_middleware : http::middleware {
	bool called = false;
	// Geerbt �ber middleware
	virtual void handle_request(http::request & req, http::response & resp, std::function<void(http::request&, http::response&)>& next) override
	{
		called = true;
		next(req, resp);
	}
};

TEST(RouterTest, NotFoundHandler) {
	dummy_request req = dummy_request::make_get("/");
	dummy_response resp;
	http::router r;
	bool called = false;
	r.notfound([&](auto& req, auto& resp) {
		called = true;
	});
	r.handle_request(req, resp);
	ASSERT_TRUE(called);
}

TEST(RouterTest, NotFound) {
	dummy_request req = dummy_request::make_get("/");
	dummy_response resp;
	http::router r;
	r.handle_request(req, resp);
	ASSERT_EQ(resp.status_code, 404);
}

TEST(RouterTest, HandlerCalled) {
	http::router r;
	bool called = false;
	r.route("GET", "/", [&](auto& req, auto& resp) {
		resp.set_status(200);
		called = true;
	});

	{
		dummy_request req = dummy_request::make_get("/");
		dummy_response resp;
		called = false;
		r.handle_request(req, resp);
		ASSERT_TRUE(called);
		ASSERT_EQ(resp.status_code, 200);
	}
	{
		dummy_request req = dummy_request::make_post("/", "");
		dummy_response resp;
		called = false;
		r.handle_request(req, resp);
		ASSERT_FALSE(called);
		ASSERT_EQ(resp.status_code, 404);
	}
}

TEST(RouterTest, RouteParams) {
	http::router r;
	bool called = false;
	r.route("GET", "/{name}", [&](http::request& req, auto& resp) {
		resp.set_status(200);
		std::shared_ptr<http::route_params> params = req.get_attribute<http::route_params>();
		ASSERT_NE(params, nullptr);
		ASSERT_EQ(params->get_selected_route(), "/{name}");
		ASSERT_TRUE(params->has_param("name"));
		ASSERT_EQ(params->get_param("name"), "test");
		called = true;
	});

	{
		dummy_request req = dummy_request::make_get("/test");
		dummy_response resp;
		called = false;
		r.handle_request(req, resp);
		ASSERT_TRUE(called);
		ASSERT_EQ(resp.status_code, 200);
	}
	{
		dummy_request req = dummy_request::make_get("/");
		dummy_response resp;
		called = false;
		r.handle_request(req, resp);
		ASSERT_FALSE(called);
		ASSERT_EQ(resp.status_code, 404);
	}
}

TEST(RouterTest, MiddleWare) {
	http::router r;
	bool called = false;
	bool called2 = false;
	auto gmw = std::make_shared<dummy_middleware>();
	auto lmw = std::make_shared<dummy_middleware>();
	auto lmw2 = std::make_shared<dummy_middleware>();
	r.use(gmw);
	r.route("GET", "/", [&](auto& req, auto& resp) {
		resp.set_status(200);
		called = true;
	}, { lmw });
	r.route("GET", "/test", [&](auto& req, auto& resp) {
		resp.set_status(200);
		called2 = true;
	}, { lmw2 });
	{
		dummy_request req = dummy_request::make_get("/");
		dummy_response resp;
		called = false;
		r.handle_request(req, resp);
		ASSERT_TRUE(called);
		ASSERT_FALSE(called2);
		ASSERT_TRUE(gmw->called);
		ASSERT_TRUE(lmw->called);
		ASSERT_FALSE(lmw2->called);
		ASSERT_EQ(resp.status_code, 200);
	}
	{
		dummy_request req = dummy_request::make_get("/test");
		dummy_response resp;
		called = false;
		called2 = false;
		gmw->called = false;
		lmw->called = false;
		lmw2->called = false;
		r.handle_request(req, resp);
		ASSERT_FALSE(called);
		ASSERT_TRUE(called2);
		ASSERT_TRUE(gmw->called);
		ASSERT_FALSE(lmw->called);
		ASSERT_TRUE(lmw2->called);
		ASSERT_EQ(resp.status_code, 200);
	}
	{
		dummy_request req = dummy_request::make_get("/test2");
		dummy_response resp;
		called = false;
		called2 = false;
		gmw->called = false;
		lmw->called = false;
		lmw2->called = false;
		r.handle_request(req, resp);
		ASSERT_FALSE(called);
		ASSERT_FALSE(called2);
		ASSERT_TRUE(gmw->called);
		ASSERT_FALSE(lmw->called);
		ASSERT_FALSE(lmw2->called);
		ASSERT_EQ(resp.status_code, 404);
	}
}