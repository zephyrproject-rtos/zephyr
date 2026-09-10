# SPDX-License-Identifier: Apache-2.0

# Configures binary tools as GNU binutils

find_program(CMAKE_OBJCOPY ${CROSS_COMPILE}objcopy PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_OBJDUMP ${CROSS_COMPILE}objdump PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_AS      ${CROSS_COMPILE}as      PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
if(CONFIG_LTO)
  find_program(CMAKE_AR      ${CROSS_COMPILE}gcc-ar      PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
  find_program(CMAKE_RANLIB  ${CROSS_COMPILE}gcc-ranlib  PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
else()
  find_program(CMAKE_AR      ${CROSS_COMPILE}ar      PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
  find_program(CMAKE_RANLIB  ${CROSS_COMPILE}ranlib  PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
endif()
find_program(CMAKE_READELF ${CROSS_COMPILE}readelf PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_NM      ${CROSS_COMPILE}nm      PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_STRIP   ${CROSS_COMPILE}strip   PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

find_program(CMAKE_GDB     ${CROSS_COMPILE}gdb-py  PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

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
include(${ZEPHYR_BASE}/cmake/bintools/gnu/target_bintools.cmake)
