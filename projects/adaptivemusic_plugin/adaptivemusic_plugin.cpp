/**
 * ======================================================
 * Metamod:Source AdaptiveMusic Plugin
 * Written by Manuel Russello
 * ======================================================
 */

#include <stdio.h>
#include <list>
#include <string>
#include "vector"
#include "engine_wrappers.h"
#include "filesystem.h"
#include "itoolentity.h"

#include "adaptivemusic_plugin.h"
#include "adaptivemusic_watcher.h"

// FMOD Includes
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include "fmod_errors.h"

// Hooks
SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *,
              bool, bool);

SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);

SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool);

SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, 0);

SH_DECL_HOOK2_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, edict_t *, bool);

SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);

SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);

SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, 0, int);

SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, edict_t *);

SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char*, const char *, char *,
              int);

SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent *, bool);

#if SOURCE_ENGINE >= SE_ORANGEBOX

SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *, const CCommand &);
#else
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
#endif

CAdaptiveMusicPlugin g_AdaptiveMusicPlugin;
IServerGameDLL *server = NULL;
IServerGameClients *gameclients = NULL;
IVEngineServer *engine = NULL;
IServerPluginHelpers *helpers = NULL;
IGameEventManager2 *gameevents = NULL;
IServerPluginCallbacks *vsp_callbacks = NULL;
ICvar *icvar = NULL;
IFileSystem *filesystem = NULL;
CGlobalVars *gpGlobals = NULL;

/**
 * Something like this is needed to register cvars/CON_COMMANDs.
 */
class BaseAccessor : public IConCommandBaseAccessor {
public:
    bool RegisterConCommandBase(ConCommandBase *pCommandBase) {
        /* Always call META_REGCVAR instead of going through the engine. */
        return META_REGCVAR(pCommandBase);
    }
} s_BaseAccessor;

PLUGIN_EXPOSE(AdaptiveMusicPlugin, g_AdaptiveMusicPlugin);

bool CAdaptiveMusicPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) {
    PLUGIN_SAVEVARS();

    GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
    GET_V_IFACE_CURRENT(GetEngineFactory, gameevents, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
    GET_V_IFACE_CURRENT(GetEngineFactory, helpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
    GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetFileSystemFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
    GET_V_IFACE_ANY(GetServerFactory, serverTools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
    GET_V_IFACE_ANY(GetServerFactory, playerinfomanager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);

    gpGlobals = ismm->GetCGlobals();

    /* Load the VSP listener.  This is usually needed for IServerPluginHelpers. */
    if ((vsp_callbacks = ismm->GetVSPInfo(NULL)) == NULL) {
        ismm->AddListener(this, this);
        ismm->EnableVSPListener();
    }

    SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, server, this, &CAdaptiveMusicPlugin::Hook_LevelInit, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, server, this, &CAdaptiveMusicPlugin::Hook_ServerActivate, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, server, this, &CAdaptiveMusicPlugin::Hook_GameFrame, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, server, this, &CAdaptiveMusicPlugin::Hook_LevelShutdown, false);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientActive, gameclients, this, &CAdaptiveMusicPlugin::Hook_ClientActive,
                        true);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, gameclients, this,
                        &CAdaptiveMusicPlugin::Hook_ClientDisconnect, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, gameclients, this,
                        &CAdaptiveMusicPlugin::Hook_ClientPutInServer, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, gameclients, this,
                        &CAdaptiveMusicPlugin::Hook_SetCommandClient, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, gameclients, this,
                        &CAdaptiveMusicPlugin::Hook_ClientSettingsChanged, false);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientConnect, gameclients, this, &CAdaptiveMusicPlugin::Hook_ClientConnect,
                        false);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientCommand, gameclients, this, &CAdaptiveMusicPlugin::Hook_ClientCommand,
                        false);

    //ENGINE_CALL(LogPrint)("All hooks started!\n");

#if SOURCE_ENGINE >= SE_ORANGEBOX
    g_pCVar = icvar;
    ConVar_Register(0, &s_BaseAccessor);
#else
    ConCommandBaseMgr::OneTimeInit(&s_BaseAccessor);
#endif

    META_CONPRINTF("AdaptiveMusic Plugin - Plugin successfully started\n");

    // Start the FMOD engine
    StartFMODEngine();

    return true;
}

bool CAdaptiveMusicPlugin::Unload(char *error, size_t maxlen) {
    SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, server, this, &CAdaptiveMusicPlugin::Hook_LevelInit, true);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, server, this, &CAdaptiveMusicPlugin::Hook_ServerActivate,
                           true);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, server, this, &CAdaptiveMusicPlugin::Hook_GameFrame, true);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, server, this, &CAdaptiveMusicPlugin::Hook_LevelShutdown,
                           false);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientActive, gameclients, this,
                           &CAdaptiveMusicPlugin::Hook_ClientActive, true);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, gameclients, this,
                           &CAdaptiveMusicPlugin::Hook_ClientDisconnect, true);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, gameclients, this,
                           &CAdaptiveMusicPlugin::Hook_ClientPutInServer, true);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, gameclients, this,
                           &CAdaptiveMusicPlugin::Hook_SetCommandClient, true);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, gameclients, this,
                           &CAdaptiveMusicPlugin::Hook_ClientSettingsChanged, false);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientConnect, gameclients, this,
                           &CAdaptiveMusicPlugin::Hook_ClientConnect, false);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientCommand, gameclients, this,
                           &CAdaptiveMusicPlugin::Hook_ClientCommand, false);

    return true;
}

void CAdaptiveMusicPlugin::OnVSPListening(IServerPluginCallbacks *iface) {
    vsp_callbacks = iface;
}

void CAdaptiveMusicPlugin::Hook_ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {
    META_LOG(g_PLAPI, "ServerActivate() called: edictCount = %d, clientMax = %d", edictCount, clientMax);
}

void CAdaptiveMusicPlugin::AllPluginsLoaded() {
    /* This is where we'd do stuff that relies on the mod or other plugins
     * being initialized (for example, cvars added and events registered).
     */
}

void CAdaptiveMusicPlugin::Hook_ClientActive(edict_t *pEntity, bool bLoadGame) {
    META_LOG(g_PLAPI, "Hook_ClientActive(%d, %d)", IndexOfEdict(pEntity), bLoadGame);
}

#if SOURCE_ENGINE >= SE_ORANGEBOX

//-----------------------------------------------------------------------------
// Purpose: Provide a CLI to interact with the AdaptiveMusic Plugin through the console
//-----------------------------------------------------------------------------
void CAdaptiveMusicPlugin::Hook_ClientCommand(edict_t *pEntity, const CCommand &args)
#else
void CAdaptiveMusicPlugin::Hook_ClientCommand(edict_t *pEntity)
#endif
{
#if SOURCE_ENGINE <= SE_DARKMESSIAH
    CCommand args;
#endif
    const char *cmd = args.Arg(0);
    if (strcmp(cmd, "amp") == 0) {
        // amp commands
        if (args.ArgC() > 1 && strcmp(args.Arg(1), "") != 0) {
            const char *arg1 = args.Arg(1);
            if (strcmp(arg1, "fmod") == 0) {
                // amp fmod commands
                if (args.ArgC() > 2 && strcmp(args.Arg(2), "") != 0) {
                    const char *arg2 = args.Arg(2);
                    if (strcmp(arg2, "status") == 0) {
                        if (fmodStudioSystem->isValid()) {
                            Msg("AdaptiveMusic Plugin - FMOD engine is currently running\n");
                        } else {
                            Msg("AdaptiveMusic Plugin - FMOD engine is not currently running\n");
                        }
                        RETURN_META(MRES_SUPERCEDE);
                    }
                    if (strcmp(arg2, "loadbank") == 0) {
                        if (args.ArgC() > 3 && strcmp(args.Arg(3), "") != 0) {
                            g_AdaptiveMusicPlugin.LoadFMODBank(args.Arg(3));
                            RETURN_META(MRES_SUPERCEDE);
                        }
                        META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine\n");
                        META_CONPRINTF("usage: amp fmod loadbank <bankname>\n");
                        RETURN_META(MRES_SUPERCEDE);
                    }
                    if (strcmp(arg2, "startevent") == 0) {
                        if (args.ArgC() > 3 && strcmp(args.Arg(3), "") != 0) {
                            g_AdaptiveMusicPlugin.StartFMODEvent(args.Arg(3));
                            RETURN_META(MRES_SUPERCEDE);
                        }
                        META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine\n");
                        META_CONPRINTF("usage: amp fmod startevent <eventpath>\n");
                        RETURN_META(MRES_SUPERCEDE);
                    }
                    if (strcmp(arg2, "stopevent") == 0) {
                        if (args.ArgC() > 3 && strcmp(args.Arg(3), "") != 0) {
                            g_AdaptiveMusicPlugin.StopFMODEvent(args.Arg(3));
                            RETURN_META(MRES_SUPERCEDE);
                        }
                        META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine\n");
                        META_CONPRINTF("usage: amp fmod stopevent <eventpath>\n");
                        RETURN_META(MRES_SUPERCEDE);
                    }
                    if (strcmp(arg2, "setglobalparameter") == 0) {
                        if (args.ArgC() > 4 && strcmp(args.Arg(3), "") != 0 && strcmp(args.Arg(4), "") != 0) {
                            g_AdaptiveMusicPlugin.SetFMODGlobalParameter(args.Arg(3), atof(args.Arg(4)));
                            RETURN_META(MRES_SUPERCEDE);
                        }
                        META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine\n");
                        META_CONPRINTF("usage: amp fmod setglobalparameter <parametername> <parametervalue>\n");
                        RETURN_META(MRES_SUPERCEDE);
                    }
                }
                META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine\n");
                META_CONPRINTF("usage: amp fmod <subcommand>\n");
                META_CONPRINTF("                help - This list of subcommands\n");
                META_CONPRINTF("                status - Show the status of the FMOD engine\n");
                META_CONPRINTF("                loadbank <bankname> - Load an FMOD bank\n");
                META_CONPRINTF("                startevent <eventpath> - Start an FMOD event\n");
                META_CONPRINTF("                stopevent <eventpath> - Stop an FMOD event\n");
                META_CONPRINTF(
                        "                setglobalparameter <parametername> <parametervalue> - Set an FMOD global parameter's value\n");
                RETURN_META(MRES_SUPERCEDE);
            }
        }
        META_CONPRINTF("AdaptiveMusic Plugin\n");
        META_CONPRINTF("usage: amp <command>\n");
        META_CONPRINTF("           help - This list of commands\n");
        META_CONPRINTF("           fmod <subcommand> - FMOD engine commands\n");
        RETURN_META(MRES_SUPERCEDE);
    }
}

void CAdaptiveMusicPlugin::Hook_ClientSettingsChanged(edict_t *pEdict) {
    /*
    if (playerinfomanager) {
        IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEdict);

        const char *name = engine->GetClientConVarValue(IndexOfEdict(pEdict), "name");

        if (playerinfo != NULL
            && name != NULL
            && strcmp(engine->GetPlayerNetworkIDString(pEdict), "BOT") != 0
            && playerinfo->GetName() != NULL
            && strcmp(name, playerinfo->GetName()) == 0) {
            char msg[128];
            MM_Format(msg, sizeof(msg), "Your name changed to \"%s\" (from \"%s\")\n", name, playerinfo->GetName());
            engine->ClientPrintf(pEdict, msg);
        }
    }
     */
}

bool CAdaptiveMusicPlugin::Hook_ClientConnect(edict_t *pEntity,
                                              const char *pszName,
                                              const char *pszAddress,
                                              char *reject,
                                              int maxrejectlen) {
    META_LOG(g_PLAPI, "Hook_ClientConnect(%d, \"%s\", \"%s\")", IndexOfEdict(pEntity), pszName, pszAddress);

    return true;
}

void CAdaptiveMusicPlugin::Hook_ClientPutInServer(edict_t *pEntity, char const *playername) {
    // Print a message
    KeyValues *kv = new KeyValues("msg");
    kv->SetString("title", "AdaptiveMusic Plugin loaded");
    kv->SetString("msg", "AdaptiveMusic Plugin loaded");
    kv->SetColor("color", Color(0, 0, 0, 255));
    kv->SetInt("level", 5);
    kv->SetInt("time", 10);
    helpers->CreateMessage(pEntity, DIALOG_MSG, kv, vsp_callbacks);
    kv->deleteThis();
    // Set variables
    pAdaptiveMusicPlayer = pEntity;
}

void CAdaptiveMusicPlugin::Hook_ClientDisconnect(edict_t *pEntity) {
    META_LOG(g_PLAPI, "Hook_ClientDisconnect(%d)", IndexOfEdict(pEntity));
    pAdaptiveMusicPlayer = nullptr;
}

void CAdaptiveMusicPlugin::Hook_GameFrame(bool simulating) {
    // Manage if the FMOD engine should be paused or not
    if (adaptiveMusicAvailable && startedFMODStudioEventPath) {
        if (simulating && knownFMODPausedState) {
            SetFMODPausedState(false);
        }
        if (!simulating && !knownFMODPausedState) {
            SetFMODPausedState(true);
        }
    }
    if (simulating) {
        // Make the watchers think
        for (auto const &watcher: *adaptiveMusicWatchers) {
            watcher->Think();
        }
    }
}

bool CAdaptiveMusicPlugin::Hook_LevelInit(const char *pMapName,
                                          char const *pMapEntities,
                                          char const *pOldLevel,
                                          char const *pLandmarkName,
                                          bool loadGame,
                                          bool background) {
    META_LOG(g_PLAPI, "Hook_LevelInit(%s)", pMapName);
    CalculateAdaptiveMusicState();
    if (adaptiveMusicAvailable) {
        InitAdaptiveMusic();
    }
    return true;
}

void CAdaptiveMusicPlugin::Hook_LevelShutdown() {
    META_LOG(g_PLAPI, "Hook_LevelShutdown()");
}

void CAdaptiveMusicPlugin::Hook_SetCommandClient(int index) {
    META_LOG(g_PLAPI, "Hook_SetCommandClient(%d)", index);
}

bool CAdaptiveMusicPlugin::Pause(char *error, size_t maxlen) {
    return true;
}

bool CAdaptiveMusicPlugin::Unpause(char *error, size_t maxlen) {
    return true;
}

const char *CAdaptiveMusicPlugin::GetLicense() {
    return "idk yet";
}

const char *CAdaptiveMusicPlugin::GetVersion() {
    return "1.0.0.0";
}

const char *CAdaptiveMusicPlugin::GetDate() {
    return __DATE__;
}

const char *CAdaptiveMusicPlugin::GetLogTag() {
    return "ADAPTIVEMUSIC";
}

const char *CAdaptiveMusicPlugin::GetAuthor() {
    return "Manuel Russello";
}

const char *CAdaptiveMusicPlugin::GetDescription() {
    return "AdaptiveMusic Plugin implementing FMOD";
}

const char *CAdaptiveMusicPlugin::GetName() {
    return "AdaptiveMusic Plugin";
}

const char *CAdaptiveMusicPlugin::GetURL() {
    return "https://hl2musicmod.russello.studio/";
}

//-----------------------------------------------------------------------------
// FMOD "client" specific methods
//-----------------------------------------------------------------------------

//// START HELPER FUNCTIONS

//-----------------------------------------------------------------------------
// Purpose: Concatenate 2 strings together
// Input:
// - str1: The starting string
// - str2: The ending string
// Output: The joined 2 strings
//-----------------------------------------------------------------------------
const char *Concatenate(const char *str1, const char *str2) {
    size_t len1 = 0;
    size_t len2 = 0;
    while (str1[len1] != '\0')
        ++len1;
    while (str2[len2] != '\0')
        ++len2;
    char *result = new char[len1 + len2 + 1]; // +1 for the null terminator
    for (size_t i = 0; i < len1; ++i)
        result[i] = str1[i];
    for (size_t i = 0; i < len2; ++i)
        result[len1 + i] = str2[i];
    result[len1 + len2] = '\0';
    return result;
}

void UTIL_StringToFloatArray(float *pVector, int count, const char *pString) {
    char *pstr, *pfront, tempString[128];
    int j;

    Q_strncpy(tempString, pString, sizeof(tempString));
    pstr = pfront = tempString;

    for (j = 0; j < count; j++)            // lifted from pr_edict.c
    {
        pVector[j] = atof(pfront);

        // skip any leading whitespace
        while (*pstr && *pstr <= ' ')
            pstr++;

        // skip to next whitespace
        while (*pstr && *pstr > ' ')
            pstr++;

        if (!*pstr)
            break;

        pstr++;
        pfront = pstr;
    }
    for (j++; j < count; j++) {
        pVector[j] = 0;
    }
}

void UTIL_StringToVector(float *pVector, const char *pString) {
    UTIL_StringToFloatArray(pVector, 3, pString);
}

//-----------------------------------------------------------------------------
// Purpose: Helper method to sanitize the name of an FMOD Bank, adding ".bank" if it's not already present
// Input: The FMOD Bank name to sanitize
// Output: The sanitized Bank name (same as the initial if it was already ending with ".bank")
//-----------------------------------------------------------------------------
const char *SanitizeBankName(const char *bankName) {
    const char *bankExtension = ".bank";
    size_t bankNameLength = strlen(bankName);
    size_t bankExtensionLength = strlen(bankExtension);
    if (bankNameLength >= bankExtensionLength &&
        Q_strcmp(bankName + bankNameLength - bankExtensionLength, bankExtension)
        == 0) {
        return bankName;
    }
    return Concatenate(bankName, bankExtension);
}

//// END HELPER FUNCTIONS

//-----------------------------------------------------------------------------
// Purpose: Start the FMOD Studio System and initialize it
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicPlugin::StartFMODEngine() {
    FMOD_RESULT result;
    result = FMOD::Studio::System::create(&fmodStudioSystem);
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine could not be created (%d): %s\n", result,
                       FMOD_ErrorString(result));
        return (result);
    }
    result = fmodStudioSystem->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine could not initialize (%d): %s\n", result,
                       FMOD_ErrorString(result));
        return (result);
    }
    META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine successfully started\n");
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Stop the FMOD Studio System
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicPlugin::StopFMODEngine() {
    FMOD_RESULT result;
    result = fmodStudioSystem->release();
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine could not be released (%d): %s\n", result,
                       FMOD_ErrorString(result));
        return (result);
    }
    META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine successfully stopped\n");
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Get the path of a Bank file in the /sound/fmod/banks folder from the GamePath
// Input: The FMOD Bank name to locate
// Output: The FMOD Bank's full path from the file system
//-----------------------------------------------------------------------------
const char *CAdaptiveMusicPlugin::GetFMODBankPath(const char *bankName) {
    const char *sanitizedBankName = SanitizeBankName(bankName);
    char *bankPath = new char[512];
    Q_snprintf(bankPath, 512, "%s/sound/fmod/banks/%s", g_SMAPI->GetBaseDir(), sanitizedBankName);
    // convert backwards slashes to forward slashes
    for (int i = 0; i < 512; i++) {
        if (bankPath[i] == '\\')
            bankPath[i] = '/';
    }
    return bankPath;
}

//-----------------------------------------------------------------------------
// Purpose: Load an FMOD Bank
// Input: The name of the FMOD Bank to load
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicPlugin::LoadFMODBank(const char *bankName) {
    if (loadedFMODStudioBankName != nullptr && (strcmp(bankName, loadedFMODStudioBankName) == 0)) {
        // Bank is already loaded
        META_CONPRINTF("AdaptiveMusic Plugin - FMOD bank requested for loading but already loaded: %s\n", bankName);
    } else {
        // Load the requested bank
        const char *bankPath = CAdaptiveMusicPlugin::GetFMODBankPath(bankName);
        FMOD_RESULT result;
        result = fmodStudioSystem->loadBankFile(bankPath, FMOD_STUDIO_LOAD_BANK_NORMAL,
                                                &loadedFMODStudioBank);
        if (result != FMOD_OK) {
            META_CONPRINTF("AdaptiveMusic Plugin - Could not load FMOD bank: %s. Error (%d): %s\n", bankName, result,
                           FMOD_ErrorString(result));
            return (-1);
        }
        const char *bankStringsName = Concatenate(bankName, ".strings");
        result = fmodStudioSystem->loadBankFile(CAdaptiveMusicPlugin::GetFMODBankPath(bankStringsName),
                                                FMOD_STUDIO_LOAD_BANK_NORMAL,
                                                &loadedFMODStudioStringsBank);
        if (result != FMOD_OK) {
            META_CONPRINTF("AdaptiveMusic Plugin - Could not load FMOD bank: %s. Error (%d): %\n", bankStringsName,
                           result,
                           FMOD_ErrorString(result));
            return (-1);
        }
        META_CONPRINTF("AdaptiveMusic Plugin - Bank successfully loaded: %s\n", bankName);
        delete[] loadedFMODStudioBankName;
        loadedFMODStudioBankName = new char[strlen(bankName) + 1];
        strcpy(loadedFMODStudioBankName, bankName);
    }
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Start an FMOD Event
// Input: The name of the FMOD Event to start
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicPlugin::StartFMODEvent(const char *eventPath) {
    if (startedFMODStudioEventPath != nullptr && (Q_strcmp(eventPath, startedFMODStudioEventPath) == 0)) {
        // Event is already loaded
        META_CONPRINTF("AdaptiveMusic Plugin - Event requested for starting but already started (%s)\n", eventPath);
    } else {
        // Event is new
        if (startedFMODStudioEventPath != nullptr && (Q_strcmp(startedFMODStudioEventPath, "") != 0)) {
            // Stop the currently playing event
            StopFMODEvent(startedFMODStudioEventPath);
        }
        const char *fullEventPath = Concatenate("event:/", eventPath);
        FMOD_RESULT result;
        result = fmodStudioSystem->getEvent(fullEventPath, &startedFMODStudioEventDescription);
        result = startedFMODStudioEventDescription->createInstance(&createdFMODStudioEventInstance);
        result = createdFMODStudioEventInstance->start();
        fmodStudioSystem->update();
        if (result != FMOD_OK) {
            META_CONPRINTF("AdaptiveMusic Plugin - Could not start Event (%s). Error: (%d) %s\n", eventPath, result,
                           FMOD_ErrorString(result));
            return (-1);
        }
        META_CONPRINTF("AdaptiveMusic Plugin - Event successfully started (%s)\n", eventPath);
        delete[] startedFMODStudioEventPath;
        startedFMODStudioEventPath = new char[strlen(eventPath) + 1];
        strcpy(startedFMODStudioEventPath, eventPath);
    }
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Stop an FMOD Event
// Input: The name of the FMOD Event to stop
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicPlugin::StopFMODEvent(const char *eventPath) {
    const char *fullEventPath = Concatenate("event:/", eventPath);
    FMOD_RESULT result;
    result = fmodStudioSystem->getEvent(fullEventPath, &startedFMODStudioEventDescription);
    result = startedFMODStudioEventDescription->releaseAllInstances();
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - Could not stop Event (%s). Error: (%d) %s\n", eventPath, result,
                       FMOD_ErrorString(result));
        return (-1);
    }
    META_CONPRINTF("AdaptiveMusic Plugin - Event successfully stopped (%s)\n", eventPath);
    delete[] startedFMODStudioEventPath;
    startedFMODStudioEventPath = new char[strlen("") + 1];
    strcpy(startedFMODStudioEventPath, "");
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Set the value for a global FMOD Parameter
// Input:
// - parameterName: The name of the FMOD Parameter to set
// - value: The value to set the FMOD Parameter to
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicPlugin::SetFMODGlobalParameter(const char *parameterName, float value) {
    FMOD_RESULT result;
    result = fmodStudioSystem->setParameterByName(parameterName, value);
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - Could not set Global Parameter value (%s) (%f). Error: (%d) %s\n",
                       parameterName, value,
                       result, FMOD_ErrorString(result));
        return (-1);
    }
    META_CONPRINTF("AdaptiveMusic Plugin - Global Parameter %s set to %f\n", parameterName, value);
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Pause/Unpause the playback of the engine
// Input:
// - pausedState: true if the desired state of the playback is "paused", false otherwise
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicPlugin::SetFMODPausedState(bool pausedState) {
    META_CONPRINTF("AdaptiveMusic Plugin - Setting the FMOD master bus paused state to %d\n", pausedState);
    FMOD::Studio::Bus *bus;
    FMOD_RESULT result;
    result = fmodStudioSystem->getBus("bus:/", &bus);
    if (result != FMOD_OK) {
        Msg("AdaptiveMusic Plugin - Could not find the FMOD master bus! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    result = bus->setPaused(pausedState);
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        Msg("AdaptiveMusic Plugin - Could not pause the FMOD master bus! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    knownFMODPausedState = pausedState;
    return (0);
}

//-----------------------------------------------------------------------------
// Adaptive Music System (inherited from the Server-side)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Checks if the level has adaptive music data
// Sets the available variable if we can find adaptive music data for this level
//-----------------------------------------------------------------------------
void CAdaptiveMusicPlugin::CalculateAdaptiveMusicState() {
    char szFullName[512];
    Q_snprintf(szFullName, sizeof(szFullName), "maps/%s_adaptivemusic.txt", STRING(gpGlobals->mapname));
    if (filesystem->FileExists(szFullName)) {
        META_CONPRINTF("AdaptiveMusic Plugin - Found adaptive music file, '%s'\n", szFullName);
        adaptiveMusicAvailable = true;
    } else {
        META_CONPRINTF("AdaptiveMusic Plugin - Could not find adaptive music file, '%s'\n", szFullName);
        adaptiveMusicAvailable = false;
        //ShutDownAdaptiveMusic();
    }
}

//-----------------------------------------------------------------------------
// Purpose: Start the AdaptiveMusic up
// Finds the adaptive music file, parse it and initialize everything related to AdaptiveMusic for this level
//-----------------------------------------------------------------------------
void CAdaptiveMusicPlugin::InitAdaptiveMusic() {
    adaptiveMusicWatchers = new std::list<CAdaptiveMusicWatcher *>();
    // If we know there's Adaptive Music data, initialize the Adaptive Music
    if (adaptiveMusicAvailable) {
        META_CONPRINTF("AdaptiveMusic Plugin - Initializing the map's adaptive music data\n");
        // Find the adaptive music file
        char szFullName[512];
        Q_snprintf(szFullName, sizeof(szFullName), "maps/%s_adaptivemusic.txt", STRING(gpGlobals->mapname));
        auto *keyValue = new KeyValues("adaptive_music");
        if (keyValue->LoadFromFile(filesystem, szFullName, "MOD")) {
            META_CONPRINTF("AdaptiveMusic Plugin - Loading adaptive music data from '%s'\n", szFullName);
            KeyValues *keyValueSubset = keyValue->GetFirstSubKey();
            while (keyValueSubset) {
                ParseKeyValue(keyValueSubset);
                keyValueSubset = keyValueSubset->GetNextKey();
            }
        } else {
            META_CONPRINTF("AdaptiveMusic Plugin - Could not load adaptive music file '%s'\n", szFullName);
        }
    }
        // Else shutdown the Adaptive Music
    else {
        ShutDownAdaptiveMusic();
    }
}

//-----------------------------------------------------------------------------
// Purpose: Shuts down the adaptive music for the map
//-----------------------------------------------------------------------------
void CAdaptiveMusicPlugin::ShutDownAdaptiveMusic() {
    META_CONPRINTF("AdaptiveMusic Plugin - Shutting down adaptive music for the map\n");
    if (startedFMODStudioEventPath != nullptr) {
        StopFMODEvent(startedFMODStudioEventPath);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Parses the provided KeyValue containing adaptive music data from a file
// Input: A KeyValue object, subset of an adaptive music file
//-----------------------------------------------------------------------------
void CAdaptiveMusicPlugin::ParseKeyValue(KeyValues *keyValue) {
    const char *keyValueName = keyValue->GetName();
    META_CONPRINTF("AdaptiveMusic Plugin - Parsing the KeyValue '%s'\n", keyValueName);
    if (!Q_strcmp(keyValueName, "globals")) {
        // The key-value element is defining the global parameters of the map
        KeyValues *element = keyValue->GetFirstSubKey();
        while (element) {
            const char *elementKey = element->GetName();
            const char *elementValue = element->GetString();
            META_CONPRINTF("AdaptiveMusic Plugin - %s: %s\n", elementKey, elementValue);
            if (!Q_strcmp(elementKey, "bank")) {
                // Load the FMOD Bank
                LoadFMODBank(elementValue);
            } else if (!Q_strcmp(elementKey, "event")) {
                // Start the FMOD Event
                StartFMODEvent(elementValue);
            }
            element = element->GetNextKey();
        }
    } else if (!Q_strcmp(keyValueName, "watcher")) {
        // The key-value element is defining a watcher and its params for the map
        KeyValues *element = keyValue->GetFirstSubKey();

        // Watcher settings
        const char *watcherType = nullptr;
        const char *watcherParam = nullptr;
        //const char* watcherEntityClass = nullptr;
        //const char* watcherEntityWatchedStatus = nullptr;
        //const char* watcherEntityWatchedScene = nullptr;
        auto *watcherZones = new std::list<CAdaptiveMusicZone *>();
        //auto* watcherScenes = new std::list<Scene>();

        // Step 1: parse all elements to gather the watcher settings
        while (element) {
            const char *elementKey = element->GetName();
            const char *elementValue = element->GetString();
            if (!Q_strcmp(elementKey, "type")) {
                watcherType = elementValue;
            } else if (!Q_strcmp(elementKey, "parameter")) {
                watcherParam = elementValue;
            }
                /*
                else if (!Q_strcmp(elementKey, "entity_class")) {
                    watcherEntityClass = elementValue;
                }
                else if (!Q_strcmp(elementKey, "entity_watched_status")) {
                    watcherEntityWatchedStatus = elementValue;
                }
                else if (!Q_strcmp(elementKey, "entity_watched_scene")) {
                    watcherEntityWatchedScene = elementValue;
                }
                */
            else if (!Q_strcmp(elementKey, "zones")) {
                // Zone list settings
                KeyValues *zone = element->GetFirstSubKey();
                while (zone) {
                    const char *zoneKey = zone->GetName();
                    if (!Q_strcmp(zoneKey, "zone")) {
                        KeyValues *zoneElement = zone->GetFirstSubKey();
                        // Create a new Zone struct
                        auto *pZone = new CAdaptiveMusicZone;
                        while (zoneElement) {
                            const char *zoneElementKey = zoneElement->GetName();
                            const char *zoneElementValue = zoneElement->GetString();
                            if (!Q_strcmp(zoneElementKey, "min_origin")) {
                                UTIL_StringToVector(pZone->minOrigin, zoneElementValue);
                            } else if (!Q_strcmp(zoneElementKey, "max_origin")) {
                                UTIL_StringToVector(pZone->maxOrigin, zoneElementValue);
                            } else if (!Q_strcmp(zoneElementKey, "parameter")) {
                                pZone->parameterName = zoneElementValue;
                            }
                            zoneElement = zoneElement->GetNextKey();
                        }
                        watcherZones->push_back(pZone);
                    } else {
                        Warning("FMOD Adaptive Music - Found an unknown \"zones\" subkey in file\n");
                    }
                    zone = zone->GetNextKey();
                }
            }
            /*
            else if (!Q_strcmp(elementKey, "scenes")) {
                // Scene list settings
                KeyValues* scene = element->GetFirstSubKey();
                while (scene) {
                    const char* sceneKey = scene->GetName();
                    if (!Q_strcmp(sceneKey, "scene")) {
                        KeyValues* sceneElement = scene->GetFirstSubKey();
                        // Create a new Scene struct
                        Scene pScene;
                        while (sceneElement) {
                            const char* sceneElementKey = sceneElement->GetName();
                            const char* sceneElementValue = sceneElement->GetString();
                            if (!Q_strcmp(sceneElementKey, "scene_name")) {
                                pScene.sceneName = sceneElementValue;
                            }
                            else if (!Q_strcmp(sceneElementKey, "scene_state")) {
                                pScene.stateName = sceneElementValue;
                            }
                            else if (!Q_strcmp(sceneElementKey, "parameter")) {
                                pScene.parameterName = sceneElementValue;
                            }
                            sceneElement = sceneElement->GetNextKey();
                        }
                        watcherScenes->push_back(pScene);
                    }
                    else {
                        Warning("FMOD Adaptive Music - Found an unknown \"scenes\" subkey in file\n");
                    }
                    scene = scene->GetNextKey();
                }
            }
            */
            element = element->GetNextKey();
        }

        // Step 2: Init the watcher according to the gathered settings
        if (watcherType == nullptr) {
            Warning("FMOD Adaptive Music - Found a AdaptiveMusicWatcher to create but no type was provided\n");
            return;
        }
        if (!Q_strcmp(watcherType, "health")) {
            // Create and init the watcher entity, then set its params
            auto *healthWatcher = new CAdaptiveMusicHealthWatcher;
            healthWatcher->SetAdaptiveMusicPlugin(this);
            healthWatcher->SetParameterName(watcherParam);
            healthWatcher->Init();
            adaptiveMusicWatchers->push_back(healthWatcher);
        }
            /*
            else if (!Q_strcmp(watcherType, "suit")) {
                // Create and spawn the watcher entity, then set its params
                CBaseEntity* pNode = CreateEntityByName("adaptive_music_suit_watcher");
                if (pNode) {
                    DispatchSpawn(pNode);
                    auto* suitWatcher = dynamic_cast<CAdaptiveMusicSuitWatcher *>(pNode);
                    suitWatcher->SetAdaptiveMusicSystem(this);
                    suitWatcher->SetParameterName(watcherParam);
                    suitWatcher->Activate();
                }
                else {
                    Warning("FMOD Adaptive Music - Failed to spawn a SuitWatcher entity\n");
                }
            }
            else if (!Q_strcmp(watcherType, "chased")) {
                // Create and spawn the watcher entity, then set its params
                CBaseEntity* pNode = CreateEntityByName("adaptive_music_chased_watcher");
                if (pNode) {
                    DispatchSpawn(pNode);
                    auto* chasedWatcher = dynamic_cast<CAdaptiveMusicChasedWatcher *>(pNode);
                    chasedWatcher->SetAdaptiveMusicSystem(this);
                    chasedWatcher->SetParameterName(watcherParam);
                    chasedWatcher->Activate();
                }
                else {
                    Warning("FMOD Adaptive Music - Failed to spawn a SuitWatcher entity\n");
                }
            }
            else if (!Q_strcmp(watcherType, "entity")) {
                // Create and spawn the watcher entity, then set its params
                CBaseEntity* pNode = CreateEntityByName("adaptive_music_entity_watcher");
                if (pNode) {
                    DispatchSpawn(pNode);
                    auto* entityWatcher = dynamic_cast<CAdaptiveMusicEntityWatcher *>(pNode);
                    entityWatcher->SetAdaptiveMusicSystem(this);
                    entityWatcher->SetEntityClass(watcherEntityClass);
                    entityWatcher->SetEntityWatchedStatus(watcherEntityWatchedStatus);
                    entityWatcher->SetEntityWatchedScene(watcherEntityWatchedScene);
                    entityWatcher->SetParameterName(watcherParam);
                    entityWatcher->Activate();
                }
                else {
                    Warning("FMOD Adaptive Music - Failed to spawn a EntityWatcher entity\n");
                }
            }
            */
        else if (!Q_strcmp(watcherType, "zone")) {
            // Step 3, for zone watchers: find the zones key and parse it to find the zones to set
            // Create and init the watcher entity, then set its params
            auto *zoneWatcher = new CAdaptiveMusicZoneWatcher;
            zoneWatcher->SetAdaptiveMusicPlugin(this);
            zoneWatcher->SetZones(watcherZones);
            zoneWatcher->Init();
            adaptiveMusicWatchers->push_back(zoneWatcher);
        }
            /*
            else if (!Q_strcmp(watcherType, "scene")) {
                // Step 3, for scene watchers: find the scenes key and parse it to find the scenes to set
                // Create and spawn the watcher entity, then set its params
                CBaseEntity* pNode = CreateEntityByName("adaptive_music_scene_watcher");
                if (pNode) {
                    DispatchSpawn(pNode);
                    auto* sceneWatcher = dynamic_cast<CAdaptiveMusicSceneWatcher *>(pNode);
                    sceneWatcher->SetAdaptiveMusicSystem(this);
                    sceneWatcher->SetScenes(watcherScenes);
                    sceneWatcher->Activate();
                }
                else {
                    Warning("FMOD Adaptive Music - Failed to spawn a ZoneWatcher entity\n");
                }
            }
             */
        else {
            Warning("FMOD Adaptive Music - Unknown watcher type: %s\n", watcherType);
        }
    }
}
