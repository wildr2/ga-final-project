/*
** RPI Game Architecture Engine
**
** Portions adapted from:
** Viper Engine - Copyright (C) 2016 Velan Studios - All Rights Reserved
**
** This file is distributed under the MIT License. See LICENSE.txt.
*/

#include "framework/ga_camera.h"
#include "framework/ga_compiler_defines.h"
#include "framework/ga_input.h"
#include "framework/ga_sim.h"
#include "framework/ga_output.h"
#include "jobs/ga_job.h"

#include "entity/ga_entity.h"

#include "audio/ga_audio_component.h"
#include "graphics/ga_cube_component.h"
#include "graphics/ga_program.h"

#include "physics/ga_intersection.tests.h"
#include "physics/ga_physics_component.h"
#include "physics/ga_physics_world.h"
#include "physics/ga_rigid_body.h"
#include "physics/ga_shape.h"

#include "soloud.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#if defined(GA_MINGW)
#include <unistd.h>
#endif

static void set_root_path(const char* exepath);
static void run_unit_tests();

void create_scene(ga_sim* sim, ga_physics_world* world, SoLoud::Soloud audioEngine)
{
	//// Cube
	//ga_entity cube;
	//ga_cube_component model(&cube, "data/textures/rpi.png");
	//ga_mat4f cube_transform;
	//cube_transform.make_scaling(2);
	//cube_transform.translate({ 0, 2, 0 });
	//cube.set_transform(cube_transform);
	//sim->add_entity(&cube);

	//// Audio Source
	//ga_entity speaker;
	//ga_audio_component audio_comp(&speaker, &audioEngine);
	//ga_mat4f spkr_transform;
	//spkr_transform.make_identity();
	//spkr_transform.translate({ -6, 1, 0 });
	//speaker.set_transform(spkr_transform);
	//sim->add_entity(&speaker);

	//// Floor
	//ga_entity floor;
	//ga_plane floor_plane;
	//floor_plane._point = { 0.0f, 0.0f, 0.0f };
	//floor_plane._normal = { 0.0f, 1.0f, 0.0f };
	//ga_physics_component floor_collider(&floor, &floor_plane, 0.0f);
	//floor_collider.get_rigid_body()->make_static();
	//world->add_rigid_body(floor_collider.get_rigid_body());
	//sim->add_entity(&floor);
}

int main(int argc, const char** argv)
{
	set_root_path(argv[0]);
	ga_job::startup(0xffff, 256, 256);

	//run_unit_tests();

	// Create objects for three phases of the frame: input, sim and output.
	ga_input* input = new ga_input();
	ga_sim* sim = new ga_sim();
	ga_physics_world* world = new ga_physics_world();
	ga_output* output = new ga_output(input->get_window());

	// Create camera.
	ga_camera* camera = new ga_camera({ 0.0f, 7.0f, 20.0f });
	ga_quatf rotation;
	rotation.make_axis_angle(ga_vec3f::y_vector(), ga_degrees_to_radians(180.0f));
	camera->rotate(rotation);
	rotation.make_axis_angle(ga_vec3f::x_vector(), ga_degrees_to_radians(15.0f));
	camera->rotate(rotation);

	// Audio Engine
	SoLoud::Soloud audioEngine;
	audioEngine.init();

	// Scene
	//create_scene(sim, world);

	// Cube
	ga_entity cube;
	ga_cube_component model(&cube, "data/textures/rpi.png");
	ga_mat4f cube_transform;
	cube_transform.make_scaling(2);
	cube_transform.translate({ 0, 2, 0 });
	cube.set_transform(cube_transform);
	sim->add_entity(&cube);

	// Audio Source
	SoLoud::Wav sfx_drums;
	int ret = sfx_drums.load("drums.wav");
	ga_entity speaker;
	ga_audio_component audio_comp(&speaker, &audioEngine, &sfx_drums);
	ga_mat4f spkr_transform;
	spkr_transform.make_identity();
	spkr_transform.translate({ -6, 1, 0 });
	speaker.set_transform(spkr_transform);
	sim->add_entity(&speaker);

	// Floor
	ga_entity floor;
	ga_plane floor_plane;
	floor_plane._point = { 0.0f, 0.0f, 0.0f };
	floor_plane._normal = { 0.0f, 1.0f, 0.0f };
	ga_physics_component floor_collider(&floor, &floor_plane, 0.0f);
	floor_collider.get_rigid_body()->make_static();
	world->add_rigid_body(floor_collider.get_rigid_body());
	sim->add_entity(&floor);


	// Main loop:
	while (true)
	{
		// We pass frame state through the 3 phases using a params object.
		ga_frame_params params;

		// Gather user input and current time.
		if (!input->update(&params))
		{
			break;
		}

		// Update the camera.
		camera->update(&params);

		// Run gameplay.
		sim->update(&params);

		// Step the physics world.
		world->step(&params);

		// Perform the late update.
		sim->late_update(&params);

		// Draw to screen.
		output->update(&params);
	}

	world->remove_all_rigid_bodies();


	delete output;
	delete world;
	delete sim;
	delete input;
	delete camera;

	ga_job::shutdown();

	return 0;
}

char g_root_path[256];
static void set_root_path(const char* exepath)
{
#if defined(GA_MSVC)
	strcpy_s(g_root_path, sizeof(g_root_path), exepath);

	// Strip the executable file name off the end of the path:
	char* slash = strrchr(g_root_path, '\\');
	if (!slash)
	{
		slash = strrchr(g_root_path, '/');
	}
	if (slash)
	{
		slash[1] = '\0';
	}
#elif defined(GA_MINGW)
	char* cwd;
	char buf[PATH_MAX + 1];
	cwd = getcwd(buf, PATH_MAX + 1);
	strcpy_s(g_root_path, sizeof(g_root_path), cwd);

	g_root_path[strlen(cwd)] = '/';
	g_root_path[strlen(cwd) + 1] = '\0';
#endif
}

void run_unit_tests()
{
	ga_intersection_utility_unit_tests();
	ga_intersection_unit_tests();
}
