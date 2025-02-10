# Copyright (c) 2023 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(llext_test)

target_sources(app PRIVATE
  src/test_llext.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/include
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
)

set(ext_names
	hello_world
	logging
	relative_jump
	object
	syscalls
	inspect
	threads_kernel_objects
	export_dependent
	export_dependency
)

if(CONFIG_ARM)
  if(NOT CONFIG_CPU_CORTEX_M0 AND NOT CONFIG_CPU_CORTEX_M0PLUS AND NOT CONFIG_CPU_CORTEX_M1)
    list(APPEND ext_names movwmovt)
  endif()
endif()

if (CONFIG_LLEXT_TYPE_ELF_RELOCATABLE AND NOT CONFIG_ARM)
    list(APPEND ext_names pre_located)
endif()

if (CONFIG_LLEXT_STORAGE_WRITABLE)
    list(APPEND ext_names find_section detached_fn)
endif()

if (NOT CONFIG_LLEXT_TYPE_ELF_SHAREDLIB)
    # ELF shared libraries do not support init sections
    list(APPEND ext_names init_fini)
endif()

# generate extension targets for each extension in 'ext_names'
foreach(ext_name ${ext_names})
  set(ext_src ${PROJECT_SOURCE_DIR}/src/${ext_name}_ext.c)
  set(ext_bin ${PROJECT_BINARY_DIR}/llext/${ext_name}.llext)
  set(ext_inc ${ZEPHYR_BINARY_DIR}/include/generated/${ext_name}.inc)
  add_llext_target(${ext_name}_ext
    OUTPUT  ${ext_bin}
    SOURCES ${ext_src}
  )
  generate_inc_file_for_target(app ${ext_bin} ${ext_inc})
endforeach()

if(NOT CONFIG_LLEXT_TYPE_ELF_OBJECT)
  add_llext_target(multi_file_ext
    OUTPUT  ${PROJECT_BINARY_DIR}/llext/multi_file.llext
    SOURCES ${PROJECT_SOURCE_DIR}/src/multi_file_ext1.c ${PROJECT_SOURCE_DIR}/src/multi_file_ext2.c
  )
  generate_inc_file_for_target(app ${PROJECT_BINARY_DIR}/llext/multi_file.llext
    ${ZEPHYR_BINARY_DIR}/include/generated/multi_file.inc
  )
endif()

if (CONFIG_LLEXT_TYPE_ELF_RELOCATABLE AND NOT CONFIG_ARM AND NOT CONFIG_RISCV)
  # Manually fix the pre_located extension's text address at a multiple of 4
  get_target_property(pre_located_target pre_located_ext lib_target)
  get_target_property(pre_located_file pre_located_ext pkg_input)
  add_llext_command(
    TARGET pre_located_ext
    POST_BUILD
    COMMAND ${CMAKE_C_COMPILER}
    ${LLEXT_APPEND_FLAGS}
    -Wl,-r -Wl,-Ttext=0xbada110c -nostdlib -nodefaultlibs -nostartfiles
    $<TARGET_OBJECTS:${pre_located_target}> -o ${pre_located_file}
  )
endif()

if(NOT CONFIG_LLEXT_TYPE_ELF_OBJECT AND CONFIG_RISCV AND CONFIG_RISCV_ISA_EXT_C)
  add_llext_target(riscv_edge_case_cb_type_ext
    OUTPUT  ${PROJECT_BINARY_DIR}/llext/riscv_edge_case_cb_type_ext.llext
    SOURCES ${PROJECT_SOURCE_DIR}/src/riscv_edge_case_cb_type.c ${PROJECT_SOURCE_DIR}/src/riscv_edge_case_cb_type_trigger.S
  )
  generate_inc_file_for_target(app ${PROJECT_BINARY_DIR}/llext/riscv_edge_case_cb_type_ext.llext
    ${ZEPHYR_BINARY_DIR}/include/generated/riscv_edge_case_cb_type.inc
  )
endif()
