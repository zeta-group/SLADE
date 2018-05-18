#pragma once

#include "common.h"

class MIDIPlayer
{
public:
	MIDIPlayer();
	~MIDIPlayer();

	// Singleton implementation
	static MIDIPlayer* getInstance()
	{
		if (!instance_)
			instance_ = new MIDIPlayer();

		return instance_;
	}

	bool isSoundfontLoaded() const { return !fs_soundfont_ids_.empty(); }
	bool isReady() const;
	void resetPlayer();

	bool   initFluidsynth();
	bool   reloadSoundfont();
	bool   openFile(const string& filename);
	bool   openData(MemChunk& mc);
	bool   play();
	bool   pause() const;
	bool   stop() const;
	bool   isPlaying() const;
	int    getPosition() const;
	bool   setPosition(int pos);
	int    getLength();
	bool   setVolume(int volume);
	string getInfo();

private:
	static MIDIPlayer* instance_;

#ifndef NO_FLUIDSYNTH
	fluid_settings_t*     fs_settings_ = nullptr;
	fluid_synth_t*        fs_synth_    = nullptr;
	fluid_player_t*       fs_player_   = nullptr;
	fluid_audio_driver_t* fs_adriver_  = nullptr;
#endif

	bool        fs_initialised_ = false;
	vector<int> fs_soundfont_ids_;

	MemChunk   data_;
	wxProcess* program_ = nullptr;
	string     file_;
	sf::Clock  timer_;
};

// Define for less cumbersome MIDIPlayer::getInstance()
#define theMIDIPlayer MIDIPlayer::getInstance()
