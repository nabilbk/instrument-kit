# SPDX-License-Identifier: MIT
# Qt Quick front end: the IdevInstruments QML module and the two demos.

# All QML modules land under one import root that mirrors their URIs, so qmllint,
# the QML language server and the runtime find them regardless of source layout.
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_modules")

add_subdirectory(IdevInstruments)
add_subdirectory(demos/common)
add_subdirectory(demos/car-cluster)
add_subdirectory(demos/cockpit-pfd)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# The demos land in the same build directory. Deploy once, after they are
# linked, so parallel windeployqt runs do not write that directory together.
if(WIN32 AND TARGET Qt6::windeployqt)
    set(_idev_qt_apps idev_qt_car_cluster idev_qt_cockpit_pfd)
    if(TARGET tst_instrument)
        list(APPEND _idev_qt_apps tst_instrument)
    endif()
    set(_idev_qt_deploy_stamp "${CMAKE_BINARY_DIR}/.qt-runtime.stamp")
    set(_idev_qt_deploy_cmd
        "$<TARGET_FILE:Qt6::windeployqt>"
        "$<IF:$<CONFIG:Debug>,--debug,--release>"
        --no-translations
        --qmldir "${CMAKE_SOURCE_DIR}/qml")
    set(_idev_test_deploy)
    if(TARGET tst_instrument)
        set(_idev_test_deploy COMMAND ${_idev_qt_deploy_cmd} "$<TARGET_FILE:tst_instrument>")
    endif()

    set(_idev_offscreen_plugin "$<TARGET_FILE_DIR:Qt6::windeployqt>/../plugins/platforms/$<IF:$<CONFIG:Debug>,qoffscreend,qoffscreen>.dll")
    add_custom_command(
        OUTPUT "${_idev_qt_deploy_stamp}"
        COMMAND ${_idev_qt_deploy_cmd} "$<TARGET_FILE:idev_qt_cockpit_pfd>"
        ${_idev_test_deploy}
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:idev_qt_cockpit_pfd>/platforms"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${_idev_offscreen_plugin}"
            "$<TARGET_FILE_DIR:idev_qt_cockpit_pfd>/platforms/"
        COMMAND "${CMAKE_COMMAND}" -E touch "${_idev_qt_deploy_stamp}"
        DEPENDS ${_idev_qt_apps}
        COMMENT "Deploying Qt runtime next to the demos"
        VERBATIM
    )
    add_custom_target(idev_qt_runtime ALL DEPENDS "${_idev_qt_deploy_stamp}")
endif()
