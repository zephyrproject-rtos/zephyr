zephyr_include_directories(.)

zephyr_sources(
  soc_irq.S
  soc_common_irq.c
  )

zephyr_sources_ifdef(CONFIG_PM power.c)
zephyr_sources_ifdef(CONFIG_PM_POLICY_CUSTOM policy.c)
