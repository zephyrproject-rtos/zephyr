# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_AUXDISPLAY_ITRON		auxdisplay_itron.c)
zephyr_library_sources_ifdef(CONFIG_AUXDISPLAY_JHD1313		auxdisplay_jhd1313.c)
