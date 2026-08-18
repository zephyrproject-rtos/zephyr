# Copyright (c) 2026 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(applet_hello_world LANGUAGES C)

set(applet_src  ${PROJECT_SOURCE_DIR}/applet/hello_world_applet.c)

if(CONFIG_APPLET_LLEXT)
  # Build the loadable extension (hello_world_ext.c -> hello_world.llext)
  # and embed it as a generated C header (hello_world.inc).
  set(applet_bin  ${PROJECT_BINARY_DIR}/llext/hello_world.llext)
  set(applet_inc  ${ZEPHYR_BINARY_DIR}/include/generated/hello_world.inc)

  add_llext_target(hello_world_ext
    OUTPUT  ${applet_bin}
    SOURCES ${applet_src}
  )
  generate_inc_file_for_target(app ${applet_bin} ${applet_inc})
else()
target_sources(app PRIVATE ${applet_src})
endif()

# Host application
target_sources(app PRIVATE src/main.c)
