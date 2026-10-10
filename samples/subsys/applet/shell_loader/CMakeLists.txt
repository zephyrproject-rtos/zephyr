# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(applet_shell_loader LANGUAGES C)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE "${PROJECT_SOURCE_DIR}")

# Add a target for each applet extension shipped with the sample. hello_world
# is standalone; ping and pong talk to each other through the shared link
# declared in applet_link.h.
foreach(ext hello_world ping pong)
  add_llext_target(${ext}_ext
    OUTPUT  "${PROJECT_BINARY_DIR}/${ext}.llext"
    SOURCES "${PROJECT_SOURCE_DIR}/applet/${ext}_applet.c"
  )
  llext_include_directories(${ext}_ext "${PROJECT_SOURCE_DIR}")
  # llext targets do not inherit the optimization level of the image. Without
  # one, k_msleep() keeps a call to the 64-bit division helper of libgcc, which
  # the extension loader cannot resolve.
  llext_compile_options(${ext}_ext $<TARGET_PROPERTY:compiler,optimization_size>)
endforeach()
