#define BOOST_TEST_MODULE SemverTest
#include <boost/test/unit_test.hpp>

#include <gulachek/catui/semver.hpp>
#include <tuple>
#include <sstream>

#include <gulachek/gtree/encoding/tuple.hpp>
#include <gulachek/gtree/encoding/unsigned.hpp>

using gulachek::catui::semver;

using tup = std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>;

namespace gt = gulachek::gtree;

BOOST_AUTO_TEST_CASE(VersionPieces)
{
	semver v{1, 2, 3};
	BOOST_TEST(v.major() == 1);
	BOOST_TEST(v.minor() == 2);
	BOOST_TEST(v.patch() == 3);
}

BOOST_AUTO_TEST_CASE(EncodedAsTuple)
{
	semver v{1, 2, 3};
	tup t;

	auto err = gt::translate(v, &t);
	BOOST_TEST(!err);
	BOOST_TEST(std::get<0>(t) == 1);
	BOOST_TEST(std::get<1>(t) == 2);
	BOOST_TEST(std::get<2>(t) == 3);
}

BOOST_AUTO_TEST_CASE(DecodedAsTuple)
{
	tup t{1, 2, 3};
	semver v;

	auto err = gt::translate(t, &v);
	BOOST_TEST(!err);
	BOOST_TEST(v.major() == 1);
	BOOST_TEST(v.minor() == 2);
	BOOST_TEST(v.patch() == 3);
}

BOOST_AUTO_TEST_CASE(VersionZeroMinorIncompatible)
{
	semver a{0, 1, 0};
	semver b{0, 2, 0};

	BOOST_TEST(!a.can_use(b));
	BOOST_TEST(!b.can_support(a));
}

BOOST_AUTO_TEST_CASE(VersionZeroPatchIncompatible)
{
	semver a{0, 1, 0};
	semver b{0, 1, 1};

	BOOST_TEST(!a.can_use(b));
	BOOST_TEST(!b.can_support(a));
}

BOOST_AUTO_TEST_CASE(MajorVersionMismatchIncompatible)
{
	semver a{1, 0, 0};
	semver b{2, 0, 0};

	BOOST_TEST(!a.can_use(b));
	BOOST_TEST(!b.can_support(a));
}

// api has all necessary features
BOOST_AUTO_TEST_CASE(MinorVersionGreaterIsCompatible)
{
	semver a{1, 0, 123};
	semver b{1, 1, 0};

	BOOST_TEST(a.can_use(b));
	BOOST_TEST(b.can_support(a));
}

// api missing a new feature
BOOST_AUTO_TEST_CASE(MinorVersionLessIsIncompatible)
{
	semver a{1, 2, 0};
	semver b{1, 1, 123};

	BOOST_TEST(!a.can_use(b));
	BOOST_TEST(!b.can_support(a));
}

// api has all necessary fixes
BOOST_AUTO_TEST_CASE(PatchVersionGreaterIsCompatible)
{
	semver a{1, 1, 0};
	semver b{1, 1, 1};

	BOOST_TEST(a.can_use(b));
	BOOST_TEST(b.can_support(a));
}

BOOST_AUTO_TEST_CASE(PatchVersionEqualIsCompatible)
{
	semver a{1, 1, 1};
	semver b{1, 1, 1};

	BOOST_TEST(a.can_use(b));
	BOOST_TEST(b.can_support(a));
}

// api missing crucial fix
BOOST_AUTO_TEST_CASE(PatchVersionLessIncompatible)
{
	semver a{1, 1, 2};
	semver b{1, 1, 1};

	BOOST_TEST(!a.can_use(b));
	BOOST_TEST(!b.can_support(a));
}

BOOST_AUTO_TEST_CASE(ParseBasicVersion)
{
	semver v;
	auto err = semver::parse("1.2.3", &v);

	BOOST_TEST(!err);
	BOOST_TEST(v.major() == 1);
	BOOST_TEST(v.minor() == 2);
	BOOST_TEST(v.patch() == 3);
}

BOOST_AUTO_TEST_CASE(ParseNegativeMajorIsError)
{
	semver v;
	auto err = semver::parse("-1.2.3", &v);

	BOOST_TEST(err);
}

BOOST_AUTO_TEST_CASE(ParseNegativeMinorIsError)
{
	semver v;
	auto err = semver::parse("1.-2.3", &v);

	BOOST_TEST(err);
}

BOOST_AUTO_TEST_CASE(ParseNegativePatchIsError)
{
	semver v;
	auto err = semver::parse("1.2.-3", &v);

	BOOST_TEST(err);
}

BOOST_AUTO_TEST_CASE(ParseIncompleteIsError)
{
	semver v;
	auto err = semver::parse("1.2", &v);

	BOOST_TEST(err);
}

BOOST_AUTO_TEST_CASE(ParseEmptyIsError)
{
	semver v;
	auto err = semver::parse("", &v);

	BOOST_TEST(err);
}

BOOST_AUTO_TEST_CASE(ParseHugeMajorIsError)
{
	semver v;
	auto err = semver::parse("99999999999.1.1", &v);

	BOOST_TEST(err);
}

BOOST_AUTO_TEST_CASE(FormatVersionInStream)
{
	semver v{1, 2, 3};
	std::ostringstream os;
	os << v;

	BOOST_TEST(os.str() == "1.2.3");
}

BOOST_AUTO_TEST_CASE(VersionsAreEqual)
{
	semver a{1, 2, 3};
	semver b{1, 2, 3};

	BOOST_TEST(a == b);
}

BOOST_AUTO_TEST_CASE(MajorVersionsAreNotEqual)
{
	semver a{1, 2, 3};
	semver b{0, 2, 3};

	BOOST_TEST(a != b);
}

BOOST_AUTO_TEST_CASE(MinorVersionIsLess)
{
	semver a{1, 1, 3};
	semver b{1, 2, 3};

	BOOST_TEST(a < b);
	BOOST_TEST(a <= b);
}

BOOST_AUTO_TEST_CASE(PatchVersionIsGreater)
{
	semver a{1, 2, 4};
	semver b{1, 2, 3};

	BOOST_TEST(a > b);
	BOOST_TEST(a >= b);
}
