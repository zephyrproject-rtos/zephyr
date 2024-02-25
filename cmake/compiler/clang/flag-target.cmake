function(clang_target_flag target_arch triple)
  if("${target_arch}" STREQUAL "arm")
    if(DEFINED CONFIG_ARMV8_1_M_MAINLINE)
       # ARMv8-M mainline is ARMv8-M mainline with additional features from ARMv8.1-M.
       set(${triple} thumbv8.1m.main-none-unknown-eabi PARENT_SCOPE)
    elseif(DEFINED CONFIG_ARMV8_M_MAINLINE)
       # ARMv8-M mainline is ARMv7-M with additional features from ARMv8-M.
       set(${triple} thumbv8m.main-none-unknown-eabi PARENT_SCOPE)
    elseif(DEFINED CONFIG_ARMV8_M_BASELINE)
      # ARMv8-M baseline is ARMv6-M with additional features from ARMv8-M.
      set(${triple} thumbv8m.base-none-unknown-eabi PARENT_SCOPE)
    elseif(DEFINED CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
      # ARMV7_M_ARMV8_M_MAINLINE means that ARMv7-M or backward compatible ARMv8-M
      # processor is used.
      set(${triple} armv7m-none-eabi PARENT_SCOPE)
    elseif(DEFINED CONFIG_ARMV6_M_ARMV8_M_BASELINE)
      # ARMV6_M_ARMV8_M_BASELINE means that ARMv6-M or ARMv8-M supporting the
      # Baseline implementation processor is used.
      set(${triple} armv6m-none-eabi PARENT_SCOPE)
    else()
      # Default ARM target supported by all processors.
      set(${triple} arm-none-eabi PARENT_SCOPE)
    endif()
  elseif("${target_arch}" STREQUAL "x86")
    if(CONFIG_64BIT)
      set(${triple} x86_64-pc-none-elf PARENT_SCOPE)
    else()
      set(${triple} i686-pc-none-elf PARENT_SCOPE)
    endif()
  endif()
endfunction()
