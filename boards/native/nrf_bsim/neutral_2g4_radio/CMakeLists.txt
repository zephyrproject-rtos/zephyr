# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# The board adds this directory when its devicetree enables the radio model.

target_sources(native_simulator INTERFACE
  "nbr_2g4_radio.c"
  "nbr_cntr.c"
)

# phy_sync_ctrl.h is in the common folder of the board
target_compile_options(native_simulator INTERFACE
  "-I${CMAKE_CURRENT_SOURCE_DIR}"
  "-I${CMAKE_CURRENT_SOURCE_DIR}/../common"
)

zephyr_include_directories(
  "."
)
