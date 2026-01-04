/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LED_STRIP_ENCODER_H_
#define LED_STRIP_ENCODER_H_

#include <stdint.h>
#include <zephyr/drivers/misc/espressif_rmt/rmt_encoder.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type of led strip encoder configuration
 */
typedef struct {
    uint32_t resolution; /**< Encoder resolution, in Hz */
} led_strip_encoder_config_t;

/**
 * @brief Create RMT encoder for encoding LED strip pixels into RMT symbols.
 *
 * @param config Encoder configuration.
 * @param ret_encoder Returned encoder handle.
 * @retval 0 If successful.
 * @retval -EINVAL for any invalid arguments.
 * @retval -ENOMEM out of memory when creating led strip encoder.
 * @retval -ENODEV because of other error.
 */
int rmt_new_led_strip_encoder(const led_strip_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);

#ifdef __cplusplus
}
#endif

#endif /* LED_STRIP_ENCODER_H_ */
