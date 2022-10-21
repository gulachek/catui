#ifndef GULACHEK_CATUI_HPP
#define GULACHEK_CATUI_HPP

#include "gulachek/catui/semver.hpp"

#include <gulachek/error.hpp>

namespace gulachek::catui
{
	// user shouldn't care about specific reason
	// this is useful for testing code paths were hit
	enum class connect_error_code
	{
		success,
		no_version,
		version_no_parse,
		version_incompatible,
		no_addr_type,
		unknown_type,
		no_addr,
		no_socket,
		no_connect,
		no_ack,
		rejected
	};
	
	CATUI_API error connect(
			const char *protocol,
			const semver &version,
			int *fd
			);
}

#endif
