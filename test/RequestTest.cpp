#include <gtest/gtest.h>
#include "dummy_connection.h"

using namespace thalhammer::grpcbackend;

struct test_attribute : attribute {

};

TEST(RequestTest, AttributeStorage) {
	auto req = std::make_unique<dummy_connection>();
	ASSERT_EQ(req->get_attribute<test_attribute>(), nullptr);
	ASSERT_FALSE(req->has_attribute<test_attribute>());
	req->set_attribute(std::make_shared<test_attribute>());
	ASSERT_TRUE(req->has_attribute<test_attribute>());
	ASSERT_NE(req->get_attribute<test_attribute>(), nullptr);
}

TEST(RequestTest, RequestMethod) {
	auto req = dummy_connection::make_get("/");
	ASSERT_TRUE(req->is_get());
	ASSERT_FALSE(req->is_post());
	req->method = "POST";
	ASSERT_FALSE(req->is_get());
	ASSERT_TRUE(req->is_post());
	req->method = "DELETE";
	ASSERT_FALSE(req->is_get());
	ASSERT_FALSE(req->is_post());
}

TEST(RequestTest, ParsedUri) {
	auto req = dummy_connection::make_get("/index.html");
	auto& uri = req->get_parsed_uri();
	ASSERT_EQ(uri.get_path(), "/index.html");
}

TEST(RequestTest, Headers) {
	auto req = dummy_connection::make_get("/index.html");
	req->req_headers.insert({ "host", "test.com" });
	ASSERT_EQ(req->get_header("test"), "");
	ASSERT_FALSE(req->has_header("test"));
	ASSERT_EQ(req->get_header("host"), "test.com");
	ASSERT_TRUE(req->has_header("host"));
	ASSERT_EQ(req->get_header("Host"), "test.com");
	ASSERT_TRUE(req->has_header("Host"));
}