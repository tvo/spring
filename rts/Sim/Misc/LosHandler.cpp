/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include <list>
#include <cstdlib>
#include <cstring>

#include "LosHandler.h"

#include "Sim/Units/Unit.h"
#include "Map/ReadMap.h"
#include "TimeProfiler.h"
#include "LogOutput.h"
#include "Platform/errorhandler.h"

using std::min;
using std::max;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CLosHandler* loshandler;


CLosHandler::CLosHandler(int numAllyTeams, int losMipLevel, int airMipLevel) :
	losMap(numAllyTeams),
	airLosMap(numAllyTeams),
	//airAlgo(int2(airSizeX, airSizeY), -1e6f, 15, readmap->mipHeightmap[airMipLevel]),
	losMipLevel(losMipLevel),
	airMipLevel(airMipLevel),
	losDiv(SQUARE_SIZE * (1 << losMipLevel)),
	airDiv(SQUARE_SIZE * (1 << airMipLevel)),
	invLosDiv(1.0f / losDiv),
	invAirDiv(1.0f / airDiv),
	airSizeX(std::max(1, readmap->width  >> airMipLevel)),
	airSizeY(std::max(1, readmap->height >> airMipLevel)),
	losSizeX(std::max(1, readmap->width  >> losMipLevel)),
	losSizeY(std::max(1, readmap->height >> losMipLevel)),
	requireSonarUnderWater(false),
	globalLOS(false),
	losAlgo(int2(losSizeX, losSizeY), -1e6f, 15, readmap->mipHeightmap[losMipLevel]),
	frameNum(0)
{
	for (int a = 0; a < numAllyTeams; ++a) {
		losMap[a].SetSize(losSizeX, losSizeY);
		airLosMap[a].SetSize(airSizeX, airSizeY);
	}
}


CLosHandler::~CLosHandler()
{
	std::list<LosInstance*>::iterator li;
	for(int a=0;a<2309;++a){
		for(li=instanceHash[a].begin();li!=instanceHash[a].end();++li){
			LosInstance* i = *li;
			i->_DestructInstance(i);
			mempool.Free(i, sizeof(LosInstance));
		}
	}

}


void CLosHandler::SetRequireSonarUnderWater(bool enabled)
{
	requireSonarUnderWater = enabled;
}


void CLosHandler::SetGlobalLOS(bool enabled)
{
	globalLOS = enabled;
}


void CLosHandler::MoveUnit(CUnit *unit, bool redoCurrent)
{
	SCOPED_TIMER("Los");
	const float3& losPos = unit->pos;

	const int allyteam = unit->allyteam;
	unit->lastLosUpdate = frameNum;

	if (unit->losRadius <= 0) {
		return;
	}

	const int baseX = max(0, min(losSizeX - 1, (int)(losPos.x * invLosDiv)));
	const int baseY = max(0, min(losSizeY - 1, (int)(losPos.z * invLosDiv)));
	const int baseSquare = baseY * losSizeX + baseX;
	const int baseAirX = max(0, min(airSizeX - 1, (int)(losPos.x * invAirDiv)));
	const int baseAirY = max(0, min(airSizeY - 1, (int)(losPos.z * invAirDiv)));

	LosInstance* instance;
	if (redoCurrent) {
		if (!unit->los) {
			return;
		}
		instance = unit->los;
		CleanupInstance(instance);
		instance->losSquares.clear();
		instance->basePos.x = baseX;
		instance->basePos.y = baseY;
		instance->baseSquare = baseSquare; //this could be a problem if several units are sharing the same instance
		instance->baseAirPos.x = baseAirX;
		instance->baseAirPos.y = baseAirY;
	} else {
		if (unit->los && (unit->los->baseSquare == baseSquare)) {
			return;
		}
		FreeInstance(unit->los);
		int hash = GetHashNum(unit);
		std::list<LosInstance*>::iterator lii;
		for (lii = instanceHash[hash].begin(); lii != instanceHash[hash].end(); ++lii) {
			if ((*lii)->baseSquare == baseSquare         &&
			    (*lii)->losSize    == unit->losRadius    &&
			    (*lii)->airLosSize == unit->airLosRadius &&
			    (*lii)->baseHeight == unit->losHeight    &&
			    (*lii)->allyteam   == allyteam) {
				AllocInstance(*lii);
				unit->los = *lii;
				return;
			}
		}
		instance=new(mempool.Alloc(sizeof(LosInstance))) LosInstance(unit->losRadius, unit->airLosRadius, allyteam, int2(baseX,baseY), baseSquare, int2(baseAirX, baseAirY), hash, unit->losHeight);
		instanceHash[hash].push_back(instance);
		unit->los=instance;
	}

	LosAdd(instance);
}


void CLosHandler::LosAdd(LosInstance* instance)
{
	assert(instance);
	assert(instance->allyteam < losMap.size());
	assert(instance->allyteam >= 0);

	losAlgo.LosAdd(instance->basePos, instance->losSize, instance->baseHeight, instance->losSquares);

	losMap[instance->allyteam].AddMapSquares(instance->losSquares, 1);
	airLosMap[instance->allyteam].AddMapArea(instance->baseAirPos, instance->airLosSize, 1);
}


void CLosHandler::FreeInstance(LosInstance* instance)
{
	if(instance==0)
		return;
	instance->refCount--;
	if(instance->refCount==0){
		CleanupInstance(instance);
		if(!instance->toBeDeleted){
			instance->toBeDeleted=true;
			toBeDeleted.push_back(instance);
		}
		if(instance->hashNum>=2310 || instance->hashNum<0){
			logOutput.Print("bad los");
		}
		if(toBeDeleted.size()>500){
			LosInstance* i=toBeDeleted.front();
			toBeDeleted.pop_front();
//			logOutput.Print("del %i",i->hashNum);
			if(i->hashNum>=2310 || i->hashNum<0){
				logOutput.Print("bad los 2");
				return;
			}
			i->toBeDeleted=false;
			if(i->refCount==0){
				std::list<LosInstance*>::iterator lii;
				for(lii=instanceHash[i->hashNum].begin();lii!=instanceHash[i->hashNum].end();++lii){
					if((*lii)==i){
						instanceHash[i->hashNum].erase(lii);
						i->_DestructInstance(i);
						mempool.Free(i,sizeof(LosInstance));
						break;
					}
				}
			}
		}
	}
}


int CLosHandler::GetHashNum(CUnit* unit)
{
	unsigned int t=unit->mapSquare*unit->losRadius+unit->allyteam;
	t^=*(unsigned int*)&unit->losHeight;
	return t%2309;
}


void CLosHandler::AllocInstance(LosInstance* instance)
{
	if (instance->refCount == 0) {
		LosAdd(instance);
	}
	instance->refCount++;
}


void CLosHandler::CleanupInstance(LosInstance* instance)
{
	losMap[instance->allyteam].AddMapSquares(instance->losSquares, -1);
	airLosMap[instance->allyteam].AddMapArea(instance->baseAirPos, instance->airLosSize, -1);
}


void CLosHandler::Update(void)
{
	++frameNum;

	while(!delayQue.empty() && delayQue.front().timeoutTime<frameNum){
		FreeInstance(delayQue.front().instance);
		delayQue.pop_front();
	}
}


void CLosHandler::DelayedFreeInstance(LosInstance* instance)
{
	DelayedInstance di;
	di.instance=instance;
	di.timeoutTime=frameNum+45;

	delayQue.push_back(di);
}
