# SPDX-License-Identifier: MIT
qt_add_library(idev_instruments STATIC)

qt_add_qml_module(idev_instruments
    URI IdevInstruments
    DEPENDENCIES
        QtQuick
    SOURCES
        instrument.h instrument.cpp
        instrumentpalette.h
        annunciatorstate.h annunciatorstate.cpp
    QML_FILES
        RoundGauge.qml
        LinearTape.qml
        AttitudeIndicator.qml
        AircraftSymbol.qml
        Annunciator.qml
)

target_link_libraries(idev_instruments
    PUBLIC
        Qt6::Quick
        idev::core
)
if(NOT MSVC)
    target_compile_options(idev_instruments PRIVATE -Wall -Wextra -Werror)
endif()
