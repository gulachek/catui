#include "gulachek/catui/semver.hpp"

#include <gulachek/gtree/encoding/tuple.hpp>
#include <gulachek/gtree/encoding/unsigned.hpp>

#include <tuple>
#include <sstream>

using tup = std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>;

namespace gulachek::catui
{
	std::uint32_t semver::major() const
	{ return major_; }

	std::uint32_t semver::minor() const
	{ return minor_; }

	std::uint32_t semver::patch() const
	{ return patch_; }

	error semver::gtree_encode(gtree::tree_writer &w) const
	{
		tup t{major_, minor_, patch_};
		gtree::encoding<tup> enc{t};
		return enc.encode(w);
	}

	error semver::gtree_decode(gtree::treeder &r)
	{
		tup t;
		gtree::decoding<tup> dec{&t};
		if (auto err = dec.decode(r))
			return err.wrap() << "failed to decode semver";

		std::tie(major_, minor_, patch_) = t;
		return {};
	}

	bool semver::can_use(const semver &api) const
	{
		if (major_ != api.major_)
			return false;

		// version 0 is considered unstable
		if (major_ == 0)
			return minor_ == api.minor_ && patch_ == api.patch_;

		if (minor_ != api.minor_)
			return minor_ < api.minor_;

		return patch_ <= api.patch_;
	}

	bool semver::can_support(const semver &application) const
	{
		return application.can_use(*this);
	}

	error semver::parse(std::string_view sv, semver *v)
	{
		std::istringstream ss{std::string{sv}};

		std::int32_t major = -1, minor = -1, patch = -1;
		char dot = 0;

		ss >> major;
		ss >> dot;
		ss >> minor;
		ss >> dot;
		ss >> patch;

		if (major < 0 || minor < 0 || patch < 0)
			return {"invalid version numbers"};

		using u = std::uint32_t;
		*v = {(u)major, (u)minor, (u)patch};
		return {};
	}

	std::ostream& operator << (std::ostream &os, const gulachek::catui::semver &v)
	{
		return os << v.major() << '.' << v.minor() << '.' << v.patch();
	}

	std::strong_ordering semver::operator <=> (const semver &rhs) const
	{
		if (auto major = major_ <=> rhs.major_; major != 0)
		{
			return major;
		}

		if (auto minor = minor_ <=> rhs.minor_; minor != 0)
		{
			return minor;
		}

		if (auto patch = patch_ <=> rhs.patch_; patch != 0)
		{
			return patch;
		}

		return std::strong_ordering::equal;
	}

	bool semver::operator == (const semver &rhs) const
	{
		return (*this <=> rhs) == 0;
	}
}

