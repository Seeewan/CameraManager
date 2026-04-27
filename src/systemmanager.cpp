#include "systemmanager.h"
#include <QDebug>

SystemManager::SystemManager(){
    try {
        system = System::GetInstance();
        valid = true;
    } catch (const Spinnaker::Exception& e) {
        qWarning() << "Spinnaker init failed:" << e.what();
        system = nullptr;
        valid = false;
    }
}

SystemManager::~SystemManager(){
    if (!valid || !system) {
        return;
    }
    try {
        system->ReleaseInstance();
    } catch (const Spinnaker::Exception& e) {
        qWarning() << "Spinnaker shutdown failed:" << e.what();
    }
}

CameraList SystemManager::getCamList() {
    if (!valid || !system) {
        return CameraList();
    }

    try {
        system->UpdateCameras(true);
        return system->GetCameras(true, true);
    } catch (const Spinnaker::Exception& e) {
        qWarning() << "Spinnaker camera refresh failed:" << e.what();
        return CameraList();
    }
}
