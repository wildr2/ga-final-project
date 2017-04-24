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
#include "soloud.h"
#include "soloud_wav.h"

/*
** Component which drives animation; updates skeleton and skinning matrices.
*/
class ga_audio_component : public ga_component
{
public:
	ga_audio_component(class ga_entity* ent, SoLoud::Soloud* audioEngine, SoLoud::Wav* wav);
	virtual ~ga_audio_component();

	//int getAudioHandle() { return _audioHandle; }

	virtual void update(struct ga_frame_params* params) override;

private:

	//void update3DAudio();
	void update_voices(std::vector<ga_vec3f> positions);

	SoLoud::Soloud* _audioEngine;
	std::vector<int> _audioHandles;
	SoLoud::Wav* _clip;

	friend class ga_listener_component;
};
