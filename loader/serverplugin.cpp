#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>
#include "loader.h"
#include <sh_memfuncinfo.h>
#include <sh_memory.h>
#include "serverplugin.h"

typedef enum
{
	PLUGIN_CONTINUE = 0,
	PLUGIN_OVERRIDE,
	PLUGIN_STOP,
} PLUGIN_RESULT;

typedef enum
{
    eQueryCvarValueStatus_ValueIntact=0,
    eQueryCvarValueStatus_CvarNotFound=1,
    eQueryCvarValueStatus_NotACvar=2,
    eQueryCvarValueStatus_CvarProtected=3
} EQueryCvarValueStatus;

typedef int QueryCvarCookie_t;
class CCommand;
class IServerPluginCallbacks;
struct edict_t;

#if defined WIN32
#define LIBRARY_EXT		".dll"
#else
#define LIBRARY_EXT		"_i486.so"
#endif

class IRandomThings
{
public:
	virtual PLUGIN_RESULT ClientCommand(edict_t *pEntity, const CCommand& args)
	{
		return PLUGIN_CONTINUE;
	}
};

/**
 * The vtable must match the general layout for ISPC.  We modify the vtable
 * based on what we get back.
 */
class ServerPlugin
{
	IVspBridge* bridge;
	unsigned int vsp_version;
	bool load_allowed;
public:
	ServerPlugin()
	{
		load_allowed = false;
	}
	virtual bool Load(QueryValveInterface engineFactory, QueryValveInterface gsFactory)
	{
		MetamodBackend backend = MMBackend_UNKNOWN;

		if (!load_allowed)
			return false;

		load_allowed = false;

		/* Check for L4D */
		if (engineFactory("VEngineServer022", NULL) != NULL &&
			engineFactory("VEngineCvar007", NULL) != NULL)
		{
			backend = MMBackend_Left4Dead;
		}
		else if (engineFactory("VEngineServer021", NULL) != NULL)
		{
			/* Check for OB */
			if (engineFactory("VEngineCvar004", NULL) != NULL &&
				engineFactory("VModelInfoServer002", NULL) != NULL)
			{
				backend = MMBackend_Episode2;
			}
			/* Check for EP1 */
			else if (engineFactory("VModelInfoServer001", NULL) != NULL &&
					 (engineFactory("VEngineCvar003", NULL) != NULL ||
					  engineFactory("VEngineCvar002", NULL) != NULL))
			{
				backend = MMBackend_Episode1;
			}
		}

		if (backend == MMBackend_UNKNOWN)
		{
			mm_LogFatal("Could not detect engine version, spewing stats:");
			return false;
		}
		else if (backend >= MMBackend_Episode2)
		{
			/* We need to insert the right type of call into this vtable */
			void **vtable_src;
			void **vtable_dest;
			IRandomThings sample;
			SourceHook::MemFuncInfo mfp_dest, mfp_src;

			mfp_dest.isVirtual = false;
			mfp_src.isVirtual = false;

			SourceHook::GetFuncInfo(&ServerPlugin::ClientCommand, mfp_dest);
			SourceHook::GetFuncInfo(&IRandomThings::ClientCommand, mfp_src);

			assert(mfp_dest.isVirtual);
			assert(mfp_dest.thisptroffs == 0);
			assert(mfp_dest.vtbloffs == 0);
			assert(mfp_src.isVirtual);
			assert(mfp_src.thisptroffs == 0);
			assert(mfp_src.vtbloffs == 0);

			vtable_src = (void **)*(void **)&sample;
			vtable_dest = (void **)*(void **)this;
			SourceHook::SetMemAccess(&vtable_dest[mfp_dest.vtblindex],
									 sizeof(void*),
									 SH_MEM_READ|SH_MEM_WRITE|SH_MEM_EXEC);
			vtable_dest[mfp_dest.vtblindex] = vtable_src[mfp_src.vtblindex];
		}

		char error[255];
		if (!mm_LoadMetamodLibrary(backend, error, sizeof(error)))
		{
			mm_LogFatal("Detected engine %d but could not load: %s", backend, error);
			return false;
		}

		typedef IVspBridge *(*GetVspBridge)();
		GetVspBridge get_bridge = (GetVspBridge)mm_GetProcAddress("GetVspBridge");
		if (get_bridge == NULL)
		{
			mm_UnloadMetamodLibrary();
			mm_LogFatal("Detected engine %d but could not find GetVspBridge callback.", backend);
			return false;
		}

		bridge = get_bridge();

		vsp_bridge_info info;

		info.engineFactory = engineFactory;
		info.gsFactory = gsFactory;
		info.vsp_callbacks = (IServerPluginCallbacks*)this;
		info.vsp_version = vsp_version;

		strcpy(error, "Unknown error");
		if (!bridge->Load(&info, error, sizeof(error)))
		{
			mm_UnloadMetamodLibrary();
			mm_LogFatal("Unknown error loading Metamod for engine %d: %s", backend, error);	
			return false;
		}

		return true;
	}
	virtual void Unload()
	{
		if (bridge == NULL)
			return;
		bridge->Unload();
	}
	virtual void Pause()
	{
	}
	virtual void UnPause()
	{
	}
	virtual const char *GetPluginDescription()
	{
		if (bridge == NULL)
			return "Metamod:Source Loader Shim";
		return bridge->GetDescription();
	}
	virtual void LevelInit(char const *pMapName)
	{
	}
	virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
	{
	}
	virtual void GameFrame(bool simulating)
	{
	}
	virtual void LevelShutdown()
	{
	}
	virtual void ClientActive(edict_t *pEntity)
	{
	}
	virtual void ClientDisconnect(edict_t *pEntity)
	{
	}
	virtual void ClientPutInServer(edict_t *pEntity, char const *playername)
	{
	}
	virtual void SetCommandClient(int index)
	{
	}
	virtual void ClientSettingsChanged(edict_t *pEdict)
	{
	}
	virtual PLUGIN_RESULT ClientConnect(bool *bAllowConnect,
										edict_t *pEntity,
										const char *pszName,
										const char *pszAddress,
										char *reject,
										int maxrejectlen) 
	{
		return PLUGIN_CONTINUE;
	}
	virtual PLUGIN_RESULT ClientCommand(edict_t *pEntity)
	{
		return PLUGIN_CONTINUE;
	}
	virtual PLUGIN_RESULT NetworkIDValidated(const char *pszUserName, const char *pszNetworkID)
	{
		return PLUGIN_CONTINUE;
	}
	virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie,
										  edict_t *pPlayerEntity,
										  EQueryCvarValueStatus eStatus,
										  const char *pCvarName,
										  const char *pCvarValue)
	{
	}
	void PrepForLoad(unsigned int version)
	{
		vsp_version = version;
		load_allowed = true;
	}
	bool IsLoaded()
	{
		return bridge != NULL;
	}
};

ServerPlugin mm_vsp_callbacks;

void *mm_GetVspCallbacks(unsigned int version)
{
	if (mm_vsp_callbacks.IsLoaded())
		return NULL;

	/* Only support versions 1 or 2 right now */
	if (version > 2)
		return NULL;

	mm_vsp_callbacks.PrepForLoad(version);

	return &mm_vsp_callbacks;
}
