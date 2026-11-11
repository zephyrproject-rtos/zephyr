#
# Copyright (c) 2017, 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library_sources(soc.c)
zephyr_library_sources_ifdef(CONFIG_SOC_RESET_HOOK gcc/startup_LPC54114_cm4.S)
