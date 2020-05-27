/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Telnet console protocol specific defines
 *
 *
 * This defines the Telnet codes, all prefixed as NVT_
 * (NVT: Network Virtual Terminal, see rfc854)
 */

#ifndef SHELL_TELNET_PROTOCOL_H__
#define SHELL_TELNET_PROTOCOL_H__

/** Printer/Keyboard codes */

/* Mandatory ones */
#define NVT_NUL				0
#define NVT_LF				10
#define NVT_CR				13

/* Optional ones */
#define NVT_BEL				7
#define NVT_BS				8
#define NVT_HT				9
#define NVT_VT				11
#define NVT_FF				12

/* Telnet commands */
#define NVT_CMD_SE			240
#define NVT_CMD_NOP			241
#define NVT_CMD_DM			242
#define NVT_CMD_BRK			243
#define NVT_CMD_IP			244
#define NVT_CMD_AO			245
#define NVT_CMD_AYT			246
#define NVT_CMD_EC			247
#define NVT_CMD_EL			248
#define NVT_CMD_GA			249
#define NVT_CMD_SB			250
#define NVT_CMD_WILL			251
#define NVT_CMD_WONT			252
#define NVT_CMD_DO			253
#define NVT_CMD_DONT			254
#define NVT_CMD_IAC			255

/* Telnet options */
#define NVT_OPT_TX_BIN			0
#define NVT_OPT_ECHO			1
#define NVT_OPT_RECONNECT		2
#define NVT_OPT_SUPR_GA			3
#define NVT_OPT_MSG_SZ_NEG		4
#define NVT_OPT_STATUS			5
#define NVT_OPT_TIMING_MARK		6
#define NVT_OPT_REMOTE_CTRL_TRANS_ECHO	7
#define NVT_OPT_OUT_LINE_WIDTH		8
#define NVT_OPT_OUT_PAGE_SZ		9
#define NVT_OPT_NEG_CR			10
#define NVT_OPT_NEG_HT			11
#define NVT_OPT_NAOHTD			12
#define NVT_OPT_NEG_OUT_FF		13
#define NVT_OPT_NEG_VT			14
#define NVT_OPT_NEG_OUT_VT		15
#define NVT_OPT_NET_OUT_LF		16
#define NVT_OPT_EXT_ASCII		17
#define NVT_OPT_LOGOUT			18
#define NVT_OPT_BYTE_MACRO		19
#define NVT_OPT_DATA_ENTRY		20
#define NVT_OPT_SUPDUP			21
#define NVT_OPT_SUPDUP_OUT		22
#define NVT_OPT_SEND_LOC		23
#define NVT_OPT_TERM_TYPE		24
#define NVT_OPT_EOR			25
#define NVT_OPT_TACACS_UID		26
#define NVT_OPT_OUT_MARK		27
#define NVT_OPT_TTYLOC			28
#define NVT_OPT_3270			29
#define NVT_OPT_X_3_PAD			30
#define NVT_OPT_NAWS			31
#define NVT_OPT_TERM_SPEED		32
#define NVT_OPT_REMOTE_FC		33
#define NVT_OPT_LINEMODE		34
#define NVT_OPT_X_LOC			35
#define NVT_OPT_ENV			36
#define NVT_OPT_AUTH			37
#define NVT_OPT_ENCRYPT_OPT		38
#define NVT_OPT_NEW_ENV			39
#define NVT_OPT_TN3270E			40
#define NVT_OPT_XAUTH			41
#define NVT_OPT_CHARSET			42
#define NVT_OPT_RSP			43
#define NVT_OPT_COM_PORT_CTRL		44
#define NVT_OPT_SUPR_LOCAL_ECHO		45
#define NVT_OPT_START_TLS		46
#define NVT_OPT_KERMIT			47
#define NVT_OPT_SEND_URL		48
#define NVT_OPT_FORWARD_X		49
#define NVT_OPT_PRAGMA_LOGON		138
#define NVT_OPT_SSPI_LOGON		139
#define NVT_OPT_PRAGMA_HB		140
#define NVT_OPT_EXT_OPT_LIST		255

/** Describes a telnet command */
struct telnet_simple_command {
	/** Mandatory IAC code */
	uint8_t iac;
	/** Type of operation (see Telnet commands above) */
	uint8_t op;
	/** Option code */
	uint8_t opt;
};

static inline void telnet_command_cpy(struct telnet_simple_command *dst,
				      struct telnet_simple_command *src)
{
	dst->iac = src->iac;
	dst->op  = src->op;
	dst->opt = src->opt;
}

#endif /* SHELL_TELNET_PROTOCOL_H__ */
