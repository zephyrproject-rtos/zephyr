# Copyright (c) 2023 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(llext_simple_test)

target_sources(app PRIVATE
  src/test_llext_simple.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/include
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
)

set(ext_names hello_world logging relative_jump object syscalls threads_kernel_objects)

if(CONFIG_ARM)
  if(NOT CONFIG_CPU_CORTEX_M0 AND NOT CONFIG_CPU_CORTEX_M0PLUS AND NOT CONFIG_CPU_CORTEX_M1)
    list(APPEND ext_names movwmovt)
  endif()
endif()

# generate extension targets foreach extension given by 'ext_names'
foreach(ext_name ${ext_names})
  set(ext_src ${PROJECT_SOURCE_DIR}/src/${ext_name}_ext.c)
  set(ext_bin ${ZEPHYR_BINARY_DIR}/${ext_name}.llext)
  set(ext_inc ${ZEPHYR_BINARY_DIR}/include/generated/${ext_name}.inc)
  add_llext_target(${ext_name}_ext
    OUTPUT  ${ext_bin}
    SOURCES ${ext_src}
  )
  generate_inc_file_for_target(app ${ext_bin} ${ext_inc})
endforeach()

if(NOT CONFIG_LLEXT_TYPE_ELF_OBJECT)
  add_llext_target(multi_file_ext
    OUTPUT  ${ZEPHYR_BINARY_DIR}/multi_file.llext
    SOURCES ${PROJECT_SOURCE_DIR}/src/multi_file_ext1.c ${PROJECT_SOURCE_DIR}/src/multi_file_ext2.c
  )
  generate_inc_file_for_target(app ${ZEPHYR_BINARY_DIR}/multi_file.llext
    ${ZEPHYR_BINARY_DIR}/include/generated/multi_file.inc
  )
endif()

# Add a dummy custom processing command to test add_llext_command
get_target_property(proc_in_file hello_world_ext lib_output)
get_target_property(proc_out_file hello_world_ext pkg_input)
add_llext_command(
  TARGET hello_world_ext
  POST_BUILD
  COMMAND echo "dummy patching ${proc_in_file} to create ${proc_out_file}"
  COMMAND ${CMAKE_COMMAND} -E copy ${proc_in_file} ${proc_out_file}
)
