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
 *      An abstraction layer for RESTful Web services (Erbium).
 *      Inspired by RESTful Contiki by Dogan Yazar.
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#ifndef REST_ENGINE_H_
#define REST_ENGINE_H_

#include <stdio.h>
#include "contiki.h"
#include "contiki-lib.h"
#include "rest-constants.h"

/* list of valid REST Enigne implementations */
#define REGISTERED_ENGINE_ERBIUM coap_rest_implementation
#define REGISTERED_ENGINE_HELIUM http_rest_implementation

/* sanity check for configured implementation */
#if !defined(REST) || (REST != REGISTERED_ENGINE_ERBIUM && REST != REGISTERED_ENGINE_HELIUM)
#error "Define a valid REST Engine implementation (REST define)!"
#endif

/*
 * The maximum buffer size that is provided for resource responses and must be respected due to the limited IP buffer.
 * Larger data must be handled by the resource and will be sent chunk-wise through a TCP stream or CoAP blocks.
 */
#ifndef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE     64
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* MIN */

struct resource_s;
struct periodic_resource_s;

/* signatures of handler functions */
typedef void (*restful_handler)(void *request, void *response,
                                uint8_t *buffer, uint16_t preferred_size,
                                int32_t *offset);
typedef void (*restful_final_handler)(struct resource_s *resource,
                                      void *request, void *response);
typedef void (*restful_periodic_handler)(void);
typedef void (*restful_response_handler)(void *data, void *response);
typedef void (*restful_trigger_handler)(void);

/* signature of the rest-engine service function */
typedef int (*service_callback_t)(void *request, void *response,
                                  uint8_t *buffer, uint16_t preferred_size,
                                  int32_t *offset);

/* data structure representing a resource in REST */
struct resource_s {
  struct resource_s *next;        /* for LIST, points to next resource defined */
  const char *url;                /*handled URL */
  rest_resource_flags_t flags;    /* handled RESTful methods */
  const char *attributes;         /* link-format attributes */
  restful_handler get_handler;    /* handler function */
  restful_handler post_handler;   /* handler function */
  restful_handler put_handler;    /* handler function */
  restful_handler delete_handler; /* handler function */
  union {
    struct periodic_resource_s *periodic; /* special data depending on flags */
    restful_trigger_handler trigger;
    restful_trigger_handler resume;
  };
};
typedef struct resource_s resource_t;

struct periodic_resource_s {
  struct periodic_resource_s *next; /* for LIST, points to next resource defined */
  const resource_t *resource;
  uint32_t period;
  struct etimer periodic_timer;
  const restful_periodic_handler periodic_handler;
};
typedef struct periodic_resource_s periodic_resource_t;

/*
 * Macro to define a RESTful resource.
 * Resources are statically defined for the sake of efficiency and better memory management.
 */
#define RESOURCE(name, attributes, get_handler, post_handler, put_handler, delete_handler) \
  resource_t name = { NULL, NULL, NO_FLAGS, attributes, get_handler, post_handler, put_handler, delete_handler, { NULL } }

#define PARENT_RESOURCE(name, attributes, get_handler, post_handler, put_handler, delete_handler) \
  resource_t name = { NULL, NULL, HAS_SUB_RESOURCES, attributes, get_handler, post_handler, put_handler, delete_handler, { NULL } }

#define SEPARATE_RESOURCE(name, attributes, get_handler, post_handler, put_handler, delete_handler, resume_handler) \
  resource_t name = { NULL, NULL, IS_SEPARATE, attributes, get_handler, post_handler, put_handler, delete_handler, { .resume = resume_handler } }

#define EVENT_RESOURCE(name, attributes, get_handler, post_handler, put_handler, delete_handler, event_handler) \
  resource_t name = { NULL, NULL, IS_OBSERVABLE, attributes, get_handler, post_handler, put_handler, delete_handler, { .trigger = event_handler } }

/*
 * Macro to define a periodic resource.
 * The corresponding [name]_periodic_handler() function will be called every period.
 * For instance polling a sensor and publishing a changed value to subscribed clients would be done there.
 * The subscriber list will be maintained by the final_handler rest_subscription_handler() (see rest-mapping header file).
 */
#define PERIODIC_RESOURCE(name, attributes, get_handler, post_handler, put_handler, delete_handler, period, periodic_handler) \
  periodic_resource_t periodic_##name; \
  resource_t name = { NULL, NULL, IS_OBSERVABLE | IS_PERIODIC, attributes, get_handler, post_handler, put_handler, delete_handler, { .periodic = &periodic_##name } }; \
  periodic_resource_t periodic_##name = { NULL, &name, period, { { 0 } }, periodic_handler };

struct rest_implementation {
  char *name;

  /** Initialize the REST implementation. */
  void (*init)(void);

  /** Register the RESTful service callback at implementation. */
  void (*set_service_callback)(service_callback_t callback);

  /** Get request URI path. */
  int (*get_url)(void *request, const char **url);

  /** Get the method of a request. */
  rest_resource_flags_t (*get_method_type)(void *request);

  /** Set the status code of a response. */
  int (*set_response_status)(void *response, unsigned int code);

  /** Get the content-type of a request. */
  int (*get_header_content_type)(void *request,
                                 unsigned int *content_format);

  /** Set the Content-Type of a response. */
  int (*set_header_content_type)(void *response,
                                 unsigned int content_format);

  /** Get the Accept types of a request. */
  int (*get_header_accept)(void *request, unsigned int *accept);

  /** Get the Length option of a request. */
  int (*get_header_length)(void *request, uint32_t *size);

  /** Set the Length option of a response. */
  int (*set_header_length)(void *response, uint32_t size);

  /** Get the Max-Age option of a request. */
  int (*get_header_max_age)(void *request, uint32_t *age);

  /** Set the Max-Age option of a response. */
  int (*set_header_max_age)(void *response, uint32_t age);

  /** Set the ETag option of a response. */
  int (*set_header_etag)(void *response, const uint8_t *etag,
                         size_t length);

  /** Get the If-Match option of a request. */
  int (*get_header_if_match)(void *request, const uint8_t **etag);

  /** Get the If-Match option of a request. */
  int (*get_header_if_none_match)(void *request);

  /** Get the Host option of a request. */
  int (*get_header_host)(void *request, const char **host);

  /** Set the location option of a response. */
  int (*set_header_location)(void *response, const char *location);

  /** Get the payload option of a request. */
  int (*get_request_payload)(void *request, const uint8_t **payload);

  /** Set the payload option of a response. */
  int (*set_response_payload)(void *response, const void *payload,
                              size_t length);

  /** Get the query string of a request. */
  int (*get_query)(void *request, const char **value);

  /** Get the value of a request query key-value pair. */
  int (*get_query_variable)(void *request, const char *name,
                            const char **value);

  /** Get the value of a request POST key-value pair. */
  int (*get_post_variable)(void *request, const char *name,
                           const char **value);

  /** Send the payload to all subscribers of the resource at url. */
  void (*notify_subscribers)(resource_t *resource);

  /** The handler for resource subscriptions. */
  restful_final_handler subscription_handler;

  /* REST status codes. */
  const struct rest_implementation_status status;

  /* REST content-types. */
  const struct rest_implementation_type type;
};

/* instance of REST implementation */
extern const struct rest_implementation REST;

/*
 * To be called by HTTP/COAP server as a callback function when a new service request appears.
 * This function dispatches the corresponding RESTful service.
 */
int rest_invoke_restful_service(void *request, void *response,
                                uint8_t *buffer, uint16_t buffer_size,
                                int32_t *offset);
/*---------------------------------------------------------------------------*/
/**
 * \brief      Initializes REST framework and starts the HTTP or CoAP process.
 */
void rest_init_engine(void);
/*---------------------------------------------------------------------------*/
/**
 *
 * \brief      Resources wanted to be accessible should be activated with the following code.
 * \param resource
 *             A RESTful resource defined through the RESOURCE macros.
 * \param path
 *             The local URI path where to provide the resource.
 */
void rest_activate_resource(resource_t *resource, char *path);
/*---------------------------------------------------------------------------*/
/**
 * \brief      Returns the list of registered RESTful resources.
 * \return     The resource list.
 */
list_t rest_get_resources(void);
/*---------------------------------------------------------------------------*/

#endif /*REST_ENGINE_H_ */
