#include "adaptivemusic_plugin.h"
#include "adaptivemusic_watcher.h"
#include "mm_utils.h"
#include "itoolentity.h"

//===========================================================================================================
// BASE WATCHER
//===========================================================================================================

// TODO remove this method, the plugin is accessible globally with g_AdaptiveMusicPlugin
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
    //META_CONPRINTF("AdaptiveMusic Plugin - Base Watcher - Thinking");
}

//===========================================================================================================
// HEALTH WATCHER
//===========================================================================================================

/**
 * Get the player's health.
 *
 * @param edict		Player's edict.
 */
int CAdaptiveMusicHealthWatcher::GetPlayerHealth(edict_t *edict) {
    if (pAdaptiveMusicPlugin->playerinfomanager) {
        IPlayerInfo *playerinfo = pAdaptiveMusicPlugin->playerinfomanager->GetPlayerInfo(edict);
        if (playerinfo != NULL && playerinfo->GetHealth() != NULL) {
            return playerinfo->GetHealth();
        }
    }
    return -1;
}

void CAdaptiveMusicHealthWatcher::Init() {
    lastKnownHealth = 0.0f;
    META_CONPRINTF("AdaptiveMusic Plugin - Health Watcher - Created\n");
}

void CAdaptiveMusicHealthWatcher::Think() {
    //META_CONPRINTF("AdaptiveMusic Plugin - Health Watcher - Thinking\n");
    if (pAdaptiveMusicPlugin->pAdaptiveMusicPlayer != nullptr) {
        auto playerHealth = (float) GetPlayerHealth(pAdaptiveMusicPlugin->pAdaptiveMusicPlayer);
        if (playerHealth != lastKnownHealth) {
            lastKnownHealth = playerHealth;
            g_AdaptiveMusicPlugin.SetFMODGlobalParameter(parameterName, playerHealth);
        }
    }
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