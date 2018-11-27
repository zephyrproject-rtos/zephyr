macro(toolchain_cc_security)

  if(NOT CONFIG_NO_OPTIMIZATIONS)
    zephyr_compile_definitions(
      _FORTIFY_SOURCE=2
    )
  endif()

  zephyr_compile_options_ifdef(CONFIG_STACK_CANARIES -fstack-protector-all)

endmacro()
