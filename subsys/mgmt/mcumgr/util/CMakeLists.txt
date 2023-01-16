#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# MCUmgr utilities, for use within the library.
# API interface for utilities is exposed by mgmt_mcumgr_util,
# and should not be exposed outside of mgmt_mcumgr.
zephyr_library(mgmt_mcumgr_util)
zephyr_library_sources(src/zcbor_bulk.c)

zephyr_include_directories(include)
