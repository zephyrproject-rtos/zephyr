# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

# KIT_PSC3M5_CC2 carries an isolated SEGGER J-Link LITE debug probe.
#
# PSC3xxF_tm is the TrustZone aware J-Link device entry matching this board
# target (CONFIG_TRUSTED_EXECUTION_SECURE=y) and the 256 KB flash of
# PSC3M5FDS2AFQ1.
board_runner_args(jlink "--device=PSC3xxF_tm" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
