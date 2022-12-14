/*
 * Copyright (C) 2006-2022 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#ifndef PYLON_DRIVER_H
#define PYLON_DRIVER_H

#include <pylon/PylonIncludes.h>
#include <yarp/dev/DeviceDriver.h>
#include <yarp/dev/IFrameGrabberControls.h>
#include <yarp/dev/IFrameGrabberImage.h>
#include <yarp/dev/IRgbVisualParams.h>
#include <yarp/os/PeriodicThread.h>
#include <yarp/os/Stamp.h>
#include <yarp/sig/Matrix.h>
#include <yarp/sig/all.h>

#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <typeinfo>

/**
 * @ingroup dev_impl_media
 *
 * \brief `pylonCamera`: YARP device driver implementation for acquiring images
 * from Pylon cameras.
 *
 * | YARP device name | YARP default nws        |
 * |:----------------:|:-----------------------:|
 * | `pylonCamera`    | `frameGrabber_nws_yarp` |
 *
 * The parameters accepted by this device are:
 * | Parameter name | SubParameter   | Type    | Units          | Default Value | Required                    | Description                                                       | Notes |
 * |:--------------:|:--------------:|:-------:|:--------------:|:-------------:|:--------------------------: |:-----------------------------------------------------------------:|:-----:|
 * | serial_number  |      -         | int     | -              |   -           | Yes                         | Serial number of the camera to be opened                          |  |
 * | period         |      -         | double  | s              |   0.0333      | No                          | Refresh period of acquistion from the camera in s                 | The cameras has a
 * value cap for the acquisition framerate, check the documentation | | rotation       |      -         | double  | degrees        |   0.0         | No                          | Rotation applied from
 * the center of the image                     | Depending the size requested some rotations are not allowed. The rotation worse the performance of the device. Allowed values: 0.0, 90.0, -90.0,
 * 180.0.| | width          |      -         | uint    | pixel          |   640         | No                          | Width of the images requested to the camera                       | The cameras
 * has a value cap for the width of the image that can provide, check the documentation. Zero or negative value not accepted | | height         |      -         | uint    | pixel          |   480 | No
 * | Height of the images requested to the camera                      | The cameras has a value cap for the width of the image that can provide, check the documentation. Zero or negative value not
 * accepted |
 *
 */

namespace
{
YARP_LOG_COMPONENT(PYLON_CAMERA, "yarp.device.pylonCamera")
}

class pylonCameraDriver : public yarp::dev::DeviceDriver, public yarp::dev::IFrameGrabberControls, public yarp::dev::IFrameGrabberImage, public yarp::dev::IRgbVisualParams
{
   private:
    using Stamp = yarp::os::Stamp;
    using Property = yarp::os::Property;
    using FlexImage = yarp::sig::FlexImage;

   public:
    pylonCameraDriver() = default;
    ~pylonCameraDriver() override = default;

    // DeviceDriver
    bool open(yarp::os::Searchable& config) override;
    bool close() override;

    // IRgbVisualParams
    int getRgbHeight() override;
    int getRgbWidth() override;
    bool getRgbSupportedConfigurations(yarp::sig::VectorOf<yarp::dev::CameraConfig>& configurations) override;
    bool getRgbResolution(int& width, int& height) override;
    bool setRgbResolution(int width, int height) override;
    bool getRgbFOV(double& horizontalFov, double& verticalFov) override;
    bool setRgbFOV(double horizontalFov, double verticalFov) override;
    bool getRgbMirroring(bool& mirror) override;
    bool setRgbMirroring(bool mirror) override;
    bool getRgbIntrinsicParam(Property& intrinsic) override;

    // IFrameGrabberControls
    bool getCameraDescription(CameraDescriptor* camera) override;
    bool hasFeature(int feature, bool* hasFeature) override;
    bool setFeature(int feature, double value) override;
    bool getFeature(int feature, double* value) override;
    bool setFeature(int feature, double value1, double value2) override;
    bool getFeature(int feature, double* value1, double* value2) override;
    bool hasOnOff(int feature, bool* HasOnOff) override;
    bool setActive(int feature, bool onoff) override;
    bool getActive(int feature, bool* isActive) override;
    bool hasAuto(int feature, bool* hasAuto) override;
    bool hasManual(int feature, bool* hasManual) override;
    bool hasOnePush(int feature, bool* hasOnePush) override;
    bool setMode(int feature, FeatureMode mode) override;
    bool getMode(int feature, FeatureMode* mode) override;
    bool setOnePush(int feature) override;

    // IFrameGrabberImage
    bool getImage(yarp::sig::ImageOf<yarp::sig::PixelRgb>& image) override;
    int height() const override;
    int width() const override;

   private:
    // method
    // inline bool setParams();
    bool setFramerate(const float _fps);
    template <class T>
    bool setOption(const std::string& option, T value, bool isEnum = false)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        // in some cases it is not used, suppressing the warning
        YARP_UNUSED(isEnum);
        bool ok{true};
        auto& node_map = m_camera_ptr->GetNodeMap();
        stopCamera();
        try
        {
            yCDebug(PYLON_CAMERA) << "Setting " << option << "to" << value;
            if constexpr (std::is_same<T, float>::value || std::is_same<T, double>::value)
            {
                Pylon::CFloatParameter(node_map, option.c_str()).SetValue(value);
            }
            else if constexpr (std::is_same<T, bool>::value)
            {
                Pylon::CBooleanParameter(node_map, option.c_str()).SetValue(value);
            }
            else if constexpr (std::is_same<T, int>::value)
            {
                Pylon::CIntegerParameter(node_map, option.c_str()).SetValue(value);
            }
            else if constexpr (std::is_same<T, const char*>::value)
            {
                if (isEnum)
                {
                    Pylon::CEnumParameter(node_map, option.c_str()).SetValue(value);
                }
                else
                {
                    Pylon::CStringParameter(node_map, option.c_str()).SetValue(value);
                }
            }
            else
            {
                yCError(PYLON_CAMERA) << "Option" << option << "has a type not supported, type" << typeid(T).name();
                startCamera();
                return false;
            }
        }
        catch (const Pylon::GenericException& e)
        {
            // Error handling.
            yCError(PYLON_CAMERA) << "Camera" << m_serial_number << "cannot set" << option << "to:" << value << "error:" << e.GetDescription();
            ok = false;
        }
        return startCamera() && ok;
    }

    template <class T>
    bool getOption(const std::string& option, T& value, bool isEnum = false)
    {
        auto& node_map = m_camera_ptr->GetNodeMap();
        // in some cases it is not used, suppressing the warning
        YARP_UNUSED(isEnum);
        try
        {
            if constexpr (std::is_same<T, float*>::value || std::is_same<T, double*>::value)
            {
                *value = Pylon::CFloatParameter(node_map, option.c_str()).GetValue();
                yCDebug(PYLON_CAMERA) << "Getting" << option << "value:" << *value;
            }
            else if constexpr (std::is_same<T, bool*>::value)
            {
                *value = Pylon::CBooleanParameter(node_map, option.c_str()).GetValue();
                yCDebug(PYLON_CAMERA) << "Getting" << option << "value:" << *value;
            }
            else if constexpr (std::is_same<T, int*>::value)
            {
                *value = Pylon::CIntegerParameter(node_map, option.c_str()).GetValue();
                yCDebug(PYLON_CAMERA) << "Getting" << option << "value:" << *value;
            }
            else if constexpr (std::is_same<T, std::string>::value)
            {
                if (isEnum)
                {
                    value = Pylon::CEnumParameter(node_map, option.c_str()).GetValue();
                    yCDebug(PYLON_CAMERA) << "Getting" << option << "value:" << value;
                }
                else
                {
                    value = Pylon::CStringParameter(node_map, option.c_str()).GetValue();
                    yCDebug(PYLON_CAMERA) << "Getting" << option << "value:" << value;
                }
            }
            else
            {
                yCError(PYLON_CAMERA) << "Option" << option << "has a type not supported, type" << typeid(T).name();
                return false;
            }
        }
        catch (const Pylon::GenericException& e)
        {
            // Error handling.
            yCError(PYLON_CAMERA) << "Camera" << m_serial_number << "cannot get" << option << "error:" << e.GetDescription();
            return false;
        }
        return true;
    }

    bool startCamera();
    bool stopCamera();

    mutable std::mutex m_mutex;

    yarp::os::Stamp m_rgb_stamp;
    mutable std::string m_lastError{""};
    bool m_verbose{false};
    bool m_initialized{false};
    float m_fps{30.0};
    double m_rotation{0.0};  // degrees
    uint32_t m_width{640};
    uint32_t m_height{480};
    Pylon::String_t m_serial_number{""};
    std::unique_ptr<Pylon::CInstantCamera> m_camera_ptr;
    bool m_rotationWithCrop{false};
};
#endif  // PYLON_DRIVER_H
