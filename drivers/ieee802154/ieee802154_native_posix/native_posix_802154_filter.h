/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Module that implements the incoming frame filter API.
 *
 */

#ifndef NATIVE_POSIX_802154_FILTER_H_
#define NATIVE_POSIX_802154_FILTER_H_

#include <stdbool.h>
#include <stdint.h>

#include "native_posix_802154_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_802154_filter Incoming frame filter API
 * @{
 * @ingroup nrf_802154
 * @brief Procedures used to discard the incoming frames that contain
 *        unexpected data in PHR or MHR.
 */

/**
 * @brief Verifies if the given part of the frame is valid.
 *
 * This function is called a few times for each received frame. The first call
 * is after the FCF is received (PSDU length is 2 and @p p_num_bytes value
 * is 3). The subsequent calls are performed when the number of bytes requested
 * by the previous call is available. The iteration ends when the function does
 * not request any more bytes to check. If the verified part of the function is
 * correct, this function returns true and sets @p p_num_bytes to the number of
 * bytes that should be available in PSDU during the next iteration. If the
 * frame is correct and there is nothing more to check, this function returns
 * true and does not modify the @p p_num_bytes value. If the verified frame is
 * incorrect, this function returns false and the @p p_num_bytes value is
 * undefined.
 *
 * @param[in]    p_data       Pointer to a buffer that contains PHR and PSDU of
 *                            the incoming frame.
 * @param[inout] p_num_bytes  Number of bytes available in @p p_data buffer.
 *                            This value is either set to the requested number
 *                            of bytes for the next iteration or remains
 *                            unchanged if no more iterations are to be
 *                            performed during the filtering of the given
 *                            frame.
 *
 * @retval NRF_802154_RX_ERROR_NONE               Verified part of the incoming
 *                                                frame is valid.
 * @retval NRF_802154_RX_ERROR_INVALID_FRAME      Verified part of the incoming
 *                                                frame is invalid.
 * @retval NRF_802154_RX_ERROR_INVALID_DEST_ADDR  Incoming frame has
 *                                                destination address that
 *                                                mismatches the address of
 *                                                this node.
 */
nrf_802154_rx_error_t nrf_802154_filter_frame_part(const uint8_t *p_data,
						   uint8_t *p_num_bytes);

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_FILTER_H_ */
