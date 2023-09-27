# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_ld_cpp)

  zephyr_link_libraries(
    -lc++
  )

endmacro()
