#define BOOST_TEST_MODULE CatuiTest
#include <boost/test/unit_test.hpp>

#include "catui.h"

#include <boost/json.hpp>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <array>
#include <string>
#include <string_view>

using std::size_t;
using std::uint8_t;

struct f {
  int write_;
  int read_;

  f() {
    int fds[2];
    if (::pipe(fds) == -1) {
      BOOST_FAIL("Failed to allocate pipe");
      ::perror("pipe");
      return;
    }

    read_ = fds[0];
    write_ = fds[1];
  }

  ~f() {
    ::close(read_);
    ::close(write_);
  }
};

BOOST_AUTO_TEST_CASE(EncodedConnectRequestIsJson) {
  std::array<uint8_t, CATUI_CONNECT_SIZE> buf;

  catui_connect_request req;
  req.catui_version.major = 1;
  req.catui_version.minor = 2;
  req.catui_version.patch = 3;

  strcpy(req.protocol, "com.example.test");

  req.version.major = 4;
  req.version.minor = 5;
  req.version.patch = 6;

  size_t msgsz;
  int ok = catui_encode_connect(&req, buf.data(), buf.size(), &msgsz);

  BOOST_REQUIRE(ok);
  std::string_view jmsg{(char *)buf.data(), msgsz};

  auto msg = boost::json::parse(jmsg).as_object();

  BOOST_TEST(msg["catui-version"] == "1.2.3");
  BOOST_TEST(msg["protocol"] == "com.example.test");
  BOOST_TEST(msg["version"] == "4.5.6");
}

BOOST_AUTO_TEST_CASE(EncodingToSmallBufFails) {
  std::array<uint8_t, 50> buf; // doesn't leave enough room

  catui_connect_request req;
  req.catui_version.major = 1;
  req.catui_version.minor = 2;
  req.catui_version.patch = 3;

  strcpy(req.protocol, "com.example.test");

  req.version.major = 4;
  req.version.minor = 5;
  req.version.patch = 6;

  size_t msgsz;
  int ok = catui_encode_connect(&req, buf.data(), buf.size(), &msgsz);

  BOOST_TEST(!ok);
}

BOOST_AUTO_TEST_CASE(DecodedConnectFromRawJson) {
  std::string msg =
      R"({"catui-version": "1.2.3", "protocol": "com.example.test", "version": "4.5.7"  } )";

  catui_connect_request req;

  int ok = catui_decode_connect(msg.data(), msg.size(), &req);

  BOOST_REQUIRE(ok);
  BOOST_TEST(req.catui_version.major == 1);
  BOOST_TEST(req.catui_version.minor == 2);
  BOOST_TEST(req.catui_version.patch == 3);

  std::string_view protocol{req.protocol};
  BOOST_TEST(protocol == "com.example.test");

  BOOST_TEST(req.version.major == 4);
  BOOST_TEST(req.version.minor == 5);
  BOOST_TEST(req.version.patch == 7);
}
