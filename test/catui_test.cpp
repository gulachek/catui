#include "catui.h"

#include <cjson/cJSON.h>
#include <gtest/gtest.h>

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

std::string json_str_prop(cJSON *j, const char *prop) {
  cJSON *item = cJSON_GetObjectItem(j, prop);
  if (!item)
    return "";
  return cJSON_GetStringValue(item);
}

struct catui_req {
  std::string catui_version;
  std::string protocol;
  std::string version;

  template <typename String> catui_req(String &&s) {
    cJSON *json = cJSON_ParseWithLength(s.data(), s.size());
    if (!json) {
      ADD_FAILURE() << "Failed to parse JSON: "
                    << std::string_view{s.data(), s.size()};
      return;
    }

    catui_version = json_str_prop(json, "catui-version");
    protocol = json_str_prop(json, "protocol");
    version = json_str_prop(json, "version");

    cJSON_free(json);
  }
};

class f : public testing::Test {
protected:
  int write_;
  int read_;

  void SetUp() override {
    int fds[2];
    if (::pipe(fds) == -1) {
      ADD_FAILURE() << "Failed to allocate pipe";
      ::perror("pipe");
      return;
    }

    read_ = fds[0];
    write_ = fds[1];
  }

  void TearDown() override {
    ::close(read_);
    ::close(write_);
  }
};

TEST(Encoding, EncodedConnectRequestIsJson) {
  std::array<char, CATUI_CONNECT_SIZE> buf;

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

  ASSERT_TRUE(ok);
  catui_req msg{buf};

  EXPECT_EQ(msg.catui_version, "1.2.3");
  EXPECT_EQ(msg.protocol, "com.example.test");
  EXPECT_EQ(msg.version, "4.5.6");
}

TEST(Encoding, EncodingToSmallBufFails) {
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

  EXPECT_FALSE(ok);
}

TEST(Encoding, DecodedConnectFromRawJson) {
  std::string msg =
      R"({"catui-version": "1.2.3", "protocol": "com.example.test", "version":
"4.5.7"  } )";

  catui_connect_request req;

  int ok = catui_decode_connect(msg.data(), msg.size(), &req);

  ASSERT_TRUE(ok);
  EXPECT_EQ(req.catui_version.major, 1);
  EXPECT_EQ(req.catui_version.minor, 2);
  EXPECT_EQ(req.catui_version.patch, 3);

  std::string_view protocol{req.protocol};
  EXPECT_EQ(protocol, "com.example.test");

  EXPECT_EQ(req.version.major, 4);
  EXPECT_EQ(req.version.minor, 5);
  EXPECT_EQ(req.version.patch, 7);
}
