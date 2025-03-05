#
# Copyright (C) 2022-2025, Xiaohua Semiconductor Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)
zephyr_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_sources(soc.c)
