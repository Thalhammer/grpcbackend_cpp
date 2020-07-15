#include <gtest/gtest.h>
#include <grpcbackend/uri.h>

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

TEST(URITest, URLDecode) {
	const char table[] = "0123456789ABCDEF";
	for(char i=INT8_MIN; i<INT8_MAX; i++) {
		ASSERT_EQ(uri::url_decode(std::string("%") + table[(i>>4)&0x0f] + table[i&0x0f])[0], i);
	}

	ASSERT_EQ(uri::url_decode("hello%20world"), "hello world");
}

TEST(URITest, URLDecodeInvalid) {
	ASSERT_THROW(uri::url_decode("%1"), invalid_uri_exception);
	ASSERT_THROW(uri("%"), invalid_uri_exception);
	ASSERT_THROW(uri("%ii"), invalid_uri_exception);
}

TEST(URITest, URLEncode) {
	const char table[] = "0123456789abcdef";
	for(char i=INT8_MIN; i<INT8_MAX; i++) {
		if((i>='A' && i <='Z')
		|| (i>='a' && i<='z')
		|| (i>='0' && i<='9')
		|| i=='-' || i=='_' || i=='.' || i=='~') {
			ASSERT_EQ(uri::url_encode(std::string(1, i)), std::string(1, i));
		} else {
			ASSERT_EQ(uri::url_encode(std::string(1, i)), std::string("%") + table[(i>>4)&0x0f] + table[i&0x0f]);
		}
	}

	ASSERT_EQ(uri::url_encode("hello world"), "hello%20world");
}

TEST(URITest, FormDataParse) {
	auto sample = R"(test=%3Ca%20id%3D%22a%22%3E%3Cb%20id%3D%22b%22%3Ehey!%3C%2Fb%3E%3C%2Fa%3E&empty1&num=10&dup=1&dup=2&empty2)";
	auto parsed = uri::parse_formdata(sample);
	ASSERT_EQ(parsed.size(), 6);
	ASSERT_EQ(parsed.count("test"), 1);
	ASSERT_EQ(parsed.count("num"), 1);
	ASSERT_EQ(parsed.count("empty1"), 1);
	ASSERT_EQ(parsed.count("empty2"), 1);
	ASSERT_EQ(parsed.count("dup"), 2);
	ASSERT_EQ(parsed.find("test")->second, "<a id=\"a\"><b id=\"b\">hey!</b></a>");
	ASSERT_EQ(parsed.find("num")->second, "10");
	ASSERT_EQ(parsed.find("empty1")->second, "");
	ASSERT_EQ(parsed.find("empty2")->second, "");
	auto it = parsed.find("dup");
	if(it->second == "1") {
		it++;
		ASSERT_EQ(it->second, "2");
	} else if(it->second == "2") {
		it++;
		ASSERT_EQ(it->second, "1");
	} else {
		FAIL();
	}
}