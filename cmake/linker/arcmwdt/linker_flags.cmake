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

# There are three options:
# - We have full MWDT libc support in the toolchain and we link MWDT libc
# - We have full Picolibc support in the toolchain and we link Picolibc
# - We use some libc provided by Zephyr itself (minimal or Picolibc).
#   In that case we must not link our MWDT libc or Picolibc,
#   but we still need to link libmw
if(CONFIG_ARCMWDT_LIBC)
  check_set_linker_property(TARGET linker APPEND PROPERTY baremetal
                            -Hlibc=mw
  )
elseif(CONFIG_PICOLIBC_USE_TOOLCHAIN)
  check_set_linker_property(TARGET linker APPEND PROPERTY baremetal
                            -Hlibc=pico
  )
else()
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
