# SPDX-License-Identifier: Apache-2.0

zephyr_cc_option(-mlongcalls)

zephyr_library()

zephyr_library_sources(
  cpu_idle.c
  fatal.c
  window_vectors.S
  xtensa-asm2-util.S
  xtensa-asm2.c
  irq_manage.c
  )

zephyr_library_sources_ifndef(CONFIG_ATOMIC_OPERATIONS_C atomic.S)
zephyr_library_sources_ifdef(CONFIG_XTENSA_USE_CORE_CRT1 crt1.S)
zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE tls.c)
zephyr_library_sources_ifdef(CONFIG_XTENSA_ENABLE_BACKTRACE xtensa_backtrace.c)
zephyr_library_sources_ifdef(CONFIG_XTENSA_ENABLE_BACKTRACE debug_helpers_asm.S)

zephyr_library_include_directories(include)

add_subdirectory(startup)
