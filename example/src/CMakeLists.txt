################################################################################
#                                                                              #
# Copyright (C) 2022 Fondazione Istitito Italiano di Tecnologia (IIT)          #
# All Rights Reserved.                                                         #
#                                                                              #
################################################################################

# @author Luca Tricerri <luca.tricerri@iit.it> 

set(grabexampleSrc  Grab.cpp
                        )
set(grabexampleHdr   ${BlockTestRunnerHdr}      
                        )

add_executable(grabexample ${grabexampleSrc} ${grabexampleHdr} )

target_link_libraries(grabexample pylon::pylon)

# install
install(TARGETS grabexample RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
