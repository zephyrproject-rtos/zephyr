#
# Copyright 2025-2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MCXA577")
board_runner_args(linkserver "--device=MCXA577:FRDM-MCXA577")

# Linkserver v25.12.xx and earlier do not include the secure regions in the
# MCXA577 memory map, so we add them here
board_runner_args(linkserver  "--override=/device/memory/-=\{\"location\":\"0x10000000\",\
                              \"size\":\"0x00200000\",\"type\":\"Flash\",\"flash-driver\":\"MCXA2TS_S.cfx\"\}")
board_runner_args(linkserver "--override=/device/memory/-=\{\"location\":\"0x30000000\",\
                             \"size\":\"0x0001C000\",\"type\":\"RAM\"\}")

board_runner_args(pyocd "--target=MCXA577")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)

if(CONFIG_BUILD_WITH_TFM)
  # Flash merged TF-M + Zephyr binary
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()