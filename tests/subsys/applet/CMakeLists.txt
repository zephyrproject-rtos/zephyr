# Copyright (c) 2026 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(applet_test)

target_sources(app PRIVATE
  src/test_applet.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/include
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
)

set(applet_names
  cpu_pinning
)

foreach(applet_name ${applet_names})
if(CONFIG_APPLET_LLEXT)
  set(applet_src ${PROJECT_SOURCE_DIR}/applet/${applet_name}_applet.c)
  set(applet_bin ${PROJECT_BINARY_DIR}/llext/${applet_name}.llext)
  set(applet_inc ${ZEPHYR_BINARY_DIR}/include/generated/${applet_name}.inc)
  add_llext_target(${applet_name}_applet
    OUTPUT  ${applet_bin}
    SOURCES ${applet_src}
  )
  generate_inc_file_for_target(app ${applet_bin} ${applet_inc})
else()
target_sources(app PRIVATE ${PROJECT_SOURCE_DIR}/applet/${applet_name}_applet.c)
endif()
endforeach()
