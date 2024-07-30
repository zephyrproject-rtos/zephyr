# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(tmag5170.c)
zephyr_library_sources_ifdef(CONFIG_TMAG5170_TRIGGER tmag5170_trigger.c)
