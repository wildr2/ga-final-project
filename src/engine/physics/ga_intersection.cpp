/*
** RPI Game Architecture Engine
**
** Portions adapted from:
** Viper Engine - Copyright (C) 2016 Velan Studios - All Rights Reserved
**
** This file is distributed under the MIT License. See LICENSE.txt.
*/

#include "ga_intersection.h"

#include "ga_shape.h"

#include <cassert>
#include <float.h>
#include <vector>

float distance_to_plane(const ga_vec3f& point, const ga_plane* plane)
{
	return plane->_normal.dot(point) - plane->_normal.dot(plane->_point);
}

float distance_to_line_segment(const ga_vec3f& point, const ga_vec3f& a, const ga_vec3f& b)
{
	ga_vec3f cross = ga_vec3f_cross((point - a), (point - b));
	ga_vec3f segment = (b - a);

	return (cross.mag() / segment.mag());
}

bool closest_points_on_lines(
	const ga_vec3f& start_a,
	const ga_vec3f& end_a,
	const ga_vec3f& start_b,
	const ga_vec3f& end_b,
	ga_vec3f& point_a,
	ga_vec3f& point_b)
{
	// Algorithm from http://geomalgorithms.com/a07-_distance.html
	ga_vec3f u_o = (end_a - start_a);
	ga_vec3f u = u_o.normal();
	ga_vec3f v_o = (end_b - start_b);
	ga_vec3f v = v_o.normal();
	ga_vec3f w0 = (start_a - start_b);

	float a = u.dot(u);
	float b = u.dot(v);
	float c = v.dot(v);
	float d = u.dot(w0);
	float e = v.dot(w0);

	float sc = (b * e - c * d) / (a * c - b * b);
	float tc = (a * e - b * d) / (a * c - b * b);

	point_a = start_a + u.scale_result(sc);
	point_b = start_b + v.scale_result(tc);

	return (sc >= 0.0f) && (sc <= u_o.mag()) && (tc >= 0.0f) && (tc <= v_o.mag());
}

ga_vec3f farthest_along_vector(const std::vector<ga_vec3f>& points, const ga_vec3f& vector)
{
	float max_dot = -FLT_MAX;
	ga_vec3f best;

	for (uint32_t i = 0; i < points.size(); ++i)
	{
		ga_vec3f point = points[i];
		if (point.dot(vector) > max_dot)
		{
			max_dot = point.dot(vector);
			best = point;
		}
	}

	return best;
}

bool intersection_unimplemented(const ga_shape* a, const ga_mat4f& transform_a, const ga_shape* b, const ga_mat4f& transform_b, ga_collision_info* info)
{
	assert(false);
	return false;
}

bool oobb_vs_plane(const ga_shape* a, const ga_mat4f& transform_a, const ga_shape* b, const ga_mat4f& transform_b, ga_collision_info* info)
{
	// Figure out which shape is which.
	ga_oobb oobb;
	ga_plane plane;

	if (a->get_type() == k_shape_oobb)
	{
		oobb = *reinterpret_cast<const ga_oobb*>(a);
		oobb._center += transform_a.get_translation();
		oobb._half_vectors[0] = transform_a.transform_vector(oobb._half_vectors[0]);
		oobb._half_vectors[1] = transform_a.transform_vector(oobb._half_vectors[1]);
		oobb._half_vectors[2] = transform_a.transform_vector(oobb._half_vectors[2]);
		plane = *reinterpret_cast<const ga_plane*>(b);
		plane._normal = transform_b.transform_vector(plane._normal);
		plane._point += transform_b.get_translation();
	}
	else
	{
		oobb = *reinterpret_cast<const ga_oobb*>(b);
		oobb._center += transform_b.get_translation();
		oobb._half_vectors[0] = transform_b.transform_vector(oobb._half_vectors[0]);
		oobb._half_vectors[1] = transform_b.transform_vector(oobb._half_vectors[1]);
		oobb._half_vectors[2] = transform_b.transform_vector(oobb._half_vectors[2]);
		plane = *reinterpret_cast<const ga_plane*>(a);
		plane._normal = transform_a.transform_vector(plane._normal);
		plane._point += transform_a.get_translation();
	}

	// Project each of the half vectors onto the plane's normal to get its 'radius.'
	ga_vec3f projection =
		oobb._half_vectors[0].project_onto_abs(plane._normal) +
		oobb._half_vectors[1].project_onto_abs(plane._normal) +
		oobb._half_vectors[2].project_onto_abs(plane._normal);
	float radius = projection.mag();

	float distance = distance_to_plane(oobb._center, &plane);

	bool collision = distance < radius;
	if (collision)
	{
		info->_penetration = radius - distance;
		info->_normal = plane._normal;

		const int32_t k_num_corners = 8;
		// We can find the collision point by finding the point penetrating farthest into the plane.
		ga_vec3f corners[k_num_corners];
		corners[0] = (oobb._center + oobb._half_vectors[0] + oobb._half_vectors[1] + oobb._half_vectors[2]);
		corners[1] = (oobb._center + oobb._half_vectors[0] + oobb._half_vectors[1] - oobb._half_vectors[2]);
		corners[2] = (oobb._center + oobb._half_vectors[0] - oobb._half_vectors[1] + oobb._half_vectors[2]);
		corners[3] = (oobb._center + oobb._half_vectors[0] - oobb._half_vectors[1] - oobb._half_vectors[2]);
		corners[4] = (oobb._center - oobb._half_vectors[0] + oobb._half_vectors[1] + oobb._half_vectors[2]);
		corners[5] = (oobb._center - oobb._half_vectors[0] + oobb._half_vectors[1] - oobb._half_vectors[2]);
		corners[6] = (oobb._center - oobb._half_vectors[0] - oobb._half_vectors[1] + oobb._half_vectors[2]);
		corners[7] = (oobb._center - oobb._half_vectors[0] - oobb._half_vectors[1] - oobb._half_vectors[2]);

		float pens[k_num_corners];
		float max_pen = FLT_MAX;
		for (int i = 0; i < k_num_corners; ++i)
		{
			pens[i] = distance_to_plane(corners[i], &plane);
			// Maximum intersection is defined by the most negative, or most below the plane.
			if (pens[i] < max_pen)
			{
				max_pen = pens[i];
			}
		}

		// Gather all the corners that equal the maximum penetration.
		std::vector<ga_vec3f> max_corners;
		for (int i = 0; i < k_num_corners; ++i)
		{
			if (ga_equalf(pens[i], max_pen))
			{
				max_corners.push_back(corners[i]);
			}
		}

		// Now, average the corners with the maximum penetration to find the collision point.
		// If the point is an entire surface or edge, we should get a point in the middle of it.
		ga_vec3f average = { 0.0f, 0.0f, 0.0f };
		for (auto& c : max_corners)
		{
			average += c;
		}
		average.scale(1.0f / static_cast<float>(max_corners.size()));

		info->_point = average + info->_normal.scale_result(info->_penetration);
	}

	return collision;
}

ga_vec3f separating_axis_point_of_collision(const ga_oobb* oobb_a, const ga_oobb* oobb_b, uint32_t min_penetration_index)
{
	// This is not the ideal way of doing this, but it should arrive at the correct result.
	ga_vec3f point_of_intersection;

	std::vector<ga_vec3f> corners_a;
	std::vector<ga_vec3f> corners_b;
	oobb_a->get_corners(corners_a);
	oobb_b->get_corners(corners_b);
		
	ga_vec3f a_to_b = oobb_b->_center - oobb_a->_center;

	// Find the two points of a closest to b.
	ga_vec3f primary_a = farthest_along_vector(corners_a, a_to_b);
	corners_a.erase(std::find(corners_a.begin(), corners_a.end(), primary_a));
	ga_vec3f secondary_a = farthest_along_vector(corners_a, a_to_b);
	corners_a.erase(std::find(corners_a.begin(), corners_a.end(), secondary_a));
	ga_vec3f tertiary_a = farthest_along_vector(corners_a, a_to_b);
	corners_a.erase(std::find(corners_a.begin(), corners_a.end(), tertiary_a));
	ga_vec3f quarternary_a = farthest_along_vector(corners_a, a_to_b);

	// Find the two points of b closest to a.
	ga_vec3f primary_b = farthest_along_vector(corners_b, -a_to_b);
	corners_b.erase(std::find(corners_b.begin(), corners_b.end(), primary_b));
	ga_vec3f secondary_b = farthest_along_vector(corners_b, -a_to_b);
	corners_b.erase(std::find(corners_b.begin(), corners_b.end(), secondary_b));
	ga_vec3f tertiary_b = farthest_along_vector(corners_b, -a_to_b);
	corners_b.erase(std::find(corners_b.begin(), corners_b.end(), tertiary_b));
	ga_vec3f quarternary_b = farthest_along_vector(corners_b, -a_to_b);

	// If the normal is one of the boxes' axes, use the closest point from the other box.
	if (min_penetration_index < 3)
	{
		// If the faces of the boxes are colliding, average the points.
		if (ga_absf(oobb_a->_half_vectors[min_penetration_index].normal().dot(oobb_b->_half_vectors[0].normal())) > 0.95f ||
			ga_absf(oobb_a->_half_vectors[min_penetration_index].normal().dot(oobb_b->_half_vectors[1].normal())) > 0.95f ||
			ga_absf(oobb_a->_half_vectors[min_penetration_index].normal().dot(oobb_b->_half_vectors[2].normal())) > 0.95f)
		{
			point_of_intersection = (primary_b + secondary_b + tertiary_b + quarternary_b).scale_result(0.25f);
		}
		else
		{
			point_of_intersection = primary_b;
		}
	}
	else if(min_penetration_index < 6)
	{
		if (ga_absf(oobb_a->_half_vectors[0].normal().dot(oobb_b->_half_vectors[min_penetration_index - 3].normal())) > 0.95f ||
			ga_absf(oobb_a->_half_vectors[1].normal().dot(oobb_b->_half_vectors[min_penetration_index - 3].normal())) > 0.95f ||
			ga_absf(oobb_a->_half_vectors[2].normal().dot(oobb_b->_half_vectors[min_penetration_index - 3].normal())) > 0.95f)
		{
			point_of_intersection = (primary_a + secondary_a + tertiary_a + quarternary_a).scale_result(0.25f);
		}
		else
		{
			point_of_intersection = primary_a;
		}
	}
	else
	{
		// Using the two half vectors crossed to get the axis of minimum penetration,
		// and the closest points, formulate two candidate edges per box.
		uint32_t cross_index = min_penetration_index - 6;
		uint32_t a_hvec_index = cross_index / 3;
		uint32_t b_hvec_index = cross_index % 3;

		ga_vec3f a_hvec = oobb_a->_half_vectors[a_hvec_index];
		ga_vec3f b_hvec = oobb_b->_half_vectors[b_hvec_index];

		ga_vec3f edges_a[2][2];
		ga_vec3f edges_b[2][2];

		bool align = ga_equalf(ga_absf((primary_a - secondary_a).normal().dot(a_hvec.normal())), 1.0f);
		edges_a[0][0] = primary_a;
		edges_a[0][1] = align ? secondary_a : tertiary_a;
		edges_a[1][0] = align ? tertiary_a : secondary_a;
		edges_a[1][1] = quarternary_a;

		align = ga_equalf(ga_absf((primary_b - secondary_b).normal().dot(b_hvec.normal())), 1.0f);
		edges_b[0][0] = primary_b;
		edges_b[0][1] = align ? secondary_b : tertiary_b;
		edges_b[1][0] = align ? tertiary_b : secondary_b;
		edges_b[1][1] = quarternary_b;

		ga_vec3f point_a;
		ga_vec3f point_b;
		float min_dist = FLT_MAX;
		for (uint32_t i = 0; i < 2; ++i)
		{
			for (uint32_t j = 0; j < 2; ++j)
			{
				ga_vec3f temp_a;
				ga_vec3f temp_b;
				if (closest_points_on_lines(edges_a[i][0], edges_a[i][1], edges_b[j][0], edges_b[j][1], temp_a, temp_b))
				{
					float dist = (temp_a - temp_b).mag();
					if (dist < min_dist)
					{
						min_dist = dist;
						point_a = temp_a;
						point_b = temp_b;
					}
				}
			}
		}

		point_of_intersection = (point_a + point_b).scale_result(0.5f);
	}

	return point_of_intersection;
}

bool separating_axis_test(const ga_shape* a, const ga_mat4f& transform_a, const ga_shape* b, const ga_mat4f& transform_b, ga_collision_info* info)
{
	bool collision = true;
	std::vector<ga_vec3f> axes;
	ga_oobb oobb_a, oobb_b;

	oobb_a = *reinterpret_cast<const ga_oobb*>(a);
	oobb_a._center += transform_a.get_translation();
	oobb_a._half_vectors[0] = transform_a.transform_vector(oobb_a._half_vectors[0]);
	oobb_a._half_vectors[1] = transform_a.transform_vector(oobb_a._half_vectors[1]);
	oobb_a._half_vectors[2] = transform_a.transform_vector(oobb_a._half_vectors[2]);

	oobb_b = *reinterpret_cast<const ga_oobb*>(b);
	oobb_b._center += transform_b.get_translation();
	oobb_b._half_vectors[0] = transform_b.transform_vector(oobb_b._half_vectors[0]);
	oobb_b._half_vectors[1] = transform_b.transform_vector(oobb_b._half_vectors[1]);
	oobb_b._half_vectors[2] = transform_b.transform_vector(oobb_b._half_vectors[2]);

	float min_penetration = FLT_MAX;
	ga_vec3f min_penetration_axis;
	uint32_t min_penetration_index = INT_MAX;

	// The local variable 'collision' should be set true or false depending on whether the
	// two boxes are interpenetrating.

	// Assemble axes
	for (int i = 0; i < 3; ++i)
	{
		axes.push_back(oobb_a._half_vectors[i].normal());
	}
	for (int i = 0; i < 3; ++i)
	{
		axes.push_back(oobb_b._half_vectors[i].normal());
	}
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			axes.push_back(ga_vec3f_cross(oobb_a._half_vectors[i],
				oobb_b._half_vectors[j]).normal());
		}
	}

	// Test for seperation
	for (int i = 0; i < axes.size(); ++i)
	{
		float proj_a = 0;
		float proj_b = 0;
		for (int j = 0; j < 3; ++j)
		{
			proj_a += abs(oobb_a._half_vectors[j].dot(axes[i]));
			proj_b += abs(oobb_b._half_vectors[j].dot(axes[i]));
		}
		float center_a = oobb_a._center.dot(axes[i]);
		float center_b = oobb_b._center.dot(axes[i]);
		float min_a = center_a - proj_a;
		float max_a = center_a + proj_a;
		float min_b = center_b - proj_b;
		float max_b = center_b + proj_b;

		// Check if not overlapping
		if (min_a > max_b || max_a < min_b)
		{
			collision = false;
			break;
		}
		else
		{
			float penetration = ga_min((max_b - min_a), (max_a - min_b));
			if (penetration < min_penetration)
			{
				min_penetration = penetration;
				min_penetration_axis = axes[i];
				min_penetration_index = i;
			}
		}
	}

	if (collision && min_penetration_index < INT_MAX)
	{
		// The normal of the collision is the axis of minimum penetration.
		info->_normal = min_penetration_axis;
		info->_penetration = min_penetration;
		info->_point = separating_axis_point_of_collision(&oobb_a, &oobb_b, min_penetration_index);
	}

	return collision;
}


bool ray_intersection_unimplemented(const ga_vec3f & ray_origin, const ga_vec3f & ray_dir,
	const ga_shape* shape, const ga_mat4f& transform, float * dist)
{
	return false;
}

bool ray_vs_oobb(const ga_vec3f& ray_origin, const ga_vec3f& ray_dir,
	const ga_shape* shape, const ga_mat4f& transform, float* dist)
{
	ga_oobb oobb = *reinterpret_cast<const ga_oobb*>(shape);
	ga_mat4f inv_tran = transform.inverse();
	ga_vec3f O = inv_tran.transform_point(ray_origin);
	ga_vec3f D = inv_tran.transform_vector(ray_dir);

	ga_vec3f min = oobb._center - oobb._half_vectors[0] - oobb._half_vectors[1] - oobb._half_vectors[2];
	ga_vec3f max = oobb._center + oobb._half_vectors[0] + oobb._half_vectors[1] + oobb._half_vectors[2];

	// Find time at crossings of face plane edges
	float tmin = (min.x - O.x) / D.x;
	float tmax = (max.x - O.x) / D.x;
	float tymin = (min.y - O.y) / D.y;
	float tymax = (max.y - O.y) / D.y;
	float tzmin = (min.z - O.z) / D.z;
	float tzmax = (max.z - O.z) / D.z;

	// Find points at crossing times
	ga_vec3f ptmin = O + D.scale_result(tmin);
	ga_vec3f ptmax = O + D.scale_result(tmax);
	ga_vec3f ptymin = O + D.scale_result(tymin);
	ga_vec3f ptymax = O + D.scale_result(tymax);
	ga_vec3f ptzmin = O + D.scale_result(tzmin);
	ga_vec3f ptzmax = O + D.scale_result(tzmax);

	// Find earliest crossing time at which the point intersects the face of the cube
	float t = -1;
	if (tmin > 0 && point_in_rect(ptmin.y, ptmin.z, min.y, min.z, max.y, max.z))
		t = tmin < t || t < 0 ? tmin : t;
	if (tmax > 0 && point_in_rect(ptmax.y, ptmax.z, min.y, min.z, max.y, max.z))
		t = tmax < t || t < 0 ? tmax : t;
	if (tymin > 0 && point_in_rect(ptymin.x, ptymin.z, min.x, min.z, max.x, max.z))
		t = tymin < t || t < 0 ? tymin : t;
	if (tymax > 0 && point_in_rect(ptymax.x, ptymax.z, min.x, min.z, max.x, max.z))
		t = tymax < t || t < 0 ? tymax : t;
	if (tzmin > 0 && point_in_rect(ptzmin.x, ptzmin.y, min.x, min.y, max.x, max.y))
		t = tzmin < t || t < 0 ? tzmin : t;
	if (tzmax > 0 && point_in_rect(ptzmax.x, ptzmin.y, min.x, min.y, max.x, max.y))
		t = tzmax < t || t < 0 ? tzmax : t;
	
	if (t >= 0)
	{
		*dist = t;
		return true;
	}
	return false;
}

bool ray_vs_plane(const ga_vec3f& ray_origin, const ga_vec3f& ray_dir,
	const ga_shape* shape, const ga_mat4f& transform, float* dist)
{
	ga_plane plane = *reinterpret_cast<const ga_plane*>(shape);
	ga_mat4f inv_tran = transform.inverse();
	ga_vec3f O = inv_tran.transform_point(ray_origin);
	ga_vec3f D = inv_tran.transform_vector(ray_dir);

	float denom = plane._normal.dot(D);
	if (abs(denom) > 0.0001f)
	{
		*dist = (plane._point - O).dot(plane._normal) / denom;
		if (*dist >= 0) return true;
	}
	return false;
}


bool point_in_rect(float x, float y, float minx, float miny, float maxx, float maxy)
{
	return x >= minx && x <= maxx && y >= miny && y <= maxy;
}