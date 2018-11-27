macro(toolchain_cc_optimizations)

  # We need to set an optimization level.
  # Default to -Os
  # unless CONFIG_NO_OPTIMIZATIONS is set, then it is -O0
  # or unless CONFIG_DEBUG is set, then it is -Og
  #
  # also, some toolchain's break with -Os, and some toolchain's break
  # with -Og so allow them to override what flag to use
  #
  # Finally, the user can use Kconfig to add compiler options that will
  # come after these options and override them
  set_ifndef(OPTIMIZE_FOR_NO_OPTIMIZATIONS_FLAG "-O0")
  set_ifndef(OPTIMIZE_FOR_DEBUG_FLAG            "-Og")
  set_ifndef(OPTIMIZE_FOR_SIZE_FLAG             "-Os")
  set_ifndef(OPTIMIZE_FOR_SPEED_FLAG            "-O2")

  if(CONFIG_NO_OPTIMIZATIONS)
    set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_NO_OPTIMIZATIONS_FLAG})
  elseif(CONFIG_DEBUG_OPTIMIZATIONS)
    set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_DEBUG_FLAG})
  elseif(CONFIG_SPEED_OPTIMIZATIONS)
    set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_SPEED_FLAG})
  elseif(CONFIG_SIZE_OPTIMIZATIONS)
    set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_SIZE_FLAG}) # Default
  else()
    assert(0 "Unreachable code. Expected optimization level to have been chosen. See misc/Kconfig.")
  endif()

  # Reduce ELF size by omitting the unwind table in the DWARF sections
  zephyr_cc_option(-fno-asynchronous-unwind-tables)

endmacro()
