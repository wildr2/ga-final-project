#pragma once

/*
** RPI Game Architecture Engine
**
** Portions adapted from:
** Viper Engine - Copyright (C) 2016 Velan Studios - All Rights Reserved
**
** This file is distributed under the MIT License. See LICENSE.txt.
*/

#include "math/ga_mat4f.h"
#include "math/ga_vec3f.h"

#include <vector>

struct ga_shape;
struct ga_plane;
class ga_rigid_body;

/*
** Information returned when a collision is detected.
** Includes the point of collision, the normal at the collision point, and
** the amount the two objects are interpenetrating.
*/
struct ga_collision_info
{
	ga_vec3f _point;
	ga_vec3f _normal;
	float _penetration;
};

struct ga_raycast_hit_info
{
	ga_vec3f _point;
	ga_vec3f _normal;
	ga_rigid_body* _collider;
};

/*
** Compute the distance from a point in 3d space to a plane.
*/
float distance_to_plane(const ga_vec3f& point, const ga_plane* plane);

/*
** Compute the distance from a point in 3d space to a line segment.
*/
float distance_to_line_segment(const ga_vec3f& point, const ga_vec3f& a, const ga_vec3f& b);

/*
** Compute the closest points on two lines.
** @returns True if the points lie with the bounds provided for each segment.
*/
bool closest_points_on_lines(
	const ga_vec3f& start_a,
	const ga_vec3f& end_a,
	const ga_vec3f& start_b,
	const ga_vec3f& end_b,
	ga_vec3f& point_a,
	ga_vec3f& point_b);

/*
** Compute the point farthest along a directional vector.
*/
ga_vec3f farthest_along_vector(const std::vector<ga_vec3f>& points, const ga_vec3f& vector);

/*
** Stub function for unimplemented collision algorithms.
*/
bool intersection_unimplemented(const ga_shape* a, const ga_mat4f& transform_a, const ga_shape* b, const ga_mat4f& transform_b, ga_collision_info* info);

/*
** Check for a collision between bounding box and plane.
*/
bool oobb_vs_plane(const ga_shape* a, const ga_mat4f& transform_a, const ga_shape* b, const ga_mat4f& transform_b, ga_collision_info* info);

/*
** Check for a collision between two oriented bounding boxes.
*/
bool separating_axis_test(const ga_shape* a, const ga_mat4f& transform_a, const ga_shape* b, const ga_mat4f& transform_b, ga_collision_info* info);


/*
** Stub function for unimplemented ray intersection algorithms.
*/
bool ray_intersection_unimplemented(const ga_vec3f & ray_origin, const ga_vec3f & ray_dir,
	const ga_shape* shape, const ga_mat4f& transform, float * dist);

/*
** Check for intersection between a ray and an oriented bounding box. 
** Dist is set to t along the ray at which the intersection occured.
*/
bool ray_vs_oobb(const ga_vec3f & ray_origin, const ga_vec3f & ray_dir,
	const ga_shape* shape, const ga_mat4f& transform, float * dist);


bool point_in_rect(float x, float y, float minx, float miny, float maxx, float maxy);