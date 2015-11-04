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
 * This file is part of the Contiki operating system.
 *
 */

#include <net/l2_buf.h>

#include "contiki/mac/mac.h"

#define DEBUG 0
#include "contiki/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

#if NET_MAC_CONF_STATS
net_mac_stats_t net_mac_stats = {0};
#endif /* NET_MAC_CONF_STATS */

/*---------------------------------------------------------------------------*/
void
mac_call_sent_callback(struct net_buf *buf, mac_callback_t sent, void *ptr, int status, int num_tx)
{
  PRINTF("buf %p mac_callback_t %p ptr %p status %d num_tx %d\n", buf,
         (void *)sent, ptr, status, num_tx);
  switch(status) {
  case MAC_TX_COLLISION:
    PRINTF("mac: collision after %d tx\n", num_tx);
    break; 
  case MAC_TX_NOACK:
    PRINTF("mac: noack after %d tx\n", num_tx);
    break;
  case MAC_TX_OK:
    PRINTF("mac: sent after %d tx\n", num_tx);
    break;
  default:
    PRINTF("mac: error %d after %d tx\n", status, num_tx);
  }

  if(sent) {
#if NET_MAC_CONF_STATS
    net_mac_stats.bytes_sent += uip_pkt_buflen(buf);
#endif
    sent(buf, ptr, status, num_tx);
  }
}
/*---------------------------------------------------------------------------*/
