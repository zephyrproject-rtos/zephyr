# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NEWLIB_LIBC)
  add_subdirectory(newlib)
elseif(CONFIG_ARCMWDT_LIBC)
  add_subdirectory(arcmwdt)
else()
  add_subdirectory(minimal)
endif()
