# Copyright (c) 2024 ANITRA system s.r.o.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(rtc_rv3028.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/rtc)
