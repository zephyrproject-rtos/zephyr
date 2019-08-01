# SPDX-License-Identifier: Apache-2.0

# Configures binary tools as host GNU binutils

find_program(CMAKE_OBJCOPY objcopy)
find_program(CMAKE_OBJDUMP objdump)
find_program(CMAKE_AR      ar     )
find_program(CMAKE_RANLILB ranlib )
find_program(CMAKE_READELF readelf)

find_program(CMAKE_GDB     gdb    )

# Use the gnu binutil abstraction macros
include(${ZEPHYR_BASE}/cmake/bintools/gnu/target_memusage.cmake)
include(${ZEPHYR_BASE}/cmake/bintools/gnu/target_objcopy.cmake)
include(${ZEPHYR_BASE}/cmake/bintools/gnu/target_objdump.cmake)
include(${ZEPHYR_BASE}/cmake/bintools/gnu/target_readelf.cmake)
include(${ZEPHYR_BASE}/cmake/bintools/gnu/target_strip.cmake)
