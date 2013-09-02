/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef _TRACK_H
#define _TRACK_H

#include "roadstrip.h"
#include "mathvector.h"
#include "quaternion.h"
#include "graphics/scenenode.h"
#include "physics/motionstate.h"
#include "physics/tracksurface.h"

#include <iosfwd>
#include <list>
#include <memory>
#include <vector>

class ROADSTRIP;
class DynamicsWorld;
class ContentManager;
class btStridingMeshInterface;
class btCollisionShape;
class btCollisionObject;

class TRACK
{
public:
	TRACK();
	~TRACK();

	/// Only begins loading the track.
    /// The track won't be loaded until more calls to ContinueDeferredLoad().
    /// Use Loaded() to see if loading is complete yet.
    /// Returns true if successful.
	bool DeferredLoad(
		ContentManager & content,
		DynamicsWorld & world,
		std::ostream & info_output,
		std::ostream & error_output,
		const std::string & trackpath,
		const std::string & trackdir,
		const std::string & effects_texturepath,
		const std::string & sharedobjectpath,
		const int anisotropy,
		const bool reverse,
		const bool dynamicobjects,
		const bool dynamicshadowsenabled,
		const bool doagressivecombining);

	bool ContinueDeferredLoad();

	/// Start loading thread.
	void Load();

	/// Number of objects to load in total.
	int ObjectsNum() const;

	/// Number of objects loaded.
	int ObjectsNumLoaded() const;

	/// Track loading status.
	bool Loaded() const;

	void Clear();

	bool CastRay(
		const MATHVECTOR <float, 3> & origin,
		const MATHVECTOR <float, 3> & direction,
		const float seglen,
		int & patch_id,
		MATHVECTOR <float, 3> & outtri,
		const BEZIER * & colpatch,
		MATHVECTOR <float, 3> & normal) const;

	/// Synchronize graphics and physics.
	void Update();

	std::pair <MATHVECTOR <float, 3>, QUATERNION <float> > GetStart(unsigned int index) const;

	int GetNumStartPositions() const
	{
		return data.start_positions.size();
	}

	const std::list <ROADSTRIP> & GetRoadList() const
	{
		return data.roads;
	}

	unsigned int GetSectors() const
	{
		return data.lap.size();
	}

	const BEZIER * GetSectorPatch(unsigned int sector) const
	{
		assert (sector < data.lap.size());
		return data.lap[sector];
	}

	void SetRacingLineVisibility(bool newvis)
	{
		racingline_visible = newvis;
	}

	bool IsReversed() const
	{
		return data.reverse;
	}

	const std::vector<TRACKSURFACE> & GetSurfaces() const
	{
		return data.surfaces;
	}

	SCENENODE & GetRacinglineNode()
	{
		if (racingline_visible)
			return data.racingline_node;
		else
			return empty_node;
	}

	SCENENODE & GetTrackNode()
	{
		return data.static_node;
	}

	SCENENODE & GetBodyNode()
	{
		return data.dynamic_node;
	}

private:
	struct DATA
	{
		DynamicsWorld* world;

		// static track objects
		SCENENODE static_node;
		std::vector<TRACKSURFACE> surfaces;
		std::vector<std::tr1::shared_ptr<MODEL> > models;
		std::vector<btStridingMeshInterface*> meshes;
		std::vector<btCollisionShape*> shapes;
		std::vector<btCollisionObject*> objects;

		// dynamic track objects
		SCENENODE dynamic_node;
		std::vector<keyed_container<SCENENODE>::handle> body_nodes;
		std::list<MotionState> body_transforms;

		// road information
		std::vector<const BEZIER*> lap;
		std::list<ROADSTRIP> roads;
		std::vector<std::pair<MATHVECTOR<float, 3>, QUATERNION<float> > > start_positions;

		// racing line data
		SCENENODE racingline_node;
		std::tr1::shared_ptr<TEXTURE> racingline_texture;

		// track state
		bool reverse;
		bool loaded;
		bool cull;

		DATA();
	};

	DATA data;
	bool racingline_visible;
	SCENENODE empty_node;

	// temporary loading data
	class LOADER;
	std::auto_ptr<LOADER> loader;
};

#endif
