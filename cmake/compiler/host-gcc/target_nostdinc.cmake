
macro(toolchain_cc_nostdinc)

  zephyr_compile_options_ifndef(CONFIG_NATIVE_APPLICATION
    -nostdinc
  )

endmacro()
