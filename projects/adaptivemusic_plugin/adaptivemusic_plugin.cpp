/**
 * ======================================================
 * Metamod:Source AdaptiveMusic Plugin
 * Written by Manuel Russello
 * ======================================================
 */

#include <stdio.h>
#include "adaptivemusic_plugin.h"

SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);

AdaptiveMusicPlugin g_AdaptiveMusicPlugin;
IServerGameDLL *server = NULL;

PLUGIN_EXPOSE(AdaptiveMusicPlugin, g_AdaptiveMusicPlugin);
bool AdaptiveMusicPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	/* Make sure we build on MM:S 1.4 */
#if defined METAMOD_PLAPI_VERSION
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
#else
	GET_V_IFACE_ANY(serverFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
#endif

	SH_ADD_HOOK_STATICFUNC(IServerGameDLL, ServerActivate, server, Hook_ServerActivate, true);

	return true;
}

bool AdaptiveMusicPlugin::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK_STATICFUNC(IServerGameDLL, ServerActivate, server, Hook_ServerActivate, true);

	return true;
}

void Hook_ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	META_LOG(g_PLAPI, "ServerActivate() called: edictCount = %d, clientMax = %d", edictCount, clientMax);
}

void AdaptiveMusicPlugin::AllPluginsLoaded()
{
	/* This is where we'd do stuff that relies on the mod or other plugins 
	 * being initialized (for example, cvars added and events registered).
	 */
}

bool AdaptiveMusicPlugin::Pause(char *error, size_t maxlen)
{
	return true;
}

bool AdaptiveMusicPlugin::Unpause(char *error, size_t maxlen)
{
	return true;
}

const char *AdaptiveMusicPlugin::GetLicense()
{
	return "idk yet";
}

const char *AdaptiveMusicPlugin::GetVersion()
{
	return "1.0.0.0";
}

const char *AdaptiveMusicPlugin::GetDate()
{
	return __DATE__;
}

const char *AdaptiveMusicPlugin::GetLogTag()
{
	return "ADAPTIVEMUSIC";
}

const char *AdaptiveMusicPlugin::GetAuthor()
{
	return "Manuel Russello";
}

const char *AdaptiveMusicPlugin::GetDescription()
{
	return "AdaptiveMusic Plugin implementing FMOD";
}

const char *AdaptiveMusicPlugin::GetName()
{
	return "AdaptiveMusic Plugin";
}

const char *AdaptiveMusicPlugin::GetURL()
{
	return "https://hl2musicmod.russello.studio/";
}
