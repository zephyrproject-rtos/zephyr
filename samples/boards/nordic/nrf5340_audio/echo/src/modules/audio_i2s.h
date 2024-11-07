/*
 *  Copyright (c) 2021, PACKETCRAFT, INC.
 *
 *  SPDX-License-Identifier: LicenseRef-PCFT
 */

#ifndef _AUDIO_I2S_H_
#define _AUDIO_I2S_H_

#include <zephyr/kernel.h>
#include <stdint.h>

/*
 * Calculate the number of bytes of one frame, as per now, this frame can either
 * be 10 or 7.5 ms. Since we can't have floats in a define we use 15/2 instead
 */

#if ((CONFIG_AUDIO_FRAME_DURATION_US == 7500) && CONFIG_SW_CODEC_LC3)

#define FRAME_SIZE_BYTES                                                                           \
	((CONFIG_I2S_LRCK_FREQ_HZ / 1000 * 15 / 2) * CONFIG_I2S_CH_NUM *                           \
	 CONFIG_AUDIO_BIT_DEPTH_OCTETS)
#else
#define FRAME_SIZE_BYTES                                                                           \
	((CONFIG_I2S_LRCK_FREQ_HZ / 1000 * 10) * CONFIG_I2S_CH_NUM * CONFIG_AUDIO_BIT_DEPTH_OCTETS)
#endif /* ((CONFIG_AUDIO_FRAME_DURATION_US == 7500) && CONFIG_SW_CODEC_LC3) */

#define BLOCK_SIZE_BYTES (FRAME_SIZE_BYTES / CONFIG_FIFO_FRAME_SPLIT_NUM)

/*
 * Calculate the number of samples in a block, divided by the number of samples
 * that will fit within a 32-bit word
 */
#define I2S_SAMPLES_NUM                                                                            \
	(BLOCK_SIZE_BYTES / (CONFIG_AUDIO_BIT_DEPTH_OCTETS) / (32 / CONFIG_AUDIO_BIT_DEPTH_BITS))

/**
 * @brief I2S block complete event callback type
 *
 * @param frame_start_ts I2S frame start timestamp
 * @param rx_buf_released Pointer to the released buffer containing received data
 * @param tx_buf_released Pointer to the released buffer that was used to sent data
 */
typedef void (*i2s_blk_comp_callback_t)(uint32_t frame_start_ts, uint32_t *rx_buf_released,
					uint32_t const *tx_buf_released);

/**
 * @brief Supply the buffers to be used in the next part of the I2S transfer
 *
 * @param tx_buf Pointer to the buffer with data to be sent
 * @param rx_buf Pointer to the buffer for received data
 */
void audio_i2s_set_next_buf(const uint8_t *tx_buf, uint32_t *rx_buf);

/**
 * @brief Start the continuous I2S transfer
 *
 * @param tx_buf Pointer to the buffer with data to be sent
 * @param rx_buf Pointer to the buffer for received data
 */
void audio_i2s_start(const uint8_t *tx_buf, uint32_t *rx_buf);

/**
 * @brief Stop the continuous I2S transfer
 */
void audio_i2s_stop(void);

/**
 * @brief Register callback function for I2S block complete event
 *
 * @param blk_comp_callback Callback function
 */
void audio_i2s_blk_comp_cb_register(i2s_blk_comp_callback_t blk_comp_callback);

/**
 * @brief Initialize I2S module
 */
void audio_i2s_init(void);

#endif /* _AUDIO_I2S_H_ */
