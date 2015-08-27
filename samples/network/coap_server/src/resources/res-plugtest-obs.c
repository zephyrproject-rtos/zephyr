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
#include "er-coap-observe.h"
#include "er-plugtest.h"

static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_put_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_delete_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_periodic_handler(void);

PERIODIC_RESOURCE(res_plugtest_obs,
                  "title=\"Observable resource which changes every 5 seconds\";obs",
                  res_get_handler,
                  NULL,
                  res_put_handler,
                  res_delete_handler,
                  5 * CLOCK_SECOND,
                  res_periodic_handler);

static int32_t obs_counter = 0;
static char obs_content[MAX_PLUGFEST_BODY];
static size_t obs_content_len = 0;
static unsigned int obs_format = 0;

static char obs_status = 0;

static void
obs_purge_list()
{
  PRINTF("### SERVER ACTION ### Purging obs list");
  coap_remove_observer_by_uri(NULL, 0, res_plugtest_obs.url);
}
static void
res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  /* Keep server log clean from ticking events */
  if(request != NULL) {
    PRINTF("/obs            GET\n");
  }
  REST.set_header_content_type(response, obs_format);
  REST.set_header_max_age(response, 5);

  if(obs_content_len) {
    REST.set_header_content_type(response, obs_format);
    REST.set_response_payload(response, obs_content, obs_content_len);
  } else {
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    REST.set_response_payload(response, obs_content,
                              snprintf(obs_content, MAX_PLUGFEST_PAYLOAD, "TICK %lu", obs_counter));
  }
  /* A post_handler that handles subscriptions will be called for periodic resources by the REST framework. */
}
static void
res_put_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  uint8_t *incoming = NULL;
  unsigned int ct = -1;

  REST.get_header_content_type(request, &ct);

  PRINTF("/obs            PUT\n");

  if(ct != obs_format) {
    obs_status = 1;
    obs_format = ct;
  } else {
    obs_content_len = REST.get_request_payload(request,
                                               (const uint8_t **)&incoming);
    memcpy(obs_content, incoming, obs_content_len);
    res_periodic_handler();
  }

  REST.set_response_status(response, REST.status.CHANGED);
}
static void
res_delete_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  PRINTF("/obs            DELETE\n");

  obs_status = 2;

  REST.set_response_status(response, REST.status.DELETED);
}
/*
 * Additionally, a handler function named [resource name]_handler must be implemented for each PERIODIC_RESOURCE.
 * It will be called by the REST manager process with the defined period.
 */
static void
res_periodic_handler()
{
  ++obs_counter;

  /* PRINTF("TICK %u for /%s\n", obs_counter, r->url); */

  if(obs_status == 1) {

    /* Notify the registered observers with the given message type, observe option, and payload. */
    REST.notify_subscribers(&res_plugtest_obs);

    PRINTF("######### sending 5.00\n");

    obs_purge_list();
  } else if(obs_status == 2) {

    /* Notify the registered observers with the given message type, observe option, and payload. */
    REST.notify_subscribers(&res_plugtest_obs);

    obs_purge_list();

    obs_counter = 0;
    obs_content_len = 0;
  } else {
    /* Notify the registered observers with the given message type, observe option, and payload. */
    REST.notify_subscribers(&res_plugtest_obs);
  } obs_status = 0;
}
