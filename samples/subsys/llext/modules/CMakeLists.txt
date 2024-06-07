# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fs_shell)

if(CONFIG_HELLO_WORLD_MODE STREQUAL "m")

  # Build the llext ...

  set(ext_name hello_world)
  set(ext_src src/${ext_name}_ext.c)
  set(ext_bin ${ZEPHYR_BINARY_DIR}/${ext_name}.llext)
  set(ext_inc ${ZEPHYR_BINARY_DIR}/include/generated/${ext_name}_ext.inc)
  add_llext_target(${ext_name}_ext
    OUTPUT  ${ext_bin}
    SOURCES ${ext_src}
  )
  generate_inc_file_for_target(app ${ext_bin} ${ext_inc})

  # ...and the code for loading and running it

  target_sources(app PRIVATE
    src/main_module.c
  )

elseif(CONFIG_HELLO_WORLD_MODE STREQUAL "y")

  # Just build the two files together

  target_sources(app PRIVATE
    src/main_builtin.c
    src/hello_world_ext.c
  )

else()
  message(FATAL_ERROR "Please choose 'y' or 'm' for CONFIG_HELLO_WORLD_MODE")
endif()
