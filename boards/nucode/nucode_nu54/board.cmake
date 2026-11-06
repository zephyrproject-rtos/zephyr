# Copyright (c) 2024 Nordic Semiconductor ASA
# Copyright (c) 2026 NUCODE Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_NRF54L15_CPUAPP)
  # Never let pyOCD recover a protected target by erasing it implicitly.
  board_runner_args(pyocd "--target=nrf54l" "--tool-opt=-O=auto_unlock=false")
  board_runner_args(jlink "--device=nRF54L15_M33" "--speed=4000")

  # The onboard DAPLink probe is the default runner.
  include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()
