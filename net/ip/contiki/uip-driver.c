/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 */

/**
 * \file
 *         A brief description of what this file is
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "net/netstack.h"
#include "net/ip/uip.h"
#include "net/ip/tcpip.h"
#include "net/packetbuf.h"
#include "net/uip-driver.h"
#include <string.h>

#define DEBUG 0
#include "net/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

/*--------------------------------------------------------------------*/
uint8_t
uip_driver_send(struct net_buf *buf, const uip_lladdr_t *lladdr)
{
  packetbuf_copyfrom(buf, &uip_buf(buf)[UIP_LLH_LEN], uip_len(buf));

  /* XXX we should provide a callback function that is called when the
     packet is sent. For now, we just supply a NULL pointer. */
  return NETSTACK_LLSEC.send(buf, NULL, NULL);
}
/*--------------------------------------------------------------------*/
static void
init(void)
{
  /*
   * Set out output function as the function to be called from uIP to
   * send a packet.
   */
  tcpip_set_outputfunc(uip_driver_send);
}
/*--------------------------------------------------------------------*/
static uint8_t
input(struct net_buf *buf)
{
  PRINTF("uip-driver(%p): input %d bytes\n", buf, packetbuf_datalen(buf));
  if(packetbuf_datalen(buf) > 0 &&
     packetbuf_datalen(buf) <= UIP_BUFSIZE - UIP_LLH_LEN) {
    memcpy(&uip_buf(buf)[UIP_LLH_LEN], packetbuf_dataptr(buf), packetbuf_datalen(buf));
    uip_len(buf) = packetbuf_datalen(buf);
    tcpip_input(buf);
    return 1;
  } else {
    PRINTF("datalen %d MAX %d\n", packetbuf_datalen(buf), UIP_BUFSIZE - UIP_LLH_LEN);
    UIP_LOG("uip-driver: too long packet discarded");
    return 0;
  }
}
/*--------------------------------------------------------------------*/
const struct network_driver uip_driver = {
  "uip",
  init,
  input
};
/*--------------------------------------------------------------------*/
