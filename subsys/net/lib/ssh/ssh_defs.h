/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Definitions and enums from SSH RFCs */

#ifndef ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_DEFS_H_
#define ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_DEFS_H_

#define SSH_IDENTITY_MAX_LEN 255
#define SSH_MIN_PACKET_LEN 16
#define SSH_MAX_PACKET_LEN 35000

#define SSH_PKT_LEN_OFFSET 0
#define SSH_PKT_LEN_SIZE 4
#define SSH_PKT_PADDING_OFFSET (SSH_PKT_LEN_OFFSET + SSH_PKT_LEN_SIZE)
#define SSH_PKT_PADDING_SIZE 1
#define SSH_PKT_MSG_ID_OFFSET (SSH_PKT_PADDING_OFFSET + SSH_PKT_PADDING_SIZE)
#define SSH_PKT_MSG_ID_SIZE 1

#define SSH_EXTENDED_DATA_STDERR 1

enum ssh_msg_id {
	SSH_MSG_DISCONNECT                = 1,
	SSH_MSG_IGNORE                    = 2,
	SSH_MSG_UNIMPLEMENTED             = 3,
	SSH_MSG_DEBUG                     = 4,
	SSH_MSG_SERVICE_REQUEST           = 5,
	SSH_MSG_SERVICE_ACCEPT            = 6,
	SSH_MSG_EXT_INFO                  = 7, /* RFC8308 extensions */
	SSH_MSG_KEXINIT                   = 20,
	SSH_MSG_NEWKEYS                   = 21,
	/* 30 to 49 are specific to the key exchange method in use */
	SSH_MSG_USERAUTH_REQUEST          = 50,
	SSH_MSG_USERAUTH_FAILURE          = 51,
	SSH_MSG_USERAUTH_SUCCESS          = 52,
	SSH_MSG_USERAUTH_BANNER           = 53,
	SSH_MSG_USERAUTH_PK_OK            = 60,
	SSH_MSG_USERAUTH_PASSWD_CHANGEREQ = 60,
	SSH_MSG_GLOBAL_REQUEST            = 80,
	SSH_MSG_REQUEST_SUCCESS           = 81,
	SSH_MSG_REQUEST_FAILURE           = 82,
	SSH_MSG_CHANNEL_OPEN              = 90,
	SSH_MSG_CHANNEL_OPEN_CONFIRMATION = 91,
	SSH_MSG_CHANNEL_OPEN_FAILURE      = 92,
	SSH_MSG_CHANNEL_WINDOW_ADJUST     = 93,
	SSH_MSG_CHANNEL_DATA              = 94,
	SSH_MSG_CHANNEL_EXTENDED_DATA     = 95,
	SSH_MSG_CHANNEL_EOF               = 96,
	SSH_MSG_CHANNEL_CLOSE             = 97,
	SSH_MSG_CHANNEL_REQUEST           = 98,
	SSH_MSG_CHANNEL_SUCCESS           = 99,
	SSH_MSG_CHANNEL_FAILURE           = 100
};

enum ssh_msg_kex_rsa_id {
	SSH_MSG_KEX_RSA_PUBKEY = 30,
	SSH_MSG_KEX_RSA_SECRET = 31,
	SSH_MSG_KEX_RSA_DONE   = 32
};

enum ssh_msg_kex_ecdh_id {
	SSH_MSG_KEX_ECDH_INIT  = 30,
	SSH_MSG_KEX_ECDH_REPLY = 31
};

#endif
