/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWM2M_ENGINE_H
#define LWM2M_ENGINE_H

#include "lwm2m_object.h"

#define ZOAP_RESPONSE_CODE_CLASS(x)	(x >> 5)
#define ZOAP_RESPONSE_CODE_DETAIL(x)	(x & 0x1F)

/* TODO: */
#define NOTIFY_OBSERVER(o, i, r)	lwm2m_notify_observer(o, i, r)
#define NOTIFY_OBSERVER_PATH(path)	lwm2m_notify_observer_path(path)

char *lwm2m_sprint_ip_addr(const struct sockaddr *addr);

int lwm2m_notify_observer(u16_t obj_id, u16_t obj_inst_id, u16_t res_id);
int lwm2m_notify_observer_path(struct lwm2m_obj_path *path);

void lwm2m_register_obj(struct lwm2m_engine_obj *obj);
void lwm2m_unregister_obj(struct lwm2m_engine_obj *obj);
struct lwm2m_engine_obj_field *
lwm2m_get_engine_obj_field(struct lwm2m_engine_obj *obj, int res_id);
int  lwm2m_create_obj_inst(u16_t obj_id, u16_t obj_inst_id,
			   struct lwm2m_engine_obj_inst **obj_inst);
int  lwm2m_delete_obj_inst(u16_t obj_id, u16_t obj_inst_id);
int  lwm2m_get_or_create_engine_obj(struct lwm2m_engine_context *context,
				    struct lwm2m_engine_obj_inst **obj_inst,
				    u8_t *created);

int lwm2m_init_message(struct net_context *net_ctx, struct zoap_packet *zpkt,
		       struct net_pkt **pkt, u8_t type, u8_t code, u16_t mid,
		       const u8_t *token, u8_t tkl);
struct zoap_pending *lwm2m_init_message_pending(struct zoap_packet *zpkt,
						struct sockaddr *addr,
						struct zoap_pending *zpendings,
						int num_zpendings);
void lwm2m_init_message_cleanup(struct net_pkt *pkt,
				struct zoap_pending *pending,
				struct zoap_reply *reply);

u16_t lwm2m_get_rd_data(u8_t *client_data, u16_t size);

int lwm2m_write_handler(struct lwm2m_engine_obj_inst *obj_inst,
			struct lwm2m_engine_res_inst *res,
			struct lwm2m_engine_obj_field *obj_field,
			struct lwm2m_engine_context *context);

int lwm2m_udp_sendto(struct net_pkt *pkt, const struct sockaddr *dst_addr);
void lwm2m_udp_receive(struct net_context *ctx, struct net_pkt *pkt,
		       struct zoap_pending *zpendings, int num_zpendings,
		       struct zoap_reply *zreplies, int num_zreplies,
		       bool handle_separate_response,
		       int (*udp_request_handler)(struct zoap_packet *request,
				struct zoap_packet *response,
				struct sockaddr *from_addr));

#endif /* LWM2M_ENGINE_H */
