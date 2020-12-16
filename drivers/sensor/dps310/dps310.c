/* Driver for Infineon DPS310 temperature and pressure sensor */

/*
 * Copyright (c) 2019 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_dps310

#include <kernel.h>
#include <drivers/sensor.h>
#include <init.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include <drivers/i2c.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(DPS310, CONFIG_SENSOR_LOG_LEVEL);

/* Register addresses as in the datasheet */
#define IFX_DPS310_REG_ADDR_PSR_B2			(0x00)
#define IFX_DPS310_REG_ADDR_TMP_B2			(0x03)
#define IFX_DPS310_REG_ADDR_PRS_CFG			(0x06)
#define IFX_DPS310_REG_ADDR_TMP_CFG			(0x07)
#define IFX_DPS310_REG_ADDR_MEAS_CFG			(0x08)
#define IFX_DPS310_REG_ADDR_CFG_REG			(0x09)
#define IFX_DPS310_REG_ADDR_INT_STS			(0x0A)
#define IFX_DPS310_REG_ADDR_FIFO_STS			(0x0B)
#define IFX_DPS310_REG_ADDR_RESET			(0x0C)
#define IFX_DPS310_REG_ADDR_PRODUCT_ID			(0x0D)
#define IFX_DPS310_REG_ADDR_COEF_0			(0x10)
#define IFX_DPS310_REG_ADDR_COEF_SRCE			(0x28)

enum {
	IFX_DPS310_MODE_IDLE			=	0x00,
	IFX_DPS310_MODE_COMMAND_PRESSURE	=	0x01,
	IFX_DPS310_MODE_COMMAND_TEMPERATURE	=	0x02,
	IFX_DPS310_MODE_BACKGROUND_PRESSURE	=	0x05,
	IFX_DPS310_MODE_BACKGROUND_TEMPERATURE	=	0x06,
	IFX_DPS310_MODE_BACKGROUND_ALL		=	0x07
};

/* Bits in registers as in the datasheet */
#define IFX_DPS310_REG_ADDR_MEAS_CFG_PRS_RDY		(0x10)
#define IFX_DPS310_REG_ADDR_MEAS_CFG_TMP_RDY		(0x20)
/*
 * If sensor is ready after self initialization
 * bits 6 and 7 in register MEAS_CFG (0x08) should be "1"
 */
#define IFX_DPS310_REG_ADDR_MEAS_CFG_SELF_INIT_OK	(0xC0)
#define IFX_DPS310_COEF_SRCE_MASK			(0x80)
#define IFX_DPS310_PRODUCT_ID				(0x10)

/* Polling time in ms*/
#define POLL_TIME_MS					(K_MSEC(10))
/* Number of times to poll before timeout */
#define POLL_TRIES					3

/*
 * Measurement times in ms for different oversampling rates
 * From Table 16 in the datasheet, rounded up for safety margin
 */
enum {
	IFX_DPS310_MEAS_TIME_OSR_1	= 4,
	IFX_DPS310_MEAS_TIME_OSR_2	= 6,
	IFX_DPS310_MEAS_TIME_OSR_4	= 9,
	IFX_DPS310_MEAS_TIME_OSR_8	= 15,
	IFX_DPS310_MEAS_TIME_OSR_16	= 28,
	IFX_DPS310_MEAS_TIME_OSR_32	= 54,
	IFX_DPS310_MEAS_TIME_OSR_64	= 105,
	IFX_DPS310_MEAS_TIME_OSR_128	= 207
};

/* Compensation scale factors from Table 9 in the datasheet */
enum {
	IFX_DPS310_SF_OSR_1		= 524288,
	IFX_DPS310_SF_OSR_2		= 1572864,
	IFX_DPS310_SF_OSR_4		= 3670016,
	IFX_DPS310_SF_OSR_8		= 7864320,
	IFX_DPS310_SF_OSR_16		= 253952,
	IFX_DPS310_SF_OSR_32		= 516096,
	IFX_DPS310_SF_OSR_64		= 1040384,
	IFX_DPS310_SF_OSR_128		= 2088960
};

/*
 * Oversampling and measurement rates configuration for pressure and temperature
 * sensor According to Table 16 of the datasheet
 */
enum {
	IFX_DPS310_RATE_1		= 0x00,
	IFX_DPS310_RATE_2		= 0x01,
	IFX_DPS310_RATE_4		= 0x02,
	IFX_DPS310_RATE_8		= 0x03,
	IFX_DPS310_RATE_16		= 0x04,
	IFX_DPS310_RATE_32		= 0x05,
	IFX_DPS310_RATE_64		= 0x06,
	IFX_DPS310_RATE_128		= 0x07
};

/* Helper macro to set temperature and pressure config register */
#define CFG_REG(MEAS_RATE, OSR_RATE)                                           \
	((((MEAS_RATE)&0x07) << 4) | ((OSR_RATE)&0x07))

/* Setup constants depending on temperature oversampling factor */
#if defined CONFIG_DPS310_TEMP_OSR_1X
#define IFX_DPS310_SF_TMP		IFX_DPS310_SF_OSR_1
#define IFX_DPS310_TMP_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_1
#define IFX_DPS310_TMP_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_1)
#define IFX_DPS310_T_SHIFT		0
#elif defined CONFIG_DPS310_TEMP_OSR_2X
#define IFX_DPS310_SF_TMP		IFX_DPS310_SF_OSR_2
#define IFX_DPS310_TMP_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_2
#define IFX_DPS310_TMP_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_2)
#define IFX_DPS310_T_SHIFT		0
#elif defined CONFIG_DPS310_TEMP_OSR_4X
#define IFX_DPS310_SF_TMP		IFX_DPS310_SF_OSR_4
#define IFX_DPS310_TMP_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_4
#define IFX_DPS310_TMP_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_4)
#define IFX_DPS310_T_SHIFT		0
#elif defined CONFIG_DPS310_TEMP_OSR_8X
#define IFX_DPS310_SF_TMP		IFX_DPS310_SF_OSR_8
#define IFX_DPS310_TMP_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_8
#define IFX_DPS310_TMP_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_8)
#define IFX_DPS310_T_SHIFT		0
#elif defined CONFIG_DPS310_TEMP_OSR_16X
#define IFX_DPS310_SF_TMP		IFX_DPS310_SF_OSR_16
#define IFX_DPS310_TMP_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_16
#define IFX_DPS310_TMP_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_16)
#define IFX_DPS310_T_SHIFT		1
#elif defined CONFIG_DPS310_TEMP_OSR_32X
#define IFX_DPS310_SF_TMP		IFX_DPS310_SF_OSR_32
#define IFX_DPS310_TMP_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_32
#define IFX_DPS310_TMP_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_32)
#define IFX_DPS310_T_SHIFT		1
#elif defined CONFIG_DPS310_TEMP_OSR_64X
#define IFX_DPS310_SF_TMP		IFX_DPS310_SF_OSR_64
#define IFX_DPS310_TMP_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_64
#define IFX_DPS310_TMP_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_64)
#define IFX_DPS310_T_SHIFT		1
#elif defined CONFIG_DPS310_TEMP_OSR_128X
#define IFX_DPS310_SF_TMP		IFX_DPS310_SF_OSR_128
#define IFX_DPS310_TMP_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_128
#define IFX_DPS310_TMP_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_128)
#define IFX_DPS310_T_SHIFT		1
#endif

/* Setup constants depending on pressure oversampling factor */
#if defined CONFIG_DPS310_PRESS_OSR_1X
#define IFX_DPS310_SF_PSR		IFX_DPS310_SF_OSR_1
#define IFX_DPS310_PSR_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_1
#define IFX_DPS310_PSR_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_1)
#define IFX_DPS310_P_SHIFT		0
#elif defined CONFIG_DPS310_PRESS_OSR_2X
#define IFX_DPS310_SF_PSR		IFX_DPS310_SF_OSR_2
#define IFX_DPS310_PSR_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_2
#define IFX_DPS310_PSR_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_2)
#define IFX_DPS310_P_SHIFT		0
#elif defined CONFIG_DPS310_PRESS_OSR_4X
#define IFX_DPS310_SF_PSR		IFX_DPS310_SF_OSR_4
#define IFX_DPS310_PSR_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_4
#define IFX_DPS310_PSR_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_4)
#define IFX_DPS310_P_SHIFT		0
#elif defined CONFIG_DPS310_PRESS_OSR_8X
#define IFX_DPS310_SF_PSR		IFX_DPS310_SF_OSR_8
#define IFX_DPS310_PSR_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_8
#define IFX_DPS310_PSR_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_8)
#define IFX_DPS310_P_SHIFT		0
#elif defined CONFIG_DPS310_PRESS_OSR_16X
#define IFX_DPS310_SF_PSR		IFX_DPS310_SF_OSR_16
#define IFX_DPS310_PSR_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_16
#define IFX_DPS310_PSR_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_16)
#define IFX_DPS310_P_SHIFT		1
#elif defined CONFIG_DPS310_PRESS_OSR_32X
#define IFX_DPS310_SF_PSR		IFX_DPS310_SF_OSR_32
#define IFX_DPS310_PSR_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_32
#define IFX_DPS310_PSR_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_32)
#define IFX_DPS310_P_SHIFT		1
#elif defined CONFIG_DPS310_PRESS_OSR_64X
#define IFX_DPS310_SF_PSR		IFX_DPS310_SF_OSR_64
#define IFX_DPS310_PSR_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_64
#define IFX_DPS310_PSR_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_64)
#define IFX_DPS310_P_SHIFT		1
#elif defined CONFIG_DPS310_PRESS_OSR_128X
#define IFX_DPS310_SF_PSR		IFX_DPS310_SF_OSR_128
#define IFX_DPS310_PSR_MEAS_TIME	IFX_DPS310_MEAS_TIME_OSR_128
#define IFX_DPS310_PSR_CFG		CFG_REG(IFX_DPS310_RATE_1, IFX_DPS310_RATE_128)
#define IFX_DPS310_P_SHIFT		1
#endif

#define DPS310_CFG_REG                                                         \
	(((IFX_DPS310_T_SHIFT & 0x01) << 3) |                                  \
	 ((IFX_DPS310_P_SHIFT & 0x01) << 2))

#define HW_BUG_FIX_SEQUENCE_LEN	5

#define POW_2_23_MINUS_1	BIT_MASK(23)
#define POW_2_24		BIT(24)
#define POW_2_15_MINUS_1	BIT_MASK(15)
#define POW_2_16		BIT(16)
#define POW_2_11_MINUS_1	BIT_MASK(11)
#define POW_2_12		BIT(12)
#define POW_2_20		BIT(20)
#define POW_2_19_MINUS_1	BIT_MASK(19)

/* Needed because the values are referenced by pointer. */
static const uint8_t REG_ADDR_MEAS_CFG = IFX_DPS310_REG_ADDR_MEAS_CFG;
static const uint8_t REG_ADDR_CALIB_COEFF_0 = IFX_DPS310_REG_ADDR_COEF_0;
static const uint8_t REG_ADDR_TMP_B2 = IFX_DPS310_REG_ADDR_TMP_B2;
static const uint8_t REG_ADDR_PSR_B2 = IFX_DPS310_REG_ADDR_PSR_B2;
static const uint8_t REG_ADDR_COEF_SRCE = IFX_DPS310_REG_ADDR_COEF_SRCE;

/* calibration coefficients */
struct dps310_cal_coeff {
	/* Pressure Sensor Calibration Coefficients */
	int32_t c00; /* 20bit */
	int32_t c10; /* 20bit */
	int16_t c01; /* 16bit */
	int16_t c11; /* 16bit */
	int16_t c20; /* 16bit */
	int16_t c21; /* 16bit */
	int16_t c30; /* 16bit */
	/* Temperature Sensor Calibration Coefficients */
	int16_t c0; /* 12bit */
	int16_t c1; /* 12bit */
};

struct dps310_data {
	const struct device *i2c_master;
	struct dps310_cal_coeff comp;
	/* Temperature Values */
	int32_t tmp_val1;
	int32_t tmp_val2;
	/* Last raw temperature value for temperature compensation */
	int32_t raw_tmp;
	/* Pressure Values */
	int32_t psr_val1;
	int32_t psr_val2;
};

struct dps310_cfg {
	char *i2c_bus_name;
	uint16_t i2c_addr;
};

/*
 * Convert the bytes from calibration memory to calibration coefficients
 * structure
 */
static void dps310_calib_coeff_creation(const uint8_t raw_coef[18],
					struct dps310_cal_coeff *comp)
{
	/* Temperature sensor compensation values */
	comp->c0 = (((uint16_t)raw_coef[0]) << 4) + (raw_coef[1] >> 4);
	/* coefficient is 2nd compliment */
	if (comp->c0 > POW_2_11_MINUS_1) {
		comp->c0 = comp->c0 - POW_2_12;
	}

	comp->c1 = (((uint16_t)(raw_coef[1] & 0x0F)) << 8) + raw_coef[2];
	/* coefficient is 2nd compliment */
	if (comp->c1 > POW_2_11_MINUS_1) {
		comp->c1 = comp->c1 - POW_2_12;
	}

	/* Pressure sensor compensation values */
	comp->c00 = (((uint32_t)raw_coef[3]) << 12) + (((uint16_t)raw_coef[4]) << 4) +
		    (raw_coef[5] >> 4);
	/* coefficient is 2nd compliment */
	if (comp->c00 > POW_2_19_MINUS_1) {
		comp->c00 = comp->c00 - POW_2_20;
	}

	comp->c10 = (((uint32_t)(raw_coef[5] & 0x0F)) << 16) +
		    (((uint16_t)raw_coef[6]) << 8) + raw_coef[7];
	/* coefficient is 2nd compliment */
	if (comp->c10 > POW_2_19_MINUS_1) {
		comp->c10 = comp->c10 - POW_2_20;
	}

	comp->c01 = (int16_t) sys_get_be16(&raw_coef[8]);
	comp->c11 = (int16_t) sys_get_be16(&raw_coef[10]);
	comp->c20 = (int16_t) sys_get_be16(&raw_coef[12]);
	comp->c21 = (int16_t) sys_get_be16(&raw_coef[14]);
	comp->c30 = (int16_t) sys_get_be16(&raw_coef[16]);
}

/* Poll one or multiple bits given by ready_mask in reg_addr */
static bool poll_rdy(struct dps310_data *data, const struct dps310_cfg *config,
		     uint8_t reg_addr, uint8_t ready_mask)
{
	/* Try only a finite number of times */
	for (int i = 0; i < POLL_TRIES; i++) {
		uint8_t reg = 0;
		int res = i2c_reg_read_byte(data->i2c_master, config->i2c_addr,
					 reg_addr, &reg);
		if (res < 0) {
			LOG_WRN("I2C error: %d", res);
			return false;
		}

		if ((reg & ready_mask) == ready_mask) {
			/* measurement is ready */
			return true;
		}

		/* give the sensor more time */
		k_sleep(POLL_TIME_MS);
	}

	return false;
}

/* Trigger a temperature measurement and wait until the result is stored */
static bool dps310_trigger_temperature(struct dps310_data *data,
				       const struct dps310_cfg *config)
{
	/* command to start temperature measurement */
	static const uint8_t tmp_meas_cmd[] = {
		IFX_DPS310_REG_ADDR_MEAS_CFG,
		IFX_DPS310_MODE_COMMAND_TEMPERATURE
	};

	/* trigger temperature measurement */
	int res = i2c_write(data->i2c_master, tmp_meas_cmd,
			    sizeof(tmp_meas_cmd), config->i2c_addr);
	if (res < 0) {
		LOG_WRN("I2C error: %d", res);
		return false;
	}

	/* give the sensor time to store measured values internally */
	k_msleep(IFX_DPS310_TMP_MEAS_TIME);

	if (!poll_rdy(data, config, IFX_DPS310_REG_ADDR_MEAS_CFG,
		      IFX_DPS310_REG_ADDR_MEAS_CFG_TMP_RDY)) {
		LOG_DBG("Poll timeout for temperature");
		return false;
	}

	return true;
}

/* Trigger a pressure measurement and wait until the result is stored */
static bool dps310_trigger_pressure(struct dps310_data *data,
				    const struct dps310_cfg *config)
{
	/* command to start pressure measurement */
	static const uint8_t psr_meas_cmd[] = {
		IFX_DPS310_REG_ADDR_MEAS_CFG,
		IFX_DPS310_MODE_COMMAND_PRESSURE
	};

	/* trigger pressure measurement */
	int res = i2c_write(data->i2c_master, psr_meas_cmd,
			    sizeof(psr_meas_cmd), config->i2c_addr);
	if (res < 0) {
		LOG_WRN("I2C error: %d", res);
		return false;
	}

	/* give the sensor time to store measured values internally */
	k_msleep(IFX_DPS310_PSR_MEAS_TIME);

	if (!poll_rdy(data, config, IFX_DPS310_REG_ADDR_MEAS_CFG,
		      IFX_DPS310_REG_ADDR_MEAS_CFG_PRS_RDY)) {
		LOG_DBG("Poll timeout for pressure");
		return false;
	}

	return true;
}

/*
 * function to fix a hardware problem on some devices
 * you have this bug if you measure around 60°C when temperature is around 20°C
 * call dps310_hw_bug_fix() directly in the init() function to fix this issue
 */
static void dps310_hw_bug_fix(struct dps310_data *data,
			      const struct dps310_cfg *config)
{
	/* setup the necessary 5 sequences to fix the hw bug */
	static const uint8_t hw_bug_fix_sequence[HW_BUG_FIX_SEQUENCE_LEN][2] = {
		/*
		 * First write valid signature on 0x0e and 0x0f
		 * to unlock address 0x62
		 */
		{ 0x0E, 0xA5 },
		{ 0x0F, 0x96 },
		/* Then update high gain value for Temperature */
		{ 0x62, 0x02 },
		/* Finally lock back the location 0x62 */
		{ 0x0E, 0x00 },
		{ 0x0F, 0x00 }
	};

	/* execute sequence for hw bug fix */
	for (int i = 0; i < HW_BUG_FIX_SEQUENCE_LEN; i++) {
		int res = i2c_write(data->i2c_master, hw_bug_fix_sequence[i], 2,
				    config->i2c_addr);
		if (res < 0) {
			LOG_WRN("I2C error: %d", res);
			return;
		}
	}
}

/*
 * Scale and compensate the raw temperature measurement value to micro °C
 * The formula is based on the Chapter 4.9.2 in the datasheet and was
 * modified to need only integer arithmetic.
 */
static void dps310_scale_temperature(int32_t tmp_raw, struct dps310_data *data)
{
	const struct dps310_cal_coeff *comp = &data->comp;

	/* first term, rescaled to micro °C */
	int32_t tmp_p0 = (1000000 / 2) * comp->c0;

	/* second term, rescaled to mirco °C */
	int32_t tmp_p1 =
		(((int64_t)1000000) * comp->c1 * tmp_raw) / IFX_DPS310_SF_TMP;

	/* calculate the temperature corresponding to the datasheet */
	int32_t tmp_final = tmp_p0 + tmp_p1; /* value is in micro °C */

	/* store calculated value */
	data->tmp_val1 = tmp_final / 1000000;
	data->tmp_val2 = tmp_final % 1000000;
}

/*
 * Scale and temperature compensate the raw pressure measurement value to
 * Kilopascal. The formula is based on the Chapter 4.9.1 in the datasheet.
 */
static void dps310_scale_pressure(int32_t tmp_raw, int32_t psr_raw,
				  struct dps310_data *data)
{
	const struct dps310_cal_coeff *comp = &data->comp;

	float psr = ((float)psr_raw) / IFX_DPS310_SF_PSR;
	float tmp = ((float)tmp_raw) / IFX_DPS310_SF_TMP;

	/* scale according to formula from datasheet */
	float psr_final = comp->c00;

	psr_final += psr * (comp->c10 + psr * (comp->c20 + psr * comp->c30));
	psr_final += tmp * comp->c01;
	psr_final += tmp * psr * (comp->c11 + psr * comp->c21);

	/* rescale from Pascal to Kilopascal */
	psr_final /= 1000;

	/* store calculated value */
	data->psr_val1 = psr_final;
	data->psr_val2 = (psr_final - data->psr_val1) * 1000000;
}

/* Convert the raw sensor data to int32_t */
static int32_t raw_to_int24(const uint8_t raw[3])
{
	/* convert from twos complement */
	int32_t res = (int32_t) sys_get_be24(raw);

	if (res > POW_2_23_MINUS_1) {
		res -= POW_2_24;
	}

	return res;
}

/* perform a single measurement of temperature and pressure */
static bool dps310_measure_tmp_psr(struct dps310_data *data,
				   const struct dps310_cfg *config)
{
	if (!dps310_trigger_temperature(data, config)) {
		return false;
	}

	if (!dps310_trigger_pressure(data, config)) {
		return false;
	}

	/* memory for pressure and temperature raw values */
	uint8_t value_raw[6];

	/* read pressure and temperature raw values in one continuous read */
	int res = i2c_write_read(data->i2c_master, config->i2c_addr,
				 &REG_ADDR_PSR_B2, 1, &value_raw,
				 sizeof(value_raw));
	if (res < 0) {
		LOG_WRN("I2C error: %d", res);
		return false;
	}

	/* convert raw data to int */
	int32_t psr_raw = raw_to_int24(&value_raw[0]);

	data->raw_tmp = raw_to_int24(&value_raw[3]);

	dps310_scale_temperature(data->raw_tmp, data);
	dps310_scale_pressure(data->raw_tmp, psr_raw, data);

	return true;
}

/*
 * perform a single pressure measurement
 * uses the stored temperature value for sensor temperature compensation
 * temperature must be measured regularly for good temperature compensation
 */
static bool dps310_measure_psr(struct dps310_data *data,
			       const struct dps310_cfg *config)
{
	/* measure pressure */
	if (!dps310_trigger_pressure(data, config)) {
		return false;
	}

	/* memory for pressure raw value */
	uint8_t value_raw[3];

	/* read pressure raw values in one continuous read */
	int res = i2c_write_read(data->i2c_master, config->i2c_addr,
				 &REG_ADDR_PSR_B2, 1, &value_raw,
				 sizeof(value_raw));
	if (res < 0) {
		LOG_WRN("I2C error: %d", res);
		return false;
	}

	/* convert raw data to int */
	int32_t psr_raw = raw_to_int24(&value_raw[0]);

	dps310_scale_pressure(data->raw_tmp, psr_raw, data);

	return true;
}

/* perform a single temperature measurement */
static bool dps310_measure_tmp(struct dps310_data *data,
			       const struct dps310_cfg *config)
{
	/* measure temperature */
	if (!dps310_trigger_temperature(data, config)) {
		return false;
	}

	/* memory for temperature raw value */
	uint8_t value_raw[3];

	/* read temperature raw values in one continuous read */
	int res = i2c_write_read(data->i2c_master, config->i2c_addr,
				 &REG_ADDR_TMP_B2, 1, &value_raw,
				 sizeof(value_raw));
	if (res < 0) {
		LOG_WRN("I2C error: %d", res);
		return false;
	}

	/* convert raw data to int */
	data->raw_tmp = raw_to_int24(&value_raw[0]);

	dps310_scale_temperature(data->raw_tmp, data);

	return true;
}

/* Initialize the sensor and apply the configuration */
static int dps310_init(const struct device *dev)
{
	struct dps310_data *data = dev->data;
	const struct dps310_cfg *config = dev->config;

	data->i2c_master = device_get_binding(config->i2c_bus_name);
	if (data->i2c_master == NULL) {
		LOG_ERR("Failed to get I2C device");
		return -EINVAL;
	}

	uint8_t product_id = 0;
	int res = i2c_reg_read_byte(data->i2c_master, config->i2c_addr,
				 IFX_DPS310_REG_ADDR_PRODUCT_ID, &product_id);

	if (res < 0) {
		LOG_ERR("No device found");
		return -EINVAL;
	}

	if (product_id != IFX_DPS310_PRODUCT_ID) {
		LOG_ERR("Device is not a DPS310");
		return -EINVAL;
	}

	LOG_DBG("Init DPS310");
	/* give the sensor time to load the calibration data */
	k_sleep(K_MSEC(40));

	/* wait for the sensor to load the calibration data */
	if (!poll_rdy(data, config, REG_ADDR_MEAS_CFG,
		      IFX_DPS310_REG_ADDR_MEAS_CFG_SELF_INIT_OK)) {
		LOG_DBG("Sensor not ready");
		return -EIO;
	}

	/* read calibration coefficients */
	uint8_t raw_coef[18] = { 0 };

	res = i2c_write_read(data->i2c_master, config->i2c_addr,
			     &REG_ADDR_CALIB_COEFF_0, 1, &raw_coef, 18);
	if (res < 0) {
		LOG_WRN("I2C error: %d", res);
		return -EIO;
	}

	/* convert calibration coefficients */
	dps310_calib_coeff_creation(raw_coef, &data->comp);

	/*
	 * check which temperature sensor was used for calibration and use it
	 * for measurements.
	 */
	uint8_t tmp_coef_srce = 0;

	res = i2c_write_read(data->i2c_master, config->i2c_addr,
			     &REG_ADDR_COEF_SRCE, 1, &tmp_coef_srce, 18);
	if (res < 0) {
		LOG_WRN("I2C error: %d", res);
		return -EIO;
	}

	/* clear all other bits */
	tmp_coef_srce &= IFX_DPS310_COEF_SRCE_MASK;

	/* merge with temperature measurement configuration */
	tmp_coef_srce |= IFX_DPS310_TMP_CFG;

	/* set complete configuration in one write */
	const uint8_t config_seq[] = {
		IFX_DPS310_REG_ADDR_PRS_CFG,	/* start register address */
		IFX_DPS310_PSR_CFG,		/* PSR_CFG */
		tmp_coef_srce,			/* TMP_CFG */
		0x00,				/* MEAS_CFG */
		DPS310_CFG_REG			/* CFG_REG */
	};

	res = i2c_write(data->i2c_master, config_seq, sizeof(config_seq),
			config->i2c_addr);
	if (res < 0) {
		LOG_WRN("I2C error: %d", res);
		return -EIO;
	}

	dps310_hw_bug_fix(data, config);
	dps310_measure_tmp_psr(data, config);

	LOG_DBG("Init OK");
	return 0;
}

/* Do a measurement and fetch the data from the sensor */
static int dps310_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct dps310_data *data = dev->data;
	const struct dps310_cfg *config = dev->config;

	LOG_DBG("Fetching sample from DPS310");

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		if (!dps310_measure_tmp(data, config)) {
			LOG_ERR("Failed to measure temperature");
			return -EIO;
		}
		break;
	case SENSOR_CHAN_PRESS:
		if (!dps310_measure_psr(data, config)) {
			LOG_ERR("Failed to measure pressure");
			return -EIO;
		}
		break;
	case SENSOR_CHAN_ALL:
		if (!dps310_measure_tmp_psr(data, config)) {
			LOG_ERR("Failed to measure temperature and pressure");
			return -EIO;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* Get the measurement data */
static int dps310_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct dps310_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		val->val1 = data->tmp_val1;
		val->val2 = data->tmp_val2;
		break;
	case SENSOR_CHAN_PRESS:
		val->val1 = data->psr_val1;
		val->val2 = data->psr_val2;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api dps310_api_funcs = {
	.sample_fetch = dps310_sample_fetch,
	.channel_get = dps310_channel_get,
};

/* support up to 2 instances */

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)
static struct dps310_data dps310_data_0;
static const struct dps310_cfg dps310_cfg_0 = {
	.i2c_bus_name = DT_INST_BUS_LABEL(0),
	.i2c_addr = DT_INST_REG_ADDR(0)
};

DEVICE_DT_INST_DEFINE(0, dps310_init, device_pm_control_nop,
		    &dps310_data_0, &dps310_cfg_0, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &dps310_api_funcs);
#endif

#if DT_NODE_HAS_STATUS(DT_DRV_INST(1), okay)
static struct dps310_data dps310_data_1;
static const struct dps310_cfg dps310_cfg_1 = {
	.i2c_bus_name = DT_INST_BUS_LABEL(1),
	.i2c_addr = DT_INST_REG_ADDR(1)
};

DEVICE_DT_INST_DEFINE(1, dps310_init, device_pm_control_nop,
		    &dps310_data_1, &dps310_cfg_1, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &dps310_api_funcs);
#endif
