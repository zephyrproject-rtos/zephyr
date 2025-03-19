# Copyright (c) 2025 IAR Systems AB
#
# SPDX-License-Identifier: Apache-2.0

# Configures binary tools as GNU binutils
include(extensions)

# Specifically choose arm-zephyr-eabi from the zephyr sdk for objcopy and friends

if("${IAR_TOOLCHAIN_VARIANT}" STREQUAL "iccarm")
  set(IAR_ZEPHYR_HOME ${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin)
  set(IAR_GNU_PREFIX arm-zephyr-eabi-)
else()
  message(ERROR "IAR_TOOLCHAIN_VARIANT not set")
endif()
find_program(CMAKE_OBJCOPY ${IAR_GNU_PREFIX}objcopy PATHS ${IAR_ZEPHYR_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_OBJDUMP ${IAR_GNU_PREFIX}objdump PATHS ${IAR_ZEPHYR_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_AS      ${IAR_GNU_PREFIX}as      PATHS ${IAR_ZEPHYR_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_AR      ${IAR_GNU_PREFIX}ar      PATHS ${IAR_ZEPHYR_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_RANLIB  ${IAR_GNU_PREFIX}ranlib  PATHS ${IAR_ZEPHYR_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_READELF ${IAR_GNU_PREFIX}readelf PATHS ${IAR_ZEPHYR_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_NM      ${IAR_GNU_PREFIX}nm      PATHS ${IAR_ZEPHYR_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_STRIP   ${IAR_GNU_PREFIX}strip   PATHS ${IAR_ZEPHYR_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_GDB     ${IAR_GNU_PREFIX}gdb-py  PATHS ${IAR_ZEPHYR_HOME} NO_DEFAULT_PATH)

if(CMAKE_GDB)
  execute_process(
    COMMAND ${CMAKE_GDB} --configuration
    RESULTS_VARIABLE GDB_CFG_ERR
    OUTPUT_QUIET
    ERROR_QUIET
    )
endif()

if(NOT CMAKE_GDB OR GDB_CFG_ERR)
  find_program(CMAKE_GDB_NO_PY ${CROSS_COMPILE}gdb PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

  if(CMAKE_GDB_NO_PY)
    set(CMAKE_GDB ${CMAKE_GDB_NO_PY} CACHE FILEPATH "Path to a program." FORCE)
  endif()
endif()

# Include bin tool properties
include(${ZEPHYR_BASE}/cmake/bintools/iar/target_bintools.cmake)
