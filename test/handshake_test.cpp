#define BOOST_TEST_MODULE HandshakeTest
#include <boost/test/unit_test.hpp>

#include <gulachek/catui/handshake.hpp>
#include <gulachek/gtree/encoding/string.hpp>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

#include <cstdlib>
#include <thread>
#include <future>

namespace catui = gulachek::catui;
namespace gt = gulachek::gtree;
using ec = catui::connect_error_code;

#define PP_SUCKS(X) # X
#define STR(X) PP_SUCKS(X)

struct unix_fixture
{
	int server_;
	std::shared_ptr<catui::connection> client_;
	const char *addr_;

	std::thread listener_;
	bool close_on_connect_ = false;
	std::string err_msg_;

	std::string recv_proto_;
	catui::semver recv_version_;

	void setup()
	{
		::setenv("GULACHEK_CATUI_VERSION", "0.1.0", 1);
		::setenv("GULACHEK_CATUI_ADDR_TYPE", "unix", 1);

		addr_ = STR(TEST_SOCK_ADDR);
		::setenv("GULACHEK_CATUI_ADDR", addr_, 1);

		::unlink(addr_);

		server_ = -1;
		client_ = nullptr;

		server_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
		BOOST_REQUIRE(server_ >= 0);

		struct sockaddr_un sockaddr;
		::memset(&sockaddr, 0, sizeof(sockaddr));
		sockaddr.sun_family = AF_UNIX;
		::strlcpy(sockaddr.sun_path, addr_, sizeof(sockaddr.sun_path));

		int bind_err = ::bind(server_, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
		BOOST_REQUIRE(bind_err == 0);

		auto listen_err = ::listen(server_, 10);
		BOOST_REQUIRE(listen_err == 0);

		listener_ = std::thread([this](){
				int client = ::accept(server_, NULL, NULL);
				if (client == -1)
				{
					return;
				}

				catui::handshake req;
				if (auto err = gt::read_fd(client, &req))
				{
					::close(client);
					return;
				}

				// do this after reading to save ourselves from SIGPIPE
				if (close_on_connect_)
				{
					::close(client);
					return;
				}

				recv_proto_ = req.protocol();
				recv_version_ = req.version();

				gt::write_fd(client, err_msg_);
				::close(client);
				return;
		});
	}

	void teardown()
	{
		::close(server_);
		::unlink(addr_);
		listener_.join();
	}

	auto connect_()
	{
		catui::handshake shake{"com.example.catui", {0,1,0}};
		auto err = shake.connect(&client_);
		return err;
	}

	auto connect_has_code_(auto uc)
	{
		auto err = connect_();
		BOOST_TEST(err.has_ucode(uc));
	}
};

BOOST_FIXTURE_TEST_SUITE(Unix, unix_fixture)

	BOOST_AUTO_TEST_CASE(NoVersion)
	{
		::unsetenv("GULACHEK_CATUI_VERSION");

		connect_has_code_(ec::no_version);
	}

	BOOST_AUTO_TEST_CASE(BadVersionScheme)
	{
		::setenv("GULACHEK_CATUI_VERSION", "cookie-monster", 1);

		connect_has_code_(ec::version_no_parse);
	}

	BOOST_AUTO_TEST_CASE(VersionUnreleasedMajorTooBig)
	{
		::setenv("GULACHEK_CATUI_VERSION", "0.2.0", 1);

		connect_has_code_(ec::version_incompatible);
	}

	BOOST_AUTO_TEST_CASE(NoAddrType)
	{
		::unsetenv("GULACHEK_CATUI_ADDR_TYPE");

		connect_has_code_(ec::no_addr_type);
	}

	BOOST_AUTO_TEST_CASE(UnknownType)
	{
		::setenv("GULACHEK_CATUI_ADDR_TYPE", "not-unix", 1);

		connect_has_code_(ec::unknown_type);
	}

	BOOST_AUTO_TEST_CASE(NoAddr)
	{
		::unsetenv("GULACHEK_CATUI_ADDR");

		connect_has_code_(ec::no_addr);
	}

	BOOST_AUTO_TEST_CASE(ConnectError)
	{
		::setenv("GULACHEK_CATUI_ADDR", "something egregious", 1);

		connect_has_code_(ec::no_connect);
	}

	BOOST_AUTO_TEST_CASE(CannotWriteRequest)
	{
		close_on_connect_ = true;

		connect_has_code_(ec::no_ack);
	}

	BOOST_AUTO_TEST_CASE(SendsProtocolAndVersion)
	{
		auto err = connect_();
		BOOST_TEST(!err);
		BOOST_TEST(recv_proto_ == "com.example.catui");
		BOOST_CHECK(recv_version_ == (catui::semver{0,1,0}));
	}

	BOOST_AUTO_TEST_CASE(ErrorMessageFromServer)
	{
		err_msg_ = "some error";
		connect_has_code_(ec::rejected);
	}

BOOST_AUTO_TEST_SUITE_END()
