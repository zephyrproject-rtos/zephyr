# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  exc.S
  exc_exit.S
  fault.c
  irq_init.c
  reboot.c
  reset.S
  stacks.c
  tcm.c
  vector_table.S
  swap.c
  swap_helper.S
  irq_manage.c
  prep_c.c
  thread.c
  cpu_idle.S
  )

zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
zephyr_library_sources_ifdef(CONFIG_USERSPACE thread.c)
zephyr_library_sources_ifdef(CONFIG_SEMIHOST semihost.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE __aeabi_read_tp.S)
zephyr_library_sources_ifdef(CONFIG_ARCH_CACHE cache.c)
