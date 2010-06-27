/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/LosMap.h"

// Bit of a hack to prevent triggering a rebuild of the complete engine
// whenever LosHandlerMockObjects.h changes.
// (The cmake dependency mechanism doesn't interpret #ifdef..#endif)
#include "LosHandlerMockObjects.h"
#include "Sim/Misc/LosHandler.cpp"

#define BOOST_TEST_MODULE(loshandler)
#include <boost/test/included/unit_test.hpp>


namespace {
struct Fixture
{
	static const int numAllyTeams = 2;
	static const int losMipLevel = 1;
	static const int airMipLevel = 2;

	// make the map the minimum size where no mipmap has empty area
	static const int mapWidth = 1 << CMockReadMap::numHeightMipMaps;
	static const int mapHeight = mapWidth;

	/// Setup
	Fixture()
	{
		// This are all globals. Ugh.
		radarhandler = new CMockRadarHandler();
		readmap = new CMockReadMap(mapWidth, mapHeight);
		loshandler = new CLosHandler(numAllyTeams, losMipLevel, airMipLevel);
	}

	/// Teardown
	~Fixture()
	{
		delete loshandler;
		delete readmap;
		delete radarhandler;
	}
};
}


BOOST_FIXTURE_TEST_CASE(test_Update, Fixture)
{
	// Mostly tests that all (mock) objects have been initialised properly.
	loshandler->Update();
}


BOOST_FIXTURE_TEST_CASE(test_MoveUnit_InBounds, Fixture)
{
	CUnit u;
	u.losRadius = 10;
	// etc
}


BOOST_FIXTURE_TEST_CASE(test_MoveUnit_OutOfBounds, Fixture)
{
}
