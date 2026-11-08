# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

add_library(fake_hci)

target_link_libraries(fake_hci PUBLIC
  kernel
  zephyr_interface
)

target_include_directories(fake_hci PUBLIC
  include
)

target_sources(fake_hci PRIVATE
  src/fake_hci.c
)
