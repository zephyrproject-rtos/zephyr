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

#include <string.h>
#include "rest-engine.h"
#include "er-coap.h"
#include "er-plugtest.h"

static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_put_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

RESOURCE(res_plugtest_validate,
         "title=\"Validation test resource\"",
         res_get_handler,
         NULL,
         res_put_handler,
         NULL);

static uint8_t validate_etag[8] = { 0 };
static uint8_t validate_etag_len = 1;
static uint8_t validate_change = 1;

static const uint8_t *bytes = NULL;
static size_t len = 0;

static void
validate_update_etag()
{
  int i;
  validate_etag_len = (random_rand() % 8) + 1;
  for(i = 0; i < validate_etag_len; ++i) {
    validate_etag[i] = random_rand();
  }
  validate_change = 0;

  PRINTF("### SERVER ACTION ### Changed ETag %u [0x%02X%02X%02X%02X%02X%02X%02X%02X]\n",
         validate_etag_len, validate_etag[0], validate_etag[1], validate_etag[2], validate_etag[3], validate_etag[4], validate_etag[5], validate_etag[6], validate_etag[7]);
}
static void
res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  coap_packet_t *const coap_req = (coap_packet_t *)request;

  if(validate_change) {
    validate_update_etag();
  }
  PRINTF("/validate       GET");
  PRINTF("(%s %u)\n", coap_req->type == COAP_TYPE_CON ? "CON" : "NON", coap_req->mid);

  if((len = coap_get_header_etag(request, &bytes)) > 0
     && len == validate_etag_len && memcmp(validate_etag, bytes, len) == 0) {
    PRINTF("validate ");
    REST.set_response_status(response, REST.status.NOT_MODIFIED);
    REST.set_header_etag(response, validate_etag, validate_etag_len);

    validate_change = 1;
    PRINTF("### SERVER ACTION ### Resouce will change\n");
  } else {
    /* Code 2.05 CONTENT is default. */
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    REST.set_header_etag(response, validate_etag, validate_etag_len);
    REST.set_header_max_age(response, 30);
    REST.set_response_payload(
      response,
      buffer,
      snprintf((char *)buffer, MAX_PLUGFEST_PAYLOAD,
               "Type: %u\nCode: %u\nMID: %u", coap_req->type, coap_req->code,
               coap_req->mid));
  }
}
static void
res_put_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
#if DEBUG > 0
  coap_packet_t *const coap_req = (coap_packet_t *)request;

  PRINTF("/validate       PUT ");
  PRINTF("(%s %u)\n", coap_req->type == COAP_TYPE_CON ? "CON" : "NON", coap_req->mid);
#endif

  if(((len = coap_get_header_if_match(request, &bytes)) > 0
      && (len == validate_etag_len
          && memcmp(validate_etag, bytes, len) == 0))
     || len == 0) {
    validate_update_etag();
    REST.set_header_etag(response, validate_etag, validate_etag_len);

    REST.set_response_status(response, REST.status.CHANGED);

    if(len > 0) {
      validate_change = 1;
      PRINTF("### SERVER ACTION ### Resouce will change\n");
    }
  } else {
    PRINTF(
      "Check %u/%u\n  [0x%02X%02X%02X%02X%02X%02X%02X%02X]\n  [0x%02X%02X%02X%02X%02X%02X%02X%02X] ",
      len,
      validate_etag_len,
      bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
      validate_etag[0], validate_etag[1], validate_etag[2], validate_etag[3], validate_etag[4], validate_etag[5], validate_etag[6], validate_etag[7]);

    REST.set_response_status(response, PRECONDITION_FAILED_4_12);
  }
}
