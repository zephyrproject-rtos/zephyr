# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

add_library(testlib)

target_link_libraries(testlib PUBLIC
  kernel
  subsys__bluetooth__host
  zephyr_interface
)

target_include_directories(testlib PUBLIC
  include
)

target_sources(testlib PRIVATE
  src/adv.c
  src/att.c
  src/att_read.c
  src/att_write.c
  src/conn_ref.c
  src/conn_wait.c
  src/connect.c
  src/disconnect.c
  src/scan.c
)

target_sources_ifdef(CONFIG_BT_SMP testlib PRIVATE
  src/security.c
)
