/*
** RPI Game Architecture Engine
**
** Portions adapted from:
** Viper Engine - Copyright (C) 2016 Velan Studios - All Rights Reserved
**
** This file is distributed under the MIT License. See LICENSE.txt.
*/

#include "ga_audio_component.h"
#include "ga_listener_component.h"
#include "graphics/ga_debug_geometry.h"
#include "graphics/ga_geometry.h"
#include "entity/ga_entity.h"

ga_audio_component::ga_audio_component(ga_entity* ent, SoLoud::Soloud* audioEngine, SoLoud::Wav* wav) : ga_component(ent)
{
	_audioEngine = audioEngine;

	_clip = wav;
	_clip->setLooping(true);
	_clip->set3dMinMaxDistance(1, MAX_AUDIO_DIST);
	_clip->set3dAttenuation(SoLoud::AudioSource::LINEAR_DISTANCE, 1);

	_audioHandles.push_back(audioEngine->play3d(*_clip, 0, 0, 0));
	ga_mat4f trans = get_entity()->get_transform();
	_audioEngine->set3dSourcePosition(_audioHandles[0],
		trans.get_translation().x,
		trans.get_translation().y,
		trans.get_translation().z);
	//update3DAudio();

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
	//update3DAudio();

	// Visualization
#if DEBUG_DRAW_AUDIO
	ga_dynamic_drawcall drawcall;
	draw_debug_sphere(0.2f, get_entity()->get_transform(), &drawcall, { 1, 0.4f, 0.4f });

	while (params->_dynamic_drawcall_lock.test_and_set(std::memory_order_acquire)) {}
	params->_dynamic_drawcalls.push_back(drawcall);
	params->_dynamic_drawcall_lock.clear(std::memory_order_release);
#endif
}

void ga_audio_component::update_voices(std::vector<ga_vec3f> positions)
{
	for (int i = 0; i < positions.size(); ++i)
	{
		if (i < _audioHandles.size())
		{
			// Update position of existing voice
			_audioEngine->set3dSourcePosition(_audioHandles[i],
				positions[i].x, positions[i].y, positions[i].z);
		}
		else
		{
			// Start playing new voice
			_audioHandles.push_back(_audioEngine->play3d(*_clip,
				positions[i].x, positions[i].y, positions[i].z));
		}
	}
	for (int i = _audioHandles.size() - 1; i >= positions.size(); --i)
	{
		// Stop playing old voices (no longer relevent)
		_audioEngine->stop(_audioHandles[i]);
		_audioHandles.pop_back();
	}
}

//void ga_audio_component::update3DAudio()
//{
//	// Update source position (for distance attenuation / panning)
//	ga_mat4f trans = get_entity()->get_transform();
//	_audioEngine->set3dSourcePosition(_audioHandle,
//		trans.get_translation().x,
//		trans.get_translation().y,
//		trans.get_translation().z);
//}

