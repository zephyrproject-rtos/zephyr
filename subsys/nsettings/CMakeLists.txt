zephyr_sources(
  settings_store.c
  settings.c
  settings_init.c
  )

zephyr_sources_ifdef(CONFIG_NSETTINGS_FCB settings_fcb.c)
