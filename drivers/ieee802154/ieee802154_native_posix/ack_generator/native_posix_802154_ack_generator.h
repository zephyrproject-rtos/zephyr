/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Module that contains an acknowledgement generator for the 802.15.4
 *        radio driver.
 *
 */

#ifndef NATIVE_POSIX_802154_ACK_GENERATOR_H
#define NATIVE_POSIX_802154_ACK_GENERATOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initializes the ACK generator module. */
void nrf_802154_ack_generator_init(void);

/** Creates an ACK in response to the provided frame and inserts it into a
 *  radio buffer.
 *
 * @param [in]  p_frame  Pointer to the buffer that contains PHR and PSDU of
 *                       the frame to respond to.
 *
 * @returns  Either pointer to a constant buffer that contains PHR and PSDU
 *           of the created ACK frame, or NULL in case of an invalid frame.
 */
const uint8_t *nrf_802154_ack_generator_create(const uint8_t *p_frame);

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_ACK_GENERATOR_H */
