# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_CANOPEN CO_driver.c)
zephyr_library_sources_ifdef(CONFIG_CANOPEN_SYNC_THREAD canopen_sync.c)
zephyr_include_directories(.)
