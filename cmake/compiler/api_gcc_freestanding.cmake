macro(toolchain_cc_freestanding)

  zephyr_compile_options(
    $<$<COMPILE_LANGUAGE:C>:-std=${CSTD}>
  )

  zephyr_compile_options(
    ${OPTIMIZATION_FLAG} # Usually -Os
    -g # TODO: build configuration enough?
    -Wall
    -Wformat
    -Wformat-security
    -Wno-format-zero-length
    -imacros ${AUTOCONF_H}
    -ffreestanding
    -Wno-main
    ${NOSTDINC_F}
    ${TOOLCHAIN_C_FLAGS}
  )

  # Don't produce a position independent executable
  zephyr_cc_option(-fno-pie)

  # Generate code that does not use a global pointer register
  zephyr_cc_option(-fno-pic)

  # Don't allow the compiler to assume strict signed overflow rules
  # I.e. don't assume overflow results in undefined behavior
  zephyr_cc_option(-fno-strict-overflow)

  if(CONFIG_OVERRIDE_FRAME_POINTER_DEFAULT)
    if(CONFIG_OMIT_FRAME_POINTER)
      zephyr_cc_option(-fomit-frame-pointer)
    else()
      zephyr_cc_option(-fno-omit-frame-pointer)
    endif()
  endif()

  # Don't reorder functions in the object file in order to improve code locality
  zephyr_cc_option(
    -fno-reorder-functions
  )

  if(NOT ${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "xcc")
    zephyr_cc_option(-fno-defer-pop)
  endif()

  zephyr_system_include_directories(${NOSTDINC})

endmacro()
