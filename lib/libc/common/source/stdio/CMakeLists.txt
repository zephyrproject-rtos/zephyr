# Copyright (c) 2024 Meta Platforms
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_GETC_UNLOCKED getc_unlocked.c)
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_PUTC_UNLOCKED putc_unlocked.c)
