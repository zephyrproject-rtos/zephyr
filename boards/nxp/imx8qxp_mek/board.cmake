#
# Copyright (c) 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_MIMX8QX6_ADSP)
  board_set_flasher_ifnset(misc-flasher)
  board_finalize_runner_args(misc-flasher)

  board_set_rimage_target(imx8)
endif()
