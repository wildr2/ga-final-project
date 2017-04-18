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

ga_listener_component::ga_listener_component(ga_entity* ent, SoLoud::Soloud* audioEngine) : ga_component(ent)
{
	_audioEngine = audioEngine;
	update3DAudio();
}

ga_listener_component::~ga_listener_component()
{
}

void ga_listener_component::update(ga_frame_params* params)
{	
	float dt = std::chrono::duration_cast<std::chrono::duration<float>>(params->_delta_time).count();

	// Sound
	update3DAudio();

	// Visualization
#if DEBUG_DRAW_AUDIO_SOURCES
	ga_mat4f tran = get_entity()->get_transform();

	ga_dynamic_drawcall loc_drawcall, ray_drawcall;
	draw_debug_sphere(0.2f, tran, &loc_drawcall, { 0.4f, 0.549f, 1 });
	draw_debug_line(tran.get_translation(), { 0, 0, 0 }, &ray_drawcall, { 0, 1, 0 });

	while (params->_dynamic_drawcall_lock.test_and_set(std::memory_order_acquire)) {}
	params->_dynamic_drawcalls.push_back(loc_drawcall);
	params->_dynamic_drawcalls.push_back(ray_drawcall);
	params->_dynamic_drawcall_lock.clear(std::memory_order_release);
#endif
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

