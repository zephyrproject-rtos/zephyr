# Copyright (c) 2019 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
  # We rely on GAS for assembling, so don't use the integrated assembler
  zephyr_compile_options_ifndef(CONFIG_X86_IAMCU $<$<COMPILE_LANGUAGE:ASM>:-no-integrated-as>)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  zephyr_compile_options($<$<COMPILE_LANGUAGE:ASM>:-Wa,--divide>)
endif()

zephyr_library_sources(
  cache.c
  cache_s.S
  cpuhalt.c
  crt0.S
  excstub.S
  intstub.S
  irq_manage.c
  swap.S
  sys_fatal_error_handler.c
  thread.c
  spec_ctrl.c
  )

zephyr_library_sources_if_kconfig(			irq_offload.c)
zephyr_library_sources_if_kconfig(			x86_mmu.c)
zephyr_library_sources_ifdef(CONFIG_X86_USERSPACE	userspace.S)
zephyr_library_sources_ifdef(CONFIG_LAZY_FP_SHARING	float.c)

# Last since we declare default exception handlers here
zephyr_library_sources(fatal.c)
