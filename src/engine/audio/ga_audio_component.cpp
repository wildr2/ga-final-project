/*
** RPI Game Architecture Engine
**
** Portions adapted from:
** Viper Engine - Copyright (C) 2016 Velan Studios - All Rights Reserved
**
** This file is distributed under the MIT License. See LICENSE.txt.
*/

#include "ga_audio_component.h"
#include "graphics/ga_debug_geometry.h"
#include "graphics/ga_geometry.h"
#include "entity/ga_entity.h"

ga_audio_component::ga_audio_component(ga_entity* ent) : ga_component(ent)
{
}

ga_audio_component::~ga_audio_component()
{
}

void ga_audio_component::update(ga_frame_params* params)
{	
#if DEBUG_DRAW_AUDIO_SOURCES
	ga_dynamic_drawcall drawcall;
	draw_debug_sphere(0.2f, get_entity()->get_transform(), &drawcall);

	while (params->_dynamic_drawcall_lock.test_and_set(std::memory_order_acquire)) {}
	params->_dynamic_drawcalls.push_back(drawcall);
	params->_dynamic_drawcall_lock.clear(std::memory_order_release);
#endif
}

