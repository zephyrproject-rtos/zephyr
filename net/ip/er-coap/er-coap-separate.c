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
 *      CoAP module for separate responses.
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include "sys/cc.h"
#include <stdio.h>
#include <string.h>
#include "er-coap-separate.h"
#include "er-coap-transactions.h"

#define DEBUG DEBUG_NONE
#include "contiki/ip/uip-debug.h"

/*---------------------------------------------------------------------------*/
/*- Separate Response API ---------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/**
 * \brief Reject a request that would require a separate response with an error message
 *
 * When the server does not have enough resources left to store the information
 * for a separate response or otherwise cannot execute the resource handler,
 * this function will respond with 5.03 Service Unavailable. The client can
 * then retry later.
 */
void
coap_separate_reject()
{
  /* TODO: Accept string pointer for custom error message */
  erbium_status_code = SERVICE_UNAVAILABLE_5_03;
  coap_error_message = "AlreadyInUse";
}
/*----------------------------------------------------------------------------*/
/**
 * \brief Initiate a separate response with an empty ACK
 * \param request The request to accept
 * \param separate_store A pointer to the data structure that will store the
 *   relevant information for the response
 *
 * When the server does not have enough resources left to store the information
 * for a separate response or otherwise cannot execute the resource handler,
 * this function will respond with 5.03 Service Unavailable. The client can
 * then retry later.
 */
void
coap_separate_accept(void *request, coap_separate_t *separate_store)
{
  coap_packet_t *const coap_req = (coap_packet_t *)request;
  coap_transaction_t *const t = coap_get_transaction_by_mid(coap_req->mid);
  coap_context_t *coap_ctx = coap_get_context(request);

  PRINTF("Separate ACCEPT: /%.*s MID %u\n", coap_req->uri_path_len,
         coap_req->uri_path, coap_req->mid);
  if(t) {
    /* send separate ACK for CON */
    if(coap_req->type == COAP_TYPE_CON) {
      coap_packet_t ack[1];

      /* ACK with empty code (0) */
      coap_init_message(ack, COAP_TYPE_ACK, 0, coap_req->mid);
      /* serializing into IPBUF: Only overwrites header parts that are already parsed into the request struct */
      coap_send_message(coap_ctx, &UIP_IP_BUF(coap_ctx->buf)->srcipaddr,
			UIP_UDP_BUF(coap_ctx->buf)->srcport,
                        uip_appdata(coap_ctx->buf),
			coap_serialize_message(ack, uip_appdata(coap_ctx->buf)));
    }

    /* store remote address */
    uip_ipaddr_copy(&separate_store->addr, &t->addr);
    separate_store->port = t->port;

    /* store correct response type */
    separate_store->type =
      coap_req->type == COAP_TYPE_CON ? COAP_TYPE_CON : COAP_TYPE_NON;
    separate_store->mid = coap_get_mid(); /* if it was a NON, we burned one MID in the engine... */

    memcpy(separate_store->token, coap_req->token, coap_req->token_len);
    separate_store->token_len = coap_req->token_len;

    separate_store->block1_num = coap_req->block1_num;
    separate_store->block1_size = coap_req->block1_size;

    separate_store->block2_num = coap_req->block2_num;
    separate_store->block2_size = coap_req->block2_size > 0 ? MIN(COAP_MAX_BLOCK_SIZE, coap_req->block2_size) : COAP_MAX_BLOCK_SIZE;

    /* signal the engine to skip automatic response and clear transaction by engine */
    erbium_status_code = MANUAL_RESPONSE;
  } else {
    PRINTF("ERROR: Response transaction for separate request not found!\n");
    erbium_status_code = INTERNAL_SERVER_ERROR_5_00;
  }
}
/*----------------------------------------------------------------------------*/
void
coap_separate_resume(void *response, coap_separate_t *separate_store,
                     uint8_t code)
{
  coap_init_message(response, separate_store->type, code,
                    separate_store->mid);
  if(separate_store->token_len) {
    coap_set_token(response, separate_store->token,
                   separate_store->token_len);
  }
  if(separate_store->block1_size) {
    coap_set_header_block1(response, separate_store->block1_num,
                           0, separate_store->block1_size);
  }
}
/*---------------------------------------------------------------------------*/
