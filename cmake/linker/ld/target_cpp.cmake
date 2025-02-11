# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_ld_cpp)

  if(NOT CONFIG_EXTERNAL_MODULE_LIBCPP)
    zephyr_link_libraries(
      -lstdc++
    )
  endif()

endmacro()
