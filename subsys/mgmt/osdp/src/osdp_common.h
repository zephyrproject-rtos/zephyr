/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDP_COMMON_H_
#define _OSDP_COMMON_H_

#include <mgmt/osdp.h>
#include <assert.h>

#define OSDP_RESP_TOUT_MS              (200)

#define OSDP_CMD_SLAB_BUF_SIZE \
	(sizeof(struct osdp_cmd) * CONFIG_OSDP_PD_COMMAND_QUEUE_SIZE)

#define ISSET_FLAG(p, f)               (((p)->flags & (f)) == (f))
#define SET_FLAG(p, f)                 ((p)->flags |= (f))
#define CLEAR_FLAG(p, f)               ((p)->flags &= ~(f))

#define BYTE_0(x)                      (uint8_t)(((x) >>  0) & 0xFF)
#define BYTE_1(x)                      (uint8_t)(((x) >>  8) & 0xFF)
#define BYTE_2(x)                      (uint8_t)(((x) >> 16) & 0xFF)
#define BYTE_3(x)                      (uint8_t)(((x) >> 24) & 0xFF)

/* casting helpers */
#define TO_OSDP(p)                     ((struct osdp *)p)
#define TO_CP(p)                       (((struct osdp *)(p))->cp)
#define TO_PD(p, i)                    (((struct osdp *)(p))->pd + i)
#define TO_CTX(p)                      ((struct osdp *)p->__parent)

#define GET_CURRENT_PD(p)              (TO_CP(p)->current_pd)
#define SET_CURRENT_PD(p, i)                                    \
	do {                                                    \
		TO_CP(p)->current_pd = TO_PD(p, i);             \
		TO_CP(p)->pd_offset = i;                        \
	} while (0)
#define PD_MASK(ctx) \
	(uint32_t)((1 << (TO_CP(ctx)->num_pd + 1)) - 1)
#define AES_PAD_LEN(x)                 ((x + 16 - 1) & (~(16 - 1)))
#define NUM_PD(ctx)                    (TO_CP(ctx)->num_pd)
#define OSDP_COMMAND_DATA_MAX_LEN      sizeof(struct osdp_cmd)

/**
 * @brief OSDP reserved commands
 */
#define CMD_POLL                0x60
#define CMD_ID                  0x61
#define CMD_CAP                 0x62
#define CMD_DIAG                0x63
#define CMD_LSTAT               0x64
#define CMD_ISTAT               0x65
#define CMD_OSTAT               0x66
#define CMD_RSTAT               0x67
#define CMD_OUT                 0x68
#define CMD_LED                 0x69
#define CMD_BUZ                 0x6A
#define CMD_TEXT                0x6B
#define CMD_RMODE               0x6C
#define CMD_TDSET               0x6D
#define CMD_COMSET              0x6E
#define CMD_DATA                0x6F
#define CMD_XMIT                0x70
#define CMD_PROMPT              0x71
#define CMD_SPE                 0x72
#define CMD_BIOREAD             0x73
#define CMD_BIOMATCH            0x74
#define CMD_KEYSET              0x75
#define CMD_CHLNG               0x76
#define CMD_SCRYPT              0x77
#define CMD_CONT                0x79
#define CMD_ABORT               0x7A
#define CMD_MAXREPLY            0x7B
#define CMD_MFG                 0x80
#define CMD_SCDONE              0xA0
#define CMD_XWR                 0xA1

/**
 * @brief OSDP reserved responses
 */
#define REPLY_ACK               0x40
#define REPLY_NAK               0x41
#define REPLY_PDID              0x45
#define REPLY_PDCAP             0x46
#define REPLY_LSTATR            0x48
#define REPLY_ISTATR            0x49
#define REPLY_OSTATR            0x4A
#define REPLY_RSTATR            0x4B
#define REPLY_RAW               0x50
#define REPLY_FMT               0x51
#define REPLY_PRES              0x52
#define REPLY_KEYPPAD           0x53
#define REPLY_COM               0x54
#define REPLY_SCREP             0x55
#define REPLY_SPER              0x56
#define REPLY_BIOREADR          0x57
#define REPLY_BIOMATCHR         0x58
#define REPLY_CCRYPT            0x76
#define REPLY_RMAC_I            0x78
#define REPLY_MFGREP            0x90
#define REPLY_BUSY              0x79
#define REPLY_XRD               0xB1

/**
 * @brief secure block types
 */
#define SCS_11                  0x11    /* CP -> PD -- CMD_CHLNG */
#define SCS_12                  0x12    /* PD -> CP -- REPLY_CCRYPT */
#define SCS_13                  0x13    /* CP -> PD -- CMD_SCRYPT */
#define SCS_14                  0x14    /* PD -> CP -- REPLY_RMAC_I */

#define SCS_15                  0x15    /* CP -> PD -- packets w MAC w/o ENC */
#define SCS_16                  0x16    /* PD -> CP -- packets w MAC w/o ENC */
#define SCS_17                  0x17    /* CP -> PD -- packets w MAC w ENC*/
#define SCS_18                  0x18    /* PD -> CP -- packets w MAC w ENC*/

/* PD Flags */
#define PD_FLAG_SC_CAPABLE      0x00000001 /* PD secure channel capable */
#define PD_FLAG_TAMPER          0x00000002 /* local tamper status */
#define PD_FLAG_POWER           0x00000004 /* local power status */
#define PD_FLAG_R_TAMPER        0x00000008 /* remote tamper status */
#define PD_FLAG_SKIP_SEQ_CHECK  0x00000040 /* disable seq checks (debug) */
#define PD_FLAG_SC_USE_SCBKD    0x00000080 /* in this SC attempt, use SCBKD */
#define PD_FLAG_SC_ACTIVE       0x00000100 /* secure channel is active */
#define PD_FLAG_PD_MODE         0x80000000 /* device is setup as PD */

enum osdp_pd_nak_code_e {
	/**
	 * @brief Dummy
	 */
	OSDP_PD_NAK_NONE,
	/**
	 * @brief Message check character(s) error (bad cksum/crc)
	 */
	OSDP_PD_NAK_MSG_CHK,
	/**
	 * @brief Command length error
	 */
	OSDP_PD_NAK_CMD_LEN,
	/**
	 * @brief Unknown Command Code – Command not implemented by PD
	 */
	OSDP_PD_NAK_CMD_UNKNOWN,
	/**
	 * @brief Unexpected sequence number detected in the header
	 */
	OSDP_PD_NAK_SEQ_NUM,
	/**
	 * @brief Unexpected sequence number detected in the header
	 */
	OSDP_PD_NAK_SC_UNSUP,
	/**
	 * @brief unsupported security block or security conditions not met
	 */
	OSDP_PD_NAK_SC_COND,
	/**
	 * @brief BIO_TYPE not supported
	 */
	OSDP_PD_NAK_BIO_TYPE,
	/**
	 * @brief BIO_FORMAT not supported
	 */
	OSDP_PD_NAK_BIO_FMT,
	/**
	 * @brief Unable to process command record
	 */
	OSDP_PD_NAK_RECORD,
	/**
	 * @brief Dummy
	 */
	OSDP_PD_NAK_SENTINEL
};

enum osdp_pd_state_e {
	OSDP_PD_STATE_IDLE,
	OSDP_PD_STATE_SEND_REPLY,
	OSDP_PD_STATE_ERR,
};

enum osdp_pkt_errors_e {
	OSDP_ERR_PKT_FMT   = -1,
	OSDP_ERR_PKT_WAIT  = -2,
	OSDP_ERR_PKT_SKIP  = -3
};

/**
 * @brief Various PD capability function codes.
 */
enum osdp_pd_cap_function_code_e {
	/**
	 * @brief Dummy.
	 */
	OSDP_PD_CAP_UNUSED,

	/**
	 * @brief This function indicates the ability to monitor the status of a
	 * switch using a two-wire electrical connection between the PD and the
	 * switch. The on/off position of the switch indicates the state of an
	 * external device.
	 *
	 * The PD may simply resolve all circuit states to an open/closed
	 * status, or it may implement supervision of the monitoring circuit.
	 * A supervised circuit is able to indicate circuit fault status in
	 * addition to open/closed status.
	 */
	OSDP_PD_CAP_CONTACT_STATUS_MONITORING,

	/**
	 * @brief This function provides a switched output, typically in the
	 * form of a relay. The Output has two states: active or inactive. The
	 * Control Panel (CP) can directly set the Output's state, or, if the PD
	 * supports timed operations, the CP can specify a time period for the
	 * activation of the Output.
	 */
	OSDP_PD_CAP_OUTPUT_CONTROL,

	/**
	 * @brief This capability indicates the form of the card data is
	 * presented to the Control Panel.
	 */
	OSDP_PD_CAP_CARD_DATA_FORMAT,

	/**
	 * @brief This capability indicates the presence of and type of LEDs.
	 */
	OSDP_PD_CAP_READER_LED_CONTROL,

	/**
	 * @brief This capability indicates the presence of and type of an
	 * Audible Annunciator (buzzer or similar tone generator)
	 */
	OSDP_PD_CAP_READER_AUDIBLE_OUTPUT,

	/**
	 * @brief This capability indicates that the PD supports a text display
	 * emulating character-based display terminals.
	 */
	OSDP_PD_CAP_READER_TEXT_OUTPUT,

	/**
	 * @brief This capability indicates that the type of date and time
	 * awareness or time keeping ability of the PD.
	 */
	OSDP_PD_CAP_TIME_KEEPING,

	/**
	 * @brief All PDs must be able to support the checksum mode. This
	 * capability indicates if the PD is capable of supporting CRC mode.
	 */
	OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT,

	/**
	 * @brief This capability indicates the extent to which the PD supports
	 * communication security (Secure Channel Communication)
	 */
	OSDP_PD_CAP_COMMUNICATION_SECURITY,

	/**
	 * @brief This capability indicates the maximum size single message the
	 * PD can receive.
	 */
	OSDP_PD_CAP_RECEIVE_BUFFERSIZE,

	/**
	 * @brief This capability indicates the maximum size multi-part message
	 * which the PD can handle.
	 */
	OSDP_PD_CAP_LARGEST_COMBINED_MESSAGE_SIZE,

	/**
	 * @brief This capability indicates whether the PD supports the
	 * transparent mode used for communicating directly with a smart card.
	 */
	OSDP_PD_CAP_SMART_CARD_SUPPORT,

	/**
	 * @brief This capability indicates the number of credential reader
	 * devices present. Compliance levels are bit fields to be assigned as
	 * needed.
	 */
	OSDP_PD_CAP_READERS,

	/**
	 * @brief This capability indicates the ability of the reader to handle
	 * biometric input
	 */
	OSDP_PD_CAP_BIOMETRICS,

	/**
	 * @brief Capability Sentinel
	 */
	OSDP_PD_CAP_SENTINEL
};

/**
 * @brief PD capability structure. Each PD capability has a 3 byte
 * representation.
 *
 * @param function_code One of enum osdp_pd_cap_function_code_e.
 * @param compliance_level A function_code dependent number that indicates what
 *                         the PD can do with this capability.
 * @param num_items Number of such capability entities in PD.
 */
struct osdp_pd_cap {
	uint8_t function_code;
	uint8_t compliance_level;
	uint8_t num_items;
};

/**
 * @brief PD ID information advertised by the PD.
 *
 * @param version 3-bytes IEEE assigned OUI
 * @param model 1-byte Manufacturer's model number
 * @param vendor_code 1-Byte Manufacturer's version number
 * @param serial_number 4-byte serial number for the PD
 * @param firmware_version 3-byte version (major, minor, build)
 */
struct osdp_pd_id {
	int version;
	int model;
	uint32_t vendor_code;
	uint32_t serial_number;
	uint32_t firmware_version;
};

struct osdp_channel {
	/**
	 * @brief pointer to a block of memory that will be passed to the
	 * send/receive method. This is optional and can be left empty.
	 */
	void *data;

	/**
	 * @brief pointer to function that copies received bytes into buffer
	 * @param data for use by underlying layers. channel_s::data is passed
	 * @param buf byte array copy incoming data
	 * @param len sizeof `buf`. Can copy utmost `len` bytes into `buf`
	 *
	 * @retval +ve: number of bytes copied on to `bug`. Must be <= `len`
	 * @retval -ve on errors
	 */
	int (*recv)(void *data, uint8_t *buf, int maxlen);

	/**
	 * @brief pointer to function that sends byte array into some channel
	 * @param data for use by underlying layers. channel_s::data is passed
	 * @param buf byte array to be sent
	 * @param len number of bytes in `buf`
	 *
	 * @retval +ve: number of bytes sent. must be <= `len`
	 * @retval -ve on errors
	 */
	int (*send)(void *data, uint8_t *buf, int len);

	/**
	 * @brief pointer to function that drops all bytes in TX/RX fifo
	 * @param data for use by underlying layers. channel_s::data is passed
	 */
	void (*flush)(void *data);
};

struct osdp_cmd_queue {
	sys_slist_t queue;
	struct k_mem_slab slab;
	uint8_t slab_buf[OSDP_CMD_SLAB_BUF_SIZE];
};

struct osdp_pd {
	void *__parent;
	int offset;
	uint32_t flags;

	/* OSDP specified data */
	int baud_rate;
	int address;
	int seq_number;
	struct osdp_pd_cap cap[OSDP_PD_CAP_SENTINEL];
	struct osdp_pd_id id;

	/* PD state management */
	enum osdp_pd_state_e pd_state;
	int64_t tstamp;
	int64_t sc_tstamp;
	int phy_state;
	uint8_t rx_buf[CONFIG_OSDP_UART_BUFFER_LENGTH];
	int rx_buf_len;
	int64_t phy_tstamp;

	int cmd_id;
	int reply_id;
	uint8_t cmd_data[OSDP_COMMAND_DATA_MAX_LEN];

	struct osdp_channel channel;
	struct osdp_cmd_queue cmd;
};

struct osdp_cp {
	void *__parent;
	uint32_t flags;

	int num_pd;

	struct osdp_pd *current_pd;	/* current operational pd's pointer */
	int pd_offset;			/* current pd's offset into ctx->pd */
};

struct osdp {
	int magic;
	uint32_t flags;

	uint8_t sc_master_key[16];
	struct osdp_cp *cp;
	struct osdp_pd *pd;
};

/* from osdp_phy.c */
int osdp_phy_packet_init(struct osdp_pd *p, uint8_t *buf, int max_len);
int osdp_phy_packet_finalize(struct osdp_pd *p, uint8_t *buf,
			       int len, int max_len);
int osdp_phy_decode_packet(struct osdp_pd *p, uint8_t *buf, int len);
void osdp_phy_state_reset(struct osdp_pd *pd);
int osdp_phy_packet_get_data_offset(struct osdp_pd *p, const uint8_t *buf);
uint8_t *osdp_phy_packet_get_smb(struct osdp_pd *p, const uint8_t *buf);

/* from osdp_common.c */
int64_t osdp_millis_now(void);
int64_t osdp_millis_since(int64_t last);
void osdp_dump(const char *head, uint8_t *buf, int len);
uint16_t osdp_compute_crc16(const uint8_t *buf, size_t len);
struct osdp_cmd *osdp_cmd_alloc(struct osdp_pd *pd);
void osdp_cmd_free(struct osdp_pd *pd, struct osdp_cmd *cmd);

/* from osdp.c */
struct osdp *osdp_get_ctx();

/* must be implemented by CP or PD */
int osdp_setup(struct osdp *ctx, struct osdp_channel *channel);
void osdp_update(struct osdp *ctx);

#endif	/* _OSDP_COMMON_H_ */
