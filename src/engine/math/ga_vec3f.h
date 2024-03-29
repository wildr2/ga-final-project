#pragma once

/*
** RPI Game Architecture Engine
**
** Portions adapted from:
** Viper Engine - Copyright (C) 2016 Velan Studios - All Rights Reserved
**
** This file is distributed under the MIT License. See LICENSE.txt.
*/

#include "math/ga_vecnf.h"

/*
** Three component floating point vector.
*/
struct ga_vec3f
{
	union
	{
		struct { float x, y, z; };
		float axes[3];
	};

	GA_VECN_FUNCTIONS(3)

	static ga_vec3f zero_vector();
	static ga_vec3f one_vector();
	static ga_vec3f x_vector();
	static ga_vec3f y_vector();
	static ga_vec3f z_vector();

	bool operator==(const ga_vec3f& other) const
	{
		return (x == other.x
			&& y == other.y
			&& z == other.z);
	}
};

#undef V_VECN_FUNCTIONS

/*
** Compute the cross product between two vectors.
*/
inline ga_vec3f ga_vec3f_cross(const ga_vec3f& __restrict a, const ga_vec3f& __restrict b)
{
	ga_vec3f result;
	result.x = (a.y * b.z) - (a.z * b.y);
	result.y = (a.z * b.x) - (a.x * b.z);
	result.z = (a.x * b.y) - (a.y * b.x);
	return result;
}


// Custom hash for ga_vec3f based on custom hasher from:
// http://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
namespace std
{
	template <>
	struct hash<ga_vec3f>
	{
		size_t operator()(const ga_vec3f& k) const
		{
			// Compute individual hash values for first, second and third
			// http://stackoverflow.com/a/1646913/126995
			size_t res = 17;
			res = res * 31 + hash<float>()(k.x);
			res = res * 31 + hash<float>()(k.y);
			res = res * 31 + hash<float>()(k.z);
			return res;
		}
	};
}