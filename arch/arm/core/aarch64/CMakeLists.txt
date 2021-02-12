# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if (CONFIG_COVERAGE)
  zephyr_compile_options($<TARGET_PROPERTY:compiler,coverage>)
  zephyr_link_libraries($<TARGET_PROPERTY:linker,coverage>)
endif ()

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

zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE ../common/tls.c)
zephyr_library_sources_ifdef(CONFIG_HAS_ARM_SMCCC smccc-call.S)
zephyr_library_sources_ifdef(CONFIG_AARCH64_IMAGE_HEADER header.S)

add_subdirectory_ifdef(CONFIG_ARM_MMU mmu)
