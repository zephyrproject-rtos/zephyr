zephyr_sources(
  cpu_idle.c
  fatal.c
  irq_manage.c
  irq_offload.c
  prep_c.c
  reset.S
  thread_entry_wrapper.S
  thread.c
)

# SMP-only sources
zephyr_sources_ifdef(CONFIG_SMP curr_cpu.c)
zephyr_sources_ifdef(CONFIG_SMP start_cpu.c)

# Use SMP ISR if SMP is configured
zephyr_sources_ifndef(CONFIG_SMP isr.S)
zephyr_sources_ifdef(CONFIG_SMP  isr_smp.S)

# Use swap or switch depending on the USE_SWITCH config
zephyr_sources_ifndef(CONFIG_USE_SWITCH swap.S)
zephyr_sources_ifdef(CONFIG_USE_SWITCH  switch.S)
