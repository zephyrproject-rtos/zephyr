zephyr_sources(
  settings_store.c
  settings.c
  settings_init.c
<<<<<<< HEAD
  )

zephyr_sources_ifdef(CONFIG_NSETTINGS_FCB settings_fcb.c)
=======
  settings_fcb.c
  )

zephyr_sources_ifdef(CONFIG_NSETTINGS_RUNTIME settings_runtime.c)
>>>>>>> subsys/settings: Reworked settings module
