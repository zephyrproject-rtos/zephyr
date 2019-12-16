# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_NEWLIB_LIBC newlib_fixes.c)
