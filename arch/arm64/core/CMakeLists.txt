# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  cpu_idle.S
  fatal.c
  irq_init.c
  irq_manage.c
  prep_c.c
  reset.S
  reset.c
  switch.S
  thread.c
  vector_table.S
)

zephyr_library_sources_ifdef(CONFIG_FPU_SHARING fpu.c fpu.S)
zephyr_library_sources_ifdef(CONFIG_ARM_MMU mmu.c mmu.S)
zephyr_library_sources_ifdef(CONFIG_ARM_MPU cortex_r/arm_mpu.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE userspace.S)
zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE tls.c)
zephyr_library_sources_ifdef(CONFIG_HAS_ARM_SMCCC smccc-call.S)
zephyr_library_sources_ifdef(CONFIG_AARCH64_IMAGE_HEADER header.S)
zephyr_library_sources_ifdef(CONFIG_CACHE_MANAGEMENT cache.c)
zephyr_library_sources_ifdef(CONFIG_SEMIHOST semihost.c)
if ((CONFIG_MP_NUM_CPUS GREATER 1) OR (CONFIG_SMP))
  zephyr_library_sources(smp.c)
endif ()

zephyr_cc_option_ifdef(CONFIG_USERSPACE -mno-outline-atomics)

add_subdirectory_ifdef(CONFIG_SOC_XENVM xen)
