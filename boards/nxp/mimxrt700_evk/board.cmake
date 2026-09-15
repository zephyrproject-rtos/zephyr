#
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_MIMXRT798S_CM33_CPU0 OR CONFIG_SECOND_CORE_MCUX)
  board_runner_args(jlink "--device=MIMXRT798S_M33_0" "--reset-after-load")
  board_runner_args(linkserver  "--device=MIMXRT798S:MIMXRT700-EVK")
  board_runner_args(linkserver  "--override=/device/memory/4=")
  board_runner_args(linkserver  "--core=cm33_core0")
elseif(CONFIG_SOC_MIMXRT798S_CM33_CPU1)
  board_runner_args(jlink "--device=MIMXRT798S_M33_1")
  board_runner_args(linkserver  "--device=MIMXRT798S:MIMXRT700-EVK")
  board_runner_args(linkserver  "--core=cm33_core1")
elseif(CONFIG_SOC_MIMXRT798S_HIFI4)
  # The DSP has no flash loader of its own, but its loadable segments live in
  # the shared FlexSPI window that the M33 core's loader can program. Flash the
  # DSP domain's own .hex through the M33 device so "west flash" programs it
  # natively (see the AMP DSP sysbuild flow), but point the GDB server at the
  # HiFi4 core so "west debug"/"west attach" land on the DSP itself.
  board_runner_args_flash(jlink "--device=MIMXRT798S_M33_0")
  board_runner_args_debug(jlink "--device=MIMXRT798S_HiFi4")
  board_runner_args_debugserver(jlink "--device=MIMXRT798S_HiFi4")
  board_runner_args_attach(jlink "--device=MIMXRT798S_HiFi4")
  board_runner_args(jlink "--reset-after-load")
endif()

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
