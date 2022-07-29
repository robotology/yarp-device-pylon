################################################################################
#                                                                              #
# Copyright (C) 2019 Fondazione Istitito Italiano di Tecnologia (IIT)          #
# All Rights Reserved.                                                         #
#                                                                              #
################################################################################

# @author Luca Tricerri <luca.tricerri@iit.it>

cmake_minimum_required(VERSION 3.5)

project(grabexample
        LANGUAGES C CXX
        VERSION 2.3.3)

set(CMAKE_AUTOMOC ON)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
if (CMAKE_VERSION VERSION_LESS 3.11)
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake-3.11)
endif()

include(GNUInstallDirs)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Build position independent code.
# Position Independent Code (PIC) is commonly used for shared libraries so that
# the same shared library code can be loaded in each program address space in a
# location where it will not overlap with any other uses of such memory.
# In particular, this option avoids problems occurring when a process wants to
# load more than one shared library at the same virtual address.
# Since shared libraries cannot predict where other shared libraries could be
# loaded, this is an unavoidable problem with the traditional shared library
# concept.
# Generating position-independent code is often the default behavior for most
# modern compilers.
# Moreover linking a static library that is not built with PIC from a shared
# library will fail on some compiler/architecture combinations.
# Further details on PIC can be found here:
# https://eli.thegreenplace.net/2011/11/03/position-independent-code-pic-in-shared-libraries/
set(CMAKE_POSITION_INDEPENDENT_CODE ON)



set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")


set(Common_LIBRARIES dl pthread)

set(CMAKE_FIND_DEBUG_MODE ON)
find_package(pylon 7.1.0 REQUIRED)

# Enable RPATH support for installed binaries and libraries
#include(AddInstallRPATHSupport)
#add_install_rpath_support(BIN_DIRS "${CMAKE_INSTALL_FULL_BINDIR}"
#                          LIB_DIRS "${CMAKE_INSTALL_FULL_LIBDIR}"
#                          INSTALL_NAME_DIR "${CMAKE_INSTALL_FULL_LIBDIR}"
#                          USE_LINK_PATH)

add_subdirectory(src)

