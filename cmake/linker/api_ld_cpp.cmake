macro(toolchain_ld_cpp)

  if(CONFIG_LIB_CPLUSPLUS)
    zephyr_ld_options(
      -lstdc++
    )
  endif()

endmacro()
