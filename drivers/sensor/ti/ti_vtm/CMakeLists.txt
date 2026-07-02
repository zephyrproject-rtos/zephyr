# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_TI_VTM ti_vtm.c)
