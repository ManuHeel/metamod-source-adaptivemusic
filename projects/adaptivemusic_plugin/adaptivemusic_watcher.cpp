#include "adaptivemusic_plugin.h"
#include "adaptivemusic_watcher.h"


//===========================================================================================================
// BASE WATCHER
//===========================================================================================================

void CAdaptiveMusicWatcher::SetAdaptiveMusicPlugin(CAdaptiveMusicPlugin *pAdaptiveMusicPluginRef) {
    pAdaptiveMusicPlugin = pAdaptiveMusicPluginRef;
}

void CAdaptiveMusicWatcher::SetParameterName(const char *pName) {
    parameterName = pName;
}

void CAdaptiveMusicWatcher::Init() {
    META_CONPRINTF("AdaptiveMusic Plugin - Base Watcher - Created\n");
}

void CAdaptiveMusicWatcher::Think() {
    //META_CONPRINTF("AdaptiveMusic Plugin - Base Watcher - Thinking at time %f\n", gpGlobals->curtime);
}

//===========================================================================================================
// HEALTH WATCHER
//===========================================================================================================

void CAdaptiveMusicHealthWatcher::Init() {
    lastKnownHealth = 0.0f;
    META_CONPRINTF("AdaptiveMusic Plugin - Health Watcher - Created\n");
}

void CAdaptiveMusicHealthWatcher::Think() {
    META_CONPRINTF("AdaptiveMusic Plugin - Health Watcher - Thinking\n");
    CBaseEntity *pPlayer = NULL;
    pPlayer = g_AdaptiveMusicPlugin.serverTools->FindEntityByClassname(pPlayer, "player");
    //CBaseEntity *entity = engine->PEntityOfEntIndex(1)->GetUnknown()->GetBaseEntity();
    //META_CONPRINTF("Health = %f\n",entity->GetHealth());
    //META_CONPRINTF("Health = %f",player->GetHealth());
    /*
    if (g_AdaptiveMusicPlugin.pAdaptiveMusicPlayer != nullptr) {
        float playerHealth = g_AdaptiveMusicPlugin.pAdaptiveMusicPlayer->GetUnknown()->GetBaseEntity()->GetHealth();
        META_CONPRINTF("Health = %f",playerHealth);
    }
     */
    /*
    if (pAdaptiveMusicPlayer != nullptr) {
        auto playerHealth = (float) pAdaptiveMusicPlayer->GetHealth();
        if (playerHealth != lastKnownHealth) {
            lastKnownHealth = playerHealth;
            // Send a FMODSetGlobalParameter usermessage
            CSingleUserRecipientFilter filter(pAdaptiveMusicPlayer);
            filter.MakeReliable();
            UserMessageBegin(filter, "FMODSetGlobalParameter");
            WRITE_STRING(parameterName);
            WRITE_FLOAT(playerHealth);
            MessageEnd();
        }
    }
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
     */
}

//===========================================================================================================
// ZONE WATCHER
//===========================================================================================================

bool IsVectorWithinZone(Vector vector, CAdaptiveMusicZone *zone) {
    if (
            (vector.x >= zone->minOrigin[0] && vector.x <= zone->maxOrigin[0]) &&
            (vector.y >= zone->minOrigin[1] && vector.y <= zone->maxOrigin[1]) &&
            (vector.z >= zone->minOrigin[2] && vector.z <= zone->maxOrigin[2])) {
        return true;
    }
    return false;
}

void CAdaptiveMusicZoneWatcher::SetZones(std::list<CAdaptiveMusicZone *> *zonesRef) {
    zones = zonesRef;
}

void CAdaptiveMusicZoneWatcher::Init() {
    META_CONPRINTF("AdaptiveMusic Plugin - Zone Watcher - Created\n");
}

void CAdaptiveMusicZoneWatcher::Think() {
    if (g_AdaptiveMusicPlugin.pAdaptiveMusicPlayer != nullptr) {
        auto currentPosition = g_AdaptiveMusicPlugin.pAdaptiveMusicPlayer->GetCollideable()->GetCollisionOrigin();
        for (auto &zone: *zones) {
            bool zoneStatus = IsVectorWithinZone(currentPosition, zone);
            if (zoneStatus != zone->lastKnownZoneStatus) {
                zone->lastKnownZoneStatus = zoneStatus;
                g_AdaptiveMusicPlugin.SetFMODGlobalParameter(zone->parameterName, zoneStatus);
            }
        }
    }
}