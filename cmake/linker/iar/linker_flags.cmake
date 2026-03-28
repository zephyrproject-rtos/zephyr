# Copyright (c) 2025 IAR Systems AB
#
# SPDX-License-Identifier: Apache-2.0

# Override the default CMake's IAR ILINK linker signature

# IAR linker doesn't support dedicated linker optimization flags.
set_property(TARGET linker PROPERTY no_optimization "")
set_property(TARGET linker PROPERTY optimization_debug "")
set_property(TARGET linker PROPERTY optimization_speed "")
set_property(TARGET linker PROPERTY optimization_size "")
set_property(TARGET linker PROPERTY optimization_size_aggressive "")

set_linker_property(TARGET linker PROPERTY undefined "--keep=")

string(APPEND CMAKE_C_LINK_FLAGS --no-wrap-diagnostics)

if(CONFIG_IAR_DATA_INIT)
  string(APPEND CMAKE_C_LINK_FLAGS " --redirect arch_data_copy=__iar_data_init3")
endif()
foreach(lang C CXX ASM)
  set(commands "--log modules,libraries,initialization,redirects,sections")
  set(CMAKE_${lang}_LINK_EXECUTABLE
  "<CMAKE_LINKER> <CMAKE_${lang}_LINK_FLAGS> <LINK_FLAGS> ${commands} <LINK_LIBRARIES> <OBJECTS> -o <TARGET>")
  set(commands)
endforeach()
