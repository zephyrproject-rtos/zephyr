# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# Files shared by Cortex-A/R and ARM11 targets.
zephyr_library_sources(
  exc.S
  irq_init.c
  reboot.c
  stacks.c
  tcm.c
  vector_table.S
  irq_manage.c
  prep_c.c
  )

zephyr_library_sources_ifdef(CONFIG_SEMIHOST semihost.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE __aeabi_read_tp.S)
zephyr_library_sources_ifdef(CONFIG_USE_SWITCH switch.S)

# The files below are replaced by arch/arm/core/arm11/ when
# CONFIG_CPU_AARCH32_ARM11 is set.  That subdirectory is added by the parent
# CMakeLists.txt and provides its own reset, cache, cpu_idle, fault, thread,
# isr_wrapper, swap_helper, and exc_exit for ARM1176JZF-S.
if(NOT CONFIG_CPU_AARCH32_ARM11)
  zephyr_library_sources(
    reset.S
    fault.c
    thread.c
    cpu_idle.c
    smp.c
    )

  zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
  zephyr_library_sources_ifdef(CONFIG_ARCH_CACHE cache.c)
  zephyr_library_sources_ifndef(CONFIG_USE_SWITCH swap_helper.S exc_exit.S)
endif()
