# SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(virtio_queue_submission)

target_sources(app PRIVATE
  src/main.c
  src/completion.c
  src/queue_free.c
  src/chain_submission.c
  src/fake_device.c
)

# The suite drives the real virtio ISR and inspects its private entry points.
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/virtio)

target_link_options(app PUBLIC -Wl,--wrap=k_work_flush)
