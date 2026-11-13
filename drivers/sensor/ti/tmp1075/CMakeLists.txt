# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(tmp1075.c)
zephyr_library_sources_ifdef(CONFIG_TMP1075_TRIGGER tmp1075_trigger.c)
