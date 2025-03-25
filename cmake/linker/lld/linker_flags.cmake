# Copyright (c) 2022 Google LLC
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# Since lld is a drop in replacement for ld, we can just use ld's flags as a base
# and adjust for lld specifics afterwards.
include(${ZEPHYR_BASE}/cmake/linker/ld/linker_flags.cmake OPTIONAL)

if(NOT CONFIG_NATIVE_LIBRARY AND NOT CONFIG_EXTERNAL_MODULE_LIBCPP)
  set_property(TARGET linker PROPERTY cpp_base ${LINKERFLAGPREFIX},-z,norelro)
endif()

set_property(TARGET linker PROPERTY no_position_independent "${LINKERFLAGPREFIX},--no-pie")

set_property(TARGET linker PROPERTY lto_arguments)

check_set_linker_property(TARGET linker PROPERTY sort_alignment ${LINKERFLAGPREFIX},--sort-section=alignment)

# Copy all of the compiler optimization properties to the equivalent linker properties
foreach(prop no_optimization optimization_debug optimization_speed
    optimization_size optimization_size_aggressive optimization_fast)
  get_property(optimization_flag TARGET compiler PROPERTY ${prop})
  set_property(TARGET linker PROPERTY ${prop} ${optimization_flag})
endforeach()

if(CONFIG_RISCV_GP)
  check_set_linker_property(TARGET linker PROPERTY relax ${LINKERFLAGPREFIX},--relax-gp)
endif()
