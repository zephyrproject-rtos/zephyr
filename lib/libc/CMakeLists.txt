# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NEWLIB_LIBC)
  add_subdirectory(newlib)
elseif(CONFIG_ARCMWDT_LIBC)
  add_subdirectory(arcmwdt)
elseif(CONFIG_MINIMAL_LIBC)
  add_subdirectory(minimal)
elseif(CONFIG_ARMCLANG_STD_LIBC)
  add_subdirectory(armstdc)
elseif(CONFIG_PICOLIBC)
  add_subdirectory(picolibc)
endif()
