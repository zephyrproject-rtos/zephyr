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
 * Implementation of the ARP Address Resolution Protocol.
 * \author Adam Dunkels <adam@dunkels.com>
 *
 */

/**
 * \addtogroup uip
 * @{
 */

/**
 * \defgroup uiparp uIP Address Resolution Protocol
 * @{
 *
 * The Address Resolution Protocol ARP is used for mapping between IP
 * addresses and link level addresses such as the Ethernet MAC
 * addresses. ARP uses broadcast queries to ask for the link level
 * address of a known IP address and the host which is configured with
 * the IP address for which the query was meant, will respond with its
 * link level address.
 *
 * \note This ARP implementation only supports Ethernet.
 */

#include "contiki/ipv4/uip_arp.h"

#include <net/ip_buf.h>
#include <string.h>

struct arp_hdr {
  struct uip_eth_hdr ethhdr;
  uint16_t hwtype;
  uint16_t protocol;
  uint8_t hwlen;
  uint8_t protolen;
  uint16_t opcode;
  struct uip_eth_addr shwaddr;
  uip_ipaddr_t sipaddr;
  struct uip_eth_addr dhwaddr;
  uip_ipaddr_t dipaddr;
} PACK_ALIAS_STRUCT;

struct ethip_hdr {
  struct uip_eth_hdr ethhdr;
  /* IP header. */
  uint8_t vhl,
    tos,
    len[2],
    ipid[2],
    ipoffset[2],
    ttl,
    proto;
  uint16_t ipchksum;
  uip_ipaddr_t srcipaddr, destipaddr;
} PACK_ALIAS_STRUCT;

#define ARP_REQUEST 1
#define ARP_REPLY   2

#define ARP_HWTYPE_ETH 1

struct arp_entry {
  uip_ipaddr_t ipaddr;
  struct uip_eth_addr ethaddr;
  uint8_t time;
};

static const struct uip_eth_addr broadcast_ethaddr =
  {{0xff,0xff,0xff,0xff,0xff,0xff}};

static struct arp_entry arp_table[UIP_ARPTAB_SIZE];
static uip_ipaddr_t ipaddr;
static uint8_t i, c;

static uint8_t arptime;
static uint8_t tmpage;

#define BUF(buf)   ((struct arp_hdr *)&uip_buf(buf)[0])
#define IPBUF(buf) ((struct ethip_hdr *)&uip_buf(buf)[0])

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_IPV4_ARP
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

/*-----------------------------------------------------------------------------------*/
/**
 * Initialize the ARP module.
 *
 */
/*-----------------------------------------------------------------------------------*/
void
uip_arp_init(void)
{
  for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
    memset(&arp_table[i].ipaddr, 0, 4);
  }
}
/*-----------------------------------------------------------------------------------*/
/**
 * Periodic ARP processing function.
 *
 * This function performs periodic timer processing in the ARP module
 * and should be called at regular intervals. The recommended interval
 * is 10 seconds between the calls.
 *
 */
/*-----------------------------------------------------------------------------------*/
void
uip_arp_timer(void)
{
  struct arp_entry *tabptr;

  ++arptime;
  for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
    tabptr = &arp_table[i];
    if(uip_ipaddr_cmp(&tabptr->ipaddr, &uip_all_zeroes_addr) &&
       arptime - tabptr->time >= UIP_ARP_MAXAGE) {
      memset(&tabptr->ipaddr, 0, 4);
    }
  }

}

/*-----------------------------------------------------------------------------------*/
static void
uip_arp_update(uip_ipaddr_t *ipaddr, struct uip_eth_addr *ethaddr)
{
  register struct arp_entry *tabptr = arp_table;

  /* Walk through the ARP mapping table and try to find an entry to
     update. If none is found, the IP -> MAC address mapping is
     inserted in the ARP table. */
  for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
    tabptr = &arp_table[i];

    /* Only check those entries that are actually in use. */
    if(!uip_ipaddr_cmp(&tabptr->ipaddr, &uip_all_zeroes_addr)) {

      /* Check if the source IP address of the incoming packet matches
         the IP address in this ARP table entry. */
      if(uip_ipaddr_cmp(ipaddr, &tabptr->ipaddr)) {

	/* An old entry found, update this and return. */
	memcpy(tabptr->ethaddr.addr, ethaddr->addr, 6);
	tabptr->time = arptime;

	return;
      }
    }
	tabptr++;
  }

  /* If we get here, no existing ARP table entry was found, so we
     create one. */

  /* First, we try to find an unused entry in the ARP table. */
  for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
    tabptr = &arp_table[i];
    if(uip_ipaddr_cmp(&tabptr->ipaddr, &uip_all_zeroes_addr)) {
      break;
    }
  }

  /* If no unused entry is found, we try to find the oldest entry and
     throw it away. */
  if(i == UIP_ARPTAB_SIZE) {
    tmpage = 0;
    c = 0;
    for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
      tabptr = &arp_table[i];
      if(arptime - tabptr->time > tmpage) {
	tmpage = arptime - tabptr->time;
	c = i;
      }
    }
    i = c;
    tabptr = &arp_table[i];
  }

  /* Now, i is the ARP table entry which we will fill with the new
     information. */
  uip_ipaddr_copy(&tabptr->ipaddr, ipaddr);
  memcpy(tabptr->ethaddr.addr, ethaddr->addr, 6);
  tabptr->time = arptime;
}
/*-----------------------------------------------------------------------------------*/
/**
 * ARP processing for incoming IP packets
 *
 * This function should be called by the device driver when an IP
 * packet has been received. The function will check if the address is
 * in the ARP cache, and if so the ARP cache entry will be
 * refreshed. If no ARP cache entry was found, a new one is created.
 *
 * This function expects an IP packet with a prepended Ethernet header
 * in the uip_buf[] buffer, and the length of the packet in the global
 * variable uip_len.
 */
/*-----------------------------------------------------------------------------------*/
#if 0
void
uip_arp_ipin(void)
{
  uip_len -= sizeof(struct uip_eth_hdr);

  /* Only insert/update an entry if the source IP address of the
     incoming IP packet comes from a host on the local network. */
  if((IPBUF->srcipaddr[0] & uip_netmask[0]) !=
     (uip_hostaddr[0] & uip_netmask[0])) {
    return;
  }
  if((IPBUF->srcipaddr[1] & uip_netmask[1]) !=
     (uip_hostaddr[1] & uip_netmask[1])) {
    return;
  }
  uip_arp_update(IPBUF->srcipaddr, &(IPBUF->ethhdr.src));

  return;
}
#endif /* 0 */
/*-----------------------------------------------------------------------------------*/
/**
 * ARP processing for incoming ARP packets.
 *
 * This function should be called by the device driver when an ARP
 * packet has been received. The function will act differently
 * depending on the ARP packet type: if it is a reply for a request
 * that we previously sent out, the ARP cache will be filled in with
 * the values from the ARP reply. If the incoming ARP packet is an ARP
 * request for our IP address, an ARP reply packet is created and put
 * into the uip_buf[] buffer.
 *
 * When the function returns, the value of the global variable uip_len
 * indicates whether the device driver should send out a packet or
 * not. If uip_len is zero, no packet should be sent. If uip_len is
 * non-zero, it contains the length of the outbound packet that is
 * present in the uip_buf[] buffer.
 *
 * This function expects an ARP packet with a prepended Ethernet
 * header in the uip_buf[] buffer, and the length of the packet in the
 * global variable uip_len.
 */
/*-----------------------------------------------------------------------------------*/
void
uip_arp_arpin(struct net_buf *buf)
{

  if(uip_len(buf) < sizeof(struct arp_hdr)) {
    uip_len(buf) = 0;
    return;
  }
  uip_len(buf) = 0;

  switch(BUF(buf)->opcode) {
  case UIP_HTONS(ARP_REQUEST):
    /* ARP request. If it asked for our address, we send out a
       reply. */
    /*    if(BUF->dipaddr[0] == uip_hostaddr[0] &&
	  BUF->dipaddr[1] == uip_hostaddr[1]) {*/
    PRINTF("uip_arp_arpin: request for %d.%d.%d.%d (we are %d.%d.%d.%d)\n",
	   BUF(buf)->dipaddr.u8[0], BUF(buf)->dipaddr.u8[1],
	   BUF(buf)->dipaddr.u8[2], BUF(buf)->dipaddr.u8[3],
	   uip_hostaddr.u8[0], uip_hostaddr.u8[1],
	   uip_hostaddr.u8[2], uip_hostaddr.u8[3]);
    if(uip_ipaddr_cmp(&BUF(buf)->dipaddr, &uip_hostaddr)) {
      /* First, we register the one who made the request in our ARP
	 table, since it is likely that we will do more communication
	 with this host in the future. */
      uip_arp_update(&BUF(buf)->sipaddr, &BUF(buf)->shwaddr);

      BUF(buf)->opcode = UIP_HTONS(ARP_REPLY);

      memcpy(BUF(buf)->dhwaddr.addr, BUF(buf)->shwaddr.addr, 6);
      memcpy(BUF(buf)->shwaddr.addr, uip_lladdr.addr, 6);
      memcpy(BUF(buf)->ethhdr.src.addr, uip_lladdr.addr, 6);
      memcpy(BUF(buf)->ethhdr.dest.addr, BUF(buf)->dhwaddr.addr, 6);

      uip_ipaddr_copy(&BUF(buf)->dipaddr, &BUF(buf)->sipaddr);
      uip_ipaddr_copy(&BUF(buf)->sipaddr, &uip_hostaddr);

      BUF(buf)->ethhdr.type = UIP_HTONS(UIP_ETHTYPE_ARP);
      uip_len(buf) = sizeof(struct arp_hdr);
    }
    break;
  case UIP_HTONS(ARP_REPLY):
    /* ARP reply. We insert or update the ARP table if it was meant
       for us. */
    if(uip_ipaddr_cmp(&BUF(buf)->dipaddr, &uip_hostaddr)) {
      uip_arp_update(&BUF(buf)->sipaddr, &BUF(buf)->shwaddr);
    }
    break;
  }

  return;
}
/*-----------------------------------------------------------------------------------*/
/**
 * Prepend Ethernet header to an outbound IP packet and see if we need
 * to send out an ARP request.
 *
 * This function should be called before sending out an IP packet. The
 * function checks the destination IP address of the IP packet to see
 * what Ethernet MAC address that should be used as a destination MAC
 * address on the Ethernet.
 *
 * If the destination IP address is in the local network (determined
 * by logical ANDing of netmask and our IP address), the function
 * checks the ARP cache to see if an entry for the destination IP
 * address is found. If so, an Ethernet header is prepended and the
 * function returns. If no ARP cache entry is found for the
 * destination IP address, the packet in the uip_buf[] is replaced by
 * an ARP request packet for the IP address. The IP packet is dropped
 * and it is assumed that they higher level protocols (e.g., TCP)
 * eventually will retransmit the dropped packet.
 *
 * If the destination IP address is not on the local network, the IP
 * address of the default router is used instead.
 *
 * When the function returns, a packet is present in the uip_buf[]
 * buffer, and the length of the packet is in the global variable
 * uip_len.
 */
/*-----------------------------------------------------------------------------------*/
void
uip_arp_out(struct net_buf *buf)
{
  struct arp_entry *tabptr = arp_table;

  /* Find the destination IP address in the ARP table and construct
     the Ethernet header. If the destination IP addres isn't on the
     local network, we use the default router's IP address instead.

     If not ARP table entry is found, we overwrite the original IP
     packet with an ARP request for the IP address. */

  /* First check if destination is a local broadcast. */
  if(uip_ipaddr_cmp(&IPBUF(buf)->destipaddr, &uip_broadcast_addr)) {
    memcpy(IPBUF(buf)->ethhdr.dest.addr, broadcast_ethaddr.addr, 6);
  } else if(IPBUF(buf)->destipaddr.u8[0] == 224) {
    /* Multicast. */
    IPBUF(buf)->ethhdr.dest.addr[0] = 0x01;
    IPBUF(buf)->ethhdr.dest.addr[1] = 0x00;
    IPBUF(buf)->ethhdr.dest.addr[2] = 0x5e;
    IPBUF(buf)->ethhdr.dest.addr[3] = IPBUF(buf)->destipaddr.u8[1];
    IPBUF(buf)->ethhdr.dest.addr[4] = IPBUF(buf)->destipaddr.u8[2];
    IPBUF(buf)->ethhdr.dest.addr[5] = IPBUF(buf)->destipaddr.u8[3];
  } else {
    /* Check if the destination address is on the local network. */
    if(!uip_ipaddr_maskcmp(&IPBUF(buf)->destipaddr, &uip_hostaddr, &uip_netmask)) {
      /* Destination address was not on the local network, so we need to
	 use the default router's IP address instead of the destination
	 address when determining the MAC address. */
      uip_ipaddr_copy(&ipaddr, &uip_draddr);
    } else {
      /* Else, we use the destination IP address. */
      uip_ipaddr_copy(&ipaddr, &IPBUF(buf)->destipaddr);
    }
    for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
      if(uip_ipaddr_cmp(&ipaddr, &tabptr->ipaddr)) {
	break;
      }
	  tabptr++;
    }

    if(i == UIP_ARPTAB_SIZE) {
      /* The destination address was not in our ARP table, so we
	 overwrite the IP packet with an ARP request. */

      memset(BUF(buf)->ethhdr.dest.addr, 0xff, 6);
      memset(BUF(buf)->dhwaddr.addr, 0x00, 6);
      memcpy(BUF(buf)->ethhdr.src.addr, uip_lladdr.addr, 6);
      memcpy(BUF(buf)->shwaddr.addr, uip_lladdr.addr, 6);

      uip_ipaddr_copy(&BUF(buf)->dipaddr, &ipaddr);
      uip_ipaddr_copy(&BUF(buf)->sipaddr, &uip_hostaddr);
      BUF(buf)->opcode = UIP_HTONS(ARP_REQUEST); /* ARP request. */
      BUF(buf)->hwtype = UIP_HTONS(ARP_HWTYPE_ETH);
      BUF(buf)->protocol = UIP_HTONS(UIP_ETHTYPE_IP);
      BUF(buf)->hwlen = 6;
      BUF(buf)->protolen = 4;
      BUF(buf)->ethhdr.type = UIP_HTONS(UIP_ETHTYPE_ARP);

      uip_appdata(buf) = &uip_buf(buf)[UIP_TCPIP_HLEN + UIP_LLH_LEN];

      uip_len(buf) = sizeof(struct arp_hdr);
      return;
    }

    /* Build an ethernet header. */
    memcpy(IPBUF(buf)->ethhdr.dest.addr, tabptr->ethaddr.addr, 6);
  }
  memcpy(IPBUF(buf)->ethhdr.src.addr, uip_lladdr.addr, 6);

  IPBUF(buf)->ethhdr.type = UIP_HTONS(UIP_ETHTYPE_IP);

  uip_len(buf) += sizeof(struct uip_eth_hdr);
}
/*-----------------------------------------------------------------------------------*/

/** @} */
/** @} */
