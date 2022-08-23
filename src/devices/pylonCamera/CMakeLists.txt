# Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
# All rights reserved.
#
# This software may be modified and distributed under the terms of the
# BSD-3-Clause license. See the accompanying LICENSE file for details.

yarp_prepare_plugin(pylonCamera
  CATEGORY device
  TYPE pylonCameraDriver
  INCLUDE pylonCameraDriver.h
  EXTRA_CONFIG WRAPPER=frameGrabber_nws_yarp
  DEPENDS "pylon_FOUND"
  DEFAULT ON
)

if(ENABLE_pylonCamera)
  yarp_add_plugin(yarp_pylonCamera)

  target_sources(yarp_pylonCamera
    PRIVATE
      pylonCameraDriver.cpp
      pylonCameraDriver.h
  )

  list(APPEND OPENCV_DEPS  opencv_core
                           opencv_video
                           opencv_imgproc)

  if (CUDA_FOUND AND OpenCV_CUDA_VERSION)
    target_compile_definitions(yarp_pylonCamera PUBLIC -DUSE_CUDA)
  endif()

  target_link_libraries(yarp_pylonCamera
    PUBLIC
      YARP::YARP_os
      YARP::YARP_sig
      YARP::YARP_dev
      YARP::YARP_cv
      pylon::pylon
      ${OPENCV_DEPS}
  )

  yarp_install(
    TARGETS yarp_pylonCamera
    EXPORT yarp-device-pylon
    COMPONENT yarp-device-pylon
    LIBRARY DESTINATION ${YARP_DYNAMIC_PLUGINS_INSTALL_DIR}
    ARCHIVE DESTINATION ${YARP_STATIC_PLUGINS_INSTALL_DIR}
    YARP_INI DESTINATION ${YARP_PLUGIN_MANIFESTS_INSTALL_DIR}
  )

  set_property(TARGET yarp_pylonCamera PROPERTY FOLDER "Plugins/Device")
endif()
