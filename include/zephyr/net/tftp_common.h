/*
 * Copyright (c) 2026 Guy Shilman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file tftp_common.h
 *
 * @brief Common TFTP protocol definitions shared by client and server.
 */

#ifndef ZEPHYR_INCLUDE_NET_TFTP_COMMON_H_
#define ZEPHYR_INCLUDE_NET_TFTP_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Default TFTP port (RFC1350) */
#define TFTP_PORT 69

/** Maximum mode string size for requests (RFC1350) */
#define TFTP_MAX_MODE_SIZE 8

/** RFC1350 data block size in bytes. */
#define TFTP_BLOCK_SIZE 512

/** RFC1350 non-request message header size in bytes. */
#define TFTP_HEADER_SIZE 4

/** Maximum wire packet size for DATA/ACK/ERROR style packets. */
#define TFTP_MAX_PACKET_SIZE (TFTP_BLOCK_SIZE + TFTP_HEADER_SIZE)

/** Maximum filename size allowed by TFTP */
#define TFTP_MAX_FILENAME_SIZE (TFTP_MAX_PACKET_SIZE - TFTP_MAX_MODE_SIZE - 4)

/**
 * @defgroup tftp_opcodes TFTP Protocol Opcodes
 * @ingroup networking
 * @since 4.4
 * @version 0.1.0
 * @brief TFTP protocol operation codes as defined in RFC1350.
 * @{
 */

/** Read Request opcode - initiates a file download from server */
#define TFTP_OPCODE_RRQ   0x0001
/** Write Request opcode - initiates a file upload to server */
#define TFTP_OPCODE_WRQ   0x0002
/** Data opcode - carries file data blocks (1-512 bytes) */
#define TFTP_OPCODE_DATA  0x0003
/** Acknowledgment opcode - confirms receipt of data block */
#define TFTP_OPCODE_ACK   0x0004
/** Error opcode - indicates protocol or file system error */
#define TFTP_OPCODE_ERROR 0x0005
/** @} */

/**
 * @defgroup tftp_errors TFTP Protocol Error Codes
 * @ingroup networking
 * @since 4.4
 * @version 0.1.0
 * @brief TFTP protocol error codes as defined in RFC1350.
 * @{
 */

/** Not defined, see error message (RFC1350) */
#define TFTP_ERROR_UNDEF       0
/** File not found - requested file does not exist */
#define TFTP_ERROR_NO_FILE     1
/** Access violation - file permissions deny operation */
#define TFTP_ERROR_ACCESS      2
/** Disk full or allocation exceeded */
#define TFTP_ERROR_DISK_FULL   3
/** Illegal TFTP operation - invalid opcode or malformed packet */
#define TFTP_ERROR_ILLEGAL_OP  4
/** Unknown transfer ID - server doesn't recognize TID */
#define TFTP_ERROR_UNKNOWN_TID 5
/** File already exists - write request for existing file */
#define TFTP_ERROR_FILE_EXISTS 6
/** No such user - user authentication failed */
#define TFTP_ERROR_NO_USER     7
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_TFTP_COMMON_H_ */
