/* Bosch BMP280 Pressure Sensor
 *
 * Copyright (c) 2023 Russ Webber
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BMP280_H
#define __BMP280_H

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>

#define BMP280_CHIP_ID1		0x56
#define BMP280_CHIP_ID2		0x57
#define BMP280_CHIP_ID3		0x58

#define BMP280_REG_CHIP_ID		0xD0
#define BMP280_REG_RESET		0xE0
#define BMP280_REG_STATUS		0xF3
#define BMP280_REG_CTRL_MEAS		0xF4
#define BMP280_REG_CONFIG		0xF5
#define BMP280_REG_RAW_PRESSURE		0xF7
#define BMP280_REG_RAW_TEMP		0xFA
#define BMP280_REG_COMPENSATION_PARAMS		0x88

#define BMP280_PWR_CTRL_MODE_SLEEP		0x0
#define BMP280_PWR_CTRL_MODE_FORCED		0x1
#define BMP280_PWR_CTRL_MODE_NORMAL		0x3

#define BMP280_SAMPLE_BUFFER_SIZE		(6)
#define BMP280_SAMPLE_TEMPERATURE_POS		3
#define BMP280_SAMPLE_PRESSURE_POS		0

#define BMP280_STATUS_MEASURING		BIT(3)
#define BMP280_STATUS_IM_UPDATE		BIT(0)

/* Oversampling resolution {1x, 2x, 4x, 8x, 16x} */
// #define BMP280_OSRS		{0x01, 0x02, 0x03, 0x04, 0x05}
const uint8_t BMP280_OSRS[] = {0x01, 0x02, 0x03, 0x04, 0x05};

#define BMP280_CMD_SOFT_RESET 0xB6

struct bmp280_sample {
	uint32_t raw_pressure;
	uint32_t raw_temp;
	int64_t temp_fine;
};

struct bmp280_cal_data {
	uint16_t t1;
	int16_t t2;
	int16_t t3;
	uint16_t p1;
	int16_t p2;
	int16_t p3;
	int16_t p4;
	int16_t p5;
	int16_t p6;
	int16_t p7;
	int16_t p8;
	int16_t p9;
} __packed;

struct bmp280_ctrl_meas {
	uint8_t power_mode : 2;
	uint8_t os_res_pressure : 3;
	uint8_t os_res_temp : 3;
} __packed;

struct bmp280_config_byte {
	uint8_t spi_3wire : 1;
	uint8_t reserved : 1;
	uint8_t iir_filter : 3;
	uint8_t t_standby : 3;
} __packed;

struct bmp280_config {
	struct i2c_dt_spec i2c;
};

struct bmp280_data {
	uint8_t odr;
	struct bmp280_cal_data cal;
	struct bmp280_sample sample;
	struct bmp280_ctrl_meas ctrl_meas;
	struct bmp280_config_byte config_byte;
};

int bmp280_reg_field_update(const struct device *dev,
			    uint8_t reg,
			    uint8_t mask,
			    uint8_t val);

#endif /* __BMP280_H */
