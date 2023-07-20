# Copyright (c) 2023 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(power.c)

zephyr_linker_sources(SECTIONS ish_aon.ld)
