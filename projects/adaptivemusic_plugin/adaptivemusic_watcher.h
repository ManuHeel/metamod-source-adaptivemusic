#ifndef ADAPTIVEMUSIC_WATCHER_H
#define ADAPTIVEMUSIC_WATCHER_H
#ifdef _WIN32
#pragma once
#endif

class CAdaptiveMusicPlugin; // Forward declaration

//===========================================================================================================
// BASE WATCHER
//===========================================================================================================

class CAdaptiveMusicWatcher {

protected:

    CAdaptiveMusicPlugin *pAdaptiveMusicPlugin;
    const char *parameterName;

public:

    void SetAdaptiveMusicPlugin(CAdaptiveMusicPlugin *pAdaptiveMusicPluginRef);

    void SetParameterName(const char *pName);

    void Init();

    void Think();

};

#endif