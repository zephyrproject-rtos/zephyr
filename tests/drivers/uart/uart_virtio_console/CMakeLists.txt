# SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uart_virtio_console)

if(TEST_SMP_LOCK_ORDER)
  target_sources(app PRIVATE src/smp_lock_order.c)
  target_link_options(app PUBLIC "LINKER:--wrap=z_impl_k_event_clear")
else()
  target_sources(app PRIVATE src/main.c)
endif()
