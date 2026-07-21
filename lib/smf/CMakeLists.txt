# SPDX-License-Identifier: Apache-2.0

zephyr_interface_library_named(smf)

target_include_directories(smf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library()

zephyr_library_sources(smf.c)

zephyr_library_link_libraries(smf)
