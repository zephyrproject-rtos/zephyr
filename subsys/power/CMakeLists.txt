zephyr_sources_ifdef(CONFIG_PM_CONTROL_OS
  power.c
  device.c
  )
zephyr_sources_ifdef(CONFIG_PM_CONTROL_STATE_LOCK pm_ctrl.c)
add_subdirectory_ifdef(CONFIG_PM_CONTROL_OS policy)
zephyr_sources_ifdef(CONFIG_DEVICE_IDLE_PM device_pm.c)
zephyr_sources_if_kconfig(reboot.c)
