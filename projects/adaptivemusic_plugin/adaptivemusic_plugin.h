/**
 * ======================================================
 * Metamod:Source AdaptiveMusic Plugin
 * Written by Manuel Russello
 * ======================================================
 */

#ifndef _INCLUDE_ADAPTIVEMUSIC_PLUGIN_H_
#define _INCLUDE_ADAPTIVEMUSIC_PLUGIN_H_

#include <ISmmPlugin.h>

// FMOD Includes
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include "fmod_errors.h"

#if defined WIN32 && !defined snprintf
#define snprintf _snprintf
#endif

// FMOD global variables
FMOD::Studio::System *fmodStudioSystem;
FMOD::Studio::Bank *loadedFmodStudioBank;
char *loadedFmodStudioBankName;
FMOD::Studio::Bank *loadedFmodStudioStringsBank;
FMOD::Studio::EventDescription *loadedFmodStudioEventDescription;
char *loadedFmodStudioEventPath;
FMOD::Studio::EventInstance *createdFmodStudioEventInstance;

// AdaptiveMusic global variables
const char *loadedBankName;
const char *startedEventPath;

class CAdaptiveMusicPlugin : public ISmmPlugin {

protected:
	// Plugin global variables
	IVEngineServer *engine;
	ISmmAPI *ismm;

public:
    bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);

    bool Unload(char *error, size_t maxlen);

    bool Pause(char *error, size_t maxlen);

    bool Unpause(char *error, size_t maxlen);

    void AllPluginsLoaded();

public:
    const char *GetAuthor();

    const char *GetName();

    const char *GetDescription();

    const char *GetURL();

    const char *GetLicense();

    const char *GetVersion();

    const char *GetDate();

    const char *GetLogTag();
/*
public:
    int StartFMODEngine();

    int StopFMODEngine();

    int LoadFMODBank(const char *bankName);

    int StartFMODEvent(const char *eventPath);

    int StopFMODEvent(const char *eventPath);

    int SetFMODGlobalParameter(const char *parameterName, float value);

    int SetFMODPausedState(bool pausedState);

    const char *GetFMODBankPath(const char *bankName);

public:
    void CalculateAdaptiveMusicState();

    void InitAdaptiveMusic();

    void ShutDownAdaptiveMusic();

    void ParseKeyValue(KeyValues *keyValue);*/
};

void Hook_ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);

extern CAdaptiveMusicPlugin g_AdaptiveMusicPlugin;

PLUGIN_GLOBALVARS();

#endif //_INCLUDE_ADAPTIVEMUSIC_PLUGIN_H_
