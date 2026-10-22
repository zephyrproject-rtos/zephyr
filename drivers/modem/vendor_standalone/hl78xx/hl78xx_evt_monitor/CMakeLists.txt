# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Netfeasa Ltd.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(hl78xx_evt_monitor.c)
# Event monitors data must be in RAM
zephyr_linker_sources(RWDATA hl78xx_evt_monitor.ld)
