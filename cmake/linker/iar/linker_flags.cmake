# Copyright (c) 2025 IAR Systems AB
#
# SPDX-License-Identifier: Apache-2.0

# Override the default CMake's IAR ILINK linker signature

# The default behaviour of these new optimization flags for the linker
# is to use the compiler optimization flags if no flags are set.
# This does not work for IAR tools as there are no optimization flags
# for the linker, and leaving the property empty causes it to be
# filled with the compiler flags.
# This workaround adds extra info to the map file only so should not
# affect the generated code
# To have this working properly, either the automatic use of compiler
# flags must be removed, or we add some kind of dummy do nothing
# command line option to the linker.
# Either way, as this is causing an error right now, we use this
# temporary workaround.

set_property(TARGET linker PROPERTY no_optimization --entry_list_in_address_order)
set_property(TARGET linker PROPERTY optimization_debug --entry_list_in_address_order)
set_property(TARGET linker PROPERTY optimization_speed --entry_list_in_address_order)
set_property(TARGET linker PROPERTY optimization_size --entry_list_in_address_order)
set_property(TARGET linker PROPERTY optimization_size_aggressive --entry_list_in_address_order)


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
