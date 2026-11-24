# SPDX-License-Identifier: MIT
# Demo helpers: scenario generators (wrapping idev_sim_*) and the shared main().
qt_add_library(idev_demo_sim STATIC)

qt_add_qml_module(idev_demo_sim
    URI IdevDemoSim
    DEPENDENCIES
        QtQuick
    SOURCES
        simulators.h simulators.cpp
        demorunner.h demorunner.cpp
)

target_include_directories(idev_demo_sim PUBLIC .)
target_link_libraries(idev_demo_sim
    PUBLIC
        Qt6::Quick
        idev::core
)
if(NOT MSVC)
    target_compile_options(idev_demo_sim PRIVATE -Wall -Wextra -Werror)
endif()
