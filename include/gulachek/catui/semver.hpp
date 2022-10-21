#ifndef GULACHEK_CATUI_SEMVER_HPP
#define GULACHEK_CATUI_SEMVER_HPP

#include <gulachek/gtree.hpp>
#include <gulachek/error.hpp>

#include <cstdint>
#include <ostream>
#include <string_view>
#include <compare>

namespace gulachek::catui
{
	// very basic semver implementation w/o prerelease/build extensions
	class CATUI_API semver
	{
		public:
			semver(
					std::uint32_t major = 0,
					std::uint32_t minor = 0,
					std::uint32_t patch = 0
					) :
				major_{major},
				minor_{minor},
				patch_{patch}
			{}

			GTREE_DECLARE_MEMBER_FNS;

			std::uint32_t major() const;
			std::uint32_t minor() const;
			std::uint32_t patch() const;

			static error parse(std::string_view sv, semver *out);

			bool can_use(const semver &api) const;
			bool can_support(const semver &application) const;

			std::strong_ordering operator <=> (const semver &rhs) const;
			bool operator == (const semver &rhs) const;

		private:
			std::uint32_t major_;
			std::uint32_t minor_;
			std::uint32_t patch_;
	};

	CATUI_API std::ostream& operator << (std::ostream &os, const gulachek::catui::semver &v);
}

#endif
