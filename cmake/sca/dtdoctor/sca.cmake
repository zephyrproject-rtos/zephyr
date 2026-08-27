# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors

include(python)
include(kconfig)

if(NOT "${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "zephyr")
  message(WARNING "dtdoctor has only been tested with the Zephyr SDK toolchain.")
endif()

set(dtdoctor_sca_wrapper_script ${ZEPHYR_BASE}/scripts/dts/dtdoctor_sca_wrapper.py)
set(edt_pickle ${CMAKE_BINARY_DIR}/zephyr/edt.pickle)

# Note that Kconfig settings should not be manipulated/overridden in CMake but until
# https://github.com/zephyrproject-rtos/zephyr/issues/96199 is addressed the below has been
# accepted since it shortens the typically long Devicetree errors that DT Doctor is helping make
# sense of anyway.
set(CONFIG_COMPILER_TRACK_MACRO_EXPANSION n)

set(dtdoctor_wrapper_cmd
  ${CMAKE_COMMAND} -E env
  ${COMMON_KCONFIG_ENV_SETTINGS}
  ${PYTHON_EXECUTABLE}
  ${dtdoctor_sca_wrapper_script}
  --edt-pickle ${edt_pickle}
  --
)

set(CMAKE_C_COMPILER_LAUNCHER   ${dtdoctor_wrapper_cmd} CACHE INTERNAL "")
set(CMAKE_CXX_COMPILER_LAUNCHER ${dtdoctor_wrapper_cmd} CACHE INTERNAL "")
set(CMAKE_ASM_COMPILER_LAUNCHER ${dtdoctor_wrapper_cmd} CACHE INTERNAL "")

set(CMAKE_C_LINKER_LAUNCHER   ${dtdoctor_wrapper_cmd} CACHE INTERNAL "")
set(CMAKE_CXX_LINKER_LAUNCHER ${dtdoctor_wrapper_cmd} CACHE INTERNAL "")
set(CMAKE_ASM_LINKER_LAUNCHER ${dtdoctor_wrapper_cmd} CACHE INTERNAL "")
