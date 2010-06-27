/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * Mock objects for compiling LosHandler.cpp and LosMap.cpp into a
 * standalone program for e.g. unittesting.
 */

#ifndef LOSHANDLERMOCKOBJECTS_H_
#define LOSHANDLERMOCKOBJECTS_H_

#include "float3.h"

// Forward declaration
struct LosInstance;

// Disable time profiler
#define SCOPED_TIMER(x)

// format string error checking
// FIXME: duplicated from LogOutput.h
#ifdef __GNUC__
#define FORMATSTRING(n) __attribute__((format(printf, n, n + 1)))
#else
#define FORMATSTRING(n)
#endif

/// Mock instance of logOutput
class CMockLogOutput
{
public:
	void Print(const char* fmt, ...) FORMATSTRING(2);
};
extern CMockLogOutput logOutput;

/// Mock class of CWorldObject
class CWorldObject
{
public:
	CWorldObject() :
		alwaysVisible(false),
		useAirLos(false)
	{
	}

	bool alwaysVisible;
	bool useAirLos;
	float3 pos;
};

/// Mock class of CUnit
class CUnit: public CWorldObject
{
public:
	CUnit() :
		isCloaked(false),
		isUnderWater(false),
		allyteam(0),
		lastLosUpdate(0),
		losRadius(0),
		airLosRadius(0),
		losHeight(0),
		mapSquare(0),
		los(NULL)
	{
	}

	bool isCloaked;
	bool isUnderWater;
	int allyteam;
	int lastLosUpdate;
	int losRadius;
	int airLosRadius;
	int losHeight;
	int mapSquare;
	LosInstance* los;
};

/// Mock instance of the readmap global
class CMockReadMap
{
public:
	CMockReadMap(int w, int h);
	~CMockReadMap();

	const int width;
	const int height;

    static const int numHeightMipMaps = 7; //! number of heightmap mipmaps, including full resolution
    float* mipHeightmap[numHeightMipMaps]; //! array of pointers to heightmap in different resolutions, mipHeightmap[0] is full resolution, mipHeightmap[n+1] is half resolution of mipHeightmap[n]
};
extern CMockReadMap* readmap;

/// Mock instance of the radarhandler global
class CMockRadarHandler
{
public:
	CMockRadarHandler() :
		inRadar(false)
	{
	}

	bool InRadar(const CUnit* unit, int allyTeam) const { return inRadar; }
	bool inRadar;
};
extern CMockRadarHandler* radarhandler;

#endif /* LOSHANDLERMOCKOBJECTS_H_ */
