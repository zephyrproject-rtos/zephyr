#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# Protocol API is only exposed to MCUmgr internals.
zephyr_library(mgmt_mcumgr_protocol)
zephyr_library_sources(src/smp.c)
