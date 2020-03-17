# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(openthread_platform)
zephyr_library_sources(
  alarm.c
  entropy.c
  logging.c
  misc.c
  platform.c
  radio.c
  settings.c
  spi.c
  uart.c
  )

zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_SHELL shell.c)
