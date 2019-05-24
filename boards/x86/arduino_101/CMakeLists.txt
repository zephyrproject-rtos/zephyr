# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(pinmux.c)
zephyr_library_sources_ifdef(CONFIG_BT_H4 nrf51_pm.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
