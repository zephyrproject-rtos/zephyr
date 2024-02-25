# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(pinnacle.c)
zephyr_library_sources(pinnacle_spi.c)
zephyr_library_sources_ifdef(CONFIG_PINNACLE_TRIGGER pinnacle_trigger.c)
