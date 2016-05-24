/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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

#include <stdio.h>
#include <string.h>

#include <net/ip_buf.h>

#include "contiki.h"
#include "contiki-net.h"
#include "contiki/ip/dhcpc.h"

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_DHCP
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#define STATE_INITIAL         0
#define STATE_SENDING         1
#define STATE_OFFER_RECEIVED  2
#define STATE_CONFIG_RECEIVED 3

static struct dhcpc_state s;
dhcpc_configured configure_cb = NULL;
dhcpc_unconfigured unconfigure_cb = NULL;

struct dhcp_msg {
  uint8_t op, htype, hlen, hops;
  uint8_t xid[4];
  uint16_t secs, flags;
  uint8_t ciaddr[4];
  uint8_t yiaddr[4];
  uint8_t siaddr[4];
  uint8_t giaddr[4];
  uint8_t chaddr[16];
#ifndef UIP_CONF_DHCP_LIGHT
  uint8_t sname[64];
  uint8_t file[128];
#endif
  uint8_t options[312];
} __packed;

#ifdef CONFIG_DHCP_BROADCAST
#define BOOTP_BROADCAST 0x8000
#elif CONFIG_DHCP_UNICAST
#define BOOTP_UNICAST   0x0000
#endif

#define DHCP_REQUEST        1
#define DHCP_REPLY          2
#define DHCP_HTYPE_ETHERNET 1
#define DHCP_HLEN_ETHERNET  6
#define DHCP_MSG_LEN      236

#define DHCPC_SERVER_PORT  67
#define DHCPC_CLIENT_PORT  68

#define DHCPDISCOVER  1
#define DHCPOFFER     2
#define DHCPREQUEST   3
#define DHCPDECLINE   4
#define DHCPACK       5
#define DHCPNAK       6
#define DHCPRELEASE   7

#define DHCP_OPTION_SUBNET_MASK   1
#define DHCP_OPTION_ROUTER        3
#define DHCP_OPTION_DNS_SERVER    6
#define DHCP_OPTION_REQ_IPADDR   50
#define DHCP_OPTION_LEASE_TIME   51
#define DHCP_OPTION_MSG_TYPE     53
#define DHCP_OPTION_SERVER_ID    54
#define DHCP_OPTION_REQ_LIST     55
#define DHCP_OPTION_END         255

static uint32_t xid;
static const uint8_t magic_cookie[4] = {99, 130, 83, 99};

/*---------------------------------------------------------------------------*/
static void
configure_dhcpc(struct dhcpc_state *s)
{
  uip_sethostaddr(&s->ipaddr);
  uip_setnetmask(&s->netmask);
  uip_setdraddr(&s->default_router);
  uip_nameserver_update(&s->dnsaddr, UIP_NAMESERVER_INFINITE_LIFETIME);

  if(configure_cb)
    configure_cb();
}
/*---------------------------------------------------------------------------*/
static void
unconfigure_dhcpc(struct dhcpc_state *s)
{
  if(unconfigure_cb)
    unconfigure_cb();
}
/*---------------------------------------------------------------------------*/
static uint8_t *
add_msg_type(uint8_t *optptr, uint8_t type)
{
  *optptr++ = DHCP_OPTION_MSG_TYPE;
  *optptr++ = 1;
  *optptr++ = type;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *
add_server_id(uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_SERVER_ID;
  *optptr++ = 4;
  memcpy(optptr, s.serverid, 4);
  return optptr + 4;
}
/*---------------------------------------------------------------------------*/
static uint8_t *
add_req_ipaddr(uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_REQ_IPADDR;
  *optptr++ = 4;
  memcpy(optptr, s.ipaddr.u16, 4);
  return optptr + 4;
}
/*---------------------------------------------------------------------------*/
static uint8_t *
add_req_options(uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_REQ_LIST;
  *optptr++ = 3;
  *optptr++ = DHCP_OPTION_SUBNET_MASK;
  *optptr++ = DHCP_OPTION_ROUTER;
  *optptr++ = DHCP_OPTION_DNS_SERVER;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *
add_end(uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_END;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static void
create_msg(struct dhcp_msg *m)
{
  m->op = DHCP_REQUEST;
  m->htype = DHCP_HTYPE_ETHERNET;
  m->hlen = s.mac_len;
  m->hops = 0;
  memcpy(m->xid, &xid, sizeof(m->xid));
  m->secs = 0;
#ifdef CONFIG_DHCP_BROADCAST
  m->flags = UIP_HTONS(BOOTP_BROADCAST); /*  Broadcast bit. */
#elif CONFIG_DHCP_UNICAST
  m->flags = UIP_HTONS(BOOTP_UNICAST); /*  Unicast bit. */
#endif

  /*  uip_ipaddr_copy(m->ciaddr, uip_hostaddr);*/
  memcpy(m->ciaddr, uip_hostaddr.u16, sizeof(m->ciaddr));
  memset(m->yiaddr, 0, sizeof(m->yiaddr));
  memset(m->siaddr, 0, sizeof(m->siaddr));
  memset(m->giaddr, 0, sizeof(m->giaddr));
  memcpy(m->chaddr, s.mac_addr, s.mac_len);
  memset(&m->chaddr[s.mac_len], 0, sizeof(m->chaddr) - s.mac_len);
#ifndef UIP_CONF_DHCP_LIGHT
  memset(m->sname, 0, sizeof(m->sname));
  memset(m->file, 0, sizeof(m->file));
#endif

  memcpy(m->options, magic_cookie, sizeof(magic_cookie));
}
/*---------------------------------------------------------------------------*/
static void
send_discover(void)
{
  uint8_t *end;
  struct dhcp_msg *m;
  struct net_buf *buf;
  int len;

  buf = ip_buf_get_reserve_tx(UIP_LLH_LEN + UIP_IPUDPH_LEN);
  if(!buf) {
    PRINTF("dhcp: failed to get buffer\n");
    return;
  }

  uip_set_udp_conn(buf) = s.conn;
  m = (struct dhcp_msg *)uip_appdata(buf);

  create_msg(m);

  end = add_msg_type(&m->options[4], DHCPDISCOVER);
  end = add_req_options(end);
  end = add_end(end);
  len = (int)(end - (uint8_t *)uip_appdata(buf));
  uip_appdatalen(buf) = len;

  uip_send_udp(buf, uip_appdata(buf), len);
}
/*---------------------------------------------------------------------------*/
static void
send_request(void)
{
  uint8_t *end;
  struct dhcp_msg *m;
  struct net_buf *buf;
  int len;

  buf = ip_buf_get_reserve_tx(UIP_LLH_LEN + UIP_IPUDPH_LEN);
  if(!buf) {
    PRINTF("dhcp: failed to get buffer\n");
    return;
  }

  uip_set_udp_conn(buf) = s.conn;
  m = (struct dhcp_msg *)uip_appdata(buf);

  create_msg(m);
  
  end = add_msg_type(&m->options[4], DHCPREQUEST);
  end = add_server_id(end);
  end = add_req_ipaddr(end);
  end = add_end(end);
  len = (int)(end - (uint8_t *)uip_appdata(buf));
  uip_appdatalen(buf) = len;
  
  uip_send_udp(buf, uip_appdata(buf), len);
}
/*---------------------------------------------------------------------------*/
static uint8_t
parse_options(uint8_t *optptr, int len)
{
  uint8_t *end = optptr + len;
  uint8_t type = 0;

  while(optptr < end) {
    switch(*optptr) {
    case DHCP_OPTION_SUBNET_MASK:
      memcpy(s.netmask.u16, optptr + 2, 4);
      break;
    case DHCP_OPTION_ROUTER:
      memcpy(s.default_router.u16, optptr + 2, 4);
      break;
    case DHCP_OPTION_DNS_SERVER:
      memcpy(s.dnsaddr.u16, optptr + 2, 4);
      break;
    case DHCP_OPTION_MSG_TYPE:
      type = *(optptr + 2);
      break;
    case DHCP_OPTION_SERVER_ID:
      memcpy(s.serverid, optptr + 2, 4);
      break;
    case DHCP_OPTION_LEASE_TIME:
      memcpy(s.lease_time, optptr + 2, 4);
      break;
    case DHCP_OPTION_END:
      return type;
    }

    optptr += optptr[1] + 2;
  }
  return type;
}
/*---------------------------------------------------------------------------*/
static uint8_t
parse_msg(struct net_buf *buf)
{
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata(buf);
  uint8_t ret = 0;
  
  if(m->op == DHCP_REPLY &&
     memcmp(m->xid, &xid, sizeof(xid)) == 0 &&
     memcmp(m->chaddr, s.mac_addr, s.mac_len) == 0) {
    memcpy(s.ipaddr.u16, m->yiaddr, 4);
    ret = parse_options(&m->options[4], uip_datalen(buf));
  }

  ip_buf_unref(buf);
  return ret;
}
/*---------------------------------------------------------------------------*/
/*
 * Is this a "fresh" reply for me? If it is, return the type.
 */
static int
msg_for_me(struct net_buf *buf)
{
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata(buf);
  uint8_t *optptr = &m->options[4];
  uint8_t *end = (uint8_t*)uip_appdata(buf) + uip_datalen(buf);

  if(m->op == DHCP_REPLY &&
     memcmp(m->xid, &xid, sizeof(xid)) == 0 &&
     memcmp(m->chaddr, s.mac_addr, s.mac_len) == 0) {
    while(optptr < end) {
      if(*optptr == DHCP_OPTION_MSG_TYPE) {
	return *(optptr + 2);
      } else if (*optptr == DHCP_OPTION_END) {
	return -1;
      }
      optptr += optptr[1] + 2;
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
PROCESS(dhcp_process, "DHCP process");
PROCESS_THREAD(dhcp_process, ev, data, buf, user_data)
{
 clock_time_t ticks;
 PROCESS_BEGIN();

 init:
  xid++;
  s.state = STATE_SENDING;
  s.ticks = CLOCK_SECOND * 8;
  while (1) {
    send_discover();
    etimer_set(&s.etimer, s.ticks, &dhcp_process);
    do {
      PROCESS_YIELD();
      if(ev == tcpip_event && uip_newdata(buf) && msg_for_me(buf) == DHCPOFFER) {
	parse_msg(buf);
	s.state = STATE_OFFER_RECEIVED;
	goto selecting;
      }
    } while (!etimer_expired(&s.etimer));

    if(s.ticks < CLOCK_SECOND * 60) {
      s.ticks *= 2;
    }
  }
  
 selecting:
  s.ticks = CLOCK_SECOND;
  do {
    send_request();
    etimer_set(&s.etimer, s.ticks, &dhcp_process);
    do {
      PROCESS_YIELD();
      if(ev == tcpip_event && uip_newdata(buf) && msg_for_me(buf) == DHCPACK) {
	parse_msg(buf);
	s.state = STATE_CONFIG_RECEIVED;
	goto bound;
      }
    } while (!etimer_expired(&s.etimer));

    if(s.ticks <= CLOCK_SECOND * 10) {
      s.ticks += CLOCK_SECOND;
    } else {
      goto init;
    }
  } while(s.state != STATE_CONFIG_RECEIVED);
  
 bound:

  PRINTF("Got IP address %d.%d.%d.%d\n", uip_ipaddr_to_quad(&s.ipaddr));
  PRINTF("Got netmask %d.%d.%d.%d\n",	 uip_ipaddr_to_quad(&s.netmask));
  PRINTF("Got DNS server %d.%d.%d.%d\n", uip_ipaddr_to_quad(&s.dnsaddr));
  PRINTF("Got default router %d.%d.%d.%d\n",
	 uip_ipaddr_to_quad(&s.default_router));
  PRINTF("Lease expires in %ld seconds\n",
	 uip_ntohs(s.lease_time[0])*65536ul + uip_ntohs(s.lease_time[1]));

  configure_dhcpc(&s);
  
#define MAX_TICKS (~((clock_time_t)0) / 2)
#define MAX_TICKS32 (~((uint32_t)0))
#define IMIN(a, b) ((a) < (b) ? (a) : (b))

  if((uip_ntohs(s.lease_time[0])*65536ul + uip_ntohs(s.lease_time[1]))*CLOCK_SECOND/2
     <= MAX_TICKS32) {
    s.ticks = (uip_ntohs(s.lease_time[0])*65536ul + uip_ntohs(s.lease_time[1])
	       )*CLOCK_SECOND/2;
  } else {
    s.ticks = MAX_TICKS32;
  }

  while(s.ticks > 0) {
    ticks = IMIN(s.ticks, MAX_TICKS);
    s.ticks -= ticks;
    etimer_set(&s.etimer, ticks, &dhcp_process);
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_TIMER);
  }

  if((uip_ntohs(s.lease_time[0])*65536ul + uip_ntohs(s.lease_time[1]))*CLOCK_SECOND/2
     <= MAX_TICKS32) {
    s.ticks = (uip_ntohs(s.lease_time[0])*65536ul + uip_ntohs(s.lease_time[1])
	       )*CLOCK_SECOND/2;
  } else {
    s.ticks = MAX_TICKS32;
  }

  /* renewing: */
  do {
    send_request();
    ticks = IMIN(s.ticks / 2, MAX_TICKS);
    s.ticks -= ticks;
    etimer_set(&s.etimer, ticks, &dhcp_process);
    do {
      PROCESS_YIELD();
      if(ev == tcpip_event && uip_newdata(buf) && msg_for_me(buf) == DHCPACK) {
	parse_msg(buf);
	goto bound;
      }
    } while(!etimer_expired(&s.etimer));
  } while(s.ticks >= CLOCK_SECOND*3);

  /* rebinding: */

  /* lease_expired: */
  unconfigure_dhcpc(&s);
  goto init;

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
dhcpc_init(const void *mac_addr, int mac_len)
{
  uip_ipaddr_t addr;
  uip_ipaddr_t ipaddr;

  s.mac_addr = mac_addr;
  s.mac_len  = mac_len;

  s.state = STATE_INITIAL;

  uip_ipaddr(&ipaddr, 0,0,0,0);
  uip_sethostaddr(&ipaddr);

  uip_ipaddr(&addr, 255,255,255,255);
  s.conn = udp_new(&addr, UIP_HTONS(DHCPC_SERVER_PORT), NULL);
  if(s.conn != NULL) {
    udp_bind(s.conn, UIP_HTONS(DHCPC_CLIENT_PORT));
  }
  process_start(&dhcp_process, NULL, NULL);
}

/*---------------------------------------------------------------------------*/
void
dhcpc_appcall(process_event_t ev, struct net_buf *buf)
{
  if(ev == tcpip_event || ev == PROCESS_EVENT_TIMER) {
    process_post_synch(&dhcp_process, ev, NULL, buf);
  }
}
/*---------------------------------------------------------------------------*/
int msg_for_dhcpc(struct net_buf *buf)
{
  int msg;

  msg = msg_for_me(buf);
  if(msg == DHCPOFFER || msg == DHCPACK)
     return 1;

  return 0;
}
/*---------------------------------------------------------------------------*/
void dhcpc_set_callbacks(dhcpc_configured conf, dhcpc_unconfigured unconf)
{
	configure_cb = conf;
	unconfigure_cb = unconf;
}
/*---------------------------------------------------------------------------*/
