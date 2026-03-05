/* vl53l5cx_platform.h - Zephyr customization of ST vl53l5cx library. */

/*
 * Copyright (c) 2021 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_VL53L1X_VL53L5CX_PLATFORM_H_
#define ZEPHYR_DRIVERS_SENSOR_VL53L1X_VL53L5CX_PLATFORM_H_

#include <zephyr/drivers/i2c.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Structure VL53L5CX_Platform needs to be filled by the customer,
 * depending on his platform. At least, it contains the VL53L5CX I2C address.
 * Some additional fields can be added, as descriptors, or platform
 * dependencies. Anything added into this structure is visible into the platform
 * layer.
 */

typedef struct {
	const struct i2c_dt_spec i2c;
	uint16_t address;
} VL53L5CX_Platform;

/*
 * @brief The macro below is used to define the number of target per zone sent
 * through I2C. This value can be changed by user, in order to tune I2C
 * transaction, and also the total memory size (a lower number of target per
 * zone means a lower RAM usage).
 */
#ifndef VL53L5CX_NB_TARGET_PER_ZONE
#define VL53L5CX_NB_TARGET_PER_ZONE (1U)
#endif

/**
 * @brief Macro that defines buffer attributes for the VL53L5CX driver,
 * adjusting memory alignment and buffer section placement.
 *
 * If CONFIG_VL53L5CX_BUFFER_CUSTOM_SECTION is defined, VL53L5CX_BUF_ATTRIBUTES
 * assigns the buffer to a specific section, .vl53l5cx_buf, in memory. This
 * can be useful if custom section placement is needed for specific memory
 * management requirements on the platform. The alignment of the buffer is
 * controlled by the CONFIG_VL53L5CX_BUFFER_ALIGN configuration parameter.
 *
 * If CONFIG_VL53L5CX_BUFFER_CUSTOM_SECTION is not defined, VL53L5CX_BUF_ATTRIBUTES
 * will only ensure that the buffer is aligned according to CONFIG_VL53L5CX_BUFFER_ALIGN
 * but without placing it in a custom section.
 */
#ifdef CONFIG_VL53L5CX_BUFFER_CUSTOM_SECTION
#define VL53L5CX_BUF_ATTRIBUTES                                                                    \
	Z_GENERIC_SECTION(.vl53l5cx_buf) __aligned(CONFIG_VL53L5CX_BUFFER_ALIGN)
#else
#define VL53L5CX_BUF_ATTRIBUTES __aligned(CONFIG_VL53L5CX_BUFFER_ALIGN)
#endif

/*
 * @brief All macro below are used to configure the sensor output. User can
 * define some macros if he wants to disable selected output, in order to reduce
 * I2C access.
 */

#define VL53L5CX_DISABLE_AMBIENT_PER_SPAD
#define VL53L5CX_DISABLE_NB_SPADS_ENABLED
#define VL53L5CX_DISABLE_AMBIENT_DMAX
#define VL53L5CX_DISABLE_NB_TARGET_DETECTED
#define VL53L5CX_DISABLE_SIGNAL_PER_SPAD
#define VL53L5CX_DISABLE_RANGE_SIGMA_MM
#define VL53L5CX_DISABLE_DISTANCE_MM
#define VL53L5CX_DISABLE_TARGET_STATUS

/*
 * @brief The macro below can be changed to switch between little and big
 * endian. By default, VL53L5CX ULD works with little endian, but user can
 * choose big endian. Just use selected the wanted configuration using macro
 * PROCESSOR_LITTLE_ENDIAN.
 */

#define PROCESSOR_LITTLE_ENDIAN
#ifdef PROCESSOR_LITTLE_ENDIAN
#define SWAP_UINT16(x) (x)
#define SWAP_UINT32(x) (x)
#else
#define SWAP_UINT16(x) (((x) >> 8) | ((x) << 8))
#define SWAP_UINT32(x)                                                                             \
	(((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24))
#endif

/**
 * @param (VL53L5CX_Platform*) p_platform : Pointer of VL53L5CX platform
 * structure.
 * @param (uint16_t) Address : I2C location of value to read.
 * @param (uint8_t) *p_values : Pointer of value to read.
 * @return (uint8_t) status : 0 if OK
 */

uint8_t RdByte(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t *p_value);

/**
 * @brief Mandatory function used to write one single byte.
 * @param (VL53L5CX_Platform*) p_platform : Pointer of VL53L5CX platform
 * structure.
 * @param (uint16_t) Address : I2C location of value to read.
 * @param (uint8_t) value : Pointer of value to write.
 * @return (uint8_t) status : 0 if OK
 */

uint8_t WrByte(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t value);

/**
 * @brief Mandatory function used to read multiples bytes.
 * @param (VL53L5CX_Platform*) p_platform : Pointer of VL53L5CX platform
 * structure.
 * @param (uint16_t) Address : I2C location of values to read.
 * @param (uint8_t) *p_values : Buffer of bytes to read.
 * @param (uint32_t) size : Size of *p_values buffer.
 * @return (uint8_t) status : 0 if OK
 */

uint8_t RdMulti(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t *p_values,
		uint32_t size);

/**
 * @brief Mandatory function used to write multiples bytes.
 * @param (VL53L5CX_Platform*) p_platform : Pointer of VL53L5CX platform
 * structure.
 * @param (uint16_t) Address : I2C location of values to write.
 * @param (uint8_t) *p_values : Buffer of bytes to write.
 * @param (uint32_t) size : Size of *p_values buffer.
 * @return (uint8_t) status : 0 if OK
 */

uint8_t WrMulti(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t *p_values,
		uint32_t size);

/**
 * @brief Mandatory function, used to swap a buffer. The buffer size is always a
 * multiple of 4 (4, 8, 12, 16, ...).
 * @param (uint8_t*) buffer : Buffer to swap, generally uint32_t
 * @param (uint16_t) size : Buffer size to swap
 */

void SwapBuffer(uint8_t *buffer, uint16_t size);

/**
 * @brief Mandatory function, used to wait during an amount of time. It must be
 * filled as it's used into the API.
 * @param (VL53L5CX_Platform*) p_platform : Pointer of VL53L5CX platform
 * structure.
 * @param (uint32_t) TimeMs : Time to wait in ms.
 * @return (uint8_t) status : 0 if wait is finished.
 */

uint8_t WaitMs(VL53L5CX_Platform *p_platform, uint32_t TimeMs);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_VL53L1X_VL53L5CX_PLATFORM_H_ */
