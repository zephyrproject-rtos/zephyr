#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(event_manager.c)
zephyr_sources_ifdef(CONFIG_SHELL event_manager_shell.c)

zephyr_linker_sources(SECTIONS em.ld)
