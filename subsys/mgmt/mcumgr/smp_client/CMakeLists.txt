#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# Protocol API is only exposed to MCUmgr internals.

zephyr_library_named(mcumgr_smp_client)
zephyr_library_sources(src/client.c)
