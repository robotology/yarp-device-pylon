/*
 * Copyright (C) 2006-2022 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#include <cmath>
#include <algorithm>
#include <iomanip>
#include <cstdint>

#include <yarp/os/LogComponent.h>
#include <yarp/os/Value.h>
#include <yarp/sig/ImageUtils.h>


#include "pylonDriver.h"

using namespace yarp::dev;
using namespace yarp::sig;
using namespace yarp::os;

using namespace std;

namespace {
YARP_LOG_COMPONENT(PYLON, "yarp.device.pylon")
}

pylonDriver::pylonDriver()
{
}


bool pylonDriver::setFramerate(const int _fps)
{
    return true;
}


bool pylonDriver::open(Searchable& config)
{
    return true;
}

bool pylonDriver::close()
{
    return true;
}

int pylonDriver::getRgbHeight()
{
    return 0;
}

int pylonDriver::getRgbWidth()
{
    return 0;
}

bool pylonDriver::getRgbSupportedConfigurations(yarp::sig::VectorOf<CameraConfig> &configurations)
{
    yCWarning(PYLON) << "getRgbSupportedConfigurations not implemented yet";
    return false;
}

bool pylonDriver::getRgbResolution(int &width, int &height)
{
    return true;
}


bool pylonDriver::setRgbResolution(int width, int height)
{

    return true;
}


bool pylonDriver::setRgbFOV(double horizontalFov, double verticalFov)
{
    // It seems to be not available...
    return false;
}

bool pylonDriver::getRgbFOV(double &horizontalFov, double &verticalFov)
{
    return true;
}

bool pylonDriver::getRgbMirroring(bool& mirror)
{
    yCWarning(PYLON) << "Mirroring not supported";
    return false;
}

bool pylonDriver::setRgbMirroring(bool mirror)
{
    yCWarning(PYLON) << "Mirroring not supported";
    return false;
}

bool pylonDriver::getRgbIntrinsicParam(Property& intrinsic)
{
    return true;
}


bool pylonDriver::getCameraDescription(CameraDescriptor* camera)
{
    return true;
}

bool pylonDriver::hasFeature(int feature, bool* hasFeature)
{
    return true;
}

bool pylonDriver::setFeature(int feature, double value)
{
    return true;
}

bool pylonDriver::getFeature(int feature, double *value)
{
    return true;
}

bool pylonDriver::setFeature(int feature, double value1, double value2)
{
    yCError(PYLON) << "No 2-valued feature are supported";
    return false;
}

bool pylonDriver::getFeature(int feature, double *value1, double *value2)
{
    yCError(PYLON) << "No 2-valued feature are supported";
    return false;
}

bool pylonDriver::hasOnOff(  int feature, bool *HasOnOff)
{
    return true;
}

bool pylonDriver::setActive( int feature, bool onoff)
{

    return true;
}

bool pylonDriver::getActive( int feature, bool *isActive)
{
    return true;
}

bool pylonDriver::hasAuto(int feature, bool *hasAuto)
{
    return true;
}

bool pylonDriver::hasManual( int feature, bool* hasManual)
{
    return true;
}

bool pylonDriver::hasOnePush(int feature, bool* hasOnePush)
{
    return hasAuto(feature, hasOnePush);
}

bool pylonDriver::setMode(int feature, FeatureMode mode)
{
    yCError(PYLON) << "Feature does not have both auto and manual mode";
    return false;
}

bool pylonDriver::getMode(int feature, FeatureMode* mode)
{
    return true;
}

bool pylonDriver::setOnePush(int feature)
{
    return true;
}

bool pylonDriver::getImage(yarp::sig::ImageOf<yarp::sig::PixelRgb>& image)
{
    return true;
}

int  pylonDriver::height() const
{
    return 0;
}

int  pylonDriver::width() const
{
    return 0;
}
