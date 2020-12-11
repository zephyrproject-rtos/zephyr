# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_VCNL4040 vcnl4040.c)
zephyr_library_sources_ifdef(CONFIG_VCNL4040_TRIGGER vcnl4040_trigger.c)
