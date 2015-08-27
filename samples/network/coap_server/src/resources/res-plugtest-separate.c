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
 *      ETSI Plugtest resource
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#define DEBUG DEBUG_NULL

#include <string.h>
#include "rest-engine.h"
#include "er-coap.h"
#include "er-coap-transactions.h"
#include "er-coap-separate.h"
#include "er-plugtest.h"

static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_resume_handler(void);

PERIODIC_RESOURCE(res_plugtest_separate,
                  "title=\"Resource which cannot be served immediately and which cannot be acknowledged in a piggy-backed way\"",
                  res_get_handler,
                  NULL,
                  NULL,
                  NULL,
                  3 * CLOCK_SECOND,
                  res_resume_handler);

/* A structure to store the required information */
typedef struct application_separate_store {
  /* Provided by Erbium to store generic request information such as remote address and token. */
  coap_separate_t request_metadata;
  /* Add fields for addition information to be stored for finalizing, e.g.: */
  char buffer[MAX_PLUGFEST_PAYLOAD];
} application_separate_store_t;

static uint8_t separate_active = 0;
static application_separate_store_t separate_store[1];

void
res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  coap_packet_t *const coap_req = (coap_packet_t *)request;

  PRINTF("/separate       ");
  if(separate_active) {
    PRINTF("REJECTED ");
    coap_separate_reject();
  } else {
    PRINTF("STORED ");
    separate_active = 1;

    /* Take over and skip response by engine. */
    coap_separate_accept(request, &separate_store->request_metadata);
    /* Be aware to respect the Block2 option, which is also stored in the coap_separate_t. */

    snprintf(separate_store->buffer, MAX_PLUGFEST_PAYLOAD,
             "Type: %u\nCode: %u\nMID: %u", coap_req->type, coap_req->code,
             coap_req->mid);
  }

  PRINTF("(%s %u)\n", coap_req->type == COAP_TYPE_CON ? "CON" : "NON", coap_req->mid);
}
static void
res_resume_handler()
{
  if(separate_active) {
    PRINTF("/separate       ");
    coap_transaction_t *transaction = NULL;
    if((transaction = coap_new_transaction(separate_store->request_metadata.mid,
					   NULL,
                                           &separate_store->request_metadata.addr,
                                           separate_store->request_metadata.port))) {
      PRINTF(
        "RESPONSE (%s %u)\n", separate_store->request_metadata.type == COAP_TYPE_CON ? "CON" : "NON", separate_store->request_metadata.mid);

      coap_packet_t response[1]; /* This way the packet can be treated as pointer as usual. */

      /* Restore the request information for the response. */
      coap_separate_resume(response, &separate_store->request_metadata, CONTENT_2_05);

      REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
      coap_set_payload(response, separate_store->buffer,
                       strlen(separate_store->buffer));

      /*
       * Be aware to respect the Block2 option, which is also stored in the coap_separate_t.
       * As it is a critical option, this example resource pretends to handle it for compliance.
       */
      coap_set_header_block2(response,
                             separate_store->request_metadata.block2_num, 0,
                             separate_store->request_metadata.block2_size);

      /* Warning: No check for serialization error. */
      transaction->packet_len = coap_serialize_message(response,
                                                       transaction->packet);
      coap_send_transaction(transaction);
      /* The engine will clear the transaction (right after send for NON, after acked for CON). */

      separate_active = 0;
    } else {
      PRINTF("ERROR (transaction)\n");
    }
  } /* if (separate_active) */
}
