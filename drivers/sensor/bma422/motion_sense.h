/* Copyright 2014 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Header for motion_sense.c */

#ifndef __CROS_EC_MOTION_SENSE_H
#define __CROS_EC_MOTION_SENSE_H

#include "atomic.h"
#include "body_detection.h"
#include "chipset.h"
#include "common.h"
#include "ec_commands.h"
#include "i2c.h"
#include "math_util.h"
#include "queue.h"
#include "task.h"
#include "timer.h"
#include "util.h"

enum sensor_state {
	/* Sensor state is unknown, out of reset. Maybe powered down */
	SENSOR_NOT_INITIALIZED = 0,
	/*
	 * Sensor is power on, has been initialized on the HOOK task.
	 * The MOTION_SENSE task is not aware of it yet.
	 */
	SENSOR_INITIALIZED = 1,
	/*
	 * We try to initialize the sensor, but fails. Will stay
	 * in this state until power-cycled.
	 */
	SENSOR_INIT_ERROR = 2,
	/*
	 * The sensor is ready for operation, an Output Data Rate
	 * has been set up, (even 0Hz).
	 */
	SENSOR_READY = 3
};

enum sensor_config {
	SENSOR_CONFIG_AP, /* Configuration requested/for the AP */
	SENSOR_CONFIG_EC_S0, /* Configuration from the EC while device in S0 */
	SENSOR_CONFIG_EC_S3, /* from the EC when device sleep */
	SENSOR_CONFIG_EC_S5, /* from the EC when device powered off */
	SENSOR_CONFIG_MAX,
};

#define SENSOR_ACTIVE_S5 (CHIPSET_STATE_SOFT_OFF | CHIPSET_STATE_HARD_OFF)
#define SENSOR_ACTIVE_S3 CHIPSET_STATE_ANY_SUSPEND
#define SENSOR_ACTIVE_S0 CHIPSET_STATE_ON
#define SENSOR_ACTIVE_S0_S3 (SENSOR_ACTIVE_S3 | SENSOR_ACTIVE_S0)
#define SENSOR_ACTIVE_S0_S3_S5 (SENSOR_ACTIVE_S0_S3 | SENSOR_ACTIVE_S5)

/*
 * Events layout:
 * 0                       8              10
 * +-----------------------+---------------+----------------------------
 * | hardware interrupts   | internal ints | activity interrupts
 * +-----------------------+---------------+----------------------------
 */

/* First 8 events for sensor interrupt lines */
#define TASK_EVENT_MOTION_INTERRUPT_NUM 8
#define TASK_EVENT_MOTION_INTERRUPT_MASK \
	((1 << TASK_EVENT_MOTION_INTERRUPT_NUM) - 1)
#define TASK_EVENT_MOTION_SENSOR_INTERRUPT(_sensor_id)        \
	BUILD_CHECK_INLINE(TASK_EVENT_CUSTOM_BIT(_sensor_id), \
			   _sensor_id < TASK_EVENT_MOTION_INTERRUPT_NUM)

/* Internal events to motion sense task.*/
#define TASK_EVENT_MOTION_FIRST_INTERNAL_EVENT TASK_EVENT_MOTION_INTERRUPT_NUM
#define TASK_EVENT_MOTION_INTERNAL_EVENT_NUM 2
#define TASK_EVENT_MOTION_FLUSH_PENDING \
	TASK_EVENT_CUSTOM_BIT(TASK_EVENT_MOTION_FIRST_INTERNAL_EVENT)
#define TASK_EVENT_MOTION_ODR_CHANGE \
	TASK_EVENT_CUSTOM_BIT(TASK_EVENT_MOTION_FIRST_INTERNAL_EVENT + 1)

/* Activity events */
#define TASK_EVENT_MOTION_FIRST_SW_EVENT \
	(TASK_EVENT_MOTION_INTERRUPT_NUM + TASK_EVENT_MOTION_INTERNAL_EVENT_NUM)
#define TASK_EVENT_MOTION_ACTIVITY_INTERRUPT(_activity_id)        \
	(TASK_EVENT_CUSTOM_BIT(TASK_EVENT_MOTION_FIRST_SW_EVENT + \
			       (_activity_id)))

#define ROUND_UP_FLAG ((uint32_t)BIT(31))
#define BASE_ODR(_odr) ((_odr) & ~ROUND_UP_FLAG)
#define BASE_RANGE(_range) ((_range) & ~ROUND_UP_FLAG)

/*
 * I2C/SPI Address flags encoding for motion sensors
 * - The generic defines, I2C_ADDR_MASK and I2C_IS_BIG_ENDIAN_MASK
 *   are defined in i2c.h.
 * - Motion sensors support some sensors on the SPI bus, so this
 *   overloads the I2C Address to use a single bit to indicate
 *   it is a SPI address instead of an I2C.  Since SPI does not
 *   use peripheral addressing, it is up to the driver to use this
 *   field as it sees fit
 */
#define ACCEL_MK_I2C_ADDR_FLAGS(addr) (addr)
#define ACCEL_MK_SPI_ADDR_FLAGS(addr) ((addr) | I2C_FLAG_ADDR_IS_SPI)

#define ACCEL_GET_I2C_ADDR(addr_flags) (I2C_STRIP_FLAGS(addr_flags))
#define ACCEL_GET_SPI_ADDR(addr_flags) ((addr_flags)&I2C_ADDR_MASK)

#define ACCEL_ADDR_IS_SPI(addr_flags) ((addr_flags)&I2C_FLAG_ADDR_IS_SPI)

/*
 * Define the frequency to use in max_frequency based on the maximal frequency
 * the sensor support and what the EC can provide.
 * Return a frequency the sensor supports.
 * Trigger a compilation error when the EC way to slow for the sensor.
 */
#define MOTION_MAX_SENSOR_FREQUENCY(_max, _step)                         \
	GENERIC_MIN(                                                     \
		(_max) / (CONFIG_EC_MAX_SENSOR_FREQ_MILLIHZ >= (_step)), \
		(_step) << __fls(CONFIG_EC_MAX_SENSOR_FREQ_MILLIHZ / (_step)))

struct motion_data_t {
	/*
	 * data rate the sensor will measure, in mHz: 0 suspended.
	 * MSB is used to know if we are rounding up.
	 */
	unsigned int odr;

	/*
	 * delay between collection by EC, in us.
	 * For non FIFO sensor, should be near 1e9/odr to
	 * collect events.
	 * For sensor with FIFO, can be much longer.
	 * 0: no collection.
	 */
	unsigned int ec_rate;
};

/*
 * When set, spoof mode will allow the EC to report arbitrary values for any of
 * the components.
 */
#define MOTIONSENSE_FLAG_IN_SPOOF_MODE BIT(1)

struct online_calib_data {
	/**
	 * Type specific data.
	 * - For Accelerometers use struct accel_cal.
	 * - For Gyroscopes (not yet implemented).
	 * - For Magnetormeters (not yet implemented).
	 */
	void *type_specific_data;

	/**
	 * Cached calibration values from the latest successful calibration
	 * pass.
	 */
	int16_t cache[3];

	/** The latest temperature reading in K, negative if not set. */
	int last_temperature;

	/** Timestamp for the latest temperature reading. */
	uint32_t last_temperature_timestamp;
};

struct motion_sensor_t {
	/* RO fields */
	uint32_t active_mask;
	char *name;
	enum motionsensor_chip chip;
	enum motionsensor_type type;
	enum motionsensor_location location;
	const struct accelgyro_drv *drv;
	/* One mutex per physical chip. */
	mutex_t *mutex;
	void *drv_data;
	/* Data used for online calibraiton, must match the sensor type. */
	struct online_calib_data
		online_calib_data[__cfg_select(CONFIG_ONLINE_CALIB, 1, 0)];

	/* i2c port */
	uint8_t port;
	/* i2c address or SPI port */
	uint16_t i2c_spi_addr_flags;

	/*
	 * Various flags, see MOTIONSENSE_FLAG_*
	 */
	uint32_t flags;

	const mat33_fp_t *rot_standard_ref;

	/*
	 * default_range: set by default by the EC.
	 * The host can change it, but rarely does.
	 */
	int default_range;

	/* Range currently used by the sensor. */
	int current_range;

	/*
	 * There are 4 configuration parameters to deal with different
	 * configuration
	 *
	 * Power   |         S0        |            S3     |      S5
	 * --------+-------------------+-------------------+-----------------
	 * From AP | <------- SENSOR_CONFIG_AP ----------> |
	 *         | Use for normal    | While sleeping    | Always disabled
	 *         | operation: game,  | For Activity      |
	 *         | screen rotation   | Recognition       |
	 * --------+-------------------+-------------------+------------------
	 * From EC |SENSOR_CONFIG_EC_S0|SENSOR_CONFIG_EC_S3|SENSOR_CONFIG_EC_S5
	 *         | Background        | Gesture  Recognition (Double tap, ...)
	 *         | Activity: compass,|
	 *         | ambient light)|
	 */
	struct motion_data_t config[SENSOR_CONFIG_MAX];

#ifdef CONFIG_BODY_DETECTION
	/* Body detection sensor configuration. */
	const struct body_detect_params *bd_params;
#endif

	/* state parameters */
	enum sensor_state state;
	intv3_t raw_xyz;
	intv3_t xyz;
	intv3_t spoof_xyz;

	/* How many flush events are pending */
	atomic_t flush_pending;

	/*
	 * Allow EC to request an higher frequency for the sensors than the AP.
	 * We will downsample according to oversampling_ratio, or ignore the
	 * samples altogether if oversampling_ratio is 0.
	 */
	uint16_t oversampling;
	uint16_t oversampling_ratio;

	/*
	 * For sensors in forced mode the ideal time to collect the next
	 * measurement.
	 *
	 * This is unused with sensors that interrupt the ec like hw fifo chips.
	 */
	uint32_t next_collection;

	/*
	 * The time in us between collection measurements
	 */
	uint32_t collection_rate;

	/* Minimum supported sampling frequency in miliHertz for this sensor */
	uint32_t min_frequency;

	/* Maximum supported sampling frequency in miliHertz for this sensor */
	uint32_t max_frequency;
};

/*
 * Mutex to protect sensor values between host command task and
 * motion sense task:
 * When we process CMD_DUMP, we want to be sure the motion sense
 * task is not updating the sensor values at the same time.
 */
extern mutex_t g_sensor_mutex;

/* Defined at board level. */
extern struct motion_sensor_t motion_sensors[];

#ifdef CONFIG_DYNAMIC_MOTION_SENSOR_COUNT
extern unsigned int motion_sensor_count;
#else
extern const unsigned int motion_sensor_count;
#endif
/* Needed if reading ALS via LPC is needed */
extern const struct motion_sensor_t *motion_als_sensors[];

/* optionally defined at board level */
extern unsigned int motion_min_interval;

/*
 * Priority of the motion sense resume/suspend hooks, to be sure associated
 * hooks are scheduled properly.
 */
#define MOTION_SENSE_HOOK_PRIO HOOK_PRIO_DEFAULT

/**
 * Take actions at end of sensor initialization:
 * - print init done status to console,
 * - set default range.
 *
 * @param sensor sensor which was just initialized
 */
int sensor_init_done(struct motion_sensor_t *sensor);

/**
 * Board specific function that is called when a double_tap event is detected.
 *
 */
void sensor_board_proc_double_tap(void);

/**
 * Board specific function to double check lid angle calculation is possible.
 *
 */
int sensor_board_is_lid_angle_available(void);

/**
 * Commit the data in a sensor's raw_xyz vector. This operation might have
 * different meanings depending on the CONFIG_ACCEL_FIFO flag.
 *
 * @param s Pointer to the sensor.
 */
void motion_sense_push_raw_xyz(struct motion_sensor_t *s);

/*
 * There are 4 variables that represent the number of sensors:
 * SENSOR_COUNT: The number of available motion sensors in board.
 * MAX_MOTION_SENSORS: Max number of sensors. This equals to SENSOR_COUNT
 *                     (+ 1 when activity sensor is available).
 * motion_sensor_count: The number of motion sensors using currently.
 * ALL_MOTION_SENSORS: motion_sensor_count (+ 1 when activity sensor is
 *                     available).
 */
#if defined(CONFIG_GESTURE_HOST_DETECTION) || defined(CONFIG_ORIENTATION_SENSOR)
/* Add an extra sensor. We may need to add more */
#define MOTION_SENSE_ACTIVITY_SENSOR_ID (motion_sensor_count)
#define ALL_MOTION_SENSORS (MOTION_SENSE_ACTIVITY_SENSOR_ID + 1)
#define MAX_MOTION_SENSORS (SENSOR_COUNT + 1)
#else
#define MOTION_SENSE_ACTIVITY_SENSOR_ID (-1)
#define ALL_MOTION_SENSORS (motion_sensor_count)
#define MAX_MOTION_SENSORS (SENSOR_COUNT)
#endif

#ifdef CONFIG_ALS_LIGHTBAR_DIMMING
#ifdef TEST_BUILD
#define MOTION_SENSE_LUX 0
#else
#define MOTION_SENSE_LUX motion_sensors[CONFIG_ALS_LIGHTBAR_DIMMING].raw_xyz[0]
#endif
#endif

/*
 * helper functions for clamping raw i32 values,
 * each sensor driver should take care of overflow condition.
 */
static inline uint16_t ec_motion_sensor_clamp_u16(const int32_t value)
{
	return (uint16_t)MIN(MAX(value, 0), (int32_t)UINT16_MAX);
}
static inline void ec_motion_sensor_clamp_u16s(uint16_t *arr, const int32_t *v)
{
	arr[0] = ec_motion_sensor_clamp_u16(v[0]);
	arr[1] = ec_motion_sensor_clamp_u16(v[1]);
	arr[2] = ec_motion_sensor_clamp_u16(v[2]);
}

static inline int16_t ec_motion_sensor_clamp_i16(const int32_t value)
{
	return MIN(MAX(value, (int32_t)INT16_MIN), (int32_t)INT16_MAX);
}
static inline void ec_motion_sensor_clamp_i16s(int16_t *arr, const int32_t *v)
{
	arr[0] = ec_motion_sensor_clamp_i16(v[0]);
	arr[1] = ec_motion_sensor_clamp_i16(v[1]);
	arr[2] = ec_motion_sensor_clamp_i16(v[2]);
}

/* direct assignment */
static inline void
ec_motion_sensor_fill_values(struct ec_response_motion_sensor_data *dst,
			     const int32_t *v)
{
	dst->data[0] = v[0];
	dst->data[1] = v[1];
	dst->data[2] = v[2];
}

#ifdef CONFIG_ZTEST
enum sensor_config motion_sense_get_ec_config(void);
#endif

#endif /* __CROS_EC_MOTION_SENSE_H */