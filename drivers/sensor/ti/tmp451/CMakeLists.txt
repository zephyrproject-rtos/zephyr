# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(tmp451.c)
zephyr_library_sources_ifdef(CONFIG_TMP451_TRIGGER tmp451_trigger.c)
