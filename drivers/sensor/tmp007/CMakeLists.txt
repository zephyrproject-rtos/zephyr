# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_TMP007 tmp007.c)
zephyr_library_sources_ifdef(CONFIG_TMP007_TRIGGER tmp007_trigger.c)
