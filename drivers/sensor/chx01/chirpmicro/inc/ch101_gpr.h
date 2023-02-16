/*
 * Copyright 2023 Chirp Microsystems. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file ch101_gpr.h
 *
 * @brief Internal definitions for the Chirp CH101 GPR sensor firmware.
 *
 * This file contains register offsets and other values for use with the CH101 GPR
 * sensor firmware.  These values are subject to change without notice.
 *
 * You should not need to edit this file or call the driver functions directly.  Doing so
 * will reduce your ability to benefit from future enhancements and releases from Chirp.
 *
 */

#ifndef CH101_GPR_H_
#define CH101_GPR_H_

#include "ch101.h"
#include "soniclib.h"
#include <stdint.h>

/* GPR firmware registers */
#define CH101_GPR_REG_OPMODE	    0x01
#define CH101_GPR_REG_TICK_INTERVAL 0x02
#define CH101_GPR_REG_PERIOD	    0x05
#define CH101_GPR_REG_CAL_TRIG	    0x06
#define CH101_GPR_REG_CAL_TRIG	    0x06
#define CH101_GPR_REG_MAX_RANGE	    0x07
#define CH101_GPR_REG_CALC	    0x08
#define CH101_GPR_REG_REV_CYCLES    0x0C
#define CH101_GPR_REG_DCO_PERIOD    0x0E
#define CH101_GPR_REG_ST_RANGE	    0x12
#define CH101_GPR_REG_READY	    0x14
#define CH101_GPR_REG_TOF_SF	    0x16
#define CH101_GPR_REG_TOF	    0x18
#define CH101_GPR_REG_AMPLITUDE	    0x1A
#define CH101_GPR_REG_CAL_RESULT    0x0A
#define CH101_GPR_REG_DATA	    0x1C

#define CH101_GPR_MAX_SAMPLES (225)

extern const char *ch101_gpr_version;
extern const uint8_t ch101_gpr_fw[CH101_FW_SIZE];

uint16_t get_ch101_gpr_fw_ram_init_addr(void);
uint16_t get_ch101_gpr_fw_ram_init_size(void);

const unsigned char *get_ram_ch101_gpr_init_ptr(void);

uint8_t ch101_gpr_init(ch_dev_t *dev_ptr, ch_group_t *grp_ptr, uint8_t i2c_addr, uint8_t dev_num,
		       uint8_t i2c_bus_index);

#endif /* CH101_GPR_H_ */
