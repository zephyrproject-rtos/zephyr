# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_LORAWAN_APP_CLOCK_SYNC clock_sync.c)
zephyr_library_sources_ifdef(CONFIG_LORAWAN_SERVICES lorawan_services.c)
