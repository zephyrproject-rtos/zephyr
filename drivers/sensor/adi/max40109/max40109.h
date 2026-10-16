/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADI_MAX40109_MAX40109_H_
#define ZEPHYR_DRIVERS_SENSOR_ADI_MAX40109_MAX40109_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor/max40109.h>
#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>

/** Register addresses for MAX40109 */

#define MAX40109_REG_CONFIG_MSB                        0x00u
#define MAX40109_REG_CONFIG_LSB                        0x01u
#define MAX40109_REG_STATUS_MSB                        0x02u
#define MAX40109_REG_STATUS_LSB                        0x03u
#define MAX40109_PGA_PRESSURE_GAIN                     0x04u
#define MAX40109_CURRENT_SOURCE                        0x05u
#define MAX40109_UNCALIBRATED_PRESSURE_MSB             0x06u
#define MAX40109_UNCALIBRATED_PRESSURE_LSB             0x07u
#define MAX40109_UNCALIBRATED_TEMPERATURE_MSB          0x08u
#define MAX40109_UNCALIBRATED_TEMPERATURE_LSB          0x09u
#define MAX40109_ADC_SAMPLE_RATE                       0x0Au
#define MAX40109_INTERRUPT_ENABLE_MSB                  0x0Bu
#define MAX40109_INTERRUPT_ENABLE_LSB                  0x0Cu
#define MAX40109_BRIDGE_DRIVE                          0x0Du
#define MAX40109_PGA_TEMPERATURE_GAIN                  0x0Eu
#define MAX40109_CALIBRATED_PRESSURE_MSB               0x0Fu
#define MAX40109_CALIBRATED_PRESSURE_LSB               0x10u
#define MAX40109_CALIBRATED_TEMPERATURE_MSB            0x11u
#define MAX40109_CALIBRATED_TEMPERATURE_LSB            0x12u
#define MAX40109_TEMP_MODE                             0x13u
#define MAX40109_SENSOR_OFFSET_CAL_CONFIG              0x14u
#define MAX40109_ANALOG_FILTER_BW                      0x15u
#define MAX40109_ZERO_PRESSURE_OFFSET_RAM_OVERRIDE_MSB 0x1Au
#define MAX40109_ZERO_PRESSURE_OFFSET_RAM_OVERRIDE_LSB 0x1Bu
#define MAX40109_ZERO_PRESSURE_OFFSET_SELECT           0x1Cu
#define MAX40109_ANALOG_OUTPUT_STAGE                   0x1Eu
#define MAX40109_SLP_MR                                0x9Bu
#define MAX40109_SLP_MREF                              0x9Du
#define MAX40109_CP_CONTROL_1                          0x9Fu
#define MAX40109_CP_CONTROL_2                          0xA0u
#define MAX40109_MTP_CONTROL                           0xA2u
#define MAX40109_MTP_STATUS                            0xA3u
#define MAX40109_MTP_PROT_ADDR                         0xA4u
#define MAX40109_MTP_PROT_WDATA_MSB                    0xA5u
#define MAX40109_MTP_PROT_WDATA_LSB                    0xA6u
#define MAX40109_MTP_PROT_RDATA_MSB                    0xA7u
#define MAX40109_MTP_PROT_RDATA_LSB                    0xA8u
#define MAX40109_MTP_LEVEL_MSB                         0xA9u
#define MAX40109_MTP_LEVEL_LSB                         0xAAu
#define MAX40109_SLP_MRV_MSB                           0xABu
#define MAX40109_SLP_MRV_LSB                           0xACu
#define MAX40109_SLP_MREFV_MSB                         0xADu
#define MAX40109_SLP_MREFV_LSB                         0xAEu
#define MAX40109_MTP_DATA0_MSB                         0xAFu
#define MAX40109_MTP_DATA0_LSB                         0xB0u
#define MAX40109_MTP_ADDR                              0xB1u

/** CONFIG Register Masks */

#define MAX40109_MTP_ENABLE_MASK                        BIT(7)
#define MAX40109_TEMP_CAL_BYPASS_MASK                   BIT(6)
#define MAX40109_PRESSURE_CAL_BYPASS_MASK               BIT(5)
#define MAX40109_DIGITAL_FILTER_MASK                    GENMASK(4, 2)
#define MAX40109_REFIN_INTERNAL_EXTERNAL_SEL_MASK       BIT(1)
#define MAX40109_CURRENT_SOURCE_REFERENCE_RESISTOR_MASK BIT(0)
#define MAX40109_SHUTDOWN_MASK                          BIT(7)
#define MAX40109_TEMP_CURRENT_MASK                      GENMASK(5, 4)
#define MAX40109_ALERT_MODE_MASK                        GENMASK(3, 1)
#define MAX40109_PGA_INPUT_MASK                         BIT(0)

/** STATUS Register Masks */
#define MAX40109_DRV_FAULT_MASK           BIT(3)
#define MAX40109_INT_FAULT_MASK           BIT(2)
#define MAX40109_TEMP_DATA_READY_MASK     BIT(1)
#define MAX40109_PRESSURE_DATA_READY_MASK BIT(0)
#define MAX40109_UV_DRV_MASK              BIT(7)
#define MAX40109_OV_DRV_MASK              BIT(6)
#define MAX40109_UV_INT_MASK              BIT(5)
#define MAX40109_OV_INT_MASK              BIT(4)
#define MAX40109_UV_INP_MINUS_MASK        BIT(3)
#define MAX40109_OV_INP_MINUS_MASK        BIT(2)
#define MAX40109_UV_INP_PLUS_MASK         BIT(1)
#define MAX40109_OV_INP_PLUS_MASK         BIT(0)

/** INTERRUPT ENABLE Register Masks */
#define MAX40109_INT_TEMP_DATA_READY_EN_MASK     BIT(1)
#define MAX40109_INT_PRESSURE_DATA_READY_EN_MASK BIT(0)
#define MAX40109_INT_UV_DRV_EN_MASK              BIT(7)
#define MAX40109_INT_OV_DRV_EN_MASK              BIT(6)
#define MAX40109_INT_UV_INT_EN_MASK              BIT(5)
#define MAX40109_INT_OV_INT_EN_MASK              BIT(4)
#define MAX40109_INT_UV_INP_MINUS_EN_MASK        BIT(3)
#define MAX40109_INT_OV_INP_MINUS_EN_MASK        BIT(2)
#define MAX40109_INT_UV_INP_PLUS_EN_MASK         BIT(1)
#define MAX40109_INT_OV_INP_PLUS_EN_MASK         BIT(0)

/** MTP Status Register Masks */
#define MAX40109_MTP_BURN_DONE_MASK         BIT(7)
#define MAX40109_MTP_ECC_ERR_2_BIT_MASK     BIT(6)
#define MAX40109_MTP_ECC_ERR_1_BIT_MASK     BIT(5)
#define MAX40109_MTP_VPP_INIT_FAIL_MASK     BIT(3)
#define MAX40109_MTP_FULL_MASK              BIT(2)
#define MAX40109_MTP_VERIFICATION_FAIL_MASK BIT(1)
#define MAX40109_MTP_VPP_ACT_MASK           BIT(0)

/** TEMP MODE Register Masks */
#define MAX40109_DRV_SCALE_MASK GENMASK(4, 3)
#define MAX40109_TEMP_MODE_MASK GENMASK(2, 0)

/** SENSOR OFFSET CAL CONFIG Register Masks */
#define MAX40109_CONNECT_TRIM_RESISTOR_MASK              BIT(2)
#define MAX40109_CONNECT_OFFSET_CALIBRATION_CURRENT_MASK BIT(1)
#define MAX40109_PGA_FUNCTIONALITY_MASK                  BIT(0)

/** MTP DATA LENGTH */
#define MAX40109_MTP_DATA_LENGTH 2

/** MTP Charge Pump Initialization Values */
#define MAX40109_CP_CONTROL_1_MTP_INIT 0x80 /* Charge pump control for MTP operations */
#define MAX40109_CP_CONTROL_2_MTP_INIT 0x1B /* Charge pump timing for MTP operations */

/** MTP Slope Parameters - Must be written before MTP burn operations */
#define MAX40109_SLP_MR_MSB_INIT    0x03 /* Slope MR MSB: upper byte of 0x0302 */
#define MAX40109_SLP_MR_LSB_INIT    0x02 /* Slope MR LSB: lower byte of 0x0302 */
#define MAX40109_SLP_MREF_MSB_INIT  0x02 /* Slope MREF MSB: upper byte of 0x0200 */
#define MAX40109_SLP_MREF_LSB_INIT  0x00 /* Slope MREF LSB: lower byte of 0x0200 */
#define MAX40109_SLP_MRV_MSB_INIT   0x03 /* Slope MRV MSB: upper byte of 0x0300 */
#define MAX40109_SLP_MRV_LSB_INIT   0x00 /* Slope MRV LSB: lower byte of 0x0300 */
#define MAX40109_SLP_MREFV_MSB_INIT 0x04 /* Slope MREFV MSB: upper byte of 0x0401 */
#define MAX40109_SLP_MREFV_LSB_INIT 0x01 /* Slope MREFV LSB: lower byte of 0x0401 */

struct max40109_config {
	struct i2c_dt_spec i2c;

#if defined(CONFIG_MAX40109_TRIGGER)
	struct gpio_dt_spec interrupt_gpio;
#endif
	uint8_t digital_filter_setup;
	uint8_t alert_mode;
	uint8_t temp_current;
	uint8_t pga_pressure_gain;
	uint8_t current_source;
	uint8_t adc_sample_rate;
	uint8_t pga_temperature_gain;
	uint8_t bridge_drive;
	uint8_t temp_mode;
	uint8_t drv_scale;
	uint8_t analog_filter_bw;
	uint8_t analog_output_stage;
#if defined(CONFIG_MAX40109_CALIBRATION)
	/** Calibration coefficients, NAN for the ones left out of devicetree */
	float calib_coeff[20];
#endif
};

struct temp_gain_map {
	int val1;
	int val2;
	uint8_t reg_value;
};

struct pressure_gain_map {
	int val1;
	uint8_t reg_value;
};

struct sampling_rate_entry {
	int pressure_rate;
	int temperature_rate;
	uint8_t reg_value;
};

struct drv_scale_entry {
	int val1;
	int val2;
	uint8_t reg_value;
};

struct analog_bw_map {
	int bw;
	uint8_t reg_val;
};

struct digital_filter_map {
	int setup;
	uint8_t reg_val;
};

#if defined(CONFIG_MAX40109_CALIBRATION)
struct calib_map {
	/** Channel from @ref max40109_sensor_channel. */
	uint16_t chan;
	/** Coefficient that channel carries. */
	enum max40109_calibration_coefficients coeff;
};

struct calibration_coeff_map {
	uint8_t cal_coeff;
	uint8_t cal_start_addr;
};
#endif /* CONFIG_MAX40109_CALIBRATION */

/**
 * @brief Describes one MTP-backed threshold attribute.
 *
 * All threshold attributes share the same shape: an 8-bit value held in a
 * bitfield of a 16-bit MTP word, optionally restricted to one channel.
 */
struct max40109_threshold_map {
	/** Attribute from @ref max40109_sensor_attr. */
	uint16_t attr;
	/** MTP word holding the field. */
	uint8_t mtp_addr;
	/** Position of the field within that word. */
	uint16_t mask;
	/** Channel this attribute applies to, or SENSOR_CHAN_ALL for any. */
	enum sensor_channel chan;
};

struct max40109_data {
	/* Measurements are 16-bit two's complement, 0x8000 is full negative scale. */
	int16_t uncalibrated_pressure;
	int16_t uncalibrated_temperature;
	int16_t calibrated_pressure;
	int16_t calibrated_temperature;
#ifdef CONFIG_MAX40109_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_sem gpio_sem;
	/* Last reported DRV/INT fault bits, so a stuck fault is logged once. */
	uint8_t fault_state;
	sensor_trigger_handler_t drv_fault_handler;
	sensor_trigger_handler_t int_fault_handler;
	sensor_trigger_handler_t temp_data_rdy_handler;
	sensor_trigger_handler_t pressure_data_rdy_handler;
	sensor_trigger_handler_t uv_drv_handler;
	sensor_trigger_handler_t ov_drv_handler;
	sensor_trigger_handler_t uv_int_handler;
	sensor_trigger_handler_t ov_int_handler;
	sensor_trigger_handler_t uv_inp_negative_handler;
	sensor_trigger_handler_t ov_inp_negative_handler;
	sensor_trigger_handler_t uv_inp_positive_handler;
	sensor_trigger_handler_t ov_inp_positive_handler;
	const struct sensor_trigger *drv_fault_trigger;
	const struct sensor_trigger *int_fault_trigger;
	const struct sensor_trigger *temp_data_rdy_trigger;
	const struct sensor_trigger *pressure_data_rdy_trigger;
	const struct sensor_trigger *uv_drv_trigger;
	const struct sensor_trigger *ov_drv_trigger;
	const struct sensor_trigger *uv_int_trigger;
	const struct sensor_trigger *ov_int_trigger;
	const struct sensor_trigger *uv_inp_negative_trigger;
	const struct sensor_trigger *ov_inp_negative_trigger;
	const struct sensor_trigger *uv_inp_positive_trigger;
	const struct sensor_trigger *ov_inp_positive_trigger;
#if defined(CONFIG_MAX40109_TRIGGER_OWN_THREAD)
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_MAX40109_THREAD_STACK_SIZE);
#elif defined(CONFIG_MAX40109_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif
};

int max40109_reg_read(const struct device *dev, uint8_t reg_addr, uint8_t *val, uint8_t length);
int max40109_reg_write(const struct device *dev, uint8_t reg_addr, uint8_t val);
int max40109_reg_write_multiple(const struct device *dev, uint8_t reg_addr, uint8_t *val,
				uint8_t length);
int max40109_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t val);
int max40109_init_interrupt(const struct device *dev);
int max40109_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

#endif /* ZEPHYR_DRIVERS_SENSOR_ADI_MAX40109_MAX40109_H_ */
