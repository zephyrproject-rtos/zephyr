/*
 * Copyright (c) 2009, Swedish Institute of Computer Science.
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
 *         MAC framer for nullmac
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki/mac/framer-nullmac.h"
#include "contiki/packetbuf.h"

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_15_4_FRAMING
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

struct nullmac_hdr {
  linkaddr_t receiver;
  linkaddr_t sender;
};

/*---------------------------------------------------------------------------*/
static int
hdr_length(struct net_buf *buf)
{
  return sizeof(struct nullmac_hdr);
}
/*---------------------------------------------------------------------------*/
static int
create(struct net_buf *buf)
{
  struct nullmac_hdr *hdr;

  if(packetbuf_hdralloc(buf, sizeof(struct nullmac_hdr))) {
    hdr = packetbuf_hdrptr(buf);
    linkaddr_copy(&(hdr->sender), &linkaddr_node_addr);
    linkaddr_copy(&(hdr->receiver), packetbuf_addr(buf, PACKETBUF_ADDR_RECEIVER));
    return sizeof(struct nullmac_hdr);
  }
  PRINTF("PNULLMAC-UT: too large header: %u\n", sizeof(struct nullmac_hdr));
  return FRAMER_FAILED;
}
/*---------------------------------------------------------------------------*/
static int
parse(struct net_buf *buf)
{
  struct nullmac_hdr *hdr;
  hdr = packetbuf_dataptr(buf);
  if(packetbuf_hdrreduce(buf, sizeof(struct nullmac_hdr))) {
    packetbuf_set_addr(buf, PACKETBUF_ADDR_SENDER, &(hdr->sender));
    packetbuf_set_addr(buf, PACKETBUF_ADDR_RECEIVER, &(hdr->receiver));

    PRINTF("PNULLMAC-IN: ");
    PRINTLLADDR((const uip_lladdr_t *)packetbuf_addr(buf, PACKETBUF_ADDR_SENDER));
    PRINTF(" ");
    PRINTLLADDR((const uip_lladdr_t *)packetbuf_addr(buf, PACKETBUF_ADDR_RECEIVER));
    PRINTF(" %u (%u)\n", packetbuf_datalen(buf), sizeof(struct nullmac_hdr));

    return sizeof(struct nullmac_hdr);
  }
  return FRAMER_FAILED;
}
/*---------------------------------------------------------------------------*/
const struct framer framer_nullmac = {
  hdr_length,
  create,
  framer_canonical_create_and_secure,
  parse
};
