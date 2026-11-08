# SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(virtio_blk_pool)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/drivers/disk
  ${ZEPHYR_BASE}/drivers/virtio
  "${ZEPHYR_BASE}/tests/drivers/virtio/queue_submission/src"
)
target_sources(app PRIVATE
  src/main.c
  "${ZEPHYR_BASE}/tests/drivers/virtio/queue_submission/src/fake_device.c"
)
