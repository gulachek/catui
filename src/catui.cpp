#include "gulachek/catui.hpp"
#include "gulachek/catui/handshake.hpp"

#include <gulachek/gtree.hpp>
#include <gulachek/gtree/encoding/string.hpp>
#include <gulachek/gtree/encoding/pair.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>

#include <sstream>
#include <cstdlib>
#include <string_view>

namespace gt = gulachek::gtree;

namespace gulachek::catui
{
	error connect(const char *protocol, const semver &version, int *fd)
	{
		error err;
		using ec = connect_error_code;

		auto version_c = ::getenv("GULACHEK_CATUI_VERSION");
		if (!version_c)
		{
			err.ucode(ec::no_version);
			err << "GULACHEK_CATUI_VERSION not found";
			return err;
		}

		semver catui_version;
		if (auto err = semver::parse(version_c, &catui_version))
		{
			auto wrap = err.wrap() << "Unable to parse semver GULACHEK_CATUI_VERSION";
			wrap.ucode(ec::version_no_parse);
			return wrap;
		}

		auto type_c = std::getenv("GULACHEK_CATUI_ADDR_TYPE");
		if (!type_c)
		{
			err.ucode(ec::no_addr_type);
			err << "GULACHEK_CATUI_ADDR_TYPE not found";
			return err;
		}

		// hard coded into implementation. entire purpose of library is to
		// handle this protocol
		semver impl_catui_version{0, 1, 0};
		if (!impl_catui_version.can_use(catui_version))
		{
			err.ucode(ec::version_incompatible);
			err << "catui implementation (" << impl_catui_version <<
				") cannot use server's version (" << catui_version << ')';
			return err;
		}

		std::string_view type{type_c};

		if (type != "unix")
		{
			err.ucode(ec::unknown_type);
			err << "Unkown GULACHEK_CATUI_ADDR_TYPE: " << type;
			return err;
		}

		auto addr_c = std::getenv("GULACHEK_CATUI_ADDR");
		if (!addr_c)
		{
			err.ucode(ec::no_addr);
			err << "GULACHEK_CATUI_ADDR not found";
			return err;
		}
		std::string_view addr{addr_c};

		*fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
		if (*fd == -1)
		{
			err.ucode(ec::no_socket);
			err << "Failed to create socket" << ::strerror(errno);
			return err;
		}

		struct sockaddr_un server;
		::memset(&server, 0, sizeof(server));
		server.sun_family = AF_UNIX;
		::strlcpy(server.sun_path, addr_c, sizeof(server.sun_path));
		server.sun_len = sizeof(server.sun_len);

		if (::connect(*fd, (struct sockaddr *)&server, sizeof(server)) == -1)
		{
			err.ucode(ec::no_connect);
			err << "Failed to connect to " << addr << ": " << ::strerror(errno);
			return err;
		}

		std::pair<std::string, catui::semver> req;
		//handshake req;
		req.first = protocol;
		req.second = version;

		if (auto werr = gt::write_fd(*fd, req))
		{
			auto wrap = werr.wrap() << "Failed to write connection request";
			wrap.ucode(ec::no_ack);
			return wrap;
		}

		std::string err_msg;
		if (auto rerr = gtree::read_fd(*fd, &err_msg))
		{
			auto wrap = rerr.wrap() << "Failed to read acknowledgement from server";
			wrap.ucode(ec::no_ack);
			return wrap;
		}

		if (!err_msg.empty())
		{
			err.ucode(ec::rejected);
			err << "Server rejected connection: " << err_msg;
			return err;
		}

		return {};
	}
}
