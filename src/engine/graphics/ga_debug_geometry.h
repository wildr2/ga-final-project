#pragma once

/*
** RPI Game Architecture Engine
**
** Portions adapted from:
** Viper Engine - Copyright (C) 2016 Velan Studios - All Rights Reserved
**
** This file is distributed under the MIT License. See LICENSE.txt.
*/

void draw_debug_sphere(float radius, const struct ga_mat4f& transform, struct ga_dynamic_drawcall* drawcall,
	const struct ga_vec3f& color);

void draw_debug_line(const struct ga_vec3f& p1, const struct ga_vec3f& p2, ga_dynamic_drawcall* drawcall,
	const struct ga_vec3f& color);