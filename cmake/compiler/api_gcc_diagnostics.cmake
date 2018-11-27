macro(toolchain_cc_diagnostics)

  zephyr_cc_option_ifdef(CONFIG_STACK_USAGE            -fstack-usage)

endmacro()
