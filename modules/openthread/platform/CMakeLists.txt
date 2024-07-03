# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(openthread_platform)
zephyr_library_sources(
  alarm.c
  entropy.c
  misc.c
  )

zephyr_library_sources_ifndef(CONFIG_OPENTHREAD_HDLC_RCP
  radio.c
  platform.c
  spi.c
  )

zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_BLE_TCAT ble.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_DIAG diag.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_COPROCESSOR uart.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_CRYPTO_PSA crypto_psa.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_SHELL shell.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_EXTERNAL_HEAP memory.c)
zephyr_library_sources_ifdef(CONFIG_SETTINGS settings.c)
zephyr_library_sources_ifndef(CONFIG_LOG_BACKEND_SPINEL logging.c)

zephyr_include_directories(.)
