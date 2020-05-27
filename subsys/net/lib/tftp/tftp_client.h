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
#define TFTP_HEADER_SIZE         4
#define TFTP_BLOCK_SIZE          512
#define TFTP_MAX_MODE_SIZE       8
#define TFTP_REQ_RETX            CONFIG_TFTPC_REQUEST_RETRANSMITS

/* Maximum amount of data that can be received in a single go ! */
#define TFTPC_MAX_BUF_SIZE       (TFTP_BLOCK_SIZE + TFTP_HEADER_SIZE)

/* Maximum filename size allowed by the TFTP Client. This is used as
 * an upper bound in the "make_request" function to ensure that there
 * are no buffer overflows. The complete "tftpc_buffer" has the size of
 * "TFTPC_MAX_BUF_SIZE" which is used when creating a request. From this
 * total size, we need 2 bytes for request info, 2 dummy NULL bytes and
 * "TFTP_MAX_MODE_SIZE" for mode info. Everything else can be used for
 * filename.
 */
#define TFTP_MAX_FILENAME_SIZE   (TFTPC_MAX_BUF_SIZE - TFTP_MAX_MODE_SIZE - 4)

/* TFTP Opcodes. */
#define READ_REQUEST             0x1
#define WRITE_REQUEST            0x2
#define DATA_OPCODE              0x3
#define ACK_OPCODE               0x4
#define ERROR_OPCODE             0x5

#define RECV_DATA_SIZE()         (tftpc_buffer_size - TFTP_HEADER_SIZE)

/* Name: send_ack
 * Description: This function sends an Ack to the TFTP
 * Server (in response to the data sent by the
 * Server).
 */
static inline int send_ack(int sock, int block)
{
	uint8_t tmp[4];

	LOG_INF("Client acking Block Number: %d", block);

	/* Fill in the "Ack" Opcode and the block no. */
	sys_put_be16(ACK_OPCODE, tmp);
	sys_put_be16(block, tmp + 2);

	/* Lets send this request buffer out. Size of request
	 * buffer is 4 bytes.
	 */
	return send(sock, tmp, 4, 0);
}
