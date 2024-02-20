/*
 * Copyright (c) 2017 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/** @defgroup DriverIcp101xx Icp101xx driver
 *  @brief    Low-level driver for Icp101xx devices
 *  @ingroup  Drivers
 *  @{
 */

#ifndef _INV_ICP101XX_H_
#define _INV_ICP101XX_H_

#include "../../../EmbUtils/InvExport.h"
#include "../../../EmbUtils/InvBool.h"
#include "../../../EmbUtils/InvError.h"

#include "Icp101xxSerif.h"

#include <stdint.h>
#include <assert.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ICP101XX_I2C_ADDR (0x63)
#define ICP10114_I2C_ADDR (0x64)

#define ICP101XX_ID                       0x08
#define ICP101XX_PRODUCT_SPECIFIC_BITMASK 0x003F

#define ICP101XX_CMD_READ_ID     0xEFC8
#define ICP101XX_OTP_READ_ADDR   0x0066
#define ICP101XX_CMD_SET_CAL_PTR 0xC595
#define ICP101XX_CMD_INC_CAL_PTR 0xC7F7
#define ICP101XX_CMD_SOFT_RESET  0x805D

#define ICP101XX_CMD_MEAS_LOW_POWER_T_FIRST       0x609C
#define ICP101XX_CMD_MEAS_LOW_POWER_P_FIRST       0x401A
#define ICP101XX_CMD_MEAS_NORMAL_T_FIRST          0x6825
#define ICP101XX_CMD_MEAS_NORMAL_P_FIRST          0x48A3
#define ICP101XX_CMD_MEAS_LOW_NOISE_T_FIRST       0x70DF
#define ICP101XX_CMD_MEAS_LOW_NOISE_P_FIRST       0x5059
#define ICP101XX_CMD_MEAS_ULTRA_LOW_NOISE_T_FIRST 0x7866
#define ICP101XX_CMD_MEAS_ULTRA_LOW_NOISE_P_FIRST 0x58E0

enum icp101xx_meas {
	ICP101XX_MEAS_LOW_POWER_T_FIRST = 0,
	ICP101XX_MEAS_LOW_POWER_P_FIRST = 1,
	ICP101XX_MEAS_NORMAL_T_FIRST = 2,
	ICP101XX_MEAS_NORMAL_P_FIRST = 3,
	ICP101XX_MEAS_LOW_NOISE_T_FIRST = 4,
	ICP101XX_MEAS_LOW_NOISE_P_FIRST = 5,
	ICP101XX_MEAS_ULTRA_LOW_NOISE_T_FIRST = 6,
	ICP101XX_MEAS_ULTRA_LOW_NOISE_P_FIRST = 7
};

typedef struct inv_icp101xx {
	struct inv_icp101xx_serif serif;
	uint32_t min_delay_us;
	uint8_t pressure_en;
	uint8_t temperature_en;
	float sensor_constants[4]; /* OTP */
	float p_Pa_calib[3];
	float LUT_lower;
	float LUT_upper;
	float quadr_factor;
	float offst_factor;
	enum icp101xx_meas measurement_mode;
} inv_icp101xx_t;

/** @brief Reset and initialize driver states
 *  @param[in] s handle to driver states structure
 *  @param[in] serif handle to SERIF object for underlying register access
 */
static inline void inv_icp101xx_reset_states(struct inv_icp101xx *s,
					     const struct inv_icp101xx_serif *serif)
{
	memset(s, 0, sizeof(*s));
	s->serif = *serif;
}

/** @brief Initialize INVPRES : check whoami through serial interface and load compensation
 * parameters
 */
int INV_EXPORT inv_icp101xx_init(struct inv_icp101xx *s);

/** @brief Check and retrieve for new data
 *  @param[out] pressure pressure data in Pascal
 *  @param[out] temperature temperature data in Degree Celsius
 *  @return     0 on success, negative value on error
 */
int INV_EXPORT inv_icp101xx_get_data(struct inv_icp101xx *s, int *raw_pressure,
				     int *raw_temperature, float *pressure, float *temperature);

/** @brief Enables / disables the invpres sensor for both pressure and temperature
 * @param[in] enable			0=off, 1=on
 * @return 0 in case of success, negative value on error
 */
int INV_EXPORT inv_icp101xx_enable_sensor(struct inv_icp101xx *s, inv_bool_t en);

/** @brief Enables / disables the invpres sensor for pressure
 * @param[in] enable			0=off, 1=on
 * @return 0 in case of success, negative value on error
 */
int INV_EXPORT inv_icp101xx_pressure_enable_sensor(struct inv_icp101xx *s, inv_bool_t en);

/** @brief Enables / disables the invpres sensor for temperature
 * @param[in] enable			0=off, 1=on
 * @return 0 in case of success, negative value on error
 */
int INV_EXPORT inv_icp101xx_temperature_enable_sensor(struct inv_icp101xx *s, inv_bool_t en);

/** @brief return WHOAMI value
 *  @param[out] whoami WHOAMI for device
 *  @return     0 on success, negative value on error
 */
int INV_EXPORT inv_icp101xx_get_whoami(struct inv_icp101xx *s, uint8_t *whoami);

/** @brief Send soft reset
 *  @return     0 on success, negative value on error
 */
int INV_EXPORT inv_icp101xx_soft_reset(struct inv_icp101xx *s);

/** @brief Hook for low-level high res system sleep() function to be implemented by upper layer
 *  ~100us resolution is sufficient
 *  @param[in] us number of us the calling thread should sleep
 */
extern void inv_icp101xx_sleep_us(int us);

#ifdef __cplusplus
}
#endif

#endif /* _INV_ICP101XX_H_ */
