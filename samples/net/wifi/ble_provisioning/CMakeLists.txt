# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wifi_ble_provisioning)

target_sources(app PRIVATE
  src/main.c
  src/gatt_svc.c
  src/wifi_prov.c
)
