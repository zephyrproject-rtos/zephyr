# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Helpers that can be used on bsim targets but don't have other
# dependencies (e.g. on Bluetooth, etc).
add_library(babblekit)

target_link_libraries(babblekit PUBLIC
  kernel
  zephyr_interface
)

target_include_directories(babblekit PUBLIC
  include
)

target_sources(babblekit PRIVATE
  src/sync.c
  src/device.c
)
