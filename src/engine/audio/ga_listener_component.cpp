/*
** RPI Game Architecture Engine
**
** Portions adapted from:
** Viper Engine - Copyright (C) 2016 Velan Studios - All Rights Reserved
**
** This file is distributed under the MIT License. See LICENSE.txt.
*/

#include "ga_listener_component.h"
#include "graphics/ga_debug_geometry.h"
#include "graphics/ga_geometry.h"
#include "entity/ga_entity.h"

ga_listener_component::ga_listener_component(ga_entity* ent, SoLoud::Soloud* audioEngine,
	ga_physics_world* world) : ga_component(ent)
{
	_world = world;
	_audioEngine = audioEngine;
	update3DAudio();

	// Nodes
	std::vector<ga_vec3f> positions = world->getMeshCorners();
	for (int i = 0; i < positions.size(); ++i)
	{
		_sound_nodes.push_back(sound_node(positions[i]));
	}

	// Edges
	for (int i = 0; i < _sound_nodes.size(); ++i)
	{
		for (int j = i+1; j < _sound_nodes.size(); ++j)
		{
			ga_vec3f v = _sound_nodes[j]._pos - _sound_nodes[i]._pos;
			if (!_world->raycast_all(_sound_nodes[i]._pos, v.normal(), NULL, v.mag()))
			{
				_sound_nodes[i]._neighbors.push_back(&_sound_nodes[j]);
				_sound_nodes[j]._neighbors.push_back(&_sound_nodes[i]);
			}
		}
	}
}

ga_listener_component::~ga_listener_component()
{
}

void ga_listener_component::update(ga_frame_params* params)
{	
	ga_vec3f pos = get_entity()->get_transform().get_translation();
	float dt = std::chrono::duration_cast<std::chrono::duration<float>>(params->_delta_time).count();
	std::vector<ga_dynamic_drawcall> drawcalls;

	// Source occlusion
	for (int i = 0; i < _sources.size(); ++i)
	{
		// Determine if occluded
		ga_vec3f source_pos = _sources[i]->get_entity()->get_transform().get_translation();
		ga_vec3f to_source = source_pos - pos;
		float dist_to_source = to_source.mag();

		std::vector<ga_raycast_hit_info> info;
		_world->raycast_all(pos, to_source.normal(), &info);
		bool occluded = false;
		for (int j = 0; j < info.size(); ++j)
		{
			if (info[j]._dist <= dist_to_source)
			{
				occluded = true;
				break;
			}
		}

		// Adjust volume
		int handle = _sources[i]->getAudioHandle();
		float vol = _audioEngine->getVolume(handle);
		vol = occluded ? 0 : 1;
		_audioEngine->setVolume(handle, vol);
		
		
#if DEBUG_DRAW_AUDIO_SOURCES
		// Visualize raycast
		ga_dynamic_drawcall drawcall;
		ga_vec3f color = { 0, 1, 0 };
		if (occluded) color = { 1, 0, 0 };
		draw_debug_line(pos, source_pos, &drawcall, color);
		drawcalls.push_back(drawcall);
#endif
	}

	// Udpate panning / distance attenuation
	update3DAudio();
	
#if DEBUG_DRAW_AUDIO_SOURCES
	// Visualize listener
	ga_dynamic_drawcall drawcall;
	draw_debug_sphere(0.2f, get_entity()->get_transform(), &drawcall, { 0.4f, 0.549f, 1 });
	drawcalls.push_back(drawcall);

	// Visualize sound nodes
	ga_audio_component* vis_source = _sources[0];
	for (int i = 0; i < _sound_nodes.size(); ++i)
	{
		float dist_from_source = _sound_nodes[i]._propogation[vis_source];
		float str = 1.0f / (1 + dist_from_source / 2.0f);
		ga_vec3f color = { str, 0, 1 - str };

		for (int j = 0; j < _sound_nodes[i]._neighbors.size(); ++j)
		{
			ga_dynamic_drawcall drawcall;
			draw_debug_line(_sound_nodes[i]._pos, _sound_nodes[i]._neighbors[j]->_pos, &drawcall, color);
			drawcalls.push_back(drawcall);
		}
	}
#endif

	// Draw
	if (drawcalls.size() > 0)
	{
		while (params->_dynamic_drawcall_lock.test_and_set(std::memory_order_acquire)) {}
		for (int i = 0; i < drawcalls.size(); ++i)
		{
			params->_dynamic_drawcalls.push_back(drawcalls[i]);
		}
		params->_dynamic_drawcall_lock.clear(std::memory_order_release);
	}
}

void ga_listener_component::registerAudioSource(ga_audio_component* source)
{
	_sources.push_back(source);
	_sound_nodes[0].propogate(source, 0);
}

void ga_listener_component::update3DAudio()
{
	// Update source position (for distance attenuation / panning)
	ga_mat4f trans = get_entity()->get_transform();
	_audioEngine->set3dListenerPosition(
		trans.get_translation().x,
		trans.get_translation().y,
		trans.get_translation().z);

	_audioEngine->update3dAudio();
}



void sound_node::propogate(ga_audio_component* source, float initial_dist)
{
	std::queue<std::pair<sound_node*, float>> open_list;
	open_list.push(std::pair<sound_node*, float>(this, initial_dist));

	while (open_list.size() > 0)
	{
		sound_node* node = open_list.front().first;
		float dist = open_list.front().second;
		open_list.pop();

		// If node has not already been visited
		if (node->_propogation.count(source) == 0)
		{
			node->_propogation[source] = dist;
			for (int i = 0; i < node->_neighbors.size(); ++i)
			{
				float dist_to_neighbor = (_neighbors[i]->_pos - node->_pos).mag();
				open_list.push(std::pair<sound_node*, float>(
					node->_neighbors[i],
					dist + dist_to_neighbor));
			}
		}		
	}


	/*if (itr == _propogation.end() || itr->second > dist)
	{
		_propogation[source] = dist;
		for (int i = 0; i < _neighbors.size(); ++i)
		{
			_neighbors[i].propogate(source, dist + (_neighbors[i]._pos - _pos).mag());
		}
	}*/
}