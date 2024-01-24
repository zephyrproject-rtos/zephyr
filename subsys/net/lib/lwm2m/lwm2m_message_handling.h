/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LWM2M_MESSAGE_HANDLING_H
#define LWM2M_MESSAGE_HANDLING_H
#include "lwm2m_object.h"
#include "lwm2m_observation.h"

/* LWM2M / CoAP Content-Formats */
#define LWM2M_FORMAT_PLAIN_TEXT	      0
#define LWM2M_FORMAT_APP_LINK_FORMAT  40
#define LWM2M_FORMAT_APP_OCTET_STREAM 42
#define LWM2M_FORMAT_APP_EXI	      47
#define LWM2M_FORMAT_APP_JSON	      50
#define LWM2M_FORMAT_APP_CBOR	      60
#define LWM2M_FORMAT_APP_SEML_JSON    110
#define LWM2M_FORMAT_APP_SENML_CBOR   112
#define LWM2M_FORMAT_OMA_PLAIN_TEXT   1541
#define LWM2M_FORMAT_OMA_OLD_TLV      1542
#define LWM2M_FORMAT_OMA_OLD_JSON     1543
#define LWM2M_FORMAT_OMA_OLD_OPAQUE   1544
#define LWM2M_FORMAT_OMA_TLV	      11542
#define LWM2M_FORMAT_OMA_JSON	      11543
/* 65000 ~ 65535 inclusive are reserved for experiments */
#define LWM2M_FORMAT_NONE	      65535

#define COAP_RESPONSE_CODE_CLASS(x)  (x >> 5)
#define COAP_RESPONSE_CODE_DETAIL(x) (x & 0x1F)

/* coap reply status */
#define COAP_REPLY_STATUS_NONE	0
#define COAP_REPLY_STATUS_ERROR 1

#define NUM_BLOCK1_CONTEXT CONFIG_LWM2M_NUM_BLOCK1_CONTEXT

#if defined(CONFIG_LWM2M_COAP_BLOCK_TRANSFER)
#define NUM_OUTPUT_BLOCK_CONTEXT CONFIG_LWM2M_NUM_OUTPUT_BLOCK_CONTEXT
#endif

int coap_options_to_path(struct coap_option *opt, int options_count,
				struct lwm2m_obj_path *path);
/* LwM2M message functions */
struct lwm2m_message *lwm2m_get_message(struct lwm2m_ctx *client_ctx);
struct lwm2m_message *find_msg(struct coap_pending *pending, struct coap_reply *reply);
void lwm2m_reset_message(struct lwm2m_message *msg, bool release);
void lm2m_message_clear_allocations(struct lwm2m_message *msg);
int lwm2m_init_message(struct lwm2m_message *msg);
int lwm2m_send_message_async(struct lwm2m_message *msg);

void lwm2m_udp_receive(struct lwm2m_ctx *client_ctx, uint8_t *buf, uint16_t buf_len,
		       struct sockaddr *from_addr);

int generate_notify_message(struct lwm2m_ctx *ctx, struct observe_node *obs, void *user_data);
/* Notification and Send operation */
int lwm2m_information_interface_send(struct lwm2m_message *msg);
int lwm2m_send_empty_ack(struct lwm2m_ctx *client_ctx, uint16_t mid);

int lwm2m_register_payload_handler(struct lwm2m_message *msg);

int lwm2m_perform_read_op(struct lwm2m_message *msg, uint16_t content_format);

int lwm2m_perform_composite_read_op(struct lwm2m_message *msg, uint16_t content_format,
				    sys_slist_t *lwm2m_path_list);

int do_composite_read_op_for_parsed_list(struct lwm2m_message *msg, uint16_t content_format,
					 sys_slist_t *path_list);

int lwm2m_discover_handler(struct lwm2m_message *msg, bool is_bootstrap);

int lwm2m_write_handler(struct lwm2m_engine_obj_inst *obj_inst, struct lwm2m_engine_res *res,
			struct lwm2m_engine_res_inst *res_inst,
			struct lwm2m_engine_obj_field *obj_field, struct lwm2m_message *msg);

enum coap_block_size lwm2m_default_block_size(void);

int lwm2m_parse_peerinfo(char *url, struct lwm2m_ctx *client_ctx, bool is_firmware_uri);
void lwm2m_clear_block_contexts(void);

static inline bool lwm2m_outgoing_is_part_of_blockwise(struct lwm2m_message *msg)
{
	return msg->block_send;
}

#endif /* LWM2M_MESSAGE_HANDLING_H */
