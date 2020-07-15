#include <gtest/gtest.h>
#include "dummy_connection.h"
#include <http/router.h>
#include <http/route_params.h>

using namespace thalhammer::grpcbackend;

struct dummy_middleware : http::middleware {
	bool called = false;
	virtual void on_request(http::connection_ptr con, std::function<void(http::connection_ptr)> next) override
	{
		called = true;
		next(con);
	}
};

TEST(RouterTest, NotFoundHandler) {
	auto con = dummy_connection::make_get("/");
	http::router r;
	bool called = false;
	r.notfound([&](auto con) {
        con->set_status(404);
		called = true;
	});
	r.on_request(con);
	ASSERT_TRUE(called);
}

TEST(RouterTest, NotFound) {
	auto con = dummy_connection::make_get("/");
	http::router r;
	r.on_request(con);
	ASSERT_EQ(con->status_code, 404);
}

TEST(RouterTest, HandlerCalled) {
	http::router r;
	bool called = false;
	r.route("GET", "/", [&](auto con) {
		con->set_status(200);
		called = true;
	});

	{
		auto con = dummy_connection::make_get("/");
		called = false;
		r.on_request(con);
		ASSERT_TRUE(called);
		ASSERT_EQ(con->status_code, 200);
	}
	{
		auto con = dummy_connection::make_post("/", "");
		called = false;
		r.on_request(con);
		ASSERT_FALSE(called);
		ASSERT_EQ(con->status_code, 404);
	}
}

TEST(RouterTest, RouteParams) {
	http::router r;
	bool called = false;
	r.route("GET", "/{name}", [&](http::connection_ptr con) {
		con->set_status(200);
		auto params = con->get_attribute<http::route_params>();
		ASSERT_NE(params, nullptr);
		ASSERT_EQ(params->get_selected_route(), "/{name}");
		ASSERT_TRUE(params->has_param("name"));
		ASSERT_EQ(params->get_param("name"), "test");
		called = true;
	});

	{
		auto con = dummy_connection::make_get("/test");
		called = false;
		r.on_request(con);
		ASSERT_TRUE(called);
		ASSERT_EQ(con->status_code, 200);
	}
	{
		auto con = dummy_connection::make_get("/");
		called = false;
		r.on_request(con);
		ASSERT_FALSE(called);
		ASSERT_EQ(con->status_code, 404);
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
	r.route("GET", "/", [&](auto con) {
		con->set_status(200);
		called = true;
	}, { lmw });
	r.route("GET", "/test", [&](auto con) {
		con->set_status(200);
		called2 = true;
	}, { lmw2 });
	{
		auto con = dummy_connection::make_get("/");
		called = false;
		r.on_request(con);
		ASSERT_TRUE(called);
		ASSERT_FALSE(called2);
		ASSERT_TRUE(gmw->called);
		ASSERT_TRUE(lmw->called);
		ASSERT_FALSE(lmw2->called);
		ASSERT_EQ(con->status_code, 200);
	}
	{
		auto con = dummy_connection::make_get("/test");
		called = false;
		called2 = false;
		gmw->called = false;
		lmw->called = false;
		lmw2->called = false;
		r.on_request(con);
		ASSERT_FALSE(called);
		ASSERT_TRUE(called2);
		ASSERT_TRUE(gmw->called);
		ASSERT_FALSE(lmw->called);
		ASSERT_TRUE(lmw2->called);
		ASSERT_EQ(con->status_code, 200);
	}
	{
		auto con = dummy_connection::make_get("/test2");
		called = false;
		called2 = false;
		gmw->called = false;
		lmw->called = false;
		lmw2->called = false;
		r.on_request(con);
		ASSERT_FALSE(called);
		ASSERT_FALSE(called2);
		ASSERT_TRUE(gmw->called);
		ASSERT_FALSE(lmw->called);
		ASSERT_FALSE(lmw2->called);
		ASSERT_EQ(con->status_code, 404);
	}
}