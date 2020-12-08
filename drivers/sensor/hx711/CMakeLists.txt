# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_HX711 hx711.c)
zephyr_library_sources_ifdef(CONFIG_HX711_TRIGGER hx711_trigger.c)
