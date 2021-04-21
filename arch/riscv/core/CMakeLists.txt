# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_RISCV_PMP pmp)

zephyr_library()

zephyr_library_sources(
  cpu_idle.c
  fatal.c
  irq_manage.c
  isr.S
  prep_c.c
  reboot.c
  reset.S
  swap.S
  thread.c
)

zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE tls.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE userspace.S)
