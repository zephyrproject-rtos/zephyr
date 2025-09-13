# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors

include(python)
include(kconfig)

if(NOT "${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "zephyr")
  message(WARNING "dtdoctor has only been tested with the Zephyr SDK toolchain.")
endif()

set(_DT_DIAG_WRAP ${ZEPHYR_BASE}/scripts/utils/dtdoctor_sca_wrapper.py)

set(_EDT_PICKLE ${CMAKE_BINARY_DIR}/zephyr/edt.pickle)

# Compiler messages for devicetree errors are unnecessarily long, especially if DT Doctor is going
# to help diagnose the error anyway
set(CONFIG_COMPILER_TRACK_MACRO_EXPANSION n)

set(_WRAP
  ${CMAKE_COMMAND} -E env
  ${COMMON_KCONFIG_ENV_SETTINGS}
  ${PYTHON_EXECUTABLE}
  ${_DT_DIAG_WRAP}
  --edt-pickle ${_EDT_PICKLE}
  --
)

set(CMAKE_C_COMPILER_LAUNCHER   ${_WRAP} CACHE INTERNAL "")
set(CMAKE_CXX_COMPILER_LAUNCHER ${_WRAP} CACHE INTERNAL "")
set(CMAKE_ASM_COMPILER_LAUNCHER ${_WRAP} CACHE INTERNAL "")

set(CMAKE_C_LINKER_LAUNCHER ${_WRAP} CACHE INTERNAL "")
set(CMAKE_CXX_LINKER_LAUNCHER ${_WRAP} CACHE INTERNAL "")
set(CMAKE_ASM_LINKER_LAUNCHER ${_WRAP} CACHE INTERNAL "")
