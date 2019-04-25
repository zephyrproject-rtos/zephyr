# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_ld_baremetal)

  # LINKERFLAGPREFIX comes from linker/ld/target.cmake
  zephyr_ld_options(
    -nostdlib
    -static
    -no-pie
    ${LINKERFLAGPREFIX},-X
    ${LINKERFLAGPREFIX},-N
  )

endmacro()
