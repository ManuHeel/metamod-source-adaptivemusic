/**
 * ======================================================
 * Metamod:Source AdaptiveMusic Plugin
 * Written by Manuel Russello
 * ======================================================
 */

#ifndef _INCLUDE_ADAPTIVEMUSIC_PLUGIN_H_
#define _INCLUDE_ADAPTIVEMUSIC_PLUGIN_H_

#include <ISmmPlugin.h>
#include <igameevents.h>
#include <iplayerinfo.h>
#include <sh_vector.h>
#include "engine_wrappers.h"

// FMOD Includes
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include "fmod_errors.h"

#if defined WIN32 && !defined snprintf
#define snprintf _snprintf
#endif


class CAdaptiveMusicPlugin : public ISmmPlugin, public IMetamodListener {

protected:
    // FMOD global variables
    FMOD::Studio::System *fmodStudioSystem;
    FMOD::Studio::Bank *loadedFMODStudioBank;
    char *loadedFMODStudioBankName;
    FMOD::Studio::Bank *loadedFMODStudioStringsBank;
    FMOD::Studio::EventDescription *loadedFMODStudioEventDescription;
    char *loadedFMODStudioEventPath;
    FMOD::Studio::EventInstance *createdFMODStudioEventInstance;
    bool knownFMODPausedState;

public: // Main Metamod:Source plugins API callbacks
    bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);

    bool Unload(char *error, size_t maxlen);

    bool Pause(char *error, size_t maxlen);

    bool Unpause(char *error, size_t maxlen);

    void AllPluginsLoaded();

public: // IMetamodListener
    void OnVSPListening(IServerPluginCallbacks *iface);

public: // Hooks
    void Hook_ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);

    bool Hook_LevelInit(const char *pMapName,
                        char const *pMapEntities,
                        char const *pOldLevel,
                        char const *pLandmarkName,
                        bool loadGame,
                        bool background);

    void Hook_GameFrame(bool simulating);

    void Hook_LevelShutdown(void);

    void Hook_ClientActive(edict_t *pEntity, bool bLoadGame);

    void Hook_ClientDisconnect(edict_t *pEntity);

    void Hook_ClientPutInServer(edict_t *pEntity, char const *playername);

    void Hook_SetCommandClient(int index);

    void Hook_ClientSettingsChanged(edict_t *pEdict);

    bool Hook_ClientConnect(edict_t *pEntity,
                            const char *pszName,
                            const char *pszAddress,
                            char *reject,
                            int maxrejectlen);

#if SOURCE_ENGINE >= SE_ORANGEBOX

    void Hook_ClientCommand(edict_t *pEntity, const CCommand &args);

#else
    void Hook_ClientCommand(edict_t *pEntity);
#endif

public: // Metamod:Source plugins information
    const char *GetAuthor();

    const char *GetName();

    const char *GetDescription();

    const char *GetURL();

    const char *GetLicense();

    const char *GetVersion();

    const char *GetDate();

    const char *GetLogTag();

public: // FMOD engine API methods
    int StartFMODEngine();

    int StopFMODEngine();

    const char *GetFMODBankPath(const char *bankName);

    int LoadFMODBank(const char *bankName);

    int StartFMODEvent(const char *eventPath);

    int StopFMODEvent(const char *eventPath);

    int SetFMODGlobalParameter(const char *parameterName, float value);

    int SetFMODPausedState(bool pausedState);
/*
public: // Adaptive music system management
    void CalculateAdaptiveMusicState();

    void InitAdaptiveMusic();

    void ShutDownAdaptiveMusic();

    void ParseKeyValue(KeyValues *keyValue);
*/
};

extern CAdaptiveMusicPlugin g_AdaptiveMusicPlugin;

PLUGIN_GLOBALVARS();

#endif //_INCLUDE_ADAPTIVEMUSIC_PLUGIN_H_
