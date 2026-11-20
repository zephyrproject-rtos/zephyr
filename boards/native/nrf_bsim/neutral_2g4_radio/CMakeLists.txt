# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# The board adds this directory for the BabbleSim LLL, which uses the model.

target_sources(native_simulator INTERFACE
  "nbr_2g4_radio.c"
  "nbr_cntr.c"
)

target_compile_options(native_simulator INTERFACE
  "-I${CMAKE_CURRENT_SOURCE_DIR}"
)

zephyr_include_directories(
  "."
)
