# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  esp32-mp.c
  loader.c
  )

  zephyr_library_sources_ifdef(CONFIG_GDBSTUB gdbstub.c)
