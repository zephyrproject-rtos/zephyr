/*
 * Copyright (c) 2026 Dotcom IoT LLP
 * SPDX-License-Identifier: Apache-2.0
 * Author: Mahendra Sondagar <mahendra@dotcom.co.in>
 * Author: Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 */
/**
 * @file tmf8801.h
 * @brief Define internal definitions and data structures for the TMF8801 Time-of-Flight sensor
 * driver.
 *
 * Defines the register map, bit fields, result frame layout, calibration
 * constants, and driver data/config structures used internally by the
 * TMF8801 sensor driver.
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMF8801_TMF8801_H_
#define ZEPHYR_DRIVERS_SENSOR_TMF8801_TMF8801_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>

/* Register Map */
#define REG_TMF8801_ENABLE   0xE0 /**< Enable register. */
#define REG_TMF8801_APPID    0x00 /**< Application ID register. */
#define REG_TMF8801_ID       0xE3 /**< Chip ID register. */
#define REG_TMF8801_APPREQID 0x02 /**< Application request ID register. */

/* Command payload registers */
#define REG_TMF8801_CMD_DATA9 0x06 /**< Command payload byte 9. */
#define REG_TMF8801_CMD_DATA8 0x07 /**< Command payload byte 8. */
#define REG_TMF8801_CMD_DATA7 0x08 /**< Command payload byte 7. */
#define REG_TMF8801_CMD_DATA6 0x09 /**< Command payload byte 6. */
#define REG_TMF8801_CMD_DATA5 0x0A /**< Command payload byte 5. */
#define REG_TMF8801_CMD_DATA4 0x0B /**< Command payload byte 4. */
#define REG_TMF8801_CMD_DATA3 0x0C /**< Command payload byte 3. */
#define REG_TMF8801_CMD_DATA2 0x0D /**< Command payload byte 2. */
#define REG_TMF8801_CMD_DATA1 0x0E /**< Command payload byte 1. */
#define REG_TMF8801_CMD_DATA0 0x0F /**< Command payload byte 0. */

/* Command execution register */
#define REG_TMF8801_COMMAND 0x10 /**< Command execution register. */

/* Calibration and algorithm state registers */
#define REG_TMF8801_FACTORYCALIB 0x20 /**< Factory calibration data write register. */
#define REG_TMF8801_STATEDATAWR  0x2E /**< Algorithm state data write register. */

/* Measurement result registers */
#define REG_TMF8801_STATUS        0x1D /**< Status register. */
#define REG_TMF8801_CONTENTS      0x1E /**< Content identification register. */
#define REG_TMF8801_RESULT_NUMBER 0x20 /**< Measurement result counter register. */

/* Interrupt control registers */
#define REG_TMF8801_INT_ENAB   0xE2 /**< Interrupt enable register. */
#define REG_TMF8801_INT_STATUS 0xE1 /**< Interrupt status register. */

/* Version and identification registers */
#define REG_TMF8801_VERSION_MAJOR 0x01 /**< Major application version register. */
#define REG_TMF8801_VERSION_MINOR 0x12 /**< Minor application version register. */
#define REG_TMF8801_VERSION_PATCH 0x13 /**< Patch application version register. */

/* ENABLE register bit definitions */
#define TMF8801_ENABLE_PON       BIT(0) /**< Power on the chip. */
#define TMF8801_ENABLE_CPU_READY BIT(6) /**< CPU ready flag. */
#define TMF8801_ENABLE_CPU_RESET BIT(7) /**< Reset the CPU. */

/* Application identifiers reported in APPID register */
#define TMF8801_APPID_APP0       0xC0 /**< Main application running. */
#define TMF8801_APPID_BOOTLOADER 0x80 /**< Bootloader active. */

/* Factory calibration data size in bytes */
#define SENSOR_TMF8801_CALIBRATION_SIZE 14 /**< Factory calibration data size, in bytes. */
#define SENSOR_TMF8801_ALGO_STATE_SIZE  11 /**< Algorithm state data size, in bytes. */

/* Measurement command configuration bit positions */
#define CMDSET_INDEX_CMD7 0 /**< Index for command payload byte 7. */
#define CMDSET_BIT_CALIB  0 /**< Calibration data load flag bit. */
#define CMDSET_BIT_ALGO   1 /**< Algorithm state data load flag bit. */

#define CMDSET_INDEX_CMD6    1 /**< Index for command payload byte 6. */
#define CMDSET_BIT_PROXIMITY 0 /**< Proximity measurement enable bit. */
#define CMDSET_BIT_DISTANCE  1 /**< Distance measurement enable bit. */
#define CMDSET_BIT_INT       4 /**< Interrupt enable bit. */
#define CMDSET_BIT_COMBINE   5 /**< Combine proximity and distance measurement bit. */

/** @brief Calibration configuration modes. */
enum tmf8801_calib_mode {
	TMF8801_MODE_NO_CALIB = 0, /**< Do not load calibration data. */
	TMF8801_MODE_CALIB = 1,    /**< Load factory calibration data. */
	TMF8801_MODE_CALIB_AND_ALGO_STATE =
		3 /**< Load factory calibration and algorithm state data. */
};

/**
 * @brief Measurement result frame.
 *
 * The TMF8801 returns an 11-byte result block beginning at
 * REG_TMF8801_STATUS (0x1D).
 */
struct tmf8801_result {
	uint8_t status;        /**< Result status (register 0x1D). */
	uint8_t reg_contents;  /**< Register contents descriptor (register 0x1E). */
	uint8_t tid;           /**< Transaction identifier (register 0x1F). */
	uint8_t result_number; /**< Sequential measurement index (register 0x20). */
	uint8_t result_info;   /**< Measurement reliability info (register 0x21). */
	uint8_t dis_l;     /**< Distance least significant byte, in millimeters (register 0x22). */
	uint8_t dis_h;     /**< Distance most significant byte, in millimeters (register 0x23). */
	uint8_t sysclock0; /**< System clock byte 0 (register 0x24). */
	uint8_t sysclock1; /**< System clock byte 1 (register 0x25). */
	uint8_t sysclock2; /**< System clock byte 2 (register 0x26). */
	uint8_t sysclock3; /**< System clock byte 3 (register 0x27). */
} __packed;

/**
 * @brief Configuration data populated from devicetree.
 */
struct tmf8801_config {
	struct i2c_dt_spec i2c;
};

/**
 * @brief Runtime driver data.
 */
struct tmf8801_data {
	/** 9 bytes written to REG_TMF8801_CMD_DATA7 */
	uint8_t measure_cmd_set[9];
	uint8_t calib_data[SENSOR_TMF8801_CALIBRATION_SIZE]; /** Factory calibration data buffer. */
	uint8_t algo_state_data[SENSOR_TMF8801_ALGO_STATE_SIZE]; /** Algorithm state data buffer. */
	uint32_t host_time[5];        /** Host system timestamps, in system ticks. */
	uint32_t module_time[5];      /** Sensor module timestamps, in clock cycles. */
	uint32_t timestamp;           /* correction factor */
	uint8_t count;                /* sample counter 0-4 */
	struct tmf8801_result result; /** Last received raw measurement result block. */
	bool measure_cmd_flag; /** Flag indicating if the measurement command has been sent. */
	uint16_t distance_mm;  /* reported distance after timestamp correction */
	uint8_t pin_config;    /* config byte for PIN0/PIN1 */
	bool initialized;      /** Device driver initialization state. */
};

#endif /* ZEPHYR_DRIVERS_SENSOR_TMF8801_TMF8801_H_ */
