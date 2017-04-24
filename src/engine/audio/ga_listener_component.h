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
#include <unordered_set>

#define DEBUG_DRAW_AUDIO_SOURCES 1

class sound_node
{
public:
	sound_node(int id, ga_vec3f pos)
	{
		_id = id;
		_pos = pos;
	}

	void propogate(ga_audio_component* source, float dist);

	int _id;
	ga_vec3f _pos;
	std::vector<sound_node*> _neighbors;
	std::map<ga_audio_component*, float> _propogation;
};

/*
** Component which drives animation; updates skeleton and skinning matrices.
*/
class ga_listener_component : public ga_component
{
public:
	ga_listener_component(class ga_entity* ent, SoLoud::Soloud* audioEngine, ga_physics_world* world);
	virtual ~ga_listener_component();

	virtual void update(struct ga_frame_params* params) override;

	void registerAudioSource(ga_audio_component* source);

private:
	void update3DAudio();

	SoLoud::Soloud* _audioEngine;
	ga_physics_world* _world;
	std::vector<ga_audio_component*> _sources; 
	std::vector<sound_node> _sound_nodes;
};
