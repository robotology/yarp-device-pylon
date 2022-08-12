# Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
# All rights reserved.
#
# This software may be modified and distributed under the terms of the
# BSD-3-Clause license. See the accompanying LICENSE file for details.

yarp_prepare_plugin(pylonCamera
  CATEGORY device
  TYPE pylonDriver
  INCLUDE pylonDriver.h
  EXTRA_CONFIG WRAPPER=frameGrabber_nws_yarp
  DEPENDS "pylon_FOUND"
  DEFAULT ON
)

if(ENABLE_pylon)
  yarp_add_plugin(yarp_pylon)

  target_sources(yarp_pylon
    PRIVATE
      pylonDriver.cpp
      pylonDriver.h
  )

  target_link_libraries(yarp_pylon
    PUBLIC
      YARP::YARP_os
      YARP::YARP_sig
      YARP::YARP_dev
      pylon::pylon
  )

  yarp_install(
    TARGETS yarp_pylon
    EXPORT yarp-device-pylon
    COMPONENT yarp-device-pylon
    LIBRARY DESTINATION ${YARP_DYNAMIC_PLUGINS_INSTALL_DIR}
    ARCHIVE DESTINATION ${YARP_STATIC_PLUGINS_INSTALL_DIR}
    YARP_INI DESTINATION ${YARP_PLUGIN_MANIFESTS_INSTALL_DIR}
  )

  set_property(TARGET yarp_pylon PROPERTY FOLDER "Plugins/Device")
endif()
