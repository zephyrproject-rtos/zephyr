# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_OPAMP_MCUX_OPAMP opamp_mcux_opamp.c)
zephyr_library_sources_ifdef(CONFIG_OPAMP_MCUX_OPAMP_FAST opamp_mcux_opamp_fast.c)
