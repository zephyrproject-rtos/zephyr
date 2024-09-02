# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

set_property(TARGET linker PROPERTY cpp_base -Hcplus)

check_set_linker_property(TARGET linker PROPERTY baremetal
                          -Hlld
                          -Hnosdata
                          -Xtimer0 # to suppress the warning message
                          -Hnoxcheck_obj
                          -Hnocplus
                          -Hhostlib=
                          -Hheap=0
                          -Hnoivt
                          -Hnocrt
)

# There are two options:
# - We have full MWDT libc support and we link MWDT libc - this is default
#   behavior and we don't need to do something for that.
# - We use minimal libc provided by Zephyr itself. In that case we must not
#   link MWDT libc, but we still need to link libmw
if(CONFIG_MINIMAL_LIBC)
  check_set_linker_property(TARGET linker APPEND PROPERTY baremetal
                            -Hnolib
                            -Hldopt=-lmw
  )
endif()

check_set_linker_property(TARGET linker PROPERTY orphan_warning
                          ${LINKERFLAGPREFIX},--orphan-handling=warn
)

check_set_linker_property(TARGET linker PROPERTY orphan_error
                          ${LINKERFLAGPREFIX},--orphan-handling=error
)

# Extra warnings options for twister run
set_property(TARGET linker PROPERTY warnings_as_errors -Wl,--fatal-warnings)

check_set_linker_property(TARGET linker PROPERTY sort_alignment -Wl,--sort-section=alignment)
