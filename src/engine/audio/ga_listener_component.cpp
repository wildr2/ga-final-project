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
	std::vector<ga_vec3f> corners = world->getMeshCorners();
	std::vector<ga_vec3f> apart_corners = world->getMeshCorners(0.05f);
	std::unordered_map<ga_vec3f, ga_vec3f> corner_to_apart_corner;
	std::unordered_map<ga_vec3f, int> corner_count;
	
	for (int i = 0; i < corners.size(); ++i)
	{
		corner_count[corners[i]]++;
		corner_to_apart_corner[corners[i]] = apart_corners[i];
	}
	std::unordered_map<ga_vec3f, int>::iterator itr;
	for (itr = corner_count.begin(); itr != corner_count.end(); ++itr)
	{
		// Create nodes for outer mesh corners (only one vertex at that location)
		if (itr->second > 0)
		{
			ga_vec3f node_pos = corner_to_apart_corner[itr->first];
			_sound_nodes.push_back(sound_node(_sound_nodes.size(), node_pos));
		}
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

	update_visible_sound_nodes(pos, drawcalls);

	// Source occlusion
	for (int i = 0; i < _sources.size(); ++i)
	{
		// Determine if occluded
		ga_vec3f source_pos = _sources[i]->get_entity()->get_transform().get_translation();
		ga_vec3f to_source = source_pos - pos;
		bool occluded = _world->raycast_all(pos, to_source.normal(), NULL, to_source.mag());

		// Set source position (virtual source position if occluded)
		int handle = _sources[i]->getAudioHandle();
		if (occluded)
		{
			ga_vec3f virtual_source = { MAX_AUDIO_DIST, MAX_AUDIO_DIST, MAX_AUDIO_DIST };
			float min_dist = MAX_AUDIO_DIST;
			for (int j = 0; j < _visible_sound_nodes.size(); ++j)
			{
				ga_vec3f node_to_listener = _visible_sound_nodes[j]->_pos - pos;
				float dist_from_source = _visible_sound_nodes[j]->_propogation[_sources[i]] +
					node_to_listener.mag();

				if (dist_from_source < min_dist)
				{
					min_dist = dist_from_source;
					virtual_source = pos + node_to_listener.normal()
						.scale_result(dist_from_source);
				}
			}

			// Use virtual source position
			_audioEngine->set3dSourcePosition(handle,
				virtual_source.x, virtual_source.y, virtual_source.z);

#if DEBUG_DRAW_AUDIO
			if (_visible_sound_nodes.size() > 0)
			{
				// Visualize node los
				ga_dynamic_drawcall drawcall;
				float str = (min_dist - MAX_AUDIO_DIST) / -MAX_AUDIO_DIST;
				ga_vec3f color = { 1 - str, str, 0 };
				draw_debug_line(pos, virtual_source, &drawcall, color);
				drawcalls.push_back(drawcall);
			}
#endif
		}
		else // Not Occluded
		{
			// Use actual source position
			_audioEngine->set3dSourcePosition(handle,
				source_pos.x, source_pos.y, source_pos.z);
		}
		
		
		
#if DEBUG_DRAW_AUDIO
		// Visualize raycast
		ga_dynamic_drawcall drawcall;
		float str = occluded ? 0 : ((source_pos-pos).mag() - MAX_AUDIO_DIST) / -MAX_AUDIO_DIST;
		ga_vec3f color = { 1 - str, str, 0 };
		draw_debug_line(pos, source_pos, &drawcall, color);
		drawcalls.push_back(drawcall);
#endif
	}
	

	// Udpate panning / distance attenuation
	update3DAudio();

	
#if DEBUG_DRAW_AUDIO
	// Visualize listener
	ga_dynamic_drawcall drawcall;
	draw_debug_sphere(0.2f, get_entity()->get_transform(), &drawcall, { 0.4f, 0.549f, 1 });
	drawcalls.push_back(drawcall);

	// Visualize sound nodes
	ga_audio_component* vis_source = _sources[0];
	for (int i = 0; i < _sound_nodes.size(); ++i)
	{
		// Find distance from source for node1
		std::map<ga_audio_component*, float>::iterator itr;
		itr = _sound_nodes[i]._propogation.find(vis_source);
		float dist_from_source = itr == _sound_nodes[i]._propogation.end() ?
			std::numeric_limits<float>::max() : itr->second;

		// Determine color of node:
		//	 Greater distance from the sound source to visualize -> more blue
		float str = (dist_from_source - MAX_AUDIO_DIST) / -MAX_AUDIO_DIST;
		ga_vec3f color = { 1 - str, str, 0 };

		ga_dynamic_drawcall drawcall;
		draw_debug_star(0.1f, _sound_nodes[i]._pos, &drawcall, color);
		drawcalls.push_back(drawcall);
	}

#if DEBUG_DRAW_SOUND_NODE_EDGES
	bool* visited = new bool[_sound_nodes.size()];
	for (int i = 0; i < _sound_nodes.size(); ++i)
	{
		visited[i] = false;
	}
	for (int i = 0; i < _sound_nodes.size(); ++i)
	{
		// Find distance from source for node1
		std::map<ga_audio_component*, float>::iterator itr;
		itr = _sound_nodes[i]._propogation.find(vis_source);
		float dist_from_source = itr == _sound_nodes[i]._propogation.end() ?
			std::numeric_limits<float>::max() : itr->second;

		// Draw lines to neighboring nodes
		for (int j = 0; j < _sound_nodes[i]._neighbors.size(); ++j)
		{
			sound_node* endpoint = _sound_nodes[i]._neighbors[j];
			if (visited[endpoint->_id]) continue;

			// Find distance from source for node2
			std::map<ga_audio_component*, float>::iterator itr2;
			itr2 = endpoint->_propogation.find(vis_source);

			if (itr2 != endpoint->_propogation.end())
			{
				dist_from_source = std::min(dist_from_source, itr2->second);
			}

			// Determine color of line to neighboring nodes based:
			// Greater distance from the sound source to visualize -> more blue
			float str = (dist_from_source - MAX_AUDIO_DIST) / -MAX_AUDIO_DIST;
			ga_vec3f color = { 1 - str, str, 0 };

			ga_dynamic_drawcall drawcall;
			draw_debug_line(_sound_nodes[i]._pos, endpoint->_pos, &drawcall, color);
			drawcalls.push_back(drawcall);
		}
		visited[_sound_nodes[i]._id] = true;
	}
#endif
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

	ga_vec3f source_pos = source->get_entity()->get_transform().get_translation();

	// Find sound nodes in LOS of source,
	//   and calculate distance from source at connected sound nodes.
	for (int i = 0; i < _sound_nodes.size(); ++i)
	{
		ga_vec3f v = _sound_nodes[i]._pos - source_pos;
		float dist = v.mag();

		if (!_world->raycast_all(source_pos, v.normal(), NULL, dist))
		{
			// Sound node in LOS
			_sound_nodes[i].propogate(source, dist);
		}
	}
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

void ga_listener_component::update_visible_sound_nodes(const ga_vec3f& pos,
	std::vector<ga_dynamic_drawcall>& drawcalls)
{
	_visible_sound_nodes.clear();

	// Find visible sound nodes
	for (int i = 0; i < _sound_nodes.size(); ++i)
	{
		ga_vec3f v = _sound_nodes[i]._pos - pos;
		if (!_world->raycast_all(pos, v.normal(), NULL, v.mag()))
		{
			// Sound node in LOS
			_visible_sound_nodes.push_back(&_sound_nodes[i]);
		}
	}
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
		if (node->_propogation.count(source) == 0 ||
			node->_propogation[source] > dist)
		{
			node->_propogation[source] = dist;
			for (int i = 0; i < node->_neighbors.size(); ++i)
			{
				float dist_to_neighbor = (node->_neighbors[i]->_pos - node->_pos).mag();
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