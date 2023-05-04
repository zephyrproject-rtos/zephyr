# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_TIME source/time/time.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_MALLOC source/stdlib/malloc.c)
