# Copyright (c) 2020 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  gdbstub.c
  )

zephyr_sources_ifdef(
  CONFIG_GDBSTUB_SERIAL_BACKEND
  gdbstub_backend_serial.c
  )
