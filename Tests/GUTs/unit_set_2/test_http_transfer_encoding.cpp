/**
 * Created by TekuConcept on December 15, 2018
 */

#include <string>

#include <gtest/gtest.h>
#include "utils/environment.h"
#include "rfc/http/transfer_encoding.h"

using namespace impact;
using namespace http;


#define NO_THROW(code) try { {code} } catch (...) { FAIL(); }
#define THROW(code)    try { {code} FAIL(); } catch (...) { }

TEST(test_http_chunk_ext_token, create)
{
    NO_THROW(
        chunk_extension_token token("foo","bar");
        EXPECT_EQ(token.name(), "foo");
        EXPECT_EQ(token.value(), "bar");
    )
    
    THROW(chunk_extension_token token("/bad", "bar"););
    THROW(chunk_extension_token token("foo", "/bad"););
}


TEST(test_http_transfer_encoding, custom)
{
    NO_THROW( // user defined transfer encoding
        transfer_encoding coding(
            "foobar",
            [](const std::string& data) -> std::string { return data; }
        );
    )
    THROW( // reserved names
        transfer_encoding coding(
            "chunked",
            [](const std::string& data) -> std::string { return data; }
        );
    )
    THROW( // names must contain VCHARs
        transfer_encoding coding(
            " bad ",
            [](const std::string& data) -> std::string { return data; }
        );
    )
}


TEST(test_http_transfer_encoding, chunked_encoding)
{
    // chunked-body   = *chunk
    //                   last-chunk
    //                   trailer-part
    //                   CRLF
    // chunk          = chunk-size [ chunk-ext ] CRLF
    //                  chunk-data CRLF
    // chunk-size     = 1*HEXDIG
    // last-chunk     = 1*("0") [ chunk-ext ] CRLF
    // chunk-data     = 1*OCTET ; a sequence of chunk-size octets
    // chunk-ext      = *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
    // chunk-ext-name = token
    // chunk-ext-val  = token / quoted-string
    // trailer-part   = *( header-field CRLF )
    
    transfer_encoding_ptr chunked;
    
    // - vanilla -
    chunked = transfer_encoding::chunked();
    EXPECT_EQ(chunked->encode("Hello World!"), "c\r\nHello World!\r\n");
    EXPECT_EQ(chunked->encode(""), "0\r\n\r\n");
    
    // - with chunk extensions -
    std::vector<chunk_extension_token> extension_list = {
        chunk_extension_token("foo", "bar"),
        chunk_extension_token("baz", "\"qux\"")
    };
    chunked = transfer_encoding::chunked(
    [&](std::vector<chunk_extension_token>** extensions)->void {
        *extensions = &extension_list;
    });
    EXPECT_EQ(chunked->encode("Hello World!"),
        "c;foo=bar;baz=\"qux\"\r\nHello World!\r\n");
    EXPECT_EQ(chunked->encode(""),
        "0;foo=bar;baz=\"qux\"\r\n\r\n");
    
    // - with trailers -
    std::vector<header_token> header_list = {
        header_token("foo", "bar"),
        header_token("baz", "qux")
    };
    chunked = transfer_encoding::chunked(nullptr,
    [&](std::vector<header_token>** trailers)->void {
        *trailers = &header_list;
    });
    EXPECT_EQ(chunked->encode(""), "0\r\nfoo: bar\r\nbaz: qux\r\n\r\n");
    
    // - NULL extensions returned -
    chunked = transfer_encoding::chunked(
    [&](std::vector<chunk_extension_token>** extensions)->void {
        *extensions = NULL;
    });
    EXPECT_EQ(chunked->encode("Hello World!"), "c\r\nHello World!\r\n");
    
    // - NULL trailers returned -
    chunked = transfer_encoding::chunked(nullptr,
    [&](std::vector<header_token>** trailers)->void {
        *trailers = NULL;
    });
    EXPECT_EQ(chunked->encode(""), "0\r\n\r\n");
}


TEST(test_http_transfer_encoding, register_encoding)
{
    { transfer_encoding::register_encoding(nullptr); } // do nothing
    {
        // predefined encodings
        auto chunked = transfer_encoding::get_by_name("chunked");
        ASSERT_NE(chunked, nullptr);
        EXPECT_EQ(chunked->name(), "chunked");
    }
    {
        // user defined encodings
        auto special = transfer_encoding::get_by_name("special");
        EXPECT_EQ(special, nullptr);
        transfer_encoding::register_encoding(transfer_encoding_ptr(
            new transfer_encoding("special",
                [](const std::string& data) -> std::string { return data; })
        ));
        special = transfer_encoding::get_by_name("special");
        ASSERT_NE(special, nullptr);
        EXPECT_EQ(special->name(), "special");
    }
}
