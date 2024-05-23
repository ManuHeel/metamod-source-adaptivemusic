#ifndef ADAPTIVEMUSIC_WATCHER_H
#define ADAPTIVEMUSIC_WATCHER_H
#ifdef _WIN32
#pragma once
#endif

#include <list>
#include <string>

//===========================================================================================================
// BASE WATCHER
//===========================================================================================================

class CAdaptiveMusicPlugin; // Forward declaration

class CAdaptiveMusicWatcher {

protected:

    CAdaptiveMusicPlugin *pAdaptiveMusicPlugin{};

    const char *parameterName;

public:

    void SetAdaptiveMusicClientPlugin(CAdaptiveMusicPlugin *pAdaptiveMusicPluginRef);

    void SetParameterName(const char *pName);

    void Init();

    void Think();

};

#endif