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



// VERY IMPORTANT ABOUT WHITE BALANCE: the YARP interfaces cannot allow to set a feature with
// 3 values, 2 is maximum and until now we always used blue and red in this order. Then we ignore
// green

static const std::vector<cameraFeature_id_t> supported_features { YARP_FEATURE_BRIGHTNESS,
                                                                  YARP_FEATURE_EXPOSURE,
                                                                  YARP_FEATURE_SHARPNESS,
                                                                  YARP_FEATURE_WHITE_BALANCE,
                                                                  YARP_FEATURE_GAMMA,
                                                                  YARP_FEATURE_GAIN,
                                                                  //YARP_FEATURE_TRIGGER, // not sure how to use it
                                                                  YARP_FEATURE_FRAME_RATE };

static const std::vector<cameraFeature_id_t> features_with_auto { YARP_FEATURE_EXPOSURE,
                                                                  YARP_FEATURE_WHITE_BALANCE,
                                                                  YARP_FEATURE_GAIN };

// Values taken from the balser documentation for da4200-30mci
static const std::map<cameraFeature_id_t, std::pair<double,double>> featureMinMax { {YARP_FEATURE_BRIGHTNESS, {-1.0, 1.0}},
                                                                                    {YARP_FEATURE_EXPOSURE, {68.0, 2300000.0}},
                                                                                    {YARP_FEATURE_SHARPNESS, {0.0, 1.0}},
                                                                                    {YARP_FEATURE_WHITE_BALANCE, {1.0, 8.0}}, // not sure about it, the doc is not clear, found empirically
                                                                                    {YARP_FEATURE_GAMMA, {0.0, 4.0}},
                                                                                    {YARP_FEATURE_GAIN, {0.0, 33.06}} };

// We usually set the features through a range between 0 an 1, we have to translate it in meaninful value for the camera
double fromZeroOneToRange(cameraFeature_id_t feature, double value){
    return value * (featureMinMax.at(feature).second - featureMinMax.at(feature).first) + featureMinMax.at(feature).first;
}

// We want the features in the range 0 1
double fromRangeToZeroOne(cameraFeature_id_t feature, double value){
    return (value - featureMinMax.at(feature).first) / (featureMinMax.at(feature).second - featureMinMax.at(feature).first);
}


bool pylonDriver::setFramerate(const float _fps)
{
    auto res = setOption("AcquisitionFrameRate",_fps);
    if (res) {
        m_fps =_fps;
    }
    return res;
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
    ok = ok && setOption("AcquisitionFrameRateEnable", true);
    ok = ok && setOption("BslScalingEnable", true);
    ok = ok && setRgbResolution(m_width, m_height);

    // TODO disabling it for testing the network, probably it is better to keep it as Auto
    ok = ok && setOption("ExposureAuto", "Off", true);

    ok = ok && setFramerate(m_fps);

    yCDebug(PYLON)<<"Starting with this fps"<<CFloatParameter(nodemap, "AcquisitionFrameRate").GetValue();

    return startCamera();
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
    bool res = false;
    if (width > 0 && height > 0)
    {
        res = setOption("Width", width);
        res = res && setOption("Height", height);
        if (res) {
            m_width = width;
            m_height = height;
        }
    }
    return res;
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
    bool b = false;
    if (!hasFeature(feature, &b) || !b)
    {
        yCError(PYLON) << "Feature not supported!";
        return false;
    }
    b = false;
    auto f = static_cast<cameraFeature_id_t>(feature);
    switch(f)
    {
    case YARP_FEATURE_BRIGHTNESS:
        b = setOption("BslBrightness", fromZeroOneToRange(f, value));
        break;
    case YARP_FEATURE_EXPOSURE:
        // According to https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/extended-controls.html
        // 1 unit = 100us, basler instead accept us. Setting directly in us.
        b = setOption("ExposureTime", fromZeroOneToRange(f, value));
        break;
    case YARP_FEATURE_SHARPNESS:
        b = setOption("BslSharpnessEnhancement", fromZeroOneToRange(f, value));
        break;
    case YARP_FEATURE_WHITE_BALANCE:
        b = false;
        yCError(PYLON)<<"White balance require 2 values";
        break;
    case YARP_FEATURE_GAMMA:
        b = setOption("Gamma", fromZeroOneToRange(f, value));
        break;
    case YARP_FEATURE_GAIN:
        b = setOption("Gain", fromZeroOneToRange(f, value));
        break;
    case YARP_FEATURE_FRAME_RATE:
        b = setFramerate(value);
        break;
    default:
        yCError(PYLON) << "Feature not supported!";
        return false;
    }

    return b;
}

bool pylonDriver::getFeature(int feature, double *value)
{
    bool b = false;
    if (!hasFeature(feature, &b) || !b)
    {
        yCError(PYLON) << "Feature not supported!";
        return false;
    }
    b = false;
    auto f = static_cast<cameraFeature_id_t>(feature);
    switch(f)
    {
    case YARP_FEATURE_BRIGHTNESS:
        b = getOption("BslBrightness", value);
        break;
    case YARP_FEATURE_EXPOSURE:
        b = getOption("ExposureTime", value);
        break;
    case YARP_FEATURE_SHARPNESS:
        b = getOption("BslSharpnessEnhancement", value);
        break;
    case YARP_FEATURE_WHITE_BALANCE:
        b = false;
        yCError(PYLON)<<"White balance is a 2-values feature";
        break;
    case YARP_FEATURE_GAMMA:
        b = getOption("Gamma", value);
        break;
    case YARP_FEATURE_GAIN:
        b = getOption("Gain", value);
        break;
    case YARP_FEATURE_FRAME_RATE:
        b = true;
        *value = m_fps;
        break;
    default:
        yCError(PYLON) << "Feature not supported!";
        return false;
    }

    *value = fromRangeToZeroOne(f,*value);
    yCDebug(PYLON)<<"In 0-1"<<*value;
    return b;
}

bool pylonDriver::setFeature(int feature, double value1, double value2)
{
    auto f = static_cast<cameraFeature_id_t>(feature);
    if (f != YARP_FEATURE_WHITE_BALANCE) {
        yCError(PYLON)<<YARP_FEATURE_WHITE_BALANCE<<"is not a 2-values feature supported";
        return false;
    }

    auto res = setOption("BalanceRatioSelector", "Blue", true);
    res = res && setOption("BalanceRatio", fromZeroOneToRange(f, value1));
    res = res && setOption("BalanceRatioSelector", "Red", true);
    res = res && setOption("BalanceRatio", fromZeroOneToRange(f, value2));
    return res;
}

bool pylonDriver::getFeature(int feature, double *value1, double *value2)
{
    auto f = static_cast<cameraFeature_id_t>(feature);
    if (f != YARP_FEATURE_WHITE_BALANCE) {
        yCError(PYLON)<<"This is not a 2-values feature supported";
        return false;
    }

    auto res = setOption("BalanceRatioSelector", "Blue", true);
    res = res && getOption("BalanceRatio", value1);
    res = res && setOption("BalanceRatioSelector", "Red", true);
    res = res && getOption("BalanceRatio", value2);
    *value1 = fromRangeToZeroOne(f,*value1);
    *value2 = fromRangeToZeroOne(f,*value2);
    yCDebug(PYLON)<<"In 0-1"<<*value1;
    yCDebug(PYLON)<<"In 0-1"<<*value2;
    return res;
}

bool pylonDriver::hasOnOff(  int feature, bool *HasOnOff)
{
    return hasAuto(feature, HasOnOff);
}

bool pylonDriver::setActive( int feature, bool onoff)
{
    bool b = false;
    if (!hasFeature(feature, &b) || !b)
    {
        yCError(PYLON) << "Feature"<<feature<<"not supported!";
        return false;
    }

    if (!hasOnOff(feature, &b) || !b)
    {
        yCError(PYLON) << "Feature"<<feature<<"does not have OnOff.. call hasOnOff() to know if a specific feature support OnOff mode";
        return false;
    }

    std::string val_to_set = onoff ? "Continuous" : "Off";

    switch(feature)
    {
    case YARP_FEATURE_EXPOSURE:
        b = setOption("ExposureAuto", val_to_set, true);
        break;
    case YARP_FEATURE_WHITE_BALANCE:
        b = setOption("BalanceWhiteAuto", val_to_set, true);
        break;
    case YARP_FEATURE_GAIN:
        b = setOption("GainAuto", val_to_set, true);
        break;
    default:
        yCError(PYLON) << "Feature"<<feature<<"not supported!";
        return false;
    }

    return b;
}

bool pylonDriver::getActive( int feature, bool *isActive)
{
    bool b = false;
    if (!hasFeature(feature, &b) || !b)
    {
        yCError(PYLON)<< "Feature"<<feature<<"not supported!";
        return false;
    }

    if (!hasOnOff(feature, &b) || !b)
    {
        yCError(PYLON) << "Feature"<<feature<<"does not have OnOff.. call hasOnOff() to know if a specific feature support OnOff mode";
        return false;
    }

    std::string val_to_get{""};

    switch(feature)
    {
    case YARP_FEATURE_EXPOSURE:
        b = getOption("ExposureAuto", val_to_get, true);
        break;
    case YARP_FEATURE_WHITE_BALANCE:
        b = getOption("BalanceWhiteAuto", val_to_get, true);
        break;
    case YARP_FEATURE_GAIN:
        b = getOption("GainAuto", val_to_get, true);
        break;
    default:
        yCError(PYLON)<<"Feature"<<feature<<"not supported!";
        return false;
    }
    if (b) {
        if (val_to_get == "Continuous") {
            *isActive = true;
        }
        else if (val_to_get == "Off") {
            *isActive = false;
        }
    }
    return b;
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
    bool b {false};
    if (!hasAuto(feature, &b) || !b)
    {
        yCError(PYLON) << "Feature"<<feature<<"not supported!";
        return false;
    }

    switch(mode)
    {
    case MODE_AUTO:
        return setActive(feature, true);
    case MODE_MANUAL:
        return setActive(feature, false);
    case MODE_UNKNOWN:
        return false;
    default:
        return false;
    }
    return b;
}

bool pylonDriver::getMode(int feature, FeatureMode* mode)
{
    bool b {false};
    if (!hasAuto(feature, &b) || !b)
    {
        yCError(PYLON) << "Feature"<<feature<<"not supported!";
        return false;
    }
    bool get_active{false};
    b = b && getActive(feature, &get_active);

    if (b) {
        if (get_active) {
            *mode = MODE_AUTO;
        }
        else {
            *mode = MODE_MANUAL;
        }
    }
    return b;
}

bool pylonDriver::setOnePush(int feature)
{
    bool b = false;
    if (!hasOnePush(feature, &b) || !b)
    {
        yCError(PYLON) << "Feature"<<feature<<"doesn't have OnePush";
        return false;
    }

    b = b && setMode(feature, MODE_AUTO);
    b = b && setMode(feature, MODE_MANUAL);

    return b;
}

bool pylonDriver::getImage(yarp::sig::ImageOf<yarp::sig::PixelRgb>& image)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    if (m_camera_ptr->IsGrabbing())
    {
        CGrabResultPtr grab_result_ptr;
        CPylonImage pylon_image;
        CImageFormatConverter pylon_format_converter;
        pylon_format_converter.OutputPixelFormat = PixelType_RGB8packed;
        // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
        // TODO change the hardcoded 5000 to the exposure time.
        try {
            m_camera_ptr->RetrieveResult( 5000, grab_result_ptr, TimeoutHandling_ThrowException);
        }
        catch (const Pylon::GenericException &e)
        {
            // Error handling.
            yCError(PYLON)<< "Camera"<<m_serial_number<<"cannot get images error:"<<e.GetDescription();
            return false;
        }
        // Image grabbed successfully?
        if (grab_result_ptr && grab_result_ptr->GrabSucceeded())
        {
            m_width  = grab_result_ptr->GetWidth();
            m_height = grab_result_ptr->GetHeight();
            size_t mem_to_wrt = m_width * m_height * image.getPixelSize();

            // TODO Check pixel code
            image.resize(m_width, m_height);
            pylon_format_converter.Convert(pylon_image, grab_result_ptr);
            if (!pylon_image.IsValid()) {
                 yCError(PYLON)<<"Frame invalid!";
                 return false;
            }
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
