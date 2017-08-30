zephyr_sources(
  exc_exit.S
  irq_init.c
  swap.S
  fault.c
  irq_manage.c
  thread.c
  cpu_idle.S
  fault_s.S
  fatal.c
  sys_fatal_error_handler.c
  thread_abort.c
)

zephyr_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
zephyr_sources_ifdef(CONFIG_CPLUSPLUS __aeabi_atexit.c)
zephyr_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)

add_subdirectory_ifdef(CONFIG_CPU_CORTEX_M cortex_m)
add_subdirectory_ifdef(CONFIG_CPU_HAS_MPU  cortex_m/mpu)
