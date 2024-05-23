/**
 * ======================================================
 * Metamod:Source AdaptiveMusic Plugin
 * Written by Manuel Russello
 * ======================================================
 */

#include <stdio.h>
#include "adaptivemusic_plugin.h"

// FMOD Includes
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include "fmod_errors.h"

SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);

CAdaptiveMusicPlugin g_AdaptiveMusicPlugin;
IServerGameDLL *server = NULL;
ICvar *icvar = NULL;

/**
 * Something like this is needed to register cvars/CON_COMMANDs.
 */
class BaseAccessor : public IConCommandBaseAccessor
{
public:
    bool RegisterConCommandBase(ConCommandBase *pCommandBase)
    {
        /* Always call META_REGCVAR instead of going through the engine. */
        return META_REGCVAR(pCommandBase);
    }
} s_BaseAccessor;

PLUGIN_EXPOSE(AdaptiveMusicPlugin, g_AdaptiveMusicPlugin);
bool CAdaptiveMusicPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

    this->ismm = ismm;

	/* Make sure we build on MM:S 1.4 */
#if defined METAMOD_PLAPI_VERSION
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
#else
	GET_V_IFACE_ANY(serverFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
#endif

    // Get the factories running to hook the Interfaces
    GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);

	SH_ADD_HOOK_STATICFUNC(IServerGameDLL, ServerActivate, server, Hook_ServerActivate, true);

#if SOURCE_ENGINE >= SE_ORANGEBOX
    g_pCVar = icvar;
    ConVar_Register(0, &s_BaseAccessor);
#else
    ConCommandBaseMgr::OneTimeInit(&s_BaseAccessor);
#endif

    // Start the FMOD engine
    // StartFMODEngine();

	return true;
}

bool CAdaptiveMusicPlugin::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK_STATICFUNC(IServerGameDLL, ServerActivate, server, Hook_ServerActivate, true);

	return true;
}

void Hook_ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	META_LOG(g_PLAPI, "ServerActivate() called: edictCount = %d, clientMax = %d", edictCount, clientMax);
}

void CAdaptiveMusicPlugin::AllPluginsLoaded()
{
	/* This is where we'd do stuff that relies on the mod or other plugins
	 * being initialized (for example, cvars added and events registered).
	 */
}

bool CAdaptiveMusicPlugin::Pause(char *error, size_t maxlen)
{
	return true;
}

bool CAdaptiveMusicPlugin::Unpause(char *error, size_t maxlen)
{
	return true;
}

const char *CAdaptiveMusicPlugin::GetLicense()
{
	return "idk yet";
}

const char *CAdaptiveMusicPlugin::GetVersion()
{
	return "1.0.0.0";
}

const char *CAdaptiveMusicPlugin::GetDate()
{
	return __DATE__;
}

const char *CAdaptiveMusicPlugin::GetLogTag()
{
	return "ADAPTIVEMUSIC";
}

const char *CAdaptiveMusicPlugin::GetAuthor()
{
	return "Manuel Russello";
}

const char *CAdaptiveMusicPlugin::GetDescription()
{
	return "AdaptiveMusic Plugin implementing FMOD";
}

const char *CAdaptiveMusicPlugin::GetName()
{
	return "AdaptiveMusic Plugin";
}

const char *CAdaptiveMusicPlugin::GetURL()
{
	return "https://hl2musicmod.russello.studio/";
}

//-----------------------------------------------------------------------------
// FMOD "client" specific methods
//-----------------------------------------------------------------------------

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

//// END HELPER FUNCTIONS

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to print the FMOD Engine Status
//-----------------------------------------------------------------------------

/*
CON_COMMAND_F(fmod_getstatus, "FMOD: Get current status of the FMOD Engine", FCVAR_NONE) {
    bool isValid = fmodStudioSystem->isValid();
    if (isValid) {
        META_CONPRINTF("FMOD Client - Engine is currently running\n");
    } else {
        META_CONPRINTF("FMOD Client - Engine is not running\n");
    }
}
*/
/*
void CC_GetStatus() {
    bool isValid = fmodStudioSystem->isValid();
    if (isValid) {
        Msg("FMOD Client - Engine is currently running\n");
    } else {
        Msg("FMOD Client - Engine is not running\n");
    }
}

ConCommand getStatus("fmod_getstatus", CC_GetStatus, "FMOD: Get current status of the FMOD Manager");

//-----------------------------------------------------------------------------
// Purpose: Load an FMOD Bank
// Input: The name of the FMOD Bank to load
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicPlugin::LoadFMODBank(const char *bankName) {
    if (loadedFmodStudioBankName != nullptr && (strcmp(bankName, loadedFmodStudioBankName) == 0)) {
        // Bank is already loaded
        Log("FMOD Client - Bank requested for loading but already loaded (%s)\n", bankName);
    } else {
        // Load the requested bank
        const char *bankPath = CAdaptiveMusicPlugin::GetFMODBankPath(bankName);
        FMOD_RESULT result;
        result = fmodStudioSystem->loadBankFile(bankPath, FMOD_STUDIO_LOAD_BANK_NORMAL,
                                                &loadedFmodStudioBank);
        if (result != FMOD_OK) {
            Warning("FMOD Client - Could not load Bank (%s). Error: (%d) %s\n", bankName, result,
                    FMOD_ErrorString(result));
            return (-1);
        }
        const char *bankStringsName = Concatenate(bankName, ".strings");
        result = fmodStudioSystem->loadBankFile(CAdaptiveMusicPlugin::GetFMODBankPath(bankStringsName),
                                                FMOD_STUDIO_LOAD_BANK_NORMAL,
                                                &loadedFmodStudioStringsBank);
        if (result != FMOD_OK) {
            Warning("FMOD Client - Could not load Strings Bank (%s). Error: (%d) %s\n", bankStringsName, result,
                    FMOD_ErrorString(result));
            return (-1);
        }
        Log("FMOD Client - Bank successfully loaded (%s)\n", bankName);
        delete[] loadedFmodStudioBankName;
        loadedFmodStudioBankName = new char[strlen(bankName) + 1];
        strcpy(loadedFmodStudioBankName, bankName);
        return (0);
    }

    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to load a FMOD Bank
// Input: The name of the FMOD Bank to load as ConCommand argument
//-----------------------------------------------------------------------------
void CC_LoadFMODBank(const CCommand &args) {
    if (args.ArgC() < 1 || strcmp(args.Arg(1), "") == 0) {
        Msg("Usage: fmod_loadbank <bankname>\n");
        return;
    }
    g_AdaptiveMusicPlugin.LoadFMODBank(args.Arg(1));
}

ConCommand loadBank("fmod_loadbank", CC_LoadFMODBank, "FMOD: Load a bank");

//-----------------------------------------------------------------------------
// Purpose: Provide a UserMessage handler to load a FMOD Bank
// Input: The name of the FMOD Bank to load
//-----------------------------------------------------------------------------
void MsgFunc_LoadFMODBank(bf_read &msg) {
    char szString[256];
    msg.ReadString(szString, sizeof(szString));
    g_AdaptiveMusicPlugin.LoadFMODBank(szString);
}

//-----------------------------------------------------------------------------
// Purpose: Start an FMOD Event
// Input: The name of the FMOD Event to start
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicPlugin::StartFMODEvent(const char *eventPath) {
    if (loadedFmodStudioEventPath != nullptr && (Q_strcmp(eventPath, loadedFmodStudioEventPath) == 0)) {
        // Event is already loaded
        Log("FMOD Client - Event requested for loading but already loaded (%s)\n", eventPath);
    } else {
        // Event is new
        if (loadedFmodStudioEventPath != nullptr && (Q_strcmp(loadedFmodStudioEventPath, "") != 0)) {
            // Stop the currently playing event
            StopFMODEvent(loadedFmodStudioEventPath);
        }
        const char *fullEventPath = Concatenate("event:/", eventPath);
        FMOD_RESULT result;
        result = fmodStudioSystem->getEvent(fullEventPath, &loadedFmodStudioEventDescription);
        result = loadedFmodStudioEventDescription->createInstance(&createdFmodStudioEventInstance);
        result = createdFmodStudioEventInstance->start();
        fmodStudioSystem->update();
        if (result != FMOD_OK) {
            Warning("FMOD Client - Could not start Event (%s). Error: (%d) %s\n", eventPath, result,
                    FMOD_ErrorString(result));
            return (-1);
        }
        Log("FMOD Client - Event successfully started (%s)\n", eventPath);
        delete[] loadedFmodStudioEventPath;
        loadedFmodStudioEventPath = new char[strlen(eventPath) + 1];
        strcpy(loadedFmodStudioEventPath, eventPath);
    }
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to start an FMOD Event
// Input: The name of the FMOD Event to start as ConCommand argument
//-----------------------------------------------------------------------------
void CC_StartFMODEvent(const CCommand &args) {
    if (args.ArgC() < 1 || Q_strcmp(args.Arg(1), "") == 0) {
        Msg("Usage: fmod_startevent <eventpath>\n");
        return;
    }
    g_AdaptiveMusicPlugin.StartFMODEvent(args.Arg(1));
}

ConCommand startEvent("fmod_startevent", CC_StartFMODEvent, "FMOD: Start an event");

//-----------------------------------------------------------------------------
// Purpose: Provide a UserMessage handler to start an FMOD Event
// Input: The name of the FMOD Event to start
//-----------------------------------------------------------------------------
void MsgFunc_StartFMODEvent(bf_read &msg) {
    char szString[256];
    msg.ReadString(szString, sizeof(szString));
    g_AdaptiveMusicPlugin.StartFMODEvent(szString);
}

//-----------------------------------------------------------------------------
// Purpose: Stop an FMOD Event
// Input: The name of the FMOD Event to stop
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicPlugin::StopFMODEvent(const char *eventPath) {
    const char *fullEventPath = Concatenate("event:/", eventPath);
    FMOD_RESULT result;
    result = fmodStudioSystem->getEvent(fullEventPath, &loadedFmodStudioEventDescription);
    result = loadedFmodStudioEventDescription->releaseAllInstances();
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        Warning("FMOD Client - Could not stop Event (%s). Error: (%d) %s\n", eventPath, result,
                FMOD_ErrorString(result));
        return (-1);
    }
    Log("FMOD Client - Event successfully stopped (%s)\n", eventPath);
    delete[] loadedFmodStudioEventPath;
    loadedFmodStudioEventPath = new char[strlen("") + 1];
    strcpy(loadedFmodStudioEventPath, "");
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to stop an FMOD Event
// Input: The name of the FMOD Event to load as ConCommand argument
//-----------------------------------------------------------------------------
void CC_StopFMODEvent(const CCommand &args) {
    if (args.ArgC() < 1 || Q_strcmp(args.Arg(1), "") == 0) {
        Msg("Usage: fmod_stopevent <eventpath>\n");
        return;
    }
    g_AdaptiveMusicPlugin.StopFMODEvent(args.Arg(1));
}

ConCommand stopEvent("fmod_stopevent", CC_StopFMODEvent, "FMOD: Stop an event");

//-----------------------------------------------------------------------------
// Purpose: Provide a UserMessage handler to stop an FMOD Event
// Input: The name of the FMOD Event to stop
//-----------------------------------------------------------------------------
void MsgFunc_StopFMODEvent(bf_read &msg) {
    char szString[256];
    msg.ReadString(szString, sizeof(szString));
    g_AdaptiveMusicPlugin.StopFMODEvent(szString);
}

//-----------------------------------------------------------------------------
// Purpose: Set the value for a global FMOD Parameter
// Input:
// - parameterName: The name of the FMOD Parameter to set
// - value: The value to set the FMOD Parameter to
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicPlugin::SetFMODGlobalParameter(const char *parameterName, float value) {
    ;
    FMOD_RESULT result;
    result = fmodStudioSystem->setParameterByName(parameterName, value);
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        Warning("FMOD Client - Could not set Global Parameter value (%s) (%f). Error: (%d) %s\n", parameterName, value,
                result, FMOD_ErrorString(result));
        return (-1);
    }
    Log("FMOD Client - Global Parameter %s set to %f\n", parameterName, value);
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to set the value for a global FMOD Parameter
// Input:
// - Arg(1): The name of the FMOD Parameter to set (to load as ConCommand argument)
// - Arg(2): The value to set the FMOD Parameter to (to load as ConCommand argument)
//-----------------------------------------------------------------------------
void CC_SetFMODGlobalParameter(const CCommand &args) {
    if (args.ArgC() < 2 || Q_strcmp(args.Arg(1), "") == 0 || Q_strcmp(args.Arg(2), "") == 0) {
        Msg("Usage: fmod_setglobalparameter <parametername> <value>\n");
        return;
    }
    g_AdaptiveMusicPlugin.SetFMODGlobalParameter(args.Arg(1), atof(args.Arg(2)));
}

ConCommand setGlobalParameter("fmod_setglobalparameter", CC_SetFMODGlobalParameter, "FMOD: Set a global parameter");

//-----------------------------------------------------------------------------
// Purpose: Provide a UserMessage handler to set the value for a global FMOD Parameter
// Input:
// - The name of the FMOD Parameter to set (to load as ConCommand argument)
// - The value to set the FMOD Parameter to (to load as ConCommand argument)
//-----------------------------------------------------------------------------
void MsgFunc_SetFMODGlobalParameter(bf_read &msg) {
    char szString[256];
    msg.ReadString(szString, sizeof(szString));
    float szFloat;
    szFloat = msg.ReadFloat();
    g_AdaptiveMusicPlugin.SetFMODGlobalParameter(szString, szFloat);
}

//-----------------------------------------------------------------------------
// Purpose: Start the FMOD Studio System and initialize it
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicPlugin::StartFMODEngine() {
    // Startup FMOD Studio System
    Msg("FMOD Client - Starting Engine\n");
    FMOD_RESULT result;
    result = FMOD::Studio::System::create(&fmodStudioSystem);
    if (result != FMOD_OK) {
        Error("FMOD Client - Engine error! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    result = fmodStudioSystem->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK) {
        Error("FMOD Client - Engine error! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    Log("FMOD Client - Engine successfully started\n");

    // Handle the usermessages (registered in hl2_usermessages.cpp)
    Log("FMOD Client - Hooking up the UserMessages\n");
    //usermessages->HookMessage("FMODLoadFMODBank", MsgFunc_LoadFMODBank);
    //usermessages->HookMessage("FMODStartFMODEvent", MsgFunc_StartFMODEvent);
    //usermessages->HookMessage("FMODStopFMODEvent", MsgFunc_StopFMODEvent);
    //usermessages->HookMessage("FMODSetFMODGlobalParameter", MsgFunc_SetFMODGlobalParameter);
    Log("FMOD Client - Successfully hooked up the UserMessages\n");

    // Start the infinitely-looping Client-Side watcher
    //StartClientSideWatcher();
    // TODO: Replace this with Pause/Unpause interface methods

    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Stop the FMOD Studio System
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicPlugin::StopFMODEngine() {
    Msg("FMOD Client - Stopping Engine\n");
    FMOD_RESULT result;
    result = fmodStudioSystem->release();
    if (result != FMOD_OK) {
        Error("FMOD Client - Engine error! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    Log("FMOD Client - Engine successfully stopped\n");

    // Stop the infinitely-looping Client-Side watcher
    // StopClientSideWatcher();
    // TODO: Replace this with Pause/Unpause interface methods

    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: TODO
// Output: TODO
//-----------------------------------------------------------------------------
int CAdaptiveMusicPlugin::SetFMODPausedState(bool pausedState) {
    Log("FMOD Client - Setting master bus paused state to %d\n", pausedState);
    FMOD::Studio::Bus *bus;
    FMOD_RESULT result;
    result = fmodStudioSystem->getBus("bus:/", &bus);
    if (result != FMOD_OK) {
        Msg("FMOD Client - Could not find the master bus! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    result = bus->setPaused(pausedState);
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        Msg("FMOD Client - Could not pause the master bus! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }

    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Sanitize the name of an FMOD Bank, adding ".bank" if it's not already present
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

//-----------------------------------------------------------------------------
// Purpose: Get the path of a Bank file in the /sound/fmod/banks folder
// Input: The FMOD Bank name to locate
// Output: The FMOD Bank's full path from the file system
//-----------------------------------------------------------------------------
const char *CAdaptiveMusicPlugin::GetFMODBankPath(const char *bankName) {
    const char *sanitizedBankName = SanitizeBankName(bankName);
    char *bankPath = new char[512];
    Q_snprintf(bankPath, 512, "%s/sound/fmod/banks/%s", ismm->GetBaseDir(), sanitizedBankName);
    // convert backwards slashes to forward slashes
    for (int i = 0; i < 512; i++) {
        if (bankPath[i] == '\\')
            bankPath[i] = '/';
    }
    return bankPath;
}
*/