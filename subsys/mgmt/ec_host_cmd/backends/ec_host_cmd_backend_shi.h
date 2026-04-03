/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_MGMT_EC_HOST_CMD_BACKENDS_EC_HOST_CMD_BACKEND_SHI_H_
#define ZEPHYR_SUBSYS_MGMT_EC_HOST_CMD_BACKENDS_EC_HOST_CMD_BACKEND_SHI_H_

#include <zephyr/device.h>

/*
 * Byte codes returned by EC over SPI interface.
 *
 * These can be used by the AP to debug the EC interface, and to determine
 * when the EC is not in a state where it will ever get around to responding
 * to the AP.
 *
 * Example of sequence of bytes read from EC for a current good transfer:
 *   1. -                  - AP asserts chip select (CS#)
 *   2. EC_SHI_OLD_READY   - AP sends first byte(s) of request
 *   3. -                  - EC starts handling CS# interrupt
 *   4. EC_SHI_RECEIVING   - AP sends remaining byte(s) of request
 *   5. EC_SHI_PROCESSING  - EC starts processing request; AP is clocking in
 *                           bytes looking for EC_SHI_FRAME_START
 *   6. -                  - EC finishes processing and sets up response
 *   7. EC_SHI_FRAME_START - AP reads frame byte
 *   8. (response packet)  - AP reads response packet
 *   9. EC_SHI_PAST_END    - Any additional bytes read by AP
 *   10 -                  - AP deasserts chip select
 *   11 -                  - EC processes CS# interrupt and sets up DMA for
 *                           next request
 *
 * If the AP is waiting for EC_SHI_FRAME_START and sees any value other than
 * the following byte values:
 *   EC_SHI_OLD_READY
 *   EC_SHI_RX_READY
 *   EC_SHI_RECEIVING
 *   EC_SHI_PROCESSING
 *
 * Then the EC found an error in the request, or was not ready for the request
 * and lost data.  The AP should give up waiting for EC_SHI_FRAME_START,
 * because the EC is unable to tell when the AP is done sending its request.
 */

/*
 * Framing byte which precedes a response packet from the EC.  After sending a
 * request, the AP will clock in bytes until it sees the framing byte, then
 * clock in the response packet.
 */
#define EC_SHI_FRAME_START 0xec

/*
 * Padding bytes which are clocked out after the end of a response packet.
 */
#define EC_SHI_PAST_END 0xed

/*
 * EC is ready to receive, and has ignored the byte sent by the AP. EC expects
 * that the AP will send a valid packet header (starting with
 * EC_COMMAND_PROTOCOL_3) in the next 32 bytes.
 *
 * NOTE: Some SPI configurations place the Most Significant Bit on SDO when
 *	 CS goes low. This macro has the Most Significant Bit set to zero,
 *	 so SDO will not be driven high when CS goes low.
 */
#define EC_SHI_RX_READY 0x78

/*
 * EC has started receiving the request from the AP, but hasn't started
 * processing it yet.
 */
#define EC_SHI_RECEIVING 0xf9

/* EC has received the entire request from the AP and is processing it. */
#define EC_SHI_PROCESSING 0xfa

/*
 * EC received bad data from the AP, such as a packet header with an invalid
 * length.  EC will ignore all data until chip select deasserts.
 */
#define EC_SHI_RX_BAD_DATA 0xfb

/*
 * EC received data from the AP before it was ready.  That is, the AP asserted
 * chip select and started clocking data before the EC was ready to receive it.
 * EC will ignore all data until chip select deasserts.
 */
#define EC_SHI_NOT_READY 0xfc

/*
 * EC was ready to receive a request from the AP.  EC has treated the byte sent
 * by the AP as part of a request packet, or (for old-style ECs) is processing
 * a fully received packet but is not ready to respond yet.
 */
#define EC_SHI_OLD_READY 0xfd

/* Supported version of host commands protocol. */
#define EC_HOST_REQUEST_VERSION 3

#endif /* ZEPHYR_SUBSYS_MGMT_EC_HOST_CMD_BACKENDS_EC_HOST_CMD_BACKEND_SHI_H_ */
