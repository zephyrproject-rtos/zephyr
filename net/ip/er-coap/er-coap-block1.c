/*
 * Copyright (c) 2014, Lars Schmertmann <SmallLars@t-online.de>.
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
 *      CoAP module for block 1 handling
 * \author
 *      Lars Schmertmann <SmallLars@t-online.de>
 */

#include <stdio.h>
#include <string.h>

#include "er-coap.h"
#include "er-coap-block1.h"

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

/*----------------------------------------------------------------------------*/

/**
 * \brief Block 1 support within a coap-ressource
 *
 *        This function will help you to use block 1. If target is null
 *        error handling and response configuration is active. On return
 *        value 0, the last block was recived, while on return value 1
 *        more blocks will follow. With target, len and maxlen this
 *        function will assemble the blocks.
 *
 *        You can find an example in:
 *        examples/er-rest-example/resources/res-b1-sep-b2.c
 *
 * \param request   Request pointer from the handler
 * \param response  Response pointer from the handler
 * \param target    Pointer to the buffer where the request payload can be assembled
 * \param len       Pointer to the variable, where the function stores the actual length
 * \param max_len   Length of the "target"-Buffer
 *
 * \return 0 if initialisation was successful
 *         -1 if initialisation failed
 */
int
coap_block1_handler(void *request, void *response, uint8_t *target, size_t *len, size_t max_len)
{
  const uint8_t *payload = 0;
  int pay_len = REST.get_request_payload(request, &payload);

  if(!pay_len || !payload) {
    erbium_status_code = REST.status.BAD_REQUEST;
    coap_error_message = "NoPayload";
    return -1;
  }

  coap_packet_t *packet = (coap_packet_t *)request;

  if(packet->block1_offset + pay_len > max_len) {
    erbium_status_code = REST.status.REQUEST_ENTITY_TOO_LARGE;
    coap_error_message = "Message to big";
    return -1;
  }

  if(target && len) {
    memcpy(target + packet->block1_offset, payload, pay_len);
    *len = packet->block1_offset + pay_len;
  }

  if(IS_OPTION(packet, COAP_OPTION_BLOCK1)) {
    PRINTF("Blockwise: block 1 request: Num: %u, More: %u, Size: %u, Offset: %u\n",
           packet->block1_num,
           packet->block1_more,
           packet->block1_size,
           packet->block1_offset);

    coap_set_header_block1(response, packet->block1_num, packet->block1_more, packet->block1_size);
    if(packet->block1_more) {
      coap_set_status_code(response, CONTINUE_2_31);
      return 1;
    }
  }

  return 0;
}
