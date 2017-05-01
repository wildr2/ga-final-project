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
#include "audio/ga_listener_component.h"
#include "util/ga_kb_move_component.h"
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

#include <strstream>
#include <string>
#include <sstream>

#if defined(GA_MINGW)
#include <unistd.h>
#endif

static void set_root_path(const char* exepath);
static void run_unit_tests();

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

void setup_scene_audio(ga_sim* sim, ga_physics_world* world, SoLoud::Soloud* audioEngine)
{
	// Listener
	ga_entity* listener_ent = new ga_entity();
	ga_listener_component* listener = new ga_listener_component(listener_ent, audioEngine, world);
	ga_kb_move_component* listener_move_comp = new ga_kb_move_component(
		listener_ent, k_button_k, k_button_j, k_button_i, k_button_l);
	ga_mat4f*  listener_transform = new ga_mat4f();
	listener_transform->make_identity();
	listener_transform->translate({ 7, 1.5f, 0 });
	listener_ent->set_transform(*listener_transform);
	sim->add_entity(listener_ent);

	// Audio Source 1
	SoLoud::Wav* sfx_drums = new SoLoud::Wav();
	sfx_drums->load(strcat(g_root_path, "/data/audio/drums.wav"));
	ga_entity* source = new ga_entity();
	ga_audio_component* audio_comp = new ga_audio_component(source, audioEngine, sfx_drums);
	//ga_kb_move_component* source_move_comp = new ga_kb_move_component(
		//source, k_button_g, k_button_f, k_button_t, k_button_h);

	ga_mat4f* source_transform = new ga_mat4f();
	source_transform->make_identity();
	source_transform->translate({ -3, 1.5f, 0 });
	source->set_transform(*source_transform);
	sim->add_entity(source);
	listener->registerAudioSource(audio_comp);
}

ga_entity* create_cube(ga_mat4f* transform, ga_sim* sim, ga_physics_world* world)
{
	ga_entity* cube = new ga_entity();
	ga_cube_component* model = new ga_cube_component(cube, "data/textures/wall3.jpg");

	cube->set_transform(*transform);

	ga_oobb* cube_oobb = new ga_oobb();
	cube_oobb->_half_vectors[0] = ga_vec3f::x_vector();
	cube_oobb->_half_vectors[1] = ga_vec3f::y_vector();
	cube_oobb->_half_vectors[2] = ga_vec3f::z_vector();
	ga_physics_component* collider = new ga_physics_component(cube, cube_oobb, 1.0f);
	collider->get_rigid_body()->make_static();
	world->add_rigid_body(collider->get_rigid_body());

	sim->add_entity(cube);
	return cube;
}
void create_scene_wall(ga_sim* sim, ga_physics_world* world)
{
	ga_mat4f tran;
	for (int i = 0; i < 3; ++i) {
		tran.make_scaling(1);
		tran.translate({ 0, i * 2.0f, 0 });
		create_cube(&tran, sim, world);

		tran.make_scaling(1);
		tran.translate({ 0, i * 2.0f, -2 });
		create_cube(&tran, sim, world);

		tran.make_scaling(1);
		tran.translate({ 0, i * 2.0f, -4 });
		create_cube(&tran, sim, world);

		tran.make_scaling(1);
		tran.translate({ 0, i * 2.0f, -6 });
		create_cube(&tran, sim, world);

		tran.make_scaling(1);
		tran.translate({ 0, i * 2.0f, -8 });
		create_cube(&tran, sim, world);

		tran.make_scaling(1);
		tran.translate({ 0, i * 2.0f, -10 });
		create_cube(&tran, sim, world);

		tran.make_scaling(1);
		tran.translate({ -2, i * 2.0f, -8 });
		create_cube(&tran, sim, world);
	}
}
void create_scene_window(ga_sim* sim, ga_physics_world* world)
{
	ga_mat4f tran;
	tran.make_scaling(1);
	tran.translate({ -4, 0, -8 });
	create_cube(&tran, sim, world);

	tran.make_scaling(1);
	tran.translate({ -6, 0, -8 });
	create_cube(&tran, sim, world);

	tran.make_scaling(1);
	tran.translate({ -6, 2, -8 });
	create_cube(&tran, sim, world);

	tran.make_scaling(1);
	tran.translate({ -6, 4, -8 });
	create_cube(&tran, sim, world);

	tran.make_scaling(1);
	tran.translate({ -4, 4, -8 });
	create_cube(&tran, sim, world);
}
void create_scene_pillar(ga_sim* sim, ga_physics_world* world)
{
	ga_mat4f tran;
	tran.make_scaling(1);
	tran.translate({ 3, 0, 0 });
	create_cube(&tran, sim, world);

	tran.make_scaling(1);
	tran.translate({ 3, 2, 0 });
	create_cube(&tran, sim, world);
}


int main(int argc, const char** argv)
{
	set_root_path(argv[0]);
	ga_job::startup(0xffff, 256, 256);

	run_unit_tests();

	// Create objects for three phases of the frame: input, sim and output.
	ga_input* input = new ga_input();
	ga_sim* sim = new ga_sim();
	ga_physics_world* world = new ga_physics_world();
	ga_output* output = new ga_output(input->get_window());

	// Create camera.
	ga_camera* camera = new ga_camera({ 0.0f, 7.0f, 20.0f });
	ga_quatf rotation;
	rotation.make_axis_angle(ga_vec3f::y_vector(), ga_degrees_to_radians(180));
	camera->rotate(rotation);
	rotation.make_axis_angle(ga_vec3f::x_vector(), ga_degrees_to_radians(15.0f));
	camera->rotate(rotation);

	// Audio Engine
	SoLoud::Soloud audioEngine;
	audioEngine.init();

	// Scene
	create_scene_wall(sim, world);
	create_scene_window(sim, world);
	//create_scene_pillar(sim, world);
	setup_scene_audio(sim, world, &audioEngine);


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
	audioEngine.deinit();

	delete output;
	delete world;
	delete sim;
	delete input;
	delete camera;

	ga_job::shutdown();

	return 0;
}

void run_unit_tests()
{
	ga_intersection_utility_unit_tests();
	ga_intersection_unit_tests();
}
