# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

check_set_linker_property(TARGET linker PROPERTY base
                          ${LINKERFLAGPREFIX},--gc-sections
                          ${LINKERFLAGPREFIX},--build-id=none
)

# Used to retrieve individual flag
set_property(TARGET linker PROPERTY gc_sections "${LINKERFLAGPREFIX},--gc-sections")

if(NOT CONFIG_NATIVE_LIBRARY AND NOT CONFIG_EXTERNAL_MODULE_LIBCPP)
  set_property(TARGET linker PROPERTY cpp_base -lstdc++)
endif()

check_set_linker_property(TARGET linker PROPERTY baremetal
    -nostdlib
    -static
    ${LINKERFLAGPREFIX},-X
    ${LINKERFLAGPREFIX},-N
)

check_set_linker_property(TARGET linker PROPERTY orphan_warning
                          ${LINKERFLAGPREFIX},--orphan-handling=warn
)

check_set_linker_property(TARGET linker PROPERTY orphan_error
                          ${LINKERFLAGPREFIX},--orphan-handling=error
)

set_property(TARGET linker PROPERTY no_gc_sections "${LINKERFLAGPREFIX},--no-gc-sections")

set_property(TARGET linker PROPERTY partial_linking "-r")

check_set_linker_property(TARGET linker PROPERTY no_relax ${LINKERFLAGPREFIX},--no-relax)

check_set_linker_property(TARGET linker PROPERTY sort_alignment
                          ${LINKERFLAGPREFIX},--sort-common=descending
                          ${LINKERFLAGPREFIX},--sort-section=alignment
)

set_property(TARGET linker PROPERTY undefined ${LINKERFLAGPREFIX},--undefined=)
