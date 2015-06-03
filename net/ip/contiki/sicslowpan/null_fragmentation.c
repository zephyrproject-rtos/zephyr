/* compression.h - Header Compression */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>

#include <net/net_buf.h>
#include <net/sicslowpan/null_fragmentation.h>
#include <net/netstack.h>

#include "contiki/ipv6/uip-ds6-nbr.h"

#define DEBUG 0
#include "net/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

static void
packet_sent(struct net_mbuf *buf, void *ptr, int status, int transmissions)
{
	const linkaddr_t *dest = packetbuf_addr(buf, PACKETBUF_ADDR_RECEIVER);
	uip_ds6_link_neighbor_callback(dest, status, transmissions);
	uip_last_tx_status(buf) = status;
}

static int fragment(struct net_buf *buf, void *ptr)
{
	struct net_mbuf *mbuf;

	mbuf = net_mbuf_get_reserve(0);
	if (!mbuf) {
		return 0;
	}

	packetbuf_copyfrom(mbuf, &uip_buf(buf)[UIP_LLH_LEN], uip_len(buf));
	packetbuf_set_addr(mbuf, PACKETBUF_ADDR_RECEIVER, &buf->dest);
	net_buf_put(buf);

	return NETSTACK_LLSEC.send(mbuf, &packet_sent, true, ptr);
}

static int reassemble(struct net_mbuf *mbuf)
{
        struct net_buf *buf;

	buf = net_buf_get_reserve(0);
	if (!buf) {
		return 0;
	}

	if(packetbuf_datalen(mbuf) > 0 &&
		packetbuf_datalen(mbuf) <= UIP_BUFSIZE - UIP_LLH_LEN) {
		memcpy(uip_buf(buf), packetbuf_dataptr(mbuf),
						packetbuf_datalen(mbuf));
		uip_len(buf) = packetbuf_datalen(mbuf);

		if (net_driver_15_4_recv(buf) < 0) {
			net_buf_put(buf);
		} else {
			net_mbuf_put(mbuf);
		}

		return 1;
        } else {
                PRINTF("packet discarded, datalen %d MAX %d\n",
				packetbuf_datalen(mbuf), UIP_BUFSIZE - UIP_LLH_LEN);
		net_buf_put(buf);
                return 0;
        }
}

const struct fragmentation null_fragmentation = {
	fragment,
	reassemble
};
