zephyr_sources(
  cpu_idle.c
  fatal.c
  irq_manage.c
  irq_offload.c
  isr.S
  prep_c.c
  reset.S
  swap.S
  thread_entry_wrapper.S
  thread.c
)

# SMP-only sources
zephyr_sources_ifdef(CONFIG_SMP curr_cpu.c)
zephyr_sources_ifdef(CONFIG_SMP start_cpu.c)
