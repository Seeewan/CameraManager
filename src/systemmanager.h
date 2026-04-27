#ifndef SYSTEMMANAGER_H
#define SYSTEMMANAGER_H

#include <QObject>
#include "spinnaker_compat.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

class SystemManager
{
public:
    SystemManager();
    ~SystemManager();

    SystemPtr getSystem(){return system;}
    CameraList getCamList();
    bool isValid() const { return valid; }

private:
    SystemPtr system;
    bool valid = false;
};

#endif // SYSTEMMANAGER_H
