
macro(toolchain_cc_nostdinc)

  if (NOT CONFIG_NEWLIB_LIBC AND
    NOT COMPILER STREQUAL "xcc" AND
    NOT CONFIG_NATIVE_APPLICATION)
    zephyr_compile_options( -nostdinc)
  endif()

endmacro()
