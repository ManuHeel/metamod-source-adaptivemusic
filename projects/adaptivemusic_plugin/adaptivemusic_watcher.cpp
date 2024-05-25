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
    META_CONPRINTF("AdaptiveMusic Plugin - Base Watcher - Thinking\n");
}