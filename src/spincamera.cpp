#include "spincamera.h"
#include <algorithm>
#include <QDebug>
#include <QPainter>
#include <QRgb>
#include <iostream>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

namespace {
bool setEnumNodeValue(INodeMap& nodeMap, const char* nodeName, const char* entryName) {
    CEnumerationPtr node = nodeMap.GetNode(nodeName);
    if (!IsAvailable(node) || !IsWritable(node)) {
        return false;
    }

    CEnumEntryPtr entry = node->GetEntryByName(entryName);
    if (!IsAvailable(entry) || !IsReadable(entry)) {
        return false;
    }

    node->SetIntValue(entry->GetValue());
    return true;
}

bool setBooleanNodeValue(INodeMap& nodeMap, const char* nodeName, bool value) {
    CBooleanPtr node = nodeMap.GetNode(nodeName);
    if (!node || !IsAvailable(node) || !IsWritable(node)) {
        return false;
    }
    node->SetValue(value);
    return true;
}

bool setFloatNodeValue(INodeMap& nodeMap, const char* nodeName, double value) {
    CFloatPtr node = nodeMap.GetNode(nodeName);
    if (!node || !IsAvailable(node) || !IsWritable(node)) {
        return false;
    }

    const double clampedValue = std::clamp(value, node->GetMin(), node->GetMax());
    node->SetValue(clampedValue);
    return true;
}

bool readFloatNodeValue(INodeMap& nodeMap, const char* nodeName, double* value) {
    CFloatPtr node = nodeMap.GetNode(nodeName);
    if (!node || !IsAvailable(node) || !IsReadable(node)) {
        return false;
    }
    *value = node->GetValue();
    return true;
}

bool readBooleanNodeValue(INodeMap& nodeMap, const char* nodeName, bool* value) {
    CBooleanPtr node = nodeMap.GetNode(nodeName);
    if (!node || !IsAvailable(node) || !IsReadable(node)) {
        return false;
    }
    *value = node->GetValue();
    return true;
}

bool readEnumNodeValue(INodeMap& nodeMap, const char* nodeName, gcstring* value) {
    CEnumerationPtr node = nodeMap.GetNode(nodeName);
    if (!node || !IsAvailable(node) || !IsReadable(node)) {
        return false;
    }
    *value = node->ToString();
    return true;
}

bool refreshFloatNodeMetadata(INodeMap& nodeMap, const char* nodeName, CameraManagerSpin::SpinCameraProperty* property) {
    CFloatPtr node = nodeMap.GetNode(nodeName);
    if (!node || !IsAvailable(node) || !IsReadable(node)) {
        return false;
    }

    const double nodeMin = node->GetMin();
    const double nodeMax = node->GetMax();
    const double preferredMin = property->getMin();
    const double preferredMax = property->getMax();

    double effectiveMin = (std::max)(nodeMin, preferredMin);
    double effectiveMax = (std::min)(nodeMax, preferredMax);

    if (effectiveMin > effectiveMax) {
        effectiveMin = nodeMin;
        effectiveMax = nodeMax;
    }

    property->setRange(effectiveMin, effectiveMax);
    property->setValue(std::clamp(node->GetValue(), effectiveMin, effectiveMax));
    return true;
}

bool hasWritableEnumNode(INodeMap& nodeMap, const char* nodeName) {
    CEnumerationPtr node = nodeMap.GetNode(nodeName);
    return node && IsAvailable(node) && IsWritable(node);
}

bool hasWritableBooleanNode(INodeMap& nodeMap, const char* nodeName) {
    CBooleanPtr node = nodeMap.GetNode(nodeName);
    return node && IsAvailable(node) && IsWritable(node);
}
}

SpinCamera::SpinCamera(Spinnaker::CameraPtr pCam){
    cam = pCam;
    try {
        std::ostringstream ss;
        ss << cam->GetDeviceID();
        deviceId = ss.str();
    } catch (const Spinnaker::Exception&) {
        std::ostringstream ss;
        ss << "camera-" << reinterpret_cast<quintptr>(cam.operator->());
        deviceId = ss.str();
    }

    try {
        GenApi::CStringPtr ptrStringSerial = cam->GetTLDeviceNodeMap().GetNode("DeviceSerialNumber");
        if (ptrStringSerial && IsReadable(ptrStringSerial)) {
            setSerialValue(QString(ptrStringSerial->GetValue()));
        } else {
            setSerialValue("unknown");
        }
    } catch (const Spinnaker::Exception&) {
        setSerialValue("unknown");
    }

    try {
        GenApi::CStringPtr ptrStringModel = cam->GetTLDeviceNodeMap().GetNode("DeviceModelName");
        if (ptrStringModel && IsReadable(ptrStringModel)) {
            setModelValue(QString(ptrStringModel->GetValue()));
        } else {
            setModelValue("unknown");
        }
    } catch (const Spinnaker::Exception&) {
        setModelValue("unknown");
    }
}
SpinCamera::~SpinCamera() {
    if(capturing) stopAutoCapture();
}

void SpinCamera::setProperty(CameraManager::CameraProperty *p) {

}

void SpinCamera::setSpinProperty(CameraManagerSpin::SpinCameraProperty* p) {
    try {
        if (!cam->IsInitialized()) {
            cam->Init();
        }
    } catch (const Spinnaker::Exception& e) {
        std::cout << "Error : " << e.what() << std::endl;
        return;
    }

    INodeMap& nodeMap = cam->GetNodeMap();

    switch(p->getType()) { //Get the type of the property

    case CameraManagerSpin::TRIGGER :

        // Armand & Nathan 20/05/2024 : enable or disable trigger
        try {
            if(p->getAuto()){
                enableTrigger = 0;
                if (!setEnumNodeValue(nodeMap, "TriggerMode", "On")) {
                    std::cout << "unable to acces the trigger mode property" << std::endl;
                }
            }else{
                enableTrigger = 1;
                if (!setEnumNodeValue(nodeMap, "TriggerMode", "Off")) {
                    std::cout << "unable to acces the trigger mode property" << std::endl;
                }
            }
        } catch (Spinnaker::Exception e) {
            std::cout << "unable to acces the trigger mode property" << std::endl;
        }
        break;
    case CameraManagerSpin::AUTOTRIGGER :

        // Armand & Nathan 20/05/2024 : change trigger source
        try {
            if(p->getAuto()){
                trigger = 0;
                if (!setEnumNodeValue(nodeMap, "TriggerSource", "Software")) {
                    std::cout << "unable to acces the trigger source property" << std::endl;
                }
            }else{
                trigger = 1;
                if (!setEnumNodeValue(nodeMap, "TriggerSource", "Line0")) {
                    std::cout << "unable to acces the trigger source property" << std::endl;
                }
            }
        } catch (Spinnaker::Exception e) {
            std::cout << "unable to acces the trigger source property" << std::endl;
            //std::cout << "Error AutoTrigger : " << e.what() << std::endl; // Error : GenICam::AccessException= Node is not writable. : AccessException thrown in node 'EventTestTimestamp' while calling 'EventTestTimestamp.SetValue()' (file 'IntegerT.h', line 77)
        }
        break;
    case CameraManagerSpin::BLACKLEVEL :
        try{
        setEnumNodeValue(nodeMap, "BlackLevelSelector", "All");
        if (!setFloatNodeValue(nodeMap, "BlackLevel", p->getValue())) {
            std::cout << "Unable to access the black level property."<< std::endl;
            return;
        }
    } catch (Spinnaker::Exception &e){
            std::cout << "Error : " << e.what() << std::endl;
        }
        break;
    case CameraManagerSpin::EXPOSURETIME :
        try{
        setEnumNodeValue(nodeMap, "ExposureMode", "Timed");
        if (p->getAuto()) {
            if (!setEnumNodeValue(nodeMap, "ExposureAuto", "Continuous")) {
                std::cout << "Unable to access the exposure auto property." << std::endl;
            }
        } else {
            setBooleanNodeValue(nodeMap, "AcquisitionFrameRateEnable", false);
            if (!setEnumNodeValue(nodeMap, "ExposureAuto", "Off")) {
                std::cout << "Unable to access the exposure auto property." << std::endl;
            } else if (!setFloatNodeValue(nodeMap, "ExposureTime", p->getValue())) {
                std::cout << "Unable to access the exposure property." << std::endl;
            }
        }
    }catch (Spinnaker::Exception &e){
            std::cout << "Error : " << e.what() << std::endl;
        }
        break;
    case CameraManagerSpin::GAIN :
        try{
        if (p->getAuto()) {
            if (!setEnumNodeValue(nodeMap, "GainAuto", "Continuous")) {
                std::cout << "Unable to access the gain auto property." << std::endl;
            }
        } else {
            if (!setEnumNodeValue(nodeMap, "GainAuto", "Off")) {
                std::cout << "Unable to access the gain auto property." << std::endl;
            } else if (!setFloatNodeValue(nodeMap, "Gain", p->getValue())) {
                std::cout << "Unable to access the gain property." << std::endl;
            }
        }
    }catch (Spinnaker::Exception &e){
            std::cout << "Error : " << e.what() << std::endl;
        }
        break;

    case CameraManagerSpin::GAMMA :
        try{
        if (!setBooleanNodeValue(nodeMap, "GammaEnable", true)) {
            std::cout << "Unable to enable gamma property." << std::endl;
        } else {
            if (!setFloatNodeValue(nodeMap, "Gamma", p->getValue())) {
                std::cout << "Unable to access the gamma property." << std::endl;
            }
        }
    }catch (Spinnaker::Exception &e){
            std::cout << "Error : " << e.what() << std::endl;
        }
        break;

    case CameraManagerSpin::FRAMERATE :
        try{
        if (!setBooleanNodeValue(nodeMap, "AcquisitionFrameRateEnable", !p->getAuto())) {
            std::cout << "Unable to access the frame rate property." << std::endl << std::endl;
        } else if(!p->getAuto()) {
            if (!setFloatNodeValue(nodeMap, "AcquisitionFrameRate", p->getValue())) {
                std::cout << "Unable to access the frame rate property." << std::endl << std::endl;
            }
        }
    }catch (Spinnaker::Exception &e){
            std::cout << "Error : " << e.what() << std::endl;
        }
        break;
    }
}

bool SpinCamera::refreshSpinPropertyMetadata(CameraManagerSpin::SpinCameraProperty* p) {
    try {
        if (!cam->IsInitialized()) {
            cam->Init();
        }
    } catch (const Spinnaker::Exception&) {
        return false;
    }

    INodeMap& nodeMap = cam->GetNodeMap();

    switch (p->getType()) {
    case CameraManagerSpin::BLACKLEVEL: {
        const bool supported = refreshFloatNodeMetadata(nodeMap, "BlackLevel", p);
        p->setCanAuto(false);
        p->setAuto(false);
        return supported;
    }
    case CameraManagerSpin::GAIN: {
        const bool supported = refreshFloatNodeMetadata(nodeMap, "Gain", p);
        gcstring gainAutoValue;
        if (readEnumNodeValue(nodeMap, "GainAuto", &gainAutoValue)) {
            p->setAuto(gainAutoValue != "Off");
        }
        p->setCanAuto(hasWritableEnumNode(nodeMap, "GainAuto"));
        return supported;
    }
    case CameraManagerSpin::EXPOSURETIME: {
        if (hasWritableEnumNode(nodeMap, "ExposureMode")) {
            setEnumNodeValue(nodeMap, "ExposureMode", "Timed");
        }
        const bool supported = refreshFloatNodeMetadata(nodeMap, "ExposureTime", p);
        gcstring exposureAutoValue;
        if (readEnumNodeValue(nodeMap, "ExposureAuto", &exposureAutoValue)) {
            p->setAuto(exposureAutoValue != "Off");
        }
        p->setCanAuto(hasWritableEnumNode(nodeMap, "ExposureAuto"));
        return supported;
    }
    case CameraManagerSpin::GAMMA: {
        if (hasWritableBooleanNode(nodeMap, "GammaEnable")) {
            setBooleanNodeValue(nodeMap, "GammaEnable", true);
        }
        const bool supported = refreshFloatNodeMetadata(nodeMap, "Gamma", p);
        p->setCanAuto(false);
        p->setAuto(false);
        p->setDecimals(3);
        return supported;
    }
    case CameraManagerSpin::FRAMERATE: {
        const bool supported = refreshFloatNodeMetadata(nodeMap, "AcquisitionFrameRate", p);
        bool frameRateEnabled = false;
        if (readBooleanNodeValue(nodeMap, "AcquisitionFrameRateEnable", &frameRateEnabled)) {
            p->setAuto(!frameRateEnabled);
        }
        p->setCanAuto(hasWritableBooleanNode(nodeMap, "AcquisitionFrameRateEnable"));
        return supported;
    }
    case CameraManagerSpin::TRIGGER:
    {
        gcstring triggerModeValue;
        if (readEnumNodeValue(nodeMap, "TriggerMode", &triggerModeValue)) {
            p->setAuto(triggerModeValue == "On");
        }
        p->setCanAuto(false);
        return hasWritableEnumNode(nodeMap, "TriggerMode");
    }
    case CameraManagerSpin::AUTOTRIGGER:
    {
        gcstring triggerSourceValue;
        if (readEnumNodeValue(nodeMap, "TriggerSource", &triggerSourceValue)) {
            p->setAuto(triggerSourceValue == "Software");
        }
        p->setCanAuto(false);
        return hasWritableEnumNode(nodeMap, "TriggerSource");
    }
    }

    return false;
}


// Lars Aksel - 30.01.2015 - Added support to ImageDetect
std::vector<Spinnaker::ImagePtr> SpinCamera::captureImage() {

    std::vector<Spinnaker::ImagePtr> result;

    // Only execute software trigger when the camera is explicitly configured for it.
    if(enableTrigger == 0 && trigger == 0){
        try {
            CEnumerationPtr ptrTriggerMode = cam->GetNodeMap().GetNode("TriggerMode");
            CEnumerationPtr ptrTriggerSource = cam->GetNodeMap().GetNode("TriggerSource");

            bool softwareTriggerEnabled = false;
            if (IsAvailable(ptrTriggerMode) && IsReadable(ptrTriggerMode) &&
                IsAvailable(ptrTriggerSource) && IsReadable(ptrTriggerSource)) {
                const gcstring triggerModeValue = ptrTriggerMode->ToString();
                const gcstring triggerSourceValue = ptrTriggerSource->ToString();
                softwareTriggerEnabled =
                    triggerModeValue == "On" &&
                    triggerSourceValue == "Software";
            }

            if (softwareTriggerEnabled) {
                CCommandPtr ptrSoftwareTriggerCommand = cam->GetNodeMap().GetNode("TriggerSoftware");
                if (IsAvailable(ptrSoftwareTriggerCommand) && IsWritable(ptrSoftwareTriggerCommand)) {
                    ptrSoftwareTriggerCommand->Execute();
                }
            }
        } catch (Exception e) {
            qInfo() << e.what();
        }
    }
    ImagePtr image = nullptr;
    ImagePtr monoImage = nullptr;
    ImagePtr coloredImage = nullptr;
    try {


        // if(cam->TriggerSource.GetValue() == Spinnaker::TriggerSource_Line0){
        //     qInfo() << "Line0";
        // }else if(cam->TriggerSource.GetValue() == Spinnaker::TriggerSource_Software){
        //     qInfo() << "Software";
        // }

        // if(cam->TriggerMode.GetValue() == Spinnaker::TriggerMode_On){
        //     qInfo() << "On";
        // }else if(cam->TriggerMode.GetValue() == Spinnaker::TriggerMode_Off){
        //     qInfo() << "Off";
        // }


        if ( cam->IsStreaming()) {
            ImageProcessor processor;

            // 20/05/2024, Armand & Nathan : try to retrieve the next image, in case of failure it return nullptr

            try {
                image = cam->GetNextImage(1000);
                monoImage = processor.Convert(image, PixelFormat_Mono8);
                coloredImage = processor.Convert(image, PixelFormat_RGB8);
            } catch (Exception e) {
                std::cout << e.what() << std::endl;
                monoImage = nullptr;
                coloredImage = nullptr;
            }

        } else {
            std::cout <<" not streaming" << std::endl;
        }

        result.push_back(monoImage);
        result.push_back(coloredImage);

        return result;

    }catch(Spinnaker::Exception &e) {
        std::cout << "Error : " << e.what() << std::endl;
    }


}

// Hugo Fournier - 3.06.2019 - Trigger Configuration Spinnaker
int SpinCamera::ConfigureTrigger(INodeMap &nodeMap) {
    int result = 0;
    std::cout << std::endl << std::endl << "*** CONFIGURING TRIGGER ***" << std::endl << std::endl;
    try
    {
        // Ensure trigger mode off
        //
        // *** NOTES ***
        // The trigger must be disabled in order to configure whether the source
        // is software or hardware.
        //
        CEnumerationPtr ptrTriggerMode = nodeMap.GetNode("TriggerMode");
        if (!IsAvailable(ptrTriggerMode) || !IsReadable(ptrTriggerMode))
        {
            std::cout << "Unable to disable trigger mode (node retrieval). Aborting..." << std::endl;
            return -1;
        }

        CEnumEntryPtr ptrTriggerModeOff = ptrTriggerMode->GetEntryByName("Off");
        if (!IsAvailable(ptrTriggerModeOff) || !IsReadable(ptrTriggerModeOff))
        {
            std::cout << "Unable to disable trigger mode (enum entry retrieval). Aborting..." << std::endl;
            return -1;
        }

        ptrTriggerMode->SetIntValue(ptrTriggerModeOff->GetValue());

        std::cout << "Trigger mode disabled..." << std::endl;

        if(trigger==1){
            if (!setEnumNodeValue(nodeMap, "TriggerSource", "Line0")) {
                std::cout << "Unable to set trigger mode (node retrieval). Aborting..." << std::endl;
                return -1;
            }
            std::cout << "Trigger source set to hardware..." << std::endl;
        }else{
            if (!setEnumNodeValue(nodeMap, "TriggerSource", "Software")) {
                std::cout << "Unable to set trigger mode (node retrieval). Aborting..." << std::endl;
                return -1;
            }
            std::cout << "Trigger source set to software..." << std::endl;
        }

        // Turn trigger mode on
        //
        // *** LATER ***
        // Once the appropriate trigger source has been set, turn trigger mode
        // on in order to retrieve images using the trigger.

        // Armand & Nathan 10/05/2024 : added enableTrigger condition to execute this part only if the user retrieves image using trigger
        if (enableTrigger == 0) {
            CEnumEntryPtr ptrTriggerModeOn = ptrTriggerMode->GetEntryByName("On");
            if (!IsAvailable(ptrTriggerModeOn) || !IsReadable(ptrTriggerModeOn))
            {
                std::cout << "Unable to enable trigger mode (enum entry retrieval). Aborting..." << std::endl;
                return -1;
            }

            ptrTriggerMode->SetIntValue(ptrTriggerModeOn->GetValue());

            std::cout << "Trigger mode turned back on..." << std::endl << std::endl;
        }

    }
    catch (Spinnaker::Exception &e)
    {
        std::cout << "Error : " << e.what() << std::endl;
        std::cout << "erreur dans trigger" << std::endl;
        result = -1;
    }
    return result;

}

int SpinCamera::trigger = 1;
int SpinCamera::enableTrigger = 1;

//Start the AutoCapture
//wrote on 11/06/2019 by French students
void SpinCamera::startAutoCapture(){
    capturing = true;
    if (!cam->IsInitialized()) {
        cam->Init();
    }

    try{
        //cam->AcquisitionStart.Execute();
        INodeMap & nodeMap = cam->GetNodeMap();
        //set trigger to software trigger
        int result = ConfigureTrigger(nodeMap);
        if (result != 0) {
            try {
                CEnumerationPtr ptrTriggerMode = nodeMap.GetNode("TriggerMode");
                if (IsAvailable(ptrTriggerMode) && IsWritable(ptrTriggerMode)) {
                    CEnumEntryPtr ptrTriggerModeOff = ptrTriggerMode->GetEntryByName("Off");
                    if (IsAvailable(ptrTriggerModeOff) && IsReadable(ptrTriggerModeOff)) {
                        ptrTriggerMode->SetIntValue(ptrTriggerModeOff->GetValue());
                    }
                }
                enableTrigger = 1;
            } catch (const Spinnaker::Exception& e) {
                std::cout << "Error : " << e.what() << std::endl;
            }
        }

        // Set acquisition mode to continuous
        CEnumerationPtr ptrAcquisitionMode = nodeMap.GetNode("AcquisitionMode");
        if (!IsAvailable(ptrAcquisitionMode) || !IsWritable(ptrAcquisitionMode))
        {
            std::cout << "Unable to set acquisition mode to continuous (node retrieval). Aborting..." << std::endl << std::endl;
            exit(-1);
        }

        CEnumEntryPtr ptrAcquisitionModeContinuous = ptrAcquisitionMode->GetEntryByName("Continuous");
        if (!IsAvailable(ptrAcquisitionModeContinuous) || !IsReadable(ptrAcquisitionModeContinuous))
        {
            std::cout << "Unable to set acquisition mode to continuous (entry 'continuous' retrieval). Aborting..." << std::endl << std::endl;
            exit(-1);
        }

        int64_t acquisitionModeContinuous = ptrAcquisitionModeContinuous->GetValue();

        ptrAcquisitionMode->SetIntValue(acquisitionModeContinuous);

        std::cout << "Acquisition mode set to continuous..." << std::endl;

        cam->BeginAcquisition();
    }catch (Spinnaker::Exception &ex){
        std::cout << "Error : " << ex.what() << std::endl;
    }
    std::cout << "valeur de capoturing: " << capturing << std::endl;
    while(capturing){

        std::vector<Spinnaker::ImagePtr> images = captureImage();

        ImagePtr image = images.at(0);
        ImagePtr coloredImage = images.at(1);

        //20/05/2024, Armand & Nathan : send frame only if the image is not null

        if(image != nullptr && coloredImage != nullptr && capturing) {
            AbstractCamera::sendFrame(image->GetData(), image->GetBufferSize(), image->GetWidth(), image->GetHeight(), false);
            AbstractCamera::sendFrame(coloredImage->GetData(), coloredImage->GetBufferSize(), coloredImage->GetWidth(), coloredImage->GetHeight(), true);
        }
    }
    try{
        //cam->AcquisitionStop.Execute();
        cam->EndAcquisition();
    }catch (Spinnaker::Exception &ex){
        std::cout << "Error : " << ex.what() << std::endl;
    }


}
//Stop the auto captured
//wrote on 11/06/2019 by French students
void SpinCamera::stopAutoCapture(){
    capturing = false;
}

unsigned char* SpinCamera::retrieveImage(unsigned int* bufferSize, unsigned int* imageWidth, unsigned int* imageHeight, bool colored) {

    if (capturing) return nullptr;
    capturing = true;
    //printf("Images begin to be retrieved\n");

    bool isAlreadyInit = cam->IsInitialized();
    if (!isAlreadyInit) {
        cam->Init();
    }

    Spinnaker::TriggerModeEnums oldTriggerValue;

    try {
        oldTriggerValue = cam->TriggerMode.GetValue();
        cam->TriggerMode.SetValue(TriggerModeEnums::TriggerMode_Off);
    } catch (Spinnaker::Exception e) {
        qInfo() << e.what();
        if (!isAlreadyInit) {
            cam = nullptr;
        }
        capturing = false;
        return nullptr;
    }

    //printf("Retrieving images...\n");
    //cam->AcquisitionStart.Execute(); <- Old version

    // Armand & Nathan 10/05/2024 : replaced method to start acquisition
    cam->BeginAcquisition();

    //printf("Retrieving 1...\n");

    std::vector<Spinnaker::ImagePtr> images = captureImage();

    ImagePtr image = nullptr;

    if(colored){
        image = images.at(1);
    }else{
        image = images.at(0);
    }

    *bufferSize = image->GetBufferSize();
    *imageWidth = image->GetWidth();
    *imageHeight = image->GetHeight();
    unsigned char* imageBuffer = new unsigned char[image->GetBufferSize()];
    memcpy(imageBuffer, image->GetData(), image->GetBufferSize());


    // Armand & Nathan 10/05/2024 : deleting the image causes Camera Manager to crash
    //delete image;

    //printf("Retrieving 2...\n");

    cam->TriggerMode.SetValue(oldTriggerValue);

    //printf("Retrieving 3...\n");
    //cam->AcquisitionStop.Execute(); <- Old version

    // Armand & Nathan 10/05/2024 : replaced method to stop acquisition
    cam->EndAcquisition();

    if (!isAlreadyInit) {
        cam = nullptr;
    }
    //printf("Images retrieved\n");
    capturing = false;
    return imageBuffer;
}

bool SpinCamera::equalsTo(AbstractCamera *c){
    return getSerial() == c->getSerial() && getString() == c->getString();
}

std::string SpinCamera::getString(){
    return deviceId;
}

ImagePtr SpinCamera::getImage(bool colored)
{
    std::vector<Spinnaker::ImagePtr> images = captureImage();

    ImagePtr image = nullptr;

    if(colored){
        image = images.at(1);
    }else{
        image = images.at(0);
    }

    return image;
}
