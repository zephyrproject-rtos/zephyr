# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(host_cmd)

target_sources(app PRIVATE
  src/driver.c
  src/deadline.c
  src/disconnect.c
  src/op.c
)

# The asynchronous command API is internal to the Host
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/bluetooth/host)
