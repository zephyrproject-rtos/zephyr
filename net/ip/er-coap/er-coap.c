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

#include <string.h>
#include <stdio.h>
#include "contiki.h"
#include "sys/cc.h"
#include "contiki-net.h"

#include "er-coap.h"
#include "er-coap-transactions.h"

#if CONFIG_NETWORK_IP_STACK_DEBUG_COAP_INTERNAL
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

/*---------------------------------------------------------------------------*/
/*- Variables ---------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
//static struct uip_udp_conn *udp_conn = NULL;
static uint16_t current_mid = 0;

coap_status_t erbium_status_code = NO_ERROR;
char *coap_error_message = "";
/*---------------------------------------------------------------------------*/
/*- Local helper functions --------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static uint16_t
coap_log_2(uint16_t value)
{
  uint16_t result = 0;

  do {
    value = value >> 1;
    result++;
  } while(value);

  return result ? result - 1 : result;
}
/*---------------------------------------------------------------------------*/
static uint32_t
coap_parse_int_option(uint8_t *bytes, size_t length)
{
  uint32_t var = 0;
  int i = 0;

  while(i < length) {
    var <<= 8;
    var |= bytes[i++];
  }
  return var;
}
/*---------------------------------------------------------------------------*/
static uint8_t
coap_option_nibble(unsigned int value)
{
  if(value < 13) {
    return value;
  } else if(value <= 0xFF + 13) {
    return 13;
  } else {
    return 14;
  }
}
/*---------------------------------------------------------------------------*/
static size_t
coap_set_option_header(unsigned int delta, size_t length, uint8_t *buffer)
{
  size_t written = 0;

  buffer[0] = coap_option_nibble(delta) << 4 | coap_option_nibble(length);

  if(delta > 268) {
    buffer[++written] = ((delta - 269) >> 8) & 0xff;
    buffer[++written] = (delta - 269) & 0xff;
  } else if(delta > 12) {
    buffer[++written] = (delta - 13);
  }

  if(length > 268) {
    buffer[++written] = ((length - 269) >> 8) & 0xff;
    buffer[++written] = (length - 269) & 0xff;
  } else if(length > 12) {
    buffer[++written] = (length - 13);
  }

  PRINTF("WRITTEN %u B opt header\n", (unsigned int) (1 + written));

  return ++written;
}
/*---------------------------------------------------------------------------*/
static size_t
coap_serialize_int_option(unsigned int number, unsigned int current_number,
                          uint8_t *buffer, uint32_t value)
{
  size_t i = 0;

  if(0xFF000000 & value) {
    ++i;
  }
  if(0xFFFF0000 & value) {
    ++i;
  }
  if(0xFFFFFF00 & value) {
    ++i;
  }
  if(0xFFFFFFFF & value) {
    ++i;
  }
  PRINTF("OPTION %u (delta %u, len %u)\n", number, number - current_number,
         (unsigned int) i);

  i = coap_set_option_header(number - current_number, i, buffer);

  if(0xFF000000 & value) {
    buffer[i++] = (uint8_t)(value >> 24);
  }
  if(0xFFFF0000 & value) {
    buffer[i++] = (uint8_t)(value >> 16);
  }
  if(0xFFFFFF00 & value) {
    buffer[i++] = (uint8_t)(value >> 8);
  }
  if(0xFFFFFFFF & value) {
    buffer[i++] = (uint8_t)(value);
  }
  return i;
}
/*---------------------------------------------------------------------------*/
static size_t
coap_serialize_array_option(unsigned int number, unsigned int current_number,
                            uint8_t *buffer, uint8_t *array, size_t length,
                            char split_char)
{
  size_t i = 0;

  PRINTF("ARRAY type %u, len %u, full [%.*s]\n", number, (unsigned int) length,
	 (int) length, array);

  if(split_char != '\0') {
    int j;
    uint8_t *part_start = array;
    uint8_t *part_end = NULL;
    size_t temp_length;

    for(j = 0; j <= length + 1; ++j) {
      PRINTF("STEP %u/%u (%c)\n", j, (unsigned int) length, array[j]);
      if(array[j] == split_char || j == length) {
        part_end = array + j;
        temp_length = part_end - part_start;

        i += coap_set_option_header(number - current_number, temp_length,
                                    &buffer[i]);
        memcpy(&buffer[i], part_start, temp_length);
        i += temp_length;

        PRINTF("OPTION type %u, delta %u, len %u, part [%.*s]\n", number,
               number - current_number, (unsigned int) i,
	       (unsigned int) temp_length, part_start);

        ++j;                    /* skip the splitter */
        current_number = number;
        part_start = array + j;
      }
    }                           /* for */
  } else {
    i += coap_set_option_header(number - current_number, length, &buffer[i]);
    memcpy(&buffer[i], array, length);
    i += length;

    PRINTF("OPTION type %u, delta %u, len %u\n", number,
           number - current_number, (unsigned int) length);
  }

  return i;
}
/*---------------------------------------------------------------------------*/
static void
coap_merge_multi_option(char **dst, size_t *dst_len, uint8_t *option,
                        size_t option_len, char separator)
{
  /* merge multiple options */
  if(*dst_len > 0) {
    /* dst already contains an option: concatenate */
    (*dst)[*dst_len] = separator;
    *dst_len += 1;

    /* memmove handles 2-byte option headers */
    memmove((*dst) + (*dst_len), option, option_len);

    *dst_len += option_len;
  } else {
    /* dst is empty: set to option */
    *dst = (char *)option;
    *dst_len = option_len;
  }
}
/*---------------------------------------------------------------------------*/
static int
coap_get_variable(const char *buffer, size_t length, const char *name,
                  const char **output)
{
  const char *start = NULL;
  const char *end = NULL;
  const char *value_end = NULL;
  size_t name_len = 0;

  /*initialize the output buffer first */
  *output = 0;

  name_len = strlen(name);
  end = buffer + length;

  for(start = buffer; start + name_len < end; ++start) {
    if((start == buffer || start[-1] == '&') && start[name_len] == '='
       && strncmp(name, start, name_len) == 0) {

      /* Point start to variable value */
      start += name_len + 1;

      /* Point end to the end of the value */
      value_end = (const char *)memchr(start, '&', end - start);
      if(value_end == NULL) {
        value_end = end;
      }
      *output = start;

      return value_end - start;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/*- Internal API ------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
void coap_init_mid(void)
{
  /* initialize transaction ID */
  current_mid = random_rand();
}

coap_context_t *
coap_init_connection(uip_ipaddr_t *server_addr, uint16_t server_port,
		     uip_ipaddr_t *peer_addr, uint16_t peer_port)
{
  coap_context_t *coap_ctx;

  if (!server_addr) {
    PRINTF("%s: local endpoint is missing\n", __func__);
    return NULL;
  }

#if defined(CONFIG_NETWORKING_WITH_IPV6)
  {
    const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;

    if (!memcmp(server_addr, &in6addr_any, sizeof(in6addr_any))) {
      uip_ds6_addr_t *uip_addr;

      uip_addr = uip_ds6_get_global(-1);
      if (!uip_addr) {
        uip_addr = uip_ds6_get_link_local(-1);
      }
      if (!uip_addr) {
        return NULL;
      }
      memcpy(server_addr, uip_addr, sizeof(*server_addr));
    }
  }
#endif

  coap_ctx = coap_context_new(server_addr, server_port);
  if (!coap_context_listen(coap_ctx, peer_addr, peer_port)) {
    return NULL;
  }

  coap_init_mid();

  return coap_ctx;
}
/*---------------------------------------------------------------------------*/
uint16_t
coap_get_mid()
{
  return ++current_mid;
}
/*---------------------------------------------------------------------------*/
void
coap_init_message(void *packet, coap_message_type_t type, uint8_t code,
                  uint16_t mid)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  /* Important thing */
  memset(coap_pkt, 0, sizeof(coap_packet_t));

  coap_pkt->type = type;
  coap_pkt->code = code;
  coap_pkt->mid = mid;
}
/*---------------------------------------------------------------------------*/
size_t
coap_serialize_message(void *packet, uint8_t *buffer)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;
  uint8_t *option;
  unsigned int current_number = 0;

  /* Initialize */
  coap_pkt->buffer = buffer;
  coap_pkt->version = 1;

  PRINTF("-Serializing MID %u to %p, ", coap_pkt->mid, coap_pkt->buffer);

  /* set header fields */
  coap_pkt->buffer[0] = 0x00;
  coap_pkt->buffer[0] |= COAP_HEADER_VERSION_MASK
    & (coap_pkt->version) << COAP_HEADER_VERSION_POSITION;
  coap_pkt->buffer[0] |= COAP_HEADER_TYPE_MASK
    & (coap_pkt->type) << COAP_HEADER_TYPE_POSITION;
  coap_pkt->buffer[0] |= COAP_HEADER_TOKEN_LEN_MASK
    & (coap_pkt->token_len) << COAP_HEADER_TOKEN_LEN_POSITION;
  coap_pkt->buffer[1] = coap_pkt->code;
  coap_pkt->buffer[2] = (uint8_t)((coap_pkt->mid) >> 8);
  coap_pkt->buffer[3] = (uint8_t)(coap_pkt->mid);

  /* empty packet, dont need to do more stuff */
  if(!coap_pkt->code) {
    PRINTF("-Done serializing empty message at %p-\n", coap_pkt->buffer);
    return 4;
  }

  /* set Token */
  PRINTF("Token (len %u)", coap_pkt->token_len);
  option = coap_pkt->buffer + COAP_HEADER_LEN;
  for(current_number = 0; current_number < coap_pkt->token_len;
      ++current_number) {
    PRINTF(" %02X", coap_pkt->token[current_number]);
    *option = coap_pkt->token[current_number];
    ++option;
  }
  PRINTF("-\n");

  /* Serialize options */
  current_number = 0;

  PRINTF("-Serializing options at %p-\n", option);

  /* The options must be serialized in the order of their number */
  COAP_SERIALIZE_BYTE_OPTION(COAP_OPTION_IF_MATCH, if_match, "If-Match");
  COAP_SERIALIZE_STRING_OPTION(COAP_OPTION_URI_HOST, uri_host, '\0',
                               "Uri-Host");
  COAP_SERIALIZE_BYTE_OPTION(COAP_OPTION_ETAG, etag, "ETag");
  COAP_SERIALIZE_INT_OPTION(COAP_OPTION_IF_NONE_MATCH,
                            content_format -
                            coap_pkt->
                            content_format /* hack to get a zero field */,
                            "If-None-Match");
  COAP_SERIALIZE_INT_OPTION(COAP_OPTION_OBSERVE, observe, "Observe");
  COAP_SERIALIZE_INT_OPTION(COAP_OPTION_URI_PORT, uri_port, "Uri-Port");
  COAP_SERIALIZE_STRING_OPTION(COAP_OPTION_LOCATION_PATH, location_path, '/',
                               "Location-Path");
  COAP_SERIALIZE_STRING_OPTION(COAP_OPTION_URI_PATH, uri_path, '/',
                               "Uri-Path");
  PRINTF("Serialize content format: %d\n", coap_pkt->content_format);
  COAP_SERIALIZE_INT_OPTION(COAP_OPTION_CONTENT_FORMAT, content_format,
                            "Content-Format");
  COAP_SERIALIZE_INT_OPTION(COAP_OPTION_MAX_AGE, max_age, "Max-Age");
  COAP_SERIALIZE_STRING_OPTION(COAP_OPTION_URI_QUERY, uri_query, '&',
                               "Uri-Query");
  COAP_SERIALIZE_INT_OPTION(COAP_OPTION_ACCEPT, accept, "Accept");
  COAP_SERIALIZE_STRING_OPTION(COAP_OPTION_LOCATION_QUERY, location_query,
                               '&', "Location-Query");
  COAP_SERIALIZE_BLOCK_OPTION(COAP_OPTION_BLOCK2, block2, "Block2");
  COAP_SERIALIZE_BLOCK_OPTION(COAP_OPTION_BLOCK1, block1, "Block1");
  COAP_SERIALIZE_INT_OPTION(COAP_OPTION_SIZE2, size2, "Size2");
  COAP_SERIALIZE_STRING_OPTION(COAP_OPTION_PROXY_URI, proxy_uri, '\0',
                               "Proxy-Uri");
  COAP_SERIALIZE_STRING_OPTION(COAP_OPTION_PROXY_SCHEME, proxy_scheme, '\0',
                               "Proxy-Scheme");
  COAP_SERIALIZE_INT_OPTION(COAP_OPTION_SIZE1, size1, "Size1");

  PRINTF("-Done serializing at %p----\n", option);

  /* Pack payload */
  if((option - coap_pkt->buffer) <= COAP_MAX_HEADER_SIZE) {
    /* Payload marker */
    if(coap_pkt->payload_len) {
      *option = 0xFF;
      ++option;
    }
    if (!coap_pkt->payload) {
      if (coap_pkt->payload_len > 0) {
	coap_error_message = "Cannot serialize as payload is NULL";
	PRINTF("ERROR: %s but payload len is %d\n", coap_error_message,
	       coap_pkt->payload_len);
	goto error;
      }
    } else {
      memmove(option, coap_pkt->payload, coap_pkt->payload_len);
    }
  } else {
    /* an error occurred: caller must check for !=0 */
    coap_error_message = "Serialized header exceeds COAP_MAX_HEADER_SIZE";
  error:
    coap_pkt->buffer = NULL;
    return 0;
  }

  PRINTF("-Done %u B (header len %u, payload len %u)-\n",
         (unsigned int) (coap_pkt->payload_len + option - buffer),
	 (unsigned int) (option - buffer),
         (unsigned int) coap_pkt->payload_len);

  PRINTF("Dump [0x%02X %02X %02X %02X  %02X %02X %02X %02X]\n",
         coap_pkt->buffer[0],
         coap_pkt->buffer[1],
         coap_pkt->buffer[2],
         coap_pkt->buffer[3],
         coap_pkt->buffer[4],
         coap_pkt->buffer[5], coap_pkt->buffer[6], coap_pkt->buffer[7]
         );

  return (option - buffer) + coap_pkt->payload_len; /* packet length */
}
/*---------------------------------------------------------------------------*/
void
coap_send_message(coap_context_t *coap_ctx,
                  uip_ipaddr_t *addr, uint16_t port,
                  const uint8_t *data, uint16_t length)
{
  if(coap_ctx != COAP_CONTEXT_NONE) {
    coap_context_send_message(coap_ctx, addr, port, data, length);
    return;
  }

  /* configure connection to reply to client */
  uip_ipaddr_copy(&uip_udp_conn(coap_ctx->buf)->ripaddr, addr);
  uip_udp_conn(coap_ctx->buf)->rport = port;

  uip_udp_packet_send(coap_ctx->buf, uip_udp_conn(coap_ctx->buf), data, length);

  PRINTF("-sent UDP datagram (%u)-\n", length);

  /* restore server socket to allow data from any node */
  memset(&uip_udp_conn(coap_ctx->buf)->ripaddr, 0,
	 sizeof(uip_udp_conn(coap_ctx->buf)->ripaddr));
  uip_udp_conn(coap_ctx->buf)->rport = 0;
}
/*---------------------------------------------------------------------------*/
coap_status_t
coap_parse_message(void *packet, uint8_t *data, uint16_t data_len)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  /* initialize packet */
  memset(coap_pkt, 0, sizeof(coap_packet_t));

  /* pointer to packet bytes */
  coap_pkt->buffer = data;

  /* parse header fields */
  coap_pkt->version = (COAP_HEADER_VERSION_MASK & coap_pkt->buffer[0])
    >> COAP_HEADER_VERSION_POSITION;
  coap_pkt->type = (COAP_HEADER_TYPE_MASK & coap_pkt->buffer[0])
    >> COAP_HEADER_TYPE_POSITION;
  coap_pkt->token_len =
    MIN(COAP_TOKEN_LEN,
        (COAP_HEADER_TOKEN_LEN_MASK & coap_pkt->
         buffer[0]) >> COAP_HEADER_TOKEN_LEN_POSITION);
  coap_pkt->code = coap_pkt->buffer[1];
  coap_pkt->mid = coap_pkt->buffer[2] << 8 | coap_pkt->buffer[3];

  if(coap_pkt->version != 1) {
    coap_error_message = "CoAP version must be 1";
    return BAD_REQUEST_4_00;
  }

  uint8_t *current_option = data + COAP_HEADER_LEN;

  memcpy(coap_pkt->token, current_option, coap_pkt->token_len);
  PRINTF("Token (len %u) [0x%02X%02X%02X%02X%02X%02X%02X%02X]\n",
         coap_pkt->token_len, coap_pkt->token[0], coap_pkt->token[1],
         coap_pkt->token[2], coap_pkt->token[3], coap_pkt->token[4],
         coap_pkt->token[5], coap_pkt->token[6], coap_pkt->token[7]
         );                     /*FIXME always prints 8 bytes */

  /* parse options */
  memset(coap_pkt->options, 0, sizeof(coap_pkt->options));
  current_option += coap_pkt->token_len;

  unsigned int option_number = 0;
  unsigned int option_delta = 0;
  unsigned int option_length = 0;

  while(current_option < data + data_len) {
    /* payload marker 0xFF, currently only checking for 0xF* because rest is reserved */
    if((current_option[0] & 0xF0) == 0xF0) {
      coap_pkt->payload = ++current_option;
      coap_pkt->payload_len = data_len - (coap_pkt->payload - data);

      /* also for receiving, the Erbium upper bound is REST_MAX_CHUNK_SIZE */
      if(coap_pkt->payload_len > REST_MAX_CHUNK_SIZE) {
        coap_pkt->payload_len = REST_MAX_CHUNK_SIZE;
        /* null-terminate payload */
      }
      coap_pkt->payload[coap_pkt->payload_len] = '\0';

      break;
    }

    option_delta = current_option[0] >> 4;
    option_length = current_option[0] & 0x0F;
    ++current_option;

    if(option_delta == 13) {
      option_delta += current_option[0];
      ++current_option;
    } else if(option_delta == 14) {
      option_delta += 255;
      option_delta += current_option[0] << 8;
      ++current_option;
      option_delta += current_option[0];
      ++current_option;
    }

    if(option_length == 13) {
      option_length += current_option[0];
      ++current_option;
    } else if(option_length == 14) {
      option_length += 255;
      option_length += current_option[0] << 8;
      ++current_option;
      option_length += current_option[0];
      ++current_option;
    }

    option_number += option_delta;

    PRINTF("OPTION %u (delta %u, len %u): ", option_number, option_delta,
           option_length);

    SET_OPTION(coap_pkt, option_number);

    switch(option_number) {
    case COAP_OPTION_CONTENT_FORMAT:
      coap_pkt->content_format = coap_parse_int_option(current_option,
                                                       option_length);
      PRINTF("Content-Format [%u]\n", coap_pkt->content_format);
      break;
    case COAP_OPTION_MAX_AGE:
      coap_pkt->max_age = coap_parse_int_option(current_option,
                                                option_length);
      PRINTF("Max-Age [%lu]\n", (long unsigned int) coap_pkt->max_age);
      break;
    case COAP_OPTION_ETAG:
      coap_pkt->etag_len = MIN(COAP_ETAG_LEN, option_length);
      memcpy(coap_pkt->etag, current_option, coap_pkt->etag_len);
      PRINTF("ETag %u [0x%02X%02X%02X%02X%02X%02X%02X%02X]\n",
             coap_pkt->etag_len, coap_pkt->etag[0], coap_pkt->etag[1],
             coap_pkt->etag[2], coap_pkt->etag[3], coap_pkt->etag[4],
             coap_pkt->etag[5], coap_pkt->etag[6], coap_pkt->etag[7]
             );                 /*FIXME always prints 8 bytes */
      break;
    case COAP_OPTION_ACCEPT:
      coap_pkt->accept = coap_parse_int_option(current_option, option_length);
      PRINTF("Accept [%u]\n", coap_pkt->accept);
      break;
    case COAP_OPTION_IF_MATCH:
      /* TODO support multiple ETags */
      coap_pkt->if_match_len = MIN(COAP_ETAG_LEN, option_length);
      memcpy(coap_pkt->if_match, current_option, coap_pkt->if_match_len);
      PRINTF("If-Match %u [0x%02X%02X%02X%02X%02X%02X%02X%02X]\n",
             coap_pkt->if_match_len, coap_pkt->if_match[0],
             coap_pkt->if_match[1], coap_pkt->if_match[2],
             coap_pkt->if_match[3], coap_pkt->if_match[4],
             coap_pkt->if_match[5], coap_pkt->if_match[6],
             coap_pkt->if_match[7]
             ); /* FIXME always prints 8 bytes */
      break;
    case COAP_OPTION_IF_NONE_MATCH:
      coap_pkt->if_none_match = 1;
      PRINTF("If-None-Match\n");
      break;

    case COAP_OPTION_PROXY_URI:
#if COAP_PROXY_OPTION_PROCESSING
      coap_pkt->proxy_uri = (char *)current_option;
      coap_pkt->proxy_uri_len = option_length;
#endif
      PRINTF("Proxy-Uri NOT IMPLEMENTED [%.*s]\n", (int)coap_pkt->proxy_uri_len,
             coap_pkt->proxy_uri);
      coap_error_message = "This is a constrained server (Contiki)";
      return PROXYING_NOT_SUPPORTED_5_05;
      break;
    case COAP_OPTION_PROXY_SCHEME:
#if COAP_PROXY_OPTION_PROCESSING
      coap_pkt->proxy_scheme = (char *)current_option;
      coap_pkt->proxy_scheme_len = option_length;
#endif
      PRINTF("Proxy-Scheme NOT IMPLEMENTED [%.*s]\n",
             (int) coap_pkt->proxy_scheme_len, coap_pkt->proxy_scheme);
      coap_error_message = "This is a constrained server (Contiki)";
      return PROXYING_NOT_SUPPORTED_5_05;
      break;

    case COAP_OPTION_URI_HOST:
      coap_pkt->uri_host = (char *)current_option;
      coap_pkt->uri_host_len = option_length;
      PRINTF("Uri-Host [%.*s]\n", (int) coap_pkt->uri_host_len,
	     coap_pkt->uri_host);
      break;
    case COAP_OPTION_URI_PORT:
      coap_pkt->uri_port = coap_parse_int_option(current_option,
                                                 option_length);
      PRINTF("Uri-Port [%u]\n", coap_pkt->uri_port);
      break;
    case COAP_OPTION_URI_PATH:
      /* coap_merge_multi_option() operates in-place on the IPBUF, but final packet field should be const string -> cast to string */
      coap_merge_multi_option((char **)&(coap_pkt->uri_path),
                              &(coap_pkt->uri_path_len), current_option,
                              option_length, '/');
      PRINTF("Uri-Path [%.*s]\n", (int) coap_pkt->uri_path_len, coap_pkt->uri_path);
      break;
    case COAP_OPTION_URI_QUERY:
      /* coap_merge_multi_option() operates in-place on the IPBUF, but final packet field should be const string -> cast to string */
      coap_merge_multi_option((char **)&(coap_pkt->uri_query),
                              &(coap_pkt->uri_query_len), current_option,
                              option_length, '&');
      PRINTF("Uri-Query [%.*s]\n", (int) coap_pkt->uri_query_len,
             coap_pkt->uri_query);
      break;

    case COAP_OPTION_LOCATION_PATH:
      /* coap_merge_multi_option() operates in-place on the IPBUF, but final packet field should be const string -> cast to string */
      coap_merge_multi_option((char **)&(coap_pkt->location_path),
                              &(coap_pkt->location_path_len), current_option,
                              option_length, '/');
      PRINTF("Location-Path [%.*s]\n", (int) coap_pkt->location_path_len,
             coap_pkt->location_path);
      break;
    case COAP_OPTION_LOCATION_QUERY:
      /* coap_merge_multi_option() operates in-place on the IPBUF, but final packet field should be const string -> cast to string */
      coap_merge_multi_option((char **)&(coap_pkt->location_query),
                              &(coap_pkt->location_query_len), current_option,
                              option_length, '&');
      PRINTF("Location-Query [%.*s]\n", (int) coap_pkt->location_query_len,
             coap_pkt->location_query);
      break;

    case COAP_OPTION_OBSERVE:
      coap_pkt->observe = coap_parse_int_option(current_option,
                                                option_length);
      PRINTF("Observe [%lu]\n", (long unsigned int) coap_pkt->observe);
      break;
    case COAP_OPTION_BLOCK2:
      coap_pkt->block2_num = coap_parse_int_option(current_option,
                                                   option_length);
      coap_pkt->block2_more = (coap_pkt->block2_num & 0x08) >> 3;
      coap_pkt->block2_size = 16 << (coap_pkt->block2_num & 0x07);
      coap_pkt->block2_offset = (coap_pkt->block2_num & ~0x0000000F)
        << (coap_pkt->block2_num & 0x07);
      coap_pkt->block2_num >>= 4;
      PRINTF("Block2 [%lu%s (%u B/blk)]\n", (long unsigned int)
	     coap_pkt->block2_num,
             coap_pkt->block2_more ? "+" : "", coap_pkt->block2_size);
      break;
    case COAP_OPTION_BLOCK1:
      coap_pkt->block1_num = coap_parse_int_option(current_option,
                                                   option_length);
      coap_pkt->block1_more = (coap_pkt->block1_num & 0x08) >> 3;
      coap_pkt->block1_size = 16 << (coap_pkt->block1_num & 0x07);
      coap_pkt->block1_offset = (coap_pkt->block1_num & ~0x0000000F)
        << (coap_pkt->block1_num & 0x07);
      coap_pkt->block1_num >>= 4;
      PRINTF("Block1 [%lu%s (%u B/blk)]\n", (long unsigned int)
	coap_pkt->block1_num,
             coap_pkt->block1_more ? "+" : "", coap_pkt->block1_size);
      break;
    case COAP_OPTION_SIZE2:
      coap_pkt->size2 = coap_parse_int_option(current_option, option_length);
      PRINTF("Size2 [%lu]\n", (long unsigned int) coap_pkt->size2);
      break;
    case COAP_OPTION_SIZE1:
      coap_pkt->size1 = coap_parse_int_option(current_option, option_length);
      PRINTF("Size1 [%lu]\n", (long unsigned int) coap_pkt->size1);
      break;
    default:
      PRINTF("unknown (%u)\n", option_number);
      /* check if critical (odd) */
      if(option_number & 1) {
        coap_error_message = "Unsupported critical option";
        return BAD_OPTION_4_02;
      }
    }

    current_option += option_length;
  }                             /* for */
  PRINTF("-Done parsing-------\n");

  return NO_ERROR;
}
/*---------------------------------------------------------------------------*/
/*- REST Engine API ---------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
int
coap_get_query_variable(void *packet, const char *name, const char **output)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(IS_OPTION(coap_pkt, COAP_OPTION_URI_QUERY)) {
    return coap_get_variable(coap_pkt->uri_query, coap_pkt->uri_query_len,
                             name, output);
  }
  return 0;
}
int
coap_get_post_variable(void *packet, const char *name, const char **output)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(coap_pkt->payload_len) {
    return coap_get_variable((const char *)coap_pkt->payload,
                             coap_pkt->payload_len, name, output);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
coap_set_status_code(void *packet, unsigned int code)
{
  if(code <= 0xFF) {
    ((coap_packet_t *)packet)->code = (uint8_t)code;
    return 1;
  } else {
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
int
coap_set_token(void *packet, const uint8_t *token, size_t token_len)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  coap_pkt->token_len = MIN(COAP_TOKEN_LEN, token_len);
  memcpy(coap_pkt->token, token, coap_pkt->token_len);

  return coap_pkt->token_len;
}
/*---------------------------------------------------------------------------*/
/*- CoAP REST Implementation API --------------------------------------------*/
/*---------------------------------------------------------------------------*/
int
coap_get_header_content_format(void *packet, unsigned int *format)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_CONTENT_FORMAT)) {
    return 0;
  }
  *format = coap_pkt->content_format;
  return 1;
}
int
coap_set_header_content_format(void *packet, unsigned int format)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  coap_pkt->content_format = format;
  SET_OPTION(coap_pkt, COAP_OPTION_CONTENT_FORMAT);
  return 1;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_accept(void *packet, unsigned int *accept)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_ACCEPT)) {
    return 0;
  }
  *accept = coap_pkt->accept;
  return 1;
}
int
coap_set_header_accept(void *packet, unsigned int accept)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  coap_pkt->accept = accept;
  SET_OPTION(coap_pkt, COAP_OPTION_ACCEPT);
  return 1;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_max_age(void *packet, uint32_t *age)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_MAX_AGE)) {
    *age = COAP_DEFAULT_MAX_AGE;
  } else {
    *age = coap_pkt->max_age;
  } return 1;
}
int
coap_set_header_max_age(void *packet, uint32_t age)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  coap_pkt->max_age = age;
  SET_OPTION(coap_pkt, COAP_OPTION_MAX_AGE);
  return 1;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_etag(void *packet, const uint8_t **etag)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_ETAG)) {
    return 0;
  }
  *etag = coap_pkt->etag;
  return coap_pkt->etag_len;
}
int
coap_set_header_etag(void *packet, const uint8_t *etag, size_t etag_len)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  coap_pkt->etag_len = MIN(COAP_ETAG_LEN, etag_len);
  memcpy(coap_pkt->etag, etag, coap_pkt->etag_len);

  SET_OPTION(coap_pkt, COAP_OPTION_ETAG);
  return coap_pkt->etag_len;
}
/*---------------------------------------------------------------------------*/
/*FIXME support multiple ETags */
int
coap_get_header_if_match(void *packet, const uint8_t **etag)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_IF_MATCH)) {
    return 0;
  }
  *etag = coap_pkt->if_match;
  return coap_pkt->if_match_len;
}
int
coap_set_header_if_match(void *packet, const uint8_t *etag, size_t etag_len)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  coap_pkt->if_match_len = MIN(COAP_ETAG_LEN, etag_len);
  memcpy(coap_pkt->if_match, etag, coap_pkt->if_match_len);

  SET_OPTION(coap_pkt, COAP_OPTION_IF_MATCH);
  return coap_pkt->if_match_len;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_if_none_match(void *packet)
{
  return IS_OPTION((coap_packet_t *)packet,
                   COAP_OPTION_IF_NONE_MATCH) ? 1 : 0;
}
int
coap_set_header_if_none_match(void *packet)
{
  SET_OPTION((coap_packet_t *)packet, COAP_OPTION_IF_NONE_MATCH);
  return 1;
}
/*---------------------------------------------------------------------------*/
int
coap_is_secure(void *packet)
{
  return coap_context_is_secure(coap_get_context(packet));
}
/*---------------------------------------------------------------------------*/
coap_context_t *
coap_get_context(void *packet)
{
  coap_packet_t *p = packet;
  return p->coap_ctx;
}
/*---------------------------------------------------------------------------*/
int
coap_set_context(void *packet, coap_context_t *coap_ctx)
{
  coap_packet_t *p = packet;
  p->coap_ctx = coap_ctx;
  return 1;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_proxy_uri(void *packet, const char **uri)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_PROXY_URI)) {
    return 0;
  }
  *uri = coap_pkt->proxy_uri;
  return coap_pkt->proxy_uri_len;
}
int
coap_set_header_proxy_uri(void *packet, const char *uri)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  /*TODO Provide alternative that sets Proxy-Scheme and Uri-* options and provide er-coap-conf define */

  coap_pkt->proxy_uri = uri;
  coap_pkt->proxy_uri_len = strlen(uri);

  SET_OPTION(coap_pkt, COAP_OPTION_PROXY_URI);
  return coap_pkt->proxy_uri_len;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_uri_host(void *packet, const char **host)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_URI_HOST)) {
    return 0;
  }
  *host = coap_pkt->uri_host;
  return coap_pkt->uri_host_len;
}
int
coap_set_header_uri_host(void *packet, const char *host)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  coap_pkt->uri_host = host;
  coap_pkt->uri_host_len = strlen(host);

  SET_OPTION(coap_pkt, COAP_OPTION_URI_HOST);
  return coap_pkt->uri_host_len;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_uri_path(void *packet, const char **path)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_URI_PATH)) {
    return 0;
  }
  *path = coap_pkt->uri_path;
  return coap_pkt->uri_path_len;
}
int
coap_set_header_uri_path(void *packet, const char *path)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  while(path[0] == '/')
    ++path;

  coap_pkt->uri_path = path;
  coap_pkt->uri_path_len = strlen(path);

  SET_OPTION(coap_pkt, COAP_OPTION_URI_PATH);
  return coap_pkt->uri_path_len;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_uri_query(void *packet, const char **query)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_URI_QUERY)) {
    return 0;
  }
  *query = coap_pkt->uri_query;
  return coap_pkt->uri_query_len;
}
int
coap_set_header_uri_query(void *packet, const char *query)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  while(query[0] == '?')
    ++query;

  coap_pkt->uri_query = query;
  coap_pkt->uri_query_len = strlen(query);

  SET_OPTION(coap_pkt, COAP_OPTION_URI_QUERY);
  return coap_pkt->uri_query_len;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_location_path(void *packet, const char **path)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_LOCATION_PATH)) {
    return 0;
  }
  *path = coap_pkt->location_path;
  return coap_pkt->location_path_len;
}
int
coap_set_header_location_path(void *packet, const char *path)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  char *query;

  while(path[0] == '/')
    ++path;

  if((query = strchr(path, '?'))) {
    coap_set_header_location_query(packet, query + 1);
    coap_pkt->location_path_len = query - path;
  } else {
    coap_pkt->location_path_len = strlen(path);
  } coap_pkt->location_path = path;

  if(coap_pkt->location_path_len > 0) {
    SET_OPTION(coap_pkt, COAP_OPTION_LOCATION_PATH);
  }
  return coap_pkt->location_path_len;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_location_query(void *packet, const char **query)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_LOCATION_QUERY)) {
    return 0;
  }
  *query = coap_pkt->location_query;
  return coap_pkt->location_query_len;
}
int
coap_set_header_location_query(void *packet, const char *query)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  while(query[0] == '?')
    ++query;

  coap_pkt->location_query = query;
  coap_pkt->location_query_len = strlen(query);

  SET_OPTION(coap_pkt, COAP_OPTION_LOCATION_QUERY);
  return coap_pkt->location_query_len;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_observe(void *packet, uint32_t *observe)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_OBSERVE)) {
    return 0;
  }
  *observe = coap_pkt->observe;
  return 1;
}
int
coap_set_header_observe(void *packet, uint32_t observe)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  coap_pkt->observe = observe;
  SET_OPTION(coap_pkt, COAP_OPTION_OBSERVE);
  return 1;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_block2(void *packet, uint32_t *num, uint8_t *more,
                       uint16_t *size, uint32_t *offset)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_BLOCK2)) {
    return 0;
  }
  /* pointers may be NULL to get only specific block parameters */
  if(num != NULL) {
    *num = coap_pkt->block2_num;
  }
  if(more != NULL) {
    *more = coap_pkt->block2_more;
  }
  if(size != NULL) {
    *size = coap_pkt->block2_size;
  }
  if(offset != NULL) {
    *offset = coap_pkt->block2_offset;
  }
  return 1;
}
int
coap_set_header_block2(void *packet, uint32_t num, uint8_t more,
                       uint16_t size)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(size < 16) {
    return 0;
  }
  if(size > 2048) {
    return 0;
  }
  if(num > 0x0FFFFF) {
    return 0;
  }
  coap_pkt->block2_num = num;
  coap_pkt->block2_more = more ? 1 : 0;
  coap_pkt->block2_size = size;

  SET_OPTION(coap_pkt, COAP_OPTION_BLOCK2);
  return 1;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_block1(void *packet, uint32_t *num, uint8_t *more,
                       uint16_t *size, uint32_t *offset)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_BLOCK1)) {
    return 0;
  }
  /* pointers may be NULL to get only specific block parameters */
  if(num != NULL) {
    *num = coap_pkt->block1_num;
  }
  if(more != NULL) {
    *more = coap_pkt->block1_more;
  }
  if(size != NULL) {
    *size = coap_pkt->block1_size;
  }
  if(offset != NULL) {
    *offset = coap_pkt->block1_offset;
  }
  return 1;
}
int
coap_set_header_block1(void *packet, uint32_t num, uint8_t more,
                       uint16_t size)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(size < 16) {
    return 0;
  }
  if(size > 2048) {
    return 0;
  }
  if(num > 0x0FFFFF) {
    return 0;
  }
  coap_pkt->block1_num = num;
  coap_pkt->block1_more = more;
  coap_pkt->block1_size = size;

  SET_OPTION(coap_pkt, COAP_OPTION_BLOCK1);
  return 1;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_size2(void *packet, uint32_t *size)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_SIZE2)) {
    return 0;
  }
  *size = coap_pkt->size2;
  return 1;
}
int
coap_set_header_size2(void *packet, uint32_t size)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  coap_pkt->size2 = size;
  SET_OPTION(coap_pkt, COAP_OPTION_SIZE2);
  return 1;
}
/*---------------------------------------------------------------------------*/
int
coap_get_header_size1(void *packet, uint32_t *size)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(!IS_OPTION(coap_pkt, COAP_OPTION_SIZE1)) {
    return 0;
  }
  *size = coap_pkt->size1;
  return 1;
}
int
coap_set_header_size1(void *packet, uint32_t size)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  coap_pkt->size1 = size;
  SET_OPTION(coap_pkt, COAP_OPTION_SIZE1);
  return 1;
}
/*---------------------------------------------------------------------------*/
int
coap_get_payload(void *packet, const uint8_t **payload)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  if(coap_pkt->payload) {
    *payload = coap_pkt->payload;
    return coap_pkt->payload_len;
  } else {
    *payload = NULL;
    return 0;
  }
}
int
coap_set_payload(void *packet, const void *payload, size_t length)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  coap_pkt->payload = (uint8_t *)payload;
  coap_pkt->payload_len = MIN(REST_MAX_CHUNK_SIZE, length);

  return coap_pkt->payload_len;
}
/*---------------------------------------------------------------------------*/
