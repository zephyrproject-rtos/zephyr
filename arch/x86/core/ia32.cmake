# Copyright (c) 2019 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
  # We rely on GAS for assembling, so don't use the integrated assembler
  zephyr_compile_options($<$<COMPILE_LANGUAGE:ASM>:-no-integrated-as>)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  zephyr_compile_options($<$<COMPILE_LANGUAGE:ASM>:-Wa,--divide>)
endif()

zephyr_library_sources(
  ia32/cache.c
  ia32/cache_s.S
  ia32/crt0.S
  ia32/excstub.S
  ia32/intstub.S
  ia32/irq_manage.c
  ia32/swap.S
  ia32/thread.c
  )

zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD		ia32/irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_X86_USERSPACE	ia32/userspace.S)
zephyr_library_sources_ifdef(CONFIG_LAZY_FPU_SHARING	ia32/float.c)
zephyr_library_sources_ifdef(CONFIG_GDBSTUB		ia32/gdbstub.c)

zephyr_library_sources_ifdef(CONFIG_DEBUG_COREDUMP	ia32/coredump.c)

# Last since we declare default exception handlers here
zephyr_library_sources(ia32/fatal.c)
