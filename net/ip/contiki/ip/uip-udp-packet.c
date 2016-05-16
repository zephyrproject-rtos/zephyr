/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 *
 */

/**
 * \file
 *         Module for sending UDP packets through uIP.
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include <net/ip_buf.h>

#include "contiki-conf.h"

#include "contiki/ip/uip-udp-packet.h"
#include "contiki/ipv6/multicast/uip-mcast6.h"

#include <string.h>

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_UDP_PACKET
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

/*---------------------------------------------------------------------------*/
uint8_t
uip_udp_packet_send(struct net_buf *buf, struct uip_udp_conn *c, const void *data, int len)
{
#if UIP_UDP
  if(data != NULL) {
    uip_set_udp_conn(buf) = c;
    uip_slen(buf) = len;
    memcpy(&uip_buf(buf)[UIP_LLH_LEN + UIP_IPUDPH_LEN], data,
           len > UIP_BUFSIZE - UIP_LLH_LEN - UIP_IPUDPH_LEN?
           UIP_BUFSIZE - UIP_LLH_LEN - UIP_IPUDPH_LEN: len);
    if (uip_process(&buf, UIP_UDP_SEND_CONN) == 0) {
      /* The packet was dropped, we can return now */
      return 0;
    }

#if UIP_CONF_IPV6_MULTICAST
  /* Let the multicast engine process the datagram before we send it */
 if(uip_is_addr_mcast_routable(&uip_udp_conn(buf)->ripaddr)) {
    UIP_MCAST6.out();
  }
#endif /* UIP_IPV6_MULTICAST */

 if (!uip_len(buf)) {
   /* Message was successfully sent, just bail out now. */
   goto out;
 }

#if NETSTACK_CONF_WITH_IPV6
 if (!tcpip_ipv6_output(buf)) {
   return 0;
 }
#else
    if(uip_len(buf) > 0) {
      tcpip_output(buf, NULL);
    }
#endif
  }
out:
  uip_slen(buf) = 0;
#endif /* UIP_UDP */
  return 1;
}
/*---------------------------------------------------------------------------*/
uint8_t
uip_udp_packet_sendto(struct net_buf *buf, struct uip_udp_conn *c, const void *data, int len,
		      const uip_ipaddr_t *toaddr, uint16_t toport)
{
  uip_ipaddr_t curaddr;
  uint16_t curport;
  uint8_t ret = 0;

  if(toaddr != NULL) {
    /* Save current IP addr/port. */
    uip_ipaddr_copy(&curaddr, &c->ripaddr);
    curport = c->rport;

    /* Load new IP addr/port */
    uip_ipaddr_copy(&c->ripaddr, toaddr);
    c->rport = toport;

    ret = uip_udp_packet_send(buf, c, data, len);

    /* Restore old IP addr/port */
    uip_ipaddr_copy(&c->ripaddr, &curaddr);
    c->rport = curport;
  }

  return ret;
}
/*---------------------------------------------------------------------------*/
