# SPDX-FileCopyrightText: 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_EQDC_MCUX eqdc_mcux.c)
