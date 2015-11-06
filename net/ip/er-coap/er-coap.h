/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      An implementation of the Constrained Application Protocol (RFC).
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#ifndef ER_COAP_H_
#define ER_COAP_H_

#include <net/ip_buf.h>

#include <stddef.h> /* for size_t */
#include "contiki-net.h"
#include "er-coap-constants.h"
#include "er-coap-conf.h"
#include "er-coap-context.h"

/* sanity check for configured values */
#define COAP_MAX_PACKET_SIZE  (COAP_MAX_HEADER_SIZE + REST_MAX_CHUNK_SIZE)
#if COAP_MAX_PACKET_SIZE > (UIP_BUFSIZE - UIP_IPH_LEN - UIP_UDPH_LEN)
#error "UIP_CONF_BUFFER_SIZE too small for REST_MAX_CHUNK_SIZE"
#endif

#if NET_COAP_CONF_STATS
typedef struct net_coap_stats {
  uint32_t recv;
  uint32_t recv_err;
  uint32_t sent;
  uint32_t re_sent;
} net_coap_stats_t;

extern net_coap_stats_t net_coap_stats;
#endif /* NET_COAP_CONF_STATS */

/* use Erbium CoAP for the REST Engine. Must come before include of rest-engine.h. */
#ifndef REST
#define REST coap_rest_implementation
#endif
#include "rest-engine.h"

/* REST_MAX_CHUNK_SIZE can be different from 2^x so we need to get next lower 2^x for COAP_MAX_BLOCK_SIZE */
#ifndef COAP_MAX_BLOCK_SIZE
#define COAP_MAX_BLOCK_SIZE           (REST_MAX_CHUNK_SIZE < 32 ? 16 : \
                                       (REST_MAX_CHUNK_SIZE < 64 ? 32 : \
                                        (REST_MAX_CHUNK_SIZE < 128 ? 64 : \
                                         (REST_MAX_CHUNK_SIZE < 256 ? 128 : \
                                          (REST_MAX_CHUNK_SIZE < 512 ? 256 : \
                                          (REST_MAX_CHUNK_SIZE < 1024 ? 512 : \
                                          (REST_MAX_CHUNK_SIZE < 2048 ? 1024 : 2048)))))))
#endif /* COAP_MAX_BLOCK_SIZE */

/* direct access into the buffer */
#define UIP_IP_BUF(buf)   ((struct uip_ip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])
#if NETSTACK_CONF_WITH_IPV6
#define UIP_UDP_BUF(buf)  ((struct uip_udp_hdr *)&uip_buf(buf)[uip_l2_l3_hdr_len(buf)])
#else
#define UIP_UDP_BUF(buf)  ((struct uip_udp_hdr *)&uip_buf(buf)[UIP_LLH_LEN + UIP_IPH_LEN])
#endif

/* bitmap for set options */
enum { OPTION_MAP_SIZE = sizeof(uint8_t) * 8 };

#define SET_OPTION(packet, opt) ((packet)->options[opt / OPTION_MAP_SIZE] |= 1 << (opt % OPTION_MAP_SIZE))
#define IS_OPTION(packet, opt) ((packet)->options[opt / OPTION_MAP_SIZE] & (1 << (opt % OPTION_MAP_SIZE)))

/* parsed message struct */
typedef struct {
  uint8_t *buffer; /* pointer to CoAP header / incoming packet buffer / memory to serialize packet */

  uint8_t version;
  coap_message_type_t type;
  uint8_t code;
  uint16_t mid;

  uint8_t token_len;
  uint8_t token[COAP_TOKEN_LEN];

  uint8_t options[COAP_OPTION_SIZE1 / OPTION_MAP_SIZE + 1]; /* bitmap to check if option is set */

  uint16_t content_format; /* parse options once and store; allows setting options in random order  */
  uint32_t max_age;
  uint8_t etag_len;
  uint8_t etag[COAP_ETAG_LEN];
  size_t proxy_uri_len;
  const char *proxy_uri;
  size_t proxy_scheme_len;
  const char *proxy_scheme;
  size_t uri_host_len;
  const char *uri_host;
  size_t location_path_len;
  const char *location_path;
  uint16_t uri_port;
  size_t location_query_len;
  const char *location_query;
  size_t uri_path_len;
  const char *uri_path;
  int32_t observe;
  uint16_t accept;
  uint8_t if_match_len;
  uint8_t if_match[COAP_ETAG_LEN];
  uint32_t block2_num;
  uint8_t block2_more;
  uint16_t block2_size;
  uint32_t block2_offset;
  uint32_t block1_num;
  uint8_t block1_more;
  uint16_t block1_size;
  uint32_t block1_offset;
  uint32_t size2;
  uint32_t size1;
  size_t uri_query_len;
  const char *uri_query;
  uint8_t if_none_match;

  coap_context_t *coap_ctx;

  uint16_t payload_len;
  uint8_t *payload;
} coap_packet_t;

/* option format serialization */
#define COAP_SERIALIZE_INT_OPTION(number, field, text) \
  if(IS_OPTION(coap_pkt, number)) { \
    PRINTF(text " [%u]\n", (unsigned int) coap_pkt->field);		\
    option += coap_serialize_int_option(number, current_number, option, coap_pkt->field); \
    current_number = number; \
  }
#define COAP_SERIALIZE_BYTE_OPTION(number, field, text) \
  if(IS_OPTION(coap_pkt, number)) { \
    PRINTF(text " %u [0x%02X%02X%02X%02X%02X%02X%02X%02X]\n", (unsigned int) coap_pkt->field##_len, \
           coap_pkt->field[0], \
           coap_pkt->field[1], \
           coap_pkt->field[2], \
           coap_pkt->field[3], \
           coap_pkt->field[4], \
           coap_pkt->field[5], \
           coap_pkt->field[6], \
           coap_pkt->field[7] \
           ); /* FIXME always prints 8 bytes */ \
    option += coap_serialize_array_option(number, current_number, option, coap_pkt->field, coap_pkt->field##_len, '\0'); \
    current_number = number; \
  }
#define COAP_SERIALIZE_STRING_OPTION(number, field, splitter, text) \
  if(IS_OPTION(coap_pkt, number)) { \
    PRINTF(text " [%.*s]\n", (int) coap_pkt->field##_len, coap_pkt->field); \
    option += coap_serialize_array_option(number, current_number, option, (uint8_t *)coap_pkt->field, coap_pkt->field##_len, splitter); \
    current_number = number; \
  }
#define COAP_SERIALIZE_BLOCK_OPTION(number, field, text) \
  if(IS_OPTION(coap_pkt, number)) \
  { \
    PRINTF(text " [%lu%s (%u B/blk)]\n", (unsigned long)coap_pkt->field##_num, coap_pkt->field##_more ? "+" : "", coap_pkt->field##_size); \
    uint32_t block = coap_pkt->field##_num << 4; \
    if(coap_pkt->field##_more) { block |= 0x8; } \
    block |= 0xF & coap_log_2(coap_pkt->field##_size / 16); \
    PRINTF(text " encoded: 0x%lX\n", (unsigned long)block);             \
    option += coap_serialize_int_option(number, current_number, option, block); \
    current_number = number; \
  }

/* to store error code and human-readable payload */
extern coap_status_t erbium_status_code;
extern char *coap_error_message;

coap_context_t *coap_init_connection(uip_ipaddr_t *server_addr,
				     uint16_t server_port,
				     uip_ipaddr_t *peer_addr,
				     uint16_t peer_port);
uint16_t coap_get_mid(void);

void coap_init_message(void *packet, coap_message_type_t type, uint8_t code,
                       uint16_t mid);
size_t coap_serialize_message(void *packet, uint8_t *buffer);
void coap_send_message(coap_context_t *coap_ctx,
                       uip_ipaddr_t *addr, uint16_t port,
                       const uint8_t *data,
                       uint16_t length);
coap_status_t coap_parse_message(void *request, uint8_t *data,
                                 uint16_t data_len);

int coap_get_query_variable(void *packet, const char *name,
                            const char **output);
int coap_get_post_variable(void *packet, const char *name,
                           const char **output);

/*---------------------------------------------------------------------------*/
int coap_is_secure(void *packet);
coap_context_t *coap_get_context(void *packet);
int coap_set_context(void *packet, coap_context_t *coap_ctx);
/*---------------------------------------------------------------------------*/

int coap_set_status_code(void *packet, unsigned int code);

int coap_set_token(void *packet, const uint8_t *token, size_t token_len);

int coap_get_header_content_format(void *packet, unsigned int *format);
int coap_set_header_content_format(void *packet, unsigned int format);

int coap_get_header_accept(void *packet, unsigned int *accept);
int coap_set_header_accept(void *packet, unsigned int accept);

int coap_get_header_max_age(void *packet, uint32_t *age);
int coap_set_header_max_age(void *packet, uint32_t age);

int coap_get_header_etag(void *packet, const uint8_t **etag);
int coap_set_header_etag(void *packet, const uint8_t *etag, size_t etag_len);

int coap_get_header_if_match(void *packet, const uint8_t **etag);
int coap_set_header_if_match(void *packet, const uint8_t *etag,
                             size_t etag_len);

int coap_get_header_if_none_match(void *packet);
int coap_set_header_if_none_match(void *packet);

int coap_get_header_proxy_uri(void *packet, const char **uri); /* in-place string might not be 0-terminated. */
int coap_set_header_proxy_uri(void *packet, const char *uri);

int coap_get_header_proxy_scheme(void *packet, const char **scheme); /* in-place string might not be 0-terminated. */
int coap_set_header_proxy_scheme(void *packet, const char *scheme);

int coap_get_header_uri_host(void *packet, const char **host); /* in-place string might not be 0-terminated. */
int coap_set_header_uri_host(void *packet, const char *host);

int coap_get_header_uri_path(void *packet, const char **path); /* in-place string might not be 0-terminated. */
int coap_set_header_uri_path(void *packet, const char *path);

int coap_get_header_uri_query(void *packet, const char **query); /* in-place string might not be 0-terminated. */
int coap_set_header_uri_query(void *packet, const char *query);

int coap_get_header_location_path(void *packet, const char **path); /* in-place string might not be 0-terminated. */
int coap_set_header_location_path(void *packet, const char *path); /* also splits optional query into Location-Query option. */

int coap_get_header_location_query(void *packet, const char **query); /* in-place string might not be 0-terminated. */
int coap_set_header_location_query(void *packet, const char *query);

int coap_get_header_observe(void *packet, uint32_t *observe);
int coap_set_header_observe(void *packet, uint32_t observe);

int coap_get_header_block2(void *packet, uint32_t *num, uint8_t *more,
                           uint16_t *size, uint32_t *offset);
int coap_set_header_block2(void *packet, uint32_t num, uint8_t more,
                           uint16_t size);

int coap_get_header_block1(void *packet, uint32_t *num, uint8_t *more,
                           uint16_t *size, uint32_t *offset);
int coap_set_header_block1(void *packet, uint32_t num, uint8_t more,
                           uint16_t size);

int coap_get_header_size2(void *packet, uint32_t *size);
int coap_set_header_size2(void *packet, uint32_t size);

int coap_get_header_size1(void *packet, uint32_t *size);
int coap_set_header_size1(void *packet, uint32_t size);

int coap_get_payload(void *packet, const uint8_t **payload);
int coap_set_payload(void *packet, const void *payload, size_t length);

void coap_init_mid(void);

#endif /* ER_COAP_H_ */
