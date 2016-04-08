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
 *      /.well-known/core resource implementation.
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include <string.h>
#include "er-coap-engine.h"

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_COAP_WELL_KNOWN
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#define ADD_CHAR_IF_POSSIBLE(char) \
  if(strpos >= *offset && bufpos < preferred_size) { \
    buffer[bufpos++] = char; \
  } \
  ++strpos

#define ADD_STRING_IF_POSSIBLE(string, op) \
  tmplen = strlen(string); \
  if(strpos + tmplen > *offset) { \
    bufpos += snprintf((char *)buffer + bufpos, \
                       preferred_size - bufpos + 1, \
                       "%s", \
                       string \
                       + (*offset - (int32_t)strpos > 0 ? \
                          *offset - (int32_t)strpos : 0)); \
    if(bufpos op preferred_size) { \
      PRINTF("res: BREAK at %s (%p)\n", string, resource); \
      break; \
    } \
  } \
  strpos += tmplen

/*---------------------------------------------------------------------------*/
/*- Resource Handlers -------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
void
well_known_core_get_handler(void *request, void *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset)
{
  size_t strpos = 0;            /* position in overall string (which is larger than the buffer) */
  size_t bufpos = 0;            /* position within buffer (bytes written) */
  size_t tmplen = 0;
  resource_t *resource = NULL;

#if COAP_LINK_FORMAT_FILTERING
  /* For filtering. */
  const char *filter = NULL;
  const char *attrib = NULL;
  const char *found = NULL;
  const char *end = NULL;
  char *value = NULL;
  char lastchar = '\0';
  int len = coap_get_header_uri_query(request, &filter);

  if(len) {
    value = strchr(filter, '=');
    value[0] = '\0';
    ++value;
    len -= strlen(filter) + 1;

    PRINTF("Filter %s = %.*s\n", filter, len, value);

    if(strcmp(filter, "href") == 0 && value[0] == '/') {
      ++value;
      --len;
    }

    lastchar = value[len - 1];
    value[len - 1] = '\0';
  }
#endif

  for(resource = (resource_t *)list_head(rest_get_resources()); resource;
      resource = resource->next) {
#if COAP_LINK_FORMAT_FILTERING
    /* Filtering */
    if(len) {
      if(strcmp(filter, "href") == 0) {
        attrib = strstr(resource->url, value);
        if(attrib == NULL || (value[-1] == '/' && attrib != resource->url)) {
          continue;
        }
        end = attrib + strlen(attrib);
      } else if(resource->attributes != NULL) {
        attrib = strstr(resource->attributes, filter);
        if(attrib == NULL
           || (attrib[strlen(filter)] != '='
               && attrib[strlen(filter)] != '"')) {
          continue;
        }
        attrib += strlen(filter) + 2;
        end = strchr(attrib, '"');
      }

      PRINTF("Filter: res has attrib %s (%s)\n", attrib, value);
      found = attrib;
      while((found = strstr(found, value)) != NULL) {
        if(found > end) {
          found = NULL;
          break;
        }
        if(lastchar == found[len - 1] || lastchar == '*') {
          break;
        }
        ++found;
      }
      if(found == NULL) {
        continue;
      }
      PRINTF("Filter: res has prefix %s\n", found);
      if(lastchar != '*'
         && (found[len] != '"' && found[len] != ' ' && found[len] != '\0')) {
        continue;
      }
      PRINTF("Filter: res has match\n");
    }
#endif

    PRINTF("res: /%s (%p)\npos: s%zu, o%ld, b%zu\n", resource->url, resource,
           strpos, (unsigned long) *offset, bufpos);

    if(strpos > 0) {
      ADD_CHAR_IF_POSSIBLE(',');
    }
    ADD_CHAR_IF_POSSIBLE('<');
    ADD_CHAR_IF_POSSIBLE('/');
    ADD_STRING_IF_POSSIBLE(resource->url, >=);
    ADD_CHAR_IF_POSSIBLE('>');

    if(resource->attributes != NULL && resource->attributes[0]) {
      ADD_CHAR_IF_POSSIBLE(';');
      ADD_STRING_IF_POSSIBLE(resource->attributes, >);
    }

    /* buffer full, but resource not completed yet; or: do not break if resource exactly fills buffer. */
    if(bufpos > preferred_size && strpos - bufpos > *offset) {
      PRINTF("res: BREAK at %s (%p)\n", resource->url, resource);
      break;
    }
  }

  if(bufpos > 0) {
    PRINTF("BUF %zu: %.*s\n", bufpos, (int)bufpos, (char *)buffer);

    coap_set_payload(response, buffer, bufpos);
    coap_set_header_content_format(response, APPLICATION_LINK_FORMAT);
  } else if(strpos > 0) {
    PRINTF("well_known_core_handler(): bufpos<=0\n");

    coap_set_status_code(response, BAD_OPTION_4_02);
    coap_set_payload(response, "BlockOutOfScope", 15);
  }

  if(resource == NULL) {
    PRINTF("res: DONE\n");
    *offset = -1;
  } else {
    PRINTF("res: MORE at %s (%p)\n", resource->url, resource);
    *offset += preferred_size;
  }
}
/*---------------------------------------------------------------------------*/
RESOURCE(res_well_known_core, "ct=40", well_known_core_get_handler, NULL,
         NULL, NULL);
/*---------------------------------------------------------------------------*/
