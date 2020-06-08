# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(loramac-node)

zephyr_library_sources_ifdef(CONFIG_LORA_SHELL shell.c)
zephyr_library_sources_ifdef(CONFIG_LORA_SX1276 sx1276.c)
