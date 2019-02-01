zephyr_sources(
  settings_store.c
  settings.c
  settings_init.c
  )

zephyr_sources_ifdef(CONFIG_SETTINGS_RUNTIME settings_runtime.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_FCB settings_fcb.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_FS settings_file.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_NVS settings_nvs.c)
zephyr_sources_ifndef(CONFIG_SETTINGS_DEFAULT_NONE settings_default.c)
