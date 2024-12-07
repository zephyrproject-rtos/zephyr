# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  exc.S
  fault.c
  irq_init.c
  reboot.c
  reset.S
  stacks.c
  tcm.c
  vector_table.S
  irq_manage.c
  prep_c.c
  thread.c
  cpu_idle.S
  smp.c
  )

zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
zephyr_library_sources_ifdef(CONFIG_USERSPACE thread.c)
zephyr_library_sources_ifdef(CONFIG_SEMIHOST semihost.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE __aeabi_read_tp.S)
zephyr_library_sources_ifdef(CONFIG_ARCH_CACHE cache.c)
zephyr_library_sources_ifdef(CONFIG_USE_SWITCH switch.S)
zephyr_library_sources_ifndef(CONFIG_USE_SWITCH swap_helper.S exc_exit.S)
