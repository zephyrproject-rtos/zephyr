# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# A board adds this directory when its devicetree enables the radio model, and
# implements bs_2g4_radio_platform.h for it.

target_sources(native_simulator INTERFACE
  "bs_2g4_radio.c"
)

target_compile_options(native_simulator INTERFACE
  "-I${CMAKE_CURRENT_SOURCE_DIR}"
)

zephyr_include_directories(
  "."
)
