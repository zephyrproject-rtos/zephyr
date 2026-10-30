/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Darshan Maru <darshan.maru@dnkmail.in>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file vl53l4cd.h
 * @brief VL53L4CD Time-of-Flight ranging sensor driver.
 *
 * Defines register addresses, bit masks, driver configuration and runtime
 * data structures for the VL53L4CD Zephyr sensor driver.
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ST_VL53L4CD_VL53L4CD_H_
#define ZEPHYR_DRIVERS_SENSOR_ST_VL53L4CD_VL53L4CD_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>

/* ---- Register addresses ---- */
#define VL53L4CD_SOFT_RESET                 0x0000 /* Software reset control */
#define VL53L4CD_I2C_SLAVE__DEVICE_ADDRESS  0x0001 /* I2C device address */
#define VL53L4CD_OSC_FREQUENCY              0x0006 /* Oscillator frequency */
#define VL53L4CD_VHV_CONFIG__TIMEOUT_MACROP 0x0008 /* VHV calibration timeout (macro periods) */
#define VL53L4CD_VHV_CONFIG_0B              0x000B /* VHV configuration (offset 0x0B) */
#define VL53L4CD_VHV_CONFIG_24              0x0024 /* VHV configuration (offset 0x24) */
#define VL53L4CD_FAST_MODE_PLUS_CTRL        0x002D /* I2C Fast-mode Plus control */
#define VL53L4CD_I2C_PULLUP_CTRL            0x002E /* I2C pull-up control (1.8 V I/O) */
#define VL53L4CD_GPIO_PULLUP_CTRL           0x002F /* GPIO pull-up control */
#define VL53L4CD_GPIO_HV_MUX__CTRL          0x0030 /* GPIO1 interrupt polarity control */
#define VL53L4CD_GPIO__TIO_HV_STATUS        0x0031 /* GPIO1 interrupt status */
#define VL53L4CD_SYSTEM__INTERRUPT          0x0046 /* Interrupt configuration */
#define VL53L4CD_RANGE_CONFIG_A             0x005E /* Range timing config A (timing budget) */
#define VL53L4CD_RANGE_CONFIG_B             0x0061 /* Range timing config B (timing budget) */
#define VL53L4CD_INTERMEASUREMENT_MS        0x006C /* Inter-measurement period */
#define VL53L4CD_SYSTEM__INTERRUPT_CLEAR    0x0086 /* Clear interrupt */
#define VL53L4CD_SYSTEM_START               0x0087 /* Start/stop ranging */
#define VL53L4CD_RESULT__RANGE_STATUS       0x0089 /* Range status (read-only) */
#define VL53L4CD_RESULT__SPAD_NB            0x008C /* Number of enabled SPADs (read-only) */
#define VL53L4CD_RESULT__SIGNAL_RATE        0x008E /* Signal rate, kcps (read-only) */
#define VL53L4CD_RESULT__AMBIENT_RATE       0x0090 /* Ambient rate, kcps (read-only) */
#define VL53L4CD_RESULT__SIGMA              0x0092 /* Range sigma estimate, mm (read-only) */
#define VL53L4CD_RESULT__DISTANCE           0x0096 /* Measured distance, mm (read-only) */
#define VL53L4CD_RESULT__OSC_CALIBRATE_VAL  0x00DE /* Oscillator calibration value */
#define VL53L4CD_FIRMWARE__SYSTEM_STATUS    0x00E5 /* Firmware boot status (read-only) */
#define VL53L4CD_IDENTIFICATION__MODEL_ID   0x010F /* Model ID: 0xEB, module type (0x0110) = 0xAA */

/* ---- ID / status values ---- */
#define VL53L4CD_MODEL_ID_MODULE_TYPE 0xEBAA /* Model ID (0xEB) and module type (0xAA) */
#define VL53L4CD_FIRMWARE_BOOTED      0x03   /* Firmware booted and ready */

/* ---- SYSTEM_START register values (0x87) ---- */
#define VL53L4CD_START_CONTINUOUS 0x21 /* Start continuous ranging */
#define VL53L4CD_START_AUTONOMOUS 0x40 /* Start autonomous ranging */
#define VL53L4CD_START_VHV        0x40 /* Start VHV calibration */
#define VL53L4CD_STOP             0x80 /* Stop ranging */

/* ---- Timing budget limits ---- */
#define VL53L4CD_TIMING_BUDGET_MIN_MS 10  /* Minimum timing budget in ms */
#define VL53L4CD_TIMING_BUDGET_MAX_MS 200 /* Maximum timing budget in ms */

/**
 * @brief Raw sample produced by the driver and consumed by the decoder.
 */
struct vl53l4cd_encoded_data {
	uint64_t timestamp_ns;          /**< Sample timestamp in nanoseconds. */
	uint32_t signal_rate_kcps;      /**< Total signal rate in kcps. */
	uint32_t ambient_rate_kcps;     /**< Total ambient rate in kcps. */
	uint32_t signal_per_spad_kcps;  /**< Signal rate per SPAD in kcps. */
	uint32_t ambient_per_spad_kcps; /**< Ambient rate per SPAD in kcps. */
	uint16_t range_mm;              /**< Measured distance in millimeters. */
	uint16_t sigma_mm;              /**< Range sigma estimate in millimeters. */
	uint8_t range_status;           /**< Raw range status code. */
	uint8_t spad_count;             /**< Number of enabled SPADs. */
};

/**
 * @brief Read-only driver configuration, populated from devicetree.
 */
struct vl53l4cd_config {
	struct i2c_dt_spec i2c;        /**< I2C bus specification from devicetree. */
	struct gpio_dt_spec xshut;     /**< XSHUT (hardware standby) GPIO. */
	uint16_t timing_budget_ms;     /**< Ranging timing budget in ms. */
	uint32_t inter_measurement_ms; /**< Inter-measurement period in ms. */
	bool io_1v8;                   /**< Enable 1.8 V I/O mode. */
	bool fast_mode_plus;           /**< Enable I2C Fast-mode Plus. */
};

/**
 * @brief Runtime driver data.
 */
struct vl53l4cd_data {
	struct k_mutex lock;           /**< Mutex protecting device access. */
	uint16_t timing_budget_ms;     /**< Active timing budget in ms. */
	uint32_t inter_measurement_ms; /**< Active inter-measurement period in ms. */
};

/**
 * @brief Submit an RTIO read request to the driver.
 *
 * @param dev        VL53L4CD device instance.
 * @param iodev_sqe  RTIO submission queue entry to process.
 */
void vl53l4cd_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Get the sensor decoder API for this driver.
 *
 * @param dev      VL53L4CD device instance.
 * @param decoder  Output pointer set to the decoder API.
 *
 * @return 0 on success, negative errno on failure.
 */
int vl53l4cd_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_ST_VL53L4CD_VL53L4CD_H_ */
