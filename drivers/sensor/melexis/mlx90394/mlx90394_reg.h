/*
 * Copyright (c) 2024 Florian Weber <Florian.Weber@live.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MLX90394_MLX90394_REG_H
#define ZEPHYR_DRIVERS_SENSOR_MLX90394_MLX90394_REG_H

#include <zephyr/sys/util_macro.h>
/* REGISTERS */
/* Status and measurent output */
#define MLX90394_REG_STAT1 0x00
#define MLX90394_REG_BXL   0x01
#define MLX90394_REG_BXH   0x02
#define MLX90394_REG_BYL   0x03
#define MLX90394_REG_BYH   0x04
#define MLX90394_REG_BZL   0x05
#define MLX90394_REG_BZH   0x06
#define MLX90394_REG_STAT2 0x07
#define MLX90394_REG_TL    0x08
#define MLX90394_REG_TH    0x09

/* Who Am I registers */
#define MLX90394_REG_CID 0x0A
#define MLX90394_REG_DID 0x0B

/* Control registers */
#define MLX90394_REG_CTRL1 0x0E
#define MLX90394_REG_CTRL2 0x0F
#define MLX90394_REG_CTRL3 0x14
#define MLX90394_REG_CTRL4 0x15

/* Reset register */
#define MLX90394_REG_RST 0x11

/* Wake On Change registers  */
#define MLX90394_REG_WOC_XL 0x58
#define MLX90394_REG_WOC_XH 0x59
#define MLX90394_REG_WOC_YL 0x5A
#define MLX90394_REG_WOC_YH 0x5B
#define MLX90394_REG_WOC_ZL 0x5C
#define MLX90394_REG_WOC_ZH 0x5D

/* VALUES */
/* STAT1 values RO */
#define MLX90394_STAT1_DRDY    BIT(0)
#define MLX90394_STAT1_DOR     BIT(3)
#define MLX90394_STAT1_RT      BIT(3)
#define MLX90394_STAT1_INT     BIT(4)
#define MLX90394_STAT1_DEFAULT (MLX90394_STAT1_RT)

/* STAT2 values RO */
#define MLX90394_STAT2_HOVF_X  BIT(0)
#define MLX90394_STAT2_HOVF_Y  BIT(1)
#define MLX90394_STAT2_HOVF_Z  BIT(2)
#define MLX90394_STAT2_DOR     BIT(3)
#define MLX90394_STAT2_DEFAULT 0

/* Who-I-Am register values RO */
#define MLX90394_CID 0x94
#define MLX90394_DID 0xaa

/* Write this value to reset Register soft resets the chip RW */
#define MLX90394_RST 0x06

/* CTRL1 values RW */
#define MLX90394_CTRL1_X_EN_BIT    4
#define MLX90394_CTRL1_Y_EN_BIT    5
#define MLX90394_CTRL1_Z_EN_BIT    6
#define MLX90394_CTRL1_MODE        GENMASK(3, 0)
#define MLX90394_CTRL1_MODE_SINGLE 1
#define MLX90394_CTRL1_X_EN        BIT(MLX90394_CTRL1_X_EN_BIT)
#define MLX90394_CTRL1_Y_EN        BIT(MLX90394_CTRL1_Y_EN_BIT)
#define MLX90394_CTRL1_Z_EN        BIT(MLX90394_CTRL1_Z_EN_BIT)
#define MLX90394_CTRL1_SWOK        BIT(7)
#define MLX90394_CTRL1_PREP(MODE, X_EN, Y_EN, Z_EN, SWOK)                                          \
	(FIELD_PREP(MLX90394_CTRL1_MODE, MODE) | FIELD_PREP(MLX90394_CTRL1_X_EN, X_EN) |           \
	 FIELD_PREP(MLX90394_CTRL1_Y_EN, Y_EN) | FIELD_PREP(MLX90394_CTRL1_Z_EN, Z_EN) |           \
	 FIELD_PREP(MLX90394_CTRL1_SWOK, SWOK))
#define MLX90394_CTRL1_DEFAULT MLX90394_CTRL1_PREP(0, 1, 1, 1, 0)

/* CTRL2 values RW */
enum mlx90394_reg_config_val {
	MLX90394_CTRL2_CONFIG_HIGH_RANGE_LOW_CURRENT = 0,
	MLX90394_CTRL2_CONFIG_HIGH_RANGE_LOW_NOISE,
	MLX90394_CTRL2_CONFIG_HIGH_SENSITIVITY_LOW_NOISE
};
#define MLX90394_CTRL2_WOC_MODE   GENMASK(1, 0)
#define MLX90394_CTRL2_INTREPB    BIT(2)
#define MLX90394_CTRL2_INTB_SCL_B BIT(3)
#define MLX90394_CTRL2_INTDUR     GENMASK(5, 4)
#define MLX90394_CTRL2_CONFIG     GENMASK(7, 6)
#define MLX90394_CTRL2_PREP(WOC_MODE, INTREPB, INTB_SCL_B, INTDUR, CONFIG)                         \
	(FIELD_PREP(MLX90394_CTRL2_WOC_MODE, WOC_MODE) |                                           \
	 FIELD_PREP(MLX90394_CTRL2_INTREPB, INTREPB) |                                             \
	 FIELD_PREP(MLX90394_CTRL2_INTB_SCL_B, INTB_SCL_B) |                                       \
	 FIELD_PREP(MLX90394_CTRL2_INTDUR, INTDUR) | FIELD_PREP(MLX90394_CTRL2_CONFIG, CONFIG))
#define MLX90394_CTRL2_DEFAULT                                                                     \
	MLX90394_CTRL2_PREP(0, 0, 1, 0, MLX90394_CTRL2_CONFIG_HIGH_RANGE_LOW_NOISE)

/* CTRL3 values RW */
#define MLX90394_CTRL3_DIG_FILT_TEMP    GENMASK(2, 0)
#define MLX90394_CTRL3_DIG_FILT_HALL_XY GENMASK(5, 3)
#define MLX90394_CTRL3_OSR_TEMP         BIT(6)
#define MLX90394_CTRL3_OSR_HALL         BIT(7)
#define MLX90394_CTRL3_PREP(DIG_FILT_TEMP, DIG_FILT_HALL_XY, OSR_TEMP, OSR_HALL)                   \
	(FIELD_PREP(MLX90394_CTRL3_DIG_FILT_TEMP, DIG_FILT_TEMP) |                                 \
	 FIELD_PREP(MLX90394_CTRL3_DIG_FILT_HALL_XY, DIG_FILT_HALL_XY) |                           \
	 FIELD_PREP(MLX90394_CTRL3_OSR_TEMP, OSR_TEMP) |                                           \
	 FIELD_PREP(MLX90394_CTRL3_OSR_HALL, OSR_HALL))
#define MLX90394_CTRL3_DEFAULT MLX90394_CTRL3_PREP(1, 4, 1, 1)

/* CTRL4 values RW BIT(6) has to be always 0 so it is not included here */
#define MLX90394_CTRL4_T_EN_BIT        5
#define MLX90394_CTRL4_DIG_FILT_HALL_Z GENMASK(2, 0)
#define MLX90394_CTRL4_DRDY_EN         BIT(3)
#define MLX90394_CTRL4_T_EN            BIT(MLX90394_CTRL4_T_EN_BIT)
#define MLX90394_CTRL4_PREP(DIG_FILT_HALL_Z, DRDY_EN, T_EN)                                        \
	(FIELD_PREP(MLX90394_CTRL4_DIG_FILT_HALL_Z, DIG_FILT_HALL_Z) |                             \
	 FIELD_PREP(MLX90394_CTRL4_DRDY_EN, DRDY_EN) | FIELD_PREP(MLX90394_CTRL4_T_EN, T_EN) |     \
	 BIT(4) | BIT(7))
#define MLX90394_CTRL4_DEFAULT MLX90394_CTRL4_PREP(5, 0, 0)

/* helper function to modify only one field */
#define MLX90394_FIELD_MOD(mask, new_field_val, val)                                               \
	((val & ~mask) | FIELD_PREP(mask, new_field_val))

#endif /* ZEPHYR_DRIVERS_SENSOR_MLX90394_MLX90394_REG_H */
