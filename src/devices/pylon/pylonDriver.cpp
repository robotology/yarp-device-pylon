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

// VERY IMPORTANT ABOUT WHITE BALANCE: the YARP interfaces cannot allow to set a feature with
// 3 values, 2 is maximum and until now we always used blue and red in this order. Then we ignore
// green

static const std::vector<cameraFeature_id_t> supported_features { YARP_FEATURE_BRIGHTNESS,
                                                                  YARP_FEATURE_EXPOSURE,
                                                                  YARP_FEATURE_SHARPNESS,
                                                                  YARP_FEATURE_WHITE_BALANCE,
                                                                  YARP_FEATURE_GAMMA,
                                                                  YARP_FEATURE_GAIN,
                                                                  YARP_FEATURE_TRIGGER, // not sure how to use it
                                                                  YARP_FEATURE_FRAME_RATE };

static const std::vector<cameraFeature_id_t> features_with_auto { YARP_FEATURE_EXPOSURE,
                                                                  YARP_FEATURE_WHITE_BALANCE,
                                                                  YARP_FEATURE_GAIN };

pylonDriver::pylonDriver() //: m_factory(CTlFactory::GetInstance())
{
}


bool pylonDriver::setFramerate(const float _fps)
{
    auto& node_map = m_camera_ptr->GetNodeMap();
    stopCamera();
    try
    {
        yCDebug(PYLON)<<"Setting framerate to"<<_fps;
        CFloatParameter(node_map, "AcquisitionFrameRate").SetValue(_fps);
        m_fps = _fps;
    }
    catch (const GenericException &e)
    {
        // Error handling.
        yCError(PYLON)<< "Camera"<<m_serial_number<<"cannot set fps to:"<<_fps<<"error:"<<e.GetDescription();
        return false;
    }
    return startCamera();
}

bool parseUint32Param(std::string param_name, std::uint32_t& param, yarp::os::Searchable& config) {
    if (config.check(param_name) && config.find(param_name).isInt32())
    {
        param = config.find(param_name).asInt32();
        return true;
    }
    else
    {
        yCWarning(PYLON) << param_name << "parameter not specifie, using"<<param;
        return false;
    }
}
bool parseFloat64Param(std::string param_name, double& param, yarp::os::Searchable& config) {
    if (config.check(param_name) && config.find(param_name).isFloat64())
    {
        param = config.find(param_name).asFloat64();
        return true;
    }
    else
    {
        yCWarning(PYLON) << param_name << "parameter not specified, using"<<param;
        return false;
    }

}
bool parseStringParam(std::string param_name, std::string& param, yarp::os::Searchable& config) {
    if (config.check(param_name) && config.find(param_name).isFloat64())
    {
        param = config.find(param_name).asFloat64();
        return true;
    }
    else
    {
        yCWarning(PYLON) << param_name << "parameter not specified, using"<<param;
        return false;
    }
}

bool pylonDriver::startCamera() {
    if (m_camera_ptr)
    {
        if (!m_camera_ptr->IsGrabbing())
        {
            m_camera_ptr->StartGrabbing();
        }
    }
    return true;
}

bool pylonDriver::stopCamera() {
    if (m_camera_ptr)
    {
        if (m_camera_ptr->IsGrabbing())
        {
            m_camera_ptr->StopGrabbing();
        }
    }
    return true;
}

bool pylonDriver::open(Searchable& config)
{
    bool ok{true};
    yCTrace(PYLON) << "input params are " << config.toString();
    if (!config.check("serial_number"))
    {
        yCError(PYLON)<< "serial_number parameter not specified";
        return false;
    }
    // TODO understand how to treat it, if string or int
    m_serial_number = config.find("serial_number").toString().c_str();

    double period{0.03};
    parseUint32Param("width", m_width, config);
    parseUint32Param("height", m_height, config);
    parseFloat64Param("period", period, config);

    if (period != 0.0)
    {
        m_fps = 1.0/period; // the fps has to be aligned with the nws period
    }

    // Initialize pylon resources
    PylonInitialize();
    // Get factory singleton
    CTlFactory& factory = CTlFactory::GetInstance();
    yCDebug(PYLON)<<"SERIAL NUMBER!"<<m_serial_number<<config.find("serial_number").asString();
    // Open the device using the S/N
    try
    {
        m_camera_ptr = std::make_unique<Pylon::CInstantCamera>(factory.CreateDevice(CDeviceInfo().SetSerialNumber(m_serial_number)));
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
    // TODO get it from conf

    auto& nodemap = m_camera_ptr->GetNodeMap();
    // TODO maybe put in a try catch
    CBooleanParameter(nodemap, "AcquisitionFrameRateEnable").SetValue(true);
    CBooleanParameter(nodemap, "BslScalingEnable").SetValue(true);
    ok = ok && setRgbResolution(m_width, m_height);

    // TODO disabling it for testing the network, probably it is better to keep it as Auto
    CEnumParameter(nodemap, "ExposureAuto").SetValue("Off");

    ok = ok && setFramerate(m_fps);

    yCDebug(PYLON)<<"Starting with this fps"<<CFloatParameter(nodemap, "AcquisitionFrameRate").GetValue();

    return ok;
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
    if (width > 0 && height > 0)
    {
        stopCamera();
        try
        {
            yCDebug(PYLON)<<"Setting width and height to"<<width<<height;
            CIntegerParameter(m_camera_ptr->GetNodeMap(), "Width").SetValue(m_width);
            CIntegerParameter(m_camera_ptr->GetNodeMap(), "Height").SetValue(m_height);
            m_width = width;
            m_height = height;
        }
        catch (const GenericException &e)
        {
            // Error handling.
            yCError(PYLON)<< "Camera"<<m_serial_number<<"cannot set width and height to"<<width<<height<<"error:"<<e.GetDescription();
            startCamera();
            return false;
        }

    }
    return startCamera();
}


bool pylonDriver::setRgbFOV(double horizontalFov, double verticalFov)
{
    yCWarning(PYLON) << "setRgbFOV not supported";
    return false;
}

bool pylonDriver::getRgbFOV(double &horizontalFov, double &verticalFov)
{
    yCWarning(PYLON) << "getRgbFOV not supported";
    return false;
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
    yCWarning(PYLON) << "getRgbIntrinsicParam not implemented yet";
    return false;
}


bool pylonDriver::getCameraDescription(CameraDescriptor* camera)
{
    yCWarning(PYLON) << "getCameraDescription not implemented yet";
    return false;
}

bool pylonDriver::hasFeature(int feature, bool* hasFeature)
{
    cameraFeature_id_t f;
    f = static_cast<cameraFeature_id_t>(feature);
    if (f < YARP_FEATURE_BRIGHTNESS || f > YARP_FEATURE_NUMBER_OF-1)
    {
        return false;
    }

    *hasFeature = std::find(supported_features.begin(), supported_features.end(), f) != supported_features.end();

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
    return true;
}

bool pylonDriver::getFeature(int feature, double *value1, double *value2)
{
    return true;
}

bool pylonDriver::hasOnOff(  int feature, bool *HasOnOff)
{
    // Not sure
    return hasAuto(feature, HasOnOff);
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
    cameraFeature_id_t f;
    f = static_cast<cameraFeature_id_t>(feature);
    if (f < YARP_FEATURE_BRIGHTNESS || f > YARP_FEATURE_NUMBER_OF-1)
    {
        return false;
    }

    *hasAuto = std::find(features_with_auto.begin(), features_with_auto.end(), f) != features_with_auto.end();

    return true;
}

bool pylonDriver::hasManual( int feature, bool* hasManual)
{
    return hasFeature(feature, hasManual);
}

bool pylonDriver::hasOnePush(int feature, bool* hasOnePush)
{
    return hasAuto(feature, hasOnePush);
}

bool pylonDriver::setMode(int feature, FeatureMode mode)
{
    return true;
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
    std::lock_guard<std::mutex> guard(m_mutex);
    //yCDebug(PYLON)<<"GETTIIMAGE";
    if (m_camera_ptr->IsGrabbing())
    {
        CGrabResultPtr grab_result_ptr;
        CPylonImage pylon_image;
        CImageFormatConverter pylon_format_converter;
        pylon_format_converter.OutputPixelFormat = PixelType_RGB8packed;
        // if(first_acquisition){
        //     yCDebug(PYLON)<<"Discard first frame";
        //     first_acquisition = false;
        //     m_camera_ptr->RetrieveResult( 5000, grab_result_ptr, TimeoutHandling_ThrowException);
        //     return true;
        // }
        // Wait for an image and then retrieve it. A timeout of 5000 ms is used.

        m_camera_ptr->RetrieveResult( 5000, grab_result_ptr, TimeoutHandling_ThrowException);
        // Image grabbed successfully?
        if (grab_result_ptr && grab_result_ptr->GrabSucceeded())
        {
            m_width  = grab_result_ptr->GetWidth();
            m_height = grab_result_ptr->GetHeight();
            size_t mem_to_wrt = m_width * m_height * image.getPixelSize();

            // TODO Check pixel code
            image.resize(m_width, m_height);

            // yCDebug(PYLON)<<"padding x"<<grab_result_ptr->GetPaddingX();
            // yCDebug(PYLON)<<"padding y"<<grab_result_ptr->GetPaddingY();
            // yCDebug(PYLON)<<"image size"<<grab_result_ptr->GetImageSize();
            // yCDebug(PYLON)<<"payload size"<<grab_result_ptr->GetPayloadSize();
            // yCDebug(PYLON)<<"pixel type is "<<CPixelTypeMapper::GetNameByPixelType(grab_result_ptr->GetPixelType());
            pylon_format_converter.Convert(pylon_image, grab_result_ptr);
            if (!pylon_image.IsValid()) {
                 yCError(PYLON)<<"Frame invalid!";
                 return false;
            }

            // Access the image data.
            //yCDebug(PYLON) << "SizeX:" << grab_result_ptr->GetWidth();
            //yCDebug(PYLON) << "SizeY:" << grab_result_ptr->GetHeight();
            // For some reason the first frame cannot be converted To be investigated
            static bool first_acquisition{true};
            if (first_acquisition) {
                yCDebug(PYLON)<<"Skipping";
                first_acquisition = false;
                return false;
            }
            memcpy((void*)image.getRawImage(), pylon_image.GetBuffer(), mem_to_wrt);
        }
        else {
            yCError(PYLON)<<"Acquisition failed";
            return false;
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
