/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Module that contains definitions of constant values used by the
 * nRF 802.15.4 driver.
 */

#ifndef NATIVE_POSIX_802154_CONST_H_
#define NATIVE_POSIX_802154_CONST_H_

#include <stdint.h>
#include "native_posix_802154_config.h"
/** The first byte of an ACK frame containing a pending bit. */
#define ACK_HEADER_WITH_PENDING 0x12
/** The first byte of an ACK frame without a pending bit. */
#define ACK_HEADER_WITHOUT_PENDING 0x02

/** Byte containing the ACK request bit (+1 for the frame length byte). */
#define ACK_REQUEST_OFFSET 1
/** ACK request bit. */
#define ACK_REQUEST_BIT (1 << 5)

/** Byte containing the dest address type (+1 for the frame length byte). */
#define DEST_ADDR_TYPE_OFFSET 2
/** Mask of bits containing the destination address type. */
#define DEST_ADDR_TYPE_MASK 0x0c
/** Bits containing the extended destination address type. */
#define DEST_ADDR_TYPE_EXTENDED 0x0c
/** Bits containing a not-present destination address type. */
#define DEST_ADDR_TYPE_NONE 0x00
/** Bits containing the short destination address type. */
#define DEST_ADDR_TYPE_SHORT 0x08
/** Offset of the destination address in the Data frame
 * (+1 for the frame length byte).
 */
#define DEST_ADDR_OFFSET 6

/** Byte containing the DSN value (+1 for the frame length byte). */
#define DSN_OFFSET 3
/** Byte containing the DSN suppression field. */
#define DSN_SUPPRESS_OFFSET 2
/** Bits containing the DSN suppression field. */
#define DSN_SUPPRESS_BIT 0x01

/** Bit containing the Frame Counter Suppression field. */
#define FRAME_COUNTER_SUPPRESS_BIT 0x20

/** Byte containing a pending bit (+1 for the frame length byte). */
#define FRAME_PENDING_OFFSET 1
/** Pending bit. */
#define FRAME_PENDING_BIT (1 << 4)

/** Byte containing the frame type bits (+1 for the frame length byte). */
#define FRAME_TYPE_OFFSET 1
/** Mask of bits containing the frame type. */
#define FRAME_TYPE_MASK 0x07
/** Bits containing the ACK frame type. */
#define FRAME_TYPE_ACK 0x02
/** Bits containing the Beacon frame type. */
#define FRAME_TYPE_BEACON 0x00
/** Bits containing the Command frame type. */
#define FRAME_TYPE_COMMAND 0x03
/** Bits containing the Data frame type. */
#define FRAME_TYPE_DATA 0x01
/** Bits containing the Extended frame type. */
#define FRAME_TYPE_EXTENDED 0x07
/** Bits containing the Fragment or the Frak frame type. */
#define FRAME_TYPE_FRAGMENT 0x06
/** Bits containing the Multipurpose frame type. */
#define FRAME_TYPE_MULTIPURPOSE 0x05

/** Byte containing the frame version bits (+1 for the frame length byte). */
#define FRAME_VERSION_OFFSET 2
/** Mask of bits containing the frame version. */
#define FRAME_VERSION_MASK 0x30
/** Bits containing the frame version 0b00. */
#define FRAME_VERSION_0 0x00
/** Bits containing the frame version 0b01. */
#define FRAME_VERSION_1 0x10
/** Bits containing the frame version 0b10. */
#define FRAME_VERSION_2 0x20
/** Bits containing the frame version 0b11. */
#define FRAME_VERSION_3 0x30

/** Mask of bits containing the length of an IE header content. */
#define IE_HEADER_LENGTH_MASK 0x3f
/** Byte containing the IE Present bit. */
#define IE_PRESENT_OFFSET 2
/** Bits containing the IE Present field. */
#define IE_PRESENT_BIT 0x02

/** Mask of bits containing Key Identifier Mode in the
 * Security Control field.
 */
#define KEY_ID_MODE_MASK 0x18
/** Bits containing the 0x00 Key Identifier Mode. */
#define KEY_ID_MODE_0 0
/** Bits containing the 0x01 Key Identifier Mode. */
#define KEY_ID_MODE_1 0x08
/** Bits containing the 0x10 Key Identifier Mode. */
#define KEY_ID_MODE_2 0x10
/** Bits containing the 0x11 Key Identifier Mode. */
#define KEY_ID_MODE_3 0x18

/** Command frame identifier for MAC Association request. */
#define MAC_CMD_ASSOC_REQ 0x01
/** Command frame identifier for MAC Association response. */
#define MAC_CMD_ASSOC_RESP 0x02
/** Command frame identifier for MAC Disaccociation notification. */
#define MAC_CMD_DISASSOC_NOTIFY 0x03
/** Command frame identifier for MAC Data Requests. */
#define MAC_CMD_DATA_REQ 0x04
/** Command frame identifier for MAC PAN ID conflict notification. */
#define MAC_CMD_PANID_CONFLICT 0x05
/** Command frame identifier for MAC Orphan notification. */
#define MAC_CMD_ORPHAN_NOTIFY 0x06
/** Command frame identifier for MAC Beacon. */
#define MAC_CMD_BEACON_REQ 0x07
/** Command frame identifier for MAC Coordinator realignment. */
#define MAC_CMD_COORD_REALIGN 0x08
/** Command frame identifier for MAC GTS request. */
#define MAC_CMD_GTS_REQUEST 0x09

/** Byte containing the PAN ID compression bit
 * (+1 for the frame length byte).
 */
#define PAN_ID_COMPR_OFFSET 1
/** PAN ID compression bit. */
#define PAN_ID_COMPR_MASK 0x40

/** Offset of PAN ID in the Data frame (+1 for the frame length byte). */
#define PAN_ID_OFFSET 4

/** Offset of the PHY header in a frame. */
#define PHR_OFFSET 0

/** Byte containing the Security Enabled bit. */
#define SECURITY_ENABLED_OFFSET 1
/** Bits containing the Security Enabled field. */
#define SECURITY_ENABLED_BIT 0x08
/** Mask of bits containing the Security level field. */
#define SECURITY_LEVEL_MASK 0x07
/** Bits containing the 32-bit Message Integrity Code (0b001). */
#define SECURITY_LEVEL_MIC_32 0x01
/** Bits containing the 64-bit Message Integrity Code (0b010). */
#define SECURITY_LEVEL_MIC_64 0x02
/** Bits containing the 128-bit Message Integrity Code (0b011). */
#define SECURITY_LEVEL_MIC_128 0x03
/** Bits containing the 32-bit Encrypted Message Integrity Code (0b101). */
#define SECURITY_LEVEL_ENC_MIC_32 0x05
/** Bits containing the 64-bit Encrypted Message Integrity Code (0b110). */
#define SECURITY_LEVEL_ENC_MIC_64 0x06
/** Bits containing the 128-bit Encrypted Message Integrity Code (0b111). */
#define SECURITY_LEVEL_ENC_MIC_128 0x07

/** Bits containing the extended source address type. */
#define SRC_ADDR_TYPE_EXTENDED 0xc0
/** Bits containing a not-present source address type. */
#define SRC_ADDR_TYPE_NONE 0x00
/** Mask of bits containing the source address type. */
#define SRC_ADDR_TYPE_MASK 0xc0
/** Byte containing the source address type (+1 for the frame length byte). */
#define SRC_ADDR_TYPE_OFFSET 2
/** Bits containing the short source address type. */
#define SRC_ADDR_TYPE_SHORT 0x80

/** Offset of the source address in the Data frame if the destination address
 * is short.
 */
#define SRC_ADDR_OFFSET_SHORT_DST 8
/** Offset of the source address in the Data frame if the destination
 * address is extended.
 */
#define SRC_ADDR_OFFSET_EXTENDED_DST 14

/** Size of the Sequence Number field. */
#define DSN_SIZE 1
/** Size of the FCF field. */
#define FCF_SIZE 2
/** Size of the FCS field. */
#define FCS_SIZE 2
/** Size of the Frame Counter field. */
#define FRAME_COUNTER_SIZE 4
/** Size of the obligatory IE Header field elements, including the
 * header termination.
 */
#define IE_HEADER_SIZE 4
/** Length of the ACK frame. */
#define IMM_ACK_LENGTH 5
/** Size of the 0x01 Key Identifier Mode field. */
#define KEY_ID_MODE_1_SIZE 1
/** Size of the 0x10 Key Identifier Mode field. */
#define KEY_ID_MODE_2_SIZE 5
/** Size of the 0x11 Key Identifier Mode field. */
#define KEY_ID_MODE_3_SIZE 9
/** Maximum size of the radio packet. */
#define MAX_PACKET_SIZE 127
/** Size of MIC with the MIC-32 and ENC-MIC-32 security attributes. */
#define MIC_32_SIZE 4
/** Size of MIC with the MIC-64 and ENC-MIC-64 security attributes. */
#define MIC_64_SIZE 8
/** Size of MIC with the MIC-128 and ENC-MIC-128 security attributes. */
#define MIC_128_SIZE 16
/** Size of the PAN ID. */
#define PAN_ID_SIZE 2
/** Size of the PHR field. */
#define PHR_SIZE 1
/** Size of the Security Control field. */
#define SECURITY_CONTROL_SIZE 1

/** Size of the Extended Mac Address. */
#define EXTENDED_ADDRESS_SIZE 8
/** Size of the Short Mac Address. */
#define SHORT_ADDRESS_SIZE 2

/** RX-to-TX or TX-to-RX turnaround time (aTurnaroundTime),
 * in microseconds (us).
 */
#define TURNAROUND_TIME 192UL
/** Time required to perform CCA detection (aCcaTime), in microseconds (us). */
#define CCA_TIME 128UL
/** Number of symbols in the basic time period used by CSMA-CA algorithm
 * (aUnitBackoffPeriod), in (us).
 */
#define UNIT_BACKOFF_PERIOD (TURNAROUND_TIME + CCA_TIME)

/** Duration of a single symbol in microseconds (us). */
#define PHY_US_PER_SYMBOL 16
/** Number of symbols in a single byte (octet). */
#define PHY_SYMBOLS_PER_OCTET 2
/** Number of symbols in the Synchronization Header (SHR). */
#define PHY_SHR_SYMBOLS 10

/** dBm value corresponding to value 0 in the EDSAMPLE register. */
#define ED_MIN_DBM (-94)
/** Factor needed to calculate the ED result based on the data from the RADIO
 * peripheral.
 */
#define ED_RESULT_FACTOR 4
/** Maximal ED result. */
#define ED_RESULT_MAX 0xff

/** Broadcast short address. */
#define BROADCAST_ADDRESS ((uint8_t[SHORT_ADDRESS_SIZE]) { 0xff, 0xff })

typedef enum {
	REQ_ORIG_HIGHER_LAYER,
	REQ_ORIG_CORE,
	REQ_ORIG_RSCH,
#if NRF_802154_CSMA_CA_ENABLED
	REQ_ORIG_CSMA_CA,
#endif /* NRF_802154_CSMA_CA_ENABLED */
#if NRF_802154_ACK_TIMEOUT_ENABLED
	REQ_ORIG_ACK_TIMEOUT,
#endif /* NRF_802154_ACK_TIMEOUT_ENABLED */
#if NRF_802154_DELAYED_TRX_ENABLED
	REQ_ORIG_DELAYED_TRX,
#endif /* NRF_802154_DELAYED_TRX_ENABLED */
} req_originator_t;

#endif /* NRD_DRV_RADIO802154_CONST_H_ */
