/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LosHandlerMockObjects.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>


CMockLogOutput logOutput;
CMockReadMap* readmap;
CMockRadarHandler* radarhandler;


void CMockLogOutput::Print(const char* fmt, ...)
{
        va_list argp;

        va_start(argp, fmt);
        vfprintf(stderr, fmt, argp);
        va_end(argp);
}


CMockReadMap::CMockReadMap(int w, int h) :
	width(w),
	height(h)
{
	for (int i = 0; i < numHeightMipMaps; ++i) {
		const int size = (width >> i) * (height >> i);
		mipHeightmap[i] = new float[size];
		std::memset(mipHeightmap[i], 0, sizeof(float) * size);
	}
}


CMockReadMap::~CMockReadMap()
{
	for (int i = 0; i < numHeightMipMaps; ++i) {
		delete[] mipHeightmap[i];
	}
}
