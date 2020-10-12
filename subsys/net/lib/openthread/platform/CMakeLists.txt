# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(openthread_platform)
zephyr_library_sources(
  alarm.c
  entropy.c
  misc.c
  platform.c
  radio.c
  settings.c
  spi.c
  )

zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_DIAG diag.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_COPROCESSOR uart.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_SHELL shell.c)
zephyr_library_sources_ifndef(CONFIG_LOG_BACKEND_SPINEL logging.c)

zephyr_include_directories(.)
