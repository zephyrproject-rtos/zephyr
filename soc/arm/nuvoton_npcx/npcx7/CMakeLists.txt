# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_library_sources_ifdef(CONFIG_PM power.c)

zephyr_sources(
  soc.c
)
