# Copyright (c) 2025 IAR Systems AB
#
# SPDX-License-Identifier: Apache-2.0

# Override the default CMake's IAR ILINK linker signature

string(APPEND CMAKE_C_LINK_FLAGS --no-wrap-diagnostics)

if(CONFIG_IAR_DATA_INIT)
  string(APPEND CMAKE_C_LINK_FLAGS " --redirect z_data_copy=__iar_data_init3")
endif()
foreach(lang C CXX ASM)
  set(commands "--log modules,libraries,initialization,redirects,sections")
  set(CMAKE_${lang}_LINK_EXECUTABLE
  "<CMAKE_LINKER> <CMAKE_${lang}_LINK_FLAGS> <LINK_FLAGS> ${commands} <LINK_LIBRARIES> <OBJECTS> -o <TARGET>")
  set(commands)
endforeach()
