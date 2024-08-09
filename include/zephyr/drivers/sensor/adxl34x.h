/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for ADI's ADXL34x motion sensor
 *
 * This exposes additional attributes for the ADXL34x.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL34X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL34X_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Accelerometer range options
 */
enum adxl34x_accel_freq {
	ADXL34X_ACCEL_FREQ_0_10,
	ADXL34X_ACCEL_FREQ_0_20,
	ADXL34X_ACCEL_FREQ_0_39,
	ADXL34X_ACCEL_FREQ_0_78,
	ADXL34X_ACCEL_FREQ_1_56,
	ADXL34X_ACCEL_FREQ_3_13,
	ADXL34X_ACCEL_FREQ_6_25,
	ADXL34X_ACCEL_FREQ_12_5,
	ADXL34X_ACCEL_FREQ_25,
	ADXL34X_ACCEL_FREQ_50,
	ADXL34X_ACCEL_FREQ_100,
	ADXL34X_ACCEL_FREQ_200,
	ADXL34X_ACCEL_FREQ_400,
	ADXL34X_ACCEL_FREQ_800,
	ADXL34X_ACCEL_FREQ_1600,
	ADXL34X_ACCEL_FREQ_3200,
};

/**
 * @brief Accelerometer range options
 */
enum adxl34x_accel_range {
	ADXL34X_RANGE_2G,
	ADXL34X_RANGE_4G,
	ADXL34X_RANGE_8G,
	ADXL34X_RANGE_16G,
};

/**
 * @brief FIFO mode
 */
enum adxl34x_fifo_mode {
	ADXL34X_FIFO_MODE_BYPASS,
	ADXL34X_FIFO_MODE_FIFO,
	ADXL34X_FIFO_MODE_STREAM,
	ADXL34X_FIFO_MODE_TRIGGER,
};

/**
 * @brief Frequency when sleeping
 */
enum adxl34x_sleep_freq {
	ADXL34X_SLEEP_FREQ_8_HZ,
	ADXL34X_SLEEP_FREQ_4_HZ,
	ADXL34X_SLEEP_FREQ_2_HZ,
	ADXL34X_SLEEP_FREQ_1_HZ,
};

/**
 * @brief Dead zone angle
 */
enum adxl34x_dead_zone_angle {
	ADXL34X_DEAD_ZONE_ANGLE_5_1,
	ADXL34X_DEAD_ZONE_ANGLE_10_2,
	ADXL34X_DEAD_ZONE_ANGLE_15_2,
	ADXL34X_DEAD_ZONE_ANGLE_20_4,
	ADXL34X_DEAD_ZONE_ANGLE_25_5,
	ADXL34X_DEAD_ZONE_ANGLE_30_8,
	ADXL34X_DEAD_ZONE_ANGLE_36_1,
	ADXL34X_DEAD_ZONE_ANGLE_41_4,
};

/**
 * @brief Divisor Bandwidth (Hz)
 */
enum adxl34x_divisor {
	ADXL34X_DIVISOR_ODR_9,
	ADXL34X_DIVISOR_ODR_22,
	ADXL34X_DIVISOR_ODR_50,
	ADXL34X_DIVISOR_ODR_100,
	ADXL34X_DIVISOR_ODR_200,
	ADXL34X_DIVISOR_ODR_400,
	ADXL34X_DIVISOR_ODR_800,
	ADXL34X_DIVISOR_ODR_1600,
};

/**
 * @brief Orientation Codes 2D
 */
enum adxl34x_orient_2d {
	ADXL34X_ORIENT_2D_POS_X,
	ADXL34X_ORIENT_2D_NEG_X,
	ADXL34X_ORIENT_2D_POS_Y,
	ADXL34X_ORIENT_2D_NEG_Y,
};

/**
 * @brief Orientation Codes 3D
 */
enum adxl34x_orient_3d {
	ADXL34X_ORIENT_3D_POS_X,
	ADXL34X_ORIENT_3D_NEG_X,
	ADXL34X_ORIENT_3D_POS_Y,
	ADXL34X_ORIENT_3D_NEG_Y,
	ADXL34X_ORIENT_3D_POS_Z,
	ADXL34X_ORIENT_3D_NEG_Z,
};

/**
 * @brief Axis control for activity and inactivity detection
 * @struct adxl34x_act_inact_ctl
 */
struct adxl34x_act_inact_ctl {
	uint8_t act_acdc: 1;       /**< Enable AC coupling for activity detection */
	uint8_t act_x_enable: 1;   /**< Enable acitivy detection on X axis */
	uint8_t act_y_enable: 1;   /**< Enable acitivy detection on Y axis */
	uint8_t act_z_enable: 1;   /**< Enable acitivy detection on Z axis */
	uint8_t inact_acdc: 1;     /**< Enable AC coupling for inactivity detection */
	uint8_t inact_x_enable: 1; /**< Enable acitivy indetection on X axis */
	uint8_t inact_y_enable: 1; /**< Enable acitivy indetection on Y axis */
	uint8_t inact_z_enable: 1; /**< Enable acitivy indetection on Z axis */
} __attribute__((__packed__));

/**
 * @brief Axis control for single tap/double tap
 * @struct adxl34x_tap_axes
 */
struct adxl34x_tap_axes {
	uint8_t improved_tab: 1; /**< Enable improved tap detection (only adxl344 and adxl346) */
	uint8_t suppress: 1;     /**< Suppresses double-tap detection */
	uint8_t tap_x_enable: 1; /**< Enable tab detection on X axis */
	uint8_t tap_y_enable: 1; /**< Enable tab detection on Y axis */
	uint8_t tap_z_enable: 1; /**< Enable tab detection on Z axis */
} __attribute__((__packed__));

/**
 * @brief Source of single tap/double tap
 * @struct adxl34x_act_tap_status
 */
struct adxl34x_act_tap_status {
	uint8_t act_x_source: 1; /**< Indicate the activity event was detected on X axis */
	uint8_t act_y_source: 1; /**< Indicate the activity event was detected on Y axis */
	uint8_t act_z_source: 1; /**< Indicate the activity event was detected on Z axis */
	uint8_t asleep: 1;       /**< Indicate if the device is sleeping */
	uint8_t tap_x_source: 1; /**< Indicate the tab event was detected on X axis */
	uint8_t tap_y_source: 1; /**< Indicate the tab event was detected on X axis */
	uint8_t tap_z_source: 1; /**< Indicate the tab event was detected on X axis */
} __attribute__((__packed__));

/**
 * @brief Data rate and power mode control
 * @struct adxl34x_bw_rate
 */
struct adxl34x_bw_rate {
	uint8_t low_power: 1;            /**< Enable reduced power operation */
	enum adxl34x_accel_freq rate: 4; /**< Bit/sample rate */
} __attribute__((__packed__));

/**
 * @brief Power-saving features control
 * @struct adxl34x_power_ctl
 */
struct adxl34x_power_ctl {
	uint8_t link: 1;                   /**< Link activity with inactivity detection */
	uint8_t auto_sleep: 1;             /**< Enable autosleep */
	uint8_t measure: 1;                /**< Enable measurements (data sampling) */
	uint8_t sleep: 1;                  /**< Enable sleep mode */
	enum adxl34x_sleep_freq wakeup: 2; /**< Bit/sample rate used when sleeping */
} __attribute__((__packed__));

/**
 * @brief Interrupt enable control
 * @struct adxl34x_int_enable
 */
struct adxl34x_int_enable {
	uint8_t data_ready: 1; /**< Enable data ready event interrupt */
	uint8_t single_tap: 1; /**< Enable single tap event interrupt */
	uint8_t double_tap: 1; /**< Enable double tap event interrupt */
	uint8_t activity: 1;   /**< Enable activity event interrupt */
	uint8_t inactivity: 1; /**< Enable inactivity event interrupt */
	uint8_t free_fall: 1;  /**< Enable free fall event interrupt */
	uint8_t watermark: 1;  /**< Enable watermark event interrupt */
	uint8_t overrun: 1;    /**< Enable overrun event interrupt */
} __attribute__((__packed__));

/**
 * @brief Interrupt mapping control
 * @struct adxl34x_int_map
 */
struct adxl34x_int_map {
	uint8_t data_ready: 1; /**< Use pin INT2 on data ready event */
	uint8_t single_tap: 1; /**< Use pin INT2 on single tap event */
	uint8_t double_tap: 1; /**< Use pin INT2 on double tap event */
	uint8_t activity: 1;   /**< Use pin INT2 on activity event */
	uint8_t inactivity: 1; /**< Use pin INT2 on inactivity event */
	uint8_t free_fall: 1;  /**< Use pin INT2 on free fall event */
	uint8_t watermark: 1;  /**< Use pin INT2 on watermark event */
	uint8_t overrun: 1;    /**< Use pin INT2 on overrun event */
} __attribute__((__packed__));

/**
 * @brief Source of interrupts
 * @struct adxl34x_int_source
 */
struct adxl34x_int_source {
	uint8_t data_ready: 1; /**< A data ready event occurred */
	uint8_t single_tap: 1; /**< A single tap event occurred */
	uint8_t double_tap: 1; /**< A double tap event occurred */
	uint8_t activity: 1;   /**< A activity event occurred */
	uint8_t inactivity: 1; /**< A inactivity event occurred */
	uint8_t free_fall: 1;  /**< A free fall event occurred */
	uint8_t watermark: 1;  /**< A watermark event occurred */
	uint8_t overrun: 1;    /**< A overrun event occurred */
} __attribute__((__packed__));

/**
 * @brief Data format control
 * @struct adxl34x_data_format
 */
struct adxl34x_data_format {
	uint8_t self_test: 1;              /**< Enable self-test force to sensor */
	uint8_t spi: 1;                    /**< Enable spi 3 wire mode, not 4 wire mode */
	uint8_t int_invert: 1;             /**< Enable active low interrupts */
	uint8_t full_res: 1;               /**< Enable full resolution mode */
	uint8_t justify: 1;                /**< Left justify data (MSB mode), not right justify */
	enum adxl34x_accel_range range: 2; /**< G range of sensor */
} __attribute__((__packed__));

/**
 * @brief FIFO control
 * @struct adxl34x_fifo_ctl
 */
struct adxl34x_fifo_ctl {
	enum adxl34x_fifo_mode fifo_mode: 2; /**< FIFO mode */
	uint8_t trigger: 1;                  /**< Use pin INT2 for trigger events */
	uint8_t samples: 5;                  /**< FIFO watermark level for interrupts */
} __attribute__((__packed__));

/**
 * @brief FIFO status
 * @struct adxl34x_fifo_status
 */
struct adxl34x_fifo_status {
	uint8_t fifo_trig: 1; /**< Indicate a FIFO watermark event occurred */
	uint8_t entries: 6;   /**< Nr of samples currently in the FIFO */
} __attribute__((__packed__));

/**
 * @brief Sign and source for single tap/double tap
 * @struct adxl34x_tap_sign
 */
struct adxl34x_tap_sign {
	uint8_t xsign: 1; /**< Initial direction (pos or neg) of the tap detected in X axis */
	uint8_t ysign: 1; /**< Initial direction (pos or neg) of the tap detected in Y axis */
	uint8_t zsign: 1; /**< Initial direction (pos or neg) of the tap detected in Z axis */
	uint8_t xtap: 1;  /**< Indicate the X axis was involved first in the tab event */
	uint8_t ytap: 1;  /**< Indicate the Y axis was involved first in the tab event */
	uint8_t ztap: 1;  /**< Indicate the Z axis was involved first in the tab event */
} __attribute__((__packed__));

/**
 * @brief Orientation configuration
 * @struct adxl34x_orient_conf
 */
struct adxl34x_orient_conf {
	uint8_t int_orient: 1; /**< Enable the orientation interrupt */
	enum adxl34x_dead_zone_angle
		dead_zone: 3;            /**< The region between two adjacent orientations */
	uint8_t int_3d: 1;               /**< Enable 3D orientation detection, not 2D */
	enum adxl34x_divisor divisor: 3; /**< Low pass filter */
} __attribute__((__packed__));

/**
 * @brief @brief Orientation status
 * @struct adxl34x_orient
 */
struct adxl34x_orient {
	uint8_t v2: 1; /**< Indicate a valid 2D orientation */
	enum adxl34x_orient_2d
		orient_2d: 2; /**< Indicate the 2D direction of the axis when event occurred */
	uint8_t v3: 1;        /**< Indicate a valid 3D orientation */
	enum adxl34x_orient_3d
		orient_3d: 3; /**< Indicate the 3D direction of the axis when event occurred */
} __attribute__((__packed__));

/**
 * @brief Registry mapping of the adxl34x
 * @struct adxl34x_cfg
 */
struct adxl34x_cfg {
	uint8_t devid; /**< Device ID */
#ifdef CONFIG_ADXL34X_EXTENDED_API
	uint8_t thresh_tap; /**< Tap threshold */
#endif                      /* CONFIG_ADXL34X_EXTENDED_API */
	int8_t ofsx;        /**< X-axis offset */
	int8_t ofsy;        /**< Y-axis offset */
	int8_t ofsz;        /**< Z-axis offset */
#ifdef CONFIG_ADXL34X_EXTENDED_API
	uint8_t dur;          /**< Tap duration */
	uint8_t latent;       /**< Tap latency */
	uint8_t window;       /**< Tap window */
	uint8_t thresh_act;   /**< Activity threshold */
	uint8_t thresh_inact; /**< Inactivity threshold */
	uint8_t time_inact;   /**< Inactivity time */
	struct adxl34x_act_inact_ctl
		act_inact_ctl;            /**< Axis control for activity and inactivity detection */
	uint8_t thresh_ff;                /**< Free-fall threshold */
	uint8_t time_ff;                  /**< Free-fall time */
	struct adxl34x_tap_axes tap_axes; /**< Axis control for single tap/double tap */
#endif                                    /* CONFIG_ADXL34X_EXTENDED_API */
	struct adxl34x_bw_rate bw_rate;   /**< Data rate and power mode control */
	struct adxl34x_power_ctl power_ctl;     /**< Power-saving features control */
	struct adxl34x_int_enable int_enable;   /**< Interrupt enable control */
	struct adxl34x_int_map int_map;         /**< Interrupt mapping control */
	struct adxl34x_data_format data_format; /**< Data format control */
	struct adxl34x_fifo_ctl fifo_ctl;       /**< FIFO control */
#ifdef CONFIG_ADXL34X_EXTENDED_API
	struct adxl34x_orient_conf orient_conf; /**< Orientation configuration */
#endif                                          /* CONFIG_ADXL34X_EXTENDED_API */
};

/* Read/write registers */
int adxl34x_get_thresh_tap(const struct device *dev, uint8_t *thresh_tap, bool use_cache);
int adxl34x_set_thresh_tap(const struct device *dev, uint8_t thresh_tap);
int adxl34x_get_ofsx(const struct device *dev, int8_t *ofsx, bool use_cache);
int adxl34x_set_ofsx(const struct device *dev, int8_t ofsx);
int adxl34x_get_ofsy(const struct device *dev, int8_t *ofsy, bool use_cache);
int adxl34x_set_ofsy(const struct device *dev, int8_t ofsy);
int adxl34x_get_ofsz(const struct device *dev, int8_t *ofsz, bool use_cache);
int adxl34x_set_ofsz(const struct device *dev, int8_t ofsz);
int adxl34x_get_dur(const struct device *dev, uint8_t *dur, bool use_cache);
int adxl34x_set_dur(const struct device *dev, uint8_t dur);
int adxl34x_get_latent(const struct device *dev, uint8_t *latent, bool use_cache);
int adxl34x_set_latent(const struct device *dev, uint8_t latent);
int adxl34x_get_window(const struct device *dev, uint8_t *window, bool use_cache);
int adxl34x_set_window(const struct device *dev, uint8_t window);
int adxl34x_get_thresh_act(const struct device *dev, uint8_t *thresh_act, bool use_cache);
int adxl34x_set_thresh_act(const struct device *dev, uint8_t thresh_act);
int adxl34x_get_thresh_inact(const struct device *dev, uint8_t *thresh_inact, bool use_cache);
int adxl34x_set_thresh_inact(const struct device *dev, uint8_t thresh_inact);
int adxl34x_get_time_inact(const struct device *dev, uint8_t *time_inact, bool use_cache);
int adxl34x_set_time_inact(const struct device *dev, uint8_t time_inact);
int adxl34x_get_act_inact_ctl(const struct device *dev, struct adxl34x_act_inact_ctl *act_inact_ctl,
			      bool use_cache);
int adxl34x_set_act_inact_ctl(const struct device *dev,
			      struct adxl34x_act_inact_ctl *act_inact_ctl);
int adxl34x_get_thresh_ff(const struct device *dev, uint8_t *thresh_ff, bool use_cache);
int adxl34x_set_thresh_ff(const struct device *dev, uint8_t thresh_ff);
int adxl34x_get_time_ff(const struct device *dev, uint8_t *time_ff, bool use_cache);
int adxl34x_set_time_ff(const struct device *dev, uint8_t time_ff);
int adxl34x_get_tap_axes(const struct device *dev, struct adxl34x_tap_axes *tap_axes,
			 bool use_cache);
int adxl34x_set_tap_axes(const struct device *dev, struct adxl34x_tap_axes *tap_axes);
int adxl34x_get_bw_rate(const struct device *dev, struct adxl34x_bw_rate *bw_rate, bool use_cache);
int adxl34x_set_bw_rate(const struct device *dev, struct adxl34x_bw_rate *bw_rate);
int adxl34x_get_power_ctl(const struct device *dev, struct adxl34x_power_ctl *power_ctl,
			  bool use_cache);
int adxl34x_set_power_ctl(const struct device *dev, struct adxl34x_power_ctl *power_ctl);
int adxl34x_get_int_enable(const struct device *dev, struct adxl34x_int_enable *int_enable,
			   bool use_cache);
int adxl34x_set_int_enable(const struct device *dev, struct adxl34x_int_enable *int_enable);
int adxl34x_get_int_map(const struct device *dev, struct adxl34x_int_map *int_map, bool use_cache);
int adxl34x_set_int_map(const struct device *dev, struct adxl34x_int_map *int_map);
int adxl34x_get_data_format(const struct device *dev, struct adxl34x_data_format *data_format,
			    bool use_cache);
int adxl34x_set_data_format(const struct device *dev, struct adxl34x_data_format *data_format);
int adxl34x_get_fifo_ctl(const struct device *dev, struct adxl34x_fifo_ctl *fifo_ctl,
			 bool use_cache);
int adxl34x_set_fifo_ctl(const struct device *dev, struct adxl34x_fifo_ctl *fifo_ctl);
int adxl34x_get_orient_conf(const struct device *dev, struct adxl34x_orient_conf *orient_conf,
			    bool use_cache);
int adxl34x_set_orient_conf(const struct device *dev, struct adxl34x_orient_conf *orient_conf);

/* Read only registers */
int adxl34x_get_devid(const struct device *dev, uint8_t *devid);
int adxl34x_get_act_tap_status(const struct device *dev,
			       struct adxl34x_act_tap_status *act_tap_status);
int adxl34x_get_int_source(const struct device *dev, struct adxl34x_int_source *int_source);
int adxl34x_get_fifo_status(const struct device *dev, struct adxl34x_fifo_status *fifo_status);
int adxl34x_get_tap_sign(const struct device *dev, struct adxl34x_tap_sign *tap_sign);
int adxl34x_get_orient(const struct device *dev, struct adxl34x_orient *orient);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL34X_H_ */
