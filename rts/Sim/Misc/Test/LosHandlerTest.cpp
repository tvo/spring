/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// Bit of a hack to prevent triggering a rebuild of the complete engine
// whenever LosHandlerMockObjects.h changes.
// (The cmake dependency mechanism doesn't interpret #ifdef..#endif)
#include "../LosMap.h"
#include "LosHandlerMockObjects.h"
#include "../LosHandler.cpp"

// quick test of cmake
int main()
{
	return 0;
}
