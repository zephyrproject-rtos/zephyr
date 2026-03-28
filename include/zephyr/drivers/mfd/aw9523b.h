/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_AW9523B_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_AW9523B_H_

struct k_sem;
struct device;

#define AW9523B_REG_INPUT0  0x00
#define AW9523B_REG_INPUT1  0x01
#define AW9523B_REG_OUTPUT0 0x02
#define AW9523B_REG_OUTPUT1 0x03
#define AW9523B_REG_CONFIG0 0x04
#define AW9523B_REG_CONFIG1 0x05
#define AW9523B_REG_INT0    0x06
#define AW9523B_REG_INT1    0x07
#define AW9523B_REG_ID      0x10
#define AW9523B_REG_CTL     0x11
#define AW9523B_REG_MODE0   0x12
#define AW9523B_REG_MODE1   0x13
#define AW9523B_REG_DIM0    0x20
#define AW9523B_REG_DIM1    0x21
#define AW9523B_REG_DIM2    0x22
#define AW9523B_REG_DIM3    0x23
#define AW9523B_REG_DIM4    0x24
#define AW9523B_REG_DIM5    0x25
#define AW9523B_REG_DIM6    0x26
#define AW9523B_REG_DIM7    0x27
#define AW9523B_REG_DIM8    0x28
#define AW9523B_REG_DIM9    0x29
#define AW9523B_REG_DIM10   0x2A
#define AW9523B_REG_DIM11   0x2B
#define AW9523B_REG_DIM12   0x2C
#define AW9523B_REG_DIM13   0x2D
#define AW9523B_REG_DIM14   0x2E
#define AW9523B_REG_DIM15   0x2F
#define AW9523B_REG_SW_RSTN 0x7F

struct k_sem *aw9523b_get_lock(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_AW9523B_H_ */
