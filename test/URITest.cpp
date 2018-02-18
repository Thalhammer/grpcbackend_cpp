#include <gtest/gtest.h>
#include <uri.h>

using namespace thalhammer::grpcbackend;

TEST(URITest, ParseSimple) {
	uri u("/test");
	ASSERT_EQ(u.get_path(), "/test");
	ASSERT_EQ(u.get_filename(), "test");
	ASSERT_EQ(u.get_extension(), "");
}

TEST(URITest, ParseNoFile) {
	uri u("/test/");
	ASSERT_EQ(u.get_path(), "/test/");
	ASSERT_EQ(u.get_filename(), "");
	ASSERT_EQ(u.get_extension(), "");
}

TEST(URITest, ParseExtension) {
	uri u("/test.jpg");
	ASSERT_EQ(u.get_path(), "/test.jpg");
	ASSERT_EQ(u.get_filename(), "test.jpg");
	ASSERT_EQ(u.get_extension(), "jpg");
}

TEST(URITest, ParseParams) {
	uri u("/test.jpg?p&p1=test&p2=a&p2=b");
	ASSERT_EQ(u.get_path(), "/test.jpg");
	ASSERT_EQ(u.get_filename(), "test.jpg");
	ASSERT_EQ(u.get_extension(), "jpg");
	ASSERT_EQ(u.get_params("p").size(), 1);
	ASSERT_EQ(u.get_param("p1"), "test");
	auto p2 = u.get_params("p2");
	ASSERT_EQ(p2.size(), 2);
	ASSERT_TRUE(p2.count("a") != 0);
	ASSERT_TRUE(p2.count("b") != 0);
}

TEST(URITest, ThrowInvalid) {
	ASSERT_THROW(uri(""), invalid_uri_exception);
	ASSERT_THROW(uri("dss"), invalid_uri_exception);
}