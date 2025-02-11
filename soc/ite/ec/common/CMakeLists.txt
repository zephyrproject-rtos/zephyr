zephyr_include_directories(.)

zephyr_sources(
  check_regs.c
  soc_irq.S
  soc_common_irq.c
  vector.S
  )

zephyr_sources_ifdef(CONFIG_PM power.c)
zephyr_sources_ifdef(CONFIG_PM_POLICY_CUSTOM policy.c)
