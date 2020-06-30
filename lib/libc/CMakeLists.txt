# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PICOLIBC)
  add_subdirectory(picolibc)
else()
if(CONFIG_NEWLIB_LIBC)
  add_subdirectory(newlib)
else()
  add_subdirectory(minimal)
endif()
endif()
