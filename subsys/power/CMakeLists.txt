zephyr_sources(
  power.c
  device.c
  )
zephyr_sources_ifdef(CONFIG_PM_CONTROL_STATE_LOCK pm_ctrl.c)
add_subdirectory(policy)
