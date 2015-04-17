/*
 *  Copyright (c) 2008, Swedish Institute of Computer Science
 *  All rights reserved.
 *
 *  Additional fixes for AVR contributed by:
 *        Colin O'Flynn coflynn@newae.com
 *        Eric Gnoske egnoske@gmail.com
 *        Blake Leverett bleverett@gmail.com
 *        Mike Vidales mavida404@gmail.com
 *        Kevin Brown kbrown3@uccs.edu
 *        Nate Bohlmann nate@elfwerks.com
 *
 *  Additional fixes for MSP430 contributed by:
 *        Joakim Eriksson
 *        Niclas Finne
 *        Nicolas Tsiftes
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the copyright holders nor the names of
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
*/

/**
 *    \addtogroup net
 *    @{
 */

/**
 *    \defgroup frame802154 802.15.4 frame creation and parsing
 *    @{
 */
/**
 *  \file
 *  \brief 802.15.4 frame creation and parsing functions
 *
 *  This file converts to and from a structure to a packed 802.15.4
 *  frame.
 *
*/


/* Includes */
#ifndef FRAME_802154_H
#define FRAME_802154_H

#include "contiki-conf.h"

#ifdef IEEE802154_CONF_PANID
#define IEEE802154_PANID           IEEE802154_CONF_PANID
#else /* IEEE802154_CONF_PANID */
#define IEEE802154_PANID           0xABCD
#endif /* IEEE802154_CONF_PANID */

/* Macros & Defines */

/** \brief These are some definitions of values used in the FCF.  See the 802.15.4 spec for details.
 *  \name FCF element values definitions
 *  @{
 */
#define FRAME802154_BEACONFRAME     (0x00)
#define FRAME802154_DATAFRAME       (0x01)
#define FRAME802154_ACKFRAME        (0x02)
#define FRAME802154_CMDFRAME        (0x03)

#define FRAME802154_BEACONREQ       (0x07)

#define FRAME802154_IEEERESERVED    (0x00)
#define FRAME802154_NOADDR          (0x00)      /**< Only valid for ACK or Beacon frames. */
#define FRAME802154_SHORTADDRMODE   (0x02)
#define FRAME802154_LONGADDRMODE    (0x03)

#define FRAME802154_NOBEACONS       (0x0F)

#define FRAME802154_BROADCASTADDR   (0xFFFF)
#define FRAME802154_BROADCASTPANDID (0xFFFF)

#define FRAME802154_IEEE802154_2003 (0x00)
#define FRAME802154_IEEE802154_2006 (0x01)

#define FRAME802154_SECURITY_LEVEL_NONE        (0)
#define FRAME802154_SECURITY_LEVEL_MIC_32      (1)
#define FRAME802154_SECURITY_LEVEL_MIC_64      (2)
#define FRAME802154_SECURITY_LEVEL_MIC_128     (3)
#define FRAME802154_SECURITY_LEVEL_ENC         (4)
#define FRAME802154_SECURITY_LEVEL_ENC_MIC_32  (5)
#define FRAME802154_SECURITY_LEVEL_ENC_MIC_64  (6)
#define FRAME802154_SECURITY_LEVEL_ENC_MIC_128 (7)

#define FRAME802154_IMPLICIT_KEY               (0)
#define FRAME802154_1_BYTE_KEY_ID_MODE         (1)
#define FRAME802154_5_BYTE_KEY_ID_MODE         (2)
#define FRAME802154_9_BYTE_KEY_ID_MODE         (3)

/**
 *    @brief  The IEEE 802.15.4 frame has a number of constant/fixed fields that
 *            can be counted to make frame construction and max payload
 *            calculations easier.
 *
 *            These include:
 *            1. FCF                  - 2 bytes       - Fixed
 *            2. Sequence number      - 1 byte        - Fixed
 *            3. Addressing fields    - 4 - 20 bytes  - Variable
 *            4. Aux security header  - 0 - 14 bytes  - Variable
 *            5. CRC                  - 2 bytes       - Fixed
*/

/**
 * \brief Defines the bitfields of the frame control field (FCF).
 */
typedef struct {
  uint8_t frame_type;        /**< 3 bit. Frame type field, see 802.15.4 */
  uint8_t security_enabled;  /**< 1 bit. True if security is used in this frame */
  uint8_t frame_pending;     /**< 1 bit. True if sender has more data to send */
  uint8_t ack_required;      /**< 1 bit. Is an ack frame required? */
  uint8_t panid_compression; /**< 1 bit. Is this a compressed header? */
  /*   uint8_t reserved; */  /**< 3 bit. Unused bits */
  uint8_t dest_addr_mode;    /**< 2 bit. Destination address mode, see 802.15.4 */
  uint8_t frame_version;     /**< 2 bit. 802.15.4 frame version */
  uint8_t src_addr_mode;     /**< 2 bit. Source address mode, see 802.15.4 */
} frame802154_fcf_t;

/** \brief 802.15.4 security control bitfield.  See section 7.6.2.2.1 in 802.15.4 specification */
typedef struct {
  uint8_t  security_level; /**< 3 bit. security level      */
  uint8_t  key_id_mode;    /**< 2 bit. Key identifier mode */
  uint8_t  reserved;       /**< 3 bit. Reserved bits       */
} frame802154_scf_t;

typedef union {
  uint32_t u32;
  uint16_t u16[2];
  uint8_t u8[4];
} frame802154_frame_counter_t;

typedef union {
  uint16_t u16[4];
  uint8_t u8[8];
} frame802154_key_source_t;

/** \brief 802.15.4 Aux security header */
typedef struct {
  frame802154_scf_t security_control;        /**< Security control bitfield */
  frame802154_frame_counter_t frame_counter; /**< Frame counter, used for security */
  frame802154_key_source_t key_source;       /**< Key Source subfield */
  uint8_t key_index;                         /**< Key Index subfield */
} frame802154_aux_hdr_t;

/** \brief Parameters used by the frame802154_create() function.  These
 *  parameters are used in the 802.15.4 frame header.  See the 802.15.4
 *  specification for details.
 */
typedef struct {
  /* The fields dest_addr and src_addr must come first to ensure they are aligned to the
   * CPU word size. Needed as they are accessed directly as linkaddr_t*. Note we cannot use
   * the type linkaddr_t directly here, as we always need 8 bytes, not LINKADDR_SIZE bytes. */
  uint8_t dest_addr[8];           /**< Destination address */
  uint8_t src_addr[8];            /**< Source address */
  frame802154_fcf_t fcf;          /**< Frame control field  */
  uint8_t seq;                    /**< Sequence number */
  uint16_t dest_pid;              /**< Destination PAN ID */
  uint16_t src_pid;               /**< Source PAN ID */
  frame802154_aux_hdr_t aux_hdr;  /**< Aux security header */
  uint8_t *payload;               /**< Pointer to 802.15.4 payload */
  int payload_len;                /**< Length of payload field */
} frame802154_t;

/* Prototypes */

int frame802154_hdrlen(frame802154_t *p);
int frame802154_create(frame802154_t *p, uint8_t *buf);
int frame802154_parse(uint8_t *data, int length, frame802154_t *pf);

/** @} */
#endif /* FRAME_802154_H */
/** @} */
/** @} */
