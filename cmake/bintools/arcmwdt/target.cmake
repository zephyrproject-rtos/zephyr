# SPDX-License-Identifier: Apache-2.0

# Configures binary tools as mwdt binutils

find_program(CMAKE_ELF2BIN ${CROSS_COMPILE}elf2bin   PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_OBJDUMP ${CROSS_COMPILE}elfdumpac PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_AS      ${CROSS_COMPILE}ccac      PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_AR      ${CROSS_COMPILE}arac      PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_RANLIB  ${CROSS_COMPILE}arac      PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_READELF ${CROSS_COMPILE}elfdumpac PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_NM      ${CROSS_COMPILE}nmac      PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_STRIP   ${CROSS_COMPILE}stripac   PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_SIZE    ${CROSS_COMPILE}sizeac    PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_ELF2HEX ${CROSS_COMPILE}elf2hex   PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> -rq <TARGET> <LINK_FLAGS> <OBJECTS>")
SET(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> -rq <TARGET> <LINK_FLAGS> <OBJECTS>")
SET(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_AR> -sq <TARGET>")
SET(CMAKE_C_ARCHIVE_FINISH "<CMAKE_AR> -sq <TARGET>")

find_program(CMAKE_GDB     ${CROSS_COMPILE}mdb     PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

function(zephyr_sdk_target_cmake)
  # Scoped loading of variables set by Zephyr SDK target.cmake.
  include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/target.cmake)

  # MWDT binutils don't support required features like section renaming, so we
  # temporarily had to use GNU objcopy instead
  find_program(CMAKE_OBJCOPY ${CROSS_COMPILE}objcopy PATHS ${ZEPHYR_SDK_INSTALL_DIR} NO_DEFAULT_PATH)
  message(STATUS "Found GNU objcopy helper for MWDT: ${CMAKE_OBJCOPY} (Zephyr SDK ${SDK_VERSION})")
endfunction()

zephyr_sdk_target_cmake()

include(${ZEPHYR_BASE}/cmake/bintools/arcmwdt/target_bintools.cmake)
