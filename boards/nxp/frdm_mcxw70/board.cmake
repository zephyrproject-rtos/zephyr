# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(linkserver "--device=MCXW70AC:FRDM-MCXW70")
board_runner_args(jlink "--device=MCXW70AC" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

if(CONFIG_BUILD_WITH_TFM)
  # Flash merged TF-M + Zephyr binary
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()
