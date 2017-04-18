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

#define DEBUG_DRAW_AUDIO_SOURCES 1

/*
** Component which drives animation; updates skeleton and skinning matrices.
*/
class ga_kb_move_component : public ga_component
{
public:
	ga_kb_move_component(class ga_entity* ent, ga_button_t up, ga_button_t left,
		ga_button_t down, ga_button_t right, float speed = 5);
	virtual ~ga_kb_move_component();

	virtual void update(struct ga_frame_params* params) override;

private:
	ga_button_t _key_left;
	ga_button_t _key_right;
	ga_button_t _key_up;
	ga_button_t _key_down;
	float _speed;
};
