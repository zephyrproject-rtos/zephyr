# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if (CONFIG_COVERAGE)
  zephyr_compile_options($<TARGET_PROPERTY:compiler,coverage>)
  zephyr_link_libraries($<TARGET_PROPERTY:linker,coverage>)
endif ()

zephyr_library_sources(
  fatal.c
  irq_init.c
  irq_manage.c
  prep_c.c
  reset.S
  switch.S
  thread.c
  vector_table.S
)

# Workaround aarch64 QEMU not responding to host OS signals
# during 'wfi'.
# See https://github.com/zephyrproject-rtos/sdk-ng/issues/255
if (CONFIG_SOC_QEMU_CORTEX_A53)
  zephyr_library_sources(cpu_idle_qemu.c)
else ()
  zephyr_library_sources(cpu_idle.S)
endif ()

zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_ARM_MMU arm_mmu.c)
