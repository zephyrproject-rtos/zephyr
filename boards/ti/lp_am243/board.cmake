# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright 2026 Siemens Mobility GmbH
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_AM2434_R5F0_0)
  board_runner_args(openocd "--gdb-client-port=3334")
elseif(CONFIG_SOC_AM2434_M4)
  board_runner_args(openocd "--gdb-client-port=3338")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
