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
		
		
		
		// Visualize raycast
#if DEBUG_DRAW_AUDIO_SOURCES
		ga_dynamic_drawcall drawcall;
		ga_vec3f color = { 0, 1, 0 };
		if (occluded) color = { 1, 0, 0 };
		draw_debug_line(pos, source_pos, &drawcall, color);
		drawcalls.push_back(drawcall);
#endif
	}

	// Udpate panning / distance attenuation
	update3DAudio();

	// Visualize listener
#if DEBUG_DRAW_AUDIO_SOURCES
	ga_dynamic_drawcall drawcall;
	draw_debug_sphere(0.2f, get_entity()->get_transform(), &drawcall, { 0.4f, 0.549f, 1 });
	drawcalls.push_back(drawcall);
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

