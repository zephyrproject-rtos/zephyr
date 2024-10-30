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

# Force LLVM to use built-in lld linker
if(NOT CONFIG_LLVM_USE_LD)
  check_set_linker_property(TARGET linker APPEND PROPERTY baremetal -fuse-ld=lld)
endif()

set_property(TARGET linker PROPERTY no_position_independent "${LINKERFLAGPREFIX},--no-pie")

set_property(TARGET linker PROPERTY lto_arguments)

check_set_linker_property(TARGET linker PROPERTY sort_alignment ${LINKERFLAGPREFIX},--sort-section=alignment)

if(CONFIG_RISCV_GP)
  check_set_linker_property(TARGET linker PROPERTY relax ${LINKERFLAGPREFIX},--relax-gp)
endif()
