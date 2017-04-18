/*
** RPI Game Architecture Engine
**
** Portions adapted from:
** Viper Engine - Copyright (C) 2016 Velan Studios - All Rights Reserved
**
** This file is distributed under the MIT License. See LICENSE.txt.
*/

#include "ga_kb_move_component.h"
#include "entity/ga_entity.h"

ga_kb_move_component::ga_kb_move_component(ga_entity* ent, ga_button_t up, ga_button_t left,
	ga_button_t down, ga_button_t right, float speed) : ga_component(ent)
{
	_key_left = left;
	_key_right = right;
	_key_up = up;
	_key_down = down;
	_speed = speed;
}

ga_kb_move_component::~ga_kb_move_component()
{
}

void ga_kb_move_component::update(ga_frame_params* params)
{	
	float dt = std::chrono::duration_cast<std::chrono::duration<float>>(params->_delta_time).count();

	ga_vec3f moveDir = { 0, 0, 0 };
	moveDir.x =
		params->_button_mask & _key_left ? -1 :
		params->_button_mask & _key_right ? 1 : 0;
	moveDir.z =
		params->_button_mask & _key_up ? 1 :
		params->_button_mask & _key_down ? -1 : 0;

	get_entity()->translate(moveDir.scale_result(_speed * dt));
}


