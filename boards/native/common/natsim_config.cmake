# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set(zephyr_build_path ${APPLICATION_BINARY_DIR}/zephyr)

if(NOT CMAKE_HOST_APPLE)
  target_link_options(native_simulator INTERFACE
    "-T ${ZEPHYR_BASE}/boards/native/common/natsim_linker_script.ld")
endif()

if(SYSROOT_DIR)
  message(NOTICE "Appending --sysroot=${SYSROOT_DIR} to native_simulator")
  target_compile_options(native_simulator INTERFACE "--sysroot=${SYSROOT_DIR}")
  target_link_options(native_simulator INTERFACE "--sysroot=${SYSROOT_DIR}")
endif()

if(CONFIG_NATIVE_SIMULATOR_STATIC_LINKING)
  target_link_options(native_simulator INTERFACE "-static")
endif()

list(JOIN CMAKE_C_COMPILER_LAUNCHER " " launcher)

if("${LINKER}" STREQUAL "lld")
  target_link_options(native_simulator INTERFACE "-fuse-ld=lld")
endif()

if(CMAKE_HOST_APPLE)
  # Mach-O cannot sort sections by name at link time, so the native simulator is
  # handed an order file instead. Tell it how to find Zephyr's own ordered
  # symbols and which section holds the constructors it must not let dyld run.
  find_program(CMAKE_NMEDIT nmedit REQUIRED)
  list(APPEND nsi_config_content
    "NSI_NMEDIT:=${CMAKE_NMEDIT}"
    "NSI_ORDER_HELPERS:=${ZEPHYR_BASE}/scripts/build/gen_macho_order.py"
    "NSI_EMBSW_CTOR_SECTION:=zinit_array"
    "NSI_EMBSW_CTOR_ID_EXPR:=s/^___zephyr_init_array_start_\\([0-9][0-9]*\\)$$/\\1/p"
  )
endif()

set(nsi_config_content
  ${nsi_config_content}
  "NSI_AR:=${CMAKE_AR}"
  "NSI_BUILD_OPTIONS:=$<JOIN:$<TARGET_PROPERTY:native_simulator,INTERFACE_COMPILE_OPTIONS>,\ >"
  "NSI_BUILD_PATH:=${zephyr_build_path}/NSI"
  "NSI_CC:=$<$<BOOL:${launcher}>:${launcher} >${CMAKE_C_COMPILER}"
  "NSI_OBJCOPY:=${CMAKE_OBJCOPY}"
  "NSI_PYTHON:=${PYTHON_EXECUTABLE}"
  "NSI_NM:=${CMAKE_NM}"
  "NSI_EMBEDDED_CPU_SW:=${zephyr_build_path}/${KERNEL_ELF_NAME} ${CONFIG_NATIVE_SIMULATOR_EXTRA_IMAGE_PATHS}"
  "NSI_EXE:=${zephyr_build_path}/${KERNEL_EXE_NAME}"
  "NSI_EXTRA_SRCS:=$<JOIN:$<TARGET_PROPERTY:native_simulator,INTERFACE_SOURCES>,\ >"
  "NSI_LINK_OPTIONS:=$<JOIN:$<TARGET_PROPERTY:native_simulator,INTERFACE_LINK_OPTIONS>,\ >"
  "NSI_EXTRA_LIBS:=$<JOIN:$<TARGET_PROPERTY:native_simulator,RUNNER_LINK_LIBRARIES>,\ >"
  "NSI_PATH:=${NSI_DIR}/"
  "NSI_N_CPUS:=${CONFIG_NATIVE_SIMULATOR_NUMBER_MCUS}"
  "NSI_LOCALIZE_OPTIONS:=--localize-symbol=CONFIG_* $<JOIN:$<TARGET_PROPERTY:native_simulator,LOCALIZE_EXTRA_OPTIONS>,\ >"
)

string(REPLACE ";" "\n" nsi_config_content "${nsi_config_content}")

file(GENERATE OUTPUT "${zephyr_build_path}/NSI/nsi_config"
  CONTENT "${nsi_config_content}"
)
