#**************************************************************************
# Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
# an affiliate of Cypress Semiconductor Corporation.
# SPDX-Licence-Identifier: Apache-2.0
#***************************************************************************

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

board_runner_args(pyocd "--target=cy8c6xx7_nosmif")
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
