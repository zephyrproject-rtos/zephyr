zephyr_sources_ifdef(CONFIG_SYS_POWER_MANAGEMENT
  power.c
  device.c
  )
zephyr_sources_ifdef(CONFIG_SYS_PM_STATE_LOCK pm_ctrl.c)
zephyr_sources_if_kconfig(reboot.c)
add_subdirectory(policy)
