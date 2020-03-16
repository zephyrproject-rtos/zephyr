/*
 * Copyright (c) 2020 InnBlue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Generic header files required by the TFTPC Module. */
#include <zephyr.h>
#include <errno.h>
#include <net/socket.h>

/* Defines for creating static arrays for TFTP communication. */
#define TFTP_MAX_FILENAME_SIZE   CONFIG_TFTPC_MAX_FILENAME_SIZE
#define TFTP_HEADER_SIZE         4
#define TFTP_BLOCK_SIZE          512
#define TFTP_MAX_MODE_SIZE       8
#define TFTP_REQ_RETX            CONFIG_TFTPC_REQUEST_RETRANSMITS

/* Maximum amount of data that can be received in a single go ! */
#define TFTPC_MAX_BUF_SIZE       (TFTP_BLOCK_SIZE + TFTP_HEADER_SIZE)

/* TFTP Default Server IP / Port. */
#define TFTPC_DEF_SERVER_IP      CONFIG_TFTPC_DEFAULT_SERVER_IP
#define TFTPC_DEF_SERVER_PORT    CONFIG_TFTPC_DEFAULT_SERVER_PORT

/* TFTP Opcodes. */
#define RRQ_OPCODE               0x1
#define WRQ_OPCODE               0x2
#define DATA_OPCODE              0x3
#define ACK_OPCODE               0x4
#define ERROR_OPCODE             0x5

/* TFTP Client Success / Error Codes. */
#define TFTPC_DATA_RX_SUCCESS     1
#define TFTPC_OP_COMPLETED        2
#define TFTPC_DUPLICATE_DATA     -1
#define TFTPC_BUFFER_OVERFLOW    -2
#define TFTPC_UNKNOWN_FAILURE    -3

/* Name: extract_u16
 * Description: This function gets u16_t buffer into a
 * u8_t buffer array.
 */
static inline u16_t extract_u16(u8_t *buf)
{
	/* Return it to the caller. */
	return ((buf[0] << 8) | buf[1]);
}

/* Name: insert_u16
 * Description: This function fills the provided u16_t
 * buffer into a u8_t buffer array.
 */
static inline void insert_u16(u8_t *buf, u16_t data)
{
	buf[0] = (data >> 8) & 0xFF;
	buf[1] = (data >> 0) & 0xFF;
}

/* Name: send_ack
 * Description: This function sends an Ack to the TFTP
 * Server (in response to the data sent by the
 * Server).
 */
static inline int send_ack(int sock, int block)
{
	u8_t tmp[4];

	LOG_INF("Client acking Block Number: %d", block);

	/* Fill in the "Ack" Opcode and the block no. */
	insert_u16(tmp, ACK_OPCODE);
	insert_u16(tmp + 2, block);

	/* Lets send this request buffer out. Size of request
	 * buffer is 4 bytes.
	 */
	return send(sock, tmp, 4, 0);
}
