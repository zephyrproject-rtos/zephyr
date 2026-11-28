# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS "$ENV{ZEPHYR_BASE}")
project(wifi_nan_hwsim)

target_sources(app PRIVATE
  src/main.c
)

# Needed only for the shared-iface scenario's topology-aware helpers, which
# use the hwsim driver's own private accessor (hwsim_get_secondary_iface())
# to resolve the NAN role's net_if -- net_if_lookup_by_dev() is ambiguous
# once a device backs more than one net_if.
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/wifi/hwsim)
