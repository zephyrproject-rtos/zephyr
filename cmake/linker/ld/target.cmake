# SPDX-License-Identifier: Apache-2.0

find_program(CMAKE_LINKER     ${CROSS_COMPILE}ld      PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

set_ifndef(LINKERFLAGPREFIX -Wl)

# Load toolchain_ld-family macros
include(${ZEPHYR_BASE}/cmake/linker/${LINKER}/target_base.cmake)
