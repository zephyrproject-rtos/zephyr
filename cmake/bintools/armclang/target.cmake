# SPDX-License-Identifier: Apache-2.0

# Configures binary tools as mwdt binutils

find_program(CMAKE_FROMELF fromelf  PATH ${TOOLCHAIN_HOME}/bin NO_DEFAULT_PATH)
find_program(CMAKE_AS      armasm    PATH ${TOOLCHAIN_HOME}/bin NO_DEFAULT_PATH)
find_program(CMAKE_AR      armar     PATH ${TOOLCHAIN_HOME}/bin NO_DEFAULT_PATH)

SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> -rq <TARGET> <LINK_FLAGS> <OBJECTS>")
SET(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> -rq <TARGET> <LINK_FLAGS> <OBJECTS>")
SET(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_AR> -sq <TARGET>")
SET(CMAKE_C_ARCHIVE_FINISH "<CMAKE_AR> -sq <TARGET>")

find_program(CMAKE_GDB     ${CROSS_COMPILE}mdb     PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

include(${CMAKE_CURRENT_LIST_DIR}/target_bintools.cmake)
