# SPDX-License-Identifier: MIT
qt_add_executable(idev_qt_car_cluster WIN32 main.cpp)

qt_add_qml_module(idev_qt_car_cluster
    URI CarCluster
    QML_FILES
        Main.qml
        CarDashboard.qml
    IMPORTS
        TARGET idev_instruments
        TARGET idev_demo_sim
)

target_link_libraries(idev_qt_car_cluster
    PRIVATE
        idev_instrumentsplugin
        idev_demo_simplugin
        idev_demo_sim
)

if(BUILD_TESTING)
    # Smoke test: load the demo offscreen with the software renderer and grab a frame.
    add_test(NAME idev_qt_car_cluster_grab
        COMMAND idev_qt_car_cluster --time 26 --grab "${CMAKE_CURRENT_BINARY_DIR}/test_grab.png")
    set_tests_properties(idev_qt_car_cluster_grab PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
        TIMEOUT 30)
endif()
