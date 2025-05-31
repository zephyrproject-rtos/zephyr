# Copyright (c) 2025 Mario Paja
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ESP32_EXT_ANTENNA)
    zephyr_library()
    zephyr_library_sources(hpcore_init.c)
endif()
