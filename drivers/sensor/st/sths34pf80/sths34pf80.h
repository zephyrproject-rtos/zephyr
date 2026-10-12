/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STHS34PF80 infrared presence and motion sensor driver.
 *
 * Defines register addresses, bit fields, custom sensor channels, driver
 * configuration and runtime data structures for the STHS34PF80 Zephyr
 * sensor driver.
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STHS34PF80_STHS34PF80_H_
#define ZEPHYR_DRIVERS_SENSOR_STHS34PF80_STHS34PF80_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/sths34pf80.h>
#include <zephyr/sys/util.h>

/* Register map */
#define STHS34PF80_REG_FUNC_CFG_ADDR 0x08
#define STHS34PF80_REG_FUNC_CFG_DATA 0x09
#define STHS34PF80_REG_LPF1          0x0C
#define STHS34PF80_REG_LPF2          0x0D
#define STHS34PF80_REG_WHO_AM_I      0x0F
#define STHS34PF80_REG_AVG_TRIM      0x10
#define STHS34PF80_REG_PAGE_RW       0x11
#define STHS34PF80_REG_CTRL0         0x17
#define STHS34PF80_REG_SENS_DATA     0x1D
#define STHS34PF80_REG_CTRL1         0x20
#define STHS34PF80_REG_CTRL2         0x21
#define STHS34PF80_REG_CTRL3         0x22
#define STHS34PF80_REG_STATUS        0x23
#define STHS34PF80_REG_FUNC_STATUS   0x25
#define STHS34PF80_REG_TOBJECT_L     0x26
#define STHS34PF80_REG_TAMBIENT_L    0x28
#define STHS34PF80_REG_TOBJ_COMP_L   0x38
#define STHS34PF80_REG_TPRESENCE_L   0x3A
#define STHS34PF80_REG_TMOTION_L     0x3C
#define STHS34PF80_REG_TAMB_SHOCK_L  0x3E

/* Embedded function register map (accessed via FUNC_CFG_ADDR/DATA) */
#define STHS34PF80_EMB_PRESENCE_THS_L   0x20
#define STHS34PF80_EMB_MOTION_THS_L     0x22
#define STHS34PF80_EMB_TAMB_SHOCK_THS_L 0x24
#define STHS34PF80_EMB_HYST_MOTION      0x26
#define STHS34PF80_EMB_HYST_PRESENCE    0x27
#define STHS34PF80_EMB_ALGO_CONFIG      0x28
#define STHS34PF80_EMB_HYST_TAMB_SHOCK  0x29
#define STHS34PF80_EMB_RESET_ALGO       0x2A

#define STHS34PF80_WHO_AM_I_VALUE 0xD3

/* CTRL0: GAIN[2:0] at bits 6:4 */
#define STHS34PF80_CTRL0_GAIN_SHIFT   4
#define STHS34PF80_CTRL0_GAIN_MASK    (BIT_MASK(3) << STHS34PF80_CTRL0_GAIN_SHIFT)
#define STHS34PF80_CTRL0_GAIN_WIDE    0x00 /**< Wide-angle gain mode. */
#define STHS34PF80_CTRL0_GAIN_DEFAULT 0x07 /**< Default gain (narrow). */

/* AVG_TRIM */
#define STHS34PF80_AVG_TRIM_AVG_T_SHIFT    4
#define STHS34PF80_AVG_TRIM_AVG_T_MASK     (BIT_MASK(2) << STHS34PF80_AVG_TRIM_AVG_T_SHIFT)
#define STHS34PF80_AVG_TRIM_AVG_TMOS_SHIFT 0
#define STHS34PF80_AVG_TRIM_AVG_TMOS_MASK  (BIT_MASK(3) << STHS34PF80_AVG_TRIM_AVG_TMOS_SHIFT)

/* CTRL1 */
#define STHS34PF80_CTRL1_BDU       BIT(4) /**< Block data update enable. */
#define STHS34PF80_CTRL1_ODR_SHIFT 0
#define STHS34PF80_CTRL1_ODR_MASK  BIT_MASK(4)

/* CTRL2 */
#define STHS34PF80_CTRL2_BOOT            BIT(7) /**< Reload OTP trimming parameters. */
#define STHS34PF80_CTRL2_FUNC_CFG_ACCESS BIT(4) /**< Enable embedded function register access. */
#define STHS34PF80_CTRL2_ONE_SHOT        BIT(0) /**< Trigger a single acquisition. */

/* CTRL3 */
#define STHS34PF80_CTRL3_INT_H_L       BIT(7) /**< Interrupt active-low when set. */
#define STHS34PF80_CTRL3_PP_OD         BIT(6) /**< Open-drain interrupt output when set. */
#define STHS34PF80_CTRL3_INT_MSK_SHIFT 3
#define STHS34PF80_CTRL3_INT_MSK_MASK  (BIT_MASK(3) << STHS34PF80_CTRL3_INT_MSK_SHIFT)
#define STHS34PF80_CTRL3_INT_LATCHED   BIT(2) /**< Latch interrupt until status is read. */
#define STHS34PF80_CTRL3_IEN_MASK      BIT_MASK(2)
#define STHS34PF80_CTRL3_RESET_ALG     BIT(1) /**< Reset embedded detection algorithms. */

/* STATUS */
#define STHS34PF80_STATUS_DRDY BIT(2) /**< New data available. */

/* FUNC_STATUS */
#define STHS34PF80_FUNC_STATUS_PRES_FLAG       BIT(2) /**< Presence detection flag. */
#define STHS34PF80_FUNC_STATUS_MOT_FLAG        BIT(1) /**< Motion detection flag. */
#define STHS34PF80_FUNC_STATUS_TAMB_SHOCK_FLAG BIT(0) /**< Ambient temperature shock flag. */

/* PAGE_RW */
#define STHS34PF80_PAGE_RW_FUNC_CFG_WRITE BIT(6) /**< Enable write access to embedded page. */
#define STHS34PF80_PAGE_RW_FUNC_CFG_READ  BIT(5) /**< Enable read access to embedded page. */

/* ALGO_CONFIG (embedded) */
#define STHS34PF80_ALGO_CONFIG_INT_PULSED BIT(3) /**< Pulsed interrupt mode (vs. latched). */
#define STHS34PF80_ALGO_CONFIG_COMP_TYPE  BIT(2) /**< Compensation type selection. */
#define STHS34PF80_ALGO_CONFIG_SEL_ABS    BIT(1) /**< Absolute detection mode. */

/* RESET_ALGO (embedded) */
#define STHS34PF80_ALGO_ENABLE_RESET BIT(0) /**< Trigger algorithm reset. */

/* Sensitivity of the temperature outputs */
#define STHS34PF80_AMBIENT_TEMP_LSB_PER_C 100  /**< LSB per degree C for ambient temperature. */
#define STHS34PF80_OBJECT_TEMP_LSB_PER_C  2000 /**< LSB per degree C for object temperature. */

/* Timing constants, in milliseconds */
#define STHS34PF80_FLAG_TIMEOUT_MS                                                                 \
	3000                        /**< Hold time of presence/motion flags after last detection. */
#define STHS34PF80_OTP_REBOOT_MS 5  /**< Wait time after OTP reload. */
#define STHS34PF80_RESET_ALG_MS  1  /**< Settle time after algorithm reset pulse. */
#define STHS34PF80_DRDY_POLL_MS  10 /**< Data-ready polling interval. */

/**
 * @brief Maximum wait time for data-ready, in ms.
 *
 * Set longer than one sample period at the slowest supported ODR (1 Hz = 1000 ms).
 */
#define STHS34PF80_DRDY_TIMEOUT_MS 1100

/** Maximum number of data-ready polling attempts before timing out. */
#define STHS34PF80_DRDY_MAX_POLLS (STHS34PF80_DRDY_TIMEOUT_MS / STHS34PF80_DRDY_POLL_MS)

/**
 * @brief Output data rate selection.
 */
enum sths34pf80_odr {
	STHS34PF80_ODR_POWER_DOWN = 0x00, /**< Power-down, no measurements. */
	STHS34PF80_ODR_0_25_HZ = 0x01,    /**< 0.25 Hz. */
	STHS34PF80_ODR_0_5_HZ = 0x02,     /**< 0.5 Hz. */
	STHS34PF80_ODR_1_HZ = 0x03,       /**< 1 Hz. */
	STHS34PF80_ODR_2_HZ = 0x04,       /**< 2 Hz. */
	STHS34PF80_ODR_4_HZ = 0x05,       /**< 4 Hz. */
	STHS34PF80_ODR_8_HZ = 0x06,       /**< 8 Hz. */
	STHS34PF80_ODR_15_HZ = 0x07,      /**< 15 Hz. */
	STHS34PF80_ODR_30_HZ = 0x08,      /**< 30 Hz. */
};

/**
 * @brief Ambient temperature averaging.
 */
enum sths34pf80_avg_t {
	STHS34PF80_AVG_T_8 = 0x00, /**< Average 8 samples. */
	STHS34PF80_AVG_T_4 = 0x01, /**< Average 4 samples. */
	STHS34PF80_AVG_T_2 = 0x02, /**< Average 2 samples. */
	STHS34PF80_AVG_T_1 = 0x03, /**< No averaging. */
};

/**
 * @brief Object temperature averaging.
 */
enum sths34pf80_avg_tmos {
	STHS34PF80_AVG_TMOS_2 = 0x00,    /**< Average 2 samples. */
	STHS34PF80_AVG_TMOS_8 = 0x01,    /**< Average 8 samples. */
	STHS34PF80_AVG_TMOS_32 = 0x02,   /**< Average 32 samples. */
	STHS34PF80_AVG_TMOS_128 = 0x03,  /**< Average 128 samples. */
	STHS34PF80_AVG_TMOS_256 = 0x04,  /**< Average 256 samples. */
	STHS34PF80_AVG_TMOS_512 = 0x05,  /**< Average 512 samples. */
	STHS34PF80_AVG_TMOS_1024 = 0x06, /**< Average 1024 samples. */
	STHS34PF80_AVG_TMOS_2048 = 0x07, /**< Average 2048 samples. */
};

/**
 * @brief STHS34PF80 driver runtime data.
 */
struct sths34pf80_data {
	/** Object temperature (TOBJECT). */
	int16_t object_temp_raw;
	/** Ambient temperature (TAMBIENT). */
	int16_t ambient_temp_raw;
	/** Compensated object temperature (TOBJ_COMP). */
	int16_t object_temp_comp_raw;
	/** Presence algorithm output (TPRESENCE). */
	int16_t presence_raw;
	/** Motion algorithm output (TMOTION). */
	int16_t motion_raw;
	/** Temperature shock algorithm output (TAMB_SHOCK). */
	int16_t temp_shock_raw;

	/** Presence detected, held for STHS34PF80_FLAG_TIMEOUT_MS. */
	bool presence_flag;
	/** Motion detected, held for STHS34PF80_FLAG_TIMEOUT_MS. */
	bool motion_flag;
	/** Ambient temperature shock detected. */
	bool temp_shock_flag;

	/** Uptime in ms of the last presence detection. */
	int64_t presence_last_detected_ms;
	/** Uptime in ms of the last motion detection. */
	int64_t motion_last_detected_ms;
};

/**
 * @brief STHS34PF80 driver configuration.
 */
struct sths34pf80_config {
	/** I2C bus and address from devicetree. */
	struct i2c_dt_spec i2c;
	/** Output data rate applied at init. */
	enum sths34pf80_odr odr;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_STHS34PF80_STHS34PF80_H_ */
