/**
 * @file i2s_handler.h
 * @author Jorg Wieme (jorg.wieme@ugent.be)
 * @brief
 * @version 0.1
 * @date 2024-11-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef _I2S_HANDLER_H_
#define _I2S_HANDLER_H_

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

#include "data_fifo.h"

/**
 * @brief Start the audio module
 *
 * @note The continuously running I2S is started
 *
 */
void audio_start(void);

/**
 * @brief Stop the audio module
 *
 */
void audio_stop(void);

/**
 * @brief Initialize the audio module
 *
 * @return 0 if successful, error otherwise
 */
int audio_init(void);

#endif /* _I2S_HANDLER_H_ */
