// Copyright © 2008-2023 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _CUSTOMSYSTEM_H
#define _CUSTOMSYSTEM_H

#include "Color.h"
#include "Polit.h"
#include "JsonFwd.h"
#include "galaxy/SystemBody.h"

#include "fixed.h"
#include "vector3.h"

class Faction;
class Galaxy;

class CustomSystemBody {
public:
	CustomSystemBody();
	~CustomSystemBody();

	std::string name;
	SystemBody::BodyType type;
	fixed radius; // in earth radii for planets, sol radii for stars (equatorial radius)
	fixed aspectRatio; // the ratio between equatorial radius and polar radius for bodies flattened due to equatorial bulge (1.0 to infinity)
	fixed mass; // earth masses or sol masses
	int averageTemp; // kelvin
	fixed semiMajorAxis; // in AUs
	fixed eccentricity;
	fixed orbitalOffset;
	fixed orbitalPhaseAtStart; // mean anomaly at start 0 to 2 pi
	fixed argOfPeriapsis;
	// TODO: these are only to be used for Lua system generation
	bool want_rand_offset;
	bool want_rand_phase;
	bool want_rand_arg_periapsis;
	// for orbiting things, latitude = inclination
	fixed inclination; // radians
	fixed longitude; // radians
	fixed rotationPeriod; // in days
	fixed rotationalPhaseAtStart; // 0 to 2 pi
	fixed axialTilt; // in radians
	std::string heightMapFilename;
	int heightMapFractal;
	// TODO: these two are separate implementations to handle Lua/Json based systems
	std::vector<uint32_t> childIndicies;
	std::vector<CustomSystemBody *> children;


	/* composition */
	fixed metallicity; // (crust) 0.0 = light (Al, SiO2, etc), 1.0 = heavy (Fe, heavy metals)
	fixed volatileGas; // 1.0 = earth atmosphere density
	fixed volatileLiquid; // 1.0 = 100% ocean cover (earth = 70%)
	fixed volatileIces; // 1.0 = 100% ice cover (earth = 3%)
	fixed volcanicity; // 0 = none, 1.0 = fucking volcanic
	fixed atmosOxidizing; // 0.0 = reducing (H2, NH3, etc), 1.0 = oxidising (CO2, O2, etc)
	fixed life; // 0.0 = dead, 1.0 = teeming

	double atmosDensity;
	Color atmosColor;

	fixed population;
	fixed agricultural;

	/* rings */
	enum RingStatus {
		WANT_RANDOM_RINGS,
		WANT_RINGS,
		WANT_NO_RINGS,
		WANT_CUSTOM_RINGS
	};
	RingStatus ringStatus;
	fixed ringInnerRadius;
	fixed ringOuterRadius;
	Color ringColor;

	Uint32 seed;
	bool want_rand_seed;
	std::string spaceStationType;

	void SanityChecks();

	void LoadFromJson(const Json &obj);

};

class CustomSystem {
public:

	static const int CUSTOM_ONLY_RADIUS = 4;
	CustomSystem();
	~CustomSystem();

	std::string name;
	std::vector<std::string> other_names;
	CustomSystemBody *sBody;
	// TODO: this holds system body objects when loaded from Json
	// This depends on serialized body order being exactly the same as
	// depth-first hierarchy traversal order.
	// Otherwise, subtle inconsistencies and outright wrong random generation
	// will creep in.
	// The fix is to fully deprecate the depth-first traversal order and
	// "flatten" StarSystemCustomGenerator::CustomGetKidsOf
	// TODO: this should act as storage for all bodies instead of holding ptrs
	std::vector<CustomSystemBody *> bodies;
	SystemBody::BodyType primaryType[4];
	unsigned numStars;
	int sectorX, sectorY, sectorZ;
	Uint32 systemIndex;
	vector3f pos;
	Uint32 seed;
	// NOTE: these are only intended to be used for Lua system generation
	bool want_rand_seed;
	bool want_rand_explored;
	bool explored;
	const Faction *faction;
	Polit::GovType govType;
	bool want_rand_lawlessness;
	fixed lawlessness; // 0.0 = lawful, 1.0 = totally lawless
	std::string shortDesc;
	std::string longDesc;

	void SanityChecks();

	bool IsRandom() const { return !sBody; }

	void LoadFromJson(const Json &systemdef);
	void SaveToJson(Json &obj);
};

class CustomSystemsDatabase {
public:
	CustomSystemsDatabase(Galaxy *galaxy, const std::string &customSysDir) :
		m_galaxy(galaxy),
		m_customSysDirectory(customSysDir) {}
	~CustomSystemsDatabase();

	void Load();

	const CustomSystem *LoadSystem(std::string_view filepath);

	const CustomSystem *LoadSystemFromJSON(std::string_view filename, const Json &systemdef);

	typedef std::vector<const CustomSystem *> SystemList;
	// XXX this is not as const-safe as it should be
	const SystemList &GetCustomSystemsForSector(int sectorX, int sectorY, int sectorZ) const;
	void AddCustomSystem(const SystemPath &path, CustomSystem *csys);
	Galaxy *GetGalaxy() const { return m_galaxy; }

	void RunLuaSystemSanityChecks(CustomSystem *csys);

private:
	typedef std::map<SystemPath, SystemList> SectorMap;
	typedef std::pair<SystemPath, size_t> SystemIndex;

	lua_State *CreateLoaderState();

	Galaxy *const m_galaxy;
	const std::string m_customSysDirectory;
	SectorMap m_sectorMap;
	SystemIndex m_lastAddedSystem;
	static const SystemList s_emptySystemList; // see: Null Object pattern
};

#endif /* _CUSTOMSYSTEM_H */
