if(CONFIG_HAS_INVENTEK_WIFI)
  if(CONFIG_WIFI_ISM43362_M3G_L44)
    zephyr_include_directories(${PROJECT_SOURCE_DIR}/ext/hal/inventek/ism43362-m3g-l44/Inc)

    zephyr_sources(inventek_private.c)
  endif()
endif()



