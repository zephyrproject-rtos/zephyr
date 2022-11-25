# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  settings_store.c
  settings.c
  settings_init.c
  settings_line.c
  )

zephyr_sources_ifdef(CONFIG_SETTINGS_RUNTIME settings_runtime.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_FILE settings_file.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_FS settings_file.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_FCB settings_fcb.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_NVS settings_nvs.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_NONE settings_none.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_SHELL settings_shell.c)
