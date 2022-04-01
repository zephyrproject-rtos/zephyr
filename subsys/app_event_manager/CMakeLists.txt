#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)
zephyr_sources(app_event_manager.c)
zephyr_sources_ifdef(CONFIG_SHELL app_event_manager_shell.c)

zephyr_linker_sources(SECTIONS aem.ld)
