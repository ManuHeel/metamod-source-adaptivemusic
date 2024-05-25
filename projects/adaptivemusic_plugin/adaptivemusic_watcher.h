#ifndef ADAPTIVEMUSIC_WATCHER_H
#define ADAPTIVEMUSIC_WATCHER_H
#ifdef _WIN32
#pragma once
#endif

// Forward declarations
class CAdaptiveMusicPlugin;

struct CAdaptiveMusicZone;

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

    virtual void Think();

};

//===========================================================================================================
// ZONE WATCHER
//===========================================================================================================
class CAdaptiveMusicZoneWatcher : public CAdaptiveMusicWatcher {

protected:

    std::list<CAdaptiveMusicZone *> *zones;

public:

    void SetZones(std::list<CAdaptiveMusicZone *> *zonesRef);

    void Init();

    void Think() override;

private:

};

#endif