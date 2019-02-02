// Copyright © 2008-2019 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _TERRAINBODY_H
#define _TERRAINBODY_H

#include "BaseSphere.h"
#include "Body.h"
#include "Camera.h"
#include "galaxy/StarSystem.h"

class Frame;
namespace Graphics {
	class Renderer;
}

class TerrainBody : public Body {
public:
	OBJDEF(TerrainBody, Body, TERRAINBODY);

	virtual void Render(Graphics::Renderer *r, const Camera *camera, const vector3d &viewCoords, const matrix4x4d &viewTransform) override;
	virtual void SubRender(Graphics::Renderer *r, const matrix4x4d &modelView, const vector3d &camPos) {}
	virtual void SetFrame(Frame *f) override;
	virtual bool OnCollision(Object *b, Uint32 flags, double relVel) override { return true; }
	virtual double GetMass() const override { return m_mass; }
	double GetTerrainHeight(const vector3d &pos) const;
	bool IsSuperType(SystemBody::BodySuperType t) const;
	virtual const SystemBody *GetSystemBody() const override { return m_sbody; }

	// returns value in metres
	double GetMaxFeatureRadius() const { return m_maxFeatureHeight; }

	// implements calls to all relevant terrain management sub-systems
	static void OnChangeDetailLevel();

protected:
	TerrainBody() = delete;
	TerrainBody(SystemBody *);
	TerrainBody(const Json &jsonObj, Space *space);
	virtual ~TerrainBody();

	void InitTerrainBody();

	virtual void SaveToJson(Json &jsonObj, Space *space) override;

private:
	const SystemBody *m_sbody;
	double m_mass;
	std::unique_ptr<BaseSphere> m_baseSphere;
	double m_maxFeatureHeight;
};

#endif
