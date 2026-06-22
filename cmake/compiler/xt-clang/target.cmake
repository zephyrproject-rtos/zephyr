# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/compiler/xcc/target.cmake)

# Flags not supported by llext linker
# (regexps are supported and match whole word)
set(LLEXT_REMOVE_FLAGS
  -ffunction-sections
  -fdata-sections
  -Os
  -mcpu=.*
)

# Flags to be added to llext code compilation
set(LLEXT_APPEND_FLAGS
  -nostdlib
  -nodefaultlibs
)

if(CONFIG_LLEXT_BUILD_PIC)
  set(LLEXT_REMOVE_FLAGS ${LLEXT_REMOVE_FLAGS}
    -fno-pic
    -fno-pie
  )
  set(LLEXT_APPEND_FLAGS ${LLEXT_APPEND_FLAGS}
    -fPIC
  )
else()
  set(LLEXT_APPEND_FLAGS ${LLEXT_APPEND_FLAGS}
    -ffreestanding
  )
endif()

if(CONFIG_LLEXT_CODEGEN_VLIW_ENABLED)
  set(LLEXT_REMOVE_FLAGS ${LLEXT_REMOVE_FLAGS}
    -mno-generate-flix
  )
endif()

if(CONFIG_LLEXT_CODEGEN_VLIW_DISABLED)
  set(LLEXT_APPEND_FLAGS ${LLEXT_APPEND_FLAGS}
    -mno-generate-flix
  )
endif()
