# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_UWB_TRANSPORT_NATIVE_SIM)
  zephyr_library_sources(uwb_transport_native_sim.c)
endif()
