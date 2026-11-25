# Copyright (c) 2026 A-Eberle <Daniel.Kampert@a-eberle.de>
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ade9000.c ade9000_config.c ade9000_conv.c)
zephyr_library_sources_ifdef(CONFIG_ADE9000_TRIGGER ade9000_trigger.c)
zephyr_library_sources_ifdef(CONFIG_ADE9000_MULTIPOINT ade9000_multipoint.c)
