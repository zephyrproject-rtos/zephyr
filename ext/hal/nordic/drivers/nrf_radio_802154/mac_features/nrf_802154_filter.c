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
 * @file
 *   This file implements incoming frame filtering according to 3 and 4 levels of filtering.
 *
 * Filtering details are specified in 802.15.4-2015: 6.7.2.
 * 1st and 2nd filtering level is performed by FSM module depending on promiscuous mode, when FCS is
 * received.
 */

#include "nrf_802154_filter.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nrf_802154_const.h"
#include "nrf_802154_pib.h"

#define FCF_CHECK_OFFSET           (PHR_SIZE + FCF_SIZE)
#define SHORT_ADDR_CHECK_OFFSET    (DEST_ADDR_OFFSET + SHORT_ADDRESS_SIZE)
#define EXTENDED_ADDR_CHECK_OFFSET (DEST_ADDR_OFFSET + EXTENDED_ADDRESS_SIZE)

/**
 * @brief Check if given frame version is allowed for given frame type.
 *
 * @param[in] frame_type     Type of incoming frame.
 * @param[in] frame_version  Version of incoming frame.
 *
 * @retval true   Given frame version is allowed for given frame type.
 * @retval false  Given frame version is not allowed for given frame type.
 */
static bool frame_type_and_version_filter(uint8_t frame_type, uint8_t frame_version)
{
    bool result;

    switch (frame_type)
    {
        case FRAME_TYPE_BEACON:
        case FRAME_TYPE_DATA:
        case FRAME_TYPE_ACK:
        case FRAME_TYPE_COMMAND:
            result = (frame_version != FRAME_VERSION_3);
            break;

        case FRAME_TYPE_MULTIPURPOSE:
            result = (frame_version == FRAME_VERSION_0);
            break;

        case FRAME_TYPE_FRAGMENT:
        case FRAME_TYPE_EXTENDED:
            result = true;
            break;

        default:
            result = false;
    }

    return result;
}

/**
 * @brief Check if given frame type may include destination address fields.
 *
 * @note Actual presence of destination address fields in the frame is indicated by FCF.
 *
 * @param[in] frame_type  Type of incoming frame.
 *
 * @retval true   Given frame type may include addressing fields.
 * @retval false  Given frame type may not include addressing fields.
 */
static bool dst_addressing_may_be_present(uint8_t frame_type)
{
    bool result;

    switch (frame_type)
    {
        case FRAME_TYPE_BEACON:
        case FRAME_TYPE_DATA:
        case FRAME_TYPE_ACK:
        case FRAME_TYPE_COMMAND:
        case FRAME_TYPE_MULTIPURPOSE:
            result = true;
            break;

        case FRAME_TYPE_FRAGMENT:
        case FRAME_TYPE_EXTENDED:
            result = false;
            break;

        default:
            result = false;
    }

    return result;
}

/**
 * @brief Get offset of end of addressing fields for given frame assuming its version is 2006.
 *
 * If given frame contains errors that prevent getting offset, this function returns false. If there
 * are no destination address fields in given frame, this function returns true and does not modify
 * @p p_num_bytes. If there is destination address in given frame, this function returns true and
 * inserts offset of addressing fields end to @p p_num_bytes.
 *
 * @param[in]  p_psdu       Pointer of PSDU of incoming frame.
 * @param[out] p_num_bytes  Offset of addressing fields end.
 * @param[in]  frame_type   Type of incoming frame.
 *
 * @retval NRF_802154_RX_ERROR_NONE               No errors in given frame were detected - it may be
 *                                                further processed.
 * @retval NRF_802154_RX_ERROR_INVALID_DEST_ADDR  The frame is valid but addressed to another node.
 * @retval NRF_802154_RX_ERROR_INVALID_FRAME      Detected an error in given frame - it should be
 *                                                discarded.
 */
static nrf_802154_rx_error_t dst_addressing_end_offset_get_2006(const uint8_t * p_psdu,
                                                                uint8_t       * p_num_bytes,
                                                                uint8_t         frame_type)
{
    nrf_802154_rx_error_t result;

    switch (p_psdu[DEST_ADDR_TYPE_OFFSET] & DEST_ADDR_TYPE_MASK)
    {
        case DEST_ADDR_TYPE_SHORT:
            *p_num_bytes = SHORT_ADDR_CHECK_OFFSET;
            result       = NRF_802154_RX_ERROR_NONE;
            break;

        case DEST_ADDR_TYPE_EXTENDED:
            *p_num_bytes = EXTENDED_ADDR_CHECK_OFFSET;
            result       = NRF_802154_RX_ERROR_NONE;
            break;

        case DEST_ADDR_TYPE_NONE:
            if (nrf_802154_pib_pan_coord_get() || (frame_type == FRAME_TYPE_BEACON))
            {
                switch (p_psdu[SRC_ADDR_TYPE_OFFSET] & SRC_ADDR_TYPE_MASK)
                {
                    case SRC_ADDR_TYPE_SHORT:
                        *p_num_bytes = SHORT_ADDR_CHECK_OFFSET;
                        result       = NRF_802154_RX_ERROR_NONE;
                        break;

                    case SRC_ADDR_TYPE_EXTENDED:
                        *p_num_bytes = EXTENDED_ADDR_CHECK_OFFSET;
                        result       = NRF_802154_RX_ERROR_NONE;
                        break;

                    default:
                        result = NRF_802154_RX_ERROR_INVALID_FRAME;
                }
            }
            else
            {
                result = NRF_802154_RX_ERROR_INVALID_DEST_ADDR;
            }

            break;

        default:
            result = NRF_802154_RX_ERROR_INVALID_FRAME;
    }

    return result;
}

/**
 * @brief Get offset of end of addressing fields for given frame assuming its version is 2015.
 *
 * If given frame contains errors that prevent getting offset, this function returns false. If there
 * are no destination address fields in given frame, this function returns true and does not modify
 * @p p_num_bytes. If there is destination address in given frame, this function returns true and
 * inserts offset of addressing fields end to @p p_num_bytes.
 *
 * @param[in]  p_psdu       Pointer of PSDU of incoming frame.
 * @param[out] p_num_bytes  Offset of addressing fields end.
 * @param[in]  frame_type   Type of incoming frame.
 *
 * @retval NRF_802154_RX_ERROR_NONE               No errors in given frame were detected - it may be
 *                                                further processed.
 * @retval NRF_802154_RX_ERROR_INVALID_DEST_ADDR  The frame is valid but addressed to another node.
 * @retval NRF_802154_RX_ERROR_INVALID_FRAME      Detected an error in given frame - it should be
 *                                                discarded.
 */
static nrf_802154_rx_error_t dst_addressing_end_offset_get_2015(const uint8_t * p_psdu,
                                                                uint8_t       * p_num_bytes,
                                                                uint8_t         frame_type)
{
    nrf_802154_rx_error_t result;

    switch (frame_type)
    {
        case FRAME_TYPE_BEACON:
        case FRAME_TYPE_DATA:
        case FRAME_TYPE_ACK:
        case FRAME_TYPE_COMMAND:
            // TODO: Implement dst addressing filtering according to 2015 spec
            result = dst_addressing_end_offset_get_2006(p_psdu, p_num_bytes, frame_type);
            break;

        case FRAME_TYPE_MULTIPURPOSE:
            // TODO: Implement dst addressing filtering according to 2015 spec
            result = NRF_802154_RX_ERROR_INVALID_FRAME;
            break;

        case FRAME_TYPE_FRAGMENT:
        case FRAME_TYPE_EXTENDED:
            // No addressing data
            result = NRF_802154_RX_ERROR_NONE;
            break;

        default:
            result = NRF_802154_RX_ERROR_INVALID_FRAME;
    }

    return result;
}

/**
 * @brief Get offset of end of addressing fields for given frame.
 *
 * If given frame contains errors that prevent getting offset, this function returns false. If there
 * are no destination address fields in given frame, this function returns true and does not modify
 * @p p_num_bytes. If there is destination address in given frame, this function returns true and
 * inserts offset of addressing fields end to @p p_num_bytes.
 *
 * @param[in]  p_psdu       Pointer of PSDU of incoming frame.
 * @param[out] p_num_bytes  Offset of addressing fields end.
 * @param[in]  frame_type   Type of incoming frame.
 *
 * @retval NRF_802154_RX_ERROR_NONE               No errors in given frame were detected - it may be
 *                                                further processed.
 * @retval NRF_802154_RX_ERROR_INVALID_DEST_ADDR  The frame is valid but addressed to another node.
 * @retval NRF_802154_RX_ERROR_INVALID_FRAME      Detected an error in given frame - it should be
 *                                                discarded.
 */
static nrf_802154_rx_error_t dst_addressing_end_offset_get(const uint8_t * p_psdu,
                                                           uint8_t       * p_num_bytes,
                                                           uint8_t         frame_type,
                                                           uint8_t         frame_version)
{
    nrf_802154_rx_error_t result;

    switch (frame_version)
    {
        case FRAME_VERSION_0:
        case FRAME_VERSION_1:
            result = dst_addressing_end_offset_get_2006(p_psdu, p_num_bytes, frame_type);
            break;

        case FRAME_VERSION_2:
            result = dst_addressing_end_offset_get_2015(p_psdu, p_num_bytes, frame_type);
            break;

        default:
            result = NRF_802154_RX_ERROR_INVALID_FRAME;
    }

    return result;
}

/**
 * Verify if destination PAN Id of incoming frame allows processing by this node.
 *
 * @param[in] p_psdu  Pointer of PSDU of incoming frame.
 *
 * @retval true   PAN Id of incoming frame allows further processing of the frame.
 * @retval false  PAN Id of incoming frame does not allow further processing.
 */
static bool dst_pan_id_check(const uint8_t * p_psdu)
{
    bool result;

    if ((0 == memcmp(&p_psdu[PAN_ID_OFFSET], nrf_802154_pib_pan_id_get(), PAN_ID_SIZE)) ||
        (0 == memcmp(&p_psdu[PAN_ID_OFFSET], BROADCAST_ADDRESS, PAN_ID_SIZE)))
    {
        result = true;
    }
    else if ((FRAME_TYPE_BEACON == (p_psdu[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK)) &&
             (0 == memcmp(nrf_802154_pib_pan_id_get(), BROADCAST_ADDRESS, PAN_ID_SIZE)))
    {
        result = true;
    }
    else
    {
        result = false;
    }

    return result;
}

/**
 * Verify if destination short address of incoming frame allows processing by this node.
 *
 * @param[in] p_psdu  Pointer of PSDU of incoming frame.
 *
 * @retval true   Destination address of incoming frame allows further processing of the frame.
 * @retval false  Destination address of incoming frame does not allow further processing.
 */
static bool dst_short_addr_check(const uint8_t * p_psdu)
{
    bool result;

    if ((0 == memcmp(&p_psdu[DEST_ADDR_OFFSET],
                     nrf_802154_pib_short_address_get(),
                     SHORT_ADDRESS_SIZE)) ||
        (0 == memcmp(&p_psdu[DEST_ADDR_OFFSET], BROADCAST_ADDRESS, SHORT_ADDRESS_SIZE)))
    {
        result = true;
    }
    else if (FRAME_TYPE_BEACON == (p_psdu[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK))
    {
        result = true;
    }
    else if (DEST_ADDR_TYPE_NONE == (p_psdu[DEST_ADDR_TYPE_OFFSET] & DEST_ADDR_TYPE_MASK) &&
             nrf_802154_pib_pan_coord_get())
    {
        result = true;
    }
    else
    {
        result = false;
    }

    return result;
}

/**
 * Verify if destination extended address of incoming frame allows processing by this node.
 *
 * @param[in] p_psdu  Pointer of PSDU of incoming frame.
 *
 * @retval true   Destination address of incoming frame allows further processing of the frame.
 * @retval false  Destination address of incoming frame does not allow further processing.
 */
static bool dst_extended_addr_check(const uint8_t * p_psdu)
{
    bool result;

    if (0 == memcmp(&p_psdu[DEST_ADDR_OFFSET],
                    nrf_802154_pib_extended_address_get(),
                    EXTENDED_ADDRESS_SIZE))
    {
        result = true;
    }
    else if (FRAME_TYPE_BEACON == (p_psdu[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK))
    {
        result = true;
    }
    else if (DEST_ADDR_TYPE_NONE == (p_psdu[DEST_ADDR_TYPE_OFFSET] & DEST_ADDR_TYPE_MASK) &&
             nrf_802154_pib_pan_coord_get())
    {
        result = true;
    }
    else
    {
        result = false;
    }

    return result;
}

nrf_802154_rx_error_t nrf_802154_filter_frame_part(const uint8_t * p_psdu, uint8_t * p_num_bytes)
{
    nrf_802154_rx_error_t result = NRF_802154_RX_ERROR_INVALID_FRAME;

    switch (*p_num_bytes)
    {
        case FCF_CHECK_OFFSET:
        {
            if (p_psdu[0] < IMM_ACK_LENGTH || p_psdu[0] > MAX_PACKET_SIZE)
            {
                result = NRF_802154_RX_ERROR_INVALID_LENGTH;
                break;
            }

            uint8_t frame_type    = p_psdu[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK;
            uint8_t frame_version = p_psdu[FRAME_VERSION_OFFSET] & FRAME_VERSION_MASK;

            if (!frame_type_and_version_filter(frame_type, frame_version))
            {
                result = NRF_802154_RX_ERROR_INVALID_FRAME;
                break;
            }

            if (!dst_addressing_may_be_present(frame_type))
            {
                result = NRF_802154_RX_ERROR_NONE;
                break;
            }

            result = dst_addressing_end_offset_get(p_psdu, p_num_bytes, frame_type, frame_version);
            break;
        }

        case SHORT_ADDR_CHECK_OFFSET:
            if (!dst_pan_id_check(p_psdu))
            {
                result = NRF_802154_RX_ERROR_INVALID_DEST_ADDR;
                break;
            }

            result = dst_short_addr_check(p_psdu) ? NRF_802154_RX_ERROR_NONE :
                     NRF_802154_RX_ERROR_INVALID_DEST_ADDR;
            break;

        case EXTENDED_ADDR_CHECK_OFFSET:
            if (!dst_pan_id_check(p_psdu))
            {
                result = NRF_802154_RX_ERROR_INVALID_DEST_ADDR;
                break;
            }

            result = dst_extended_addr_check(p_psdu) ? NRF_802154_RX_ERROR_NONE :
                     NRF_802154_RX_ERROR_INVALID_DEST_ADDR;
            break;

        default:
            assert(false);
    }

    return result;
}
