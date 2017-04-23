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
#include "physics/ga_rigid_body.h"
#include "physics/ga_shape.h"

ga_listener_component::ga_listener_component(ga_entity* ent, SoLoud::Soloud* audioEngine,
	ga_physics_world* world) : ga_component(ent)
{
	_world = world;
	_audioEngine = audioEngine;
	update3DAudio();

	// Nodes
	for (int i = 0; i < world->_bodies.size(); ++i)
	{
		ga_rigid_body* rb = world->_bodies[i];
		ga_shape* shape = rb->_shape;
		ga_oobb* cube = (ga_oobb*)shape;
		if (cube != NULL)
		{
			std::vector<ga_vec3f> corners;
			cube->get_corners(corners);
			for (int j = 0; j < corners.size(); ++j)
			{
				ga_vec3f pos = corners[j] + (corners[j] - cube->_center).normal().scale_result(0.1f);
				pos = rb->_transform.transform_point(pos);
				_sound_nodes.push_back(sound_node(pos));
			}
		}
	}
	

	/*sound_node sn1 = sound_node({ -2.2f, 1, 2.2f });
	sound_node sn2 = sound_node({ 2.2f, 1, 2.2f });
	sound_node sn3 = sound_node({ 2.2f, 1, -2.2f });
	sound_node sn4 = sound_node({ -2.2f, 1, -2.2f });

	sn1._neighbors = { sn2, sn4 };
	sn2._neighbors = { sn1, sn3 };
	sn3._neighbors = { sn2, sn4 };
	sn4._neighbors = { sn1, sn3 };

	_sound_nodes = { sn1, sn2, sn3, sn4 };*/

	/*_sound_nodes.push_back(sound_node({ -5, 1, 5 }));
	_sound_nodes.push_back(sound_node({ 5, 1, 5 }));
	_sound_nodes.push_back(sound_node({ 5, 1, -5 }));
	_sound_nodes.push_back(sound_node({ -5, 1, -5 }));*/

	for (int i = 0; i < _sound_nodes.size(); ++i)
	{
		for (int j = i; j < _sound_nodes.size(); ++j)
		{
			_sound_nodes[i]._neighbors.push_back(_sound_nodes[j]);
			_sound_nodes[j]._neighbors.push_back(_sound_nodes[i]);

			ga_vec3f v = _sound_nodes[j]._pos - _sound_nodes[i]._pos;
			if (!_world->raycast_all(_sound_nodes[i]._pos, v.normal(), NULL, v.mag()))
			{
				_sound_nodes[i]._neighbors.push_back(_sound_nodes[j]);
				_sound_nodes[j]._neighbors.push_back(_sound_nodes[i]);
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
		ga_vec3f source_pos = _sources[i]->get_entity()->get_transform().get_translation();
		std::vector<ga_raycast_hit_info> info;
		bool hit = _world->raycast_all(pos, source_pos - pos, &info);

		// Show raycast
#if DEBUG_DRAW_AUDIO_SOURCES
		ga_dynamic_drawcall drawcall;
		ga_vec3f color = { 0, 1, 0 };
		if (hit) color = { 1, 0, 0 };
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
	for (int i = 0; i < _sound_nodes.size(); ++i)
	{
		for (int j = 0; j < _sound_nodes[i]._neighbors.size(); ++j)
		{
			ga_dynamic_drawcall drawcall;
			draw_debug_line(_sound_nodes[i]._pos, _sound_nodes[i]._neighbors[j]._pos, &drawcall, { 0, 0, 1 });
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
