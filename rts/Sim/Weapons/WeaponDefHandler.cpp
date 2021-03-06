/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include "WeaponDefHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Game/Game.h"
#include "Lua/LuaParser.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/TAPalette.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "System/FileSystem/FileHandler.h"
#include "System/LogOutput.h"
#include "Sound/ISound.h"
#include "System/Util.h"
#include "System/Exceptions.h"

using std::min;
using std::max;

CR_BIND(WeaponDef, );


CWeaponDefHandler* weaponDefHandler = NULL;


CWeaponDefHandler::CWeaponDefHandler()
{
	const LuaTable rootTable = game->defsParser->GetRoot().SubTable("WeaponDefs");
	if (!rootTable.IsValid()) {
		throw content_error("Error loading WeaponDefs");
	}

	vector<string> weaponNames;
	rootTable.GetKeys(weaponNames);

	numWeaponDefs = weaponNames.size();
	weaponDefs = new WeaponDef[numWeaponDefs];

	for (int wid = 0; wid < numWeaponDefs; wid++) {
		WeaponDef& wd = weaponDefs[wid];
		wd.id = wid;
		wd.name = weaponNames[wid];
		weaponID[wd.name] = wid;

		const LuaTable wdTable = rootTable.SubTable(wd.name);
		ParseWeapon(wdTable, wd);
	}
}


CWeaponDefHandler::~CWeaponDefHandler()
{
	delete[] weaponDefs;
}


void CWeaponDefHandler::ParseWeapon(const LuaTable& wdTable, WeaponDef& wd)
{
	bool manualBombSettings; //Allow the user to manually specify the burst and burstrate for his AircraftBomb
	int color;
	int color2;

	wd.tdfId = wdTable.GetInt("id", 0);

	wd.filename    = wdTable.GetString("filename", "unknown");
	wd.description = wdTable.GetString("name",     "Weapon");
	wd.cegTag      = wdTable.GetString("cegTag",   "");

	wd.avoidFriendly = wdTable.GetBool("avoidFriendly", true);
	wd.avoidFeature  = wdTable.GetBool("avoidFeature",  true);
	wd.avoidNeutral  = wdTable.GetBool("avoidNeutral",  false);

	wd.collisionFlags = 0;
	const bool collideFriendly = wdTable.GetBool("collideFriendly", true);
	const bool collideFeature  = wdTable.GetBool("collideFeature",  true);
	const bool collideNeutral  = wdTable.GetBool("collideNeutral",  true);
	if (!collideFriendly) { wd.collisionFlags |= COLLISION_NOFRIENDLY; }
	if (!collideFeature)  { wd.collisionFlags |= COLLISION_NOFEATURE;  }
	if (!collideNeutral)  { wd.collisionFlags |= COLLISION_NONEUTRAL;  }

	wd.minIntensity = wdTable.GetFloat("minIntensity", 0.0f);

	manualBombSettings = wdTable.GetBool("manualBombSettings", false);
	wd.turret   = wdTable.GetBool("turret",      false);
	wd.highTrajectory = wdTable.GetInt("highTrajectory", 2);
	wd.noSelfDamage   = wdTable.GetBool("noSelfDamage", false);
	wd.impactOnly     = wdTable.GetBool("impactOnly",   false);

	wd.waterweapon   = wdTable.GetBool("waterWeapon",     false);
	wd.fireSubmersed = wdTable.GetBool("fireSubmersed",   wd.waterweapon);
	wd.submissile    = wdTable.GetBool("submissile",      false);
	wd.tracks        = wdTable.GetBool("tracks",          false);
	wd.fixedLauncher = wdTable.GetBool("fixedLauncher",   false);
	wd.noExplode     = wdTable.GetBool("noExplode",       false);
	wd.isShield      = wdTable.GetBool("isShield",        false);
	wd.maxvelocity   = wdTable.GetFloat("weaponVelocity", 0.0f);
	wd.beamtime      = wdTable.GetFloat("beamTime",       1.0f);
	wd.beamburst     = wdTable.GetBool("beamburst",       false);

	wd.waterBounce   = wdTable.GetBool("waterBounce",    false);
	wd.groundBounce  = wdTable.GetBool("groundBounce",   false);
	wd.bounceSlip    = wdTable.GetFloat("bounceSlip",    1.0f);
	wd.bounceRebound = wdTable.GetFloat("bounceRebound", 1.0f);
	wd.numBounce     = wdTable.GetInt("numBounce",       -1);

	wd.thickness      = wdTable.GetFloat("thickness",      2.0f);
	wd.corethickness  = wdTable.GetFloat("coreThickness",  0.25f);
	wd.laserflaresize = wdTable.GetFloat("laserFlareSize", 15.0f);
	wd.intensity      = wdTable.GetFloat("intensity",      0.9f);
	wd.duration       = wdTable.GetFloat("duration",       0.05f);
	wd.falloffRate    = wdTable.GetFloat("fallOffRate",    0.5f);
	wd.lodDistance    = wdTable.GetInt("lodDistance",      1000);

	wd.visuals.modelName     = wdTable.GetString("model",       "");
	wd.visuals.explosionScar = wdTable.GetBool("explosionScar", true);
	wd.visuals.smokeTrail    = wdTable.GetBool("smokeTrail",    false);
	wd.visuals.alwaysVisible = wdTable.GetBool("alwaysVisible", false);
	wd.visuals.sizeDecay     = wdTable.GetFloat("sizeDecay",    0.0f);
	wd.visuals.alphaDecay    = wdTable.GetFloat("alphaDecay",   1.0f);
	wd.visuals.separation    = wdTable.GetFloat("separation",   1.0f);
	wd.visuals.noGap         = wdTable.GetBool("noGap",         true);
	wd.visuals.stages        = wdTable.GetInt("stages",         5);

	wd.gravityAffected = wdTable.GetBool("gravityAffected", false);

	wd.type = wdTable.GetString("weaponType", "Cannon");

	const bool melee = (wd.type == "Melee");
	wd.targetBorder = wdTable.GetFloat("targetBorder", melee ? 1.0f : 0.0f);
	if (wd.targetBorder > 1.0f) {
		logOutput.Print("warning: targetBorder truncated to 1 (was %f)", wd.targetBorder);
		wd.targetBorder = 1.0f;
	} else if (wd.targetBorder < -1.0f) {
		logOutput.Print("warning: targetBorder truncated to -1 (was %f)", wd.targetBorder);
		wd.targetBorder = -1.0f;
	}
	wd.cylinderTargetting = wdTable.GetFloat("cylinderTargetting", melee ? 1.0f : 0.0f);

	wd.range = wdTable.GetFloat("range", 10.0f);
	const float accuracy       = wdTable.GetFloat("accuracy",   0.0f);
	const float sprayAngle     = wdTable.GetFloat("sprayAngle", 0.0f);
	const float movingAccuracy = wdTable.GetFloat("movingAccuracy", accuracy);
	// should really be tan but TA seem to cap it somehow
	// should also be 7fff or ffff theoretically but neither seems good
	wd.accuracy       = sin((accuracy)       * PI / 0xafff);
	wd.sprayAngle     = sin((sprayAngle)     * PI / 0xafff);
	wd.movingAccuracy = sin((movingAccuracy) * PI / 0xafff);

	wd.targetMoveError = wdTable.GetFloat("targetMoveError", 0.0f);
	wd.leadLimit = wdTable.GetFloat("leadLimit", -1.0f);
	wd.leadBonus = wdTable.GetFloat("leadBonus", 0.0f);

	// setup the default damages
	const LuaTable dmgTable = wdTable.SubTable("damage");
	float defDamage = dmgTable.GetFloat("default", 0.0f);
	if (defDamage == 0.0f) {
		defDamage = 1.0f; //avoid division by zeroes
	}
	for (int a = 0; a < damageArrayHandler->GetNumTypes(); ++a) {
		wd.damages[a] = defDamage;
	}

	map<string, float> damages;
	dmgTable.GetMap(damages);

	map<string, float>::const_iterator di;
	for (di = damages.begin(); di != damages.end(); ++di) {
		const int type = damageArrayHandler->GetTypeFromName(di->first);
		if (type != 0) {
			float dmg = di->second;
			if (dmg != 0.0f) {
				wd.damages[type] = dmg;
			} else {
				wd.damages[type] = 1.0f;
			}
		}
	}

	wd.damages.impulseFactor = wdTable.GetFloat("impulseFactor", 1.0f);
	wd.damages.impulseBoost  = wdTable.GetFloat("impulseBoost",  0.0f);
	wd.damages.craterMult    = wdTable.GetFloat("craterMult",    wd.damages.impulseFactor);
	wd.damages.craterBoost   = wdTable.GetFloat("craterBoost",   0.0f);

	wd.areaOfEffect = wdTable.GetFloat("areaOfEffect", 8.0f) * 0.5f;
	wd.edgeEffectiveness = wdTable.GetFloat("edgeEffectiveness", 0.0f);
	// prevent 0/0 division in CGameHelper::Explosion
	if (wd.edgeEffectiveness > 0.999f) {
		wd.edgeEffectiveness = 0.999f;
	}

	wd.projectilespeed = wdTable.GetFloat("weaponVelocity", 0.0f) / GAME_SPEED;
	wd.startvelocity = max(0.01f, wdTable.GetFloat("startVelocity", 0.0f) / GAME_SPEED);
	wd.weaponacceleration = wdTable.GetFloat("weaponAcceleration", 0.0f) / GAME_SPEED / GAME_SPEED;
	wd.reload = wdTable.GetFloat("reloadTime", 1.0f);
	wd.salvodelay = wdTable.GetFloat("burstRate", 0.1f);
	wd.salvosize = wdTable.GetInt("burst", 1);
	wd.projectilespershot = wdTable.GetInt("projectiles", 1);
	wd.maxAngle = wdTable.GetFloat("tolerance", 3000.0f) * 180.0f / 0x7fff;
	wd.restTime = 0.0f;
	wd.metalcost = wdTable.GetFloat("metalPerShot", 0.0f);
	wd.energycost = wdTable.GetFloat("energyPerShot", 0.0f);
	wd.selfExplode = wdTable.GetBool("burnblow", false);
	wd.predictBoost = wdTable.GetFloat("predictBoost", wd.selfExplode ? 0.5f : 0.0f);
	wd.sweepFire = wdTable.GetBool("sweepfire", false);
	wd.canAttackGround = wdTable.GetBool("canAttackGround", true);
	wd.myGravity = wdTable.GetFloat("myGravity", 0.0f);

	wd.fireStarter = wdTable.GetFloat("fireStarter", 0.0f) * 0.01f;
	wd.paralyzer = wdTable.GetBool("paralyzer", false);
	if (wd.paralyzer) {
		wd.damages.paralyzeDamageTime = max(0, wdTable.GetInt("paralyzeTime", 10));
	} else {
		wd.damages.paralyzeDamageTime = 0;
	}

	const float defShake = wd.paralyzer ? 0.0f : wd.damages.GetDefaultDamage();
	wd.cameraShake = wdTable.GetFloat("cameraShake", defShake);
	wd.cameraShake = max(0.0f, wd.cameraShake);

	// 0.78.2.1 backwards compatibility
	bool defaultSoundTrigger = (wd.type == "BeamLaser" && !wd.beamburst) || 
								wd.type == "Melee"  || wd.type == "Rifle";
	wd.soundTrigger = wdTable.GetBool("soundTrigger", defaultSoundTrigger);

	//sunparser->GetDef(wd.highTrajectory, "0", weaponname + "minbarrelangle");
	wd.stockpile     = wdTable.GetBool("stockpile", false);
	wd.stockpileTime = wdTable.GetFloat("stockpileTime", wd.reload);
	wd.interceptor   = wdTable.GetInt("interceptor", 0);
	wd.targetable    = wdTable.GetInt("targetable",  0);
	wd.manualfire    = wdTable.GetBool("commandfire", false);
	wd.coverageRange = wdTable.GetFloat("coverage", 0.0f);

	// FIXME -- remove the old style ?
	LuaTable shTable = wdTable.SubTable("shield");
	const float3 shieldBadColor (1.0f, 0.5f, 0.5f);
	const float3 shieldGoodColor(0.5f, 0.5f, 1.0f);
	if (shTable.IsValid()) {
		wd.shieldRepulser         = shTable.GetBool("repulser",       false);
		wd.smartShield            = shTable.GetBool("smart",          false);
		wd.exteriorShield         = shTable.GetBool("exterior",       false);
		wd.visibleShield          = shTable.GetBool("visible",        false);
		wd.visibleShieldRepulse   = shTable.GetBool("visibleRepulse", false);
		wd.visibleShieldHitFrames = shTable.GetInt("visibleHitFrames", 0);
		wd.shieldEnergyUse        = shTable.GetFloat("energyUse",        0.0f);
		wd.shieldForce            = shTable.GetFloat("force",            0.0f);
		wd.shieldRadius           = shTable.GetFloat("radius",           0.0f);
		wd.shieldMaxSpeed         = shTable.GetFloat("maxSpeed",         0.0f);
		wd.shieldPower            = shTable.GetFloat("power",            0.0f);
		wd.shieldPowerRegen       = shTable.GetFloat("powerRegen",       0.0f);
		wd.shieldPowerRegenEnergy = shTable.GetFloat("powerRegenEnergy", 0.0f);
		wd.shieldRechargeDelay    = (int)(shTable.GetFloat("rechargeDelay", 0) * GAME_SPEED);
		wd.shieldStartingPower    = shTable.GetFloat("startingPower",    0.0f);
		wd.shieldInterceptType    = shTable.GetInt("interceptType", 0);
		wd.shieldBadColor         = shTable.GetFloat3("badColor",  shieldBadColor);
		wd.shieldGoodColor        = shTable.GetFloat3("goodColor", shieldGoodColor);
		wd.shieldAlpha            = shTable.GetFloat("alpha", 0.2f);
	}
	else {
		wd.shieldRepulser         = wdTable.GetBool("shieldRepulser",       false);
		wd.smartShield            = wdTable.GetBool("smartShield",          false);
		wd.exteriorShield         = wdTable.GetBool("exteriorShield",       false);
		wd.visibleShield          = wdTable.GetBool("visibleShield",        false);
		wd.visibleShieldRepulse   = wdTable.GetBool("visibleShieldRepulse", false);
		wd.visibleShieldHitFrames = wdTable.GetInt("visibleShieldHitFrames", 0);
		wd.shieldEnergyUse        = wdTable.GetFloat("shieldEnergyUse",        0.0f);
		wd.shieldForce            = wdTable.GetFloat("shieldForce",            0.0f);
		wd.shieldRadius           = wdTable.GetFloat("shieldRadius",           0.0f);
		wd.shieldMaxSpeed         = wdTable.GetFloat("shieldMaxSpeed",         0.0f);
		wd.shieldPower            = wdTable.GetFloat("shieldPower",            0.0f);
		wd.shieldPowerRegen       = wdTable.GetFloat("shieldPowerRegen",       0.0f);
		wd.shieldPowerRegenEnergy = wdTable.GetFloat("shieldPowerRegenEnergy", 0.0f);
		wd.shieldRechargeDelay    = (int)(wdTable.GetFloat("shieldRechargeDelay", 0) * GAME_SPEED);
		wd.shieldStartingPower    = wdTable.GetFloat("shieldStartingPower",    0.0f);
		wd.shieldInterceptType    = wdTable.GetInt("shieldInterceptType", 0);
		wd.shieldBadColor         = wdTable.GetFloat3("shieldBadColor",  shieldBadColor);
		wd.shieldGoodColor        = wdTable.GetFloat3("shieldGoodColor", shieldGoodColor);
		wd.shieldAlpha            = wdTable.GetFloat("shieldAlpha", 0.2f);
	}


	int defInterceptType = 0;
	if ((wd.type == "Cannon") || (wd.type == "EmgCannon")) {
		defInterceptType = 1;
	} else if ((wd.type == "LaserCannon") || (wd.type == "BeamLaser")) {
		defInterceptType = 2;
	} else if ((wd.type == "StarburstLauncher") || (wd.type == "MissileLauncher")) {
		defInterceptType = 4;
	} else if (wd.type == "AircraftBomb") {
		defInterceptType = 8;
	} else if (wd.type == "Flame") {
		defInterceptType = 16;
	} else if (wd.type == "TorpedoLauncher") {
		defInterceptType = 32;
	} else if (wd.type == "LightningCannon") {
		defInterceptType = 64;
	} else if (wd.type == "Rifle") {
		defInterceptType = 128;
	} else if (wd.type == "Melee") {
		defInterceptType = 256;
	}
	wd.interceptedByShieldType = wdTable.GetInt("interceptedByShieldType", defInterceptType);

	wd.wobble = wdTable.GetFloat("wobble", 0.0f) * PI / 0x7fff / 30.0f;
	wd.dance = wdTable.GetFloat("dance", 0.0f) / GAME_SPEED;
	wd.trajectoryHeight = wdTable.GetFloat("trajectoryHeight", 0.0f);

	wd.noAutoTarget = (wd.manualfire || wd.interceptor || wd.isShield);

	wd.onlyTargetCategory = 0xffffffff;
	if (wdTable.GetBool("toAirWeapon", false)) {
		// fix if we sometime call aircrafts otherwise
		wd.onlyTargetCategory = CCategoryHandler::Instance()->GetCategories("VTOL");
		//logOutput.Print("air only weapon %s %i",weaponname.c_str(),wd.onlyTargetCategory);
	}

	wd.largeBeamLaser = wdTable.GetBool("largeBeamLaser", false);
	wd.visuals.tilelength  = wdTable.GetFloat("tileLength", 200.0f);
	wd.visuals.scrollspeed = wdTable.GetFloat("scrollSpeed",  5.0f);
	wd.visuals.pulseSpeed  = wdTable.GetFloat("pulseSpeed",   1.0f);
	wd.visuals.beamdecay   = wdTable.GetFloat("beamDecay",    1.0f);
	wd.visuals.beamttl     = wdTable.GetInt("beamTTL", 0);

	if (wd.type == "Cannon") {
		wd.heightmod = wdTable.GetFloat("heightMod", 0.8f);
	} else if (wd.type == "BeamLaser" || wd.type == "LightningCannon") {
		wd.heightmod = wdTable.GetFloat("heightMod", 1.0f);
	} else {
		wd.heightmod = wdTable.GetFloat("heightMod", 0.2f);
	}

	wd.supplycost = 0.0f;

	wd.onlyForward = !wd.turret && (wd.type != "StarburstLauncher");

	color  = wdTable.GetInt("color",  0);
	color2 = wdTable.GetInt("color2", 0);

	const float3 rgbcol = hs2rgb(color / float(255), color2 / float(255));
	wd.visuals.color  = wdTable.GetFloat3("rgbColor",  rgbcol);
	wd.visuals.color2 = wdTable.GetFloat3("rgbColor2", float3(1.0f, 1.0f, 1.0f));

	wd.uptime = wdTable.GetFloat("weaponTimer", 0.0f);
	wd.flighttime = wdTable.GetFloat("flightTime", 0) * 32;

	wd.turnrate = wdTable.GetFloat("turnRate", 0.0f) * PI / 0x7fff / 30.0f;

	if ((wd.type == "AircraftBomb") && !manualBombSettings) {
		if (wd.reload < 0.5f) {
			wd.salvodelay = min(0.2f, wd.reload);
			wd.salvosize = (int)(1 / wd.salvodelay) + 1;
			wd.reload = 5;
		} else {
			wd.salvodelay = min(0.4f, wd.reload);
			wd.salvosize = 2;
		}
	}
	//if(!wd.turret && (wd.type != "TorpedoLauncher")) {
	//	wd.maxAngle*=0.4f;
	//}

	//2+min(damages[0]*0.0025f,weaponDef->areaOfEffect*0.1f)
	const float tempsize = 2.0f + min(wd.damages[0] * 0.0025f, wd.areaOfEffect * 0.1f);
	wd.size = wdTable.GetFloat("size", tempsize);
	wd.sizeGrowth = wdTable.GetFloat("sizeGrowth", 0.2f);
	wd.collisionSize = wdTable.GetFloat("collisionSize", 0.05f);

	wd.visuals.colorMap = 0;
	const string colormap = wdTable.GetString("colormap", "");
	if (colormap != "") {
		wd.visuals.colorMap = CColorMap::LoadFromDefString(colormap);
	}

	wd.heightBoostFactor = wdTable.GetFloat("heightBoostFactor", -1.0f);
	wd.proximityPriority = wdTable.GetFloat("proximityPriority", 1.0f);

	// get some weapon specific defaults
	if (wd.type == "Cannon") {
		// CExplosiveProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.9f);

		wd.visuals.color = wdTable.GetFloat3("rgbColor", float3(1.0f, 0.5f, 0.0f));
		wd.intensity = wdTable.GetFloat("intensity", 0.2f);
	} else if (wd.type == "Rifle") {
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.9f);
	} else if (wd.type == "Melee") {
		// ...
	} else if (wd.type == "Flame") {
		// CFlameProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.2f);
		wd.size              = wdTable.GetFloat("size",      tempsize);
		wd.sizeGrowth        = wdTable.GetFloat("sizeGrowth",    0.5f);
		wd.collisionSize     = wdTable.GetFloat("collisionSize", 0.5f);
		wd.duration          = wdTable.GetFloat("flameGfxTime",  1.2f);
	} else if (wd.type == "MissileLauncher") {
		// CMissileProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
	} else if (wd.type == "LaserCannon") {
		// CLaserProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
		wd.collisionSize = wdTable.GetFloat("collisionSize", 0.5f);

		wd.visuals.hardStop = wdTable.GetBool("hardstop", false);
	} else if (wd.type == "BeamLaser") {
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
	} else if (wd.type == "LightningCannon") {
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);

		wd.thickness = wdTable.GetFloat("thickness", 0.8f);
	} else if (wd.type == "EmgCannon") {
		// CEmgProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);

		wd.size = wdTable.GetFloat("size", 3.0f);
		wd.visuals.color = wdTable.GetFloat3("rgbColor", float3(0.9f, 0.9f, 0.2f));
	} else if (wd.type == "DGun") {
		// CFireBallProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
		wd.collisionSize = wdTable.GetFloat("collisionSize", 10.0f);
	} else if (wd.type == "StarburstLauncher") {
		// CStarburstProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
	} else {
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.0f);
	}


	const LuaTable texTable = wdTable.SubTable("textures");

	wd.visuals.texNames[0] = texTable.GetString(1, wdTable.GetString("texture1", ""));
	wd.visuals.texNames[1] = texTable.GetString(2, wdTable.GetString("texture2", ""));
	wd.visuals.texNames[2] = texTable.GetString(3, wdTable.GetString("texture3", ""));
	wd.visuals.texNames[3] = texTable.GetString(4, wdTable.GetString("texture4", ""));

	wd.visuals.expGenTag = wdTable.GetString("explosionGenerator", "");
	wd.visuals.bounceExpGenTag = wdTable.GetString("bounceExplosionGenerator", "");

	const float gd = max(30.0f, wd.damages[0] / 20.0f);
	const float defExpSpeed = (8.0f + (gd * 2.5f)) / (9.0f + (sqrt(gd) * 0.7f)) * 0.5f;
	wd.explosionSpeed = wdTable.GetFloat("explosionSpeed", defExpSpeed);

	// Dynamic Damage
	wd.dynDamageInverted = wdTable.GetBool("dynDamageInverted", false);
	wd.dynDamageExp      = wdTable.GetFloat("dynDamageExp",   0.0f);
	wd.dynDamageMin      = wdTable.GetFloat("dynDamageMin",   0.0f);
	wd.dynDamageRange    = wdTable.GetFloat("dynDamageRange", 0.0f);

	LoadSound(wdTable, wd.firesound, "start");
	LoadSound(wdTable, wd.soundhit,  "hit");

	if ((wd.firesound.getVolume(0) == -1.0f) ||
	    (wd.soundhit.getVolume(0)  == -1.0f)) {
		// no volume (-1.0f) read from weapon definition, set it dynamically here
		if (wd.damages[0] <= 50.0f) {
			wd.soundhit.setVolume(0, 5.0f);
			wd.firesound.setVolume(0, 5.0f);
		}
		else {
			float soundVolume = sqrt(wd.damages[0] * 0.5f);

			if (wd.type == "LaserCannon") {
				soundVolume *= 0.5f;
			}

			float hitSoundVolume = soundVolume;

			if ((soundVolume > 100.0f) &&
			    ((wd.type == "MissileLauncher") ||
			     (wd.type == "StarburstLauncher"))) {
				soundVolume = 10.0f * sqrt(soundVolume);
			}

			if (wd.firesound.getVolume(0) == -1.0f) {
				wd.firesound.setVolume(0, soundVolume);
			}

			soundVolume = hitSoundVolume;

			if (wd.areaOfEffect > 8.0f) {
				soundVolume *= 2.0f;
			}
			if (wd.type == "DGun") {
				soundVolume *= 0.15f;
			}
			if (wd.soundhit.getVolume(0) == -1.0f) {
				wd.soundhit.setVolume(0, soundVolume);
			}
		}
	}

	// custom parameters table
	wdTable.SubTable("customParams").GetMap(wd.customParams);
}


void CWeaponDefHandler::LoadSound(const LuaTable& wdTable,
                                  GuiSoundSet& gsound, const string& soundCat)
{
	string name = "";
	float volume = -1.0f;

	if (soundCat == "start") {
		name   = wdTable.GetString("soundStart", "");
		volume = wdTable.GetFloat("soundStartVolume", -1.0f);
	}
	else if (soundCat == "hit") {
		name   = wdTable.GetString("soundHit", "");
		volume = wdTable.GetFloat("soundHitVolume", -1.0f);
	}

	if (name != "") {
		const int id = LoadSoundFile(name);
		if (id > 0)
		{
			GuiSoundSet::Data soundData(name, 0, volume);
			gsound.sounds.push_back(soundData);
			gsound.setID(0, id);
		}
	}
}


const WeaponDef *CWeaponDefHandler::GetWeapon(const string& weaponname2)
{
	string weaponname(StringToLower(weaponname2));

	map<string,int>::iterator ii=weaponID.find(weaponname);
	if(ii == weaponID.end())
		return NULL;

	return &weaponDefs[ii->second];
}


const WeaponDef* CWeaponDefHandler::GetWeaponById(int weaponDefId)
{
	if ((weaponDefId < 0) || (weaponDefId >= numWeaponDefs)) {
		return NULL;
	}
	return &weaponDefs[weaponDefId];
}


float3 CWeaponDefHandler::hs2rgb(float h, float s)
{
	if(h>0.5f)
		h+=0.1f;
	if(h>1)
		h-=1;

	s=1;
	float invSat=1-s;
	float3 col(invSat/2,invSat/2,invSat/2);

	if(h<1/6.0f){
		col.x+=s;
		col.y+=s*(h*6);
	} else if(h<1/3.0f){
		col.y+=s;
		col.x+=s*((1/3.0f-h)*6);

	} else if(h<1/2.0f){
		col.y+=s;
		col.z+=s*((h-1/3.0f)*6);

	} else if(h<2/3.0f){
		col.z+=s;
		col.y+=s*((2/3.0f-h)*6);

	} else if(h<5/6.0f){
		col.z+=s;
		col.x+=s*((h-2/3.0f)*6);

	} else {
		col.x+=s;
		col.z+=s*((3/3.0f-h)*6);
	}
	return col;
}


DamageArray CWeaponDefHandler::DynamicDamages(DamageArray damages, float3 startPos, float3 curPos, float range, float exp, float damageMin, bool inverted)
{
	DamageArray dynDamages(damages);

	curPos.y = 0;
	startPos.y = 0;

	float travDist = (curPos-startPos).Length()>range?range:(curPos-startPos).Length();
	float ddmod = 0;

	if (damageMin > 0)
		ddmod = damageMin/damages[0]; // get damage mod from first damage type

	if (inverted == true) {
		for(int i = 0; i < damageArrayHandler->GetNumTypes(); ++i) {
			dynDamages[i] = damages[i] - (1 - pow(1 / range * travDist, exp)) * damages[i];

			if (damageMin > 0)
				dynDamages[i] = max(damages[i] * ddmod, dynDamages[i]);

			// div by 0
			dynDamages[i] = max(0.0001f, dynDamages[i]);
//			logOutput.Print("D%i: %f (%f) - mod %f", i, dynDamages[i], damages[i], ddmod);
//			logOutput.Print("F%i: %f - (1 - (1/%f * %f) ^ %f) * %f", i, damages[i], range, travDist, exp, damages[i]);
		}
	}
	else {
		for(int i = 0; i < damageArrayHandler->GetNumTypes(); ++i) {
			dynDamages[i] = (1 - pow(1 / range * travDist, exp)) * damages[i];

			if (damageMin > 0)
				dynDamages[i] = max(damages[i] * ddmod, dynDamages[i]);

			// div by 0
			dynDamages[i] = max(0.0001f, dynDamages[i]);
//			logOutput.Print("D%i: %f (%f) - mod %f", i, dynDamages[i], damages[i], ddmod);
//			logOutput.Print("F%i: (1 - (1/%f * %f) ^ %f) * %f", i, range, travDist, exp, damages[i]);
		}
	}
	return dynDamages;
}
