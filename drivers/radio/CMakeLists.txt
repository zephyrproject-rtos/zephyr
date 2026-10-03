# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/radio.h)

zephyr_library()

zephyr_include_directories(${CONFIG_BOARD})

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_RADIO_SILABS radio_silabs.c ${CONFIG_BOARD}/rail_config.c)
# zephyr-keep-sorted-stop
