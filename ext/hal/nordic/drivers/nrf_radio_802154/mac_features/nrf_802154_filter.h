/* Copyright (c) 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @brief This file implements incoming frame filter API.
 *
 */

#ifndef NRF_802154_FILTER_H_
#define NRF_802154_FILTER_H_

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_types.h"

/**
 * @defgroup nrf_802154_filter Incoming frame filter API.
 * @{
 * @ingroup nrf_802154
 * @brief Procedures used to discard incoming frames that contain unexpected data in PHR or MHR.
 */

/**
 * @brief Verify if given part of the frame is valid.
 *
 * This function is called a few times for each received frame. First call is after FCF is received
 * (PSDU length is 1 - @p p_num_bytes value is 1). Subsequent calls are performed when number of
 * bytes requested by previous call is available. Iteration ends when function does not request
 * any more bytes to check.
 * If verified part of the function is correct this function returns true and sets @p p_num_bytes to
 * number of bytes that should be available in PSDU during next iteration. If frame is correct and
 * there is nothing more to check, function returns true and does not modify @p p_num_bytes value.
 * If verified frame is incorrect this function returns false and @p p_num_bytes value is undefined.
 *
 * @param[in]    p_psdu       Pointer to PSDU of incoming frame.
 * @param[inout] p_num_bytes  Number of bytes available in @p p_psdu buffer. This value is set to
 *                            requested number of bytes for next iteration or this value is
 *                            unchanged if no more iterations shall be performed during filtering of
 *                            given frame.
 *
 * @retval NRF_802154_RX_ERROR_NONE               Verified part of the incoming frame is valid.
 * @retval NRF_802154_RX_ERROR_INVALID_FRAME      Verified part of the incoming frame is invalid.
 * @retval NRF_802154_RX_ERROR_INVALID_DEST_ADDR  Incoming frame has destination address that
 *                                                mismatches address of this node.
 */
nrf_802154_rx_error_t nrf_802154_filter_frame_part(const uint8_t * p_psdu, uint8_t * p_num_bytes);

#endif /* NRF_802154_FILTER_H_ */
