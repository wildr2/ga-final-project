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

#define DEBUG_DRAW_AUDIO_SOURCES 1

/*
** Component which drives animation; updates skeleton and skinning matrices.
*/
class ga_audio_component : public ga_component
{
public:
	ga_audio_component(class ga_entity* ent, SoLoud::Soloud* audioEngine, SoLoud::Wav* wav);
	virtual ~ga_audio_component();

	int getAudioHandle() { return _audioHandle; }

	virtual void update(struct ga_frame_params* params) override;

private:

	void update3DAudio();

	SoLoud::Soloud* _audioEngine;
	int _audioHandle;
	
};
