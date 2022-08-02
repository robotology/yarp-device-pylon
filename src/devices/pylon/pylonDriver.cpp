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
using namespace Pylon;

using namespace std;

namespace {
YARP_LOG_COMPONENT(PYLON, "yarp.device.pylon")
}

pylonDriver::pylonDriver() //: m_factory(CTlFactory::GetInstance())
{
}


bool pylonDriver::setFramerate(const int _fps)
{
    return true;
}


bool pylonDriver::open(Searchable& config)
{
    yCTrace(PYLON) << "input params are " << config.toString();
    if (!config.check("serial_number"))
    {
        yCError(PYLON)<< "serial_number parameter not specified";
        return false;
    }
    yCDebug(PYLON)<<"1";
    m_serial_number = config.find("serial_number").toString().c_str();
    // Initialize pylon resources
    PylonInitialize();
    // Get factory singleton
    CTlFactory& factory = CTlFactory::GetInstance();
    yCDebug(PYLON)<<"SERIAL NUMBER!"<<m_serial_number<<config.find("serial_number").asString();
    yCDebug(PYLON)<<"2"<<m_serial_number;
    // Open the device using the S/N
    try
    {
        m_camera_ptr = std::make_unique<Pylon::CInstantCamera>(factory.CreateDevice(CDeviceInfo().SetSerialNumber(m_serial_number)));
        yCDebug(PYLON)<<"3";
        if (m_camera_ptr) {
            m_camera_ptr->Open();
            if (!m_camera_ptr->IsOpen()) {
                yCError(PYLON)<< "Camera"<<m_serial_number<<"cannot be opened";
                return false;
            }
        }
        else {
            yCError(PYLON)<< "Camera"<<m_serial_number<<"cannot be opened";
            return false;
        }
    }
    catch (const GenericException &e)
    {
        // Error handling.
        yCError(PYLON)<< "Camera"<<m_serial_number<<"cannot be opened, error:"<<e.GetDescription();
        return false;
    }
    yCDebug(PYLON)<<"4";
    // TODO get it from conf
    m_width  = 1024;
    m_height = 768;
    // Configuration is done, let's start grabbing
    m_camera_ptr->StartGrabbing();
    return true;
}

bool pylonDriver::close()
{   
    if (m_camera_ptr->IsPylonDeviceAttached()) {
        m_camera_ptr->DetachDevice();
    }
    // Releases all pylon resources. 
    PylonTerminate(); 
    return true;
}

int pylonDriver::getRgbHeight()
{
    return m_height;
}

int pylonDriver::getRgbWidth()
{
    return m_width;
}

bool pylonDriver::getRgbSupportedConfigurations(yarp::sig::VectorOf<CameraConfig> &configurations)
{
    yCWarning(PYLON) << "getRgbSupportedConfigurations not implemented yet";
    return false;
}

bool pylonDriver::getRgbResolution(int &width, int &height)
{
    width = m_width;
    height = m_height;
    return true;
}


bool pylonDriver::setRgbResolution(int width, int height)
{
    // TODO
    return false;
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
    if (m_camera_ptr->IsGrabbing())
    {
         // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
        m_camera_ptr->RetrieveResult( 5000, m_grab_result_ptr, TimeoutHandling_ThrowException);

        // Image grabbed successfully?
        if (m_grab_result_ptr && m_grab_result_ptr->GrabSucceeded())
        {
            m_width  = m_grab_result_ptr->GetWidth();
            m_height = m_grab_result_ptr->GetHeight();
            // TODO Check pixel code
            image.resize(m_width, m_height);

            // Access the image data.
            yCDebug(PYLON) << "SizeX:" << m_grab_result_ptr->GetWidth();
            yCDebug(PYLON) << "SizeY:" << m_grab_result_ptr->GetHeight();
            image.setExternal(m_grab_result_ptr->GetBuffer(), m_width, m_height);
        } 
        return true;
    }
    else
    {
        yCError(PYLON)<<"Errors in retrieving images";
        return false;
    }
}

int  pylonDriver::height() const
{
    return m_height;
}

int  pylonDriver::width() const
{
    return m_width;
}
