#ifndef CATUI_INSTALL_ROOT
#error Please define CATUI_INSTALL_ROOT for your target system
#endif

#include "gulachek/catui/handshake.hpp"

#include <gulachek/gtree.hpp>
#include <gulachek/gtree/encoding/string.hpp>
#include <gulachek/gtree/encoding/vector.hpp>
#include <gulachek/gtree/encoding/pair.hpp>

#include <gulachek/dictionary.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>

#include <sstream>
#include <vector>
#include <cstdlib>
#include <string_view>
#include <filesystem>

namespace gt = gulachek::gtree;

#define PP_SUCKS(X) # X
#define STR(X) PP_SUCKS(X)

namespace gulachek::catui
{
	using pair = std::pair<std::string, semver>;

	std::string_view handshake::protocol() const
	{ return protocol_; }

	const semver& handshake::version() const
	{ return version_; }

	static error connect_remote(std::shared_ptr<connection> *pconn)
	{
		error err;
		using ec = connect_error_code;

		auto type_c = std::getenv("GULACHEK_CATUI_ADDR_TYPE");
		if (!type_c)
		{
			err.ucode(ec::no_addr_type);
			err << "GULACHEK_CATUI_ADDR_TYPE not found";
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

		int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
		if (fd == -1)
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

		if (::connect(fd, (struct sockaddr *)&server, sizeof(server)) == -1)
		{
			err.ucode(ec::no_connect);
			err << "Failed to connect to " << addr << ": " << ::strerror(errno);
			return err;
		}

		*pconn = std::make_shared<connection>(fd, fd, true);
		return {};
	}

	static error exec(const std::vector<std::string> &argv)
	{
		if (argv.size() < 1)
			return {"Need at least a file path to exec"};

		std::vector<const char*> raw_argv;
		raw_argv.resize(argv.size() + 1); // null terminated

		for (std::size_t i = 0; i < argv.size(); ++i)
			raw_argv[i] = argv[i].c_str();

		raw_argv[argv.size()] = nullptr;

		if (::execv(raw_argv[0], (char *const *)raw_argv.data()))
		{
			error err;
			err << "execv: " << ::strerror(errno);
			return err;
		}

		return {};
	}

	error handshake::connect(std::shared_ptr<connection> *pconn) const
	{
		error err;
		using ec = connect_error_code;

		// hard coded into implementation. entire purpose of library is to
		// handle this protocol
		semver impl_catui_version{0, 1, 0};

		std::shared_ptr<connection> conn;

		auto version_c = ::getenv("GULACHEK_CATUI_VERSION");
		if (version_c)
		{
			semver catui_version;
			if (auto err = semver::parse(version_c, &catui_version))
			{
				auto wrap = err.wrap() << "Unable to parse semver GULACHEK_CATUI_VERSION";
				wrap.ucode(ec::catui_version_no_parse);
				return wrap;
			}

			if (!impl_catui_version.can_use(catui_version))
			{
				err.ucode(ec::catui_version_incompatible);
				err << "catui implementation (" << impl_catui_version <<
					") cannot use server's version (" << catui_version << ')';
				return err;
			}

			// connect to remote server
			if (auto err = connect_remote(&conn))
			{
				return err;
			}

			if (auto werr = conn->write(*this))
			{
				auto wrap = werr.wrap() << "Failed to write connection request";
				wrap.ucode(ec::no_ack);
				return wrap;
			}
		}
		else
		{
			std::filesystem::path root{STR(CATUI_INSTALL_ROOT)};
			auto major = std::to_string(version_.major());
			auto config = root / protocol_ / major / "config.gt";
			dictionary dict;

			if (auto err = gt::read_file(config, &dict))
			{
				auto wrap = err.wrap() << "Error reading installed config";
				wrap.ucode(ec::no_config);
				return wrap;
			}

			semver catui_version;
			if (auto err = dict.read("catui_version", &catui_version))
			{
				auto wrap = err.wrap() << "Unable to parse semver catui_version from config: " << config;
				wrap.ucode(ec::catui_version_no_parse);
				return wrap;
			}

			if (!impl_catui_version.can_use(catui_version))
			{
				err.ucode(ec::catui_version_incompatible);
				err << "catui implementation (" << impl_catui_version <<
					") cannot use installed version (" << catui_version << ')';
				return err;
			}

			semver installed_version;
			if (auto rerr = dict.read("version", &installed_version))
			{
				err.ucode(ec::no_version);
				err << "cannot read version from config " << config;
				return err;
			}

			if (!version_.can_use(installed_version))
			{
				err.ucode(ec::version_incompatible);
				err << protocol_ << " implementation (" << version_ <<
					") cannot use installed version (" << installed_version << ')';
				return err;
			}

			std::vector<std::string> execv;
			if (auto rerr = dict.read("exec", &execv))
			{
				err.ucode(ec::no_exec);
				err << "cannot read 'exec' from config " << config;
				return err;
			}

			int pipe_in[2], pipe_out[2];
			if (::pipe(pipe_in) || ::pipe(pipe_out))
			{
				err.ucode(ec::no_pipe);
				err << "pipe: " << ::strerror(errno);
				return err;
			}

			int pid = ::fork();
			if (pid < 0)
			{
				err.ucode(ec::no_fork);
				err << "fork: " << ::strerror(errno);
				return err;
			}
			else if (pid == 0)
			{
				if (::dup2(pipe_in[0], STDIN_FILENO) < 0)
				{
					::perror("dup2 stdin");
					std::exit(EXIT_FAILURE);
				}

				if (::dup2(pipe_out[1], STDOUT_FILENO) < 0)
				{
					::perror("dup2 stdout");
					std::exit(EXIT_FAILURE);
				}

				::close(pipe_in[1]);
				::close(pipe_out[0]);

				auto err = exec(execv);
				std::cerr << "Failed to exec platform: " << err << std::endl;
				std::exit(EXIT_FAILURE);
			}

			// parent process doesn't need these
			::close(pipe_in[0]);
			::close(pipe_out[1]);

			conn = std::make_shared<connection>(pipe_out[0], pipe_in[1], false);
		}

		std::string err_msg;
		if (auto rerr = conn->read(&err_msg))
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

		*pconn = std::move(conn);
		return {};
	}

	error handshake::gtree_encode(gtree::tree_writer &w) const
	{
		pair p{protocol_, version_};
		gtree::encoding<pair> enc{p};
		return enc.encode(w);
	}

	error handshake::gtree_decode(gtree::treeder &r)
	{
		pair p;
		gtree::decoding<pair> dec{&p};
		if (auto err = dec.decode(r))
			return err.wrap() << "failed to decode handshake";

		std::tie(protocol_, version_) = std::move(p);
		return {};
	}
}
