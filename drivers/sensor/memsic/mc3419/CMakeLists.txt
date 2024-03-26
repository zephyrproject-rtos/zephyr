# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023 Linumiz

zephyr_library()

zephyr_library_sources(mc3419.c)
zephyr_library_sources_ifdef(CONFIG_MC3419_TRIGGER mc3419_trigger.c)
