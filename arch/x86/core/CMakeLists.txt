if (COMPILER STREQUAL "clang")
  # We rely on GAS for assembling, so don't use the integrated assembler
  zephyr_compile_options_ifndef(CONFIG_X86_IAMCU $<$<COMPILE_LANGUAGE:ASM>:-no-integrated-as>)
endif()

zephyr_compile_options($<$<COMPILE_LANGUAGE:ASM>:-Wa,--divide>)

add_subdirectory_ifdef(CONFIG_DEBUG_INFO debug)

zephyr_sources(
  cache.c
  cache_s.S
  cpuhalt.c
  crt0.S
  excstub.S
  intstub.S
  irq_manage.c
  msr.c
  swap.S
  sys_fatal_error_handler.c
  thread.c
  )

zephyr_sources_if_kconfig(                irq_offload.c)
zephyr_sources_if_kconfig(                x86_mmu.c)
zephyr_sources_if_kconfig(                reboot_rst_cnt.c)
zephyr_sources_ifdef(CONFIG_FP_SHARING    float.c)
zephyr_sources_ifdef(CONFIG_X86_USERSPACE userspace.S)

# Last since we declare default exception handlers here
zephyr_sources(fatal.c)
