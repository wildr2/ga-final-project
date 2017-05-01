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
#include "soloud_biquadresonantfilter.h"
#include <iostream>


ga_listener_component::ga_listener_component(ga_entity* ent, SoLoud::Soloud* audio_engine,
	ga_physics_world* world) : ga_component(ent)
{
	_world = world;
	_audio_engine = audio_engine;
	update_3D_audio();

	SoLoud::BiquadResonantFilter low_pass_filter;
	low_pass_filter.setParams(SoLoud::BiquadResonantFilter::LOWPASS, 44100, MAX_LOWPASS_CUTOFF, 2);
	_audio_engine->setGlobalFilter(0, &low_pass_filter);


	// Nodes
	std::vector<ga_vec3f> corners = world->get_mesh_corners();
	std::vector<ga_vec3f> apart_corners = world->get_mesh_corners(0.05f);
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
		if (itr->second == 1 || itr->second == 3)
		{
			ga_vec3f node_pos = corner_to_apart_corner[itr->first];
			if (itr->second == 3) node_pos = itr->first; // use original corner position for concave corners
			if (node_pos.y < 0) node_pos += { 0, 0.1f, 0 }; // prevent edges between ground layer nodes
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
		float lowpass_cutoff = MAX_LOWPASS_CUTOFF;
		if (occluded)
		{
			ga_vec3f virtual_source = { MAX_AUDIO_DIST, MAX_AUDIO_DIST, MAX_AUDIO_DIST };
			ga_vec3f hear_dir = { 0, 0, 0 };
			float min_dist = MAX_AUDIO_DIST;
			for (int j = 0; j < _visible_sound_nodes.size(); ++j)
			{
				ga_vec3f node_to_listener = _visible_sound_nodes[j]->_pos - pos;
				float dist_from_source = _visible_sound_nodes[j]->_distance[_sources[i]] +
					node_to_listener.mag();

				// Find influence of incoming sound from this point in the environment (sound node)
				//   based on the distance of the node form the source, and the direction of the sound
				//   at the node
				float str = std::max(0.0f, (dist_from_source - MAX_AUDIO_DIST) / -MAX_AUDIO_DIST);
				float directness = node_to_listener.normal().dot(
					_visible_sound_nodes[j]->get_incoming_dir(_sources[i]));
				directness = 1 - (directness + 1) / 2.0f;
				str *= directness;

				// Update hear direction
				ga_vec3f dir = node_to_listener.normal().scale_result(str);
				hear_dir += dir;

				// Update minimum source to listener distance
				if (dist_from_source < min_dist)
				{
					min_dist = dist_from_source;
				}

#if DEBUG_DRAW_AUDIO
				if (_visible_sound_nodes.size() > 0)
				{
					// Visualize visible node LOS
					ga_dynamic_drawcall drawcall;
					ga_vec3f color = { 1 - str, str, 0 };
					draw_debug_line(pos, _visible_sound_nodes[j]->_pos, &drawcall, color);
					drawcalls.push_back(drawcall);
				}
#endif
			}

			if (hear_dir.mag() == 0)
			{
				// hear_dir is <0,0,0> when the listener is further than MAX_AUDIO_DIST
				//  away from the source, or is at the same position of the source (in which
				//  cases direction does not matter, but <0,0,0> is not a valid direction).
				hear_dir = { 1, 0, 0 };
			}
			else
			{
				hear_dir.normalize();
			}
			virtual_source = pos + hear_dir.scale_result(min_dist);

			// Use virtual source position
			_audio_engine->set3dSourcePosition(handle,
				virtual_source.x, virtual_source.y, virtual_source.z);

			// Update low pass filter cutoff - greater disparity between the shortest path from listener to source
			//   and the direct (occluded) path => a lower cutoff frequency
			float path_extension = std::pow((to_source.mag() / min_dist), 2.0f);
			lowpass_cutoff = path_extension * MAX_LOWPASS_CUTOFF;
			

#if DEBUG_DRAW_AUDIO
			if (_visible_sound_nodes.size() > 0)
			{
				// Visualize hear direction
				ga_dynamic_drawcall drawcall;
				float str = (min_dist - MAX_AUDIO_DIST) / -MAX_AUDIO_DIST;
				ga_vec3f color = { 1 - str, str, 0 };
				draw_debug_line(pos, virtual_source, &drawcall, color);
				drawcalls.push_back(drawcall);

				// Virtual source
				ga_dynamic_drawcall drawcall_vs;
				ga_mat4f tran;
				tran.make_translation(virtual_source);
				draw_debug_sphere(0.2f, tran, &drawcall_vs, { 1, 1, 1 });
				drawcalls.push_back(drawcall_vs);
			}
#endif
		}
		else // Not Occluded
		{
			// Use actual source position
			_audio_engine->set3dSourcePosition(handle,
				source_pos.x, source_pos.y, source_pos.z);
		}

		// Apply lowpass
		_audio_engine->setFilterParameter(0, 0, SoLoud::BiquadResonantFilter::FREQUENCY, lowpass_cutoff);
		std::cout << "lowpass cutoff: " << lowpass_cutoff << std::endl;
		
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
	update_3D_audio();

	
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
		itr = _sound_nodes[i]._distance.find(vis_source);
		float dist_from_source = itr == _sound_nodes[i]._distance.end() ?
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
		itr = _sound_nodes[i]._distance.find(vis_source);
		float dist_from_source = itr == _sound_nodes[i]._distance.end() ?
			std::numeric_limits<float>::max() : itr->second;

		// Draw lines to neighboring nodes
		for (int j = 0; j < _sound_nodes[i]._neighbors.size(); ++j)
		{
			sound_node* endpoint = _sound_nodes[i]._neighbors[j];
			if (visited[endpoint->_id]) continue;

			// Find distance from source for node2
			std::map<ga_audio_component*, float>::iterator itr2;
			itr2 = endpoint->_distance.find(vis_source);

			if (itr2 != endpoint->_distance.end())
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

void ga_listener_component::register_audio_source(ga_audio_component* source)
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

void ga_listener_component::update_3D_audio()
{
	// Update source position (for distance attenuation / panning)
	ga_mat4f trans = get_entity()->get_transform();
	_audio_engine->set3dListenerPosition(
		trans.get_translation().x,
		trans.get_translation().y,
		trans.get_translation().z);

	_audio_engine->update_3D_audio();
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



ga_vec3f sound_node::get_incoming_dir(ga_audio_component* source)
{
	sound_node* prev = _prev[source];
	if (prev == NULL)
	{
		ga_vec3f source_pos = source->get_entity()->get_transform().get_translation();
		return (_pos - source_pos).normal();
	}
	return (_pos - prev->_pos).normal();
}
void sound_node::propogate(ga_audio_component* source, float initial_dist)
{	
	ga_vec3f source_pos = source->get_entity()->get_transform().get_translation();

	std::queue<std::pair<sound_node*, sound_node*>> open_list; // node, prev node
	open_list.push(std::pair<sound_node*, sound_node*>(this, NULL));

	while (open_list.size() > 0)
	{
		sound_node* node = open_list.front().first;
		sound_node* prev = open_list.front().second;
		float dist;
		if (prev == NULL)
		{
			dist = initial_dist;
		}
		else
		{
			ga_vec3f prev_to_node = node->_pos - prev->_pos;
			sound_node* prev2 = prev->_prev[source];
			ga_vec3f prev2_pos = prev2 == NULL ? source_pos : prev2->_pos;
			ga_vec3f prev2_to_prev = prev->_pos - prev2_pos;

			dist = prev->_distance[source] + prev_to_node.mag();
			
			// Value between 0 and 1, where 1 is most bend from previous path
			/*float bend = 1 - (1 + prev_to_node.normal().dot(prev2_to_prev.normal())) / 2.0f;
			dist += bend * MAX_AUDIO_DIST;*/
		}

		open_list.pop();

		// If shorter path to node has not already been found, 
		//  update the distance to the node, and propogate to neighbors.
		if (node->_distance.count(source) == 0 ||
			node->_distance[source] > dist)
		{
			node->_distance[source] = dist;
			node->_prev[source] = prev;

			for (int i = 0; i < node->_neighbors.size(); ++i)
			{
				open_list.push(std::pair<sound_node*, sound_node*>(
					node->_neighbors[i], node));
			}
		}		
	}
}