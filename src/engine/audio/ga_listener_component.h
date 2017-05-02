#pragma once

/*
** RPI Game Architecture Engine
**
** Portions adapted from:
** Viper Engine - Copyright (C) 2016 Velan Studios - All Rights Reserved
**
** This file is distributed under the MIT License. See LICENSE.txt.
*/

#include "entity/ga_component.h"
#include "ga_audio_component.h"
#include "physics/ga_physics_world.h"
#include "soloud.h"
#include "soloud_wav.h"

#include <map>
#include <queue>
#include <unordered_map>

#define MAX_AUDIO_DIST 20.0f
#define MAX_LOWPASS_CUTOFF 10000
#define DEBUG_DRAW_AUDIO 1
#define DEBUG_DRAW_SOUND_NODE_EDGES 0

/*
** A node in the 'nav-mesh' for sound. Stores the distances of shortest paths
** to audio components (sound sorces).
*/
class sound_node
{
public:
	sound_node(int id, ga_vec3f pos)
	{
		_id = id;
		_pos = pos;
	}
	
	/* Update values for shortest distance from the specified source to all connected
	 * nodes in a breadth first fashion. */
	void propogate(ga_audio_component* source, float dist);

	/* Get the direction of sound propogation from the specified source (direction
	*  from the previous node to this one in the path from the specified sound source). */
	ga_vec3f get_incoming_dir(ga_audio_component* source);

private:
	int _id;
	ga_vec3f _pos;
	std::vector<sound_node*> _neighbors;

	// distanceof shortest paths from sound sources
	std::map<ga_audio_component*, float> _distance;

	// previous node in shortest paths from sound sources
	std::map<ga_audio_component*, sound_node*> _prev; 


	friend class ga_listener_component;
};

/*
** Component for determining how sound from registered audio components should play;
** based on the listener and audio components' entitiy positions relative to static colliders in ga_physics_world.
*/
class ga_listener_component : public ga_component
{
public:
	ga_listener_component(class ga_entity* ent, SoLoud::Soloud* audio_engine, ga_physics_world* world);
	virtual ~ga_listener_component();

	/* Add an audio source so that the listener can contol how it sounds
	*   (distance attenuation, panning, filter affects and attenuation based on
	* position relative to geometry */
	void register_audio_source(ga_audio_component* source);
	
	virtual void update(struct ga_frame_params* params) override;

private:
	/* Update listener position in SoLoud (for distance attenuation / panning) */
	void update_3D_audio();

	/* Raycast to find which sound nodes have LOS to the listener (and visualise these rays) */
	void ga_listener_component::update_visible_sound_nodes(const ga_vec3f& pos,
		std::vector<ga_dynamic_drawcall>& drawcalls);

	/* Manipulate how registered audio components sound (volume attenuation, panning, and filtering) */
	void update_sources(std::vector<ga_dynamic_drawcall>& drawcalls);

	/* Return the position from which a sound source should be perceived as coming from, and
	   determine the distance of the shortest path to the sound source (min_dist). */
	ga_vec3f calc_virtual_source_pos(ga_audio_component* source, float* min_dist,
		std::vector<ga_dynamic_drawcall>& drawcalls);


	void debug_draw_listener(std::vector<ga_dynamic_drawcall>& drawcalls);

	void debug_draw_soundnodes(ga_audio_component* vis_source, std::vector<ga_dynamic_drawcall>& drawcalls);

	void ga_listener_component::debug_draw_soundnode_edges(ga_audio_component* vis_source,
		std::vector<ga_dynamic_drawcall>& drawcalls);



	SoLoud::Soloud* _audio_engine;
	ga_physics_world* _world;
	std::vector<ga_audio_component*> _sources; 

	// The sound propogation graph and its nodes currently with LOS to the listener 
	std::vector<sound_node> _sound_nodes;
	std::vector<sound_node*> _visible_sound_nodes;
};
