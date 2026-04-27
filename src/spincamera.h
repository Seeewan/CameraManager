#ifndef SPINCAMERA_H
#define SPINCAMERA_H

#include "abstractcamera.h"
#include <iostream>
#include <sstream>
#include "spincameraproperty.h"

#include "spinnaker_compat.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

class SpinCamera : public AbstractCamera{
public:
    SpinCamera(Spinnaker::CameraPtr pCam);
    ~SpinCamera();

    //Constant to get the trigger source
    //wrote on 12/06/2019 by French students
    static int trigger;

    //Constant to enable or disable trigger
    //20/05/2024 written  by Armand & Nathan
    static int enableTrigger;

    inline Spinnaker::CameraPtr getCamera() {
        return cam;
    }
    void setSpinProperty(CameraManagerSpin::SpinCameraProperty* p);
    bool refreshSpinPropertyMetadata(CameraManagerSpin::SpinCameraProperty* p) override;
    void setProperty(CameraManager::CameraProperty* p);
    void startAutoCapture();
    void stopAutoCapture();
    int ConfigureTrigger(INodeMap & nodeMap);
    unsigned char* retrieveImage(unsigned int* bufferSize, unsigned int* imageWidth, unsigned int* imageHeight, bool colored = false);

    bool equalsTo(AbstractCamera *c);
    std::string getString();
    Spinnaker::ImagePtr getImage(bool colored = false);

private:
    Spinnaker::CameraPtr cam;
    std::string deviceId = "unknown-device";
    std::vector<Spinnaker::ImagePtr> captureImage();

};

#endif // SPINCAMERA_H
