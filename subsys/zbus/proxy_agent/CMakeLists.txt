#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library_sources(zbus_proxy_agent.c)

zephyr_linker_sources(SECTIONS zbus_proxy_agent_sections.ld)

zephyr_library_sources_ifdef(CONFIG_ZBUS_PROXY_AGENT_IPC zbus_proxy_agent_ipc.c)
