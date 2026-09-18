# SPDX-License-Identifier: Apache-2.0

# Configures binary tools as the native Darwin tools.

find_program(CMAKE_AR      ar     )
find_program(CMAKE_RANLIB  ranlib )
find_program(CMAKE_READELF otool  REQUIRED)
find_program(CMAKE_NM      nm     )
find_program(CMAKE_STRIP   strip  )
find_program(CMAKE_OBJDUMP objdump)

# Use the common properties for the tools with GNU compatible command semantics.
include(${ZEPHYR_BASE}/cmake/bintools/gnu/target_bintools.cmake)

# otool takes neither readelf's options nor its output format. Only the Mach-O
# header and load command dump has an equivalent, the rest stays unset so a user
# of those properties fails loudly instead of parsing the wrong output.
set_property(TARGET bintools PROPERTY readelf_flag_headers -hvl)
