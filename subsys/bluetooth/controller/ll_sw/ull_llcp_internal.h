/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* LLCP Procedure */
enum llcp_proc {
	PROC_UNKNOWN,
	PROC_LE_PING,
	PROC_FEATURE_EXCHANGE,
	PROC_MIN_USED_CHANS,
	PROC_VERSION_EXCHANGE,
	PROC_ENCRYPTION_START,
	PROC_PHY_UPDATE,
	PROC_TERMINATE,
};

/* LLCP Procedure Context */
struct proc_ctx {
	/* Must be the first for sys_slist to work */
	sys_snode_t node;

	/* PROC_ */
	enum llcp_proc proc;

	enum pdu_data_llctrl_type response_opcode;

	/* Procedure FSM */
	uint8_t state;

	/* Expected opcode to be recieved next */
	enum pdu_data_llctrl_type rx_opcode;

	/* Last transmitted opcode used for unknown/reject */
	enum pdu_data_llctrl_type tx_opcode;

	/* Instant collision */
	int collision;

	/* Procedure pause */
	int pause;

	/* TX node awaiting ack */
	struct node_tx * tx_ack;

	/* Procedure data */
	union {
		/* Used by Minimum Used Channels Procedure */
		struct {
			uint8_t phys;
			uint8_t min_used_chans;
		} muc;

		/* Used by Encryption Procedure */
		struct {
			uint8_t error;
		} enc;

		/* PHY Update */
		struct {
			uint8_t error;
			uint16_t instant;
		} pu;

		/* Use by ACL Termination Procedure */
		struct {
			uint8_t error_code;
		} term;

	} data;
	struct {
		uint8_t type;
	} unknown_response;

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

/*
 * LLCP Resource Management
 */
bool ull_cp_priv_tx_alloc_is_available(void);

static inline bool tx_alloc_is_available(void)
{
	return ull_cp_priv_tx_alloc_is_available();
}

struct node_tx *ull_cp_priv_tx_alloc(void);

static inline struct node_tx *tx_alloc(void)
{
	return ull_cp_priv_tx_alloc();
}

bool ull_cp_priv_ntf_alloc_is_available(void);

static inline bool ntf_alloc_is_available(void)
{
	return ull_cp_priv_ntf_alloc_is_available();
}

struct node_rx_pdu *ull_cp_priv_ntf_alloc(void);

static inline struct node_rx_pdu *ntf_alloc(void)
{
	return ull_cp_priv_ntf_alloc();
}

struct proc_ctx *ull_cp_priv_create_local_procedure(enum llcp_proc proc);

static inline struct proc_ctx *create_local_procedure(enum llcp_proc proc)
{
	return ull_cp_priv_create_local_procedure(proc);
}

struct proc_ctx *ull_cp_priv_create_remote_procedure(enum llcp_proc proc);

static inline struct proc_ctx *create_remote_procedure(enum llcp_proc proc)
{
	return ull_cp_priv_create_remote_procedure(proc);
}


/*
 * ULL -> LLL Interface
 */
void ull_cp_priv_tx_enqueue(struct ull_cp_conn *conn, struct node_tx *tx);

static inline void tx_enqueue(struct ull_cp_conn *conn, struct node_tx *tx)
{
	return ull_cp_priv_tx_enqueue(conn, tx);
}

void ull_cp_priv_tx_pause_data(struct ull_cp_conn *conn);

static inline void tx_pause_data(struct ull_cp_conn *conn)
{
	return ull_cp_priv_tx_pause_data(conn);
}

void ull_cp_priv_tx_flush(struct ull_cp_conn *conn);

static inline void tx_flush(struct ull_cp_conn *conn)
{
	return ull_cp_priv_tx_flush(conn);
}


/*
 * LLCP Local Procedure Common
 */
void ull_cp_priv_lp_comm_tx_ack(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_tx *tx);

static inline void lp_comm_tx_ack(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_tx *tx)
{
	return ull_cp_priv_lp_comm_tx_ack(conn, ctx, tx);
}

void ull_cp_priv_lp_comm_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);

static inline void lp_comm_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	return ull_cp_priv_lp_comm_rx(conn, ctx, rx);
}

void ull_cp_priv_lp_comm_init_proc(struct proc_ctx *ctx);

static inline void lp_comm_init_proc(struct proc_ctx *ctx)
{
	return ull_cp_priv_lp_comm_init_proc(ctx);
}

void ull_cp_priv_lp_comm_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param);

static inline void lp_comm_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param)
{
	return ull_cp_priv_lp_comm_run(conn, ctx, param);
}

/*
 * LLCP Remote Procedure Common
 */
void ull_cp_priv_rp_comm_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);

static inline void rp_comm_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	return ull_cp_priv_rp_comm_rx(conn, ctx, rx);
}

void ull_cp_priv_rp_comm_init_proc(struct proc_ctx *ctx);

static inline void rp_comm_init_proc(struct proc_ctx *ctx)
{
	return ull_cp_priv_rp_comm_init_proc(ctx);
}

void ull_cp_priv_rp_comm_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param);

static inline void rp_comm_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param)
{
	return ull_cp_priv_rp_comm_run(conn, ctx, param);
}


/*
 * LLCP Local Procedure Encryption
 */
void ull_cp_priv_lp_enc_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);

static inline void lp_enc_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	return ull_cp_priv_lp_enc_rx(conn, ctx, rx);
}

void ull_cp_priv_lp_enc_init_proc(struct proc_ctx *ctx);

static inline void lp_enc_init_proc(struct proc_ctx *ctx)
{
	return ull_cp_priv_lp_enc_init_proc(ctx);
}

void ull_cp_priv_lp_enc_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param);

static inline void lp_enc_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param)
{
	return ull_cp_priv_lp_enc_run(conn, ctx, param);
}


/*
 * LLCP Remote Procedure Encryption
 */
void ull_cp_priv_rp_enc_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);

static inline void rp_enc_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	return ull_cp_priv_rp_enc_rx(conn, ctx, rx);
}

void ull_cp_priv_rp_enc_init_proc(struct proc_ctx *ctx);

static inline void rp_enc_init_proc(struct proc_ctx *ctx)
{
	return ull_cp_priv_rp_enc_init_proc(ctx);
}

void ull_cp_priv_rp_enc_ltk_req_reply(struct ull_cp_conn *conn, struct proc_ctx *ctx);

static inline void rp_enc_ltk_req_reply(struct ull_cp_conn *conn, struct proc_ctx *ctx)
{
	return ull_cp_priv_rp_enc_ltk_req_reply(conn, ctx);
}

void ull_cp_priv_rp_enc_ltk_req_neg_reply(struct ull_cp_conn *conn, struct proc_ctx *ctx);

static inline void rp_enc_ltk_req_neg_reply(struct ull_cp_conn *conn, struct proc_ctx *ctx)
{
	return ull_cp_priv_rp_enc_ltk_req_neg_reply(conn, ctx);
}

void ull_cp_priv_rp_enc_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param);

static inline void rp_enc_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param)
{
	return ull_cp_priv_rp_enc_run(conn, ctx, param);
}


/*
 * LLCP Local Procedure PHY Update
 */
void ull_cp_priv_lp_pu_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);

static inline void lp_pu_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	return ull_cp_priv_lp_pu_rx(conn, ctx, rx);
}

void ull_cp_priv_lp_pu_init_proc(struct proc_ctx *ctx);

static inline void lp_pu_init_proc(struct proc_ctx *ctx)
{
	return ull_cp_priv_lp_pu_init_proc(ctx);
}

void ull_cp_priv_lp_pu_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param);

static inline void lp_pu_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param)
{
	return ull_cp_priv_lp_pu_run(conn, ctx, param);
}


/*
 * LLCP Remote Procedure PHY Update
 */
void ull_cp_priv_rp_pu_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);

static inline void rp_pu_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	return ull_cp_priv_rp_pu_rx(conn, ctx, rx);
}

void ull_cp_priv_rp_pu_init_proc(struct proc_ctx *ctx);

static inline void rp_pu_init_proc(struct proc_ctx *ctx)
{
	return ull_cp_priv_rp_pu_init_proc(ctx);
}

void ull_cp_priv_rp_pu_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param);

static inline void rp_pu_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param)
{
	return ull_cp_priv_rp_pu_run(conn, ctx, param);
}

/*
 * Terminate Helper
 */
void ull_cp_priv_pdu_encode_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu);

static inline void pdu_encode_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_encode_terminate_ind(ctx, pdu);
}

void ull_cp_priv_ntf_encode_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu);

static inline void ntf_encode_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	return ull_cp_priv_ntf_encode_terminate_ind(ctx, pdu);
}

void ull_cp_priv_pdu_decode_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu);

static inline void pdu_decode_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_decode_terminate_ind(ctx, pdu);
}

/*
 * LLCP Local Request
 */
struct proc_ctx *lr_peek(struct ull_cp_conn *conn);
void ull_cp_priv_lr_tx_ack(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_tx *tx);

static inline void lr_tx_ack(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_tx *tx)
{
	return ull_cp_priv_lr_tx_ack(conn, ctx, tx);
}

void ull_cp_priv_lr_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);

static inline void lr_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	return ull_cp_priv_lr_rx(conn, ctx, rx);
}

void ull_cp_priv_lr_enqueue(struct ull_cp_conn *conn, struct proc_ctx *ctx);

static inline void lr_enqueue(struct ull_cp_conn *conn, struct proc_ctx *ctx)
{
	return ull_cp_priv_lr_enqueue(conn, ctx);
}

void ull_cp_priv_lr_init(struct ull_cp_conn *conn);

static inline void lr_init(struct ull_cp_conn *conn)
{
	return ull_cp_priv_lr_init(conn);
}

void ull_cp_priv_lr_run(struct ull_cp_conn *conn);

static inline void lr_run(struct ull_cp_conn *conn)
{
	return ull_cp_priv_lr_run(conn);
}

void ull_cp_priv_lr_complete(struct ull_cp_conn *conn);

static inline void lr_complete(struct ull_cp_conn *conn)
{
	return ull_cp_priv_lr_complete(conn);
}

void ull_cp_priv_lr_connect(struct ull_cp_conn *conn);

static inline void lr_connect(struct ull_cp_conn *conn)
{
	return ull_cp_priv_lr_connect(conn);
}

void ull_cp_priv_lr_disconnect(struct ull_cp_conn *conn);

static inline void lr_disconnect(struct ull_cp_conn *conn)
{
	return ull_cp_priv_lr_disconnect(conn);
}


/*
 * LLCP Remote Request
 */
void ull_cp_priv_rr_set_incompat(struct ull_cp_conn *conn, enum proc_incompat incompat);

static inline void rr_set_incompat(struct ull_cp_conn *conn, enum proc_incompat incompat)
{
	return ull_cp_priv_rr_set_incompat(conn, incompat);
}

bool ull_cp_priv_rr_get_collision(struct ull_cp_conn *conn);

static inline bool rr_get_collision(struct ull_cp_conn *conn)
{
	return ull_cp_priv_rr_get_collision(conn);
}

struct proc_ctx *rr_peek(struct ull_cp_conn *conn);
void ull_cp_priv_rr_tx_ack(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_tx *tx);

static inline void rr_tx_ack(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_tx *tx)
{
	return ull_cp_priv_rr_tx_ack(conn, ctx, tx);
}

void ull_cp_priv_rr_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx);

static inline void rr_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	return ull_cp_priv_rr_rx(conn, ctx, rx);
}

void ull_cp_priv_rr_init(struct ull_cp_conn *conn);

static inline void rr_init(struct ull_cp_conn *conn)
{
	return ull_cp_priv_rr_init(conn);
}

void ull_cp_priv_rr_prepare(struct ull_cp_conn *conn, struct node_rx_pdu *rx);

static inline void rr_prepare(struct ull_cp_conn *conn, struct node_rx_pdu *rx)
{
	return ull_cp_priv_rr_prepare(conn, rx);
}

void ull_cp_priv_rr_run(struct ull_cp_conn *conn);

static inline void rr_run(struct ull_cp_conn *conn)
{
	return ull_cp_priv_rr_run(conn);
}

void ull_cp_priv_rr_complete(struct ull_cp_conn *conn);

static inline void rr_complete(struct ull_cp_conn *conn)
{
	return ull_cp_priv_rr_complete(conn);
}

void ull_cp_priv_rr_connect(struct ull_cp_conn *conn);

static inline void rr_connect(struct ull_cp_conn *conn)
{
	return ull_cp_priv_rr_connect(conn);
}

void ull_cp_priv_rr_disconnect(struct ull_cp_conn *conn);

static inline void rr_disconnect(struct ull_cp_conn *conn)
{
	return ull_cp_priv_rr_disconnect(conn);
}

void ull_cp_priv_rr_new(struct ull_cp_conn *conn, struct node_rx_pdu *rx);

static inline void rr_new(struct ull_cp_conn *conn, struct node_rx_pdu *rx)
{
	return ull_cp_priv_rr_new(conn, rx);
}

/*
 * LE Ping Procedure Helper
 */
void ull_cp_priv_pdu_encode_ping_req(struct pdu_data *pdu);

static inline void pdu_encode_ping_req(struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_encode_ping_req(pdu);
}

void ull_cp_priv_pdu_encode_ping_rsp(struct pdu_data *pdu);

static inline void pdu_encode_ping_rsp(struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_encode_ping_rsp(pdu);
}

/*
 * Unknown response helper
 */
void ull_cp_priv_pdu_decode_unknown_rsp(struct proc_ctx *ctx,
					struct pdu_data *pdu);
static inline void pdu_decode_unknown_rsp(struct proc_ctx *ctx,
					   struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_decode_unknown_rsp(ctx, pdu);
}

void ull_cp_priv_ntf_encode_unknown_rsp(struct proc_ctx *ctx,
					struct pdu_data *pdu);

static inline void ntf_encode_unknown_rsp(struct proc_ctx *ctx,
					  struct pdu_data *pdu)

{
	return ull_cp_priv_ntf_encode_unknown_rsp(ctx, pdu);
}


/*
 * Feature Exchange Procedure Helper
 */
void ull_cp_priv_pdu_encode_feature_req(struct ull_cp_conn *conn,
					struct pdu_data *pdu);

static inline void pdu_encode_feature_req(struct ull_cp_conn *conn,
					   struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_encode_feature_req(conn, pdu);
}

void ull_cp_priv_pdu_encode_feature_rsp(struct ull_cp_conn *conn,
					struct pdu_data *pdu);

static inline void pdu_encode_feature_rsp(struct ull_cp_conn *conn,
					struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_encode_feature_rsp(conn, pdu);
}

void ull_cp_priv_ntf_encode_feature_rsp(struct ull_cp_conn *conn,
					struct pdu_data *pdu);

static inline void ntf_encode_feature_rsp(struct ull_cp_conn *conn,
				    struct pdu_data *pdu)
{
	return ull_cp_priv_ntf_encode_feature_rsp(conn, pdu);
}

void ull_cp_priv_ntf_encode_feature_req(struct ull_cp_conn *conn,
					      struct pdu_data *pdu);

static inline void ntf_encode_feature_req(struct ull_cp_conn *conn,
					  struct pdu_data *pdu)
{
	return ull_cp_priv_ntf_encode_feature_req(conn, pdu);
}

void ull_cp_priv_pdu_decode_feature_req(struct ull_cp_conn *conn,
					struct pdu_data *pdu);

static inline void pdu_decode_feature_req(struct ull_cp_conn *conn,
					  struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_decode_feature_req(conn, pdu);
}

void ull_cp_priv_pdu_decode_feature_rsp(struct ull_cp_conn *conn,
					struct pdu_data *pdu);

static inline void pdu_decode_feature_rsp(struct ull_cp_conn *conn,
					  struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_decode_feature_rsp(conn, pdu);
}

/*
 * Minimum number of used channels Procedure Helper
 */
void ull_cp_priv_pdu_encode_min_used_chans_ind(struct proc_ctx *ctx, struct pdu_data *pdu);

static inline void pdu_encode_min_used_chans_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_encode_min_used_chans_ind(ctx, pdu);
}

void ull_cp_priv_pdu_decode_min_used_chans_ind(struct ull_cp_conn *conn, struct pdu_data *pdu);

static inline void pdu_decode_min_used_chans_ind(struct ull_cp_conn *conn, struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_decode_min_used_chans_ind(conn, pdu);
}

/*
 * Version Exchange Procedure Helper
 */
void ull_cp_priv_pdu_encode_version_ind(struct pdu_data *pdu);

static inline void pdu_encode_version_ind(struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_encode_version_ind(pdu);
}

void ull_cp_priv_ntf_encode_version_ind(struct ull_cp_conn *conn, struct pdu_data *pdu);

static inline void ntf_encode_version_ind(struct ull_cp_conn *conn, struct pdu_data *pdu)
{
	return ull_cp_priv_ntf_encode_version_ind(conn, pdu);
}

void ull_cp_priv_pdu_decode_version_ind(struct ull_cp_conn *conn, struct pdu_data *pdu);

static inline void pdu_decode_version_ind(struct ull_cp_conn *conn, struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_decode_version_ind(conn, pdu);
}


/*
 * Encryption Start Procedure Helper
 */
void ull_cp_priv_pdu_encode_enc_req(struct pdu_data *pdu);

static inline void pdu_encode_enc_req(struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_encode_enc_req(pdu);
}

void ull_cp_priv_pdu_encode_enc_rsp(struct pdu_data *pdu);

static inline void pdu_encode_enc_rsp(struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_encode_enc_rsp(pdu);
}

void ull_cp_priv_pdu_encode_start_enc_req(struct pdu_data *pdu);

static inline void pdu_encode_start_enc_req(struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_encode_start_enc_req(pdu);
}

void ull_cp_priv_pdu_encode_start_enc_rsp(struct pdu_data *pdu);

static inline void pdu_encode_start_enc_rsp(struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_encode_start_enc_rsp(pdu);
}

void ull_cp_priv_pdu_encode_reject_ind(struct pdu_data *pdu, uint8_t error_code);

static inline void pdu_encode_reject_ind(struct pdu_data *pdu, uint8_t error_code)
{
	return ull_cp_priv_pdu_encode_reject_ind(pdu, error_code);
}

void ull_cp_priv_pdu_encode_reject_ext_ind(struct pdu_data *pdu, uint8_t reject_opcode, uint8_t error_code);

static inline void pdu_encode_reject_ext_ind(struct pdu_data *pdu, uint8_t reject_opcode, uint8_t error_code)
{
	return ull_cp_priv_pdu_encode_reject_ext_ind(pdu, reject_opcode, error_code);
}


/*
 * PHY Update Procedure Helper
 */
void ull_cp_priv_pdu_encode_phy_req(struct pdu_data *pdu);

static inline void pdu_encode_phy_req(struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_encode_phy_req(pdu);
}

void ull_cp_priv_pdu_encode_phy_rsp(struct pdu_data *pdu);

static inline void pdu_encode_phy_rsp(struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_encode_phy_rsp(pdu);
}

void ull_cp_priv_pdu_encode_phy_update_ind(struct pdu_data *pdu, uint16_t instant);

static inline void pdu_encode_phy_update_ind(struct pdu_data *pdu, uint16_t instant)
{
	return ull_cp_priv_pdu_encode_phy_update_ind(pdu, instant);
}

void ull_cp_priv_pdu_decode_phy_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu);

static inline void pdu_decode_phy_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	return ull_cp_priv_pdu_decode_phy_update_ind(ctx, pdu);
}

#ifdef ZTEST_UNITTEST
bool lr_is_disconnected(struct ull_cp_conn *conn);
bool lr_is_idle(struct ull_cp_conn *conn);
bool rr_is_disconnected(struct ull_cp_conn *conn);
bool rr_is_idle(struct ull_cp_conn *conn);
#endif
