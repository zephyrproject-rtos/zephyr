# SPDX-License-Identifier: Apache-2.0

# Configures binary tools as host GNU binutils

find_program(CMAKE_OBJCOPY objcopy)
find_program(CMAKE_OBJDUMP objdump)
find_program(CMAKE_AR      ar     )
find_program(CMAKE_RANLIB  ranlib )
find_program(CMAKE_READELF readelf)
if(CMAKE_HOST_APPLE)
  if(NOT CMAKE_READELF)
    find_program(CMAKE_READELF otool)
  endif()
  if(CMAKE_READELF MATCHES "otool$")
    set(HOST_GNU_READELF_IS_OTOOL TRUE)
  endif()
endif()
find_program(CMAKE_NM      nm)
find_program(CMAKE_STRIP   strip)

find_program(CMAKE_GDB     gdb    )

# Include bin tool properties
include(${ZEPHYR_BASE}/cmake/bintools/gnu/target_bintools.cmake)
