#ifndef GULACHEK_CATUI_HANDSHAKE_HPP
#define GULACHEK_CATUI_HANDSHAKE_HPP

#include "gulachek/catui/semver.hpp"
#include "gulachek/catui/connection.hpp"

#include <gulachek/gtree.hpp>
#include <gulachek/error.hpp>

#include <utility>
#include <string_view>
#include <string>
#include <memory>

namespace gulachek::catui
{
	class CATUI_API handshake
	{
		public:
			handshake(
					const char *protocol = "",
					semver version = {}
					) :
				protocol_{protocol},
				version_{std::move(version)}
			{}

			GTREE_DECLARE_MEMBER_FNS;

			std::string_view protocol() const;
			const semver& version() const;

			error connect(std::shared_ptr<connection> *pconn) const;

			static bool is_protocol(std::string_view proto);

		private:
			std::string protocol_;
			semver version_;
	};

	// user shouldn't care about specific reason
	// this is useful for testing code paths were hit
	enum class connect_error_code
	{
		success,
		no_catui_version,
		catui_version_no_parse,
		catui_version_incompatible,
		no_addr_type,
		unknown_type,
		no_addr,
		no_socket,
		no_connect,
		no_ack,
		rejected,
		no_config,
		no_version,
		version_incompatible,
		no_exec,
		no_fork,
		no_pipe,
		bad_protocol
	};
}

#endif
