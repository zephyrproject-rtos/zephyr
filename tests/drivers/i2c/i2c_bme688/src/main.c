/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include "sensor.h"

#define SENSOR_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(bosch_bme680)
#define I2C_TEST_NODE DT_PARENT(SENSOR_NODE)
#define DEVICE_ADDRESS (uint8_t)DT_REG_ADDR(SENSOR_NODE)

static const struct device *const i2c_device = DEVICE_DT_GET(I2C_TEST_NODE);
static struct calibration_coeffs cal_coeffs;
static int32_t t_fine;

/* Read data from the senors register */
static uint8_t read_sensor_register(uint8_t register_address)
{
	int err;
	uint8_t response = 0;

	err = i2c_reg_read_byte(i2c_device, DEVICE_ADDRESS, register_address, &response);
	zassert_equal(err, 0, "i2c_read(%x)' failed with error: %d\n", register_address, err);
	TC_PRINT("I2C read reg, addr: 0x%x, val: 0x%x\n", register_address, response);
	return response;
}

/* Burst read data from the sensor registers */
static void burst_read_sensor_registers(uint8_t starting_register_address, uint8_t number_of_bytes,
					uint8_t *data_buffer)
{
	int err;

	zassert_true(number_of_bytes <= MAX_BURST_READ_SIZE,
		     "Too many bytes to read %d, max burst read size is set to: %d",
		     number_of_bytes, MAX_BURST_READ_SIZE);
	err = i2c_burst_read(i2c_device, DEVICE_ADDRESS, starting_register_address, data_buffer,
			     number_of_bytes);
	zassert_equal(err, 0, "i2c_burst_read(%x, %x)' failed with error: %d\n",
		      starting_register_address, number_of_bytes, err);
	TC_PRINT("I2C burst read, start addr: 0x%x, number of bytes: %d\n",
		 starting_register_address, number_of_bytes);
}

/* Write sensor register */
static void write_sensor_register(uint8_t register_address, int8_t value)
{
	int err;

	err = i2c_reg_write_byte(i2c_device, DEVICE_ADDRESS, register_address, value);
	zassert_equal(err, 0, "i2c_reg_write_byte(%x, %x)' failed with error: %d\n",
		      register_address, value, err);
	TC_PRINT("I2C reg write, addr: 0x%x, val: 0x%x\n", register_address, value);
}

/* Set IIR filter for the temperature and pressure measurements */
static void set_sensor_iir_filter(void)
{
	uint8_t response = 0;

	TC_PRINT("Set IIR filter\n");
	response = read_sensor_register(CONF_REGISTER_ADDRESS);
	response &= ~IIR_FILER_ORDER_BIT_MASK;
	response |= IIR_FILER_COEFF_3 << IIR_FILER_ORDER_BIT_SHIFT;
	write_sensor_register(CONF_REGISTER_ADDRESS, response);
	read_sensor_register(CONF_REGISTER_ADDRESS);
}

/* Read calibration coefficients for temperature, humifity and pressure */
static void read_calibration_coeffs(struct calibration_coeffs *coeffs)
{
	uint8_t register_data[MAX_BURST_READ_SIZE] = { 0 };

	/* Humidity */
	TC_PRINT("Reading humidity calibration coefficients\n");
	burst_read_sensor_registers(HUMI_PAR_REGISTERS_START_ADDRESS, HUMI_PAR_REGISTERS_COUNT,
				    register_data);
	coeffs->par_h1 = (uint16_t)(((uint16_t)register_data[HUMI_PAR_H1_MSB_BUF_POSITION] << 4) |
				    (register_data[HUMI_PAR_H1_LSB_BUF_POSITION] &
				     HUMI_PAR_H1_LSB_BIT_MASK));
	coeffs->par_h2 = (uint16_t)(((uint16_t)register_data[HUMI_PAR_H2_MSB_BUF_POSITION] << 4) |
				    ((register_data[HUMI_PAR_H2_LSB_BUF_POSITION]) >> 4));

	coeffs->par_h3 = (uint8_t)register_data[HUMI_PAR_H3_BUF_POSITION];
	coeffs->par_h4 = (uint8_t)register_data[HUMI_PAR_H4_BUF_POSITION];
	coeffs->par_h5 = (uint8_t)register_data[HUMI_PAR_H5_BUF_POSITION];
	coeffs->par_h6 = (uint8_t)register_data[HUMI_PAR_H6_BUF_POSITION];
	coeffs->par_h7 = (uint8_t)register_data[HUMI_PAR_H7_BUF_POSITION];

	/* Temperature */
	TC_PRINT("Reading temperature calibration coefficients\n");
	burst_read_sensor_registers(TEMP_PAR_T1_REGISTER_ADDRESS_LSB, 2, register_data);
	coeffs->par_t1 = (uint16_t)(((uint16_t)register_data[1] << 8) | (uint16_t)register_data[0]);
	burst_read_sensor_registers(TEMP_PAR_T2_REGISTER_ADDRESS_LSB, 2, register_data);
	coeffs->par_t2 = (uint16_t)(((uint16_t)register_data[1] << 8) | (uint16_t)register_data[0]);
	coeffs->par_t3 = (uint8_t)read_sensor_register(TEMP_PAR_T3_REGISTER_ADDRESS);

	/* Pressure */
	TC_PRINT("Reading pressure calibration coefficients\n");
	burst_read_sensor_registers(PRES_PAR_P1_REGISTER_ADDRESS_LSB, 4, register_data);
	coeffs->par_p1 = (uint16_t)(((uint16_t)register_data[1] << 8) | (uint16_t)register_data[0]);
	coeffs->par_p2 = (int16_t)(((uint16_t)register_data[3] << 8) | (uint16_t)register_data[2]);
	coeffs->par_p3 = (int8_t)read_sensor_register(PRES_PAR_P3_REGISTER_ADDRESS);
	burst_read_sensor_registers(PRES_PAR_P4_REGISTER_ADDRESS_LSB, 4, register_data);
	coeffs->par_p4 = (int16_t)(((uint16_t)register_data[1] << 8) | (uint16_t)register_data[0]);
	coeffs->par_p5 = (int16_t)(((uint16_t)register_data[3] << 8) | (uint16_t)register_data[2]);
	coeffs->par_p6 = (int8_t)read_sensor_register(PRES_PAR_P6_REGISTER_ADDRESS);
	coeffs->par_p7 = (int8_t)read_sensor_register(PRES_PAR_P7_REGISTER_ADDRESS);
	burst_read_sensor_registers(PRES_PAR_P8_REGISTER_ADDRESS_LSB, 4, register_data);
	coeffs->par_p8 = (int16_t)(((uint16_t)register_data[1] << 8) | (uint16_t)register_data[0]);
	coeffs->par_p9 = (int16_t)(((uint16_t)register_data[3] << 8) | (uint16_t)register_data[2]);
	coeffs->par_p10 = read_sensor_register(PRES_PAR_P10_REGISTER_ADDRESS);
}

/* Configure temperature, pressure and humidity measurements */
static void configure_measurements(void)
{
	unsigned char response = 0;

	TC_PRINT("Configure measurements\n");

	/* Humidity */
	response = read_sensor_register(CTRL_HUM_REGISTER_ADDRESS);
	response &= ~HUMIDITY_OVERSAMPLING_BIT_MSK;
	response |= HUMIDITY_OVERSAMPLING_1X << HUMIDITY_OVERSAMPLING_BIT_SHIFT;
	write_sensor_register(CTRL_HUM_REGISTER_ADDRESS, response);

	/* Temperature*/
	response = read_sensor_register(CTRL_MEAS_REGISTER_ADDRESS);
	response &= ~TEMP_OVERSAMPLING_BIT_MSK;
	response |= TEMPERATURE_OVERSAMPLING_2X << TEMP_OVERSAMPLING_BIT_SHIFT;

	write_sensor_register(CTRL_MEAS_REGISTER_ADDRESS, response);

	/* Pressure */
	response = read_sensor_register(CTRL_MEAS_REGISTER_ADDRESS);
	response &= ~PRES_OVERSAMPLING_BIT_MSK;
	response |= PRESSURE_OVERSAMPLING_16X << PRES_OVERSAMPLING_BIT_SHIFT;

	write_sensor_register(CTRL_MEAS_REGISTER_ADDRESS, response);
	read_sensor_register(CTRL_MEAS_REGISTER_ADDRESS);
	set_sensor_iir_filter();
}

/* Set the sensor operation mode */
static void set_sensor_mode(uint8_t sensor_mode)
{
	unsigned char response = 0;

	TC_PRINT("Set sensor mode to: 0x%x\n", sensor_mode);

	response = read_sensor_register(CTRL_MEAS_REGISTER_ADDRESS);
	response &= ~CTRL_MEAS_MODE_BIT_MSK;
	response |= sensor_mode << CTRL_MEAS_MODE_BIT_SHIFT;
	write_sensor_register(CTRL_MEAS_REGISTER_ADDRESS, response);
	read_sensor_register(CTRL_MEAS_REGISTER_ADDRESS);
}

/* Read the raw ADC temperature measurement result */
static uint32_t read_adc_temperature(void)
{
	uint32_t adc_temperature = 0;

	TC_PRINT("Reading ADC temperature\n");
	adc_temperature = (uint32_t)(((uint32_t)read_sensor_register(TEMP_ADC_DATA_MSB_0) << 12) |
				     ((uint32_t)read_sensor_register(TEMP_ADC_DATA_LSB_0) << 4) |
				     ((uint32_t)read_sensor_register(TEMP_ADC_DATA_XLSB_0) >> 4));

	return adc_temperature;
}

/* Read the raw ADC pressure measurement result */
static uint32_t read_adc_pressure(void)
{
	uint32_t pres_adc = 0;

	TC_PRINT("Reading ADC pressure\n");
	pres_adc = (uint32_t)(((uint32_t)read_sensor_register(PRES_ADC_DATA_MSB_0) << 12) |
			      ((uint32_t)read_sensor_register(PRES_ADC_DATA_LSB_0) << 4) |
			      ((uint32_t)read_sensor_register(PRES_ADC_DATA_XLSB_0) >> 4));

	return pres_adc;
}

/* Read the raw ADC humidity measurement result */
static uint16_t read_adc_humidity(void)
{
	uint16_t hum_adc = 0;

	TC_PRINT("Reading ADC humidity\n");
	hum_adc = (uint16_t)(((uint16_t)read_sensor_register(HUM_ADC_DATA_MSB_0) << 8) |
			     (uint16_t)read_sensor_register(HUM_ADC_DATA_LSB_0));

	return hum_adc;
}

ZTEST(i2c_controller_to_sensor, test_i2c_basic_memory_read)
{
	int err;
	uint8_t entire_sensor_memory[SENSOR_MEMORY_SIZE_IN_BYTES] = { 0 };

	TC_PRINT("Device address 0x%x\n", DEVICE_ADDRESS);

	err = i2c_read(i2c_device, entire_sensor_memory, SENSOR_MEMORY_SIZE_IN_BYTES,
		       DEVICE_ADDRESS);
	zassert_equal(err, 0, "i2c_read' failed with error: %d\n", err);
}

ZTEST(i2c_controller_to_sensor, test_i2c_controlled_sensor_operation)
{
	int err;
	uint8_t response = 0;
	int16_t temperature = 0;
	uint32_t pressure = 0;
	uint32_t humidity = 0;
	uint32_t i2c_config = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;
	uint8_t measurements_left = MEASUREMENT_CYCLES + 1;

	TC_PRINT("Device address 0x%x\n", DEVICE_ADDRESS);

	err = i2c_configure(i2c_device, i2c_config);
	zassert_equal(err, 0, "i2c_configure' failed with error: %d\n", err);

	response = read_sensor_register(CHIP_ID_REGISTER_ADDRESS);
	TC_PRINT("Chip_Id: %d\n", response);

	response = read_sensor_register(VARIANT_ID_REGISTER_ADDRESS);
	TC_PRINT("Variant_Id: %d\n", response);

	write_sensor_register(RESET_REGISTER_ADDRESS, RESET_DEVICE);
	k_sleep(K_MSEC(SLEEP_TIME_MS));

	read_calibration_coeffs(&cal_coeffs);

	configure_measurements();
	set_sensor_mode(FORCED_MODE);

	while (measurements_left) {
		response = read_sensor_register(MEAS_STATUS_0_REG_ADDRESS);
		TC_PRINT("Meas status 0, meas in progress: %d, new data: %d\n",
			 response & MEASUREMENT_IN_PROGRESS_BIT_MASK,
			 response & MEASUREMENT_NEW_DATA_BIT_MASK);
		if (response & MEASUREMENT_NEW_DATA_BIT_MASK) {
			temperature =
				calculate_temperature(read_adc_temperature(), &t_fine, &cal_coeffs);
			pressure = calculate_pressure(read_adc_pressure(), t_fine, &cal_coeffs);
			humidity = calculate_humidity(read_adc_humidity(), t_fine, &cal_coeffs);
			TC_PRINT("Temperature: %d.%d C deg\n", temperature / 100,
				 temperature % 100);
			TC_PRINT("Pressure: %d hPa\n", pressure / 100);
			TC_PRINT("Humidity: %d %%\n", humidity / 1000);
			set_sensor_mode(FORCED_MODE);

			/* Check if the results are within reasonable ranges
			 * for laboratory room usage
			 * This is asserted to catch values
			 * that may be results of an erroneous
			 * bus operation (corrupted read or write)
			 */
			zassert_true(
				(temperature / 100 >= 5) && (temperature / 100 <= 55),
				"Temperature is outside of the allowed range for labroatory use");
			zassert_true((pressure / 100 >= 700) && (pressure / 100 <= 1300),
				     "Pressure is outside of the allowed range for labroatory use");
			zassert_true((humidity / 1000 >= 10) && (humidity / 1000 <= 90),
				     "Humidity is outside of the allowed range for labroatory use");
			measurements_left--;
		}
		k_sleep(K_MSEC(SLEEP_TIME_MS));
	}
}

/*
 * Test setup
 */
void *test_setup(void)
{
	zassert_true(device_is_ready(i2c_device), "i2c device is not ready");
	return NULL;
}

ZTEST_SUITE(i2c_controller_to_sensor, NULL, test_setup, NULL, NULL, NULL);
