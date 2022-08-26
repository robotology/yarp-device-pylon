
![YARP logo](https://raw.githubusercontent.com/robotology/yarp/master/doc/images/yarp-robot-24.png "yarp-device-pylon")
yarp-device-pylon
=================

This is the [pylon](https://www.baslerweb.com/en/products/basler-pylon-camera-software-suite/) device for [YARP](https://www.yarp.it/).
It supports the [Basler cameras](https://docs.baslerweb.com/cameras).

The **Basler™** cameras currently compatible with YARP are:
- [daa4200-30mci](https://docs.baslerweb.com/embedded-vision/daa4200-30mci)

License
-------

[![License](https://img.shields.io/badge/license-BSD--3--Clause%20%2B%20others-19c2d8.svg)](https://github.com/robotology/yarp-device-realsense2/blob/master/LICENSE)

This software may be modified and distributed under the terms of the
BSD-3-Clause license. See the accompanying LICENSE file for details.

The pylonCamera device uses the
[pylon](https://www.baslerweb.com/en/products/basler-pylon-camera-software-suite/) sdk, released
under the [pylon license](https://docs.baslerweb.com/licensing-information).
See the relative documentation for the terms of the license.

How to use Basler pylon cameras as a YARP device
---------------------------------------------------
### Dependencies
Before proceeding further, please install the following dependencies:
- [YARP 3.5 or greater](https://www.yarp.it/)
- [pylon](https://www.baslerweb.com/en/products/basler-pylon-camera-software-suite/)
- [OpenCV](https://opencv.org/) ( + [CUDA](https://opencv.org/platforms/cuda/) optional)

### Build and install yarp-device-pylon

```bash
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=<installation_path> ..
make
make install
```
In order to make the device detectable, add `<installation_path>/share/yarp` to the `YARP_DATA_DIRS` environment variable of the system.

Alternatively, if `YARP` has been installed using the [robotology-superbuild](https://github.com/robotology/robotology-superbuild), it is possible to use `<directory-where-you-downloaded-robotology-superbuild>/build/install` as the `<installation_path>`.

### How to run pylonCamera driver

From command line:

```bash
yarpdev --device frameGrabber_nws_yarp --subdevice pylonCamera --name /right_cam --serial_number 1234567 --period 0.033 --width 640 --height 480 --rotation 90.0
```

or

```
yarpdev --from PylonConf.ini
```

Where `PylonConf.ini`:

```ini
device frameGrabber_nws_yarp
subdevice pylonCamera
name /right_cam
serial_number 1234567
period 0.033
width 640
height 480
rotation 90.0
```


This is instead the minimum number of parameters for running the device, the default nws is `frameGrabber_nws_yarp`:
```
yarpdev --device pylonCamera --serial_number 1234567
```

## Device documentation
This device driver exposes the `yarp::dev::IFrameGrabberImage` and
`yarp::dev::IFrameGrabberControls` interfaces to read the images and operate on
the available settings.
See the documentation for more details about each interface.

| YARP device name | YARP default nws        |
|:----------------:|:-----------------------:|
| `pylonCamera`    | `frameGrabber_nws_yarp` |

The parameters accepted by this device are:
| Parameter name | SubParameter   | Type    | Units          | Default Value | Required                    | Description                                                       | Notes |
|:--------------:|:--------------:|:-------:|:--------------:|:-------------:|:--------------------------: |:-----------------------------------------------------------------:|:-----:|
| serial_number  |      -         | int     | -              |   -           | Yes                         | Serial number of the camera to be opened                          |  |
| period         |      -         | double  | s              |   0.0333      | No                          | Refresh period of acquistion from the camera in s                 | The cameras has a value cap for the acquisition framerate, check the documentation |
| rotation       |      -         | double  | degrees        |   0.0         | No                          | Rotation applied from the center of the image                     | Depending the size requested some rotations are not allowed. The rotation worse the performance of the device. Allowed values: 0.0, 90.0, -90.0, 180.0.|
| width          |      -         | uint    | pixel          |   640         | No                          | Width of the images requested to the camera                       | The cameras has a value cap for the width of the image that can provide, check the documentation. Zero or negative value not accepted |
| height         |      -         | uint    | pixel          |   480         | No                          | Height of the images requested to the camera                       | The cameras has a value cap for the width of the image that can provide, check the documentation. Zero or negative value not accepted |

**Suggested resolutions**
|resolution|carrier|fps|
|-|-|-|
|640x480|mjpeg|30|
|1024x768|mjpeg|30|
|1920x1080|mjpeg|20|

## Informations for developers

[This](./doc/dev-informations.md) page contains useful informations for developers.

Maintainers
--------------
This repository is maintained by:

| | | | |
|:---:|:---:|:---:|:---:|
| [<img src="https://github.com/Nicogene.png" width="40">](https://github.com/Nicogene) | [@Nicogene](https://github.com/Nicogene) | [<img src="https://github.com/triccyx.png" width="40">](https://github.com/triccyx) | [@triccyx](https://github.com/triccyx) |
