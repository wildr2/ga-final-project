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

ga_audio_component::ga_audio_component(ga_entity* ent, SoLoud::Soloud* audioEngine, SoLoud::Wav* wav) : ga_component(ent)
{
	_audioEngine = audioEngine;

	wav->setLooping(true);
	wav->set3dMinMaxDistance(1, 20);
	wav->set3dAttenuation(SoLoud::AudioSource::LINEAR_DISTANCE, 1);

	_audioHandle = audioEngine->play3d(*wav, 0, 0, 0);
	update3DAudio();

	/*audioEngine->stop(sfx_drumsHandle);
	audioEngine->setPause(sfx_drumsHandle, true);
	audioEngine->setPause(sfx_drumsHandle, false);
	audioEngine->setVolume(sfx_drumsHandle, 1);*/
	//audioEngine->setVolume(sfx_drumsHandle, 1.0f);
	//audioEngine->set3dListenerPosition(5, 0, 0);
	//audioEngine->update3dAudio();
}

ga_audio_component::~ga_audio_component()
{
}

void ga_audio_component::update(ga_frame_params* params)
{	
	float dt = std::chrono::duration_cast<std::chrono::duration<float>>(params->_delta_time).count();

	// Sound
	update3DAudio();

	// Visualization
#if DEBUG_DRAW_AUDIO
	ga_dynamic_drawcall drawcall;
	draw_debug_sphere(0.2f, get_entity()->get_transform(), &drawcall, { 1, 0.4f, 0.4f });

	while (params->_dynamic_drawcall_lock.test_and_set(std::memory_order_acquire)) {}
	params->_dynamic_drawcalls.push_back(drawcall);
	params->_dynamic_drawcall_lock.clear(std::memory_order_release);
#endif
}

void ga_audio_component::update3DAudio()
{
	// Update source position (for distance attenuation / panning)
	ga_mat4f trans = get_entity()->get_transform();
	_audioEngine->set3dSourcePosition(_audioHandle,
		trans.get_translation().x,
		trans.get_translation().y,
		trans.get_translation().z);
}

