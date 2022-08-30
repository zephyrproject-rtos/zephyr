/** @file
 @brief PPP private header

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/ppp.h>

/**
 * FSM flags that control how it operates.
 */
#define FSM_RESTART    BIT(0) /**< Treat 2nd OPEN as DOWN followed by UP    */

/**
 * PPP packet format.
 */
struct ppp_packet {
	uint8_t code;
	uint8_t id;
	uint16_t length;
} __packed;

/** Max Terminate-Request transmissions */
#define MAX_TERMINATE_REQ  CONFIG_NET_L2_PPP_MAX_TERMINATE_REQ_RETRANSMITS

/** Max Configure-Request transmissions */
#define MAX_CONFIGURE_REQ CONFIG_NET_L2_PPP_MAX_CONFIGURE_REQ_RETRANSMITS

#define PPP_BUF_ALLOC_TIMEOUT	K_MSEC(100)

/** Protocol handler information. */
struct ppp_protocol_handler {
	/** Protocol init function */
	void (*init)(struct ppp_context *ctx);

	/** Process a received packet */
	enum net_verdict (*handler)(struct ppp_context *ctx,
				    struct net_if *iface,
				    struct net_pkt *pkt);

	/** Lower layer up */
	void (*lower_up)(struct ppp_context *ctx);

	/** Lower layer down */
	void (*lower_down)(struct ppp_context *ctx);

	/** Enable this protocol */
	void (*open)(struct ppp_context *ctx);

	/** Disable this protocol */
	void (*close)(struct ppp_context *ctx, const uint8_t *reason);

	/** PPP protocol number */
	uint16_t protocol;
};

struct ppp_peer_option_info {
	uint8_t code;
	int (*parse)(struct ppp_fsm *fsm, struct net_pkt *pkt,
		     void *user_data);
	int (*nack)(struct ppp_fsm *fsm, struct net_pkt *ret_pkt,
		    void *user_data);
};

#define PPP_PEER_OPTION(_code, _parse, _nack)	\
	{					\
		.code = _code,			\
		.parse = _parse,		\
		.nack = _nack,			\
	}

int ppp_config_info_req(struct ppp_fsm *fsm,
			struct net_pkt *pkt,
			uint16_t length,
			struct net_pkt *ret_pkt,
			enum ppp_protocol_type protocol,
			const struct ppp_peer_option_info *options_info,
			size_t num_options_info,
			void *user_data);

#define PPP_PROTO_GET_NAME(proto_name)		\
	(ppp_protocol_handler_##proto_name)

#define PPP_PROTOCOL_REGISTER(name, proto, init_func, proto_handler,	\
			      proto_lower_up, proto_lower_down,		\
			      proto_open, proto_close)			\
	static const STRUCT_SECTION_ITERABLE(ppp_protocol_handler,	\
					PPP_PROTO_GET_NAME(name)) = {	\
		.protocol = proto,					\
		.init = init_func,					\
		.handler = proto_handler,				\
		.lower_up = proto_lower_up,				\
		.lower_down = proto_lower_down,				\
		.open = proto_open,					\
		.close = proto_close,					\
	}

void ppp_queue_pkt(struct net_pkt *pkt);
const char *ppp_phase_str(enum ppp_phase phase);
const char *ppp_state_str(enum ppp_state state);
const char *ppp_proto2str(uint16_t proto);
const char *ppp_pkt_type2str(enum ppp_packet_type type);
const char *ppp_option2str(enum ppp_protocol_type protocol, int type);
void ppp_fsm_name_set(struct ppp_fsm *fsm, const char *name);

#if CONFIG_NET_L2_PPP_LOG_LEVEL < LOG_LEVEL_DBG
void ppp_change_phase(struct ppp_context *ctx, enum ppp_phase new_phase);
void ppp_change_state(struct ppp_fsm *fsm, enum ppp_state new_state);
#else
void ppp_change_phase_debug(struct ppp_context *ctx,
			    enum ppp_phase new_phase,
			    const char *caller, int line);

#define ppp_change_phase(ctx, state)				\
	ppp_change_phase_debug(ctx, state, __func__, __LINE__)

#define ppp_change_state(fsm, state)				\
	ppp_change_state_debug(fsm, state, __func__, __LINE__)

void ppp_change_state_debug(struct ppp_fsm *fsm, enum ppp_state new_state,
			    const char *caller, int line);
#endif

struct ppp_context *ppp_fsm_ctx(struct ppp_fsm *fsm);
struct net_if *ppp_fsm_iface(struct ppp_fsm *fsm);
int ppp_send_pkt(struct ppp_fsm *fsm, struct net_if *iface,
		 enum ppp_packet_type type, uint8_t id,
		 void *data, uint32_t data_len);
void ppp_send_proto_rej(struct net_if *iface, struct net_pkt *pkt,
			uint16_t protocol);

void ppp_fsm_init(struct ppp_fsm *fsm, uint16_t protocol);
void ppp_fsm_lower_up(struct ppp_fsm *fsm);
void ppp_fsm_lower_down(struct ppp_fsm *fsm);
void ppp_fsm_open(struct ppp_fsm *fsm);
void ppp_fsm_close(struct ppp_fsm *fsm, const uint8_t *reason);
void ppp_fsm_proto_reject(struct ppp_fsm *fsm);
enum net_verdict ppp_fsm_input(struct ppp_fsm *fsm, uint16_t proto,
			       struct net_pkt *pkt);
enum net_verdict ppp_fsm_recv_protocol_rej(struct ppp_fsm *fsm,
					   uint8_t id,
					   struct net_pkt *pkt);
enum net_verdict ppp_fsm_recv_echo_req(struct ppp_fsm *fsm,
				       uint8_t id,
				       struct net_pkt *pkt);
enum net_verdict ppp_fsm_recv_echo_reply(struct ppp_fsm *fsm,
					 uint8_t id,
					 struct net_pkt *pkt);
enum net_verdict ppp_fsm_recv_discard_req(struct ppp_fsm *fsm,
					  uint8_t id,
					  struct net_pkt *pkt);

const struct ppp_protocol_handler *ppp_lcp_get(void);
int ppp_parse_options(struct ppp_fsm *fsm, struct net_pkt *pkt,
		      uint16_t length,
		      int (*parse)(struct net_pkt *pkt, uint8_t code,
				   uint8_t len, void *user_data),
		      void *user_data);

void ppp_link_established(struct ppp_context *ctx, struct ppp_fsm *fsm);
void ppp_link_authenticated(struct ppp_context *ctx);
void ppp_link_terminated(struct ppp_context *ctx);
void ppp_link_down(struct ppp_context *ctx);
void ppp_link_needed(struct ppp_context *ctx);

void ppp_network_up(struct ppp_context *ctx, int proto);
void ppp_network_down(struct ppp_context *ctx, int proto);
void ppp_network_done(struct ppp_context *ctx, int proto);
void ppp_network_all_down(struct ppp_context *ctx);

struct ppp_my_option_info {
	uint8_t code;
	int (*conf_req_add)(struct ppp_context *ctx, struct net_pkt *pkt);
	int (*conf_ack_handle)(struct ppp_context *ctx, struct net_pkt *pkt,
			       uint8_t oplen);
	int (*conf_nak_handle)(struct ppp_context *ctx, struct net_pkt *pkt,
			       uint8_t oplen);
};

#define PPP_MY_OPTION(_code, _req_add, _handle_ack, _handle_nak)	\
	{								\
		.code = _code,						\
		.conf_req_add = _req_add,				\
		.conf_ack_handle = _handle_ack,				\
		.conf_nak_handle = _handle_nak,				\
	}

struct net_pkt *ppp_my_options_add(struct ppp_fsm *fsm, size_t packet_len);

int ppp_my_options_parse_conf_ack(struct ppp_fsm *fsm,
				  struct net_pkt *pkt,
				  uint16_t length);

int ppp_my_options_parse_conf_nak(struct ppp_fsm *fsm,
				  struct net_pkt *pkt,
				  uint16_t length);

int ppp_my_options_parse_conf_rej(struct ppp_fsm *fsm,
				  struct net_pkt *pkt,
				  uint16_t length);

uint32_t ppp_my_option_flags(struct ppp_fsm *fsm, uint8_t code);

static inline bool ppp_my_option_is_acked(struct ppp_fsm *fsm,
					  uint8_t code)
{
	return ppp_my_option_flags(fsm, code) & PPP_MY_OPTION_ACKED;
}
