/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LosHandlerMockObjects.h"
#include <cstdarg>
#include <cstdio>


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
