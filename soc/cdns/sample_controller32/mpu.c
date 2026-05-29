/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <xtensa/config/core-isa.h>

#include <zephyr/devicetree.h>
#include <zephyr/arch/xtensa/mpu.h>
#include <zephyr/sys/util.h>

const struct xtensa_mpu_range xtensa_soc_mpu_ranges[0];
const int xtensa_soc_mpu_ranges_num;
