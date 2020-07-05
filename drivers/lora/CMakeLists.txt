# SPDX-License-Identifier: Apache-2.0

# LoRa drivers depend on the include directories exposed by the loramac-node
# library. Hence, if it exists then the source files are added to that otherwise
# a library with same name is created.
if(TARGET loramac-node)
	set(ZEPHYR_CURRENT_LIBRARY loramac-node)
else()
	zephyr_library_named(loramac-node)
endif()

zephyr_library_sources_ifdef(CONFIG_LORA_SHELL shell.c)
zephyr_library_sources_ifdef(CONFIG_HAS_SEMTECH_RADIO_DRIVERS hal_common.c)
zephyr_library_sources_ifdef(CONFIG_HAS_SEMTECH_RADIO_DRIVERS sx12xx_common.c)
zephyr_library_sources_ifdef(CONFIG_LORA_SX1276 sx1276.c)
zephyr_library_sources_ifdef(CONFIG_LORA_SX126X sx126x.c)
