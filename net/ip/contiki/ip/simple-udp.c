/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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
 *         Code for the simple-udp module.
 * \author
 *         Adam Dunkels <adam@sics.se>
 *
 */

/**
 * \addtogroup simple-udp
 * @{
 */

#include "contiki-net.h"
#include "net/ip/simple-udp.h"

#include <string.h>


PROCESS(simple_udp_process, "Simple UDP process");
static uint8_t started = 0;
static uint8_t databuffer[UIP_BUFSIZE];

#define UIP_IP_BUF   ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

/*---------------------------------------------------------------------------*/
static void
init_simple_udp(void)
{
  if(started == 0) {
    process_start(&simple_udp_process, NULL);
    started = 1;
  }
}
/*---------------------------------------------------------------------------*/
int
simple_udp_send(struct simple_udp_connection *c,
                const void *data, uint16_t datalen)
{
  if(c->udp_conn != NULL) {
    uip_udp_packet_sendto(c->udp_conn, data, datalen,
                          &c->remote_addr, UIP_HTONS(c->remote_port));
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
simple_udp_sendto(struct simple_udp_connection *c,
                  const void *data, uint16_t datalen,
                  const uip_ipaddr_t *to)
{
  if(c->udp_conn != NULL) {
    uip_udp_packet_sendto(c->udp_conn, data, datalen,
                          to, UIP_HTONS(c->remote_port));
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
simple_udp_sendto_port(struct simple_udp_connection *c,
		       const void *data, uint16_t datalen,
		       const uip_ipaddr_t *to,
		       uint16_t port)
{
  if(c->udp_conn != NULL) {
    uip_udp_packet_sendto(c->udp_conn, data, datalen,
                          to, UIP_HTONS(port));
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
simple_udp_register(struct simple_udp_connection *c,
                    uint16_t local_port,
                    uip_ipaddr_t *remote_addr,
                    uint16_t remote_port,
                    simple_udp_callback receive_callback)
{

  init_simple_udp();

  c->local_port = local_port;
  c->remote_port = remote_port;
  if(remote_addr != NULL) {
    uip_ipaddr_copy(&c->remote_addr, remote_addr);
  }
  c->receive_callback = receive_callback;

  PROCESS_CONTEXT_BEGIN(&simple_udp_process);
  c->udp_conn = udp_new(remote_addr, UIP_HTONS(remote_port), c);
  if(c->udp_conn != NULL) {
    udp_bind(c->udp_conn, UIP_HTONS(local_port));
  }
  PROCESS_CONTEXT_END();

  if(c->udp_conn == NULL) {
    return 0;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(simple_udp_process, ev, data)
{
  struct simple_udp_connection *c;
  PROCESS_BEGIN();
  
  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == tcpip_event) {

      /* An appstate pointer is passed to use from the IP stack
         through the 'data' pointer. We registered this appstate when
         we did the udp_new() call in simple_udp_register() as the
         struct simple_udp_connection pointer. So we extract this
         pointer and use it when calling the reception callback. */
      c = (struct simple_udp_connection *)data;

      /* Defensive coding: although the appstate *should* be non-null
         here, we make sure to avoid the program crashing on us. */
      if(c != NULL) {

        /* If we were called because of incoming data, we should call
           the reception callback. */
        if(uip_newdata()) {
          /* Copy the data from the uIP data buffer into our own
             buffer to avoid the uIP buffer being messed with by the
             callee. */
          memcpy(databuffer, uip_appdata, uip_datalen());

          /* Call the client process. We use the PROCESS_CONTEXT
             mechanism to temporarily switch process context to the
             client process. */
          if(c->receive_callback != NULL) {
            PROCESS_CONTEXT_BEGIN(c->client_process);
            c->receive_callback(c,
                                &(UIP_IP_BUF->srcipaddr),
                                UIP_HTONS(UIP_IP_BUF->srcport),
                                &(UIP_IP_BUF->destipaddr),
                                UIP_HTONS(UIP_IP_BUF->destport),
                                databuffer, uip_datalen());
            PROCESS_CONTEXT_END();
          }
        }
      }
    }

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/** @} */
