/*
 * SPDX-FileCopyrightText: Copyright (c) Michele Tavecchio <tavecchiomichele03@gmail.com>
 * SPDX-FileCopyrightText: Copyright (c) Alessandro Sola <alessandrosola03@gmail.com>
 * SPDX-FileCopyrightText: Copyright (c) 2017 Melexis N.V.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MLX90632_MLX90632_H_
#define ZEPHYR_DRIVERS_SENSOR_MLX90632_MLX90632_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>

#define MLX90632_STARTUP_TIME_MS 64
#define MLX90632_RESET_DELAY_US  150

#define MLX90632_EE_ID0          0x2405
#define MLX90632_EE_ID1          0x2406
#define MLX90632_EE_ID2          0x2407
#define MLX90632_EE_PRODUCT_CODE 0x2409
#define MLX90632_DSPv5           0x05
#define MLX90632_EE_VERSION      0x240b

#define MLX90632_REG_STATUS     0x3FFF
#define MLX90632_STAT_EE_BUSY   BIT(9)
#define MLX90632_STAT_BROWN_OUT BIT(8)
#define MLX90632_STAT_DATA_RDY  BIT(0)
#define MLX90632_XTD_RNG_BIT    0x7F00
#define MLX90632_REG_CTRL       0x3001
#define MLX90632_BIT_SOB        BIT(11)

#define MLX90632_EE_P_R 0x240c
#define MLX90632_EE_P_G 0x240e
#define MLX90632_EE_P_T 0x2410
#define MLX90632_EE_P_O 0x2412
#define MLX90632_EE_Aa  0x2414
#define MLX90632_EE_Ab  0x2416
#define MLX90632_EE_Ba  0x2418
#define MLX90632_EE_Bb  0x241a
#define MLX90632_EE_Ca  0x241c
#define MLX90632_EE_Cb  0x241e
#define MLX90632_EE_Da  0x2420
#define MLX90632_EE_Db  0x2422
#define MLX90632_EE_Ea  0x2424
#define MLX90632_EE_Eb  0x2426
#define MLX90632_EE_Fa  0x2428
#define MLX90632_EE_Fb  0x242a
#define MLX90632_EE_Ga  0x242c
#define MLX90632_EE_Gb  0x242e
#define MLX90632_EE_Ka  0x242f
#define MLX90632_EE_Ha  0x2481
#define MLX90632_EE_Hb  0x2482

#define MLX90632_RAM_4 0x4003
#define MLX90632_RAM_5 0x4004
#define MLX90632_RAM_6 0x4005
#define MLX90632_RAM_7 0x4006
#define MLX90632_RAM_8 0x4007
#define MLX90632_RAM_9 0x4008

#define MLX90632_RAM_52 0x4033
#define MLX90632_RAM_53 0x4034
#define MLX90632_RAM_54 0x4035
#define MLX90632_RAM_55 0x4036
#define MLX90632_RAM_56 0x4037
#define MLX90632_RAM_57 0x4038
#define MLX90632_RAM_58 0x4039
#define MLX90632_RAM_59 0x403A
#define MLX90632_RAM_60 0x403B

#define MLX90632_EE_MEDICAL_MEAS1  0x24E1
#define MLX90632_EE_MEDICAL_MEAS2  0x24E2
#define MLX90632_EE_EXTENDED_MEAS1 0x24F1
#define MLX90632_EE_EXTENDED_MEAS2 0x24F2
#define MLX90632_EE_EXTENDED_MEAS3 0x24F3

#define MLX90632_REG_UNLOCK    0x3005
#define MLX90632_RESET_CMD     0x0006
#define MLX90632_EE_UNLOCK_KEY 0x554C
#define MLX90632_REG_I2C_ADDR  0x3000

struct mlx90632_config {
	struct i2c_dt_spec i2c;
};

struct mlx90632_data {
	int32_t p_r;
	int32_t p_g;
	int32_t p_t;
	int32_t p_o;
	int32_t aa;
	int32_t ab;
	int32_t ba;
	int32_t bb;
	int32_t ca;
	int32_t cb;
	int32_t da;
	int32_t db;
	int32_t ea;
	int32_t eb;
	int32_t fa;
	int32_t fb;
	int32_t ga;
	int16_t gb;
	int16_t ka;
	int16_t ha;
	int16_t hb;

	int16_t ram_4;
	int16_t ram_5;
	int16_t ram_6;
	int16_t ram_7;
	int16_t ram_8;
	int16_t ram_9;

	int16_t ram_52;
	int16_t ram_53;
	int16_t ram_54;
	int16_t ram_55;
	int16_t ram_56;
	int16_t ram_57;
	int16_t ram_58;
	int16_t ram_59;
	int16_t ram_60;

	int32_t ambient_temp;
	int32_t object_temp;

	uint8_t last_cycle;

	const struct device *dev;
	struct k_work_delayable data_work;
	struct k_sem data_sem;
	int work_ret;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_MLX90632_MLX90632_H_ */
