# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

ExternalZephyrProject_Add(
  APPLICATION secondary
  SOURCE_DIR ${APP_DIR}/secondary
  BOARD nrf54h20dk/nrf54h20/cpuapp
)
