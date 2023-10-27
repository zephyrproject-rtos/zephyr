# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

zephyr_cc_option(-march=pentium)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
