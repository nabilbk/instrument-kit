# SPDX-License-Identifier: MIT
find_package(Qt6 6.8 REQUIRED COMPONENTS Test)

qt_add_executable(tst_instrument tst_instrument.cpp)
target_include_directories(tst_instrument PRIVATE ../IdevInstruments)
target_link_libraries(tst_instrument PRIVATE idev_instruments Qt6::Test)
add_test(NAME qt_tst_instrument COMMAND tst_instrument)
