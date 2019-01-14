zephyr_sources(
  settings_store.c
  settings.c
  settings_init.c
  settings_fcb.c
  settings_nvs.c
  settings_file.c
  )

zephyr_sources_ifdef(CONFIG_NSETTINGS_RUNTIME settings_runtime.c)
