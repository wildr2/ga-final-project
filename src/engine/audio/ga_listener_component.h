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

class sound_node
{
public:
	sound_node(int id, ga_vec3f pos)
	{
		_id = id;
		_pos = pos;
	}

	void propogate(ga_audio_component* source, float dist);
	ga_vec3f get_incoming_dir(ga_audio_component* source);

	int _id;
	ga_vec3f _pos;
	std::vector<sound_node*> _neighbors;
	std::map<ga_audio_component*, float> _distance;
	std::map<ga_audio_component*, sound_node*> _prev; 
};

/*
** Component which drives animation; updates skeleton and skinning matrices.
*/
class ga_listener_component : public ga_component
{
public:
	ga_listener_component(class ga_entity* ent, SoLoud::Soloud* audio_engine, ga_physics_world* world);
	virtual ~ga_listener_component();

	virtual void update(struct ga_frame_params* params) override;

	void register_audio_source(ga_audio_component* source);

private:
	void update_3D_audio();

	void ga_listener_component::update_visible_sound_nodes(const ga_vec3f& pos,
		std::vector<ga_dynamic_drawcall>& drawcalls);

	SoLoud::Soloud* _audio_engine;
	ga_physics_world* _world;
	std::vector<ga_audio_component*> _sources; 
	std::vector<sound_node> _sound_nodes;
	std::vector<sound_node*> _visible_sound_nodes;
};
