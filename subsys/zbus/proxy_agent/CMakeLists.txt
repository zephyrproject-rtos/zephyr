#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library_sources(zbus_proxy_agent.c)

zephyr_linker_sources(SECTIONS zbus_proxy_agent_sections.ld)
