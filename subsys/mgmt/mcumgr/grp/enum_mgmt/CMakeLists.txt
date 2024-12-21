#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2024 Extreme Engineering Solutions
#
# SPDX-License-Identifier: Apache-2.0

# Enumeration management group public API is exported by MCUmgr interface API,
# when Enumeration Management is enabled.
zephyr_library(mgmt_mcumgr_grp_enum)
zephyr_library_sources(src/enum_mgmt.c)

zephyr_library_include_directories(include)
