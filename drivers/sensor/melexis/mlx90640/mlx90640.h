/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MELEXIS_MLX90640_MLX90640_H_
#define ZEPHYR_DRIVERS_SENSOR_MELEXIS_MLX90640_MLX90640_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/mlx90640.h>
#include <zephyr/sys/util.h>

#define MLX90640_REG_RAM           0x0400
#define MLX90640_REG_EEPROM        0x2400
#define MLX90640_REG_STATUS        0x8000
#define MLX90640_REG_CONTROL       0x800D
#define MLX90640_REG_DEVICE_ID     0x2407

#define MLX90640_EEPROM_WORDS      832
#define MLX90640_RAM_WORDS         832
#define MLX90640_FRAME_WORDS       834

#define MLX90640_STATUS_NEW_DATA   BIT(3)
#define MLX90640_STATUS_SUBPAGE    BIT(0)
#define MLX90640_STATUS_CLEAR      0x0030

#define MLX90640_CTRL_REFRESH_MASK GENMASK(9, 7)
#define MLX90640_CTRL_CHESS        BIT(12)
#define MLX90640_CTRL_SUBPAGES     BIT(0)

#define MLX90640_SCALE_ALPHA       0.000001f

#define MLX90640_MAX_BAD_PIXELS    5

struct mlx90640_params {
	int16_t k_vdd;
	int16_t vdd25;
	float kv_ptat;
	float kt_ptat;
	float v_ptat25;
	float alpha_ptat;
	float gain_ee;
	float tgc;
	float ks_ta;
	uint8_t resolution_ee;
	uint8_t calibration_mode_ee;
	float ks_to[5];
	int16_t ct[5];
	uint16_t alpha[MLX90640_PIXELS];
	uint8_t alpha_scale;
	int16_t offset[MLX90640_PIXELS];
	int8_t kta[MLX90640_PIXELS];
	uint8_t kta_scale;
	int8_t kv[MLX90640_PIXELS];
	uint8_t kv_scale;
	float cp_alpha[2];
	int16_t cp_offset[2];
	float cp_kta;
	float cp_kv;
	float il_chess_c[3];
	uint16_t broken[MLX90640_MAX_BAD_PIXELS];
	uint16_t outlier[MLX90640_MAX_BAD_PIXELS];
	uint8_t broken_count;
	uint8_t outlier_count;
};

struct mlx90640_config {
	struct i2c_dt_spec i2c;
	const struct device *vin_supply;
	uint8_t refresh_rate;
	uint32_t emissivity;
	int8_t ta_shift;
};

struct mlx90640_data {
	struct mlx90640_params params;
	uint16_t frame[MLX90640_FRAME_WORDS];
	float to[MLX90640_PIXELS];
	float ta;
	float emissivity;
	uint8_t refresh_rate;
};

int mlx90640_extract_params(struct mlx90640_params *p, const uint16_t *ee);
float mlx90640_calc_ta(const struct mlx90640_params *p, const uint16_t *frame);
void mlx90640_calc_to(const struct mlx90640_params *p, const uint16_t *frame, float emissivity,
		      float tr, float *to);

#endif /* ZEPHYR_DRIVERS_SENSOR_MELEXIS_MLX90640_MLX90640_H_ */
