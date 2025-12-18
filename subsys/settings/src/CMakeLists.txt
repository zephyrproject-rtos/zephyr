# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  settings_store.c
  settings.c
  settings_init.c
  settings_line.c
  )

zephyr_sources_ifdef(CONFIG_SETTINGS_RUNTIME settings_runtime.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_FILE settings_file.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_FCB settings_fcb.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_NVS settings_nvs.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_NONE settings_none.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_SHELL settings_shell.c)
zephyr_sources_ifdef(CONFIG_SETTINGS_ZMS settings_zms.c)
if(CONFIG_SETTINGS_TFM_PSA)
  zephyr_sources(settings_tfm_psa.c)
  zephyr_library_link_libraries(tfm_api)
endif()
zephyr_sources_ifdef(CONFIG_SETTINGS_RETENTION settings_retention.c)
