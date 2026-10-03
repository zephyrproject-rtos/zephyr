#
# Copyright 2020, 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_BOARD_MIMXRT685_EVK_MIMXRT685S_CM33)
  board_runner_args(jlink "--device=MIMXRT685S_M33" "--reset-after-load")
  board_runner_args(linkserver  "--device=MIMXRT685S:EVK-MIMXRT685")
elseif(CONFIG_BOARD_MIMXRT685_EVK_MIMXRT685S_HIFI4)
  # The DSP has no flash loader of its own, but its loadable segments live in
  # the shared FlexSPI window that the M33 core's loader can program. Flash the
  # DSP domain's own .hex through the M33 device so "west flash" programs it
  # natively (see the AMP DSP sysbuild flow), but point the GDB server at the
  # HiFi4 core so "west debug"/"west attach" land on the DSP itself.
  board_runner_args_flash(jlink "--device=MIMXRT685S_M33")
  board_runner_args_debug(jlink "--device=MIMXRT685S_HiFi4")
  board_runner_args_debugserver(jlink "--device=MIMXRT685S_HiFi4")
  board_runner_args_attach(jlink "--device=MIMXRT685S_HiFi4")
  board_runner_args(jlink "--reset-after-load")
  board_runner_args(linkserver  "--device=MIMXRT685S:EVK-MIMXRT685")
endif()

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
