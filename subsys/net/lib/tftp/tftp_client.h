/*
 * Copyright (c) 2020 InnBlue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Generic header files required by the TFTPC Module. */
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tftp.h>

/* Defines for creating static arrays for TFTP communication. */
#define TFTP_MAX_MODE_SIZE       8
#define TFTP_REQ_RETX            CONFIG_TFTPC_REQUEST_RETRANSMITS

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

/* Error Codes */

/** Not defined, see error message (if any). */
#define TFTP_ERROR_UNDEF               0
/** File not found. */
#define TFTP_ERROR_NO_FILE             1
/** Access violation. */
#define TFTP_ERROR_ACCESS              2
/** Disk full or allocation exceeded. */
#define TFTP_ERROR_DISK_FULL           3
/** Illegal TFTP operation. */
#define TFTP_ERROR_ILLEGAL_OP          4
/** Unknown transfer ID. */
#define TFTP_ERROR_UNKNOWN_TRANSFER_ID 5
/** File already exists. */
#define TFTP_ERROR_FILE_EXISTS         6
/** No such user. */
#define TFTP_ERROR_NO_USER             7

struct tftphdr_ack {
	uint16_t opcode;
	uint16_t block;
};
