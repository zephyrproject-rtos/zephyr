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

set(IAR_LINK_FLAGS --no-wrap-diagnostics)

if(CONFIG_IAR_DATA_INIT)
  string(APPEND IAR_LINK_FLAGS " --redirect arch_data_copy=__iar_data_init3")
endif()

if(CONFIG_STATIC_INIT_IAR)
  # Zephyr calls __iar_dynamic_initialization() itself, see kernel/init.c
  string(APPEND IAR_LINK_FLAGS " --manual_dynamic_initialization")
endif()

# The link language is CXX when the application contains C++ sources
foreach(lang C CXX)
  string(APPEND CMAKE_${lang}_LINK_FLAGS "${IAR_LINK_FLAGS}")
endforeach()

foreach(lang C CXX ASM)
  set(commands "--log modules,libraries,initialization,redirects,sections")
  set(CMAKE_${lang}_LINK_EXECUTABLE
  "<CMAKE_LINKER> <CMAKE_${lang}_LINK_FLAGS> <LINK_FLAGS> ${commands} <LINK_LIBRARIES> <OBJECTS> -o <TARGET>")
  set(commands)
endforeach()
