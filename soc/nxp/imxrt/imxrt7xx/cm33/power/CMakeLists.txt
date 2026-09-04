# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_SOC_MIMXRT7XX_PM_XIP_HANDOVER     power_xip.c)

# Shared low-power entry sequence and the resource dependency closure, used by
# both domain modules. This directory is only added for PM or POWEROFF builds, so
# they need no guard of their own.
zephyr_sources(power_common.c power_resources.c)
