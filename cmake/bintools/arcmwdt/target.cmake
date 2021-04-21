# SPDX-License-Identifier: Apache-2.0

# Configures binary tools as mwdt binutils

find_program(CMAKE_ELF2BIN ${CROSS_COMPILE}elf2bin   PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_OBJDUMP ${CROSS_COMPILE}elfdumpac PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_AS      ${CROSS_COMPILE}ccac      PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_AR      ${CROSS_COMPILE}arac      PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_RANLIB  ${CROSS_COMPILE}arac      PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_READELF ${CROSS_COMPILE}elfdumpac PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_NM      ${CROSS_COMPILE}nmac      PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_STRIP   ${CROSS_COMPILE}stripac   PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_SIZE    ${CROSS_COMPILE}sizeac    PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_ELF2HEX ${CROSS_COMPILE}elf2hex   PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> -rq <TARGET> <LINK_FLAGS> <OBJECTS>")
SET(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> -rq <TARGET> <LINK_FLAGS> <OBJECTS>")
SET(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_AR> -sq <TARGET>")
SET(CMAKE_C_ARCHIVE_FINISH "<CMAKE_AR> -sq <TARGET>")

find_program(CMAKE_GDB     ${CROSS_COMPILE}mdb     PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

# MWDT binutils don't support required features like section renameing, so we
# temporarily had to use GNU objcopy instead
find_program(CMAKE_OBJCOPY arc-elf32-objcopy)
if (NOT CMAKE_OBJCOPY)
  find_program(CMAKE_OBJCOPY arc-linux-objcopy)
endif()

if (NOT CMAKE_OBJCOPY)
  find_program(CMAKE_OBJCOPY objcopy)
endif()

if(NOT CMAKE_OBJCOPY)
  message(FATAL_ERROR "Zephyr unable to find any GNU objcopy (ARC or host one)")
endif()

include(${ZEPHYR_BASE}/cmake/bintools/arcmwdt/target_bintools.cmake)
