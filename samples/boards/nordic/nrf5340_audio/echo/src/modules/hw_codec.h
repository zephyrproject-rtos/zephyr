/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HW_CODEC_H_
#define _HW_CODEC_H_

#include <stdint.h>

/**
 * @brief  Set volume on HW_CODEC
 *
 * @details Also unmutes the volume on HW_CODEC
 *
 * @param  set_val  Set the volume to a specific value.
 *                  This range of the value is between 0 to 128.
 *
 * @return 0 if successful, error otherwise
 */
int hw_codec_volume_set(uint8_t set_val);

/**
 * @brief  Adjust volume on HW_CODEC
 *
 * @details Also unmute the volume on HW_CODEC
 *
 * @param  adjustment  The adjustment in dB, can be negative or positive.
 *			If the value 0 is used, the previous known value will be
 *			written, default value will be used if no previous value
 *			exists
 *
 * @return 0 if successful, error otherwise
 */
int hw_codec_volume_adjust(int8_t adjustment);

/**
 * @brief Decrease output volume on HW_CODEC by 3 dB
 *
 * @details Also unmute the volume on HW_CODEC
 *
 * @return 0 if successful, error otherwise
 */
int hw_codec_volume_decrease(void);

/**
 * @brief Increase output volume on HW_CODEC by 3 dB
 *
 * @details Also unmute the volume on HW_CODEC
 *
 * @return 0 if successful, error otherwise
 */
int hw_codec_volume_increase(void);

/**
 * @brief  Mute volume on HW_CODEC
 *
 * @return 0 if successful, error otherwise
 */
int hw_codec_volume_mute(void);

/**
 * @brief  Unmute volume on HW_CODEC
 *
 * @return 0 if successful, error otherwise
 */
int hw_codec_volume_unmute(void);

/**
 * @brief Enable relevant settings in HW_CODEC to
 *        send and receive PCM data over I2S
 *
 * @note  FLL1 must be toggled after I2S has started to enable HW_CODEC
 *
 * @return 0 if successful, error otherwise
 */
int hw_codec_default_conf_enable(void);

/**
 * @brief Reset HW_CODEC
 *
 * @note  This will first disable output, then do a soft reset
 *
 * @return 0 if successful, error otherwise
 */
int hw_codec_soft_reset(void);

/**
 * @brief Initialize HW_CODEC
 *
 * @return 0 if successful, error otherwise
 */
int hw_codec_init(void);

#endif /* _HW_CODEC_H_ */
