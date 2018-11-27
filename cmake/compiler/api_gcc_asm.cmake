macro(toolchain_cc_asm)

  zephyr_compile_options(
    $<$<COMPILE_LANGUAGE:ASM>:-xassembler-with-cpp>
    $<$<COMPILE_LANGUAGE:ASM>:-D_ASMLANGUAGE>
  )

  if(CONFIG_READABLE_ASM)
    zephyr_cc_option(-fno-reorder-blocks)
    zephyr_cc_option(-fno-ipa-cp-clone)
    zephyr_cc_option(-fno-partial-inlining)
  endif()

endmacro()
