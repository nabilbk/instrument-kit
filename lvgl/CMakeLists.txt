# SPDX-License-Identifier: MIT
# LVGL 9 front end: widgets library, demos and PC hosts (headless PNG, optional SDL).

set(IDEV_LVGL_DIR "${PROJECT_SOURCE_DIR}/third_party/lvgl" CACHE PATH
    "LVGL v9 source tree (git clone -b v9.2.2 https://github.com/lvgl/lvgl.git)")
option(IDEV_LVGL_SDL "Build the interactive SDL2 simulator" OFF)
option(IDEV_LVGL_HEADLESS "Build the headless PNG renderer (needs libpng)" ON)

if(NOT EXISTS "${IDEV_LVGL_DIR}/lvgl.h")
    message(FATAL_ERROR "LVGL not found in ${IDEV_LVGL_DIR}. Clone it with:\n"
        "  git clone --depth 1 -b v9.2.2 https://github.com/lvgl/lvgl.git third_party/lvgl")
endif()

# LVGL's own CMake: point it at our lv_conf.h and skip what we do not use.
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/config/lv_conf.h" CACHE STRING "" FORCE)
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)
add_subdirectory("${IDEV_LVGL_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/lvgl" EXCLUDE_FROM_ALL)
target_compile_definitions(lvgl PUBLIC IDEV_LVGL_SDL=$<BOOL:${IDEV_LVGL_SDL}>)

add_library(idev_lvgl STATIC
    src/idev_lv_common.c
    src/idev_round_gauge.c
    src/idev_linear_tape.c
    src/idev_attitude.c
    src/idev_annunciator.c
)
add_library(idev::lvgl ALIAS idev_lvgl)
target_include_directories(idev_lvgl
    PUBLIC  include
    PRIVATE src
)
target_link_libraries(idev_lvgl PUBLIC idev::core lvgl::lvgl)
if(NOT MSVC)
    target_compile_options(idev_lvgl PRIVATE -Wall -Wextra -Werror)
endif()

add_library(idev_lvgl_demos STATIC
    demos/car_cluster.c
    demos/cockpit_pfd.c
)
target_include_directories(idev_lvgl_demos PUBLIC demos)
target_link_libraries(idev_lvgl_demos PUBLIC idev::lvgl)
if(NOT MSVC)
    target_compile_options(idev_lvgl_demos PRIVATE -Wall -Wextra -Werror)
endif()

if(IDEV_LVGL_HEADLESS)
    find_package(PNG REQUIRED)
    add_executable(idev_lvgl_headless platform/headless_main.c)
    target_compile_definitions(idev_lvgl_headless PRIVATE _POSIX_C_SOURCE=200809L _ISOC11_SOURCE)
    target_compile_features(idev_lvgl_headless PRIVATE c_std_11)
    target_link_libraries(idev_lvgl_headless PRIVATE idev_lvgl_demos PNG::PNG)

    if(BUILD_TESTING)
        foreach(_demo car pfd)
            add_test(NAME lvgl_render_${_demo}
                COMMAND idev_lvgl_headless --demo ${_demo} --png "${CMAKE_CURRENT_BINARY_DIR}/test_${_demo}.png")
        endforeach()
        add_test(NAME lvgl_render_invalid COMMAND idev_lvgl_headless --demo pfd --invalid)
        add_test(NAME lvgl_memreport COMMAND idev_lvgl_headless --memreport)
        add_test(NAME lvgl_selftest COMMAND idev_lvgl_headless --selftest)
    endif()
endif()

if(IDEV_LVGL_SDL)
    find_package(SDL2 REQUIRED)
    target_link_libraries(lvgl PUBLIC SDL2::SDL2)
    add_executable(idev_lvgl_sdl platform/sdl_main.c)
    target_link_libraries(idev_lvgl_sdl PRIVATE idev_lvgl_demos SDL2::SDL2)
endif()
