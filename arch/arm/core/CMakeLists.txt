# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_compile_options_ifdef(CONFIG_COVERAGE_GCOV
  -ftest-coverage
  -fprofile-arcs
  -fno-inline
  )

zephyr_library_sources_if_kconfig(swap.c)
zephyr_library_sources_ifdef(CONFIG_SWAP swap_helper.S)

zephyr_library_sources_if_kconfig(irq_manage.c)
zephyr_library_sources_if_kconfig(thread.c)
zephyr_library_sources_if_kconfig(cpu_idle.S)

zephyr_library_sources_ifdef(CONFIG_FATAL fatal.c)
zephyr_library_sources_ifdef(CONFIG_FATAL fault_s.S)

zephyr_library_sources_ifdef(CONFIG_ARM_NMI nmi.c)
zephyr_library_sources_ifdef(CONFIG_ARM_NMI nmi_on_reset.S)

zephyr_library_sources_if_kconfig(prep_c.c)
zephyr_library_sources_ifdef(CONFIG_ARM_EXC_EXIT exc_exit.S)
zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
zephyr_library_sources_ifdef(CONFIG_CPLUSPLUS __aeabi_atexit.c)
zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_CPU_CORTEX_M0 irq_relay.S)
zephyr_library_sources_ifdef(CONFIG_USERSPACE userspace.S)

add_subdirectory_ifdef(CONFIG_CPU_CORTEX_M cortex_m)
add_subdirectory_ifdef(CONFIG_ARM_MPU cortex_m/mpu)
add_subdirectory_ifdef(CONFIG_CPU_CORTEX_M_HAS_CMSE cortex_m/cmse)
add_subdirectory_ifdef(CONFIG_ARM_SECURE_FIRMWARE cortex_m/tz)
add_subdirectory_ifdef(CONFIG_ARM_NONSECURE_FIRMWARE cortex_m/tz)

add_subdirectory_ifdef(CONFIG_CPU_CORTEX_R cortex_r)
