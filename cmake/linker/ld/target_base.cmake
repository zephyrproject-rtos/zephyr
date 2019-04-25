# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_ld_base)

  # TOOLCHAIN_LD_FLAGS comes from compiler/gcc/target.cmake
  zephyr_ld_options(
    ${TOOLCHAIN_LD_FLAGS}
  )

endmacro()
