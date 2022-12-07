# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_sources_ifdef(CONFIG_CACHE_ASPEED	cache_aspeed.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE		cache_handlers.c)
