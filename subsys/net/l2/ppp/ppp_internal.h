/** @file
 @brief PPP private header

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/ppp.h>

/**
 * FSM flags that control how it operates.
 */
#define FSM_RESTART    BIT(0) /**< Treat 2nd OPEN as DOWN followed by UP    */

/**
 * PPP packet format.
 */
struct ppp_packet {
	u8_t code;
	u8_t id;
	u16_t length;
} __packed;

/** Timeout in milliseconds */
#define PPP_TIMEOUT K_SECONDS(3)

/** Max Terminate-Request transmissions */
#define MAX_TERMINATE_REQ  CONFIG_NET_L2_PPP_MAX_TERMINATE_REQ_RETRANSMITS

/** Max Configure-Request transmissions */
#define MAX_CONFIGURE_REQ CONFIG_NET_L2_PPP_MAX_CONFIGURE_REQ_RETRANSMITS

/** Max number of LCP options */
#define MAX_LCP_OPTIONS CONFIG_NET_L2_PPP_MAX_OPTIONS

/** Max number of IPCP options */
#define MAX_IPCP_OPTIONS 3

/** Max number of IPV6CP options */
#define MAX_IPV6CP_OPTIONS 1

/*
 * Special alignment is needed for ppp_protocol_handler. This is the
 * same issue as in net_if. See net_if.h __net_if_align for explanation.
 */
#define __ppp_proto_align __aligned(32)

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
	void (*close)(struct ppp_context *ctx, const u8_t *reason);

	/** PPP protocol number */
	u16_t protocol;
} __ppp_proto_align;

#define PPP_PROTO_GET_NAME(proto_name)		\
	(ppp_protocol_handler_##proto_name)

#define PPP_PROTOCOL_REGISTER(name, proto, init_func, proto_handler,	\
			      proto_lower_up, proto_lower_down,		\
			      proto_open, proto_close)			\
	static const struct ppp_protocol_handler			\
	(PPP_PROTO_GET_NAME(name)) __used				\
	__attribute__((__section__(".net_ppp_proto.data"))) = {		\
		.protocol = proto,					\
		.init = init_func,					\
		.handler = proto_handler,				\
		.lower_up = proto_lower_up,				\
		.lower_down = proto_lower_down,				\
		.open = proto_open,					\
		.close = proto_close,					\
	}

extern struct ppp_protocol_handler __net_ppp_proto_start[];
extern struct ppp_protocol_handler __net_ppp_proto_end[];

const char *ppp_phase_str(enum ppp_phase phase);
const char *ppp_state_str(enum ppp_state state);
const char *ppp_proto2str(u16_t proto);
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

struct net_buf *ppp_get_net_buf(struct net_buf *root_buf, u8_t len);
int ppp_send_pkt(struct ppp_fsm *fsm, struct net_if *iface,
		 enum ppp_packet_type type, u8_t id,
		 void *data, u32_t data_len);
void ppp_send_proto_rej(struct net_if *iface, struct net_pkt *pkt,
			u16_t protocol);

void ppp_fsm_init(struct ppp_fsm *fsm, u16_t protocol);
void ppp_fsm_lower_up(struct ppp_fsm *fsm);
void ppp_fsm_lower_down(struct ppp_fsm *fsm);
void ppp_fsm_open(struct ppp_fsm *fsm);
void ppp_fsm_close(struct ppp_fsm *fsm, const u8_t *reason);
void ppp_fsm_proto_reject(struct ppp_fsm *fsm);
enum net_verdict ppp_fsm_input(struct ppp_fsm *fsm, u16_t proto,
			       struct net_pkt *pkt);
enum net_verdict ppp_fsm_recv_protocol_rej(struct ppp_fsm *fsm,
					   u8_t id,
					   struct net_pkt *pkt);
enum net_verdict ppp_fsm_recv_echo_req(struct ppp_fsm *fsm,
				       u8_t id,
				       struct net_pkt *pkt);
enum net_verdict ppp_fsm_recv_echo_reply(struct ppp_fsm *fsm,
					 u8_t id,
					 struct net_pkt *pkt);
enum net_verdict ppp_fsm_recv_discard_req(struct ppp_fsm *fsm,
					  u8_t id,
					  struct net_pkt *pkt);

const struct ppp_protocol_handler *ppp_lcp_get(void);
enum net_verdict ppp_parse_options(struct ppp_fsm *fsm,
				   struct net_pkt *pkt,
				   u16_t length,
				   struct ppp_option_pkt options[],
				   int options_len);

void ppp_link_established(struct ppp_context *ctx, struct ppp_fsm *fsm);
void ppp_link_terminated(struct ppp_context *ctx);
void ppp_link_down(struct ppp_context *ctx);
void ppp_link_needed(struct ppp_context *ctx);

void ppp_network_up(struct ppp_context *ctx, int proto);
void ppp_network_down(struct ppp_context *ctx, int proto);
void ppp_network_done(struct ppp_context *ctx, int proto);
void ppp_network_all_down(struct ppp_context *ctx);
