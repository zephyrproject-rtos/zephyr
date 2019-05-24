# SPDX-License-Identifier: Apache-2.0


macro(toolchain_cc_nostdinc)

  if (NOT CONFIG_NEWLIB_LIBC AND
    NOT COMPILER STREQUAL "xcc" AND
    NOT CONFIG_NATIVE_APPLICATION)
    zephyr_compile_options( -nostdinc)
    zephyr_system_include_directories(${NOSTDINC})
  endif()

endmacro()
