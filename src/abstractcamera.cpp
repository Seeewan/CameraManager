#include <iostream>
#include <QByteArray>
#include <QDebug>
#include <QMetaObject>
#include "abstractcamera.h"

AbstractCamera::AbstractCamera() : thread(this) {
    capturing= false;
    container = nullptr;
    coloredContainer = nullptr;
    serial = "unknown";
    model = "unknown";
}

AbstractCamera::~AbstractCamera() {
}

bool AbstractCamera::equalsTo(AbstractCamera* c){
    return this == c;
}

void AbstractCamera::startCapture(VideoOpenGLWidget* videoWidget, VideoOpenGLWidget* coloredVideoWidget){
    if(videoWidget == NULL || coloredVideoWidget == NULL){
        //qDebug() << "[ERROR] startCapture(VideoOpenGLWidget): videoWidget is NULL";
        //cout << "---------------------------------------" << endl << "Erreur" << endl;
        return;

    }
    container = videoWidget;
    coloredContainer = coloredVideoWidget;
    thread.start();
}

void AbstractCamera::sendFrame(void* imgBuffer, unsigned int bufferSize, unsigned int imageWidth, unsigned int imageHeight, bool colored) {
    /*
     * 13/05/2024
     * Modified by Nathan & Armand - added a verification to prevent updating a widget that isnt opened (and therefore crashed the application)
    */
    VideoOpenGLWidget* targetWidget = colored ? coloredContainer : container;
    if (targetWidget == nullptr || imgBuffer == nullptr || bufferSize == 0) {
        return;
    }

    QByteArray frameCopy(reinterpret_cast<const char*>(imgBuffer), static_cast<int>(bufferSize));
    QMetaObject::invokeMethod(targetWidget, [targetWidget, frameCopy, bufferSize, imageWidth, imageHeight]() mutable {
        if (!targetWidget->isVisible()) {
            return;
        }
        targetWidget->updateImage(reinterpret_cast<unsigned char*>(frameCopy.data()), bufferSize, imageWidth, imageHeight);
    }, Qt::QueuedConnection);
}


