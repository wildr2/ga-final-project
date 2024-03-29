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
** Component which models a source of sound in 3D space; plays a wav file on loop.
*/
class ga_audio_component : public ga_component
{
public:
	ga_audio_component(class ga_entity* ent, SoLoud::Soloud* audio_engine, SoLoud::Wav* wav);
	virtual ~ga_audio_component();

	int get_audio_handle() { return _audio_handle; }

	virtual void update(struct ga_frame_params* params) override;

private:

	SoLoud::Soloud* _audio_engine;
	int _audio_handle;
	
};
