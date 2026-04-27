#ifndef SPINNAKER_COMPAT_H
#define SPINNAKER_COMPAT_H

#if defined(CAMERA_MANAGER_USE_SYSTEM_SPINNAKER_HEADERS)
#if defined(__has_include)
#if __has_include(<spinnaker/SpinnakerPlatform.h>)
#include <spinnaker/SpinnakerPlatform.h>
#ifdef SPINNAKER_DEPRECATED_CLASS
#undef SPINNAKER_DEPRECATED_CLASS
#endif
#define SPINNAKER_DEPRECATED_CLASS(msg) class SPINNAKER_API [[deprecated(msg)]]

#include <spinnaker/Camera.h>
#include <spinnaker/CameraList.h>
#include <spinnaker/CameraPtr.h>
#include <spinnaker/Exception.h>
#include <spinnaker/Image.h>
#include <spinnaker/ImagePtr.h>
#include <spinnaker/ImageProcessor.h>
#include <spinnaker/System.h>
#include <spinnaker/SystemPtr.h>
#include <spinnaker/SpinGenApi/SpinnakerGenApi.h>
#elif __has_include(<Spinnaker/SpinnakerPlatform.h>)
#include <Spinnaker/SpinnakerPlatform.h>
#ifdef SPINNAKER_DEPRECATED_CLASS
#undef SPINNAKER_DEPRECATED_CLASS
#endif
#define SPINNAKER_DEPRECATED_CLASS(msg) class SPINNAKER_API [[deprecated(msg)]]

#include <Spinnaker/Camera.h>
#include <Spinnaker/CameraList.h>
#include <Spinnaker/CameraPtr.h>
#include <Spinnaker/Exception.h>
#include <Spinnaker/Image.h>
#include <Spinnaker/ImagePtr.h>
#include <Spinnaker/ImageProcessor.h>
#include <Spinnaker/System.h>
#include <Spinnaker/SystemPtr.h>
#include <Spinnaker/SpinGenApi/SpinnakerGenApi.h>
#else
#error "Unable to find system Spinnaker headers. Set SPINNAKER_ROOT or disable CAMERA_MANAGER_USE_SYSTEM_SPINNAKER_HEADERS."
#endif
#else
#error "__has_include is required to resolve system Spinnaker headers."
#endif
#else
#include "include/Spinnaker/Spinnaker.h"
#include "include/Spinnaker/SpinGenApi/SpinnakerGenApi.h"
#endif


#endif
