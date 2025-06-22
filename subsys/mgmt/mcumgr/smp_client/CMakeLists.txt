#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# Protocol API is only exposed to MCUmgr internals.

zephyr_library()
zephyr_library_sources(src/client.c)
