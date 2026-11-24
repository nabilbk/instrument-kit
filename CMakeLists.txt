# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.21)

project(IdevInstrumentKit
    VERSION 0.1.0
    DESCRIPTION "Instrument widgets with one data model for Qt Quick and LVGL"
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(IDEV_BUILD_QT "Build the Qt Quick module and demos (needs Qt 6.8+)" OFF)
option(IDEV_BUILD_LVGL "Build the LVGL widgets and demos" ON)
include(CTest)

if(IDEV_BUILD_QT)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Gui Qml Quick)
    qt_standard_project_setup(REQUIRES 6.8)
endif()

add_subdirectory(core)

if(IDEV_BUILD_LVGL)
    add_subdirectory(lvgl)
endif()

if(IDEV_BUILD_QT)
    add_subdirectory(qml)
endif()
