################################################################################
#                                                                              #
# Copyright (C) 2019 Fondazione Istitito Italiano di Tecnologia (IIT)          #
# All Rights Reserved.                                                         #
#                                                                              #
################################################################################

# @author Nicolo Genesio <nicolo.genesio@iit.it> 

set(grabexampleSrc  Grab.cpp
                        )
set(grabexampleHdr   ${BlockTestRunnerHdr}      
                        )

add_executable(grabexample ${grabexampleSrc} ${grabexampleHdr} )

target_link_libraries(grabexample pylon::pylon)

# install
install(TARGETS grabexample RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
