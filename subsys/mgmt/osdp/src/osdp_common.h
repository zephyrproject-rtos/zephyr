/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSDP_COMMON_H_
#define _OSDP_COMMON_H_

#include <zephyr/mgmt/osdp.h>
#include <zephyr/sys/__assert.h>

#define OSDP_RESP_TOUT_MS              (200)

#define OSDP_QUEUE_SLAB_SIZE \
	(sizeof(union osdp_ephemeral_data) * CONFIG_OSDP_PD_COMMAND_QUEUE_SIZE)

#define ISSET_FLAG(p, f)               (((p)->flags & (f)) == (f))
#define SET_FLAG(p, f)                 ((p)->flags |= (f))
#define CLEAR_FLAG(p, f)               ((p)->flags &= ~(f))

#define BYTE_0(x)                      (uint8_t)(((x) >>  0) & 0xFF)
#define BYTE_1(x)                      (uint8_t)(((x) >>  8) & 0xFF)
#define BYTE_2(x)                      (uint8_t)(((x) >> 16) & 0xFF)
#define BYTE_3(x)                      (uint8_t)(((x) >> 24) & 0xFF)

#define GET_CURRENT_PD(p)              ((p)->current_pd)
#define SET_CURRENT_PD(p, i)                                    \
	do {                                                    \
		(p)->current_pd = osdp_to_pd(p, i);             \
	} while (0)
#define PD_MASK(ctx) \
	(uint32_t)((1 << ((ctx)->num_pd)) - 1)
#define AES_PAD_LEN(x)                 ((x + 16 - 1) & (~(16 - 1)))
#define NUM_PD(ctx)                    ((ctx)->num_pd)

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
#define PD_FLAG_AWAIT_RESP      0x00000020 /* set after command is sent */
#define PD_FLAG_SKIP_SEQ_CHECK  0x00000040 /* disable seq checks (debug) */
#define PD_FLAG_SC_USE_SCBKD    0x00000080 /* in this SC attempt, use SCBKD */
#define PD_FLAG_SC_ACTIVE       0x00000100 /* secure channel is active */
#define PD_FLAG_SC_SCBKD_DONE   0x00000200 /* SCBKD check is done */
#define PD_FLAG_PKT_HAS_MARK    0x00000400 /* Packet has mark byte */
#define PD_FLAG_PKT_SKIP_MARK   0x00000800 /* CONFIG_OSDP_SKIP_MARK_BYTE */
#define PD_FLAG_INSTALL_MODE    0x40000000 /* PD is in install mode */
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

enum osdp_cp_phy_state_e {
	OSDP_CP_PHY_STATE_IDLE,
	OSDP_CP_PHY_STATE_SEND_CMD,
	OSDP_CP_PHY_STATE_REPLY_WAIT,
	OSDP_CP_PHY_STATE_WAIT,
	OSDP_CP_PHY_STATE_ERR,
	OSDP_CP_PHY_STATE_ERR_WAIT,
	OSDP_CP_PHY_STATE_CLEANUP,
};

enum osdp_cp_state_e {
	OSDP_CP_STATE_INIT,
	OSDP_CP_STATE_IDREQ,
	OSDP_CP_STATE_CAPDET,
	OSDP_CP_STATE_SC_INIT,
	OSDP_CP_STATE_SC_CHLNG,
	OSDP_CP_STATE_SC_SCRYPT,
	OSDP_CP_STATE_SET_SCBK,
	OSDP_CP_STATE_ONLINE,
	OSDP_CP_STATE_OFFLINE
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

/* Unused type only to estimate ephemeral_data size */
union osdp_ephemeral_data {
	struct osdp_cmd cmd;
	struct osdp_event event;
};
#define OSDP_EPHEMERAL_DATA_MAX_LEN sizeof(union osdp_ephemeral_data)

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

struct osdp_queue {
	sys_slist_t queue;
	struct k_mem_slab slab;
	uint8_t slab_buf[OSDP_QUEUE_SLAB_SIZE];
};

#ifdef CONFIG_OSDP_SC_ENABLED
struct osdp_secure_channel {
	uint8_t scbk[16];
	uint8_t s_enc[16];
	uint8_t s_mac1[16];
	uint8_t s_mac2[16];
	uint8_t r_mac[16];
	uint8_t c_mac[16];
	uint8_t cp_random[8];
	uint8_t pd_random[8];
	uint8_t pd_client_uid[8];
	uint8_t cp_cryptogram[16];
	uint8_t pd_cryptogram[16];
};
#endif

struct osdp_pd {
	void *osdp_ctx;
	int idx;
	uint32_t flags;

	/* OSDP specified data */
	int baud_rate;
	int address;
	int seq_number;
	struct osdp_pd_cap cap[OSDP_PD_CAP_SENTINEL];
	struct osdp_pd_id id;

	/* PD state management */
#ifdef CONFIG_OSDP_MODE_PD
	enum osdp_pd_state_e state;
#else
	enum osdp_cp_state_e state;
	enum osdp_cp_phy_state_e phy_state;
	int64_t phy_tstamp;
#endif
	int64_t tstamp;
	uint8_t rx_buf[CONFIG_OSDP_UART_BUFFER_LENGTH];
	int rx_buf_len;

	int cmd_id;
	int reply_id;
	uint8_t ephemeral_data[OSDP_EPHEMERAL_DATA_MAX_LEN];

	struct osdp_channel channel;

	union {
		struct osdp_queue cmd;    /* Command queue (CP Mode only) */
		struct osdp_queue event;  /* Command queue (PD Mode only) */
	};

	/* PD command callback to app with opaque arg pointer as passed by app */
	void *command_callback_arg;
	pd_command_callback_t command_callback;

#ifdef CONFIG_OSDP_SC_ENABLED
	int64_t sc_tstamp;
	struct osdp_secure_channel sc;
#endif
};

struct osdp {
	int magic;
	uint32_t flags;
	int num_pd;
	struct osdp_pd *current_pd;	/* current operational pd's pointer */
	struct osdp_pd *pd;
#ifdef CONFIG_OSDP_SC_ENABLED
	uint8_t sc_master_key[16];
#endif
	/* CP event callback to app with opaque arg pointer as passed by app */
	void *event_callback_arg;
	cp_event_callback_t event_callback;
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

/* from osdp.c */
struct osdp *osdp_get_ctx();

/* from osdp_cp.c */
#ifdef CONFIG_OSDP_MODE_CP
int osdp_extract_address(int *address);
#endif

#ifdef CONFIG_OSDP_SC_ENABLED
void osdp_encrypt(uint8_t *key, uint8_t *iv, uint8_t *data, int len);
void osdp_decrypt(uint8_t *key, uint8_t *iv, uint8_t *data, int len);
#endif

/* from osdp_sc.c */
void osdp_compute_scbk(struct osdp_pd *pd, uint8_t *scbk);
void osdp_compute_session_keys(struct osdp_pd *pd);
void osdp_compute_cp_cryptogram(struct osdp_pd *pd);
int osdp_verify_cp_cryptogram(struct osdp_pd *pd);
void osdp_compute_pd_cryptogram(struct osdp_pd *pd);
int osdp_verify_pd_cryptogram(struct osdp_pd *pd);
void osdp_compute_rmac_i(struct osdp_pd *pd);
int osdp_decrypt_data(struct osdp_pd *pd, int is_cmd, uint8_t *data, int len);
int osdp_encrypt_data(struct osdp_pd *pd, int is_cmd, uint8_t *data, int len);
int osdp_compute_mac(struct osdp_pd *pd, int is_cmd,
		     const uint8_t *data, int len);
void osdp_sc_init(struct osdp_pd *pd);
void osdp_fill_random(uint8_t *buf, int len);

/* must be implemented by CP or PD */
int osdp_setup(struct osdp *ctx, uint8_t *key);
void osdp_update(struct osdp *ctx);

static inline struct osdp *pd_to_osdp(struct osdp_pd *pd)
{
	return pd->osdp_ctx;
}

static inline struct osdp_pd *osdp_to_pd(struct osdp *ctx, int pd_idx)
{
	return ctx->pd + pd_idx;
}

static inline bool is_pd_mode(struct  osdp_pd *pd)
{
	return ISSET_FLAG(pd, PD_FLAG_PD_MODE);
}

static inline bool is_cp_mode(struct  osdp_pd *pd)
{
	return !ISSET_FLAG(pd, PD_FLAG_PD_MODE);
}

static inline bool sc_is_capable(struct osdp_pd *pd)
{
	return ISSET_FLAG(pd, PD_FLAG_SC_CAPABLE);
}

static inline bool sc_is_active(struct osdp_pd *pd)
{
	return ISSET_FLAG(pd, PD_FLAG_SC_ACTIVE);
}

static inline void sc_activate(struct osdp_pd *pd)
{
	SET_FLAG(pd, PD_FLAG_SC_ACTIVE);
}

static inline void sc_deactivate(struct osdp_pd *pd)
{
	CLEAR_FLAG(pd, PD_FLAG_SC_ACTIVE);
}

#endif	/* _OSDP_COMMON_H_ */
