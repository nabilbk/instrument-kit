# SPDX-License-Identifier: MIT
# idev_core: GUI-independent C99 library shared by the LVGL and Qt front ends.

find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)

set(IDEV_SPEC_FILE "${PROJECT_SOURCE_DIR}/spec/instruments.json"
    CACHE FILEPATH "Instrument specification (JSON)")
set(_generated "${CMAKE_CURRENT_BINARY_DIR}/generated/idev_instruments_cfg.c")

add_custom_command(
    OUTPUT "${_generated}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/tools/gen_instruments.py"
            "${IDEV_SPEC_FILE}" "${_generated}"
    DEPENDS "${IDEV_SPEC_FILE}" "${PROJECT_SOURCE_DIR}/tools/gen_instruments.py"
    COMMENT "Generating instrument table from ${IDEV_SPEC_FILE}"
    VERBATIM
)

add_library(idev_core STATIC
    src/instrument.c
    src/sim.c
    "${_generated}"
)
add_library(idev::core ALIAS idev_core)

target_include_directories(idev_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(idev_core PUBLIC c_std_99)
set_target_properties(idev_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(NOT MSVC)
    target_link_libraries(idev_core PUBLIC m)
    target_compile_options(idev_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(BUILD_TESTING)
    add_executable(idev_core_tests tests/test_core.c)
    target_link_libraries(idev_core_tests PRIVATE idev::core)
    add_test(NAME idev_core_tests COMMAND idev_core_tests)

    # S1.2: the generator must reject invalid specifications.
    foreach(_bad overlap out_of_range duplicate)
        add_test(NAME spec_rejects_${_bad}
            COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/tools/gen_instruments.py"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/bad_${_bad}.json"
                    "${CMAKE_CURRENT_BINARY_DIR}/rejected_${_bad}.c")
        set_tests_properties(spec_rejects_${_bad} PROPERTIES WILL_FAIL TRUE)
    endforeach()
endif()
