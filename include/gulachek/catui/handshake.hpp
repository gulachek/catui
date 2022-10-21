#ifndef GULACHEK_CATUI_HANDSHAKE_HPP
#define GULACHEK_CATUI_HANDSHAKE_HPP

#include <gulachek/gtree.hpp>

#include <utility>
#include <string_view>

namespace gulachek::catui
{
	class handshake
	{
		public:
			GTREE_DECLARE_MEMBER_FNS;

			std::string_view protocol() const;

	};
}

#endif
