#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# Protocol API is only exposed to MCUmgr internals.

zephyr_library(mgmt_mcumgr_client_protocol)
zephyr_library_sources(src/client.c)
