/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* LLCP Memory Pool Descriptor */
struct llcp_mem_pool {
	void *free;
	uint8_t *pool;
};

/* LLCP Procedure */
enum llcp_proc {
	PROC_UNKNOWN,
	PROC_LE_PING,
	PROC_FEATURE_EXCHANGE,
	PROC_MIN_USED_CHANS,
	PROC_VERSION_EXCHANGE,
	PROC_ENCRYPTION_START,
	PROC_ENCRYPTION_PAUSE,
	PROC_PHY_UPDATE,
	PROC_CONN_UPDATE,
	PROC_CONN_PARAM_REQ,
	PROC_TERMINATE,
	PROC_CHAN_MAP_UPDATE,
	PROC_DATA_LENGTH_UPDATE,
	PROC_CTE_REQ,
	PROC_CIS_CREATE,
	PROC_CIS_TERMINATE,
	PROC_SCA_UPDATE,
	/* A helper enum entry, to use in pause procedure context */
	PROC_NONE = 0x0,
};

/* Generic IDLE state to be used across all procedures
 * This allows a cheap procedure alloc/init handling
 */
enum llcp_proc_state_idle {
	LLCP_STATE_IDLE
};


enum llcp_tx_q_pause_data_mask {
	LLCP_TX_QUEUE_PAUSE_DATA_ENCRYPTION = 0x01,
	LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE = 0x02,
	LLCP_TX_QUEUE_PAUSE_DATA_DATA_LENGTH = 0x04,
	LLCP_TX_QUEUE_PAUSE_DATA_TERMINATE = 0x08,
};

#if ((CONFIG_BT_CTLR_LLCP_COMMON_TX_CTRL_BUF_NUM <\
			(CONFIG_BT_CTLR_LLCP_TX_PER_CONN_TX_CTRL_BUF_NUM_MAX *\
			CONFIG_BT_CTLR_LLCP_CONN)) &&\
			(CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM <\
			CONFIG_BT_CTLR_LLCP_TX_PER_CONN_TX_CTRL_BUF_NUM_MAX))
#define LLCP_TX_CTRL_BUF_QUEUE_ENABLE
#define LLCP_TX_CTRL_BUF_COUNT ((CONFIG_BT_CTLR_LLCP_COMMON_TX_CTRL_BUF_NUM  +\
			(CONFIG_BT_CTLR_LLCP_CONN * CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM)))
#else
#define LLCP_TX_CTRL_BUF_COUNT (CONFIG_BT_CTLR_LLCP_TX_PER_CONN_TX_CTRL_BUF_NUM_MAX *\
				CONFIG_BT_CTLR_LLCP_CONN)
#endif

#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
enum llcp_wait_reason {
	WAITING_FOR_NOTHING,
	WAITING_FOR_TX_BUFFER,
};
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */

#if defined(CONFIG_BT_CTLR_LE_ENC)
struct llcp_enc {
	uint8_t error;

	/* NOTE: To save memory, SKD(m|s) and IV(m|s) are
	 * generated just-in-time for PDU enqueuing and are
	 * therefore not present in this structure.
	 * Further candidates for optimizing memory layout.
	 *	* Overlay memory
	 *	* Repurpose memory used by lll.ccm_tx/rx?
	 */

	/* Central: Rand and EDIV are input copies from
	 * HCI that only live until the LL_ENC_REQ has
	 * been enqueued.
	 *
	 * Peripheral: Rand and EDIV are input copies from
	 * the LL_ENC_REQ that only live until host
	 * notification has been enqueued.
	 */

	/* 64 bit random number. */
	uint8_t rand[8];

	/* 16 bit encrypted diversifier.*/
	uint8_t ediv[2];

	/* 128 bit long term key. */
	uint8_t ltk[16];

	/* SKD is the concatenation of SKDm and SKDs and
	 * is used to calculate the session key, SK.
	 *
	 * Lifetime:
	 * M    : Generate SKDm and IVm
	 * M->S : LL_ENC_REQ(Rand, EDIV, SKDm, IVm)
	 * S	: Notify host (Rand, EDIV)
	 * S    : Generate SKDs and IVs
	 * S    : Calculate SK = e(LTK, SKD)
	 * M<-S : LL_ENC_RSP(SKDs, IVs)
	 * M    : Calculate SK = e(LTK, SKD)
	 *
	 * where security function e generates 128-bit
	 * encryptedData from a 128-bit key and 128-bit
	 * plaintextData using the AES-128-bit block
	 * cypher as defined in FIPS-1971:
	 *	encryptedData = e(key, plaintextData)
	 */
	union {

		/* 128-bit session key diversifier */
		uint8_t skd[16];
		struct {
			uint8_t skdm[8];
			uint8_t skds[8];
		};
	};
};
#endif /* CONFIG_BT_CTLR_LE_ENC */

/* LLCP Procedure Context */
struct proc_ctx {
	/* Must be the first for sys_slist to work */
	sys_snode_t node;

	/* llcp_mem_pool owner of this context */
	struct llcp_mem_pool *owner;

#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
	/* Wait list next pointer */
	sys_snode_t wait_node;
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */

	/* PROC_ */
	enum llcp_proc proc;

	enum pdu_data_llctrl_type response_opcode;

	/* Procedure FSM */
	uint8_t state;

	/* Expected opcode to be received next */
	enum pdu_data_llctrl_type rx_opcode;

	/* Greedy RX (used for central encryption) */
	uint8_t rx_greedy;

	/* Last transmitted opcode used for unknown/reject */
	enum pdu_data_llctrl_type tx_opcode;

	/*
	 * This flag is set to 1 when we are finished with the control
	 * procedure and it is safe to release the context ctx
	 */
	uint8_t done;

#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
	/* Procedure wait reason */
	enum llcp_wait_reason wait_reason;
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */

	struct {
		/* Rx node link element */
		memq_link_t *link;
		/* TX node awaiting ack */
		struct node_tx *tx_ack;
		/* most recent RX node */
		struct node_rx_pdu *rx;
		/* pre-allocated TX node */
		struct node_tx *tx;
	} node_ref;

	/* Procedure data */
	union {
		/* Feature Exchange Procedure */
		struct {
			uint8_t host_initiated:1;
		} fex;
		/* Used by Minimum Used Channels Procedure */
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
		struct {
			uint8_t phys;
			uint8_t min_used_chans;
		} muc;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */

#if defined(CONFIG_BT_CTLR_LE_ENC)
		/* Used by Encryption Procedure */
		struct llcp_enc enc;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_PHY)
		/* PHY Update */
		struct {
			uint8_t tx:3;
			uint8_t rx:3;
			uint8_t flags:1;
			uint8_t host_initiated:1;
			uint8_t ntf_pu:1;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
			uint8_t ntf_dle:1;
			struct node_rx_pdu *ntf_dle_node;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
			uint8_t error;
			uint16_t instant;
			uint8_t c_to_p_phy;
			uint8_t p_to_c_phy;
		} pu;
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		struct {
			uint8_t ntf_dle;
		} dle;
#endif

		/* Connection Update & Connection Parameter Request */
		struct {
			uint8_t error;
			uint8_t rejected_opcode;
			uint8_t params_changed;
			uint8_t win_size;
			uint16_t instant;
			uint16_t interval_min;
			uint16_t interval_max;
			uint16_t latency;
			uint16_t timeout;
			uint32_t win_offset_us;
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
			uint8_t  preferred_periodicity;
			uint16_t reference_conn_event_count;
			uint16_t offsets[6];
#endif /* defined(CONFIG_BT_CTLR_CONN_PARAM_REQ) */
		} cu;

		/* Use by ACL Termination Procedure */
		struct {
			uint8_t error_code;
		} term;

		/* Use by Channel Map Update Procedure */
		struct {
			uint16_t instant;
			uint8_t chm[5];
		} chmu;
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
		/* Use by CTE Request Procedure */
		struct {
			uint8_t type:2;
			uint8_t min_len:5;
		} cte_req;

		struct llcp_df_cte_remote_rsp {
			/* Storage for information that received LL_CTE_RSP PDU includes CTE */
			uint8_t has_cte;
		} cte_remote_rsp;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
		/* Use by CTE Response Procedure */
		struct llcp_df_cte_remote_req {
			uint8_t cte_type;
			uint8_t min_cte_len;
		} cte_remote_req;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */
		struct {
			uint8_t  error;
			uint16_t cis_handle;
			uint8_t  cig_id;
			uint8_t  cis_id;
			uint16_t conn_event_count;
			uint16_t iso_interval;
			uint32_t cis_offset_min;
			uint32_t cis_offset_max;
#if defined(CONFIG_BT_PERIPHERAL)
			uint32_t host_request_to;
#endif /* defined(CONFIG_BT_PERIPHERAL) */
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
			uint32_t cig_sync_delay;
			uint32_t cis_sync_delay;
			uint8_t  c_phy;
			uint8_t  p_phy;
			uint16_t c_max_sdu;
			uint16_t p_max_sdu;
			uint8_t  framed;
			uint32_t c_sdu_interval;
			uint32_t p_sdu_interval;
			uint8_t  nse;
			uint16_t c_max_pdu;
			uint16_t p_max_pdu;
			uint32_t sub_interval;
			uint8_t  p_bn;
			uint8_t  c_bn;
			uint8_t  c_ft;
			uint8_t  p_ft;
			uint8_t  aa[4];
#endif /* defined(CONFIG_BT_CTLR_CENTRAL_ISO) */
		} cis_create;

		struct {
			uint8_t  cig_id;
			uint8_t  cis_id;
			uint8_t error_code;
		} cis_term;
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
		struct {
			uint8_t sca;
			uint8_t error_code;
		} sca_update;
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */
	} data;

	struct {
		uint8_t type;
	} unknown_response;

	struct {
		uint8_t reject_opcode;
		uint8_t error_code;
	} reject_ext_ind;
};

/* Procedure Incompatibility */
enum proc_incompat {
	/* Local procedure has not sent first PDU */
	INCOMPAT_NO_COLLISION,

	/* Local incompatible procedure has sent first PDU */
	INCOMPAT_RESOLVABLE,

	/* Local incompatible procedure has received first PDU */
	INCOMPAT_RESERVED,
};

/* Invalid LL Control PDU Opcode */
#define ULL_LLCP_INVALID_OPCODE (0xFFU)

static inline bool is_instant_passed(uint16_t instant, uint16_t event_count)
{
	/*
	 * BLUETOOTH CORE SPECIFICATION Version 5.2 | Vol 6, Part B
	 * 5.5.1 ACL Control Procedures
	 * When a periph receives such a PDU where (Instant – connEventCount) modulo
	 * 65536 is less than 32767 and Instant is not equal to connEventCount, the periph
	 * shall listen to all the connection events until it has confirmation that the central
	 * has received its acknowledgment of the PDU or connEventCount equals
	 * Instant.
	 *
	 * x % 2^n == x & (2^n - 1)
	 *
	 * 65535 = 2^16 - 1
	 * 65536 = 2^16
	 */

	return ((instant - event_count) & 0xFFFFU) > 0x7FFFU;
}

static inline bool is_instant_not_passed(uint16_t instant, uint16_t event_count)
{
	/*
	 * BLUETOOTH CORE SPECIFICATION Version 5.2 | Vol 6, Part B
	 * 5.5.1 ACL Control Procedures
	 * When a periph receives such a PDU where (Instant – connEventCount) modulo
	 * 65536 is less than 32767 and Instant is not equal to connEventCount, the periph
	 * shall listen to all the connection events until it has confirmation that the central
	 * has received its acknowledgment of the PDU or connEventCount equals
	 * Instant.
	 *
	 * x % 2^n == x & (2^n - 1)
	 *
	 * 65535 = 2^16 - 1
	 * 65536 = 2^16
	 */

	return ((instant - event_count) & 0xFFFFU) < 0x7FFFU;
}

static inline bool is_instant_reached_or_passed(uint16_t instant, uint16_t event_count)
{
	/*
	 * BLUETOOTH CORE SPECIFICATION Version 5.2 | Vol 6, Part B
	 * 5.5.1 ACL Control Procedures
	 * When a periph receives such a PDU where (Instant – connEventCount) modulo
	 * 65536 is less than 32767 and Instant is not equal to connEventCount, the periph
	 * shall listen to all the connection events until it has confirmation that the central
	 * has received its acknowledgment of the PDU or connEventCount equals
	 * Instant.
	 *
	 * x % 2^n == x & (2^n - 1)
	 *
	 * 65535 = 2^16 - 1
	 * 65536 = 2^16
	 */

	return ((event_count - instant) & 0xFFFFU) <= 0x7FFFU;
}

/*
 * LLCP Resource Management
 */
bool llcp_ntf_alloc_is_available(void);
bool llcp_ntf_alloc_num_available(uint8_t count);
struct node_rx_pdu *llcp_ntf_alloc(void);
struct proc_ctx *llcp_create_local_procedure(enum llcp_proc proc);
struct proc_ctx *llcp_create_remote_procedure(enum llcp_proc proc);
void llcp_nodes_release(struct ll_conn *conn, struct proc_ctx *ctx);
bool llcp_tx_alloc_peek(struct ll_conn *conn, struct proc_ctx *ctx);
void llcp_tx_alloc_unpeek(struct proc_ctx *ctx);
struct node_tx *llcp_tx_alloc(struct ll_conn *conn, struct proc_ctx *ctx);
void llcp_proc_ctx_release(struct proc_ctx *ctx);
void llcp_ntf_set_pending(struct ll_conn *conn);
void llcp_ntf_clear_pending(struct ll_conn *conn);
bool llcp_ntf_pending(struct ll_conn *conn);
void llcp_rx_node_retain(struct proc_ctx *ctx);
void llcp_rx_node_release(struct proc_ctx *ctx);

/*
 * ULL -> LLL Interface
 */
void llcp_tx_enqueue(struct ll_conn *conn, struct node_tx *tx);
void llcp_tx_pause_data(struct ll_conn *conn, enum llcp_tx_q_pause_data_mask pause_mask);
void llcp_tx_resume_data(struct ll_conn *conn, enum llcp_tx_q_pause_data_mask resume_mask);

/*
 * LLCP Procedure Response Timeout
 */
void llcp_lr_prt_restart(struct ll_conn *conn);
void llcp_lr_prt_restart_with_value(struct ll_conn *conn, uint16_t value);
void llcp_lr_prt_stop(struct ll_conn *conn);
void llcp_rr_prt_restart(struct ll_conn *conn);
void llcp_rr_prt_stop(struct ll_conn *conn);

/*
 * LLCP Local Procedure Common
 */
void llcp_lp_comm_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, struct node_tx *tx);
void llcp_lp_comm_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);
void llcp_lp_comm_init_proc(struct proc_ctx *ctx);
void llcp_lp_comm_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param);

/*
 * LLCP Remote Procedure Common
 */
void llcp_rp_comm_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, struct node_tx *tx);
void llcp_rp_comm_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);
void llcp_rp_comm_init_proc(struct proc_ctx *ctx);
void llcp_rp_comm_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param);

#if defined(CONFIG_BT_CTLR_LE_ENC)
/*
 * LLCP Local Procedure Encryption
 */
void llcp_lp_enc_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);
void llcp_lp_enc_init_proc(struct proc_ctx *ctx);
void llcp_lp_enc_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param);

/*
 * LLCP Remote Procedure Encryption
 */
void llcp_rp_enc_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);
void llcp_rp_enc_init_proc(struct proc_ctx *ctx);
void llcp_rp_enc_ltk_req_reply(struct ll_conn *conn, struct proc_ctx *ctx);
void llcp_rp_enc_ltk_req_neg_reply(struct ll_conn *conn, struct proc_ctx *ctx);
bool llcp_rp_enc_ltk_req_reply_allowed(struct ll_conn *conn, struct proc_ctx *ctx);
void llcp_rp_enc_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param);

#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_PHY)
/*
 * LLCP Local Procedure PHY Update
 */
void llcp_lp_pu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);
void llcp_lp_pu_init_proc(struct proc_ctx *ctx);
void llcp_lp_pu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param);
void llcp_lp_pu_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, void *param);
void llcp_lp_pu_tx_ntf(struct ll_conn *conn, struct proc_ctx *ctx);
bool llcp_lp_pu_awaiting_instant(struct proc_ctx *ctx);
#endif /* CONFIG_BT_CTLR_PHY */

/*
 * LLCP Local Procedure Connection Update
 */
void llcp_lp_cu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);
void llcp_lp_cu_init_proc(struct proc_ctx *ctx);
void llcp_lp_cu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param);
bool llcp_lp_cu_awaiting_instant(struct proc_ctx *ctx);

/*
 * LLCP Local Channel Map Update
 */
void llcp_lp_chmu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);
void llcp_lp_chmu_init_proc(struct proc_ctx *ctx);
void llcp_lp_chmu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param);
bool llcp_lp_chmu_awaiting_instant(struct proc_ctx *ctx);

#if defined(CONFIG_BT_CTLR_PHY)
/*
 * LLCP Remote Procedure PHY Update
 */
void llcp_rp_pu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);
void llcp_rp_pu_init_proc(struct proc_ctx *ctx);
void llcp_rp_pu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param);
void llcp_rp_pu_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, void *param);
void llcp_rp_pu_tx_ntf(struct ll_conn *conn, struct proc_ctx *ctx);
bool llcp_rp_pu_awaiting_instant(struct proc_ctx *ctx);
#endif /* CONFIG_BT_CTLR_PHY */

/*
 * LLCP Remote Procedure Connection Update
 */
void llcp_rp_cu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);
void llcp_rp_cu_init_proc(struct proc_ctx *ctx);
void llcp_rp_cu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param);
void llcp_rp_conn_param_req_reply(struct ll_conn *conn, struct proc_ctx *ctx);
void llcp_rp_conn_param_req_neg_reply(struct ll_conn *conn, struct proc_ctx *ctx);
bool llcp_rp_conn_param_req_apm_awaiting_reply(struct proc_ctx *ctx);
void llcp_rp_conn_param_req_apm_reply(struct ll_conn *conn, struct proc_ctx *ctx);
bool llcp_rp_cu_awaiting_instant(struct proc_ctx *ctx);

/*
 * Terminate Helper
 */
void llcp_pdu_encode_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_decode_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu);

/*
 * LLCP Local Request
 */
struct proc_ctx *llcp_lr_peek(struct ll_conn *conn);
struct proc_ctx *llcp_lr_peek_proc(struct ll_conn *conn, uint8_t proc);
bool llcp_lr_ispaused(struct ll_conn *conn);
void llcp_lr_pause(struct ll_conn *conn);
void llcp_lr_resume(struct ll_conn *conn);
void llcp_lr_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, struct node_tx *tx);
void llcp_lr_tx_ntf(struct ll_conn *conn, struct proc_ctx *ctx);
void llcp_lr_rx(struct ll_conn *conn, struct proc_ctx *ctx, memq_link_t *link,
		struct node_rx_pdu *rx);
void llcp_lr_enqueue(struct ll_conn *conn, struct proc_ctx *ctx);
void llcp_lr_init(struct ll_conn *conn);
void llcp_lr_run(struct ll_conn *conn);
void llcp_lr_complete(struct ll_conn *conn);
void llcp_lr_connect(struct ll_conn *conn);
void llcp_lr_disconnect(struct ll_conn *conn);
void llcp_lr_terminate(struct ll_conn *conn);
void llcp_lr_check_done(struct ll_conn *conn, struct proc_ctx *ctx);

/*
 * LLCP Remote Request
 */
void llcp_rr_set_incompat(struct ll_conn *conn, enum proc_incompat incompat);
bool llcp_rr_get_collision(struct ll_conn *conn);
void llcp_rr_set_paused_cmd(struct ll_conn *conn, enum llcp_proc);
enum llcp_proc llcp_rr_get_paused_cmd(struct ll_conn *conn);
struct proc_ctx *llcp_rr_peek(struct ll_conn *conn);
bool llcp_rr_ispaused(struct ll_conn *conn);
void llcp_rr_pause(struct ll_conn *conn);
void llcp_rr_resume(struct ll_conn *conn);
void llcp_rr_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, struct node_tx *tx);
void llcp_rr_tx_ntf(struct ll_conn *conn, struct proc_ctx *ctx);
void llcp_rr_rx(struct ll_conn *conn, struct proc_ctx *ctx, memq_link_t *link,
		struct node_rx_pdu *rx);
void llcp_rr_init(struct ll_conn *conn);
void llcp_rr_prepare(struct ll_conn *conn, struct node_rx_pdu *rx);
void llcp_rr_run(struct ll_conn *conn);
void llcp_rr_complete(struct ll_conn *conn);
void llcp_rr_connect(struct ll_conn *conn);
void llcp_rr_disconnect(struct ll_conn *conn);
void llcp_rr_terminate(struct ll_conn *conn);
void llcp_rr_new(struct ll_conn *conn, memq_link_t *link, struct node_rx_pdu *rx,
		 bool valid_pdu);
void llcp_rr_check_done(struct ll_conn *conn, struct proc_ctx *ctx);

#if defined(CONFIG_BT_CTLR_LE_PING)
/*
 * LE Ping Procedure Helper
 */
void llcp_pdu_encode_ping_req(struct pdu_data *pdu);
void llcp_pdu_encode_ping_rsp(struct pdu_data *pdu);
#endif /* CONFIG_BT_CTLR_LE_PING */
/*
 * Unknown response helper
 */

void llcp_pdu_encode_unknown_rsp(struct proc_ctx *ctx,
					struct pdu_data *pdu);
void llcp_pdu_decode_unknown_rsp(struct proc_ctx *ctx,
					struct pdu_data *pdu);
void llcp_ntf_encode_unknown_rsp(struct proc_ctx *ctx,
					struct pdu_data *pdu);

/*
 * Feature Exchange Procedure Helper
 */
void llcp_pdu_encode_feature_req(struct ll_conn *conn,
					struct pdu_data *pdu);
void llcp_pdu_encode_feature_rsp(struct ll_conn *conn,
					struct pdu_data *pdu);
void llcp_ntf_encode_feature_rsp(struct ll_conn *conn,
					struct pdu_data *pdu);
void llcp_ntf_encode_feature_req(struct ll_conn *conn,
					      struct pdu_data *pdu);
void llcp_pdu_decode_feature_req(struct ll_conn *conn,
					struct pdu_data *pdu);
void llcp_pdu_decode_feature_rsp(struct ll_conn *conn,
					struct pdu_data *pdu);

#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
/*
 * Minimum number of used channels Procedure Helper
 */
#if defined(CONFIG_BT_PERIPHERAL)
void llcp_pdu_encode_min_used_chans_ind(struct proc_ctx *ctx, struct pdu_data *pdu);
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
void llcp_pdu_decode_min_used_chans_ind(struct ll_conn *conn, struct pdu_data *pdu);
#endif /* CONFIG_BT_CENTRAL */
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */

/*
 * Version Exchange Procedure Helper
 */
void llcp_pdu_encode_version_ind(struct pdu_data *pdu);
void llcp_ntf_encode_version_ind(struct ll_conn *conn, struct pdu_data *pdu);
void llcp_pdu_decode_version_ind(struct ll_conn *conn, struct pdu_data *pdu);

#if defined(CONFIG_BT_CTLR_LE_ENC)
/*
 * Encryption Start Procedure Helper
 */
#if defined(CONFIG_BT_CENTRAL)
void llcp_pdu_encode_enc_req(struct proc_ctx *ctx, struct pdu_data *pdu);
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
void llcp_ntf_encode_enc_req(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_encode_enc_rsp(struct pdu_data *pdu);
void llcp_pdu_encode_start_enc_req(struct pdu_data *pdu);
#endif /* CONFIG_BT_PERIPHERAL */

void llcp_pdu_encode_start_enc_rsp(struct pdu_data *pdu);

#if defined(CONFIG_BT_CENTRAL)
void llcp_pdu_encode_pause_enc_req(struct pdu_data *pdu);
#endif /* CONFIG_BT_CENTRAL */

void llcp_pdu_encode_pause_enc_rsp(struct pdu_data *pdu);
#endif /* CONFIG_BT_CTLR_LE_ENC */

void llcp_pdu_encode_reject_ind(struct pdu_data *pdu, uint8_t error_code);
void llcp_pdu_encode_reject_ext_ind(struct pdu_data *pdu, uint8_t reject_opcode,
				    uint8_t error_code);
void llcp_pdu_decode_reject_ext_ind(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_ntf_encode_reject_ext_ind(struct proc_ctx *ctx, struct pdu_data *pdu);

/*
 * PHY Update Procedure Helper
 */
void llcp_pdu_encode_phy_req(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_decode_phy_req(struct proc_ctx *ctx, struct pdu_data *pdu);

#if defined(CONFIG_BT_PERIPHERAL)
void llcp_pdu_encode_phy_rsp(struct ll_conn *conn, struct pdu_data *pdu);
void llcp_pdu_decode_phy_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu);
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
void llcp_pdu_encode_phy_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_decode_phy_rsp(struct proc_ctx *ctx, struct pdu_data *pdu);
#endif /* CONFIG_BT_CENTRAL */

/*
 * Connection Update Procedure Helper
 */
void llcp_pdu_encode_conn_param_req(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_decode_conn_param_req(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_encode_conn_param_rsp(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_decode_conn_param_rsp(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_encode_conn_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_decode_conn_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu);

/*
 * Remote Channel Map Update Procedure Helper
 */
void llcp_pdu_encode_chan_map_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_decode_chan_map_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_rp_chmu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);
void llcp_rp_chmu_init_proc(struct proc_ctx *ctx);
void llcp_rp_chmu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param);
bool llcp_rp_chmu_awaiting_instant(struct proc_ctx *ctx);

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
/*
 * Data Length Update Procedure Helper
 */
void llcp_pdu_encode_length_req(struct ll_conn *conn, struct pdu_data *pdu);
void llcp_pdu_encode_length_rsp(struct ll_conn *conn, struct pdu_data *pdu);
void llcp_pdu_decode_length_req(struct ll_conn *conn, struct pdu_data *pdu);
void llcp_pdu_decode_length_rsp(struct ll_conn *conn, struct pdu_data *pdu);
void llcp_ntf_encode_length_change(struct ll_conn *conn,
					struct pdu_data *pdu);

#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
/*
 * Sleep Clock Accuracy Update Procedure Helper
 */
void llcp_pdu_encode_clock_accuracy_req(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_encode_clock_accuracy_rsp(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_decode_clock_accuracy_req(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_decode_clock_accuracy_rsp(struct proc_ctx *ctx, struct pdu_data *pdu);

#endif /* CONFIG_BT_CTLR_SCA_UPDATE */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
/*
 * Constant Tone Request Procedure Helper
 */
void llcp_pdu_encode_cte_req(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_decode_cte_rsp(struct proc_ctx *ctx, const struct pdu_data *pdu);
void llcp_ntf_encode_cte_req(struct pdu_data *pdu);
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
/*
 * Constant Tone Response Procedure Helper
 */
void llcp_pdu_decode_cte_req(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_encode_cte_rsp(const struct proc_ctx *ctx, struct pdu_data *pdu);
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */

void llcp_lp_cc_init_proc(struct proc_ctx *ctx);
void llcp_lp_cc_offset_calc_reply(struct ll_conn *conn, struct proc_ctx *ctx);
void llcp_lp_cc_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);
void llcp_lp_cc_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param);
bool llcp_lp_cc_is_active(struct proc_ctx *ctx);
bool llcp_lp_cc_awaiting_established(struct proc_ctx *ctx);
void llcp_lp_cc_established(struct ll_conn *conn, struct proc_ctx *ctx);
bool llcp_lp_cc_cancel(struct ll_conn *conn, struct proc_ctx *ctx);

void llcp_rp_cc_init_proc(struct proc_ctx *ctx);
void llcp_rp_cc_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);
void llcp_rp_cc_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param);
bool llcp_rp_cc_awaiting_reply(struct proc_ctx *ctx);
bool llcp_rp_cc_awaiting_established(struct proc_ctx *ctx);
void llcp_rp_cc_accept(struct ll_conn *conn, struct proc_ctx *ctx);
void llcp_rp_cc_reject(struct ll_conn *conn, struct proc_ctx *ctx);
bool llcp_rp_cc_awaiting_instant(struct proc_ctx *ctx);
void llcp_rp_cc_established(struct ll_conn *conn, struct proc_ctx *ctx);

void llcp_pdu_decode_cis_req(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_encode_cis_rsp(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_decode_cis_ind(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_encode_cis_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_decode_cis_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_encode_cis_req(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_encode_cis_ind(struct proc_ctx *ctx, struct pdu_data *pdu);
void llcp_pdu_decode_cis_rsp(struct proc_ctx *ctx, struct pdu_data *pdu);

#ifdef ZTEST_UNITTEST
bool llcp_lr_is_disconnected(struct ll_conn *conn);
bool llcp_lr_is_idle(struct ll_conn *conn);
struct proc_ctx *llcp_lr_dequeue(struct ll_conn *conn);

bool llcp_rr_is_disconnected(struct ll_conn *conn);
bool llcp_rr_is_idle(struct ll_conn *conn);
struct proc_ctx *llcp_rr_dequeue(struct ll_conn *conn);
void llcp_rr_enqueue(struct ll_conn *conn, struct proc_ctx *ctx);

uint16_t llcp_local_ctx_buffers_free(void);
uint16_t llcp_remote_ctx_buffers_free(void);
uint16_t llcp_ctx_buffers_free(void);
uint8_t llcp_common_tx_buffer_alloc_count(void);
struct proc_ctx *llcp_proc_ctx_acquire(void);
struct proc_ctx *llcp_create_procedure(enum llcp_proc proc);
#endif
