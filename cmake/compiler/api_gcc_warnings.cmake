macro(toolchain_cc_warnings)

  zephyr_cc_option(-Wno-pointer-sign)
  zephyr_cc_option(-Wno-unused-but-set-variable)

  # Force an error when things like SYS_INIT(foo, ...) occur with a missing header.
  zephyr_cc_option(-Werror=implicit-int)

  # Prohibit void pointer arithmetic. Illegal in C99
  zephyr_cc_option(-Wpointer-arith)

  # Prohibit date/time macros, which would make the build non-deterministic
  # cc-option(-Werror=date-time)


  # ==========================================================================
  #
  # cmake -DW=... settings
  #
  # W=1 - warnings that may be relevant and does not occur too often
  # W=2 - warnings that occur quite often but may still be relevant
  # W=3 - the more obscure warnings, can most likely be ignored
  # ==========================================================================
  if(W MATCHES "1")
    zephyr_compile_options(
      -Wextra
      -Wunused
      -Wno-unused-parameter
      -Wmissing-declarations
      -Wmissing-format-attribute
      -Wold-style-definition
    )
    zephyr_cc_option(
      -Wmissing-prototypes
      -Wmissing-include-dirs
      -Wunused-but-set-variable
      -Wno-missing-field-initializers
    )
  endif()

  if(W MATCHES "2")
    zephyr_compile_options(
      -Waggregate-return
      -Wcast-align
      -Wdisabled-optimization
      -Wnested-externs
      -Wshadow
    )
    zephyr_cc_option(
      -Wlogical-op
      -Wmissing-field-initializers
    )
  endif()

  if(W MATCHES "3")
    zephyr_compile_options(
      -Wbad-function-cast
      -Wcast-qual
      -Wconversion
      -Wpacked
      -Wpadded
      -Wpointer-arith
      -Wredundant-decls
      -Wswitch-default
    )
    zephyr_cc_option(
      -Wpacked-bitfield-compat
      -Wvla
    )
  endif()

endmacro()
