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
 * \addtogroup uip6
 * @{
 */

/**
 * \file
 *    The uIP TCP/IPv6 stack code.
 *
 * \author Adam Dunkels <adam@sics.se>
 * \author Julien Abeille <jabeille@cisco.com> (IPv6 related code)
 * \author Mathilde Durvy <mdurvy@cisco.com> (IPv6 related code)
 */

/*
 * uIP is a small implementation of the IP, UDP and TCP protocols (as
 * well as some basic ICMP stuff). The implementation couples the IP,
 * UDP, TCP and the application layers very tightly. To keep the size
 * of the compiled code down, this code frequently uses the goto
 * statement. While it would be possible to break the uip_process()
 * function into many smaller functions, this would increase the code
 * size because of the overhead of parameter passing and the fact that
 * the optimizer would not be as efficient.
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
 * If the application wishes to send data to the peer, it should put
 * its data into the uip_buf. The uip_appdata pointer points to the
 * first available byte. The TCP/IP stack will calculate the
 * checksums, and fill in the necessary header fields and finally send
 * the packet back to the peer.
 */

#include <net/ip_buf.h>
#include <net/net_ip.h>
#include <errno.h>

#include "contiki/ip/uip.h"
#include "contiki/ip/uipopt.h"
#include "contiki/ipv6/uip-icmp6.h"
#include "contiki/ipv6/uip-nd6.h"
#include "contiki/ipv6/uip-ds6.h"
#include "contiki/ipv6/multicast/uip-mcast6.h"

#include <string.h>

/* Need to set the connection status in context when TCP connection is
 * established.
 */
extern void net_context_set_connection_status(struct net_context *context,
					      int status);
void *net_context_get_internal_connection(struct net_context *context);
void net_context_set_internal_connection(struct net_context *context,
					 void *conn);
struct net_context *net_context_find_internal_connection(void *conn);
void net_context_tcp_set_pending(struct net_context *context,
				 struct net_buf *buf);

/*---------------------------------------------------------------------------*/
/* For Debug, logging, statistics                                            */
/*---------------------------------------------------------------------------*/

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_IPV6
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#if UIP_CONF_IPV6_RPL
#include "rpl/rpl.h"
#endif /* UIP_CONF_IPV6_RPL */

#if UIP_LOGGING == 1
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif /* UIP_LOGGING == 1 */

#if UIP_STATISTICS == 1
struct uip_stats uip_stat;
#endif /* UIP_STATISTICS == 1 */
 

/*---------------------------------------------------------------------------*/
/**
 * \name Layer 2 variables
 * @{
 */
/*---------------------------------------------------------------------------*/
/** Host L2 address */
#if UIP_CONF_LL_802154
uip_lladdr_t uip_lladdr;
#else /*UIP_CONF_LL_802154*/
uip_lladdr_t uip_lladdr = {{0x00,0x06,0x98,0x00,0x02,0x32}};
#endif /*UIP_CONF_LL_802154*/
/** @} */

/*---------------------------------------------------------------------------*/
/**
 * \name Layer 3 variables
 * @{
 */
/*---------------------------------------------------------------------------*/
/**
 * \brief Type of the next header in IPv6 header or extension headers
 *
 * Can be the next header field in the IPv6 header or in an extension header.
 * When doing fragment reassembly, we must change the value of the next header
 * field in the header before the fragmentation header, hence we need a pointer
 * to this field.
 */
#if 0
/* Global variables cannot be used, these are moved to net_buf */
uint8_t *uip_next_hdr;
/** \brief bitmap we use to record which IPv6 headers we have already seen */
uint8_t uip_ext_bitmap = 0;
/**
 * \brief length of the extension headers read. updated each time we process
 * a header
 */
uint8_t uip_ext_len = 0;
/** \brief length of the header options read */
uint8_t uip_ext_opt_offset = 0;
#endif
/** @} */

/*---------------------------------------------------------------------------*/
/* Buffers                                                                   */
/*---------------------------------------------------------------------------*/
/**
 * \name Buffer defines
 * @{
 */
#define FBUF(buf)                             ((struct uip_tcpip_hdr *)&uip_reassbuf(buf)[0])
#define UIP_IP_BUF(buf)                          ((struct uip_ip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])
#define UIP_ICMP_BUF(buf)                      ((struct uip_icmp_hdr *)&uip_buf(buf)[uip_l2_l3_hdr_len(buf)])
#define UIP_UDP_BUF(buf)                        ((struct uip_udp_hdr *)&uip_buf(buf)[UIP_LLH_LEN + UIP_IPH_LEN])
#define UIP_TCP_BUF(buf)                        ((struct uip_tcp_hdr *)&uip_buf(buf)[UIP_LLH_LEN + UIP_IPH_LEN])
#define UIP_EXT_BUF(buf)                        ((struct uip_ext_hdr *)&uip_buf(buf)[uip_l2_l3_hdr_len(buf)])
#define UIP_ROUTING_BUF(buf)                ((struct uip_routing_hdr *)&uip_buf(buf)[uip_l2_l3_hdr_len(buf)])
#define UIP_FRAG_BUF(buf)                      ((struct uip_frag_hdr *)&uip_buf(buf)[uip_l2_l3_hdr_len(buf)])
#define UIP_HBHO_BUF(buf)                      ((struct uip_hbho_hdr *)&uip_buf(buf)[uip_l2_l3_hdr_len(buf)])
#define UIP_DESTO_BUF(buf)                    ((struct uip_desto_hdr *)&uip_buf(buf)[uip_l2_l3_hdr_len(buf)])
#define UIP_EXT_HDR_OPT_BUF(buf)            ((struct uip_ext_hdr_opt *)&uip_buf(buf)[uip_l2_l3_hdr_len(buf) + uip_ext_opt_offset(buf)])
#define UIP_EXT_HDR_OPT_PADN_BUF(buf)  ((struct uip_ext_hdr_opt_padn *)&uip_buf(buf)[uip_l2_l3_hdr_len(buf) + uip_ext_opt_offset(buf)])
#if UIP_CONF_IPV6_RPL
#define UIP_EXT_HDR_OPT_RPL_BUF(buf)    ((struct uip_ext_hdr_opt_rpl *)&uip_buf(buf)[uip_l2_l3_hdr_len(buf) + uip_ext_opt_offset(buf)])
#endif /* UIP_CONF_IPV6_RPL */
#define UIP_ICMP6_ERROR_BUF(buf)            ((struct uip_icmp6_error *)&uip_buf(buf)[uip_l2_l3_icmp_hdr_len(buf)])
/** @} */
/**
 * \name Buffer variables
 * @{
 */
/** Packet buffer for incoming and outgoing packets */
#if 0
/* Global variables cannot be used, these are now in net_buf */
#ifndef UIP_CONF_EXTERNAL_BUFFER
uip_buf_t uip_aligned_buf;
#endif /* UIP_CONF_EXTERNAL_BUFFER */

/* The uip_appdata pointer points to application data. */
void *uip_appdata;
/* The uip_appdata pointer points to the application data which is to be sent*/
void *uip_sappdata;
#endif

#if UIP_URGDATA > 0
/* The uip_urgdata pointer points to urgent data (out-of-band data), if present */
void *uip_urgdata;
uint16_t uip_urglen, uip_surglen;
#endif /* UIP_URGDATA > 0 */

/* The uip_len is either 8 or 16 bits, depending on the maximum packet size.*/
/* uint16_t uip_len, uip_slen; No longer used as len is part of net_buf */
/** @} */

/*---------------------------------------------------------------------------*/
/**
 * \name General variables
 * @{
 */
/*---------------------------------------------------------------------------*/

#if 0
/* Global variables cannot be used, these are now in net_buf */

/* The uip_flags variable is used for communication between the TCP/IP stack
and the application program. */
uint8_t uip_flags;

/* uip_conn always points to the current connection (set to NULL for UDP). */
struct uip_conn *uip_conn;
#endif

#if UIP_ACTIVE_OPEN || UIP_UDP
/* Keeps track of the last port used for a new connection. */
static uint16_t lastport;
#endif /* UIP_ACTIVE_OPEN || UIP_UDP */
/** @} */

/*---------------------------------------------------------------------------*/
/* TCP                                                                       */
/*---------------------------------------------------------------------------*/
/**
 * \name TCP defines
 *@{
 */
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
/** @} */
/**
 * \name TCP variables
 *@{
 */
#if UIP_TCP
/* The uip_conns array holds all TCP connections. */
struct uip_conn uip_conns[UIP_CONNS];

/* The uip_listenports list all currently listning ports. */
uint16_t uip_listenports[UIP_LISTENPORTS];

/* The iss variable is used for the TCP initial sequence number. */
static uint8_t iss[4];

/* Temporary variables. */
uint8_t uip_acc32[4];
static uint8_t opt;
static uint16_t tmp16;
#endif /* UIP_TCP */
/** @} */

/*---------------------------------------------------------------------------*/
/**
 * \name UDP variables
 * @{
 */
/*---------------------------------------------------------------------------*/
#if UIP_UDP
#if 0
/* Moved to net_buf */
struct uip_udp_conn *uip_udp_conn;
#endif
struct uip_udp_conn uip_udp_conns[UIP_UDP_CONNS];
#endif /* UIP_UDP */
/** @} */

/*---------------------------------------------------------------------------*/
/**
 * \name ICMPv6 variables
 * @{
 */
/*---------------------------------------------------------------------------*/
#if UIP_CONF_ICMP6
/** single possible icmpv6 "connection" */
struct uip_icmp6_conn uip_icmp6_conns;
#endif /*UIP_CONF_ICMP6*/
/** @} */

/*---------------------------------------------------------------------------*/
/* Functions                                                                 */
/*---------------------------------------------------------------------------*/
#if (!UIP_ARCH_ADD32 && UIP_TCP)
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

#endif /* UIP_ARCH_ADD32 && UIP_TCP */

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
  
  while(dataptr < last_byte) {   /* At least two more bytes */
    t = (dataptr[0] << 8) + dataptr[1];
    sum += t;
    if(sum < t) {
      sum++;      /* carry */
    }
    dataptr += 2;
  }
  
  if(dataptr == last_byte) {
    t = (dataptr[0] << 8) + 0;
    sum += t;
    if(sum < t) {
      sum++;      /* carry */
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
  PRINTF("uip_ipchksum: sum 0x%04x\n", sum);
  return (sum == 0) ? 0xffff : uip_htons(sum);
}
#endif
/*---------------------------------------------------------------------------*/
static uint16_t
upper_layer_chksum(struct net_buf *buf, uint8_t proto)
{
/* gcc 4.4.0 - 4.6.1 (maybe 4.3...) with -Os on 8 bit CPUS incorrectly compiles:
 * int bar (int);
 * int foo (unsigned char a, unsigned char b) {
 *   int len = (a << 8) + b; //len becomes 0xff00&<random>+b
 *   return len + bar (len);
 * }
 * upper_layer_len triggers this bug unless it is declared volatile.
 * See https://sourceforge.net/apps/mantisbt/contiki/view.php?id=3
 */
  volatile uint16_t upper_layer_len;
  uint16_t sum;
  
  upper_layer_len = (((uint16_t)(UIP_IP_BUF(buf)->len[0]) << 8) + UIP_IP_BUF(buf)->len[1] - uip_ext_len(buf));
  
  PRINTF("Upper layer checksum len: %d from: %d\n", upper_layer_len,
	 UIP_IPH_LEN + UIP_LLH_LEN + uip_ext_len(buf));

  /* First sum pseudoheader. */
  /* IP protocol and length fields. This addition cannot carry. */
  sum = upper_layer_len + proto;
  /* Sum IP source and destination addresses. */
  sum = chksum(sum, (uint8_t *)&UIP_IP_BUF(buf)->srcipaddr, 2 * sizeof(uip_ipaddr_t));

  /* Sum TCP header and data. */
  sum = chksum(sum, &uip_buf(buf)[UIP_IPH_LEN + UIP_LLH_LEN + uip_ext_len(buf)],
               upper_layer_len);
    
  return (sum == 0) ? 0xffff : uip_htons(sum);
}
/*---------------------------------------------------------------------------*/
uint16_t
uip_icmp6chksum(struct net_buf *buf)
{
  return upper_layer_chksum(buf, UIP_PROTO_ICMP6);
  
}
/*---------------------------------------------------------------------------*/
#if UIP_TCP
uint16_t
uip_tcpchksum(struct net_buf *buf)
{
  return upper_layer_chksum(buf, UIP_PROTO_TCP);
}
#endif /* UIP_TCP */
/*---------------------------------------------------------------------------*/
#if UIP_UDP && UIP_UDP_CHECKSUMS
uint16_t
uip_udpchksum(struct net_buf *buf)
{
  return upper_layer_chksum(buf, UIP_PROTO_UDP);
}
#endif /* UIP_UDP && UIP_UDP_CHECKSUMS */
#endif /* UIP_ARCH_CHKSUM */
/*---------------------------------------------------------------------------*/
void
uip_init(void)
{
#if UIP_TCP
  uint8_t c;
#endif

  uip_ds6_init();
  uip_icmp6_init();
  uip_nd6_init();

#if UIP_TCP
  for(c = 0; c < UIP_LISTENPORTS; ++c) {
    uip_listenports[c] = 0;
  }
  for(c = 0; c < UIP_CONNS; ++c) {
    uip_conns[c].tcpstateflags = UIP_CLOSED;
    uip_conns[c].len = 0;
  }

  {
    /* Randomise initial seq number */
    uint32_t c = clock_get_cycle();
    iss[0] = c & 0xff;
    iss[1] = (c & 0xff00) >> 8;
    iss[2] = (c & 0xff0000) >> 16;
    iss[3] = (c & 0xff000000) >> 24;
  }
#endif /* UIP_TCP */

#if UIP_ACTIVE_OPEN || UIP_UDP
  lastport = 1024;
#endif /* UIP_ACTIVE_OPEN || UIP_UDP */

#if UIP_UDP
  memset(&uip_udp_conns, 0, sizeof(uip_udp_conns));
#endif /* UIP_UDP */

#if UIP_CONF_IPV6_MULTICAST
  UIP_MCAST6.init();
#endif
}
/*---------------------------------------------------------------------------*/
#if UIP_TCP && UIP_ACTIVE_OPEN
struct uip_conn *
uip_connect(const uip_ipaddr_t *ripaddr, uint16_t rport)
{
  register struct uip_conn *conn, *cconn;
  uint8_t c;

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

  conn->rcv_nxt[0] = 0;
  conn->rcv_nxt[1] = 0;
  conn->rcv_nxt[2] = 0;
  conn->rcv_nxt[3] = 0;

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
#endif /* UIP_TCP && UIP_ACTIVE_OPEN */
/*---------------------------------------------------------------------------*/
void
remove_ext_hdr(struct net_buf *buf)
{
  /* Remove ext header before TCP/UDP processing. */
  if(uip_ext_len(buf) > 0) {
    PRINTF("Cutting ext-header before processing (extlen: %d, uiplen: %d)\n",
	   uip_ext_len(buf), uip_len(buf));
    if(uip_len(buf) < UIP_IPH_LEN + uip_ext_len(buf)) {
      PRINTF("ERROR: uip_len too short compared to ext len\n");
      uip_ext_len(buf) = 0;
      uip_len(buf) = 0;
      return;
    }
    memmove(((uint8_t *)UIP_TCP_BUF(buf)),
	    (uint8_t *)UIP_TCP_BUF(buf) + uip_ext_len(buf),
	    uip_len(buf) - UIP_IPH_LEN - uip_ext_len(buf));

    uip_len(buf) -= uip_ext_len(buf);
    ip_buf_len(buf) = uip_len(buf);

    /* Update the IP length. */
    UIP_IP_BUF(buf)->len[0] = (uip_len(buf) - UIP_IPH_LEN) >> 8;
    UIP_IP_BUF(buf)->len[1] = (uip_len(buf) - UIP_IPH_LEN) & 0xff;
    uip_ext_len(buf) = 0;
  }
}
/*---------------------------------------------------------------------------*/
#if UIP_UDP
struct uip_udp_conn *
uip_udp_new(const uip_ipaddr_t *ripaddr, uint16_t rport)
{
  register struct uip_udp_conn *conn;
  uint8_t c;

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
  conn->ttl = uip_ds6_if.cur_hop_limit;
  
  return conn;
}
#endif /* UIP_UDP */
/*---------------------------------------------------------------------------*/
#if UIP_TCP
void
uip_unlisten(uint16_t port)
{
  uint8_t c;
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
  uint8_t c;
  for(c = 0; c < UIP_LISTENPORTS; ++c) {
    if(uip_listenports[c] == 0) {
      uip_listenports[c] = port;
      return;
    }
  }
}
#endif
/*---------------------------------------------------------------------------*/

#if UIP_CONF_IPV6_REASSEMBLY
#define UIP_REASS_BUFSIZE (UIP_BUFSIZE - UIP_LLH_LEN)

static uint8_t uip_reassbuf[UIP_REASS_BUFSIZE];

static uint8_t uip_reassbitmap[UIP_REASS_BUFSIZE / (8 * 8)];
/*the first byte of an IP fragment is aligned on an 8-byte boundary */

static const uint8_t bitmap_bits[8] = {0xff, 0x7f, 0x3f, 0x1f,
                                    0x0f, 0x07, 0x03, 0x01};
static uint16_t uip_reasslen;
static uint8_t uip_reassflags;

#define UIP_REASS_FLAG_LASTFRAG 0x01
#define UIP_REASS_FLAG_FIRSTFRAG 0x02
#define UIP_REASS_FLAG_ERROR_MSG 0x04


/*
 * See RFC 2460 for a description of fragmentation in IPv6
 * A typical Ipv6 fragment
 *  +------------------+--------+--------------+
 *  |  Unfragmentable  |Fragment|    first     |
 *  |       Part       | Header |   fragment   |
 *  +------------------+--------+--------------+
 */


struct etimer uip_reass_timer; /* timer for reassembly */
uint8_t uip_reass_on; /* equal to 1 if we are currently reassembling a packet */

static uint32_t uip_id; /* For every packet that is to be fragmented, the source
                        node generates an Identification value that is present
                        in all the fragments */
#define IP_MF   0x0001

static uint16_t
uip_reass(void)
{
  uint16_t offset=0;
  uint16_t len;
  uint16_t i;
  
  /* If ip_reasstmr is zero, no packet is present in the buffer */
  /* We first write the unfragmentable part of IP header into the reassembly
     buffer. The reset the other reassembly variables. */
  if(uip_reass_on == 0) {
    PRINTF("Starting reassembly\n");
    memcpy(FBUF, UIP_IP_BUF(buf), uip_ext_len(buf) + UIP_IPH_LEN);
    /* temporary in case we do not receive the fragment with offset 0 first */
    etimer_set(&uip_reass_timer, UIP_REASS_MAXAGE*CLOCK_SECOND, &tcpip_process);
    uip_reass_on = 1;
    uip_reassflags = 0;
    uip_id = UIP_FRAG_BUF->id;
    /* Clear the bitmap. */
    memset(uip_reassbitmap, 0, sizeof(uip_reassbitmap));
  }
  /*
   * Check if the incoming fragment matches the one currently present
   * in the reasembly buffer. If so, we proceed with copying the fragment
   * into the buffer.
   */
  if(uip_ipaddr_cmp(&FBUF->srcipaddr, &UIP_IP_BUF(buf)->srcipaddr) &&
     uip_ipaddr_cmp(&FBUF->destipaddr, &UIP_IP_BUF(buf)->destipaddr) &&
     UIP_FRAG_BUF->id == uip_id) {
    len = uip_len(buf) - uip_ext_len(buf) - UIP_IPH_LEN - UIP_FRAGH_LEN;
    offset = (uip_ntohs(UIP_FRAG_BUF->offsetresmore) & 0xfff8);
    /* in byte, originaly in multiple of 8 bytes*/
    PRINTF("len %d\n", len);
    PRINTF("offset %d\n", offset);
    if(offset == 0){
      uip_reassflags |= UIP_REASS_FLAG_FIRSTFRAG;
      /*
       * The Next Header field of the last header of the Unfragmentable
       * Part is obtained from the Next Header field of the first
       * fragment's Fragment header.
       */
      *uip_next_hdr = UIP_FRAG_BUF->next;
      memcpy(FBUF, UIP_IP_BUF(buf), uip_ext_len(buf) + UIP_IPH_LEN);
      PRINTF("src ");
      PRINT6ADDR(&FBUF->srcipaddr);
      PRINTF("dest ");
      PRINT6ADDR(&FBUF->destipaddr);
      PRINTF("next %d\n", UIP_IP_BUF(buf)->proto);
      
    }
    
    /* If the offset or the offset + fragment length overflows the
       reassembly buffer, we discard the entire packet. */
    if(offset > UIP_REASS_BUFSIZE ||
       offset + len > UIP_REASS_BUFSIZE) {
      uip_reass_on = 0;
      etimer_stop(&uip_reass_timer);
      return 0;
    }

    /* If this fragment has the More Fragments flag set to zero, it is the
       last fragment*/
    if((uip_ntohs(UIP_FRAG_BUF->offsetresmore) & IP_MF) == 0) {
      uip_reassflags |= UIP_REASS_FLAG_LASTFRAG;
      /*calculate the size of the entire packet*/
      uip_reasslen = offset + len;
      PRINTF("LAST FRAGMENT reasslen %d\n", uip_reasslen);
    } else {
      /* If len is not a multiple of 8 octets and the M flag of that fragment
         is 1, then that fragment must be discarded and an ICMP Parameter
         Problem, Code 0, message should be sent to the source of the fragment,
         pointing to the Payload Length field of the fragment packet. */
      if(len % 8 != 0){
        uip_icmp6_error_output(ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER, 4);
        uip_reassflags |= UIP_REASS_FLAG_ERROR_MSG;
        /* not clear if we should interrupt reassembly, but it seems so from
           the conformance tests */
        uip_reass_on = 0;
        etimer_stop(&uip_reass_timer);
        return uip_len(buf);
      }
    }
    
    /* Copy the fragment into the reassembly buffer, at the right
       offset. */
    memcpy((uint8_t *)FBUF + UIP_IPH_LEN + uip_ext_len(buf) + offset,
           (uint8_t *)UIP_FRAG_BUF + UIP_FRAGH_LEN, len);
    
    /* Update the bitmap. */
    if(offset >> 6 == (offset + len) >> 6) {
      uip_reassbitmap[offset >> 6] |=
        bitmap_bits[(offset >> 3) & 7] &
        ~bitmap_bits[((offset + len) >> 3)  & 7];
    } else {
      /* If the two endpoints are in different bytes, we update the
         bytes in the endpoints and fill the stuff inbetween with
         0xff. */
      uip_reassbitmap[offset >> 6] |= bitmap_bits[(offset >> 3) & 7];
 
      for(i = (1 + (offset >> 6)); i < ((offset + len) >> 6); ++i) {
        uip_reassbitmap[i] = 0xff;
      }
      uip_reassbitmap[(offset + len) >> 6] |=
        ~bitmap_bits[((offset + len) >> 3) & 7];
    }
  
    /* Finally, we check if we have a full packet in the buffer. We do
       this by checking if we have the last fragment and if all bits
       in the bitmap are set. */
    
    if(uip_reassflags & UIP_REASS_FLAG_LASTFRAG) {
      /* Check all bytes up to and including all but the last byte in
         the bitmap. */
      for(i = 0; i < (uip_reasslen >> 6); ++i) {
        if(uip_reassbitmap[i] != 0xff) {
          return 0;
        }
      }
      /* Check the last byte in the bitmap. It should contain just the
         right amount of bits. */
      if(uip_reassbitmap[uip_reasslen >> 6] !=
         (uint8_t)~bitmap_bits[(uip_reasslen >> 3) & 7]) {
        return 0;
      }

     /* If we have come this far, we have a full packet in the
         buffer, so we copy it to uip_buf. We also reset the timer. */
      uip_reass_on = 0;
      etimer_stop(&uip_reass_timer);

      uip_reasslen += UIP_IPH_LEN + uip_ext_len(buf);
      memcpy(UIP_IP_BUF(buf), FBUF, uip_reasslen);
      UIP_IP_BUF(buf)->len[0] = ((uip_reasslen - UIP_IPH_LEN) >> 8);
      UIP_IP_BUF(buf)->len[1] = ((uip_reasslen - UIP_IPH_LEN) & 0xff);
      PRINTF("REASSEMBLED PAQUET %d (%d)\n", uip_reasslen,
             (UIP_IP_BUF(buf)->len[0] << 8) | UIP_IP_BUF(buf)->len[1]);
   
      return uip_reasslen;
      
    }
  } else {
    PRINTF("Already reassembling another paquet\n");
  }
  return 0;
}

void
uip_reass_over(struct net_buf *buf)
{
   /* to late, we abandon the reassembly of the packet */

  uip_reass_on = 0;
  etimer_stop(&uip_reass_timer);

  if(uip_reassflags & UIP_REASS_FLAG_FIRSTFRAG){
    PRINTF("FRAG INTERRUPTED TOO LATE\n");
    /* If the first fragment has been received, an ICMP Time Exceeded
       -- Fragment Reassembly Time Exceeded message should be sent to the
       source of that fragment. */
    /** \note
     * We don't have a complete packet to put in the error message.
     * We could include the first fragment but since its not mandated by
     * any RFC, we decided not to include it as it reduces the size of
     * the packet.
     */
    uip_len(buf) = 0;
    uip_ext_len(buf) = 0;
    memcpy(UIP_IP_BUF(buf), FBUF, UIP_IPH_LEN); /* copy the header for src
                                              and dest address*/
    uip_icmp6_error_output(ICMP6_TIME_EXCEEDED, ICMP6_TIME_EXCEED_REASSEMBLY, 0);
    
    UIP_STAT(++uip_stat.ip.sent);
    uip_flags(buf) = 0;
  }
}

#endif /* UIP_CONF_IPV6_REASSEMBLY */

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
#endif
/*---------------------------------------------------------------------------*/

/**
 * \brief Process the options in Destination and Hop By Hop extension headers
 */
static uint8_t
ext_hdr_options_process(struct net_buf *buf)
{
 /*
  * Length field in the extension header: length of the header in units of
  * 8 bytes, excluding the first 8 bytes
  * length field in an option : the length of data in the option
  */
  if (((UIP_EXT_BUF(buf)->len << 3) + 8) > buf->len) {
    PRINTF("Corrupted packet, extension header %d too long (max %d bytes)\n",
	   (UIP_EXT_BUF(buf)->len << 3) + 8, buf->len);
    return 1; /* invalid packet, drop it */
  }

  uip_ext_opt_offset(buf) = 2;
  while(uip_ext_opt_offset(buf) < ((UIP_EXT_BUF(buf)->len << 3) + 8)) {
    switch(UIP_EXT_HDR_OPT_BUF(buf)->type) {
      /*
       * for now we do not support any options except padding ones
       * PAD1 does not make sense as the header must be 8bytes aligned,
       * hence we can only have
       */
      case UIP_EXT_HDR_OPT_PAD1:
        PRINTF("Processing PAD1 option\n");
        uip_ext_opt_offset(buf) += 1;
        break;
      case UIP_EXT_HDR_OPT_PADN:
        PRINTF("Processing PADN option\n");
        uip_ext_opt_offset(buf) += UIP_EXT_HDR_OPT_PADN_BUF(buf)->opt_len + 2;
        break;
      case UIP_EXT_HDR_OPT_RPL:
		/* Fixes situation when a node that is not using RPL
		 * joins a network which does. The received packages will include the
		 * RPL header and processed by the "default" case of the switch
		 * (0x63 & 0xC0 = 0x40). Hence, the packet is discarded as the header
		 * is considered invalid.
		 * Using this fix, the header is ignored, and the next header (if
		 * present) is processed.
		 */
#if UIP_CONF_IPV6_RPL
        PRINTF("Processing RPL option\n");
        if(rpl_verify_header(buf, uip_ext_opt_offset(buf))) {
          PRINTF("RPL Option Error: Dropping Packet\n");
          return 1;
        }
#endif /* UIP_CONF_IPV6_RPL */
        uip_ext_opt_offset(buf) += (UIP_EXT_HDR_OPT_BUF(buf)->len) + 2;
        return 0;
      default:
        /*
         * check the two highest order bits of the option
         * - 00 skip over this option and continue processing the header.
         * - 01 discard the packet.
         * - 10 discard the packet and, regardless of whether or not the
         *   packet's Destination Address was a multicast address, send an
         *   ICMP Parameter Problem, Code 2, message to the packet's
         *   Source Address, pointing to the unrecognized Option Type.
         * - 11 discard the packet and, only if the packet's Destination
         *   Address was not a multicast address, send an ICMP Parameter
         *   Problem, Code 2, message to the packet's Source Address,
         *   pointing to the unrecognized Option Type.
         */
        PRINTF("MSB %x\n", UIP_EXT_HDR_OPT_BUF(buf)->type);
        switch(UIP_EXT_HDR_OPT_BUF(buf)->type & 0xC0) {
          case 0:
            break;
          case 0x40:
            return 1;
          case 0xC0:
            if(uip_is_addr_mcast(&UIP_IP_BUF(buf)->destipaddr)) {
              return 1;
            }
          case 0x80:
            uip_icmp6_error_output(buf, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_OPTION,
                             (uint32_t)UIP_IPH_LEN + uip_ext_len(buf) + uip_ext_opt_offset(buf));
            return 2;
        }
        /* in the cases were we did not discard, update ext_opt* */
        uip_ext_opt_offset(buf) += UIP_EXT_HDR_OPT_BUF(buf)->len + 2;
        break;
    }
  }
  return 0;
}

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
  uint8_t c;
#endif /* UIP_TCP */
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
      UIP_TCP_BUF(buf)->flags = 0;
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
    /* Check if we were invoked because of the perodic timer fireing. */
  if(flag == UIP_TIMER) {
    /* Reset the length variables. */
#if UIP_TCP
    uip_len(buf) = 0;
    uip_slen(buf) = 0;

    /* Increase the initial sequence number. */
    if(++iss[3] == 0) {
      if(++iss[2] == 0) {
        if(++iss[1] == 0) {
          ++iss[0];
        }
      }
    }
  tcp_retry:
    /*
     * Check if the connection is in a state in which we simply wait
     * for the connection to time out. If so, we increase the
     * connection's timer and remove the connection if it times
     * out.
     */
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

      /*
       * If the connection has outstanding data, we increase the
       * connection's timer and see if it has reached the RTO value
       * in which case we retransmit.
       */
      if(uip_outstanding(uip_connr)) {
        if(uip_connr->timer-- == 0) {
          if(uip_connr->nrtx == UIP_MAXRTX ||
             ((uip_connr->tcpstateflags == UIP_SYN_SENT ||
               uip_connr->tcpstateflags == UIP_SYN_RCVD) &&
              uip_connr->nrtx == UIP_MAXSYNRTX)) {
            uip_connr->tcpstateflags = UIP_CLOSED;
            /*
             * We call UIP_APPCALL() with uip_flags set to
             * UIP_TIMEDOUT to inform the application that the
             * connection has timed out.
             */
            uip_flags(buf) = UIP_TIMEDOUT;
            UIP_APPCALL(buf);
                  
            /* We also send a reset packet to the remote host. */
            UIP_TCP_BUF(buf)->flags = TCP_RST | TCP_ACK;
            goto tcp_send_nodata;
          }
               
          /* Exponential backoff. */
          uip_connr->timer = UIP_RTO << (uip_connr->nrtx > 4?
                                         4:
                                         uip_connr->nrtx);
          ++(uip_connr->nrtx);
               
          /*
           * Ok, so we need to retransmit. We do this differently
           * depending on which state we are in. In ESTABLISHED, we
           * call upon the application so that it may prepare the
           * data for the retransmit. In SYN_RCVD, we resend the
           * SYNACK that we sent earlier and in LAST_ACK we have to
           * retransmit our FINACK.
           */
          UIP_STAT(++uip_stat.tcp.rexmit);
          switch(uip_connr->tcpstateflags & UIP_TS_MASK) {
            case UIP_SYN_RCVD:
              /* In the SYN_RCVD state, we should retransmit our SYNACK. */
              goto tcp_send_synack;
                     
#if UIP_ACTIVE_OPEN
            case UIP_SYN_SENT:
              /* In the SYN_SENT state, we retransmit out SYN. */
              UIP_TCP_BUF(buf)->flags = 0;
              goto tcp_send_syn;
#endif /* UIP_ACTIVE_OPEN */
                     
            case UIP_ESTABLISHED:
              /*
               * In the ESTABLISHED state, we call upon the application
               * to do the actual retransmit after which we jump into
               * the code for sending out the packet (the apprexmit
               * label).
               */
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
        /*
         * If there was no need for a retransmission, we poll the
         * application for new data.
         */
        uip_flags(buf) = UIP_POLL;
        UIP_APPCALL(buf);
        goto appsend;
      }
    }
    goto drop;
#endif /* UIP_TCP */
  }
#if UIP_UDP
  if(flag == UIP_UDP_TIMER) {
    if(uip_udp_conn(buf)->lport != 0) {
      uip_set_conn(buf) = NULL;
      uip_sappdata(buf) = uip_appdata(buf) = &uip_buf(buf)[UIP_IPUDPH_LEN + UIP_LLH_LEN];
      uip_len(buf) = uip_slen(buf) = 0;
      uip_flags(buf) = UIP_POLL;
      UIP_UDP_APPCALL(buf);
      goto udp_send;
    } else {
      goto drop;
    }
  }
#endif /* UIP_UDP */

  
  /* This is where the input processing starts. */
  UIP_STAT(++uip_stat.ip.recv);
   
  /* Start of IP input header processing code. */
   
  /* Check validity of the IP header. */
  if((UIP_IP_BUF(buf)->vtc & 0xf0) != 0x60)  { /* IP version and header length. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.vhlerr);
    UIP_LOG("ipv6: invalid version.");
    goto drop;
  }
  /*
   * Check the size of the packet. If the size reported to us in
   * uip_len is smaller the size reported in the IP header, we assume
   * that the packet has been corrupted in transit. If the size of
   * uip_len is larger than the size reported in the IP packet header,
   * the packet has been padded and we set uip_len to the correct
   * value..
   */
   
  if((UIP_IP_BUF(buf)->len[0] << 8) + UIP_IP_BUF(buf)->len[1] <= uip_len(buf)) {
    uip_len(buf) = (UIP_IP_BUF(buf)->len[0] << 8) + UIP_IP_BUF(buf)->len[1] + UIP_IPH_LEN;
    /*
     * The length reported in the IPv6 header is the
     * length of the payload that follows the
     * header. However, uIP uses the uip_len variable
     * for holding the size of the entire packet,
     * including the IP header. For IPv4 this is not a
     * problem as the length field in the IPv4 header
     * contains the length of the entire packet. But
     * for IPv6 we need to add the size of the IPv6
     * header (40 bytes).
     */
  } else {
    UIP_LOG("ip: packet shorter than reported in IP header.");
    PRINTF("IPv6 packet size %d buf len %d\n",
	   (UIP_IP_BUF(buf)->len[0] << 8) + UIP_IP_BUF(buf)->len[1],
	   uip_len(buf));
    goto drop;
  }
  
  PRINTF("IPv6 packet received from ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->srcipaddr);
  PRINTF(" to ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->destipaddr);
  PRINTF("\n");

  if(uip_is_addr_mcast(&UIP_IP_BUF(buf)->srcipaddr)){
    UIP_STAT(++uip_stat.ip.drop);
    PRINTF("Dropping packet, src is mcast\n");
    goto drop;
  }

#if UIP_CONF_ROUTER
  /*
   * Next header field processing. In IPv6, we can have extension headers,
   * if present, the Hop-by-Hop Option must be processed before forwarding
   * the packet.
   */
  uip_next_hdr = &UIP_IP_BUF->proto;
  uip_ext_len(buf) = 0;
  uip_ext_bitmap = 0;
  if(*uip_next_hdr == UIP_PROTO_HBHO) {
#if UIP_CONF_IPV6_CHECKS
    uip_ext_bitmap |= UIP_EXT_HDR_BITMAP_HBHO;
#endif /* UIP_CONF_IPV6_CHECKS */
    switch(ext_hdr_options_process()) {
      case 0:
        /* continue */
        uip_next_hdr = &UIP_EXT_BUF->next;
        uip_ext_len(buf) += (UIP_EXT_BUF->len << 3) + 8;
        break;
      case 1:
	PRINTF("Dropping packet after extension header processing\n");
        /* silently discard */
        goto drop;
      case 2:
	PRINTF("Sending error message after extension header processing\n");
        /* send icmp error message (created in ext_hdr_options_process)
         * and discard*/
        goto send;
    }
  }

  /*
   * Process Packets with a routable multicast destination:
   * - We invoke the multicast engine and let it do its thing
   *   (cache, forward etc).
   * - We never execute the datagram forwarding logic in this file here. When
   *   the engine returns, forwarding has been handled if and as required.
   * - Depending on the return value, we either discard or deliver up the stack
   *
   * All multicast engines must hook in here. After this function returns, we
   * expect UIP_BUF to be unmodified
   */
#if UIP_CONF_IPV6_MULTICAST
  if(uip_is_addr_mcast_routable(&UIP_IP_BUF->destipaddr)) {
    if(UIP_MCAST6.in() == UIP_MCAST6_ACCEPT) {
      /* Deliver up the stack */
      goto process;
    } else {
      /* Don't deliver up the stack */
      goto drop;
    }
  }
#endif /* UIP_IPV6_CONF_MULTICAST */

  /* TBD Some Parameter problem messages */
  if(!uip_ds6_is_my_addr(&UIP_IP_BUF->destipaddr) &&
     !uip_ds6_is_my_maddr(&UIP_IP_BUF->destipaddr)) {
    if(!uip_is_addr_mcast(&UIP_IP_BUF->destipaddr) &&
       !uip_is_addr_link_local(&UIP_IP_BUF->destipaddr) &&
       !uip_is_addr_link_local(&UIP_IP_BUF->srcipaddr) &&
       !uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr) &&
       !uip_is_addr_loopback(&UIP_IP_BUF->destipaddr)) {


      /* Check MTU */
      if(uip_len > UIP_LINK_MTU) {
        uip_icmp6_error_output(ICMP6_PACKET_TOO_BIG, 0, UIP_LINK_MTU);
        UIP_STAT(++uip_stat.ip.drop);
        goto send;
      }
      /* Check Hop Limit */
      if(UIP_IP_BUF->ttl <= 1) {
        uip_icmp6_error_output(ICMP6_TIME_EXCEEDED,
                               ICMP6_TIME_EXCEED_TRANSIT, 0);
        UIP_STAT(++uip_stat.ip.drop);
        goto send;
      }

#if UIP_CONF_IPV6_RPL
      if(rpl_update_header_empty()) {
        /* Packet can not be forwarded */
        PRINTF("RPL Forward Option Error\n");
        goto drop;
      }
#endif /* UIP_CONF_IPV6_RPL */

      UIP_IP_BUF->ttl = UIP_IP_BUF->ttl - 1;
      PRINTF("Forwarding packet to ");
      PRINT6ADDR(&UIP_IP_BUF->destipaddr);
      PRINTF("\n");
      UIP_STAT(++uip_stat.ip.forwarded);
      goto send;
    } else {
      if((uip_is_addr_link_local(&UIP_IP_BUF->srcipaddr)) &&
         (!uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr)) &&
         (!uip_is_addr_loopback(&UIP_IP_BUF->destipaddr)) &&
         (!uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)) &&
         (!uip_ds6_is_addr_onlink((&UIP_IP_BUF->destipaddr)))) {
        PRINTF("LL source address with off link destination, dropping\n");
        uip_icmp6_error_output(ICMP6_DST_UNREACH,
                               ICMP6_DST_UNREACH_NOTNEIGHBOR, 0);
        goto send;
      }
      PRINTF("Dropping packet, not for me and link local or multicast\n");
      UIP_STAT(++uip_stat.ip.drop);
      goto drop;
    }
  }
#else /* UIP_CONF_ROUTER */
  if(!uip_ds6_is_my_addr(&UIP_IP_BUF(buf)->destipaddr) &&
     !uip_ds6_is_my_maddr(&UIP_IP_BUF(buf)->destipaddr) &&
     !uip_is_addr_mcast(&UIP_IP_BUF(buf)->destipaddr) &&
     !uip_is_addr_loopback(&UIP_IP_BUF(buf)->destipaddr)) {
    PRINTF("Dropping packet, not for me\n");
    UIP_STAT(++uip_stat.ip.drop);
    goto drop;
  }

  /*
   * Next header field processing. In IPv6, we can have extension headers,
   * they are processed here
   */
  uip_next_hdr(buf) = &UIP_IP_BUF(buf)->proto;
  uip_ext_len(buf) = 0;
  uip_ext_bitmap(buf) = 0;
#endif /* UIP_CONF_ROUTER */

#if UIP_CONF_ROUTER
#if UIP_CONF_IPV6_MULTICAST
  process:
#endif
#endif

  while(1) {
    switch(*(uip_next_hdr(buf))){
#if UIP_TCP
      case UIP_PROTO_TCP:
        /* TCP, for both IPv4 and IPv6 */
        goto tcp_input;
#endif /* UIP_TCP */
#if UIP_UDP
      case UIP_PROTO_UDP:
        /* UDP, for both IPv4 and IPv6 */
        goto udp_input;
#endif /* UIP_UDP */
      case UIP_PROTO_ICMP6:
        /* ICMPv6 */
        goto icmp6_input;
      case UIP_PROTO_HBHO:
        PRINTF("Processing hbh header\n");
        /* Hop by hop option header */
#if UIP_CONF_IPV6_CHECKS
        /* Hop by hop option header. If we saw one HBH already, drop */
        if(uip_ext_bitmap(buf) & UIP_EXT_HDR_BITMAP_HBHO) {
          goto bad_hdr;
        } else {
          uip_ext_bitmap(buf) |= UIP_EXT_HDR_BITMAP_HBHO;
        }
#endif /*UIP_CONF_IPV6_CHECKS*/
        switch(ext_hdr_options_process(buf)) {
          case 0:
            /*continue*/
            uip_next_hdr(buf) = &UIP_EXT_BUF(buf)->next;
            uip_ext_len(buf) += (UIP_EXT_BUF(buf)->len << 3) + 8;
            break;
          case 1:
            /*silently discard*/
            goto drop;
          case 2:
            /* send icmp error message (created in ext_hdr_options_process)
             * and discard*/
            goto send;
        }
        break;
      case UIP_PROTO_DESTO:
#if UIP_CONF_IPV6_CHECKS
        /* Destination option header. if we saw two already, drop */
        PRINTF("Processing desto header\n");
        if(uip_ext_bitmap(buf) & UIP_EXT_HDR_BITMAP_DESTO1) {
          if(uip_ext_bitmap(buf) & UIP_EXT_HDR_BITMAP_DESTO2) {
            goto bad_hdr;
          } else{
            uip_ext_bitmap(buf) |= UIP_EXT_HDR_BITMAP_DESTO2;
          }
        } else {
          uip_ext_bitmap(buf) |= UIP_EXT_HDR_BITMAP_DESTO1;
        }
#endif /*UIP_CONF_IPV6_CHECKS*/
        switch(ext_hdr_options_process(buf)) {
          case 0:
            /*continue*/
            uip_next_hdr(buf) = &UIP_EXT_BUF(buf)->next;
            uip_ext_len(buf) += (UIP_EXT_BUF(buf)->len << 3) + 8;
            break;
          case 1:
            /*silently discard*/
            goto drop;
          case 2:
            /* send icmp error message (created in ext_hdr_options_process)
             * and discard*/
            goto send;
        }
        break;
      case UIP_PROTO_ROUTING:
#if UIP_CONF_IPV6_CHECKS
        /* Routing header. If we saw one already, drop */
        if(uip_ext_bitmap(buf) & UIP_EXT_HDR_BITMAP_ROUTING) {
          goto bad_hdr;
        } else {
          uip_ext_bitmap(buf) |= UIP_EXT_HDR_BITMAP_ROUTING;
        }
#endif /*UIP_CONF_IPV6_CHECKS*/
        /*
         * Routing Header  length field is in units of 8 bytes, excluding
         * As per RFC2460 section 4.4, if routing type is unrecognized:
         * if segments left = 0, ignore the header
         * if segments left > 0, discard packet and send icmp error pointing
         * to the routing type
         */

        PRINTF("Processing Routing header\n");
        if(UIP_ROUTING_BUF(buf)->seg_left > 0) {
          uip_icmp6_error_output(buf, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER, UIP_IPH_LEN + uip_ext_len(buf) + 2);
          UIP_STAT(++uip_stat.ip.drop);
          UIP_LOG("ip6: unrecognized routing type");
          goto send;
        }
        uip_next_hdr(buf) = &UIP_EXT_BUF(buf)->next;
        uip_ext_len(buf) += (UIP_EXT_BUF(buf)->len << 3) + 8;
        break;
      case UIP_PROTO_FRAG:
        /* Fragmentation header:call the reassembly function, then leave */
#if UIP_CONF_IPV6_REASSEMBLY
        PRINTF("Processing frag header\n");
        uip_len(buf) = uip_reass();
        if(uip_len(buf) == 0) {
          goto drop;
        }
        if(uip_reassflags & UIP_REASS_FLAG_ERROR_MSG){
          /* we are not done with reassembly, this is an error message */
          goto send;
        }
        /*packet is reassembled, reset the next hdr to the beginning
           of the IP header and restart the parsing of the reassembled pkt*/
        PRINTF("Processing reassembled packet\n");
        uip_ext_len(buf) = 0;
        uip_ext_bitmap(buf) = 0;
        uip_next_hdr(buf) = &UIP_IP_BUF(buf)->proto;
        break;
#else /* UIP_CONF_IPV6_REASSEMBLY */
        UIP_STAT(++uip_stat.ip.drop);
        UIP_STAT(++uip_stat.ip.fragerr);
        UIP_LOG("ip: fragment dropped.");
        goto drop;
#endif /* UIP_CONF_IPV6_REASSEMBLY */
      case UIP_PROTO_NONE:
        goto drop;
      default:
        goto bad_hdr;
    }
  }
  bad_hdr:
  /*
   * RFC 2460 send error message parameterr problem, code unrecognized
   * next header, pointing to the next header field
   */
  uip_icmp6_error_output(buf, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_NEXTHEADER, (uint32_t)(uip_next_hdr(buf) - (uint8_t *)UIP_IP_BUF(buf)));
  UIP_STAT(++uip_stat.ip.drop);
  UIP_STAT(++uip_stat.ip.protoerr);
  UIP_LOG("ip6: unrecognized header");
  goto send;
  /* End of headers processing */
  
  icmp6_input:
  /* This is IPv6 ICMPv6 processing code. */
  PRINTF("icmp6_input: length %d type: %d \n", uip_len(buf), UIP_ICMP_BUF(buf)->type);

#if UIP_CONF_IPV6_CHECKS
  /* Compute and check the ICMP header checksum */
  if(uip_icmp6chksum(buf) != 0xffff) {
    UIP_STAT(++uip_stat.icmp.drop);
    UIP_STAT(++uip_stat.icmp.chkerr);
    UIP_LOG("icmpv6: bad checksum.");
    PRINTF("icmpv6: bad checksum.");
    goto drop;
  }
#endif /*UIP_CONF_IPV6_CHECKS*/

  UIP_STAT(++uip_stat.icmp.recv);
  /*
   * Here we process incoming ICMPv6 packets
   * For echo request, we send echo reply
   * For ND pkts, we call the appropriate function in uip-nd6.c
   * We do not treat Error messages for now
   * If no pkt is to be sent as an answer to the incoming one, we
   * "goto drop". Else we just break; then at the after the "switch"
   * we "goto send"
   */
#if UIP_CONF_ICMP6
  UIP_ICMP6_APPCALL(UIP_ICMP_BUF(buf)->type);
#endif /*UIP_CONF_ICMP6*/

  /*
   * Search generic input handlers.
   * The handler is in charge of setting uip_len to 0
   */
  if(uip_icmp6_input(buf, UIP_ICMP_BUF(buf)->type,
                     UIP_ICMP_BUF(buf)->icode) == UIP_ICMP6_INPUT_ERROR) {
    PRINTF("Unknown ICMPv6 message type/code %d\n", UIP_ICMP_BUF(buf)->type);
    UIP_STAT(++uip_stat.icmp.drop);
    UIP_STAT(++uip_stat.icmp.typeerr);
    UIP_LOG("icmp6: unknown ICMPv6 message.");
    uip_len(buf) = 0;
  }
  
  if(uip_len(buf) > 0) {
    goto send;
  } else {
    goto drop;
  }
  /* End of IPv6 ICMP processing. */
   

#if UIP_UDP
  /* UDP input processing. */
 udp_input:

  remove_ext_hdr(buf);

  PRINTF("Receiving UDP packet\n");
 
  /* UDP processing is really just a hack. We don't do anything to the
     UDP/IP headers, but let the UDP application do all the hard
     work. If the application sets uip_slen, it has a packet to
     send. */
#if UIP_UDP_CHECKSUMS
  uip_len(buf) = uip_len(buf) - UIP_IPUDPH_LEN;
  uip_appdata(buf) = &uip_buf(buf)[UIP_IPUDPH_LEN + UIP_LLH_LEN];
  /* XXX hack: UDP/IPv6 receivers should drop packets with UDP
     checksum 0. Here, we explicitly receive UDP packets with checksum
     0. This is to be able to debug code that for one reason or
     another miscomputes UDP checksums. The reception of zero UDP
     checksums should be turned into a configration option. */
  if(UIP_UDP_BUF(buf)->udpchksum != 0 && uip_udpchksum(buf) != 0xffff) {
    UIP_STAT(++uip_stat.udp.drop);
    UIP_STAT(++uip_stat.udp.chkerr);
    PRINTF("udp: bad checksum 0x%04x 0x%04x\n", UIP_UDP_BUF(buf)->udpchksum,
           uip_udpchksum(buf));
    goto drop;
  }
#else /* UIP_UDP_CHECKSUMS */
  uip_len(buf) = uip_len(buf) - UIP_IPUDPH_LEN;
#endif /* UIP_UDP_CHECKSUMS */

  /* Make sure that the UDP destination port number is not zero. */
  if(UIP_UDP_BUF(buf)->destport == 0) {
    PRINTF("udp: zero port.\n");
    goto drop;
  }

  /* Demultiplex this UDP packet between the UDP "connections". */
  for(i = 0; i < UIP_UDP_CONNS; i++) {
    /* If the local UDP port is non-zero, the connection is considered
       to be used. If so, the local port number is checked against the
       destination port number in the received packet. If the two port
       numbers match, the remote port number is checked if the
       connection is bound to a remote port. Finally, if the
       connection is bound to a remote IP address, the source IP
       address of the packet is checked. */
#if 0
    PRINTF("%d: %p lport %d <- %d rport %d <- %d addr ",
	   i, &uip_udp_conns[i],
	   uip_ntohs(uip_udp_conns[i].lport), uip_ntohs(UIP_UDP_BUF(buf)->destport),
	   uip_ntohs(uip_udp_conns[i].rport), uip_ntohs(UIP_UDP_BUF(buf)->srcport));
    PRINT6ADDR(&uip_udp_conns[i].ripaddr);
    PRINTF(" <- ");
    PRINT6ADDR(&UIP_IP_BUF(buf)->srcipaddr);
    PRINTF("\n");
#endif /* 0 */
    if(uip_udp_conns[i].lport != 0 &&
       UIP_UDP_BUF(buf)->destport == uip_udp_conns[i].lport &&
       (uip_udp_conns[i].rport == 0 ||
        UIP_UDP_BUF(buf)->srcport == uip_udp_conns[i].rport) &&
       (uip_is_addr_unspecified(&uip_udp_conns[i].ripaddr) ||
        uip_ipaddr_cmp(&UIP_IP_BUF(buf)->srcipaddr, &uip_udp_conns[i].ripaddr))) {
      uip_set_udp_conn(buf) = &uip_udp_conns[i];
      goto udp_found;
    }
  }
  uip_set_udp_conn(buf) = NULL;
  PRINTF("udp: no matching connection found\n");
  UIP_STAT(++uip_stat.udp.drop);

#if UIP_UDP_SEND_UNREACH_NOPORT
  uip_icmp6_error_output(buf, ICMP6_DST_UNREACH, ICMP6_DST_UNREACH_NOPORT, 0);
  goto send;
#else
  uip_len(buf) = 0;
  goto drop;
#endif

 udp_found:
  PRINTF("In udp_found\n");
  UIP_STAT(++uip_stat.udp.recv);
 
  uip_set_conn(buf) = NULL;
  uip_flags(buf) = UIP_NEWDATA;
  uip_sappdata(buf) = uip_appdata(buf) = &uip_buf(buf)[UIP_IPUDPH_LEN + UIP_LLH_LEN];
  uip_slen(buf) = 0;
  UIP_UDP_APPCALL(buf);
  if(uip_slen(buf) == 0) {
    /* If the application does not want to send anything, then uip_slen(buf)
     * will be 0. In this case we MUST NOT set uip_len(buf) to 0 as that would
     * cause the net_buf to be released by rx fiber. In this case it is
     * application responsibility to release the buffer. Returning 0 here
     * will mean that tcpip.c:packet_input() will not try to call
     * tcpip_ipv6_output().
     */
    return 0;
  }

 udp_send:
  PRINTF("In udp_send\n");

  uip_len(buf) = uip_slen(buf) + UIP_IPUDPH_LEN;

  /* For IPv6, the IP length field does not include the IPv6 IP header
     length. */
  UIP_IP_BUF(buf)->len[0] = ((uip_len(buf) - UIP_IPH_LEN) >> 8);
  UIP_IP_BUF(buf)->len[1] = ((uip_len(buf) - UIP_IPH_LEN) & 0xff);

  UIP_IP_BUF(buf)->ttl = uip_udp_conn(buf)->ttl;
  UIP_IP_BUF(buf)->proto = UIP_PROTO_UDP;

  UIP_UDP_BUF(buf)->udplen = UIP_HTONS(uip_slen(buf) + UIP_UDPH_LEN);
  UIP_UDP_BUF(buf)->udpchksum = 0;

  UIP_UDP_BUF(buf)->srcport  = uip_udp_conn(buf)->lport;
  UIP_UDP_BUF(buf)->destport = uip_udp_conn(buf)->rport;

  uip_ipaddr_copy(&UIP_IP_BUF(buf)->destipaddr, &uip_udp_conn(buf)->ripaddr);
  uip_ds6_select_src(&UIP_IP_BUF(buf)->srcipaddr, &UIP_IP_BUF(buf)->destipaddr);

  uip_appdata(buf) = &uip_buf(buf)[UIP_LLH_LEN + UIP_IPTCPH_LEN];

#if UIP_UDP_CHECKSUMS
  /* Calculate UDP checksum. */
  UIP_UDP_BUF(buf)->udpchksum = ~(uip_udpchksum(buf));
  if(UIP_UDP_BUF(buf)->udpchksum == 0) {
    UIP_UDP_BUF(buf)->udpchksum = 0xffff;
  }
#endif /* UIP_UDP_CHECKSUMS */

#if UIP_CONF_IPV6_RPL
  rpl_insert_header(buf);
#endif /* UIP_CONF_IPV6_RPL */

  UIP_STAT(++uip_stat.udp.sent);
  goto ip_send_nolen;
#endif /* UIP_UDP */

#if UIP_TCP
  /* TCP input processing. */
 tcp_input:

  remove_ext_hdr(buf);

  UIP_STAT(++uip_stat.tcp.recv);
  PRINTF("Receiving TCP packet\n");
  /* Start of TCP input header processing code. */
  
  if(uip_tcpchksum(buf) != 0xffff) {   /* Compute and check the TCP
                                       checksum. */
    UIP_STAT(++uip_stat.tcp.drop);
    UIP_STAT(++uip_stat.tcp.chkerr);
    PRINTF("tcp: bad checksum 0x%04x 0x%04x\n", UIP_TCP_BUF(buf)->tcpchksum,
           uip_tcpchksum(buf));
    goto drop;
  }

  /* Make sure that the TCP port number is not zero. */
  if(UIP_TCP_BUF(buf)->destport == 0 || UIP_TCP_BUF(buf)->srcport == 0) {
    PRINTF("tcp: zero port.");
    goto drop;
  }

  /* Demultiplex this segment. */
  /* First check any active connections. */
  for(uip_connr = &uip_conns[0]; uip_connr <= &uip_conns[UIP_CONNS - 1];
      ++uip_connr) {
    if(uip_connr->tcpstateflags != UIP_CLOSED &&
       UIP_TCP_BUF(buf)->destport == uip_connr->lport &&
       UIP_TCP_BUF(buf)->srcport == uip_connr->rport &&
       uip_ipaddr_cmp(&UIP_IP_BUF(buf)->srcipaddr, &uip_connr->ripaddr)) {
      goto found;
    }
  }

  /* If we didn't find and active connection that expected the packet,
     either this packet is an old duplicate, or this is a SYN packet
     destined for a connection in LISTEN. If the SYN flag isn't set,
     it is an old packet and we send a RST. */
  if((UIP_TCP_BUF(buf)->flags & TCP_CTL) != TCP_SYN) {
    goto reset;
  }
  
  tmp16 = UIP_TCP_BUF(buf)->destport;
  /* Next, check listening connections. */
  for(c = 0; c < UIP_LISTENPORTS; ++c) {
    if(tmp16 == uip_listenports[c]) {
      goto found_listen;
    }
  }
  
  /* No matching connection found, so we send a RST packet. */
  UIP_STAT(++uip_stat.tcp.synrst);

 reset:
  PRINTF("In reset\n");
  /* We do not send resets in response to resets. */
  if(UIP_TCP_BUF(buf)->flags & TCP_RST) {
    goto drop;
  }

  UIP_STAT(++uip_stat.tcp.rst);
  
  UIP_TCP_BUF(buf)->flags = TCP_RST | TCP_ACK;
  uip_len(buf) = UIP_IPTCPH_LEN;
  UIP_TCP_BUF(buf)->tcpoffset = 5 << 4;

  /* Flip the seqno and ackno fields in the TCP header. */
  c = UIP_TCP_BUF(buf)->seqno[3];
  UIP_TCP_BUF(buf)->seqno[3] = UIP_TCP_BUF(buf)->ackno[3];
  UIP_TCP_BUF(buf)->ackno[3] = c;
  
  c = UIP_TCP_BUF(buf)->seqno[2];
  UIP_TCP_BUF(buf)->seqno[2] = UIP_TCP_BUF(buf)->ackno[2];
  UIP_TCP_BUF(buf)->ackno[2] = c;
  
  c = UIP_TCP_BUF(buf)->seqno[1];
  UIP_TCP_BUF(buf)->seqno[1] = UIP_TCP_BUF(buf)->ackno[1];
  UIP_TCP_BUF(buf)->ackno[1] = c;
  
  c = UIP_TCP_BUF(buf)->seqno[0];
  UIP_TCP_BUF(buf)->seqno[0] = UIP_TCP_BUF(buf)->ackno[0];
  UIP_TCP_BUF(buf)->ackno[0] = c;

  /* We also have to increase the sequence number we are
     acknowledging. If the least significant byte overflowed, we need
     to propagate the carry to the other bytes as well. */
  if(++UIP_TCP_BUF(buf)->ackno[3] == 0) {
    if(++UIP_TCP_BUF(buf)->ackno[2] == 0) {
      if(++UIP_TCP_BUF(buf)->ackno[1] == 0) {
        ++UIP_TCP_BUF(buf)->ackno[0];
      }
    }
  }
 
  /* Swap port numbers. */
  tmp16 = UIP_TCP_BUF(buf)->srcport;
  UIP_TCP_BUF(buf)->srcport = UIP_TCP_BUF(buf)->destport;
  UIP_TCP_BUF(buf)->destport = tmp16;
  
  /* Swap IP addresses. */
  uip_ipaddr_copy(&UIP_IP_BUF(buf)->destipaddr, &UIP_IP_BUF(buf)->srcipaddr);
  uip_ds6_select_src(&UIP_IP_BUF(buf)->srcipaddr, &UIP_IP_BUF(buf)->destipaddr);
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
  uip_connr->lport = UIP_TCP_BUF(buf)->destport;
  uip_connr->rport = UIP_TCP_BUF(buf)->srcport;
  uip_ipaddr_copy(&uip_connr->ripaddr, &UIP_IP_BUF(buf)->srcipaddr);
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
  uip_connr->rcv_nxt[3] = UIP_TCP_BUF(buf)->seqno[3];
  uip_connr->rcv_nxt[2] = UIP_TCP_BUF(buf)->seqno[2];
  uip_connr->rcv_nxt[1] = UIP_TCP_BUF(buf)->seqno[1];
  uip_connr->rcv_nxt[0] = UIP_TCP_BUF(buf)->seqno[0];
  uip_add_rcv_nxt(buf, 1);

  /* Parse the TCP MSS option, if present. */
  if((UIP_TCP_BUF(buf)->tcpoffset & 0xf0) > 0x50) {
    for(c = 0; c < ((UIP_TCP_BUF(buf)->tcpoffset >> 4) - 5) << 2 ;) {
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
  UIP_TCP_BUF(buf)->flags = TCP_ACK;

tcp_send_syn:
  UIP_TCP_BUF(buf)->flags |= TCP_SYN;
  tcp_set_retrans_timer(uip_connr);
#else /* UIP_ACTIVE_OPEN */
 tcp_send_synack:
  UIP_TCP_BUF(buf)->flags = TCP_SYN | TCP_ACK;
#endif /* UIP_ACTIVE_OPEN */
  
  /* We send out the TCP Maximum Segment Size option with our
     SYNACK. */
  UIP_TCP_BUF(buf)->optdata[0] = TCP_OPT_MSS;
  UIP_TCP_BUF(buf)->optdata[1] = TCP_OPT_MSS_LEN;
  UIP_TCP_BUF(buf)->optdata[2] = (UIP_TCP_MSS) / 256;
  UIP_TCP_BUF(buf)->optdata[3] = (UIP_TCP_MSS) & 255;
  uip_len(buf) = UIP_IPTCPH_LEN + TCP_OPT_MSS_LEN;
  UIP_TCP_BUF(buf)->tcpoffset = ((UIP_TCPH_LEN + TCP_OPT_MSS_LEN) / 4) << 4;
  goto tcp_send;

  /* This label will be jumped to if we found an active connection. */
 found:
  PRINTF("In found\n");
  uip_set_conn(buf) = uip_connr;
  uip_flags(buf) = 0;
  /* We do a very naive form of TCP reset processing; we just accept
     any RST and kill our connection. We should in fact check if the
     sequence number of this reset is wihtin our advertised window
     before we accept the reset. */
  if(UIP_TCP_BUF(buf)->flags & TCP_RST) {
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
    c = (UIP_TCP_BUF(buf)->tcpoffset >> 4) << 2;
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
	((UIP_TCP_BUF(buf)->flags & TCP_CTL) == (TCP_SYN | TCP_ACK))) ||
       (((uip_connr->tcpstateflags & UIP_TS_MASK) == UIP_SYN_RCVD) &&
	((UIP_TCP_BUF(buf)->flags & TCP_CTL) == TCP_SYN)))) {
    if((uip_len(buf) > 0 || ((UIP_TCP_BUF(buf)->flags & (TCP_SYN | TCP_FIN)) != 0)) &&
       (UIP_TCP_BUF(buf)->seqno[0] != uip_connr->rcv_nxt[0] ||
        UIP_TCP_BUF(buf)->seqno[1] != uip_connr->rcv_nxt[1] ||
        UIP_TCP_BUF(buf)->seqno[2] != uip_connr->rcv_nxt[2] ||
        UIP_TCP_BUF(buf)->seqno[3] != uip_connr->rcv_nxt[3])) {

      if((UIP_TCP_BUF(buf)->flags & TCP_SYN)) {
        if((uip_connr->tcpstateflags & UIP_TS_MASK) == UIP_SYN_RCVD) {
          goto tcp_send_synack;
#if UIP_ACTIVE_OPEN
        } else if((uip_connr->tcpstateflags & UIP_TS_MASK) == UIP_SYN_SENT) {
          goto tcp_send_syn;
#endif
        }
      }
      goto tcp_send_ack;
    }
  }

  /* Next, check if the incoming segment acknowledges any outstanding
     data. If so, we update the sequence number, reset the length of
     the outstanding data, calculate RTT estimations, and reset the
     retransmission timer. */
  if((UIP_TCP_BUF(buf)->flags & TCP_ACK) && uip_outstanding(uip_connr)) {
    uip_add32(uip_connr->snd_nxt, uip_connr->len);

    if(UIP_TCP_BUF(buf)->ackno[0] == uip_acc32[0] &&
       UIP_TCP_BUF(buf)->ackno[1] == uip_acc32[1] &&
       UIP_TCP_BUF(buf)->ackno[2] == uip_acc32[2] &&
       UIP_TCP_BUF(buf)->ackno[3] == uip_acc32[3]) {
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
      if((UIP_TCP_BUF(buf)->flags & TCP_CTL) == TCP_SYN) {
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
         (UIP_TCP_BUF(buf)->flags & TCP_CTL) == (TCP_SYN | TCP_ACK)) {

        /* Parse the TCP MSS option, if present. */
        if((UIP_TCP_BUF(buf)->tcpoffset & 0xf0) > 0x50) {
          for(c = 0; c < ((UIP_TCP_BUF(buf)->tcpoffset >> 4) - 5) << 2 ;) {
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
        uip_connr->rcv_nxt[0] = UIP_TCP_BUF(buf)->seqno[0];
        uip_connr->rcv_nxt[1] = UIP_TCP_BUF(buf)->seqno[1];
        uip_connr->rcv_nxt[2] = UIP_TCP_BUF(buf)->seqno[2];
        uip_connr->rcv_nxt[3] = UIP_TCP_BUF(buf)->seqno[3];
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

      if(UIP_TCP_BUF(buf)->flags & TCP_FIN && !(uip_connr->tcpstateflags & UIP_STOPPED)) {
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
        UIP_TCP_BUF(buf)->flags = TCP_FIN | TCP_ACK;
        goto tcp_send_nodata;
      }

      /* Check the URG flag. If this is set, the segment carries urgent
         data that we must pass to the application. */
      if((UIP_TCP_BUF(buf)->flags & TCP_URG) != 0) {
#if UIP_URGDATA > 0
        uip_urglen = (UIP_TCP_BUF->urgp[0] << 8) | UIP_TCP_BUF->urgp[1];
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
        uip_appdata(buf) = ((char *)uip_appdata(buf)) + ((UIP_TCP_BUF(buf)->urgp[0] << 8) | UIP_TCP_BUF(buf)->urgp[1]);
        uip_len(buf) -= (UIP_TCP_BUF(buf)->urgp[0] << 8) | UIP_TCP_BUF(buf)->urgp[1];
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
      tmp16 = ((uint16_t)UIP_TCP_BUF(buf)->wnd[0] << 8) + (uint16_t)UIP_TCP_BUF(buf)->wnd[1];
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

	  /* Eventually the uip_connr->buf will be freed
	   * by net_core.c:net_send()
	   */

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
          UIP_TCP_BUF(buf)->flags = TCP_RST | TCP_ACK;
          goto tcp_send_nodata;
        }

        if(uip_flags(buf) & UIP_CLOSE) {
          uip_slen(buf) = 0;
          uip_connr->len = 1;
          uip_connr->tcpstateflags = UIP_FIN_WAIT_1;
          uip_connr->nrtx = 0;
          UIP_TCP_BUF(buf)->flags = TCP_FIN | TCP_ACK;
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
          UIP_TCP_BUF(buf)->flags = TCP_ACK | TCP_PSH;
          /* Send the packet. */
          goto tcp_send_noopts;
        }
        /* If there is no data to send, just send out a pure ACK if
           there is newdata. */
        if(uip_flags(buf) & UIP_NEWDATA) {
          uip_len(buf) = UIP_TCPIP_HLEN;
          UIP_TCP_BUF(buf)->flags = TCP_ACK;
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
      if(UIP_TCP_BUF(buf)->flags & TCP_FIN) {
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
      if(UIP_TCP_BUF(buf)->flags & TCP_FIN) {
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
  UIP_TCP_BUF(buf)->flags = TCP_ACK;

 tcp_send_nodata:
  if (flag != UIP_TCP_SEND_CONN) {
    PRINTF("In tcp_send_nodata\n");
    uip_len(buf) = UIP_IPTCPH_LEN;
    buf->len = UIP_IPTCPH_LEN;
  }

 tcp_send_noopts:
  UIP_TCP_BUF(buf)->tcpoffset = (UIP_TCPH_LEN / 4) << 4;

  /* We're done with the input processing. We are now ready to send a
     reply. Our job is to fill in all the fields of the TCP and IP
     headers before calculating the checksum and finally send the
     packet. */
 tcp_send:
  PRINTF("In tcp_send\n");
   
  UIP_TCP_BUF(buf)->ackno[0] = uip_connr->rcv_nxt[0];
  UIP_TCP_BUF(buf)->ackno[1] = uip_connr->rcv_nxt[1];
  UIP_TCP_BUF(buf)->ackno[2] = uip_connr->rcv_nxt[2];
  UIP_TCP_BUF(buf)->ackno[3] = uip_connr->rcv_nxt[3];
  
  UIP_TCP_BUF(buf)->seqno[0] = uip_connr->snd_nxt[0];
  UIP_TCP_BUF(buf)->seqno[1] = uip_connr->snd_nxt[1];
  UIP_TCP_BUF(buf)->seqno[2] = uip_connr->snd_nxt[2];
  UIP_TCP_BUF(buf)->seqno[3] = uip_connr->snd_nxt[3];

  UIP_TCP_BUF(buf)->srcport  = uip_connr->lport;
  UIP_TCP_BUF(buf)->destport = uip_connr->rport;

  uip_ipaddr_copy(&UIP_IP_BUF(buf)->destipaddr, &uip_connr->ripaddr);
  uip_ds6_select_src(&UIP_IP_BUF(buf)->srcipaddr, &UIP_IP_BUF(buf)->destipaddr);
  PRINTF("Sending TCP packet to ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->destipaddr);
  PRINTF(" from ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->srcipaddr);
  PRINTF("\n");

  if(uip_connr->tcpstateflags & UIP_STOPPED) {
    /* If the connection has issued uip_stop(), we advertise a zero
       window so that the remote host will stop sending data. */
    UIP_TCP_BUF(buf)->wnd[0] = UIP_TCP_BUF(buf)->wnd[1] = 0;
  } else {
    UIP_TCP_BUF(buf)->wnd[0] = ((UIP_RECEIVE_WINDOW) >> 8);
    UIP_TCP_BUF(buf)->wnd[1] = ((UIP_RECEIVE_WINDOW) & 0xff);
  }

 tcp_send_noconn:
  UIP_IP_BUF(buf)->proto = UIP_PROTO_TCP;

  UIP_IP_BUF(buf)->ttl = uip_ds6_if.cur_hop_limit;
  UIP_IP_BUF(buf)->len[0] = ((uip_len(buf) - UIP_IPH_LEN) >> 8);
  UIP_IP_BUF(buf)->len[1] = ((uip_len(buf) - UIP_IPH_LEN) & 0xff);

  UIP_TCP_BUF(buf)->urgp[0] = UIP_TCP_BUF(buf)->urgp[1] = 0;
  
  /* Calculate TCP checksum. */
  UIP_TCP_BUF(buf)->tcpchksum = 0;
  UIP_TCP_BUF(buf)->tcpchksum = ~(uip_tcpchksum(buf));
  UIP_STAT(++uip_stat.tcp.sent);

#endif /* UIP_TCP */
#if UIP_UDP
 ip_send_nolen:
#endif
  UIP_IP_BUF(buf)->vtc = 0x60;
  UIP_IP_BUF(buf)->tcflow = 0x00;
  UIP_IP_BUF(buf)->flow = 0x00;
 send:
  PRINTF("Sending packet with length %d (%d)\n", uip_len(buf),
         (UIP_IP_BUF(buf)->len[0] << 8) | UIP_IP_BUF(buf)->len[1]);
  
  UIP_STAT(++uip_stat.ip.sent);
  /* Return and let the caller do the actual transmission. */
  uip_flags(buf) = 0;
  buf->len = uip_len(buf);
  return 1;

 drop:
  uip_len(buf) = 0;
  uip_ext_len(buf) = 0;
  uip_ext_bitmap(buf) = 0;
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
       int ret;

       ret = tcpip_ipv6_output(buf);
       if (!ret) {
         PRINTF("Packet %p sending failed.\n", buf);
	 ip_buf_sent_status(buf) = -EAGAIN;
       }
    }
  }
}
#endif /* UIP_TCP */
/*---------------------------------------------------------------------------*/
/** @} */
