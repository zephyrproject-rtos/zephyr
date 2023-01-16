#
# Copyright (c) 2021-2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library(mgmt_mcumgr_grp_zephyr)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_GRP_ZBASIC_STORAGE_ERASE src/basic_mgmt.c)
