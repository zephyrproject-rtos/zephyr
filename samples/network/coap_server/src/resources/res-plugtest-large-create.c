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

static void res_post_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/*
 * Large resource that can be created using POST method
 */
RESOURCE(res_plugtest_large_create,
         "title=\"Large resource that can be created using POST method\";rt=\"block\"",
         NULL,
         res_post_handler,
         NULL,
         NULL);

static void
res_post_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  coap_packet_t *const coap_req = (coap_packet_t *)request;

  uint8_t *incoming = NULL;
  size_t len = 0;

  unsigned int ct = -1;

  if(!REST.get_header_content_type(request, &ct)) {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
    const char *error_msg = "NoContentType";
    REST.set_response_payload(response, error_msg, strlen(error_msg));
    return;
  }

  if((len = REST.get_request_payload(request, (const uint8_t **)&incoming))) {
    if(coap_req->block1_num * coap_req->block1_size + len <= 2048) {
      REST.set_response_status(response, REST.status.CREATED);
      REST.set_header_location(response, "/nirvana");
      coap_set_header_block1(response, coap_req->block1_num, 0,
                             coap_req->block1_size);
    } else {
      REST.set_response_status(response, REST.status.REQUEST_ENTITY_TOO_LARGE);
      const char *error_msg = "2048B max.";
      REST.set_response_payload(response, error_msg, strlen(error_msg));
      return;
    }
  } else {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
    const char *error_msg = "NoPayload";
    REST.set_response_payload(response, error_msg, strlen(error_msg));
    return;
  }
}
