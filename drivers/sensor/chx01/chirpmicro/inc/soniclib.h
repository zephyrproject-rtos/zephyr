/*
 * Copyright 2023 Chirp Microsystems. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file soniclib.h
 *
 * @brief Chirp SonicLib public API and support functions for Chirp ultrasonic sensors.
 *
 * Chirp SonicLib is a set of API functions and sensor driver routines designed to easily
 * control Chirp ultrasonic sensors from an embedded C application.  It allows an
 * application developer to obtain ultrasonic range data from one or more devices, without
 * needing to develop special low-level code to interact with the sensors directly.
 *
 * The SonicLib API functions provide a consistent interface for an application to use
 * Chirp sensors in various situations.  This is especially important, because
 * all Chirp sensors are completely programmable, including the register map.  The
 * SonicLib interfaces allow an application to use new Chirp sensor firmware images,
 * without requiring code changes.  Only a single initialization parameter must be modified
 * to use the new sensor firmware.
 *
 * @note All operation of the sensor is controlled through the set of functions, data structures,
 * and symbolic values defined in this header file.  You should not need to modify this file
 * or the SonicLib functions, or use lower-level internal functions such as described in
 * the ch_driver.h file.  Using any of these non-public methods will reduce your ability to
 * benefit from future enhancements and releases from Chirp.
 *
 *
 * #### Board Support Package
 * SonicLib also defines a set of board support package (BSP) functions that must be
 * provided by the developer, board vendor, or Chirp.  The BSP functions are NOT part of
 * SonicLib - they are external interface routines that allow the SonicLib functions
 * to access the peripherals on the target board.  These functions, which all begin with
 * a "chbsp_" prefix, are described in the chirp_bsp.h header file.  See the descriptions
 * in that file for more detailed information on the BSP interfaces.
 *
 * The BSP also provides the required \a chirp_board_config.h header file, which contains
 * definitions of how many (possible) sensors and I2C buses are present on the board.  These
 * values are used for static array allocations in SonicLib.
 *
 *
 * #### Basic Operating Sequence
 * At a high level, an application using SonicLib will do the following:
 *  -# Initialize the hardware on the board, by calling the BSP's \a chbsp_board_init() function.
 *  -# Initialize the SonicLib data structures, by calling \a ch_init() for each sensor.
 *  -# Program and start the sensor(s), by calling \a ch_group_start().
 *  -# Set up a handler function to process interrupts from the sensor.
 *  -# Set up a triggering mechanism using a board timer, using \a chbsp_periodic_timer_init() etc.,
 *  (unless the sensor will be used in free-running mode, in which no external trigger is needed). A
 *  timer handler routine will typically trigger the sensor(s) using \a ch_group_trigger().
 *  -# Configure the sensor's operating mode and range, using \a ch_set_config() (or equivalent
 *  single-setting functions).
 *
 * At this point, the sensor will begin to perform measurements.  At the end of each
 * measurement cycle, the sensor will interrupt the host controller using its INT line.
 * The handler routine set up in step #4 above will be called, and it should cause the
 * application to read the measurement results from the sensor(s), using \a ch_get_range()
 * and optionally \a ch_get_amplitude() and/or \a ch_get_iq_data().
 *
 * Do not trigger a new measurement until the previous measurement has completed and all needed
 * data has been read from the device (including I/Q data, if \a ch_get_iq_data() is used).  If any
 * I/O operations are still active, the new measurement may be corrupted.
 */

#ifndef __SONICLIB_H_
#define __SONICLIB_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/* Preliminary structure type definitions to resolve include order */
typedef struct ch_dev_t ch_dev_t;
typedef struct ch_group_t ch_group_t;

/*==============  Chirp header files for installed sensor firmware packages ===================
 *
 *   If you are installing a new Chirp sensor firmware package, you must add
 *   the name of the firmware include file to the list below.  "GPR" refers
 *   to General Purpose Rangefinding versions.
 */

#include "ch101.h"
#include "ch201.h"

/* General Purpose Rangefinding (GPR) */
#include "ch101_gpr.h"                       /* GPR - CH101 */
/* #include "ch101_gpr_wd.h" */              /* GPR - CH101 (watchdog) */
/* #include "ch101_gpr_narrow.h" */          /* GPR narrow FoV horn - CH101 */
/* #include "ch101_gpr_narrow_wd.h" */       /* GPR narrow FoV horn - CH101 (watchdog) */
#include "ch101_gpr_sr.h"                    /* GPR Short-range - CH101 */
/* #include "ch101_gpr_sr_wd.h" */           /* GPR Short-range - CH101 (watchdog) */
/* #include "ch101_gpr_sr_narrow.h" */       /* GPR Short-range, narrow FoV horn - CH101 */
/* #include "ch101_gpr_sr_narrow_wd.h" */    /* GPR Short-range, narrow FoV horn - CH101 (watchdog) */
/* #include "ch101_gpr_rxopt.h" */           /* GPR Receive optimized - CH101 */
/* #include "ch101_gpr_rxopt_wd.h" */        /* GPR Receive optimized - CH101 (watchdog) */
/* #include "ch101_gpr_rxopt_narrow.h" */    /* GPR Receive optimized, narrow FoV horn - CH101 */
/* #include "ch101_gpr_rxopt_narrow_wd.h" */ /* GPR Rx optimized, narrow FoV horn - CH101 (watchdog) */
/* #include "ch201_gprmt.h" */               /* GPR / Multi-Threshold - CH201 */
/* #include "ch201_gprmt_wd.h" */            /* GPR / Multi-Threshold - CH201 (watchdog) */
/* #include "ch201_gprstr.h" */              /* GPR / Stationary Target Rejection - CH201 */
/* #include "ch201_gprstr_wd.h" */           /* GPR / Stationary Target Rejection - CH201 (watchdog) */

/* Special Purpose */
/* #include "ch101_finaltest.h" */ /* Finaltest test firmware - CH101 */
/* #include "ch101_floor.h" */     /* Floor detection - CH101 */
/* #include "ch101_gppc.h" */      /* General Purpose Pitch Catch - CH101 */
/* #include "ch101_sonicsync.h" */ /* SonicSync - CH101 */
/* #include "ch201_finaltest.h" */ /* Finaltest test firmware - CH201 */
/* #include "ch201_presence.h" */  /* Presence detection - CH201 */

/* ADD NEW HEADER FILES HERE */

/*======================== End of sensor firmware header files ================================*/

/* Miscellaneous header files */

#include "chirp_board_config.h" /* Header from board support package containing h/w params */
#include "ch_driver.h"		/* Internal Chirp driver defines */
#include "ch_math_utils.h"	/* math utility functions */

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* SonicLib API/Driver version */
/** SonicLib major version. */
#define SONICLIB_VER_MAJOR (2)
/** SonicLib minor version. */
#define SONICLIB_VER_MINOR (1)
/** SonicLib revision. */
#define SONICLIB_VER_REV   (8)

/* Chirp sensor part numbers */
/** Integer sensor identifier for CH101. */
#define CH101_PART_NUMBER (101)
/** Integer sensor identifier for CH201. */
#define CH201_PART_NUMBER (201)

/* Max expected number of samples per measurement (actual value depends on sensor f/w used) */
/** Max expected samples per measurement for CH101. */
#define CH101_MAX_NUM_SAMPLES (225)
/** Max expected samples per measurement for CH201. */
#define CH201_MAX_NUM_SAMPLES (450)

/* Misc definitions */
/** Range value returned if no target was detected. */
#define CH_NO_TARGET	 (0xFFFFFFFF)
/** Minimum range value returned for very short distances */
#define CH_MIN_RANGE_VAL (0x0001)

/** I2C address of sensor programming interface. */
#define CH_I2C_ADDR_PROG (0x45)
/** Signature byte in sensor (1 of 2). */
#define CH_SIG_BYTE_0	 (0x0a)
/** Signature byte in sensor (2 of 2). */
#define CH_SIG_BYTE_1	 (0x02)

/** Number of internal detection thresholds (CH201 only). */
#define CH_NUM_THRESHOLDS   (6)
/** Speed of sound, in meters per second. */
#define CH_SPEEDOFSOUND_MPS (343)

/** Return value codes. */
typedef enum {
	RET_OK = 0,
	RET_ERR = 1
} ch_retval;

/** Range data types. */
typedef enum {
	/** One way - gets full pulse/echo distance & divides by 2. */
	CH_RANGE_ECHO_ONE_WAY = 0,
	/** Round trip - full pulse/echo distance. */
	CH_RANGE_ECHO_ROUND_TRIP = 1,
	/** Direct - for receiving node in pitch-catch mode. */
	CH_RANGE_DIRECT = 2,
} ch_range_t;

/** Sensor operating modes. */
typedef enum {
	/** Idle mode - low-power sleep, no sensing is enabled. */
	CH_MODE_IDLE = 0x00,
	/** Free-running mode - sensor uses internal clock to wake and measure. */
	CH_MODE_FREERUN = 0x02,
	/** Triggered transmit/receive mode - transmits and receives when INT line triggered.*/
	CH_MODE_TRIGGERED_TX_RX = 0x10,
	/** Triggered receive-only mode - for pitch-catch operation with another sensor. */
	CH_MODE_TRIGGERED_RX_ONLY = 0x20
} ch_mode_t;

/** Sensor reset types. */
typedef enum {
	/** Hard reset. */
	CH_RESET_HARD = 0,
	/** Soft reset. */
	CH_RESET_SOFT = 1
} ch_reset_t;

/** I/O blocking mode flags. */
typedef enum {
	/** Blocking mode. */
	CH_IO_MODE_BLOCK = 0,
	/** Non-blocking mode. */
	CH_IO_MODE_NONBLOCK = 1
} ch_io_mode_t;

/** Time plan. */
typedef enum {
	CH_TIME_PLAN_1 = 0,
	CH_TIME_PLAN_2 = 1,
	CH_TIME_PLAN_3 = 2,
	CH_TIME_PLAN_NONE = 255
} ch_time_plan_t;

/** I2C info structure. */
typedef struct {
	/** I2C device address */
	uint8_t address;
	/** I2C bus index */
	uint8_t bus_num;
	/** flags for special handling by Chirp driver */
	uint16_t drv_flags;
} ch_i2c_info_t;

/* Flags for special I2C handling by Chirp driver (drv_flags field in ch_info_t struct) */
/** I2C interface needs reset after non-blocking transfer. */
#define I2C_DRV_FLAG_RESET_AFTER_NB (0x00000001)
/** Use programming interface for non-blocking transfer. */
#define I2C_DRV_FLAG_USE_PROG_NB    (0x00000002)

/** Sensor I/Q data value. */
typedef struct {
	/** Q component of sample */
	int16_t q;
	/** I component of sample */
	int16_t i;
} ch_iq_sample_t;

/** Detection threshold value (CH201 only). */
typedef struct {
	uint16_t start_sample;
	uint16_t level;
} ch_thresh_t;

/** Multiple detection threshold structure (CH201 only). */
typedef struct {
	ch_thresh_t threshold[CH_NUM_THRESHOLDS];
} ch_thresholds_t;

/** Calibration result structure. */
typedef struct {
	uint16_t dco_period;
	uint16_t rev_cycles;
} ch_cal_result_t;

/** Combined configuration structure. */
typedef struct {
	/** operating mode */
	ch_mode_t mode;
	/** maximum range, in mm */
	uint16_t max_range;
	/** static target rejection range, in mm (0 if unused) */
	uint16_t static_range;
	/** sample interval, only used if in free-running mode */
	uint16_t sample_interval;
	/** ptr to detection thresholds structure (if supported), should be NULL (0) for CH101 */
	ch_thresholds_t *thresh_ptr;
	/** time plan, should be 0 for CH201 */
	ch_time_plan_t time_plan;
	/** enable(1) or disable(0) target detection interrupt */
	uint8_t enable_target_int;
} ch_config_t;

/** ASIC firmware init function pointer typedef. */
typedef uint8_t (*ch_fw_init_func_t)(ch_dev_t *dev_ptr, ch_group_t *grp_ptr, uint8_t i2c_addr,
				     uint8_t dev_num, uint8_t i2c_bus_index);

/* API function pointer typedefs. */
typedef uint8_t (*ch_fw_load_func_t)(ch_dev_t *dev_ptr);
typedef uint8_t (*ch_get_config_func_t)(ch_dev_t *dev_ptr, ch_config_t *config_ptr);
typedef uint8_t (*ch_set_config_func_t)(ch_dev_t *dev_ptr, ch_config_t *config_ptr);
typedef uint8_t (*ch_set_mode_func_t)(ch_dev_t *dev_ptr, ch_mode_t mode);
typedef uint8_t (*ch_set_sample_interval_func_t)(ch_dev_t *dev_ptr, uint16_t sample_interval);
typedef uint8_t (*ch_set_num_samples_func_t)(ch_dev_t *dev_ptr, uint16_t num_samples);
typedef uint8_t (*ch_set_max_range_func_t)(ch_dev_t *dev_ptr, uint16_t max_range);
typedef uint8_t (*ch_set_sample_window_func_t)(ch_dev_t *dev_ptr, uint16_t start_sample,
					       uint16_t end_sample);
typedef uint32_t (*ch_get_range_func_t)(ch_dev_t *dev_ptr, ch_range_t range_type);
typedef uint32_t (*ch_get_tof_us_func_t)(ch_dev_t *dev_ptr);
typedef uint16_t (*ch_get_amplitude_func_t)(ch_dev_t *dev_ptr);
typedef uint16_t (*ch_get_amplitude_avg_func_t)(ch_dev_t *dev_ptr);
typedef uint8_t (*ch_set_frequency_func_t)(ch_dev_t *dev_ptr, uint32_t target_freq_Hz);
typedef uint32_t (*ch_get_frequency_func_t)(ch_dev_t *dev_ptr);
typedef uint8_t (*ch_get_iq_data_func_t)(ch_dev_t *dev_ptr, ch_iq_sample_t *buf_ptr,
					 uint16_t start_sample, uint16_t num_samples,
					 ch_io_mode_t io_mode);
typedef uint8_t (*ch_get_amplitude_data_func_t)(ch_dev_t *dev_ptr, uint16_t *buf_ptr,
						uint16_t start_sample, uint16_t num_samples,
						ch_io_mode_t io_mode);
typedef uint16_t (*ch_samples_to_mm_func_t)(ch_dev_t *dev_ptr, uint16_t num_samples);
typedef uint16_t (*ch_mm_to_samples_func_t)(ch_dev_t *dev_ptr, uint16_t num_mm);
typedef uint8_t (*ch_set_threshold_func_t)(ch_dev_t *dev_ptr, uint8_t threshold_index,
					   uint16_t amplitude);
typedef uint16_t (*ch_get_threshold_func_t)(ch_dev_t *dev_ptr, uint8_t threshold_index);
typedef uint8_t (*ch_set_thresholds_func_t)(ch_dev_t *dev_ptr, ch_thresholds_t *thresh_ptr);
typedef uint8_t (*ch_get_thresholds_func_t)(ch_dev_t *dev_ptr, ch_thresholds_t *thresh_ptr);
typedef uint8_t (*ch_set_target_interrupt_func_t)(ch_dev_t *dev_ptr, uint8_t enable);
typedef uint8_t (*ch_get_target_interrupt_func_t)(ch_dev_t *dev_ptr);
typedef uint8_t (*ch_set_static_range_func_t)(ch_dev_t *dev_ptr, uint16_t static_range);
typedef uint8_t (*ch_set_static_coeff_func_t)(ch_dev_t *dev_ptr, uint8_t static_coeff);
typedef uint8_t (*ch_get_static_coeff_func_t)(ch_dev_t *dev_ptr);
typedef uint8_t (*ch_set_rx_holdoff_func_t)(ch_dev_t *dev_ptr, uint16_t rx_holdoff);
typedef uint16_t (*ch_get_rx_holdoff_func_t)(ch_dev_t *dev_ptr);
typedef uint8_t (*ch_set_rx_low_gain_func_t)(ch_dev_t *dev_ptr, uint16_t num_samples);
typedef uint16_t (*ch_get_rx_low_gain_func_t)(ch_dev_t *dev_ptr);
typedef uint8_t (*ch_set_tx_length_func_t)(ch_dev_t *dev_ptr, uint8_t tx_length);
typedef uint8_t (*ch_get_tx_length_func_t)(ch_dev_t *dev_ptr);
typedef uint8_t (*ch_get_demodulated_rx_data_func_t)(ch_dev_t *dev_ptr, uint8_t rx_pulse_length,
						     uint8_t *data_ptr);
typedef uint8_t (*ch_set_modulated_tx_data_func_t)(ch_dev_t *dev_ptr, uint8_t ta_data);
typedef uint8_t (*ch_get_rx_pulse_length_func_t)(ch_dev_t *dev_ptr);
typedef uint8_t (*ch_set_time_plan_func_t)(ch_dev_t *dev_ptr, ch_time_plan_t time_plan);
typedef ch_time_plan_t (*ch_get_time_plan_func_t)(ch_dev_t *dev_ptr);
typedef uint8_t (*ch_set_cal_result_func_t)(ch_dev_t *dev_ptr, ch_cal_result_t *cal_ptr);
typedef uint8_t (*ch_get_cal_result_func_t)(ch_dev_t *dev_ptr, ch_cal_result_t *cal_ptr);

/** API function pointer structure (internal use). */
typedef struct {
	ch_fw_load_func_t fw_load;
	ch_set_mode_func_t set_mode;
	ch_set_sample_interval_func_t set_sample_interval;
	ch_set_num_samples_func_t set_num_samples;
	ch_set_max_range_func_t set_max_range;
	ch_set_sample_window_func_t set_sample_window;
	ch_get_range_func_t get_range;
	ch_get_tof_us_func_t get_tof_us;
	ch_get_amplitude_func_t get_amplitude;
	ch_get_amplitude_avg_func_t get_amplitude_avg;
	ch_set_frequency_func_t set_frequency;
	ch_get_frequency_func_t get_frequency;
	ch_get_iq_data_func_t get_iq_data;
	ch_get_amplitude_data_func_t get_amplitude_data;
	ch_samples_to_mm_func_t samples_to_mm;
	ch_mm_to_samples_func_t mm_to_samples;
	ch_set_threshold_func_t set_threshold;
	ch_get_threshold_func_t get_threshold;
	ch_set_thresholds_func_t set_thresholds;
	ch_get_thresholds_func_t get_thresholds;
	ch_set_target_interrupt_func_t set_target_interrupt;
	ch_get_target_interrupt_func_t get_target_interrupt;
	ch_set_static_range_func_t set_static_range;
	ch_set_static_coeff_func_t set_static_coeff;
	ch_get_static_coeff_func_t get_static_coeff;
	ch_set_rx_holdoff_func_t set_rx_holdoff;
	ch_get_rx_holdoff_func_t get_rx_holdoff;
	ch_set_rx_low_gain_func_t set_rx_low_gain;
	ch_get_rx_low_gain_func_t get_rx_low_gain;
	ch_get_demodulated_rx_data_func_t get_demodulated_rx_data;
	ch_set_tx_length_func_t set_tx_length;
	ch_get_tx_length_func_t get_tx_length;
	ch_set_modulated_tx_data_func_t set_modulated_tx_data;
	ch_get_rx_pulse_length_func_t get_rx_pulse_length;
	ch_set_time_plan_func_t set_time_plan;
	ch_get_time_plan_func_t get_time_plan;
	ch_set_cal_result_func_t set_cal_result;
	ch_get_cal_result_func_t get_cal_result;
} ch_api_funcs_t;

/** Data-ready interrupt callback routine pointer. */
typedef void (*ch_io_int_callback_t)(ch_group_t *grp_ptr, uint8_t io_index);

/** Non-blocking I/O complete callback routine pointer. */
typedef void (*ch_io_complete_callback_t)(ch_group_t *grp_ptr);

/** Periodic timer callback routine pointer. */
typedef void (*ch_timer_callback_t)(void);

/*  Chirp sensor group configuration structure. */

/** @note The \a CHIRP_MAX_NUM_SENSORS and \a CHIRP_NUM_I2C_BUSES symbols must be defined
 * by the user.  Normally this is done in the \b chirp_board_config.h header file that is
 * part of the board support package.
 */
struct ch_group_t { /* [note tag name matches type to help Doxygen linkage] */
		    /** Number of ports (max possible sensor connections) */
	uint8_t num_ports;
	/** Number of I2C buses on this board */
	uint8_t num_i2c_buses;
	/** Number of sensors detected */
	uint8_t sensor_count;
	/** Flags for special I2C handling by Chirp driver, from \a chbsp_get_i2c_info()*/
	uint16_t i2c_drv_flags;
	/** Real-time clock calibration pulse length (in ms) */
	uint16_t rtc_cal_pulse_ms;
	/** Pre-trigger delay for rx-only sensors (in us) */
	uint16_t pretrig_delay_us;
	/** Addr of hook routine to call when device found on bus */
	chdrv_discovery_hook_t disco_hook;
	/** Addr of routine to call when sensor interrupts */
	ch_io_int_callback_t io_int_callback;
	/** Addr of routine to call when non-blocking I/O completes */
	ch_io_complete_callback_t io_complete_callback;
	/** Array of pointers to ch_dev_t structures for individual sensors */
	ch_dev_t *device[CHIRP_MAX_NUM_SENSORS];
	/** Array of counters for connected sensors per bus */
	uint8_t num_connected[CHIRP_NUM_I2C_BUSES];
	/** Array of I2C non-blocking transaction queues (one per bus) */
	chdrv_i2c_queue_t i2c_queue[CHIRP_NUM_I2C_BUSES];
};

/** Chirp sensor device structure. */
struct ch_dev_t {
	/** Pointer to parent group structure. */
	ch_group_t *group;
	/** Sensor operating mode. */
	ch_mode_t mode;
	/** Value set when sensor has locked */
	uint8_t freqLockValue;
	/** Frequency counter cycles */
	uint16_t freqCounterCycles;
	/** Maximum range, in mm */
	uint16_t max_range;
	/** Static target rejection range, in samples (0 if unused) */
	uint16_t static_range;
	/** Sample interval (in ms), only if in free-running mode */
	uint16_t sample_interval;
	/** Real-time clock calibration result for the sensor. */
	uint16_t rtc_cal_result;
	/** Operating frequency for the sensor. */
	uint32_t op_frequency;
	/** Bandwidth for the sensor. */
	uint16_t bandwidth;
	/** Scale factor for the sensor. */
	uint16_t scale_factor;
	/** Current I2C addresses. */
	uint8_t i2c_address;
	/** Assigned application I2C address for device in normal operation*/
	uint8_t app_i2c_address;
	/** Flags for special I2C handling by Chirp driver */
	uint16_t i2c_drv_flags;
	/** Integer part number (e.g. 101 for a CH101 device). */
	uint16_t part_number;
	/** Oversampling factor (power of 2) */
	int8_t oversample;
	/** Sensor connection status: 1 if discovered and successfully initialized, 0 otherwise. */
	uint8_t sensor_connected;
	/** Index value (device number) identifying device within group */
	uint8_t io_index;
	/** Index value identifying which I2C bus is used for this device. */
	uint8_t i2c_bus_index;
	/** Maximum number of receiver samples for this sensor firmware */
	uint16_t max_samples;
	/** Number of receiver samples for the current max range setting. */
	uint16_t num_rx_samples;
	/** Starting sample of sample window, if supported */
	uint16_t win_start_sample;
	/** Number of samples in sample window, if supported */
	uint16_t num_win_samples;

	/* Sensor Firmware-specific Linkage Definitions */
	/** Pointer to string identifying sensor firmware version. */
	const char *fw_version_string;
	/** Pointer to start of sensor firmware image to be loaded */
	const uint8_t *firmware;
	/** Pointer to ram initialization data */
	const uint8_t *ram_init;
	/**
	 * Pointer to function preparing sensor pulse timer to measure real-time clock (RTC)
	 * calibration pulse sent to device.
	 */
	void (*prepare_pulse_timer)(ch_dev_t *dev_ptr);
	/**
	 * Pointer to function to read RTC calibration pulse timer result from sensor and place
	 * value in the \a rtc_cal_result field.
	 */
	void (*store_pt_result)(ch_dev_t *dev_ptr);
	/** Pointer to function to read operating frequency and place value in the \a op_frequency
	 * field. */
	void (*store_op_freq)(ch_dev_t *dev_ptr);
	/** Pointer to function to read operating bandwidth and place value in the \a bandwidth
	 * field. */
	void (*store_bandwidth)(ch_dev_t *dev_ptr);
	/** Pointer to function to calculate scale factor and place value in \a scalefactor field.
	 */
	void (*store_scalefactor)(ch_dev_t *dev_ptr);
	/** Pointer to function returning locked state for sensor. */
	uint8_t (*get_locked_state)(ch_dev_t *dev_ptr);
	/** Pointer to function returning ram init size for sensor. */
	uint16_t (*get_fw_ram_init_size)(void);
	/** Pointer to function returning start address of ram initialization area in the sensor. */
	uint16_t (*get_fw_ram_init_addr)(void);

	/* API and callback functions */
	/** Structure containing API function pointers. */
	ch_api_funcs_t api_funcs;
};

/* API function prototypes and documentation */

/**
 * @brief Initialize the device descriptor for a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param grp_ptr pointer to the ch_group_t descriptor for sensor group to join
 * @param dev_num number of the device within the sensor group (identifies which physical
 * sensor)
 * @param fw_init_func pointer to the sensor firmware initialization function (determines sensor
 *                     	feature set)
 *
 * @return 0 if success, 1 if error
 *
 * This function is used to initialize various Chirp SonicLib structures before using a sensor.
 * The ch_dev_t device descriptor is the primary data structure used to manage a sensor, and its
 * address will subsequently be used as a handle to identify the sensor when calling most API
 * functions.
 *
 * The \a dev_ptr parameter is the address of the ch_dev_t descriptor structure that will be
 * initialized and then used to identify and manage this sensor. The \a grp_ptr parameter is the
 * address of a ch_group_t structure describing the sensor group that will include the new sensor.
 * Both the ch_dev_t structure and the ch_group_t structure must have already been allocated before
 * this function is called.
 *
 * Generally, an application will require only one ch_group_t structure to manage all Chirp sensors.
 * However, a separate ch_dev_t structure must be allocated for each sensor.
 *
 * \a dev_num is a simple index value that uniquely identifies a sensor within a group.  Each
 * possible sensor (i.e. each physical port on the board that could have a Chirp sensor attached)
 * has a number, starting with zero (0).  The device number is constant - it remains associated with
 * a specific port even if no sensor is actually attached.  Often, the \a dev_num value is used by
 * both the application and the board support package as an index into arrays containing per-sensor
 * information (e.g. data read from the sensors, pin assignments, etc.).
 *
 * The Chirp sensor is fully re-programmable, and the specific features and capabilities can be
 * modified by using different sensor firmware images.  The \a fw_init_func parameter is the address
 * (name) of the sensor firmware initialization routine that should be used to program the sensor
 * and prepare it for operation. The selection of this routine name is the only required change when
 * switching from one sensor firmware image to another.
 *
 * @note This function only performs internal initialization of data structures, etc.  It does not
 * actually initialize the physical sensor device(s).  See \a ch_group_start().
 */
uint8_t ch_init(ch_dev_t *dev_ptr, ch_group_t *grp_ptr, uint8_t dev_num,
		ch_fw_init_func_t fw_init_func);

/**
 * @brief Program and start a group of sensors.
 *
 * @param grp_ptr pointer to the ch_group_t descriptor for sensor group to be started
 *
 * @return 0 if successful, 1 if error
 *
 * This function performs the actual discovery, programming, and initialization sequence for all
 * sensors within a sensor group.  Each sensor must have previously been added to the group by
 * calling \a ch_init().
 *
 * In brief, this function does the following for each sensor:
 * - Probe the possible sensor ports using I2C bus and each sensor's PROG line, to discover if
 * sensor is connected.
 * - Reset sensor.
 * - Program sensor with firmware (version specified during \a ch_init()).
 * - Assign unique I2C address to sensor (specified by board support package, see \a
 * chbsp_i2c_get_info()).
 * - Start sensor execution.
 * - Wait for sensor to lock (complete initialization, including self-test).
 * - Send timed pulse on INT line to calibrate sensor Real-Time Clock (RTC).
 *
 * After this routine returns successfully, the sensor configuration may be set and ultrasonic
 * measurements may begin.
 *
 * @note
 * When the sensor is programmed, the entire sensor firmware image is normally written in a
 * single, large I2C transfer.  To limit the size of this transfer for a specific hardware platform,
 * define the \b MAX_PROG_XFER_SIZE symbol in the \b chirp_board_config.h header file.  For more
 * information, see chirp_bsp.h.
 */
uint8_t ch_group_start(ch_group_t *grp_ptr);

/**
 * @brief Get current configuration settings for a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param config_ptr pointer to a ch_config_t structure to receive configuration values
 *
 * @return 0 if successful, 1 if error
 *
 * This function obtains the current configuration settings from the sensor and returns them
 * in a ch_config_t structure, whose address is specified by \a config_ptr.
 *
 * @note The individual configuration values returned in the ch_config_t structure may also be
 * obtained by using dedicated single-value functions.  See \a ch_get_mode(), \a ch_get_max_range(),
 * \a ch_get_sample_interval(), \a ch_get_static_range(), and \a ch_get_thresholds().
 */
uint8_t ch_get_config(ch_dev_t *dev_ptr, ch_config_t *config_ptr);

/**
 * @brief Set multiple configuration settings for a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param config_ptr pointer to a ch_config_t structure containing new configuration
 * values
 *
 * @return 0 if successful, 1 if error
 *
 * This function sets multiple configuration options within the sensor.  The configuration settings
 * are passed in a ch_config_t structure, whose address is specified by \a config_ptr.  The fields
 * in the ch_config_t structure must have been set with your new configuration values before this
 * function is called.
 *
 * @note The individual configuration values set by this function may also be set using dedicated
 * single-value functions. These two methods are completely equivalent and may be freely mixed.
 * See \a ch_set_mode(), \a ch_set_max_range(), \a ch_set_sample_interval(), \a
 * ch_set_static_range(), and \a ch_set_thresholds().
 */
uint8_t ch_set_config(ch_dev_t *dev_ptr, ch_config_t *config_ptr);

/**
 * @brief Trigger a measurement on one sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * This function generates a pulse on the INT line for a single sensor.  If the sensor is in either
 * \a CH_MODE_TRIGGERED_TX_RX or \a CH_MODE_TRIGGERED_RX_ONLY mode, this pulse will begin a
 * measurement cycle.
 *
 * To simultaneously trigger all sensors in a group, use \a ch_group_trigger().
 *
 * @note Do not trigger a new measurement until the previous measurement has completed and all
 * needed data has been read from the device (including I/Q data, if \a ch_get_iq_data() is used).
 * If any I/O operations are still active, the new measurement may be corrupted.
 */
void ch_trigger(ch_dev_t *dev_ptr);

/**
 * @brief Trigger a measurement on a group of sensors.
 *
 * @param grp_ptr pointer to the ch_group_t descriptor for this group of sensors
 *
 * This function generates a pulse on the INT line for each sensor in the sensor group.  If a sensor
 * is in either \a CH_MODE_TRIGGERED_TX_RX or \a CH_MODE_TRIGGERED_RX_ONLY mode, this pulse will
 * begin a measurement cycle.
 *
 * If a two or more sensors are operating in pitch-catch mode (in which one transmits and the others
 * receive), this function must be used to start a measurement cycle, so that the devices are
 * synchronized.
 *
 * To trigger a single sensor, use \a ch_trigger().
 *
 * @note Do not trigger a new measurement until the previous measurement has completed and all
 * needed data has been read from the device (including I/Q data, if \a ch_get_iq_data() is used).
 * If any I/O operations are still active, the new measurement may be corrupted.
 */
void ch_group_trigger(ch_group_t *grp_ptr);

/**
 * @brief Reset a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param reset_type type of reset (\a CH_RESET_HARD or \a CH_RESET_SOFT)
 *
 * This function resets a sensor.  The \a reset_type parameter indicates if a software reset or
 * full hardware reset is requested.
 */
void ch_reset(ch_dev_t *dev_ptr, ch_reset_t reset_type);

/**
 * @brief Reset a group of sensors.
 *
 * @param grp_ptr pointer to the ch_group_t descriptor for this group of sensors
 * @param reset_type type of reset (\a CH_RESET_HARD or \a CH_RESET_SOFT)
 *
 * This function resets all sensors in a sensor group.  The \a reset_type parameter indicates if a
 * software reset or full hardware reset is requested.
 */
void ch_group_reset(ch_group_t *grp_ptr, ch_reset_t reset_type);

/**
 * @brief Indicate if a sensor is connected.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @return 1 if the sensor is connected, 0 otherwise
 */
uint8_t ch_sensor_is_connected(ch_dev_t *dev_ptr);

/**
 * @brief Get part number for a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @return Integer part number
 *
 * This function returns the Chirp part number for the specified device.  The
 * part number is a simple integer value, for example 101 for a CH101 device.
 */
uint16_t ch_get_part_number(ch_dev_t *dev_ptr);

/**
 * @brief Get device number (I/O index values) for a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return Device number
 *
 * This function returns the device number (I/O index) of the sensor within its sensor group.
 * Normally, this also corresponds to the sensor's port number on the board, and is used for
 * indexing arrays of pin definitions etc. within the board support package routines.
 */
uint8_t ch_get_dev_num(ch_dev_t *dev_ptr);

/**
 * @brief Get device descriptor pointer for a sensor.
 *
 * @param grp_ptr pointer to the ch_group_t descriptor for this group of sensors
 * @param dev_num device number within sensor group
 *
 * @return Pointer to ch_dev_t descriptor structure
 *
 * This function returns the address of the ch_dev_t device descriptor for a certain sensor in
 * a sensor group.  The sensor is identified within the group by the \a dev_num device number.
 */
ch_dev_t *ch_get_dev_ptr(ch_group_t *grp_ptr, uint8_t dev_num);

/**
 * @brief Get the total number of sensor ports (possible sensors) in a sensor group.
 *
 * @param grp_ptr pointer to the ch_group_t descriptor for this group of sensors
 *
 * @return Total number of ports (possible sensors) in the sensor group
 *
 * This function returns the maximum number of possible sensors within a sensor group.  Typically,
 * the number of sensors is limited by the physical connections on the board being used, so the
 * number of sensor ports on the board is returned by this function.
 */
uint8_t ch_get_num_ports(ch_group_t *grp_ptr);

/**
 * @brief Get the active I2C address for a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return I2C address, or 0 if error
 *
 * This function returns the currently active I2C address for a sensor device.  This function
 * may be used by board support package routines to determine the proper I2C address to use for
 * a specified sensor.
 */
uint8_t ch_get_i2c_address(ch_dev_t *dev_ptr);

/**
 * @brief Get the active I2C bus for a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return I2C bus index
 *
 * This function returns the I2C bus index for a sensor device.  This function
 * may be used by board support package routines to determine the proper I2C bus to use for
 * a specified sensor.
 */
uint8_t ch_get_i2c_bus(ch_dev_t *dev_ptr);

/**
 * @brief Get the firmware version description string for a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return Pointer to character string describing sensor firmware version
 *
 * This function returns a pointer to a string that describes the sensor firmware being used
 * on the device.
 */
const char *ch_get_fw_version_string(ch_dev_t *dev_ptr);

/**
 * @brief Get the current operating mode for a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t config structure
 *
 * @return Sensor operating mode
 *
 * This function returns the current operating mode for the sensor, one of:
 *	- \a CH_MODE_IDLE - low power idle mode, no measurements take place
 *	- \a CH_MODE_FREERUN - free-running mode, sensor uses internal clock to wake and measure
 *	- \a CH_MODE_TRIGGERED_TX_RX - hardware-triggered, sensor both transmits and receives
 *	- \a CH_MODE_TRIGGERED_RX_ONLY - hardware triggered, sensor only receives
 */
ch_mode_t ch_get_mode(ch_dev_t *dev_ptr);

/**
 * @brief Configure a sensor for the specified operating mode.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param mode the new operating mode for the sensor
 * @return 0 if successful.
 *
 * This function sets the sensor to operate in the specified mode, which
 * must be one of the following:
 *	- \a CH_MODE_IDLE - low power idle mode, no measurements take place
 *	- \a CH_MODE_FREERUN - free-running mode, sensor uses internal clock to wake and measure
 *	- \a CH_MODE_TRIGGERED_TX_RX - hardware-triggered, sensor both transmits and receives
 *	- \a CH_MODE_TRIGGERED_RX_ONLY - hardware triggered, sensor only receives
 */
uint8_t ch_set_mode(ch_dev_t *dev_ptr, ch_mode_t mode);

/**
 * @brief Get the internal sample timing interval for a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @return Interval between samples (in ms), or 0 if device is not in free-running mode
 *
 * This function returns the interval between measurements, in milliseconds, for
 * a sensor operating in free-running mode.  If the sensor is in a different operating
 * mode (e.g. a triggered mode), zero is returned.
 */
uint16_t ch_get_sample_interval(ch_dev_t *dev_ptr);

/**
 * @brief Configure the internal sample interval for a sensor in freerunning mode.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param interval_ms interval between samples, in milliseconds.
 * @return 0 if successful, 1 if arguments are invalid.
 *
 * This function sets the sample interval for a sensor operating in freerunning mode
 * (\a CH_MODE_FREERUN).  The sensor will use its internal clock to wake and perform a
 * measurement every \a interval_ms milliseconds.  A value of zero for \a interval_ms
 * is not valid.
 *
 * @note This function has no effect for a sensor operating in one of the triggered modes.
 * The sample interval for a triggered device is determined by the external trigger timing.
 */
uint8_t ch_set_sample_interval(ch_dev_t *dev_ptr, uint16_t interval_ms);

/**
 * @brief Get the number of samples per measurement cycle.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor struct
 *
 * @return Number of samples per measurement cycle
 *
 * This function returns the current number of samples which the Chirp sensor will perform
 * during each measurement cycle.  The number of samples directly corresponds to the
 * range at which the sensor can detect, so this value is determined by the current maximum
 * range setting for the sensor.  Also see \a ch_get_max_range().
 *
 */
uint16_t ch_get_num_samples(ch_dev_t *dev_ptr);

/**
 * @brief Set the sensor sample count directly.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor struct
 * @param num_samples number of samples during each measurement cycle
 *
 * @return 0 if successful
 *
 * This function directly sets the number of samples which the Chirp sensor will perform
 * during a single measurement cycle.  The number of samples directly corresponds to the
 * range at which the sensor can detect.
 *
 * Also see \a ch_set_max_range().
 *
 * @note Normally, the sample is count is not set using this function, but is instead set
 * indirectly using either \a ch_set_max_range() or \a ch_set_config(), both of which
 * automatically set the sample count based on a specified range in millimeters.
 */
uint8_t ch_set_num_samples(ch_dev_t *dev_ptr, uint16_t num_samples);

/**
 * @brief Get the maximum possible sample count per measurement.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor struct
 *
 * @return Maximum sample count for this device and firmware
 *
 * This function returns the maximum number of samples that can be included in a measurement.
 * This maximum sample count will vary depending on the sensor device (CH201 devices will
 * have higher sample counts than CH101 devices) and by the specific sensor firmware
 * that is being used.
 *
 * To get the number of these possible samples that are currently active (based on the maximum
 * range setting), use \a ch_get_num_samples().
 *
 */
uint16_t ch_get_max_samples(ch_dev_t *dev_ptr);

/**
 * @brief Get the maximum range setting for a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return Maximum range setting, in millimeters
 *
 * This function returns the current maximum detection range setting for the sensor,
 * in millimeters.
 *
 * @note The maximum range may also be obtained, along with other settings, using the
 * \a ch_get_config() function.
 */
uint16_t ch_get_max_range(ch_dev_t *dev_ptr);

/**
 * @brief Set the maximum range for a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param max_range maximum range, in millimeters
 *
 * @return 0 if successful, non-zero if error
 *
 * This function sets the maximum detection range for the sensor, in millimeters.  The
 * detection range setting controls how long the sensor will listen (i.e. how many samples
 * it will capture) during each measurement cycle.  (The number of samples is automatically
 * calculated for the specified range.)
 *
 * @note The maximum range may also be specified, along with other settings, using the
 * \a ch_set_config() function.  These two methods are completely equivalent and may
 * be freely mixed.
 */
uint8_t ch_set_max_range(ch_dev_t *dev_ptr, uint16_t max_range);

/**
 * @brief Get the sample window for amplitude averaging.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param start_sample_ptr pointer to variable to be updated with sample number of first sample
 * @param num_samples_ptr pointer to variable to be updated with number of samples
 *
 * @return 0 if successful, non-zero if error
 *
 * This function obtains the current range of samples that are included in the sample window used
 * for amplitude averaging.  \a start_sample_ptr is a pointer to a variable that will be updated
 * with the number of the first sample in the sample window.  \a num_samples_ptr is a pointer to a
 * variable that will be updated with the the total number of samples in the sample window.
 *
 * Also see \a ch_get_amplitude_avg() and \a ch_set_sample_window().
 *
 * @note Internal sample window averaging is available when using special sensor firmware packages
 * from Chirp.  In General Purpose Rangefinding (GPR) firmware, a software only implementation
 * is used.
 *
 */
uint8_t ch_get_sample_window(ch_dev_t *dev_ptr, uint16_t *start_sample_ptr,
			     uint16_t *num_samples_ptr);

/**
 * @brief Set the sample window for amplitude averaging.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param start_sample sample number of first sample in window
 * @param num_samples number of samples to include in window
 *
 * @return 0 if successful, non-zero if error
 *
 * This function sets the sample range to be included in the sample window used for
 * amplitude averaging.  \a start_sample is the number of the first sample that will be
 * included in the averaging window.  \a num_samples is the total number of samples
 * that will be included.
 *
 * Also see \a ch_get_amplitude_avg() and \a ch_get_sample_window().
 *
 * @note Internal sample window averaging is available when using special sensor firmware packages
 * from Chirp.  In General Purpose Rangefinding (GPR) firmware, a software only implementation
 * is used.
 */
uint8_t ch_set_sample_window(ch_dev_t *dev_ptr, uint16_t start_sample, uint16_t num_samples);

/**
 * @brief Get static target rejection range setting.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return Static target rejection range setting, in samples, or 0 if not enabled
 *
 * This function returns the number of samples at the beginning of a measurement cycle over
 * which static target rejection filtering will be applied.   Also see \a ch_set_static_range().
 *
 * To calculate the physical distance that corresponds to the number of samples, use the
 * \a ch_samples_to_mm() function.
 */
uint16_t ch_get_static_range(ch_dev_t *dev_ptr);

/**
 * @brief Configure static target rejection.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param num_samples number of sensor samples (at beginning of measurement cycle) over
 *                    	which static targets will be rejected
 *
 * @return 0 if successful, non-zero if error
 *
 * Static target rejection is a special processing mode in which the sensor will actively filter
 * out signals from close, non-moving objects, so that they do not continue to generate range
 * readings.  This allows detection and reporting of target objects that are farther away than the
 * static objects.  (Normally, the sensor reports the range value for the closest detected object.)
 *
 * Static target rejection is applied for a specified number of samples, starting at the beginning
 * of a measurement cycle* (i.e. for the closest objects).  The num_samples parameter specifies the
 * number of samples that will be filtered.  To calculate the appropriate value for \a num_samples
 * to filter over a certain physical distance, use the \a ch_mm_to_samples() function.
 */
uint8_t ch_set_static_range(ch_dev_t *dev_ptr, uint16_t num_samples);

/**
 * @brief Get the measured range from a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param range_type the range type to be reported (e.g. one-way vs. round-trip)
 *
 * @return Range in millimeters times 32, or \a CH_NO_TARGET (0xFFFFFFFF) if no target was detected,
 *         or 0 if error
 *
 * This function reads the measurement result registers from the sensor and then computes the actual
 * range.  It should be called after the sensor has indicated that a measurement cycle is complete
 * by generating a signal on the INT line.  (Typically, this will be set up by an interrupt handler
 * associated with that input line.)
 *
 * The \a range_type parameter indicates whether the measurement is based on the one-way or
 * round-trip distance to/from a target, or the direct distance between two sensors operating in
 * pitch-catch mode. The possible values are:
 *   - \a CH_RANGE_ECHO_ONE_WAY -  gets full pulse/echo round-trip distance, then divides by 2
 *   - \a CH_RANGE_ECHO_ROUND_TRIP	- full pulse/echo round-trip distance
 *   - \a CH_RANGE_DIRECT - for receiving sensor in pitch-catch mode (one-way)
 *
 * This function returns the measured range as a 32-bit integer.  For maximum precision, the range
 * value is returned in a fixed-point format with 5 fractional bits.  So, the return value is the
 * number of millimeters times 32. Divide the value by 32 (shift right 5 bits) to get whole mm, or
 * use floating point (i.e.  divide by 32.0f) to preserve the full sub-millimeter precision.
 *
 * If the sensor did not successfully find the range of a target during the most recent measurement,
 * the returned range value will be \a CH_NO_TARGET.  If an error occurs when getting or calculating
 * the range, zero (0) will be returned.
 *
 * @note This function only reports the results from the most recently completed measurement cycle.
 * It does not actually trigger a measurement.
 *
 * @note The \a range_type parameter only controls how this function interprets the results from the
 * measurement cycle.  It does not change the sensor mode.
 *
 */
uint32_t ch_get_range(ch_dev_t *dev_ptr, ch_range_t range_type);

/**
 * @brief Get the measured Time-of-flight from a sensor in the ultrasound periodic timer tick.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return Time-of-flight in microseconds,
 *         or 0 if error
 *
 * This function reads the measurement result registers from the sensor and then computes the
 * Time-of-flight in microseconds.
 *
 * This function returns the measured time as a 32-bit integer.
 *
 * If the sensor did not successfully find the range of a target during the most recent measurement,
 * the returned value will be \a zero (0).  If an error occurs when getting or calculating the
 * range, zero (0) will be returned.
 *
 * @note This function only reports the results from the most recently completed measurement cycle.
 * It does not actually trigger a measurement.
 *
 * @note This function is only available when using special sensor firmware packages
 * from Chirp.
 *
 */
uint32_t ch_get_tof_us(ch_dev_t *dev_ptr);

/**
 * @brief Get the measured amplitude from a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return Amplitude value for most recent successful range reading
 *
 * This function returns the amplitude value for the most recent successful range measurement by
 * the sensor. The amplitude is representative of the incoming sound pressure.  The value is
 * expressed in internal sensor counts and is not calibrated to any standard units.
 *
 * The amplitude value is not updated if a measurement cycle resulted in \a CH_NO_TARGET, as
 * returned by \a ch_get_range().
 */
uint16_t ch_get_amplitude(ch_dev_t *dev_ptr);

/**
 * @brief Get the averaged measured amplitude over the sample window.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return Average amplitude value for samples in current sample window
 *
 * This function returns the average amplitude value for the most recent measurement cycle across
 * the samples within the current sample window.  The sample window is a subset of the overall
 * sensor data, and is specified by the \a ch_set_sample_window() function.
 *
 * The amplitude is representative of the incoming sound pressure.  The value is expressed in
 * internal sensor counts and is not calibrated to any standard units.
 *
 * @note Sample window averaging is only available when using special sensor firmware packages
 * from Chirp.  It is generally not available in General Purpose Rangefinding (GPR) firmware.
 *
 */
uint16_t ch_get_amplitude_avg(ch_dev_t *dev_ptr);

/**
 * @brief Get the bandwidth of a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * This function returns the operating frequency of the sensor.  This is the primary frequency of
 * the ultrasonic pulse that is emitted by the device when transmitting.
 *
 * @return Sensor bandwidth, in Hz, or 0 if error or bandwidth measurement is not available
 *
 * @note The bandwidth measurement is only available when using special sensor firmware packages
 * from Chirp.  It is generally not available in General Purpose Rangefinding (GPR) firmware.
 */
uint16_t ch_get_bandwidth(ch_dev_t *dev_ptr);

/**
 * @brief Set the operating frequency of a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param target_freq_hz operating frequency in Hz
 *
 * @return 0 if success, 1 if error
 *
 * This function sets the target operating frequency of the sensor.  This is the primary frequency
 * of the ultrasonic pulse that is emitted by the device when transmitting.  The resulting operating
 * frequency may be slightly different than the requested value.
 *
 * See also \a ch_get_frequency().
 *
 * @note This function is only available when using special sensor firmware packages from Chirp.
 */
uint8_t ch_set_frequency(ch_dev_t *dev_ptr, uint32_t target_freq_hz);

/**
 * @brief Get the operating frequency of a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * This function returns the operating frequency of the sensor.  This is the primary frequency of
 * the ultrasonic pulse that is emitted by the device when transmitting.
 *
 * @return Frequency, in Hz
 */
uint32_t ch_get_frequency(ch_dev_t *dev_ptr);

/**
 * @brief Get the real-time clock calibration value.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return RTC calibration value
 *
 * This function returns the real-time clock (RTC) calibration value read from the sensor during
 * \a ch_group_start().  The RTC calibration value is calculated by the sensor during the RTC
 * calibration pulse, and it is used internally in calculations that convert between time and
 * distance.
 */
uint16_t ch_get_rtc_cal_result(ch_dev_t *dev_ptr);

/**
 * @brief Get the real-time clock calibration pulse length.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return RTC pulse length, in ms
 *
 * This function returns the length (duration), in milliseconds, of the the real-time clock (RTC)
 * calibration pulse used for the sensor.  The pulse is applied to the sensor's INT line during
 * \a ch_group_start() to calibrate the sensor's internal clock.  The pulse length is specified by
 * the board support package during the \a chbsp_board_init() function.
 *
 * The RTC calibration pulse length is used internally in calculations that convert between time
 * and distance.
 */
uint16_t ch_get_rtc_cal_pulselength(ch_dev_t *dev_ptr);

/**
 * @brief Get the calibration scale factor of a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * This function returns the calibration scale factor of the sensor.  The scale factor is an
 * internal value generated during the initialization of the device.
 *
 * @return Scale factor value, or 0 if error or not available
 *
 * @note The scale factor value is only available when using special sensor firmware packages
 * from Chirp.
 */
uint16_t ch_get_scale_factor(ch_dev_t *dev_ptr);

/**
 * @brief Get the raw I/Q measurement data from a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param buf_ptr pointer to data buffer where I/Q data will be written
 * @param start_sample starting sample number within measurement data (0 = start of data)
 * @param num_samples number of samples to read from sensor
 * @param mode whether I/Q read should block (\a CH_IO_MODE_BLOCK (0) = blocking,
 * 												   	\a CH_IO_MODE_NONBLOCK (1) =
 * non-blocking)
 *
 * @return 0 if successful, 1 if error
 *
 * This function reads the raw I/Q measurement data from the sensor.  The I/Q data set includes
 * a discrete pair of values for each of the samples that make up a full measurement cycle.   Each
 * individual sample is reported as a pair of values, I and Q, in a quadrature format.  The I/Q
 * values may be used to calculate the relative amplitude of the measured ultrasound signal.
 *
 * The contents of the I/Q trace are updated on every measurement cycle, even if no target was
 * detected (i.e. even if \a ch_get_range() returns \a CH_NO_TARGET).  (Note that this is different
 * than the regular amplitude value, as returned by \a ch_get_amplitude(), which is \a not updated
 * unless a target is detected.)
 *
 * Each sample I/Q pair consists of two signed 16-bit integers and is described by the
 * \a ch_iq_sample_t structure. To convert any given pair of I/Q values to the amplitude value for
 * that sample, square both I and Q, and take the square root of the sum:
 * 			\f[Amp_n = \sqrt{(I_n)^2 + (Q_n)^2}\f]
 * Amplitude values in the sensor are expressed only in internal ADC counts (least-significant bits,
 * or LSBs) and are not calibrated to any standard units.
 *
 * The number of samples used in each I/Q trace is determined by the maximum range setting for the
 * device.  If it is set to less than the maximum possible range, not all samples will contain valid
 * data.  To determine the number of active samples within the trace, use \a ch_get_num_samples().
 *
 *  - To read all valid I/Q data, set \a start_sample to zero (0), and set \a num_samples to the
 *  value returned by \a ch_get_num_samples().
 *
 * To determine what sample number corresponds to a physical distance, use \a ch_mm_to_samples().
 *
 * To allow more flexibilty in your application, the I/Q data readout from the device may be done
 * in a non-blocking mode, by setting \a mode to \a CH_IO_MODE_NONBLOCK (1).  In non-blocking
 * mode, the I/O operation takes place using DMA access in the background.  This function will
 * return immediately, and a notification will later be issued when the I/Q has been read.  To
 * use the \a non_block option, the board support package (BSP) you are using must provide the
 * \a chbsp_i2c_read_nb() and \a chbsp_i2c_read_mem_nb() functions.  To use non-blocking reads
 * of the I/Q data, you must specify a callback routine that will be called when the I/Q read
 * completes.  See \a ch_io_complete_callback_set().
 *
 * Non-blocking reads are managed together for a group of sensors.  To perform a non-blocking read:
 *
 *  -# Register a callback function using \a ch_io_complete_callback_set().
 *  -# Define and initialize a handler for the DMA interrupts generated.
 *  -# Synchronize with all sensors whose I/Q data should be read by waiting for all to indicate
 *     data ready.
 *  -# Set up a non-blocking read on each sensor, using \a ch_get_iq_data() with \a mode =
 *     \a CH_IO_MODE_NONBLOCK (1).
 *  -# Start the non-blocking reads on all sensors in the group, using \a ch_io_start_nb().
 *  -# Your callback function (set in step #1 above) will be called as each individual sensor's
 *     read completes.  Your callback function should initiate any further processing of the I/Q
 *     data, possibly by setting a flag that will be checked from within the application's main
 *     execution loop.  The callback function will likely be called at interrupt level, so the
 *     amount of processing within it should be kept to a minimum.
 *
 * For a CH101 sensor, up to 150 samples are taken during each measurement cycle; for a CH201
 * sensor, up to 450 samples are taken.  So, a complete CH101 I/Q trace will contain up to 600
 * bytes of data (150 samples x 4 bytes per sample), and a CH201 I/Q trace may contain up to
 * 1800 bytes.  The buffer specified by \a buf_ptr must be large enough to hold this amount of data.
 *
 * When the I/Q data is read from the sensor, the additional time required to transfer the I/Q
 * data over the I2C bus must be taken into account when planning how often the sensor can be
 * read (sample interval).
 *
 * @note It is important that any data I/O operations to or from the sensor, including reading
 * the I/Q data, complete before a new measurement cycle is triggered, or the new measurement may
 * be affected.
 *
 * @note This function only obtains the data from the most recently completed measurement cycle.
 * It does not actually trigger a measurement.
 */
uint8_t ch_get_iq_data(ch_dev_t *dev_ptr, ch_iq_sample_t *buf_ptr, uint16_t start_sample,
		       uint16_t num_samples, ch_io_mode_t mode);

/**
 * @brief Get the raw amplitude measurement data from a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param buf_ptr pointer to data buffer where amplitude data will be written
 * @param start_sample starting sample number within measurement data (0 = start of data)
 * @param num_samples number of samples to read from sensor
 * @param mode whether read should block (\a CH_IO_MODE_BLOCK (0) = blocking,
 * 											   	\a CH_IO_MODE_NONBLOCK (1) =
 * non-blocking)
 *
 * @return 0 if successful, 1 if error
 *
 * The raw amplitude data are updated on every measurement cycle, even if no target was
 * detected (i.e. even if \a ch_get_range() returns \a CH_NO_TARGET).  (Note that this is different
 * than the regular amplitude value, as returned by \a ch_get_amplitude(), which is \a not updated
 * unless a target is detected.)
 *
 * Each sample amplitude consists of one unsigned 16-bit integer value.
 *
 * Amplitude values in the sensor are expressed only in internal ADC counts (least-significant bits,
 * or LSBs) and are not calibrated to any standard units.
 *
 * The number of samples available in each amplitude trace is determined by the maximum range
 * setting for the device.  If it is set to less than the maximum possible range, not all samples
 * will contain valid data.  To determine the number of active samples within the trace, use \a
 * ch_get_num_samples().
 *
 *  - To read all valid amplitude data, set \a start_sample to zero (0), and set \a num_samples to
 * the value returned by \a ch_get_num_samples().
 *
 * To determine what sample number corresponds to a physical distance, use \a ch_mm_to_samples().
 *
 * To allow more flexibilty in your application, the amplitude data readout from the device may be
 * done in a non-blocking mode, by setting \a mode to \a CH_IO_MODE_NONBLOCK (1).  In non-blocking
 * mode, the I/O operation takes place using DMA access in the background.  This function will
 * return immediately, and a notification will later be issued when the amplitude has been read.  To
 * use the \a non_block option, the board support package (BSP) you are using must provide the
 * \a chbsp_i2c_read_nb() and \a chbsp_i2c_read_mem_nb() functions.  To use non-blocking reads
 * of the amplitude data, you must specify a callback routine that will be called when the amplitude
 * read completes.  See \a ch_io_complete_callback_set().
 *
 * @note Non-blocking amplitude reads are only supported when using certain Chirp sensor firmware
 * types which support direct readout of the amplitude data.  Other firmware types read I/Q data
 * and calculate the amplitudes as part of this function, and will require use of blocking mode
 * (CH_IO_MODE_BLOCK).  If non-blocking mode is specified when using sensor f/w that does not
 * support it, this function will return an error.
 *
 * Non-blocking reads are managed together for a group of sensors.  To perform a non-blocking read:
 *
 *  -# Register a callback function using \a ch_io_complete_callback_set().
 *  -# Define and initialize a handler for the DMA interrupts generated.
 *  -# Synchronize with all sensors whose amplitude data should be read by waiting for all to
 * indicate data ready.
 *  -# Set up a non-blocking read on each sensor, using \a ch_get_amplitude_data() with \a mode =
 *     \a CH_IO_MODE_NONBLOCK (1).
 *  -# Start the non-blocking reads on all sensors in the group, using \a ch_io_start_nb().
 *  -# Your callback function (set in step #1 above) will be called as each individual sensor's
 *     read completes.  Your callback function should initiate any further processing of the I/Q
 *     data, possibly by setting a flag that will be checked from within the application's main
 *     execution loop.  The callback function will likely be called at interrupt level, so the
 *     amount of processing within it should be kept to a minimum.
 *
 * For a CH101 sensor, up to 150 samples are taken during each measurement cycle; for a CH201
 * sensor, up to 450 samples are taken.  So, a complete CH101 amplitude trace will contain up to 300
 * bytes of data (150 samples x 2 bytes per sample), and a CH201 amplitude trace may contain up to
 * 900 bytes.  The buffer specified by \a buf_ptr must be large enough to hold this amount of data.
 *
 * When the amplitude data is read from the sensor, the additional time required to transfer the
 * amplitude data over the I2C bus must be taken into account when planning how often the sensor can
 * be read (sample interval).
 *
 * @note It is important that any data I/O operations to or from the sensor, including reading
 * the amplitude data, complete before a new measurement cycle is triggered, or the new measurement
 * may be affected.
 *
 * @note This function only obtains the data from the most recently completed measurement cycle.
 * It does not actually trigger a measurement.
 */
uint8_t ch_get_amplitude_data(ch_dev_t *dev_ptr, uint16_t *buf_ptr, uint16_t start_sample,
			      uint16_t num_samples, ch_io_mode_t mode);

/**
 * @brief Convert sample count to millimeters for a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param num_samples sample count to be converted
 *
 * @return Number of millimeters
 *
 * This function converts the sample count specified in \a num_samples and converts it to the
 * corresponding physical distance in millimeters.  The conversion uses values set during device
 * initialization and calibration that describe the internal timing of the sensor.
 *
 * This function may be helpful when working with both physical distances (as reported by the
 * \a ch_get_range() function) and sample-oriented values, such as data obtained from
 * \a ch_get_iq_data() or parameters for static target rejection (see \a ch_set_static_range()).
 */
uint16_t ch_samples_to_mm(ch_dev_t *dev_ptr, uint16_t num_samples);

/**
 * @brief Convert millimeters to sample count for a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param num_mm number of millimeters to be converted
 *
 * @return Number of samples
 *
 * This function converts the distance in millimeters specified in \a num_mm and converts it
 * to the corresponding number of sensor samples.  The conversion uses values set during device
 * initialization and calibration that describe the internal timing of the sensor, along with
 * the current maximum range setting for the device.
 *
 * This function may be helpful when working with both physical distances (as reported by the
 * \a ch_get_range() function) and sample-oriented values, such as data obtained from
 * \a ch_get_iq_data() or parameters for static target rejection (see \a ch_set_static_range()).
 */
uint16_t ch_mm_to_samples(ch_dev_t *dev_ptr, uint16_t num_mm);

/**
 * @brief Calculate amplitude from sample I/Q values.
 *
 * @param iq_sample_ptr pointer to ch_iq_data_t structure containing the I/Q data
 *
 * @return Amplitude value
 *
 * This function converts the I and Q values from a single raw sensor sample to an amplitude
 * value according to the following formula:
 * 			\f[Amp = \sqrt{(I)^2 + (Q)^2}\f]
 *
 * Amplitude values in the sensor are expressed only in internal ADC counts (least-significant bits,
 * or LSBs) and are not calibrated to any standard units.
 *
 * Also see \a ch_get_iq_data() and \a ch_get_amplitude_data().
 */
uint16_t ch_iq_to_amplitude(ch_iq_sample_t *iq_sample_ptr);

/**
 * @brief Start non-blocking I/O operation(s) for a group of sensors.
 *
 * @param grp_ptr pointer to the ch_group_t descriptor for sensor group
 *
 * @return 0 if success, 1 if error
 *
 * This function starts one or more non-blocking I/O operations on a group of sensors.
 * Generally, the I/O operations are non-blocking I/Q data read requests individually
 * generated using \a ch_get_iq_data().
 *
 * This function will return immediately after the I/O operations are started.  When the I/O
 * operations complete, the callback function registered using \a ch_io_callback_set() will be
 * called.
 *
 * See \a ch_get_iq_data() for more information.
 */
uint8_t ch_io_start_nb(ch_group_t *grp_ptr);

/**
 * @brief Register sensor interrupt callback routine for a group of sensors.
 *
 * @param grp_ptr pointer to the ch_group_t sensor group descriptor structure
 * @param callback_func_ptr pointer to callback function to be called when sensor interrupts
 *
 * This function registers the routine specified by \a callback_func_ptr to be called whenever the
 * sensor interrupts.  Generally, such an interrupt indicates that a measurement cycle has completed
 * and the sensor has data ready to be read.  All sensors in a sensor group use the same callback
 * function, which receives the interrupting device's device number (port number) as an input
 * parameter to identify the specific interrupting device.
 *
 */
void ch_io_int_callback_set(ch_group_t *grp_ptr, ch_io_int_callback_t callback_func_ptr);

/**
 * @brief Register non-blocking I/O complete callback routine for a group of sensors.
 *
 * @param grp_ptr pointer to the ch_group_t group descriptor structure
 * @param callback_func_ptr pointer to callback function to be called when non-blocking I/O
 * operations complete
 *
 * This function registers the routine specified by \a callback_func_ptr to be called when all
 * outstanding non-blocking I/O operations complete for a group of sensors.   The non-blocking I/O
 * operations must have previously been initiated using \a ch_io_start_nb().
 */
void ch_io_complete_callback_set(ch_group_t *grp_ptr, ch_io_complete_callback_t callback_func_ptr);

/**
 * @brief Notify SonicLib that a non-blocking I/O operation has completed.
 *
 * @param grp_ptr pointer to the ch_group_t sensor group descriptor structure
 * @param i2c_bus_index identifier indicating on which I2C bus the I/O operation was completed
 *
 * This function should be called from your non-blocking I/O interrupt handler each time a
 * non-blocking I/O operation completes.  The \a i2c_bus_index parameter should indicate which I2C
 * bus is being reported.
 *
 * When all outstanding non-blocking I/O operations are complete, SonicLib will call the callback
 * function previously registered using \a ch_io_complete_callback_set().
 */
void ch_io_notify(ch_group_t *grp_ptr, uint8_t i2c_bus_index);

/**
 * @brief Get the detection threshold.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param threshold_index index to the thresholds, starting from 0
 *
 * @return amplitude threshold value
 *
 * This function obtains the detection threshold value with the specified \a threshold_index
 * from the sensor and returns the corresponding threshold amplitude.
 *
 * See also \a ch_set_threshold().
 *
 * @note This function is only available when using special sensor firmware packages from Chirp.
 *
 * @note For CH201 GPRMT firmware, which supports multiple thresholds with programmable sample
 * ranges, use \a ch_get_thresholds().
 */
uint16_t ch_get_threshold(ch_dev_t *dev_ptr, uint8_t threshold_index);

/**
 * @brief Set the detection threshold.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param threshold_index index to the thresholds, starting from 0
 * @param amplitude amplitude threshold value
 *
 * @return 0 if success, 1 if error
 *
 * This function sets the detection threshold value with the specified \a threshold_index value
 * for the sensor.  The detection treshold is the minimum amplitude required for a target detection
 * to be reported.  The \a amplitude value is the new required amplitude, expressed in the same
 * internal units used in \a ch_get_amplitude() and other functions.
 *
 * See also \a ch_get_threshold().
 *
 * @note This function is only available when using select sensor firmware packages from Chirp.
 *
 * @note For CH201 GPRMT firmware, which supports multiple thresholds with programmable sample
 * ranges, use \a ch_set_thresholds().
 */
uint8_t ch_set_threshold(ch_dev_t *dev_ptr, uint8_t threshold_index, uint16_t amplitude);

/**
 * @brief Get detection thresholds (CH201 only).
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param thresh_ptr pointer to ch_thresholds_t structure to receive threshold data
 *
 * @return 0 if success, 1 if error
 *
 * This function obtains the current detection threshold values from the sensor and
 * returns them in a ch_thresholds_t structure specified by \a thresh_ptr.  The
 * ch_thresholds_t structure holds an array of ch_thresh_t structures, each of which
 * contains a starting sample number and amplitude threshold value.
 *
 * See also \a ch_set_thresholds().
 *
 * @note This function supports CH201 GPRMT sensor firmware, with multiple detection thresholds
 * and programmable sample ranges.  If using other firmware with individual detection tresholds,
 * use \a ch_get_threshold().
 */
uint8_t ch_get_thresholds(ch_dev_t *dev_ptr, ch_thresholds_t *thresh_ptr);

/**
 * @brief Set detection thresholds (CH201 only).
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param thresh_ptr pointer to ch_thresholds_t structure containing threshold data
 *
 * @return 0 if success, 1 if error
 *
 * This function obtains the current detection threshold values from the sensor and
 * returns them in a ch_thresholds_t structure specified by \a thresh_ptr.  The
 * ch_thresholds_t structure holds an array of ch_thresh_t structures, each of which
 * contains a starting sample number and amplitude threshold value.
 *
 * To use this function, first initialize the ch_thresh_t sample/level pair of values for
 * each threshold.  The CH201 GPR firmware supports six (6) thresholds.  Each threshold has a
 * maximum sample length of 255.
 *
 * See also \a ch_get_thresholds().
 *
 * @note This function supports CH201 GPRMT sensor firmware, with multiple detection thresholds
 * and programmable sample ranges.  If using other firmware with individual detection tresholds,
 * use \a ch_set_threshold().
 */
uint8_t ch_set_thresholds(ch_dev_t *dev_ptr, ch_thresholds_t *thresh_ptr);

/**
 * @brief Enable or disable target detection interrupt mode.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param enable 1 to enable target interrupt mode, 0 to disable
 *
 * @return 0 if success, 1 if error
 *
 * This function enables or disables the target detection interrupt mode.
 *
 * In normal operation (if target detection interrupt mode is not enabled), the sensor will
 * assert the INT line at the end of a measurement whether or not a target was detected.
 *
 * When target detection interrupt mode is enabled, the sensor will only assert the INT
 * line at the end of a measurement if a target object was detected.  If no target is detected,
 * the sensor will not assert the INT line.  There is no indication from the sensor
 * that the measurement has completed.
 *
 * To use this function, set \a enable to 1 to enable target detection interrupt mode.  Set
 * \a enable to 0 to disable the target detection interrupt mode.
 *
 * @note Target detection interrupt mode is only available in select Chirp sensor firmware versions.
 */
uint8_t ch_set_target_interrupt(ch_dev_t *dev_ptr, uint8_t enable);

/**
 * @brief Get the target detection interrupt mode setting.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return 1 if target detection interrupt mode is enabled, 0 if disabled
 *
 * This function returns the current setting for target detection interrupt mode.
 *
 * @note Target detection interrupt mode is only available in select Chirp sensor firmware versions.
 */
uint8_t ch_get_target_interrupt(ch_dev_t *dev_ptr);

/**
 * @brief Set the static coefficient for sensor IIR filter.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param static_coeff static coefficient for the IIR filter
 *
 * @return 0 if success, 1 if error
 *
 * This function sets the static coefficient for the sensor's internal IIR filter.
 *
 * If using shorter ultrasound transmit pulses, you may decrease the static coefficient.
 * If using longer ultrasound transmit pulses, you may increase the static coefficient.
 *
 * See also \a ch_get_static_coeff().
 *
 * @note This function is only available in select Chirp sensor firmware versions.
 */
uint8_t ch_set_static_coeff(ch_dev_t *dev_ptr, uint8_t static_coeff);

/**
 * @brief Get the static coefficient for IIR filter.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return IIR static coefficient
 *
 * This function gets the static coefficient value for the sensor's internal IIR filter.
 *
 * See also \a ch_get_static_coeff().
 *
 * @note This function is only available in select Chirp sensor firmware versions.
 */
uint8_t ch_get_static_coeff(ch_dev_t *dev_ptr);

/**
 * @brief Set the receive holdoff sample count.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param num_samples number of samples to be ignored at the beginning of each measurement
 *
 * @return 0 if success, 1 if error
 *
 * This function sets the receive (rx) holdoff sample count.  \a num_samples specifies a number of
 * samples at the beginning of a measurement that will be ignored for the purpose of
 * detecting a target.  (These samples correspond to the closest distances from the sensor.)
 *
 * To convert a physical distance into a sample count value to use here, use \a ch_mm_to_samples().
 *
 * See also \a ch_get_rx_holdoff().
 */
uint8_t ch_set_rx_holdoff(ch_dev_t *dev_ptr, uint16_t num_samples);

/**
 * @brief Get the receive holdoff sample count.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return number of samples that are ignored at the beginning of each measurement
 *
 * This function gets the receive (rx) holdoff sample count.  The rx holdoff count
 * is the number of samples at the beginning of a measurement that will be ignored for
 * the purpose of detecting a target, as previously set by \a ch_set_rx_holdoff().
 *
 * To convert the returned sample count to a physical distance, use \a ch_samples_to_mm().
 *
 * See also \a ch_set_rx_holdoff().
 */
uint16_t ch_get_rx_holdoff(ch_dev_t *dev_ptr);

/**
 * @brief Set the receive low-gain sample count.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param num_samples number of samples that have low gain at beginning of measurement
 *
 * @return 0 if success, 1 if error
 *
 * This function sets the receive (rx) low-gain range in the sensor.  The low-gain range consists
 * of samples at the beginning of a measurement that will have a lower gain applied during the
 * sensor receive operation.  (These samples correspond to the closest distances from the sensor.)
 *
 * The low-gain range must be less than the current maximum range setting.  If \a num_samples is
 * greater than or equal to the maximum range setting, it will automatically be reduced.
 *
 * See also \a ch_get_rx_low_gain(), \a ch_get_num_samples().
 *
 * @note This function is only available in CH201 sensor firmware.
 */
uint8_t ch_set_rx_low_gain(ch_dev_t *dev_ptr, uint16_t num_samples);

/**
 * @brief Get the receive low-gain sample count.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return number of samples that have low gain at beginning of measurement
 *
 * This function gets the receive (rx) low-gain range from the sensor.  The low-gain range is
 * a count of samples at the beginning of a measurement that will have lower gain applied
 * during the ultrasound receive operation.
 *
 * The returned low-gain range is either the default value for the sensor firmware, or
 * a value previously set by \a ch_set_rx_low_gain().
 *
 * To convert the returned sample count to a physical distance, use \a ch_samples_to_mm().
 *
 * See also \a ch_set_rx_low_gain().
 *
 * @note This function is only available in CH201 sensor firmware.
 */
uint16_t ch_get_rx_low_gain(ch_dev_t *dev_ptr);

/**
 * @brief Set the ultrasound transmit pulse length.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param num_cycles transmit pulse length (number of cycles per transmission)
 *
 * @return 0 if success, 1 if error
 *
 * This function sets the length (duration) of the ultrasonic pulse sent by the
 * sensor when it transmits. \a num_cycles specifies the length of the pulse.  The units
 * are the number of active PMUT cycles per transmission.
 *
 * See also \a ch_get_tx_length().
 *
 * @note This feature is only available in select Chirp sensor firmware versions.
 */
uint8_t ch_set_tx_length(ch_dev_t *dev_ptr, uint8_t num_cycles);

/**
 * @brief Get the ultrasound transmit pulse length.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return transmit pulse length (number of cycles per transmission)
 *
 * This function gets the length of the ultrasonic pulse sent by the sensor when
 * it transmit.  The transmit length is the number of active PMUT cycles per transmission.
 *
 * See also \a ch_set_tx_length().
 *
 * @note This feature is only available in select Chirp sensor firmware versions.
 */
uint8_t ch_get_tx_length(ch_dev_t *dev_ptr);

/**
 * @brief Get the detected length of the received ultrasound pulse.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * @return pulse length
 *
 * This function gets the detected length of the received ultrasonic pulse.
 *
 * @note This function is only available in select Chirp sensor firmware versions.
 */
uint8_t ch_get_rx_pulse_length(ch_dev_t *dev_ptr);

/**
 * @brief Configure SonicSync timing plan.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param time_plan time plan identifier
 *
 * This routine sets the timing plan for a SonicSync master or slave node.  The possible values are:
 *  SONICSYNC_TIME_PLAN_1		(0),
 *  SONICSYNC_TIME_PLAN_2		(1),
 *  SONICSYNC_TIME_PLAN_3		(2), and
 *  SONICSYNC_TIME_PLAN_NONE	(255)
 *
 * For a master device, this value specifies the timing plan that will be used for the master and
 * slave pair.  (The default value, SONICSYNC_TIME_PLAN_1, is used if this routine is not called.)
 * SONICSYNC_TIME_PLAN_NONE should not be specified as the master node time plan.
 *
 * By default, a slave device initially uses SONICSYNC_TIME_PLAN_NONE, which causes the slave to
 * cycle through the various time plans attempting to pair with the master.  (This is the behavior
 * if this routine is not called.) When the slave successfully discovers and pairs with the master,
 * the successful time plan is recorded and then used for all subsequent exchanges, unless it is
 * reset to SONICSYNC_TIME_PLAN_NONE by a call to this routine.  Once the slave has successfully
 * paired with a master device, the time plan value may be determined by calling the \a
 * ch_get_time_plan() routine.
 *
 * However, a slave device may be forced to only use a single time plan by calling this routine
 * before the master is discovered.  This may be desirable to speed up the master discovery or to
 * avoid pairing with nearby sensor pairs that use a different time plan.
 *
 * See also \a ch_get_time_plan().
 *
 * @return 0 if successful.
 *
 * @note This function is only available when using Chirp SonicSync sensor firmware.
 */
uint8_t ch_set_time_plan(ch_dev_t *dev_ptr, ch_time_plan_t time_plan);

/**
 * @brief Get SonicSync timing plan.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * This routine returns the timing plan currently in use for a SonicSync master or slave node.  The
 * possible values are: SONICSYNC_TIME_PLAN_1		(0), SONICSYNC_TIME_PLAN_2		(1),
 *  SONICSYNC_TIME_PLAN_3		(2), and
 *  SONICSYNC_TIME_PLAN_NONE	(255)
 *
 * For a master device, this value will always be the value specified in a previous call to \a
 * ch_set_time_plan(). If no such call was made, the master device will use the default time plan,
 * SONICSYNC_TIME_PLAN_1, and that value will be returned by this routine.
 *
 * By default, a slave device initially uses SONICSYNC_TIME_PLAN_NONE, which causes the slave to
 * cycle through the various time plans attempting to pair with the master.  Until the slave has
 * successfully paired with a master node, this routine will return SONICSYNC_TIME_PLAN_NONE.
 *
 * Once the slave has successfully discovered and paired with a master, this routine will return the
 * value indicating the time plan.  Therefore, this routine may be used a polling mechanism to
 * determine when a slave has successfully paired.  Once paired, the time plan value for the slave
 * will not change (unless explicitly set using the \a ch_set_time_plan() routine).
 *
 * See also \a ch_set_time_plan().
 *
 * @return time plan value currently in use
 *
 * @note This function is only available when using Chirp SonicSync sensor firmware.
 */
ch_time_plan_t ch_get_time_plan(ch_dev_t *dev_ptr);

/**
 * @brief Enable/disable receive-only sensor pre-triggering.
 *
 * @param grp_ptr pointer to the ch_group_t group descriptor structure
 * @param enable 1 to enable pre-triggering, 0 to disable
 *
 * This function enables or disables pre-triggering of the receive-only sensor during Pitch-Catch
 * operation.  When pre-triggering is enabled, sensors in CH_MODE_TRIGGERED_RX_ONLY mode will be
 * triggered slightly before sensors in CH_MODE_TRIGGERED_TX_RX mode when \a ch_group_trigger() is
 * called. This improves the ability of a receive-only sensor to detect the transmitted pulse at
 * very short distances.
 *
 * If enabled, pre-triggering is used for all receive-only sensors in the sensor group.
 *
 * To use this function, set \a enable to 1 to enable pre-triggering, or 0 to disable
 * pre-triggering.
 *
 * See also \a ch_get_rx_pretrigger().
 *
 * @note  Enabling pre-triggering will reduce the maximum range of the receive-only sensor(s),
 * relative to the setting specified in \a ch_set_max_range(), by about 200mm.  You may want to
 * increase the maximum range setting accordingly.
 */
void ch_set_rx_pretrigger(ch_group_t *grp_ptr, uint8_t enable);

/**
 * @brief Get receive-only sensor pre-triggering setting.
 *
 * @param grp_ptr pointer to the ch_group_t group descriptor structure
 *
 * This function returns the current state (enabled or disabled) of pre-triggering receive-only
 * sensors.
 *
 * See also \a ch_set_rx_pretrigger().
 *
 * @return 1 if receive pre-triggering is enabled, 0 if disabled
 */
uint8_t ch_get_rx_pretrigger(ch_group_t *grp_ptr);

/**
 * @brief Check sensor firmware program
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * This function confirms that the contents of the sensor's program memory match
 * the firmware that was loaded into it, as specified during the \a ch_init()
 * call.  The memory contents are read back from the sensor and are compared with
 * the original byte values used to program the device.
 *
 * @return 0 if firmware matches the original program, or 1 if mismatch (error)
 */
uint8_t ch_check_program(ch_dev_t *dev_ptr);

/**
 * @brief Set modulated TX data.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param tx_data The data to be transmitted over ultrasound
 *
 * This function sets the modulated TX data
 * The tx_data must be between 0-31 decimal
 *
 * @return 0 if successful.
 *
 * @note This function is only available when using Chirp GPPC sensor firmware.
 */
uint8_t ch_set_modulated_tx_data(ch_dev_t *dev_ptr, uint8_t tx_data);

/**
 * @brief Get the demodulated data from the received ultrasound pulse.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param rx_pulse_length the detected length of the received ultrasound pulse
 * @param *data_ptr				pointer to the data
 *
 * This function gets the raw data from the received ultrasound pulse and demodulates it
 *
 * @return 0 if successful.
 *
 * @note This function is only available when using Chirp GPPC sensor firmware.
 */
uint8_t ch_get_demodulated_rx_data(ch_dev_t *dev_ptr, uint8_t rx_pulse_length, uint8_t *data_ptr);

/**
 * @brief Set the calibration result.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param cal_ptr pointer to ch_cal_result_t structure to receive calibration data
 *
 * @return 0 if success, 1 if error
 *
 * WARNING: This function should not be used to set the calibration result to a fixed value,
 * even one individually calculated for each sensor, as this could change over the lifetime
 * of the sensor; rather, this function could be used to update the calibration result if the
 * calibration result calculated by CHx01 at startup (i.e. returned by ch_get_cal_result())
 * is sufficiently different than expected or sensor performance is not good.
 *
 * This function sets the calibration result with a ch_cal_result_t structure specified by \a
 * cal_ptr for the sensor. The ch_cal_result_t structure contains DCO period and reverse drive
 * cycles.
 *
 * To use this function, first initialize the ch_cal_result_t DCO period/reverse drive cycles pair
 * of values
 *
 * See also \a ch_get_cal_result().
 */
uint8_t ch_set_cal_result(ch_dev_t *dev_ptr, ch_cal_result_t *cal_ptr);

/**
 * @brief Get the calibration result.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure
 * @param cal_ptr pointer to ch_cal_result_t structure to receive calibration data
 *
 * @return 0 if success, 1 if error
 *
 * This function obtains the current calibration result from the sensor and
 * returns them in a ch_cal_result_t structure specified by \a cal_ptr.  The
 * ch_cal_result_t structure contains DCO period and reverse drive cycles.
 *
 * See also \a ch_set_cal_result().
 */
uint8_t ch_get_cal_result(ch_dev_t *dev_ptr, ch_cal_result_t *cal_ptr);

#endif /* __SONICLIB_H_ */
