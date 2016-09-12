/*
 * Copyright (c) 2001-2003, Adam Dunkels.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 *
 */

/**
 * \file
 * The uIP TCP/IP stack code.
 * \author Adam Dunkels <adam@dunkels.com>
 */

/**
 * \addtogroup uip
 * @{
 */

#define DEBUG_PRINTF(...) /*printf(__VA_ARGS__)*/

/*
 * uIP is a small implementation of the IP, UDP and TCP protocols (as
 * well as some basic ICMP stuff). The implementation couples the IP,
 * UDP, TCP and the application layers very tightly. To keep the size
 * of the compiled code down, this code frequently uses the goto
 * statement. While it would be possible to break the uip_process()
 * function into many smaller functions, this would increase the code
 * size because of the overhead of parameter passing and the fact that
 * the optimier would not be as efficient.
 *
 * The principle is that we have a small buffer, called the uip_buf,
 * in which the device driver puts an incoming packet. The TCP/IP
 * stack parses the headers in the packet, and calls the
 * application. If the remote host has sent data to the application,
 * this data is present in the uip_buf and the application read the
 * data from there. It is up to the application to put this data into
 * a byte stream if needed. The application will not be fed with data
 * that is out of sequence.
 *
 * If the application whishes to send data to the peer, it should put
 * its data into the uip_buf. The uip_appdata pointer points to the
 * first available byte. The TCP/IP stack will calculate the
 * checksums, and fill in the necessary header fields and finally send
 * the packet back to the peer.
*/

#include "contiki/ip/uip.h"
#include "contiki/ip/uipopt.h"
#include "contiki/ipv4/uip_arp.h"

#include "contiki/ipv4/uip-neighbor.h"

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_IPV4
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#include <net/ip_buf.h>
#include <string.h>
#include <errno.h>

#ifdef CONFIG_DHCP
#include "contiki/ip/dhcpc.h"
#endif

extern void net_context_set_connection_status(struct net_context *context,
					      int status);
void *net_context_get_internal_connection(struct net_context *context);
void net_context_set_internal_connection(struct net_context *context,
					 void *conn);
struct net_context *net_context_find_internal_connection(void *conn);
void net_context_tcp_set_pending(struct net_context *context,
				 struct net_buf *buf);

/*---------------------------------------------------------------------------*/
/* Variable definitions. */


/* The IP address of this host. If it is defined to be fixed (by
   setting UIP_FIXEDADDR to 1 in uipopt.h), the address is set
   here. Otherwise, the address */
#if UIP_FIXEDADDR > 0
const uip_ipaddr_t uip_hostaddr =
  { UIP_IPADDR0, UIP_IPADDR1, UIP_IPADDR2, UIP_IPADDR3 };
const uip_ipaddr_t uip_draddr =
  { UIP_DRIPADDR0, UIP_DRIPADDR1, UIP_DRIPADDR2, UIP_DRIPADDR3 };
const uip_ipaddr_t uip_netmask =
  { UIP_NETMASK0, UIP_NETMASK1, UIP_NETMASK2, UIP_NETMASK3 };
#else
uip_ipaddr_t uip_hostaddr, uip_draddr, uip_netmask;
#endif /* UIP_FIXEDADDR */

const uip_ipaddr_t uip_broadcast_addr =
#if NETSTACK_CONF_WITH_IPV6
  { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };
#else /* NETSTACK_CONF_WITH_IPV6 */
  { { 0xff, 0xff, 0xff, 0xff } };
#endif /* NETSTACK_CONF_WITH_IPV6 */
const uip_ipaddr_t uip_all_zeroes_addr = { { 0x0, /* rest is 0 */ } };

#if UIP_FIXEDETHADDR
const uip_lladdr_t uip_lladdr = {{UIP_ETHADDR0,
					  UIP_ETHADDR1,
					  UIP_ETHADDR2,
					  UIP_ETHADDR3,
					  UIP_ETHADDR4,
					  UIP_ETHADDR5}};
#else
uip_lladdr_t uip_lladdr = {{0,0,0,0,0,0}};
#endif

#if 0
/* These are moved to net_buf.h */
/* The packet buffer that contains incoming packets. */
uip_buf_t uip_aligned_buf;

void *uip_appdata;               /* The uip_appdata pointer points to
				    application data. */
void *uip_sappdata;              /* The uip_appdata pointer points to
				    the application data which is to
				    be sent. */
#if UIP_URGDATA > 0
void *uip_urgdata;               /* The uip_urgdata pointer points to
				    urgent data (out-of-band data), if
				    present. */
uint16_t uip_urglen, uip_surglen;
#endif /* UIP_URGDATA > 0 */

uint16_t uip_len, uip_slen;
                             /* The uip_len is either 8 or 16 bits,
				depending on the maximum packet
				size. */

uint8_t uip_flags;     /* The uip_flags variable is used for
				communication between the TCP/IP stack
				and the application program. */
struct uip_conn *uip_conn;   /* uip_conn always points to the current
				connection. */
#endif /* 0 */

struct uip_conn uip_conns[UIP_CONNS];
                             /* The uip_conns array holds all TCP
				connections. */
uint16_t uip_listenports[UIP_LISTENPORTS];
                             /* The uip_listenports list all currently
				listning ports. */
#if UIP_UDP
#if 0
/* Moved to net_buf */
struct uip_udp_conn *uip_udp_conn;
#endif /* 0 */
struct uip_udp_conn uip_udp_conns[UIP_UDP_CONNS];
#endif /* UIP_UDP */

static uint16_t ipid;           /* Ths ipid variable is an increasing
				number that is used for the IP ID
				field. */

void uip_setipid(uint16_t id) { ipid = id; }

static uint8_t iss[4];          /* The iss variable is used for the TCP
				initial sequence number. */

#if UIP_ACTIVE_OPEN || UIP_UDP
static uint16_t lastport;       /* Keeps track of the last port used for
				a new connection. */
#endif /* UIP_ACTIVE_OPEN || UIP_UDP */

/* Temporary variables. */
uint8_t uip_acc32[4];
static uint8_t c;
#if UIP_TCP
static uint8_t opt;
static uint16_t tmp16;
#endif /* UIP_TCP */

/* Structures and definitions. */
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20
#define TCP_CTL 0x3f

#define TCP_OPT_END     0   /* End of TCP options list */
#define TCP_OPT_NOOP    1   /* "No-operation" TCP option */
#define TCP_OPT_MSS     2   /* Maximum segment size TCP option */

#define TCP_OPT_MSS_LEN 4   /* Length of TCP MSS option. */

#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO       8

#define ICMP_DEST_UNREACHABLE        3
#define ICMP_PORT_UNREACHABLE        3

#define ICMP6_ECHO_REPLY             129
#define ICMP6_ECHO                   128
#define ICMP6_NEIGHBOR_SOLICITATION  135
#define ICMP6_NEIGHBOR_ADVERTISEMENT 136

#define ICMP6_FLAG_S (1 << 6)

#define ICMP6_OPTION_SOURCE_LINK_ADDRESS 1
#define ICMP6_OPTION_TARGET_LINK_ADDRESS 2


/* Macros. */
#define BUF(buf) ((struct uip_tcpip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])
#define FBUF ((struct uip_tcpip_hdr *)&uip_reassbuf[0])
#define ICMPBUF(buf) ((struct uip_icmpip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])
#define UDPBUF(buf) ((struct uip_udpip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])


#if UIP_STATISTICS == 1
struct uip_stats uip_stat;
#define UIP_STAT(s) s
#else
#define UIP_STAT(s)
#endif /* UIP_STATISTICS == 1 */

#if UIP_LOGGING == 1
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif /* UIP_LOGGING == 1 */

#if ! UIP_ARCH_ADD32
void
uip_add32(uint8_t *op32, uint16_t op16)
{
  uip_acc32[3] = op32[3] + (op16 & 0xff);
  uip_acc32[2] = op32[2] + (op16 >> 8);
  uip_acc32[1] = op32[1];
  uip_acc32[0] = op32[0];

  if(uip_acc32[2] < (op16 >> 8)) {
    ++uip_acc32[1];
    if(uip_acc32[1] == 0) {
      ++uip_acc32[0];
    }
  }


  if(uip_acc32[3] < (op16 & 0xff)) {
    ++uip_acc32[2];
    if(uip_acc32[2] == 0) {
      ++uip_acc32[1];
      if(uip_acc32[1] == 0) {
	++uip_acc32[0];
      }
    }
  }
}

#endif /* UIP_ARCH_ADD32 */

#if ! UIP_ARCH_CHKSUM
/*---------------------------------------------------------------------------*/
static uint16_t
chksum(uint16_t sum, const uint8_t *data, uint16_t len)
{
  uint16_t t;
  const uint8_t *dataptr;
  const uint8_t *last_byte;

  dataptr = data;
  last_byte = data + len - 1;

  while(dataptr < last_byte) {	/* At least two more bytes */
    t = (dataptr[0] << 8) + dataptr[1];
    sum += t;
    if(sum < t) {
      sum++;		/* carry */
    }
    dataptr += 2;
  }

  if(dataptr == last_byte) {
    t = (dataptr[0] << 8) + 0;
    sum += t;
    if(sum < t) {
      sum++;		/* carry */
    }
  }

  /* Return sum in host byte order. */
  return sum;
}
/*---------------------------------------------------------------------------*/
uint16_t
uip_chksum(uint16_t *data, uint16_t len)
{
  return uip_htons(chksum(0, (uint8_t *)data, len));
}
/*---------------------------------------------------------------------------*/
#ifndef UIP_ARCH_IPCHKSUM
uint16_t
uip_ipchksum(struct net_buf *buf)
{
  uint16_t sum;

  sum = chksum(0, &uip_buf(buf)[UIP_LLH_LEN], UIP_IPH_LEN);
  DEBUG_PRINTF("uip_ipchksum: sum 0x%04x\n", sum);
  return (sum == 0) ? 0xffff : uip_htons(sum);
}
#endif
/*---------------------------------------------------------------------------*/
static uint16_t
upper_layer_chksum(struct net_buf *buf, uint8_t proto)
{
  uint16_t upper_layer_len;
  uint16_t sum;

#if NETSTACK_CONF_WITH_IPV6
  upper_layer_len = (((uint16_t)(BUF->len[0]) << 8) + BUF->len[1]);
#else /* NETSTACK_CONF_WITH_IPV6 */
  upper_layer_len = (((uint16_t)(BUF(buf)->len[0]) << 8) + BUF(buf)->len[1]) - UIP_IPH_LEN;
#endif /* NETSTACK_CONF_WITH_IPV6 */

  /* First sum pseudoheader. */

  /* IP protocol and length fields. This addition cannot carry. */
  sum = upper_layer_len + proto;
  /* Sum IP source and destination addresses. */
  sum = chksum(sum, (uint8_t *)&BUF(buf)->srcipaddr, 2 * sizeof(uip_ipaddr_t));

  /* Sum TCP header and data. */
  sum = chksum(sum, &uip_buf(buf)[UIP_IPH_LEN + UIP_LLH_LEN],
	       upper_layer_len);

  return (sum == 0) ? 0xffff : uip_htons(sum);
}
/*---------------------------------------------------------------------------*/
#if NETSTACK_CONF_WITH_IPV6
uint16_t
uip_icmp6chksum(void)
{
  return upper_layer_chksum(UIP_PROTO_ICMP6);

}
#endif /* NETSTACK_CONF_WITH_IPV6 */
/*---------------------------------------------------------------------------*/
uint16_t
uip_tcpchksum(struct net_buf *buf)
{
  return upper_layer_chksum(buf, UIP_PROTO_TCP);
}
/*---------------------------------------------------------------------------*/
#if UIP_UDP_CHECKSUMS
uint16_t
uip_udpchksum(void)
{
  return upper_layer_chksum(UIP_PROTO_UDP);
}
#endif /* UIP_UDP_CHECKSUMS */
#endif /* UIP_ARCH_CHKSUM */
/*---------------------------------------------------------------------------*/
void
uip_init(void)
{
  for(c = 0; c < UIP_LISTENPORTS; ++c) {
    uip_listenports[c] = 0;
  }
  for(c = 0; c < UIP_CONNS; ++c) {
    uip_conns[c].tcpstateflags = UIP_CLOSED;
  }
#if UIP_ACTIVE_OPEN || UIP_UDP
  lastport = 1024;
#endif /* UIP_ACTIVE_OPEN || UIP_UDP */

#if UIP_UDP
  for(c = 0; c < UIP_UDP_CONNS; ++c) {
    uip_udp_conns[c].lport = 0;
  }
#endif /* UIP_UDP */


  /* IPv4 initialization. */
#if UIP_FIXEDADDR == 0
  /*  uip_hostaddr[0] = uip_hostaddr[1] = 0;*/
#endif /* UIP_FIXEDADDR */

}
/*---------------------------------------------------------------------------*/
#if UIP_ACTIVE_OPEN
struct uip_conn *
uip_connect(const uip_ipaddr_t *ripaddr, uint16_t rport)
{
  register struct uip_conn *conn, *cconn;

  /* Find an unused local port. */
 again:
  ++lastport;

  if(lastport >= 32000) {
    lastport = 4096;
  }

  /* Check if this port is already in use, and if so try to find
     another one. */
  for(c = 0; c < UIP_CONNS; ++c) {
    conn = &uip_conns[c];
    if(conn->tcpstateflags != UIP_CLOSED &&
       conn->lport == uip_htons(lastport)) {
      goto again;
    }
  }

  conn = 0;
  for(c = 0; c < UIP_CONNS; ++c) {
    cconn = &uip_conns[c];
    if(cconn->tcpstateflags == UIP_CLOSED) {
      conn = cconn;
      break;
    }
    if(cconn->tcpstateflags == UIP_TIME_WAIT) {
      if(conn == 0 ||
	 cconn->timer > conn->timer) {
	conn = cconn;
      }
    }
  }

  if(conn == 0) {
    return 0;
  }

  conn->tcpstateflags = UIP_SYN_SENT;

  conn->snd_nxt[0] = iss[0];
  conn->snd_nxt[1] = iss[1];
  conn->snd_nxt[2] = iss[2];
  conn->snd_nxt[3] = iss[3];

  conn->initialmss = conn->mss = UIP_TCP_MSS;

  conn->len = 1;   /* TCP length of the SYN is one. */
  conn->nrtx = 0;
  conn->timer = 1; /* Send the SYN next time around. */
  conn->rto = UIP_RTO;
  conn->sa = 0;
  conn->sv = 16;   /* Initial value of the RTT variance. */
  conn->lport = uip_htons(lastport);
  conn->rport = rport;
  uip_ipaddr_copy(&conn->ripaddr, ripaddr);

  return conn;
}
#endif /* UIP_ACTIVE_OPEN */
/*---------------------------------------------------------------------------*/
#if UIP_UDP
struct uip_udp_conn *
uip_udp_new(const uip_ipaddr_t *ripaddr, uint16_t rport)
{
  register struct uip_udp_conn *conn;

  /* Find an unused local port. */
 again:
  ++lastport;

  if(lastport >= 32000) {
    lastport = 4096;
  }

  for(c = 0; c < UIP_UDP_CONNS; ++c) {
    if(uip_udp_conns[c].lport == uip_htons(lastport)) {
      goto again;
    }
  }


  conn = 0;
  for(c = 0; c < UIP_UDP_CONNS; ++c) {
    if(uip_udp_conns[c].lport == 0) {
      conn = &uip_udp_conns[c];
      break;
    }
  }

  if(conn == 0) {
    return 0;
  }

  conn->lport = UIP_HTONS(lastport);
  conn->rport = rport;
  if(ripaddr == NULL) {
    memset(&conn->ripaddr, 0, sizeof(uip_ipaddr_t));
  } else {
    uip_ipaddr_copy(&conn->ripaddr, ripaddr);
  }
  conn->ttl = UIP_TTL;

  return conn;
}
#endif /* UIP_UDP */
/*---------------------------------------------------------------------------*/
void
uip_unlisten(uint16_t port)
{
  for(c = 0; c < UIP_LISTENPORTS; ++c) {
    if(uip_listenports[c] == port) {
      uip_listenports[c] = 0;
      return;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
uip_listen(uint16_t port)
{
  for(c = 0; c < UIP_LISTENPORTS; ++c) {
    if(uip_listenports[c] == 0) {
      uip_listenports[c] = port;
      return;
    }
  }
}
/*---------------------------------------------------------------------------*/
/* XXX: IP fragment reassembly: not well-tested. */

#if UIP_REASSEMBLY && !NETSTACK_CONF_WITH_IPV6
#define UIP_REASS_BUFSIZE (UIP_BUFSIZE - UIP_LLH_LEN)
static uint8_t uip_reassbuf[UIP_REASS_BUFSIZE];
static uint8_t uip_reassbitmap[UIP_REASS_BUFSIZE / (8 * 8)];
static const uint8_t bitmap_bits[8] = {0xff, 0x7f, 0x3f, 0x1f,
				    0x0f, 0x07, 0x03, 0x01};
static uint16_t uip_reasslen;
static uint8_t uip_reassflags;
#define UIP_REASS_FLAG_LASTFRAG 0x01
static uint8_t uip_reasstmr;

#define IP_MF   0x20

static uint8_t
uip_reass(void)
{
  uint16_t offset, len;
  uint16_t i;

  /* If ip_reasstmr is zero, no packet is present in the buffer, so we
     write the IP header of the fragment into the reassembly
     buffer. The timer is updated with the maximum age. */
  if(uip_reasstmr == 0) {
    memcpy(uip_reassbuf, &BUF->vhl, UIP_IPH_LEN);
    uip_reasstmr = UIP_REASS_MAXAGE;
    uip_reassflags = 0;
    /* Clear the bitmap. */
    memset(uip_reassbitmap, 0, sizeof(uip_reassbitmap));
  }

  /* Check if the incoming fragment matches the one currently present
     in the reasembly buffer. If so, we proceed with copying the
     fragment into the buffer. */
  if(BUF->srcipaddr[0] == FBUF->srcipaddr[0] &&
     BUF->srcipaddr[1] == FBUF->srcipaddr[1] &&
     BUF->destipaddr[0] == FBUF->destipaddr[0] &&
     BUF->destipaddr[1] == FBUF->destipaddr[1] &&
     BUF->ipid[0] == FBUF->ipid[0] &&
     BUF->ipid[1] == FBUF->ipid[1]) {

    len = (BUF->len[0] << 8) + BUF->len[1] - (BUF->vhl & 0x0f) * 4;
    offset = (((BUF->ipoffset[0] & 0x3f) << 8) + BUF->ipoffset[1]) * 8;

    /* If the offset or the offset + fragment length overflows the
       reassembly buffer, we discard the entire packet. */
    if(offset > UIP_REASS_BUFSIZE ||
       offset + len > UIP_REASS_BUFSIZE) {
      uip_reasstmr = 0;
      goto nullreturn;
    }

    /* Copy the fragment into the reassembly buffer, at the right
       offset. */
    memcpy(&uip_reassbuf[UIP_IPH_LEN + offset],
	   (char *)BUF + (int)((BUF->vhl & 0x0f) * 4),
	   len);

    /* Update the bitmap. */
    if(offset / (8 * 8) == (offset + len) / (8 * 8)) {
      /* If the two endpoints are in the same byte, we only update
	 that byte. */

      uip_reassbitmap[offset / (8 * 8)] |=
	     bitmap_bits[(offset / 8 ) & 7] &
	     ~bitmap_bits[((offset + len) / 8 ) & 7];
    } else {
      /* If the two endpoints are in different bytes, we update the
	 bytes in the endpoints and fill the stuff inbetween with
	 0xff. */
      uip_reassbitmap[offset / (8 * 8)] |=
	bitmap_bits[(offset / 8 ) & 7];
      for(i = 1 + offset / (8 * 8); i < (offset + len) / (8 * 8); ++i) {
	uip_reassbitmap[i] = 0xff;
      }
      uip_reassbitmap[(offset + len) / (8 * 8)] |=
	~bitmap_bits[((offset + len) / 8 ) & 7];
    }

    /* If this fragment has the More Fragments flag set to zero, we
       know that this is the last fragment, so we can calculate the
       size of the entire packet. We also set the
       IP_REASS_FLAG_LASTFRAG flag to indicate that we have received
       the final fragment. */

    if((BUF->ipoffset[0] & IP_MF) == 0) {
      uip_reassflags |= UIP_REASS_FLAG_LASTFRAG;
      uip_reasslen = offset + len;
    }

    /* Finally, we check if we have a full packet in the buffer. We do
       this by checking if we have the last fragment and if all bits
       in the bitmap are set. */
    if(uip_reassflags & UIP_REASS_FLAG_LASTFRAG) {
      /* Check all bytes up to and including all but the last byte in
	 the bitmap. */
      for(i = 0; i < uip_reasslen / (8 * 8) - 1; ++i) {
	if(uip_reassbitmap[i] != 0xff) {
	  goto nullreturn;
	}
      }
      /* Check the last byte in the bitmap. It should contain just the
	 right amount of bits. */
      if(uip_reassbitmap[uip_reasslen / (8 * 8)] !=
	 (uint8_t)~bitmap_bits[uip_reasslen / 8 & 7]) {
	goto nullreturn;
      }

      /* If we have come this far, we have a full packet in the
	 buffer, so we allocate a pbuf and copy the packet into it. We
	 also reset the timer. */
      uip_reasstmr = 0;
      memcpy(BUF, FBUF, uip_reasslen);

      /* Pretend to be a "normal" (i.e., not fragmented) IP packet
	 from now on. */
      BUF->ipoffset[0] = BUF->ipoffset[1] = 0;
      BUF->len[0] = uip_reasslen >> 8;
      BUF->len[1] = uip_reasslen & 0xff;
      BUF->ipchksum = 0;
      BUF->ipchksum = ~(uip_ipchksum());

      return uip_reasslen;
    }
  }

 nullreturn:
  return 0;
}
#endif /* UIP_REASSEMBLY */
/*---------------------------------------------------------------------------*/
#if UIP_TCP
static void
uip_add_rcv_nxt(struct net_buf *buf, uint16_t n)
{
  uip_add32(uip_conn(buf)->rcv_nxt, n);
  uip_conn(buf)->rcv_nxt[0] = uip_acc32[0];
  uip_conn(buf)->rcv_nxt[1] = uip_acc32[1];
  uip_conn(buf)->rcv_nxt[2] = uip_acc32[2];
  uip_conn(buf)->rcv_nxt[3] = uip_acc32[3];
}
#endif /* UIP_TCP */

#if UIP_TCP
static inline void handle_tcp_retransmit_timer(struct net_buf *not_used,
					       void *ptr)
{
  struct uip_conn *conn = ptr;

  PRINTF("%s: connection %p buf %p\n", __func__, conn, conn ? conn->buf : 0);
  if (conn && conn->buf) {
    conn->timer = 0;
    if (uip_process(&conn->buf, UIP_TIMER)) {
      tcpip_resend_syn(conn, conn->buf);
    }
  }
}

static inline void tcp_set_retrans_timer(struct uip_conn *conn)
{
  ctimer_set(NULL, &conn->retransmit_timer, CLOCK_SECOND,
	     &handle_tcp_retransmit_timer, conn);
}

static inline void tcp_cancel_retrans_timer(struct uip_conn *conn)
{
  ctimer_stop(&conn->retransmit_timer);
}
#endif /* UIP_TCP */
/*---------------------------------------------------------------------------*/
uint8_t
uip_process(struct net_buf **buf_out, uint8_t flag)
{
  struct net_buf *buf = *buf_out;
#if UIP_TCP
  register struct uip_conn *uip_connr = uip_conn(buf);
#endif

#if UIP_UDP
  int i;
  if(flag == UIP_UDP_SEND_CONN) {
    goto udp_send;
  }
#endif /* UIP_UDP */
#if UIP_TCP
  if(flag != UIP_TCP_SEND_CONN) {
#endif
  uip_sappdata(buf) = uip_appdata(buf) = &uip_buf(buf)[UIP_IPTCPH_LEN + UIP_LLH_LEN];
#if UIP_TCP
  }
#endif
  /* Check if we were invoked because of a poll request for a
     particular connection. */
#if UIP_TCP
  if(flag == UIP_POLL_REQUEST || flag == UIP_TCP_SEND_CONN) {

    /* If the connection is not found, and we are initiating the
     * connection, try to get it.
     */
    if (!uip_connr) {
      uip_connr = net_context_get_internal_connection(ip_buf_context(buf));
      if (!uip_connr) {
        PRINTF("No matching connection found for buf %p\n", buf);
        ip_buf_sent_status(buf) = -ENOTCONN;
	return 0;
      } else {
        uip_set_conn(buf) = uip_connr;
        PRINTF("Using connection %p for buf %p\n", uip_conn(buf), buf);
      }
    }

    if((uip_connr->tcpstateflags & UIP_TS_MASK) == UIP_ESTABLISHED &&
       !uip_outstanding(uip_connr)) {
      if (flag == UIP_POLL) {
        uip_flags(buf) = UIP_POLL;
      }
      UIP_APPCALL(buf);
      goto appsend;
#if UIP_ACTIVE_OPEN
    } else if((uip_connr->tcpstateflags & UIP_TS_MASK) == UIP_SYN_SENT) {
      if (uip_connr->nrtx > UIP_MAXSYNRTX) {
        /* SYN has been sent too many times, just stop the connection.
	 */
	PRINTF("Too many SYN sent, dropping connection %p\n", uip_connr);
	net_context_set_connection_status(ip_buf_context(buf), -ETIMEDOUT);
	goto drop;
      }

      /* In the SYN_SENT state, we retransmit out SYN. */
      BUF(buf)->flags = 0;
      goto tcp_send_syn;
#endif /* UIP_ACTIVE_OPEN */
    }
    if (flag == UIP_TCP_SEND_CONN) {
      switch (uip_connr->tcpstateflags & UIP_TS_MASK) {
      case UIP_CLOSED:
      case UIP_FIN_WAIT_1:
      case UIP_FIN_WAIT_2:
      case UIP_CLOSING:
      case UIP_TIME_WAIT:
        ip_buf_sent_status(buf) = -ECONNABORTED;
	goto drop;
      }
      if (uip_outstanding(uip_connr)) {
        ip_buf_sent_status(buf) = -EAGAIN;
        PRINTF("Retry to send packet len %d, outstanding data len %d, "
	       "conn %p\n", uip_len(buf), uip_outstanding(uip_connr),
		uip_connr);
	flag = UIP_TIMER;
	goto tcp_retry;
      }
    }
    goto drop;
  }
#else /* TCP */
  if(flag == UIP_POLL_REQUEST) {
    goto drop;
  }
#endif /* UIP_TCP */

    /* Check if we were invoked because of the periodic timer firing. */
  if(flag == UIP_TIMER) {
#if UIP_REASSEMBLY
    if(uip_reasstmr != 0) {
      --uip_reasstmr;
    }
#endif /* UIP_REASSEMBLY */

#if UIP_TCP
    /* Increase the initial sequence number. */
    if(++iss[3] == 0) {
      if(++iss[2] == 0) {
	if(++iss[1] == 0) {
	  ++iss[0];
	}
      }
    }

    /* Reset the length variables. */
    uip_len(buf) = 0;
    uip_slen(buf) = 0;
  tcp_retry:
    /* Check if the connection is in a state in which we simply wait
       for the connection to time out. If so, we increase the
       connection's timer and remove the connection if it times
       out. */
    if(uip_connr->tcpstateflags == UIP_TIME_WAIT ||
       uip_connr->tcpstateflags == UIP_FIN_WAIT_2) {
      ++(uip_connr->timer);
      if(uip_connr->timer == UIP_TIME_WAIT_TIMEOUT) {
	uip_connr->tcpstateflags = UIP_CLOSED;
      }
    } else if(uip_connr->tcpstateflags != UIP_CLOSED) {
      if (!uip_connr->buf) {
        /* There cannot be any data pending if buf is NULL */
        uip_outstanding(uip_connr) = 0;
      }

      /* If the connection has outstanding data, we increase the
	 connection's timer and see if it has reached the RTO value
	 in which case we retransmit. */

      if(uip_outstanding(uip_connr)) {
	if(uip_connr->timer-- == 0) {
	  if(uip_connr->nrtx == UIP_MAXRTX ||
	     ((uip_connr->tcpstateflags == UIP_SYN_SENT ||
	       uip_connr->tcpstateflags == UIP_SYN_RCVD) &&
	      uip_connr->nrtx == UIP_MAXSYNRTX)) {
	    uip_connr->tcpstateflags = UIP_CLOSED;

	    /* We call UIP_APPCALL() with uip_flags set to
	       UIP_TIMEDOUT to inform the application that the
	       connection has timed out. */
	    uip_flags(buf) = UIP_TIMEDOUT;
	    UIP_APPCALL(buf);

	    /* We also send a reset packet to the remote host. */
	    BUF(buf)->flags = TCP_RST | TCP_ACK;
	    goto tcp_send_nodata;
	  }

	  /* Exponential backoff. */
	  uip_connr->timer = UIP_RTO << (uip_connr->nrtx > 4?
					 4:
					 uip_connr->nrtx);
	  ++(uip_connr->nrtx);

	  /* Ok, so we need to retransmit. We do this differently
	     depending on which state we are in. In ESTABLISHED, we
	     call upon the application so that it may prepare the
	     data for the retransmit. In SYN_RCVD, we resend the
	     SYNACK that we sent earlier and in LAST_ACK we have to
	     retransmit our FINACK. */
	  UIP_STAT(++uip_stat.tcp.rexmit);
	  switch(uip_connr->tcpstateflags & UIP_TS_MASK) {
	  case UIP_SYN_RCVD:
	    /* In the SYN_RCVD state, we should retransmit our
               SYNACK. */
	    goto tcp_send_synack;

#if UIP_ACTIVE_OPEN
	  case UIP_SYN_SENT:
	    /* In the SYN_SENT state, we retransmit out SYN. */
	    BUF(buf)->flags = 0;
	    goto tcp_send_syn;
#endif /* UIP_ACTIVE_OPEN */

	  case UIP_ESTABLISHED:
	    /* In the ESTABLISHED state, we call upon the application
               to do the actual retransmit after which we jump into
               the code for sending out the packet (the apprexmit
               label). */
	    uip_flags(buf) = UIP_REXMIT;
	    UIP_APPCALL(buf);
	    goto apprexmit;

	  case UIP_FIN_WAIT_1:
	  case UIP_CLOSING:
	  case UIP_LAST_ACK:
	    /* In all these states we should retransmit a FINACK. */
	    goto tcp_send_finack;

	  }
	}
      } else if((uip_connr->tcpstateflags & UIP_TS_MASK) == UIP_ESTABLISHED) {
	/* If there was no need for a retransmission, we poll the
           application for new data. */
        uip_flags(buf) = UIP_POLL;
	UIP_APPCALL(buf);
	goto appsend;
      }
    }
#endif
    goto drop;
  }
#if UIP_UDP
  if(flag == UIP_UDP_TIMER) {
    if(uip_udp_conn(buf)->lport != 0) {
      uip_set_conn(buf) = NULL;
      uip_sappdata(buf) = uip_appdata(buf) = &uip_buf(buf)[UIP_LLH_LEN + UIP_IPUDPH_LEN];
      uip_len(buf) = uip_slen(buf) = 0;
      uip_flags(buf) = UIP_POLL;
      UIP_UDP_APPCALL(buf);
      goto udp_send;
    } else {
      goto drop;
    }
  }
#endif

  /* This is where the input processing starts. */
  UIP_STAT(++uip_stat.ip.recv);

  /* Start of IP input header processing code. */

#if NETSTACK_CONF_WITH_IPV6
  /* Check validity of the IP header. */
  if((BUF(buf)->vtc & 0xf0) != 0x60)  { /* IP version and header length. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.vhlerr);
    UIP_LOG("ipv6: invalid version.");
    goto drop;
  }
#else /* NETSTACK_CONF_WITH_IPV6 */
  /* Check validity of the IP header. */
  if(BUF(buf)->vhl != 0x45)  { /* IP version and header length. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.vhlerr);
    UIP_LOG("ip: invalid version or header length.");
    goto drop;
  }
#endif /* NETSTACK_CONF_WITH_IPV6 */

  /* Check the size of the packet. If the size reported to us in
     uip_len is smaller the size reported in the IP header, we assume
     that the packet has been corrupted in transit. If the size of
     uip_len is larger than the size reported in the IP packet header,
     the packet has been padded and we set uip_len to the correct
     value. */

  if((BUF(buf)->len[0] << 8) + BUF(buf)->len[1] <= uip_len(buf)) {
    uip_len(buf) = (BUF(buf)->len[0] << 8) + BUF(buf)->len[1];
#if NETSTACK_CONF_WITH_IPV6
    uip_len += 40; /* The length reported in the IPv6 header is the
		      length of the payload that follows the
		      header. However, uIP uses the uip_len variable
		      for holding the size of the entire packet,
		      including the IP header. For IPv4 this is not a
		      problem as the length field in the IPv4 header
		      contains the length of the entire packet. But
		      for IPv6 we need to add the size of the IPv6
		      header (40 bytes). */
#endif /* NETSTACK_CONF_WITH_IPV6 */
  } else {
    UIP_LOG("ip: packet shorter than reported in IP header.");
    goto drop;
  }

#if !NETSTACK_CONF_WITH_IPV6
  /* Check the fragment flag. */
  if((BUF(buf)->ipoffset[0] & 0x3f) != 0 ||
     BUF(buf)->ipoffset[1] != 0) {
#if UIP_REASSEMBLY
    uip_len = uip_reass();
    if(uip_len == 0) {
      goto drop;
    }
#else /* UIP_REASSEMBLY */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.fragerr);
    UIP_LOG("ip: fragment dropped.");
    goto drop;
#endif /* UIP_REASSEMBLY */
  }
#endif /* NETSTACK_CONF_WITH_IPV6 */

  if(uip_ipaddr_cmp(&uip_hostaddr, &uip_all_zeroes_addr)) {
    /* If we are configured to use ping IP address configuration and
       haven't been assigned an IP address yet, we accept all ICMP
       packets. */
#if UIP_PINGADDRCONF && !NETSTACK_CONF_WITH_IPV6
    if(BUF->proto == UIP_PROTO_ICMP) {
      UIP_LOG("ip: possible ping config packet received.");
      goto icmp_input;
    } else {
      UIP_LOG("ip: packet dropped since no address assigned.");
      goto drop;
    }
#endif /* UIP_PINGADDRCONF */

  } else {
    /* If IP broadcast support is configured, we check for a broadcast
       UDP packet, which may be destined to us. */
#if UIP_BROADCAST
    DEBUG_PRINTF("UDP IP checksum 0x%04x\n", uip_ipchksum());
    if(BUF->proto == UIP_PROTO_UDP &&
       (uip_ipaddr_cmp(&BUF->destipaddr, &uip_broadcast_addr) ||
	(BUF->destipaddr.u8[0] & 224) == 224)) {  /* XXX this is a
						     hack to be able
						     to receive UDP
						     multicast
						     packets. We check
						     for the bit
						     pattern of the
						     multicast
						     prefix. */
      goto udp_input;
    }
#endif /* UIP_BROADCAST */

    /* Check if the packet is destined for our IP address. */
#if !NETSTACK_CONF_WITH_IPV6
#ifdef CONFIG_DHCP
    /* DHCP message destination address in DHCP OFFER and ACK
     * packets, destination address is 255.255.255.255, so skip
     * addres comparison in this case
     */
    if(BUF(buf)->proto == UIP_PROTO_UDP) {
       uip_appdata(buf) = &uip_buf(buf)[UIP_LLH_LEN + UIP_IPUDPH_LEN];
    }

    if(msg_for_dhcpc(buf)) {
      if(!(uip_ipaddr_cmp(&BUF(buf)->destipaddr, &uip_hostaddr) ||
          uip_ipaddr_cmp(&BUF(buf)->destipaddr, &uip_broadcast_addr))) {
         UIP_STAT(++uip_stat.ip.drop);
         goto drop;
      }
    } else
#endif
    if(!uip_ipaddr_cmp(&BUF(buf)->destipaddr, &uip_hostaddr)) {
      UIP_STAT(++uip_stat.ip.drop);
      goto drop;
    }
#else /* NETSTACK_CONF_WITH_IPV6 */
    /* For IPv6, packet reception is a little trickier as we need to
       make sure that we listen to certain multicast addresses (all
       hosts multicast address, and the solicited-node multicast
       address) as well. However, we will cheat here and accept all
       multicast packets that are sent to the ff02::/16 addresses. */
    if(!uip_ipaddr_cmp(&BUF->destipaddr, &uip_hostaddr) &&
       BUF->destipaddr.u16[0] != UIP_HTONS(0xff02)) {
      UIP_STAT(++uip_stat.ip.drop);
      goto drop;
    }
#endif /* NETSTACK_CONF_WITH_IPV6 */
  }

#if !NETSTACK_CONF_WITH_IPV6
  if(uip_ipchksum(buf) != 0xffff) { /* Compute and check the IP header
				    checksum. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.chkerr);
    UIP_LOG("ip: bad checksum.");
    goto drop;
  }
#endif /* NETSTACK_CONF_WITH_IPV6 */

#if UIP_TCP
  if(BUF(buf)->proto == UIP_PROTO_TCP) { /* Check for TCP packet. If so,
				       proceed with TCP input
				       processing. */
    goto tcp_input;
  }
#endif

#if UIP_UDP
  if(BUF(buf)->proto == UIP_PROTO_UDP) {
    goto udp_input;
  }
#endif /* UIP_UDP */

#if !NETSTACK_CONF_WITH_IPV6
  /* ICMPv4 processing code follows. */
  if(BUF(buf)->proto != UIP_PROTO_ICMP) { /* We only allow ICMP packets from
					here. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.protoerr);
    UIP_LOG("ip: neither tcp nor icmp.");
    goto drop;
  }

#if UIP_PINGADDRCONF
 icmp_input:
#endif /* UIP_PINGADDRCONF */
  UIP_STAT(++uip_stat.icmp.recv);

  /* ICMP echo (i.e., ping) processing. This is simple, we only change
     the ICMP type from ECHO to ECHO_REPLY and adjust the ICMP
     checksum before we return the packet. */
  if(ICMPBUF(buf)->type != ICMP_ECHO) {
    UIP_STAT(++uip_stat.icmp.drop);
    UIP_STAT(++uip_stat.icmp.typeerr);
    UIP_LOG("icmp: not icmp echo.");
    goto drop;
  }

  /* If we are configured to use ping IP address assignment, we use
     the destination IP address of this ping packet and assign it to
     ourself. */
#if UIP_PINGADDRCONF
  if(uip_ipaddr_cmp(&uip_hostaddr, &uip_all_zeroes_addr)) {
    uip_hostaddr = BUF->destipaddr;
  }
#endif /* UIP_PINGADDRCONF */

  ICMPBUF(buf)->type = ICMP_ECHO_REPLY;

  if(ICMPBUF(buf)->icmpchksum >= UIP_HTONS(0xffff - (ICMP_ECHO << 8))) {
    ICMPBUF(buf)->icmpchksum += UIP_HTONS(ICMP_ECHO << 8) + 1;
  } else {
    ICMPBUF(buf)->icmpchksum += UIP_HTONS(ICMP_ECHO << 8);
  }

  /* Swap IP addresses. */
  uip_ipaddr_copy(&BUF(buf)->destipaddr, &BUF(buf)->srcipaddr);
  uip_ipaddr_copy(&BUF(buf)->srcipaddr, &uip_hostaddr);

  UIP_STAT(++uip_stat.icmp.sent);
  BUF(buf)->ttl = UIP_TTL;
  goto ip_send_nolen;

  /* End of IPv4 input header processing code. */
#else /* !NETSTACK_CONF_WITH_IPV6 */

  /* This is IPv6 ICMPv6 processing code. */
  DEBUG_PRINTF("icmp6_input: length %d\n", uip_len);

  if(BUF->proto != UIP_PROTO_ICMP6) { /* We only allow ICMPv6 packets from
					 here. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.protoerr);
    UIP_LOG("ip: neither tcp nor icmp6.");
    goto drop;
  }

  UIP_STAT(++uip_stat.icmp.recv);

  /* If we get a neighbor solicitation for our address we should send
     a neighbor advertisement message back. */
  if(ICMPBUF->type == ICMP6_NEIGHBOR_SOLICITATION) {
    if(uip_ipaddr_cmp(&ICMPBUF->icmp6data, &uip_hostaddr)) {

      if(ICMPBUF->options[0] == ICMP6_OPTION_SOURCE_LINK_ADDRESS) {
	/* Save the sender's address in our neighbor list. */
	uip_neighbor_add(&ICMPBUF->srcipaddr, &(ICMPBUF->options[2]));
      }

      /* We should now send a neighbor advertisement back to where the
	 neighbor solicication came from. */
      ICMPBUF->type = ICMP6_NEIGHBOR_ADVERTISEMENT;
      ICMPBUF->flags = ICMP6_FLAG_S; /* Solicited flag. */

      ICMPBUF->reserved1 = ICMPBUF->reserved2 = ICMPBUF->reserved3 = 0;

      uip_ipaddr_copy(&ICMPBUF->destipaddr, &ICMPBUF->srcipaddr);
      uip_ipaddr_copy(&ICMPBUF->srcipaddr, &uip_hostaddr);
      ICMPBUF->options[0] = ICMP6_OPTION_TARGET_LINK_ADDRESS;
      ICMPBUF->options[1] = 1;  /* Options length, 1 = 8 bytes. */
      memcpy(&(ICMPBUF->options[2]), &uip_lladdr, sizeof(uip_lladdr));
      ICMPBUF->icmpchksum = 0;
      ICMPBUF->icmpchksum = ~uip_icmp6chksum();

      goto send;

    }
    goto drop;
  } else if(ICMPBUF->type == ICMP6_ECHO) {
    /* ICMP echo (i.e., ping) processing. This is simple, we only
       change the ICMP type from ECHO to ECHO_REPLY and update the
       ICMP checksum before we return the packet. */

    ICMPBUF->type = ICMP6_ECHO_REPLY;

    uip_ipaddr_copy(&BUF->destipaddr, &BUF->srcipaddr);
    uip_ipaddr_copy(&BUF->srcipaddr, &uip_hostaddr);
    ICMPBUF->icmpchksum = 0;
    ICMPBUF->icmpchksum = ~uip_icmp6chksum();

    UIP_STAT(++uip_stat.icmp.sent);
    goto send;
  } else {
    DEBUG_PRINTF("Unknown icmp6 message type %d\n", ICMPBUF->type);
    UIP_STAT(++uip_stat.icmp.drop);
    UIP_STAT(++uip_stat.icmp.typeerr);
    UIP_LOG("icmp: unknown ICMP message.");
    goto drop;
  }

  /* End of IPv6 ICMP processing. */

#endif /* !NETSTACK_CONF_WITH_IPV6 */

#if UIP_UDP
  /* UDP input processing. */
 udp_input:
  /* UDP processing is really just a hack. We don't do anything to the
     UDP/IP headers, but let the UDP application do all the hard
     work. If the application sets uip_slen, it has a packet to
     send. */
#if UIP_UDP_CHECKSUMS
  uip_len = uip_len - UIP_IPUDPH_LEN;
  uip_appdata = &uip_buf[UIP_LLH_LEN + UIP_IPUDPH_LEN];
  if(UDPBUF->udpchksum != 0 && uip_udpchksum() != 0xffff) {
    UIP_STAT(++uip_stat.udp.drop);
    UIP_STAT(++uip_stat.udp.chkerr);
    UIP_LOG("udp: bad checksum.");
    goto drop;
  }
#else /* UIP_UDP_CHECKSUMS */
  uip_len(buf) = uip_len(buf) - UIP_IPUDPH_LEN;
#endif /* UIP_UDP_CHECKSUMS */

  /* Make sure that the UDP destination port number is not zero. */
  if(UDPBUF(buf)->destport == 0) {
    UIP_LOG("udp: zero port.");
    goto drop;
  }

  /* Demultiplex this UDP packet between the UDP "connections". */
  for(i = 0, uip_set_udp_conn(buf) = &uip_udp_conns[0];
      i < UIP_UDP_CONNS && uip_udp_conn(buf) < &uip_udp_conns[UIP_UDP_CONNS];
      i++, uip_set_udp_conn(buf) += sizeof(struct uip_udp_conn)) {
    /* If the local UDP port is non-zero, the connection is considered
       to be used. If so, the local port number is checked against the
       destination port number in the received packet. If the two port
       numbers match, the remote port number is checked if the
       connection is bound to a remote port. Finally, if the
       connection is bound to a remote IP address, the source IP
       address of the packet is checked. */
    if(uip_udp_conn(buf)->lport != 0 &&
       UDPBUF(buf)->destport == uip_udp_conn(buf)->lport &&
       (uip_udp_conn(buf)->rport == 0 ||
        UDPBUF(buf)->srcport == uip_udp_conn(buf)->rport) &&
       (uip_ipaddr_cmp(&uip_udp_conn(buf)->ripaddr, &uip_all_zeroes_addr) ||
	uip_ipaddr_cmp(&uip_udp_conn(buf)->ripaddr, &uip_broadcast_addr) ||
	uip_ipaddr_cmp(&BUF(buf)->srcipaddr, &uip_udp_conn(buf)->ripaddr))) {
      goto udp_found;
    }
  }
  UIP_LOG("udp: no matching connection found");
  UIP_STAT(++uip_stat.udp.drop);
#if UIP_CONF_ICMP_DEST_UNREACH && !NETSTACK_CONF_WITH_IPV6
  /* Copy fields from packet header into payload of this ICMP packet. */
  memcpy(&(ICMPBUF->payload[0]), ICMPBUF, UIP_IPH_LEN + 8);

  /* Set the ICMP type and code. */
  ICMPBUF->type = ICMP_DEST_UNREACHABLE;
  ICMPBUF->icode = ICMP_PORT_UNREACHABLE;

  /* Calculate the ICMP checksum. */
  ICMPBUF->icmpchksum = 0;
  ICMPBUF->icmpchksum = ~uip_chksum((uint16_t *)&(ICMPBUF->type), 36);

  /* Set the IP destination address to be the source address of the
     original packet. */
  uip_ipaddr_copy(&BUF->destipaddr, &BUF->srcipaddr);

  /* Set our IP address as the source address. */
  uip_ipaddr_copy(&BUF->srcipaddr, &uip_hostaddr);

  /* The size of the ICMP destination unreachable packet is 36 + the
     size of the IP header (20) = 56. */
  uip_len = 36 + UIP_IPH_LEN;
  ICMPBUF->len[0] = 0;
  ICMPBUF->len[1] = (uint8_t)uip_len;
  ICMPBUF->ttl = UIP_TTL;
  ICMPBUF->proto = UIP_PROTO_ICMP;

  goto ip_send_nolen;
#else /* UIP_CONF_ICMP_DEST_UNREACH */
  goto drop;
#endif /* UIP_CONF_ICMP_DEST_UNREACH */

 udp_found:
  PRINTF("In udp_found\n");
  UIP_STAT(++uip_stat.udp.recv);
  uip_set_conn(buf) = NULL;
  uip_flags(buf) = UIP_NEWDATA;
  uip_sappdata(buf) = uip_appdata(buf) = &uip_buf(buf)[UIP_LLH_LEN + UIP_IPUDPH_LEN];
  uip_slen(buf) = 0;
  UIP_UDP_APPCALL(buf);
  if(uip_slen(buf) == 0) {
    /* If the application does not want to send anything, then uip_slen(buf)
     * will be 0. In this case we MUST NOT set uip_len(buf) to 0 as that would
     * cause the net_buf to be released by rx fiber. In this case it is
     * application responsibility to release the buffer.
     */
    return 0;
  }

 udp_send:
  PRINTF("In udp_send\n");
  if(uip_slen(buf) == 0) {
    goto drop;
  }
  uip_len(buf) = uip_slen(buf) + UIP_IPUDPH_LEN;
  ip_buf_len(buf) = uip_len(buf);

#if NETSTACK_CONF_WITH_IPV6
  /* For IPv6, the IP length field does not include the IPv6 IP header
     length. */
  BUF->len[0] = ((uip_len - UIP_IPH_LEN) >> 8);
  BUF->len[1] = ((uip_len - UIP_IPH_LEN) & 0xff);
#else /* NETSTACK_CONF_WITH_IPV6 */
  BUF(buf)->len[0] = (uip_len(buf) >> 8);
  BUF(buf)->len[1] = (uip_len(buf) & 0xff);
#endif /* NETSTACK_CONF_WITH_IPV6 */

  BUF(buf)->ttl = uip_udp_conn(buf)->ttl;
  BUF(buf)->proto = UIP_PROTO_UDP;

  UDPBUF(buf)->udplen = UIP_HTONS(uip_slen(buf) + UIP_UDPH_LEN);
  UDPBUF(buf)->udpchksum = 0;

  BUF(buf)->srcport  = uip_udp_conn(buf)->lport;
  BUF(buf)->destport = uip_udp_conn(buf)->rport;

  uip_ipaddr_copy(&BUF(buf)->srcipaddr, &uip_hostaddr);
  uip_ipaddr_copy(&BUF(buf)->destipaddr, &uip_udp_conn(buf)->ripaddr);

  uip_appdata(buf) = &uip_buf(buf)[UIP_LLH_LEN + UIP_IPTCPH_LEN];

#if UIP_UDP_CHECKSUMS
  /* Calculate UDP checksum. */
  UDPBUF->udpchksum = ~(uip_udpchksum());
  if(UDPBUF->udpchksum == 0) {
    UDPBUF->udpchksum = 0xffff;
  }
#endif /* UIP_UDP_CHECKSUMS */

  UIP_STAT(++uip_stat.udp.sent);
  goto ip_send_nolen;
#endif /* UIP_UDP */

  /* TCP input processing. */
#if UIP_TCP
 tcp_input:
  UIP_STAT(++uip_stat.tcp.recv);

  /* Start of TCP input header processing code. */

  if(uip_tcpchksum(buf) != 0xffff) {   /* Compute and check the TCP
				       checksum. */
    UIP_STAT(++uip_stat.tcp.drop);
    UIP_STAT(++uip_stat.tcp.chkerr);
    UIP_LOG("tcp: bad checksum.");
    goto drop;
  }

  /* Make sure that the TCP port number is not zero. */
  if(BUF(buf)->destport == 0 || BUF(buf)->srcport == 0) {
    UIP_LOG("tcp: zero port.");
    goto drop;
  }

  /* Demultiplex this segment. */
  /* First check any active connections. */
  for(uip_connr = &uip_conns[0]; uip_connr <= &uip_conns[UIP_CONNS - 1];
      ++uip_connr) {
    if(uip_connr->tcpstateflags != UIP_CLOSED &&
       BUF(buf)->destport == uip_connr->lport &&
       BUF(buf)->srcport == uip_connr->rport &&
       uip_ipaddr_cmp(&BUF(buf)->srcipaddr, &uip_connr->ripaddr)) {
      goto found;
    }
  }

  /* If we didn't find an active connection that expected the packet,
     either this packet is an old duplicate, or this is a SYN packet
     destined for a connection in LISTEN. If the SYN flag isn't set,
     it is an old packet and we send a RST. */
  if((BUF(buf)->flags & TCP_CTL) != TCP_SYN) {
    goto reset;
  }

  tmp16 = BUF(buf)->destport;
  /* Next, check listening connections. */
  for(c = 0; c < UIP_LISTENPORTS; ++c) {
    if(tmp16 == uip_listenports[c]) {
      goto found_listen;
    }
  }

  /* No matching connection found, so we send a RST packet. */
  UIP_STAT(++uip_stat.tcp.synrst);

 reset:
  /* We do not send resets in response to resets. */
  if(BUF(buf)->flags & TCP_RST) {
    goto drop;
  }

  UIP_STAT(++uip_stat.tcp.rst);

  BUF(buf)->flags = TCP_RST | TCP_ACK;
  uip_len(buf) = UIP_IPTCPH_LEN;
  BUF(buf)->tcpoffset = 5 << 4;

  /* Flip the seqno and ackno fields in the TCP header. */
  c = BUF(buf)->seqno[3];
  BUF(buf)->seqno[3] = BUF(buf)->ackno[3];
  BUF(buf)->ackno[3] = c;

  c = BUF(buf)->seqno[2];
  BUF(buf)->seqno[2] = BUF(buf)->ackno[2];
  BUF(buf)->ackno[2] = c;

  c = BUF(buf)->seqno[1];
  BUF(buf)->seqno[1] = BUF(buf)->ackno[1];
  BUF(buf)->ackno[1] = c;

  c = BUF(buf)->seqno[0];
  BUF(buf)->seqno[0] = BUF(buf)->ackno[0];
  BUF(buf)->ackno[0] = c;

  /* We also have to increase the sequence number we are
     acknowledging. If the least significant byte overflowed, we need
     to propagate the carry to the other bytes as well. */
  if(++BUF(buf)->ackno[3] == 0) {
    if(++BUF(buf)->ackno[2] == 0) {
      if(++BUF(buf)->ackno[1] == 0) {
	++BUF(buf)->ackno[0];
      }
    }
  }

  /* Swap port numbers. */
  tmp16 = BUF(buf)->srcport;
  BUF(buf)->srcport = BUF(buf)->destport;
  BUF(buf)->destport = tmp16;

  /* Swap IP addresses. */
  uip_ipaddr_copy(&BUF(buf)->destipaddr, &BUF(buf)->srcipaddr);
  uip_ipaddr_copy(&BUF(buf)->srcipaddr, &uip_hostaddr);

  /* And send out the RST packet! */
  goto tcp_send_noconn;

  /* This label will be jumped to if we matched the incoming packet
     with a connection in LISTEN. In that case, we should create a new
     connection and send a SYNACK in return. */
 found_listen:
  PRINTF("In found listen\n");
  /* First we check if there are any connections avaliable. Unused
     connections are kept in the same table as used connections, but
     unused ones have the tcpstate set to CLOSED. Also, connections in
     TIME_WAIT are kept track of and we'll use the oldest one if no
     CLOSED connections are found. Thanks to Eddie C. Dost for a very
     nice algorithm for the TIME_WAIT search. */
  uip_connr = 0;
  for(c = 0; c < UIP_CONNS; ++c) {
    if(uip_conns[c].tcpstateflags == UIP_CLOSED) {
      uip_connr = &uip_conns[c];
      break;
    }
    if(uip_conns[c].tcpstateflags == UIP_TIME_WAIT) {
      if(uip_connr == 0 ||
	 uip_conns[c].timer > uip_connr->timer) {
	uip_connr = &uip_conns[c];
      }
    }
  }

  if(uip_connr == 0) {
    /* All connections are used already, we drop packet and hope that
       the remote end will retransmit the packet at a time when we
       have more spare connections. */
    UIP_STAT(++uip_stat.tcp.syndrop);
    UIP_LOG("tcp: found no unused connections.");
    goto drop;
  }
  uip_set_conn(buf) = uip_connr;

  /* Fill in the necessary fields for the new connection. */
  uip_connr->rto = uip_connr->timer = UIP_RTO;
  uip_connr->sa = 0;
  uip_connr->sv = 4;
  uip_connr->nrtx = 0;
  uip_connr->lport = BUF(buf)->destport;
  uip_connr->rport = BUF(buf)->srcport;
  uip_ipaddr_copy(&uip_connr->ripaddr, &BUF(buf)->srcipaddr);
  uip_connr->tcpstateflags = UIP_SYN_RCVD;

  uip_connr->snd_nxt[0] = iss[0];
  uip_connr->snd_nxt[1] = iss[1];
  uip_connr->snd_nxt[2] = iss[2];
  uip_connr->snd_nxt[3] = iss[3];
  uip_connr->len = 1;

  if (flag == UIP_TCP_SEND_CONN) {
    /* So we are trying send some data to other host */
    if (uip_connr->buf && uip_connr->buf != buf) {
      uip_connr->buf = ip_buf_ref(buf);
    }
  }

  /* rcv_nxt should be the seqno from the incoming packet + 1. */
  uip_connr->rcv_nxt[3] = BUF(buf)->seqno[3];
  uip_connr->rcv_nxt[2] = BUF(buf)->seqno[2];
  uip_connr->rcv_nxt[1] = BUF(buf)->seqno[1];
  uip_connr->rcv_nxt[0] = BUF(buf)->seqno[0];
  uip_add_rcv_nxt(buf, 1);

  /* Parse the TCP MSS option, if present. */
  if((BUF(buf)->tcpoffset & 0xf0) > 0x50) {
    for(c = 0; c < ((BUF(buf)->tcpoffset >> 4) - 5) << 2 ;) {
      opt = uip_buf(buf)[UIP_TCPIP_HLEN + UIP_LLH_LEN + c];
      if(opt == TCP_OPT_END) {
	/* End of options. */
	break;
      } else if(opt == TCP_OPT_NOOP) {
	++c;
	/* NOP option. */
      } else if(opt == TCP_OPT_MSS &&
		uip_buf(buf)[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == TCP_OPT_MSS_LEN) {
	/* An MSS option with the right option length. */
	tmp16 = ((uint16_t)uip_buf(buf)[UIP_TCPIP_HLEN + UIP_LLH_LEN + 2 + c] << 8) |
	  (uint16_t)uip_buf(buf)[UIP_IPTCPH_LEN + UIP_LLH_LEN + 3 + c];
	uip_connr->initialmss = uip_connr->mss =
	  tmp16 > UIP_TCP_MSS? UIP_TCP_MSS: tmp16;

	/* And we are done processing options. */
	break;
      } else {
	/* All other options have a length field, so that we easily
	   can skip past them. */
	if(uip_buf(buf)[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == 0) {
	  /* If the length field is zero, the options are malformed
	     and we don't process them further. */
	  break;
	}
	c += uip_buf(buf)[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c];
      }
    }
  }

  /* Our response will be a SYNACK. */
#if UIP_ACTIVE_OPEN
 tcp_send_synack:
  BUF(buf)->flags = TCP_ACK;

 tcp_send_syn:
  BUF(buf)->flags |= TCP_SYN;
  tcp_set_retrans_timer(uip_connr);
#else /* UIP_ACTIVE_OPEN */
 tcp_send_synack:
  BUF(buf)->flags = TCP_SYN | TCP_ACK;
#endif /* UIP_ACTIVE_OPEN */

  /* We send out the TCP Maximum Segment Size option with our
     SYNACK. */
  BUF(buf)->optdata[0] = TCP_OPT_MSS;
  BUF(buf)->optdata[1] = TCP_OPT_MSS_LEN;
  BUF(buf)->optdata[2] = (UIP_TCP_MSS) / 256;
  BUF(buf)->optdata[3] = (UIP_TCP_MSS) & 255;
  uip_len(buf) = UIP_IPTCPH_LEN + TCP_OPT_MSS_LEN;
  BUF(buf)->tcpoffset = ((UIP_TCPH_LEN + TCP_OPT_MSS_LEN) / 4) << 4;
  goto tcp_send;

  /* This label will be jumped to if we found an active connection. */
 found:
  PRINTF("In found\n");
  uip_set_conn(buf) = uip_connr;
  uip_flags(buf) = 0;
  /* We do a very naive form of TCP reset processing; we just accept
     any RST and kill our connection. We should in fact check if the
     sequence number of this reset is within our advertised window
     before we accept the reset. */
  if(BUF(buf)->flags & TCP_RST) {
    uip_connr->tcpstateflags = UIP_CLOSED;
    UIP_LOG("tcp: got reset, aborting connection.");
    uip_flags(buf) = UIP_ABORT;
    UIP_APPCALL(buf);
    goto drop;
  }

  if (flag != UIP_TCP_SEND_CONN) {
    /* If flag is set to UIP_TCP_SEND_CONN, then it means that we are
     * trying to send the actual packet coming from user. In this case
     * do not mess with the packet length.
     */

    /* Calculate the length of the data, if the application has sent
       any data to us. */
    c = (BUF(buf)->tcpoffset >> 4) << 2;
    /* uip_len will contain the length of the actual TCP data. This is
       calculated by subtracing the length of the TCP header (in
       c) and the length of the IP header (20 bytes). */
    uip_len(buf) = uip_len(buf) - c - UIP_IPH_LEN;
  }

  /* First, check if the sequence number of the incoming packet is
     what we're expecting next. If not, we send out an ACK with the
     correct numbers in, unless we are in the SYN_RCVD state and
     receive a SYN, in which case we should retransmit our SYNACK
     (which is done futher down). */
  if(!((((uip_connr->tcpstateflags & UIP_TS_MASK) == UIP_SYN_SENT) &&
	((BUF(buf)->flags & TCP_CTL) == (TCP_SYN | TCP_ACK))) ||
       (((uip_connr->tcpstateflags & UIP_TS_MASK) == UIP_SYN_RCVD) &&
	((BUF(buf)->flags & TCP_CTL) == TCP_SYN)))) {
    if((uip_len(buf) > 0 || ((BUF(buf)->flags & (TCP_SYN | TCP_FIN)) != 0)) &&
       (BUF(buf)->seqno[0] != uip_connr->rcv_nxt[0] ||
	BUF(buf)->seqno[1] != uip_connr->rcv_nxt[1] ||
	BUF(buf)->seqno[2] != uip_connr->rcv_nxt[2] ||
	BUF(buf)->seqno[3] != uip_connr->rcv_nxt[3])) {
      goto tcp_send_ack;
    }
  }

  /* Next, check if the incoming segment acknowledges any outstanding
     data. If so, we update the sequence number, reset the length of
     the outstanding data, calculate RTT estimations, and reset the
     retransmission timer. */
  if((BUF(buf)->flags & TCP_ACK) && uip_outstanding(uip_connr)) {
    uip_add32(uip_connr->snd_nxt, uip_connr->len);

    if(BUF(buf)->ackno[0] == uip_acc32[0] &&
       BUF(buf)->ackno[1] == uip_acc32[1] &&
       BUF(buf)->ackno[2] == uip_acc32[2] &&
       BUF(buf)->ackno[3] == uip_acc32[3]) {
      /* Update sequence number. */
      uip_connr->snd_nxt[0] = uip_acc32[0];
      uip_connr->snd_nxt[1] = uip_acc32[1];
      uip_connr->snd_nxt[2] = uip_acc32[2];
      uip_connr->snd_nxt[3] = uip_acc32[3];

      /* Do RTT estimation, unless we have done retransmissions. */
      if(uip_connr->nrtx == 0) {
	signed char m;
	m = uip_connr->rto - uip_connr->timer;
	/* This is taken directly from VJs original code in his paper */
	m = m - (uip_connr->sa >> 3);
	uip_connr->sa += m;
	if(m < 0) {
	  m = -m;
	}
	m = m - (uip_connr->sv >> 2);
	uip_connr->sv += m;
	uip_connr->rto = (uip_connr->sa >> 3) + uip_connr->sv;

      }
      /* Set the acknowledged flag. */
      uip_flags(buf) = UIP_ACKDATA;
      /* Reset the retransmission timer. */
      uip_connr->timer = uip_connr->rto;

      /* Reset length of outstanding data. */
      uip_connr->len = 0;
    }

  }

  /* Do different things depending on in what state the connection is. */
  switch(uip_connr->tcpstateflags & UIP_TS_MASK) {
    /* CLOSED and LISTEN are not handled here. CLOSE_WAIT is not
	implemented, since we force the application to close when the
	peer sends a FIN (hence the application goes directly from
	ESTABLISHED to LAST_ACK). */
  case UIP_SYN_RCVD:
    /* In SYN_RCVD we have sent out a SYNACK in response to a SYN, and
       we are waiting for an ACK that acknowledges the data we sent
       out the last time. Therefore, we want to have the UIP_ACKDATA
       flag set. If so, we enter the ESTABLISHED state. */
    if(uip_flags(buf) & UIP_ACKDATA) {
      uip_connr->tcpstateflags = UIP_ESTABLISHED;
      uip_flags(buf) = UIP_CONNECTED;
      uip_connr->len = 0;
      if(uip_len(buf) > 0) {
        uip_flags(buf) |= UIP_NEWDATA;
        uip_add_rcv_nxt(buf, uip_len(buf));
      }
      uip_slen(buf) = 0;
      UIP_APPCALL(buf);
      goto appsend;
    }
    /* We need to retransmit the SYNACK */
    if((BUF(buf)->flags & TCP_CTL) == TCP_SYN) {
      goto tcp_send_synack;
    }
    goto drop;
#if UIP_ACTIVE_OPEN
  case UIP_SYN_SENT:
    /* In SYN_SENT, we wait for a SYNACK that is sent in response to
       our SYN. The rcv_nxt is set to sequence number in the SYNACK
       plus one, and we send an ACK. We move into the ESTABLISHED
       state. */
    if((uip_flags(buf) & UIP_ACKDATA) &&
       (BUF(buf)->flags & TCP_CTL) == (TCP_SYN | TCP_ACK)) {

      /* Parse the TCP MSS option, if present. */
      if((BUF(buf)->tcpoffset & 0xf0) > 0x50) {
	for(c = 0; c < ((BUF(buf)->tcpoffset >> 4) - 5) << 2 ;) {
	  opt = uip_buf(buf)[UIP_IPTCPH_LEN + UIP_LLH_LEN + c];
	  if(opt == TCP_OPT_END) {
	    /* End of options. */
	    break;
	  } else if(opt == TCP_OPT_NOOP) {
	    ++c;
	    /* NOP option. */
	  } else if(opt == TCP_OPT_MSS &&
		    uip_buf(buf)[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == TCP_OPT_MSS_LEN) {
	    /* An MSS option with the right option length. */
	    tmp16 = (uip_buf(buf)[UIP_TCPIP_HLEN + UIP_LLH_LEN + 2 + c] << 8) |
	      uip_buf(buf)[UIP_TCPIP_HLEN + UIP_LLH_LEN + 3 + c];
	    uip_connr->initialmss =
	      uip_connr->mss = tmp16 > UIP_TCP_MSS? UIP_TCP_MSS: tmp16;

	    /* And we are done processing options. */
	    break;
	  } else {
	    /* All other options have a length field, so that we easily
	       can skip past them. */
	    if(uip_buf(buf)[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == 0) {
	      /* If the length field is zero, the options are malformed
		 and we don't process them further. */
	      break;
	    }
	    c += uip_buf(buf)[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c];
	  }
	}
      }
      uip_connr->tcpstateflags = UIP_ESTABLISHED;
      uip_connr->rcv_nxt[0] = BUF(buf)->seqno[0];
      uip_connr->rcv_nxt[1] = BUF(buf)->seqno[1];
      uip_connr->rcv_nxt[2] = BUF(buf)->seqno[2];
      uip_connr->rcv_nxt[3] = BUF(buf)->seqno[3];
      uip_add_rcv_nxt(buf, 1);
      uip_flags(buf) = UIP_CONNECTED | UIP_NEWDATA;
      uip_connr->len = 0;
      uip_len(buf) = 0;
      uip_slen(buf) = 0;
      ip_buf_sent_status(buf) = 0;
      uip_set_conn(buf) = uip_connr;

      if (uip_connr->buf) {
        /* Now that we know the original connection request, clear
	 * the buf in connr
	 */
        net_context_set_connection_status(ip_buf_context(uip_connr->buf), 0);
	net_context_set_internal_connection(ip_buf_context(uip_connr->buf),
					    uip_connr);
	tcp_cancel_retrans_timer(uip_connr);

	/* We received ACK for syn */
	if (uip_connr->buf) {
          net_context_set_connection_status(ip_buf_context(uip_connr->buf), -EINPROGRESS);
	}

	/* Now send the pending data */
	buf = uip_connr->buf;
	*buf_out = buf;

	uip_flags(buf) = UIP_CONNECTED;
      }
      /* Right now we have received SYN-ACK so we can now start to send data.
       * The UIP_APPCALL() will cause a call to this function by the
       * net_context.c TCP process thread which will call
       * handle_tcp_connection().
       */
      UIP_APPCALL(buf);
      PRINTF("Returning now buf %p ref %p\n", buf, buf->ref);
      return 0;
    }
    /* Inform the application that the connection failed */
    uip_flags(buf) = UIP_ABORT;
    UIP_APPCALL(buf);
    /* The connection is closed after we send the RST */
    uip_conn(buf)->tcpstateflags = UIP_CLOSED;
    goto reset;
#endif /* UIP_ACTIVE_OPEN */

  case UIP_ESTABLISHED:
    /* In the ESTABLISHED state, we call upon the application to feed
    data into the uip_buf. If the UIP_ACKDATA flag is set, the
    application should put new data into the buffer, otherwise we are
    retransmitting an old segment, and the application should put that
    data into the buffer.

    If the incoming packet is a FIN, we should close the connection on
    this side as well, and we send out a FIN and enter the LAST_ACK
    state. We require that there is no outstanding data; otherwise the
    sequence numbers will be screwed up. */

    if(BUF(buf)->flags & TCP_FIN && !(uip_connr->tcpstateflags & UIP_STOPPED)) {
      if(uip_outstanding(uip_connr)) {
	goto drop;
      }
      uip_add_rcv_nxt(buf, 1 + uip_len(buf));
      uip_flags(buf) |= UIP_CLOSE;
      if(uip_len(buf) > 0) {
	uip_flags(buf) |= UIP_NEWDATA;
      }
      UIP_APPCALL(buf);
      uip_connr->len = 1;
      uip_connr->tcpstateflags = UIP_LAST_ACK;
      uip_connr->nrtx = 0;
    tcp_send_finack:
      BUF(buf)->flags = TCP_FIN | TCP_ACK;
      goto tcp_send_nodata;
    }

    /* Check the URG flag. If this is set, the segment carries urgent
       data that we must pass to the application. */
    if((BUF(buf)->flags & TCP_URG) != 0) {
#if UIP_URGDATA > 0
      uip_urglen = (BUF->urgp[0] << 8) | BUF->urgp[1];
      if(uip_urglen > uip_len) {
	/* There is more urgent data in the next segment to come. */
	uip_urglen = uip_len;
      }
      uip_add_rcv_nxt(uip_urglen);
      uip_len -= uip_urglen;
      uip_urgdata = uip_appdata;
      uip_appdata += uip_urglen;
    } else {
      uip_urglen = 0;
#else /* UIP_URGDATA > 0 */
      uip_appdata(buf) = ((char *)uip_appdata(buf)) + ((BUF(buf)->urgp[0] << 8) | BUF(buf)->urgp[1]);
      uip_len(buf) -= (BUF(buf)->urgp[0] << 8) | BUF(buf)->urgp[1];
#endif /* UIP_URGDATA > 0 */
    }

    /* If uip_len > 0 we have TCP data in the packet, and we flag this
       by setting the UIP_NEWDATA flag and update the sequence number
       we acknowledge. If the application has stopped the dataflow
       using uip_stop(), we must not accept any data packets from the
       remote host. */
    if(uip_len(buf) > 0 && !(uip_connr->tcpstateflags & UIP_STOPPED)) {
      uip_flags(buf) |= UIP_NEWDATA;
      uip_add_rcv_nxt(buf, uip_len(buf));
    }

    /* Check if the available buffer space advertised by the other end
       is smaller than the initial MSS for this connection. If so, we
       set the current MSS to the window size to ensure that the
       application does not send more data than the other end can
       handle.

       If the remote host advertises a zero window, we set the MSS to
       the initial MSS so that the application will send an entire MSS
       of data. This data will not be acknowledged by the receiver,
       and the application will retransmit it. This is called the
       "persistent timer" and uses the retransmission mechanim.
    */
    tmp16 = ((uint16_t)BUF(buf)->wnd[0] << 8) + (uint16_t)BUF(buf)->wnd[1];
    if(tmp16 > uip_connr->initialmss ||
       tmp16 == 0) {
      tmp16 = uip_connr->initialmss;
    }
    uip_connr->mss = tmp16;

    /* If this packet constitutes an ACK for outstanding data (flagged
       by the UIP_ACKDATA flag, we should call the application since it
       might want to send more data. If the incoming packet had data
       from the peer (as flagged by the UIP_NEWDATA flag), the
       application must also be notified.

       When the application is called, the global variable uip_len
       contains the length of the incoming data. The application can
       access the incoming data through the global pointer
       uip_appdata, which usually points UIP_IPTCPH_LEN + UIP_LLH_LEN
       bytes into the uip_buf array.

       If the application wishes to send any data, this data should be
       put into the uip_appdata and the length of the data should be
       put into uip_len. If the application don't have any data to
       send, uip_len must be set to 0. */
    if(uip_flags(buf) & (UIP_NEWDATA | UIP_ACKDATA)) {
      if(flag != UIP_TCP_SEND_CONN) {
        /* Do not reset the slen because we are being called
         * directly by the application.
         */
        uip_slen(buf) = 0;
      }

      if (uip_connr->buf) {
	  net_context_tcp_set_pending(ip_buf_context(uip_connr->buf), NULL);
          net_context_set_internal_connection(ip_buf_context(uip_connr->buf),
					      uip_connr);

	  /* At this point we have received ACK to data in uip_connr->buf */

	  /* This is not an error but tells net_core.c:net_send() that
	   * user should be able to send now more data.
	   */
	  net_context_set_connection_status(ip_buf_context(uip_connr->buf),
					    EISCONN);

	  ip_buf_unref(uip_connr->buf);
	  uip_connr->buf = NULL;

	  tcp_cancel_retrans_timer(uip_connr);

	} else {
	  /* We have no pending data so this will cause ACK to be sent to
	   * peer in few lines below.
	   */
	  uip_flags(buf) |= UIP_NEWDATA;
	}

      UIP_APPCALL(buf);

    appsend:

      if(uip_flags(buf) & UIP_ABORT) {
	uip_slen(buf) = 0;
	uip_connr->tcpstateflags = UIP_CLOSED;
	BUF(buf)->flags = TCP_RST | TCP_ACK;
	goto tcp_send_nodata;
      }

      if(uip_flags(buf) & UIP_CLOSE) {
	uip_slen(buf) = 0;
	uip_connr->len = 1;
	uip_connr->tcpstateflags = UIP_FIN_WAIT_1;
	uip_connr->nrtx = 0;
	BUF(buf)->flags = TCP_FIN | TCP_ACK;
	goto tcp_send_nodata;
      }

      /* If uip_slen > 0, the application has data to be sent. */
      if(uip_slen(buf) > 0) {

	/* If the connection has acknowledged data, the contents of
	   the ->len variable should be discarded. */
	if((uip_flags(buf) & UIP_ACKDATA) != 0) {
	  uip_connr->len = 0;
	}

	/* If the ->len variable is non-zero the connection has
	   already data in transit and cannot send anymore right
	   now. */
	if(uip_connr->len == 0) {

	  /* The application cannot send more than what is allowed by
	     the mss (the minumum of the MSS and the available
	     window). */
	  if(uip_slen(buf) > uip_connr->mss) {
	    uip_slen(buf) = uip_connr->mss;
	  }

	  /* Remember how much data we send out now so that we know
	     when everything has been acknowledged. */
	  uip_connr->len = uip_slen(buf);

	  PRINTF("Setting connection %p to pending length %d\n",
		 uip_connr, uip_connr->len);

	  if (uip_connr->buf) {
	    if (uip_connr->buf != buf) {
	      PRINTF("Data packet %p already pending....\n",
		     uip_connr->buf);
	    }
	  } else {
	    uip_connr->buf = ip_buf_ref(buf);
	  }

	} else {

	  /* If the application already had unacknowledged data, we
	     make sure that the application does not send (i.e.,
	     retransmit) out more than it previously sent out. */
	  uip_slen(buf) = uip_connr->len;
	}
      }
      uip_connr->nrtx = 0;
    apprexmit:
      uip_appdata(buf) = uip_sappdata(buf);

      /* If the application has data to be sent, or if the incoming
         packet had new data in it, we must send out a packet. */
      if(uip_slen(buf) > 0 && uip_connr->len > 0) {
	/* Add the length of the IP and TCP headers. */
	uip_len(buf) = uip_connr->len + UIP_TCPIP_HLEN;
	/* We always set the ACK flag in response packets. */
	BUF(buf)->flags = TCP_ACK | TCP_PSH;
	/* Send the packet. */
	goto tcp_send_noopts;
      }
      /* If there is no data to send, just send out a pure ACK if
	 there is newdata. */
      if(uip_flags(buf) & UIP_NEWDATA) {
	uip_len(buf) = UIP_TCPIP_HLEN;
	BUF(buf)->flags = TCP_ACK;
	goto tcp_send_noopts;
      }
    }
    goto drop;
  case UIP_LAST_ACK:
    /* We can close this connection if the peer has acknowledged our
       FIN. This is indicated by the UIP_ACKDATA flag. */
    if(uip_flags(buf) & UIP_ACKDATA) {
      uip_connr->tcpstateflags = UIP_CLOSED;
      uip_flags(buf) = UIP_CLOSE;
      UIP_APPCALL(buf);
    }
    break;

  case UIP_FIN_WAIT_1:
    /* The application has closed the connection, but the remote host
       hasn't closed its end yet. Thus we do nothing but wait for a
       FIN from the other side. */
    if(uip_len(buf) > 0) {
      uip_add_rcv_nxt(buf, uip_len(buf));
    }
    if(BUF(buf)->flags & TCP_FIN) {
      if(uip_flags(buf) & UIP_ACKDATA) {
	uip_connr->tcpstateflags = UIP_TIME_WAIT;
	uip_connr->timer = 0;
	uip_connr->len = 0;
      } else {
	uip_connr->tcpstateflags = UIP_CLOSING;
      }
      uip_add_rcv_nxt(buf, 1);
      uip_flags(buf) = UIP_CLOSE;
      UIP_APPCALL(buf);
      goto tcp_send_ack;
    } else if(uip_flags(buf) & UIP_ACKDATA) {
      uip_connr->tcpstateflags = UIP_FIN_WAIT_2;
      uip_connr->len = 0;
      goto drop;
    }
    if(uip_len(buf) > 0) {
      goto tcp_send_ack;
    }
    goto drop;

  case UIP_FIN_WAIT_2:
    if(uip_len(buf) > 0) {
      uip_add_rcv_nxt(buf, uip_len(buf));
    }
    if(BUF(buf)->flags & TCP_FIN) {
      uip_connr->tcpstateflags = UIP_TIME_WAIT;
      uip_connr->timer = 0;
      uip_add_rcv_nxt(buf, 1);
      uip_flags(buf) = UIP_CLOSE;
      UIP_APPCALL(buf);
      goto tcp_send_ack;
    }
    if(uip_len(buf) > 0) {
      goto tcp_send_ack;
    }
    goto drop;

  case UIP_TIME_WAIT:
    goto tcp_send_ack;

  case UIP_CLOSING:
    if(uip_flags(buf) & UIP_ACKDATA) {
      uip_connr->tcpstateflags = UIP_TIME_WAIT;
      uip_connr->timer = 0;
    }
  }
  goto drop;

  /* We jump here when we are ready to send the packet, and just want
     to set the appropriate TCP sequence numbers in the TCP header. */
 tcp_send_ack:
  PRINTF("In tcp_send_ack\n");
  BUF(buf)->flags = TCP_ACK;

 tcp_send_nodata:
  if (flag != UIP_TCP_SEND_CONN) {
    PRINTF("In tcp_send_nodata\n");
    uip_len(buf) = UIP_IPTCPH_LEN;
    buf->len = UIP_IPTCPH_LEN;
  }

 tcp_send_noopts:
  BUF(buf)->tcpoffset = (UIP_TCPH_LEN / 4) << 4;

  /* We're done with the input processing. We are now ready to send a
     reply. Our job is to fill in all the fields of the TCP and IP
     headers before calculating the checksum and finally send the
     packet. */
 tcp_send:
  PRINTF("In tcp_send\n");

  BUF(buf)->ackno[0] = uip_connr->rcv_nxt[0];
  BUF(buf)->ackno[1] = uip_connr->rcv_nxt[1];
  BUF(buf)->ackno[2] = uip_connr->rcv_nxt[2];
  BUF(buf)->ackno[3] = uip_connr->rcv_nxt[3];

  BUF(buf)->seqno[0] = uip_connr->snd_nxt[0];
  BUF(buf)->seqno[1] = uip_connr->snd_nxt[1];
  BUF(buf)->seqno[2] = uip_connr->snd_nxt[2];
  BUF(buf)->seqno[3] = uip_connr->snd_nxt[3];

  BUF(buf)->srcport  = uip_connr->lport;
  BUF(buf)->destport = uip_connr->rport;

  uip_ipaddr_copy(&BUF(buf)->srcipaddr, &uip_hostaddr);
  uip_ipaddr_copy(&BUF(buf)->destipaddr, &uip_connr->ripaddr);
  PRINTF("Sending TCP packet to ");
  PRINT6ADDR(&BUF(buf)->destipaddr);
  PRINTF(" from ");
  PRINT6ADDR(&BUF(buf)->srcipaddr);
  PRINTF("\n");

  if(uip_connr->tcpstateflags & UIP_STOPPED) {
    /* If the connection has issued uip_stop(), we advertise a zero
       window so that the remote host will stop sending data. */
    BUF(buf)->wnd[0] = BUF(buf)->wnd[1] = 0;
  } else {
    BUF(buf)->wnd[0] = ((UIP_RECEIVE_WINDOW) >> 8);
    BUF(buf)->wnd[1] = ((UIP_RECEIVE_WINDOW) & 0xff);
  }

 tcp_send_noconn:
  BUF(buf)->proto = UIP_PROTO_TCP;

  BUF(buf)->ttl = UIP_TTL;
#if NETSTACK_CONF_WITH_IPV6
  /* For IPv6, the IP length field does not include the IPv6 IP header
     length. */
  BUF->len[0] = ((uip_len - UIP_IPH_LEN) >> 8);
  BUF->len[1] = ((uip_len - UIP_IPH_LEN) & 0xff);
#else /* NETSTACK_CONF_WITH_IPV6 */
  BUF(buf)->len[0] = (uip_len(buf) >> 8);
  BUF(buf)->len[1] = (uip_len(buf) & 0xff);
#endif /* NETSTACK_CONF_WITH_IPV6 */

  BUF(buf)->urgp[0] = BUF(buf)->urgp[1] = 0;

  /* Calculate TCP checksum. */
  BUF(buf)->tcpchksum = 0;
  BUF(buf)->tcpchksum = ~(uip_tcpchksum(buf));
#endif

 ip_send_nolen:
#if NETSTACK_CONF_WITH_IPV6
  BUF->vtc = 0x60;
  BUF->tcflow = 0x00;
  BUF->flow = 0x00;
#else /* NETSTACK_CONF_WITH_IPV6 */
  BUF(buf)->vhl = 0x45;
  BUF(buf)->tos = 0;
  BUF(buf)->ipoffset[0] = BUF(buf)->ipoffset[1] = 0;
  ++ipid;
  BUF(buf)->ipid[0] = ipid >> 8;
  BUF(buf)->ipid[1] = ipid & 0xff;
  /* Calculate IP checksum. */
  BUF(buf)->ipchksum = 0;
  BUF(buf)->ipchksum = ~(uip_ipchksum(buf));
  PRINTF("uip ip_send_nolen: chkecum 0x%04x\n", uip_ipchksum(buf));
#endif /* NETSTACK_CONF_WITH_IPV6 */
#if UIP_TCP
  UIP_STAT(++uip_stat.tcp.sent);
#endif
#if NETSTACK_CONF_WITH_IPV6
 send:
#endif /* NETSTACK_CONF_WITH_IPV6 */
  PRINTF("Sending packet with length %d (%d)\n", uip_len(buf),
	       (BUF(buf)->len[0] << 8) | BUF(buf)->len[1]);

  UIP_STAT(++uip_stat.ip.sent);
  /* Return and let the caller do the actual transmission. */
  uip_flags(buf) = 0;
  return 1;

 drop:
  uip_len(buf) = 0;
  uip_flags(buf) = 0;

#if UIP_TCP
  /* Clear any pending packet */
  if (uip_connr && uip_connr->buf) {
    tcp_cancel_retrans_timer(uip_connr);
    switch (uip_connr->tcpstateflags & UIP_TS_MASK) {
    case UIP_FIN_WAIT_1:
    case UIP_FIN_WAIT_2:
    case UIP_CLOSING:
    case UIP_TIME_WAIT:
      ip_buf_unref(uip_connr->buf);
      uip_connr->buf = NULL;
    }
  }
#endif

  return 0;
}
/*---------------------------------------------------------------------------*/
uint16_t
uip_htons(uint16_t val)
{
  return UIP_HTONS(val);
}

uint32_t
uip_htonl(uint32_t val)
{
  return UIP_HTONL(val);
}
/*---------------------------------------------------------------------------*/
#if UIP_TCP
void
uip_send(struct net_buf *buf, const void *data, int len)
{
  int copylen;
#define MIN(a,b) ((a) < (b)? (a): (b))

  uip_sappdata(buf) = ip_buf_appdata(buf);

  if(uip_sappdata(buf) != NULL) {
    copylen = MIN(len, UIP_BUFSIZE - UIP_LLH_LEN - UIP_TCPIP_HLEN -
		  (int)((char *)uip_sappdata(buf) -
			(char *)&uip_buf(buf)[UIP_LLH_LEN + UIP_TCPIP_HLEN]));
  } else {
    copylen = MIN(len, UIP_BUFSIZE - UIP_LLH_LEN - UIP_TCPIP_HLEN);
  }
  if(copylen > 0) {
    uip_slen(buf) = copylen;
    if(data != uip_sappdata(buf)) {
      if(uip_sappdata(buf) == NULL) {
        memmove((char *)&uip_buf(buf)[UIP_LLH_LEN + UIP_TCPIP_HLEN],
               (data), uip_slen(buf));
      } else {
        memmove(uip_sappdata(buf), (data), uip_slen(buf));
      }
    }
    if (uip_process(&buf, UIP_TCP_SEND_CONN)) {
       int ret = tcpip_output(buf, NULL);
       if (!ret) {
         PRINTF("Packet %p sending failed.\n", buf);
	 ip_buf_sent_status(buf) = -EAGAIN;
       }
    }
  }
}
#endif

#if UIP_UDP
void
uip_send_udp(struct net_buf *buf, const void *data, int len)
{
  uip_slen(buf) = len;

  if (uip_process(&buf, UIP_UDP_SEND_CONN)) {
      int ret = tcpip_output(buf, NULL);
      if (!ret) {
         PRINTF("Packet %p sending failed.\n", buf);
         ip_buf_unref(buf);
      }
   }
}
#endif
/*---------------------------------------------------------------------------*/
/** @}*/
