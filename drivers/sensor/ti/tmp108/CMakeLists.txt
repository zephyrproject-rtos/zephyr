# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(tmp108.c)
zephyr_library_sources_ifdef(CONFIG_TMP108_ALERT_INTERRUPTS tmp108_trigger.c)
