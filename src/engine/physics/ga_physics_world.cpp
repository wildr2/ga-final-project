/*
** RPI Game Architecture Engine
**
** Portions adapted from:
** Viper Engine - Copyright (C) 2016 Velan Studios - All Rights Reserved
**
** This file is distributed under the MIT License. See LICENSE.txt.
*/

#include "ga_physics_world.h"
#include "ga_rigid_body.h"
#include "ga_shape.h"

#include "framework/ga_drawcall.h"
#include "framework/ga_frame_params.h"

#include <algorithm>
#include <assert.h>
#include <ctime>

typedef bool (*intersection_func_t)(const ga_shape* a, const ga_mat4f& transform_a, const ga_shape* b, const ga_mat4f& transform_b, ga_collision_info* info);
typedef bool (*intersect_ray_func_t)(const ga_vec3f& ray_origin, const ga_vec3f& ray_dir, const ga_shape* shape, const ga_mat4f& transform, float* dist);

static intersection_func_t k_dispatch_table[k_shape_count][k_shape_count];
static intersect_ray_func_t k_ray_dispatch_table[k_shape_count];

ga_physics_world::ga_physics_world()
{
	// Clear the dispatch tables.
	for (int i = 0; i < k_shape_count; ++i)
	{
		for (int j = 0; j < k_shape_count; ++j)
		{
			k_dispatch_table[i][j] = intersection_unimplemented;
		}
		k_ray_dispatch_table[i] = ray_intersection_unimplemented;
	}

	k_dispatch_table[k_shape_oobb][k_shape_oobb] = separating_axis_test;
	k_dispatch_table[k_shape_plane][k_shape_oobb] = oobb_vs_plane;
	k_dispatch_table[k_shape_oobb][k_shape_plane] = oobb_vs_plane;
	k_ray_dispatch_table[k_shape_oobb] = ray_vs_oobb;
	k_ray_dispatch_table[k_shape_plane] = ray_vs_plane;

	// Default gravity to Earth's constant.
	_gravity = { 0.0f, -9.807f, 0.0f };
}

ga_physics_world::~ga_physics_world()
{
	assert(_bodies.size() == 0);
}

void ga_physics_world::add_rigid_body(ga_rigid_body* body)
{
	while (_bodies_lock.test_and_set(std::memory_order_acquire)) {}
	_bodies.push_back(body);
	_bodies_lock.clear(std::memory_order_release);
}

void ga_physics_world::remove_rigid_body(ga_rigid_body* body)
{
	while (_bodies_lock.test_and_set(std::memory_order_acquire)) {}
	_bodies.erase(std::remove(_bodies.begin(), _bodies.end(), body));
	_bodies_lock.clear(std::memory_order_release);
}
void ga_physics_world::remove_all_rigid_bodies()
{
	while (_bodies.size() > 0)
	{
		ga_rigid_body* body = _bodies[_bodies.size() - 1];
		while (_bodies_lock.test_and_set(std::memory_order_acquire)) {}
		_bodies.erase(std::remove(_bodies.begin(), _bodies.end(), body));
		_bodies_lock.clear(std::memory_order_release);
	}	
}

void ga_physics_world::step(ga_frame_params* params)
{
	while (_bodies_lock.test_and_set(std::memory_order_acquire)) {}

	// Step the physics sim.
	for (int i = 0; i < _bodies.size(); ++i)
	{
		if (_bodies[i]->_flags & k_static) continue;

		ga_rigid_body* body = _bodies[i];

		if ((_bodies[i]->_flags & k_weightless) == 0)
		{
			body->_forces.push_back(_gravity);
		}

		step_linear_dynamics(params, body);
		step_angular_dynamics(params, body);
	}

	test_intersections(params);

	_bodies_lock.clear(std::memory_order_release);
}

void ga_physics_world::test_intersections(ga_frame_params* params)
{
	// Intersection tests. Naive N^2 comparisons.
	for (int i = 0; i < _bodies.size(); ++i)
	{
		for (int j = i + 1; j < _bodies.size(); ++j)
		{
			ga_shape* shape_a = _bodies[i]->_shape;
			ga_shape* shape_b = _bodies[j]->_shape;
			intersection_func_t func = k_dispatch_table[shape_a->get_type()][shape_b->get_type()];

			ga_collision_info info;
			bool collision = func(shape_a, _bodies[i]->_transform, shape_b, _bodies[j]->_transform, &info);
			if (collision)
			{
				std::time_t time = std::chrono::system_clock::to_time_t(
					std::chrono::system_clock::now());

#if defined(GA_PHYSICS_DEBUG_DRAW)
				ga_dynamic_drawcall collision_draw;
				collision_draw._positions.push_back(ga_vec3f::zero_vector());
				collision_draw._positions.push_back(info._normal);
				collision_draw._indices.push_back(0);
				collision_draw._indices.push_back(1);
				collision_draw._color = { 1.0f, 1.0f, 0.0f };
				collision_draw._draw_mode = GL_LINES;
				collision_draw._material = nullptr;
				collision_draw._transform.make_translation(info._point);

				while (params->_dynamic_drawcall_lock.test_and_set(std::memory_order_acquire)) {}
				params->_dynamic_drawcalls.push_back(collision_draw);
				params->_dynamic_drawcall_lock.clear(std::memory_order_release);
#endif
				// We should not attempt to resolve collisions if we're paused and have not single stepped.
				bool should_resolve = params->_delta_time > std::chrono::milliseconds(0) || params->_single_step;

				if (should_resolve)
				{
					resolve_collision(_bodies[i], _bodies[j], &info);
				}
			}
		}
	}
}

bool ga_physics_world::raycast_all(const ga_vec3f& ray_origin, const ga_vec3f& ray_dir,
	std::vector<ga_raycast_hit_info>* hit_info, float max_dist)
{
	bool hit = false;
	for (int i = 0; i < _bodies.size(); ++i)
	{
		ga_shape* shape = _bodies[i]->_shape;
		float t = 0;
		intersect_ray_func_t func = k_ray_dispatch_table[shape->get_type()];
		if (func(ray_origin, ray_dir, shape, _bodies[i]->_transform, &t) &&
			t < max_dist)
		{
			if (hit_info != NULL)
			{
				ga_raycast_hit_info info;
				info._collider = _bodies[i];
				hit_info->push_back(info);
			}
			hit = true;
		}
	}
	return hit;
}

std::vector<ga_vec3f> ga_physics_world::get_mesh_corners(float away_dist)
{
	std::vector<ga_vec3f> out;

	for (int i = 0; i < _bodies.size(); ++i)
	{
		ga_rigid_body* rb = _bodies[i];
		ga_shape* shape = rb->_shape;
		ga_oobb* cube = (ga_oobb*)shape;
		if (cube != NULL)
		{
			std::vector<ga_vec3f> corners;
			cube->get_corners(corners);
			for (int j = 0; j < corners.size(); ++j)
			{
				ga_vec3f pos = corners[j];
				if (away_dist > 0) pos += (corners[j] - cube->_center).normal().scale_result(away_dist);
				pos = rb->_transform.transform_point(pos);
				out.push_back(pos);
			}
		}
	}
	return out;
}

void ga_physics_world::step_linear_dynamics(ga_frame_params* params, ga_rigid_body* body)
{
	// Linear dynamics.
	ga_vec3f overall_force = ga_vec3f::zero_vector();
	while (body->_forces.size() > 0)
	{
		overall_force += body->_forces.back();
		body->_forces.pop_back();
	}


	float dt = std::chrono::duration_cast<std::chrono::duration<float>>(params->_delta_time).count();
	
	// Force - and so acceleration - is assumed constant throughout the timestep (we 
	// don't have a means to calculate acceleration as a function of time). So here the 
	// Runge Kutta method is used only to account for changing velocity over the timestep.
	ga_vec3f a = overall_force.scale_result(1.0f / body->_mass);

	// This can be simplified, but I have kept it like this to more clearly show the Runge Kutta method.
	// It seems a little strange to use this approach when force is going to be assumed constant over the 
	// timestep.
	ga_vec3f v1 = body->_velocity;
	ga_vec3f v2 = v1 + a.scale_result(0.5f * dt);
	ga_vec3f v3 = v1 + a.scale_result(0.5f * dt);
	ga_vec3f v4 = v1 + a.scale_result(dt);

	ga_vec3f r = body->_transform.get_translation();
	r = r + (v1 + v2.scale_result(2) + v3.scale_result(2) + v4).scale_result(dt / 6.0f);
	
	body->_transform.set_translation(r);
	body->_velocity = v4;
}

void ga_physics_world::step_angular_dynamics(ga_frame_params* params, ga_rigid_body* body)
{
	// TODO: Homework 5 BONUS.
	// Step the angular dynamics portion of the rigid body.
	// The rigid body's angular momentum, angular velocity, orientation, and transform
	// should all be updated with new values.
}

void ga_physics_world::resolve_collision(ga_rigid_body* body_a, ga_rigid_body* body_b, ga_collision_info* info)
{
	// First move the objects so they no longer intersect.
	// Each object will be moved proportionally to their incoming velocities.
	// If an object is static, it won't be moved.
	float total_velocity = body_a->_velocity.mag() + body_b->_velocity.mag();
	float percentage_a = (body_a->_flags & k_static) ? 0.0f : body_a->_velocity.mag() / total_velocity;
	float percentage_b = (body_a->_flags & k_static) ? 0.0f : body_b->_velocity.mag() / total_velocity;

	// To avoid instability, nudge the two objects slightly farther apart.
	const float k_nudge = 0.001f;
	if ((body_a->_flags & k_static) == 0 && body_a->_velocity.mag2() > 0.0f)
	{
		float pen_a = info->_penetration * percentage_a + k_nudge;
		body_a->_transform.set_translation(body_a->_transform.get_translation() - body_a->_velocity.normal().scale_result(pen_a));
	}
	if ((body_b->_flags & k_static) == 0 && body_b->_velocity.mag2() > 0.0f)
	{
		float pen_b = info->_penetration * percentage_b + k_nudge;
		body_b->_transform.set_translation(body_b->_transform.get_translation() - body_b->_velocity.normal().scale_result(pen_b));
	}

	// Average the coefficients of restitution.
	float cor_average = (body_a->_coefficient_of_restitution + body_b->_coefficient_of_restitution) / 2.0f;

	ga_vec3f n = info->_normal;
	ga_vec3f v1 = body_a->_velocity;
	ga_vec3f v2 = body_b->_velocity;
	float m1 = body_a->_mass;
	float m2 = body_b->_mass;

	if (body_a->_flags & k_static) // Body A is static
	{
		ga_vec3f impulse = n.scale_result(
			-(1 + cor_average) * m2 * v2.dot(n));

		ga_vec3f a2 = impulse.scale_result(1.0f / m2);
		body_b->_velocity = v2 + a2;
	}
	else if (body_b->_flags & k_static) // Body B is static
	{
		ga_vec3f impulse = n.scale_result(
			-(1 + cor_average) * m1 * v1.dot(n));

		ga_vec3f a1 = impulse.scale_result(1.0f / m1);
		body_a->_velocity = v1 + a1;
	}
	else
	{
		ga_vec3f impulse = n.scale_result(
			((1 + cor_average) * (v2.dot(n) - v1.dot(n))) /
			(1.0f / m1 + 1.0f / m2));

		ga_vec3f a1 = impulse.scale_result(1.0f / m1);
		ga_vec3f a2 = -impulse.scale_result(1.0f / m2);
		body_a->_velocity = v1 + a1;
		body_b->_velocity = v2 + a2;
	}
}

