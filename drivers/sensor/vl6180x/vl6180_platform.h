/* vl6180_platform.h - Zephyr customization of ST vl6180x library.
 * (library is located in ext/hal/st/lib/sensor/vl6180x/)
 */

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_VL6180_VL6180_PLATFORM_H_
#define ZEPHYR_DRIVERS_SENSOR_VL6180_VL6180_PLATFORM_H_

/**
 * @def VL6180_HAVE_MULTI_READ
 * @brief Enable I2C multi read support
 *
 * When set to 1, multi read operations are done (when necessary) by the API functions (mainly
 * WAF) to access a bunch of registers instead of individual ones (for speed increase). This
 * requires the @a VL6180_RdMulti() function to be implemented.
 */
#define VL6180_HAVE_MULTI_READ          1


/**
 * @def VL6180_CACHED_REG
 * @brief Enable Cached Register mode
 *
 * In addition to the multi read mode (#VL6180_HAVE_MULTI_READ set to 1), this mode implements
 * an advanced register access mode to speed-up ranging measurements by reading all results
 * registers in one shot (using multi read operation). All post-processing operations (like WAF)
 * are done by accessing the cached registers instead of doing individual register access.
 * @warning It is mandatory to set #VL6180_HAVE_MULTI_READ to 1 to benefit from this advanced mode
 */
#define VL6180_CACHED_REG          0

#include "vl6180_def.h"
#include "vl6180_platform_log.h"

/**
 * @struct VL6180_Dev_t
 * @brief  Generic PAL device type that does link between API and platform
 * abstraction layer
 *
 */
typedef struct {
	struct VL6180DevData_t Data;          /*!< embed ST VL6180 Dev  data as "Data"*/
	/*!< user specific field */
	uint8_t   I2cDevAddr;      /* i2c device address user specific field */
	uint8_t   comms_type;      /* VL53L0X_COMMS_I2C or VL53L0X_COMMS_SPI */
	uint16_t  comms_speed_khz; /* Comms speed [kHz] */
	const struct device *i2c;
} VL6180_Dev_t;

/**
 * @brief Declare the device Handle as a pointer of the structure VL53L0X_Dev_t
 *
 */
typedef VL6180_Dev_t *VL6180Dev_t;

/**
 * @file vl6180_platform.h
 *
 * @brief All end user OS/platform/application porting
 */

/** @defgroup api_platform Platform
 *  @brief Platform-dependent code
 */


/** @ingroup api_platform
 * @{*/

 /**
  * @brief For user convenience to place or give any required data attribute
  * to the built-in single device instance \n
  * Useful only when Configuration @a #VL6180_SINGLE_DEVICE_DRIVER is active
  *
  * @ingroup api_platform
  */
#define VL6180_DEV_DATA_ATTR

/**
 * @def ROMABLE_DATA
 * @brief API Read only data that can be place in rom/flash are declared with that extra keyword
 *
 * For user convenience, use compiler specific attribute or keyword to place all read-only data in
 * required data area.
 * For example using gcc section  :
 *  @code
 *  #define ROMABLE_DATA  __attribute__ ((section ("user_rom")))
 *  // you may need to edit your link script file to place user_rom section in flash/rom memory
 *  @endcode
 *
 * @ingroup api_platform
 */
#define ROMABLE_DATA
/*  #define ROMABLE_DATA  __attribute__ ((section ("user_rom"))) */

/**
 * @def VL6180_RANGE_STATUS_ERRSTRING
 * @brief  Activate error code translation into string.
 * TODO: michel to apdate
 * @ingroup api_platform
 */
#define VL6180_RANGE_STATUS_ERRSTRING 1

/**
 * @def VL6180_SINGLE_DEVICE_DRIVER
 * @brief Enable lightweight single vl6180x device driver.
 *
 * Value __1__ =>  Single device capable.
 * Configure optimized API for single device driver with static data and minimal use of ref
 * pointer.
 * Limited to single device driver or application in non multi thread/core environment.
 *
 * Value __0__ =>  Multiple device capable. User must review "device" structure and type in
 * vl6180_platform.h files.
 * @ingroup api_platform
 */
#define VL6180_SINGLE_DEVICE_DRIVER 0

/**
 * @def VL6180_SAFE_POLLING_ENTER
 * @brief Ensure safe polling method when set
 *
 * Polling for a condition can be hazardous and result in infinite looping if any previous
 * interrupt status condition is not cleared.
 * Setting these flags enforce error clearing on start of polling method to avoid it.
 * the drawback are :
 * @li extra use-less i2c bus usage and traffic
 * @li potentially slower measure rate.
 * If application ensures interrupt get cleared on mode or interrupt configuration change
 * then keep option disabled.
 * To be safe set these option to 1
 * @ingroup api_platform
 */
#define VL6180_SAFE_POLLING_ENTER  0


/**
 * @brief Enable start/end logging facilities. It can generates traces log to help problem tracking
 * analysis and solving
 *
 * Requires porting  @a #LOG_FUNCTION_START, @a #LOG_FUNCTION_END, @a #LOG_FUNCTION_END_FMT
 * @ingroup api_platform
 */
#define VL6180_LOG_ENABLE  0

/*! [device_type_multi] */
/**
 * @def VL6180DevDataGet
 * @brief Get ST private structure @a VL6180DevData_t data access (needed only in multi devices
 * configuration)
 *
 * It may be used and a real data "ref" not just as "get" for sub-structure item
 * like VL6180DevDataGet(FilterData.field)[i] or VL6180DevDataGet(FilterData.MeasurementIndex)++
 * @param dev   The device
 * @param field ST structure filed name
 * @ingroup api_platform
 */
#define VL6180DevDataGet(dev, field) (dev->Data.field)

/**
 * @def VL6180DevDataSet(dev, field, data)
 * @brief  Set ST private structure @a VL6180DevData_t data field (needed only in multi devices
 * configuration)
 * @param dev    The device
 * @param field  ST structure field name
 * @param data   Data to be set
 * @ingroup api_platform
 */
#define VL6180DevDataSet(dev, field, data) (dev->Data.field) = (data)
/*! [device_type_multi] */

/**
 * @brief execute delay in all polling api calls : @a VL6180_RangePollMeasurement()
 *
 * A typical multi-thread or RTOs implementation is to sleep the task for some 5ms (with 100Hz max
 * rate faster polling is not needed).
 * if nothing specific is needed, you can define it as an empty/void macro
 * @code
 * #define VL6180_PollDelay(...) (void)0
 * @endcode
 * @param dev The device
 * @ingroup api_platform
 */
void VL6180_PollDelay(VL6180Dev_t dev); /* usualy best implemanted a a real fucntion */

#endif  /* VL6180_PLATFORM */

