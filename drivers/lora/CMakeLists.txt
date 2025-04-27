# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_LORA_SHELL shell.c)

add_subdirectory_ifdef(CONFIG_LORA_MODULE_BACKEND_LORAMAC_NODE loramac_node)
