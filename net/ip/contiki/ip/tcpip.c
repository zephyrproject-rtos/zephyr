/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 *         Code for tunnelling uIP packets over the Rime mesh routing module
 *
 * \author  Adam Dunkels <adam@sics.se>\author
 * \author  Mathilde Durvy <mdurvy@cisco.com> (IPv6 related code)
 * \author  Julien Abeille <jabeille@cisco.com> (IPv6 related code)
 */

#include <net/ip_buf.h>

#include "contiki-net.h"
#include "contiki/ip/uip-split.h"
#include "contiki/ip/uip-packetqueue.h"

#if NETSTACK_CONF_WITH_IPV6
#include "contiki/ipv6/uip-nd6.h"
#include "contiki/ipv6/uip-ds6.h"
#endif

#include <string.h>

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_RECV_SEND
#define TCPIP_CONF_ANNOTATE_TRANSMISSIONS 1
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#ifdef CONFIG_DHCP
#include "contiki/ip/dhcpc.h"
#endif

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

#define UIP_ICMP_BUF(buf) ((struct uip_icmp_hdr *)&uip_buf(buf)[UIP_LLIPH_LEN + uip_ext_len(buf)])
#define UIP_IP_BUF(buf) ((struct uip_ip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])
#define UIP_TCP_BUF(buf) ((struct uip_tcpip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])

#ifdef UIP_FALLBACK_INTERFACE
extern struct uip_fallback_interface UIP_FALLBACK_INTERFACE;
#endif

#if UIP_CONF_IPV6_RPL
#include "rpl/rpl.h"
#endif

process_event_t tcpip_event;
#if UIP_CONF_ICMP6
process_event_t tcpip_icmp6_event;
#endif /* UIP_CONF_ICMP6 */

/* Periodic check of active connections. */
static struct etimer periodic;

#if NETSTACK_CONF_WITH_IPV6 && UIP_CONF_IPV6_REASSEMBLY
/* Timer for reassembly. */
extern struct etimer uip_reass_timer;
#endif

#if UIP_TCP
/**
 * \internal Structure for holding a TCP port and a process ID.
 */
struct listenport {
  uint16_t port;
  struct process *p;
};

static struct internal_state {
  struct listenport listenports[UIP_LISTENPORTS];
  struct process *p;
} s;
#endif

enum {
  TCP_POLL,
  UDP_POLL,
  PACKET_INPUT
};

/* Called on IP packet output. */

static uint8_t (* outputfunc)(struct net_buf *buf, const uip_lladdr_t *a);

uint8_t
tcpip_output(struct net_buf *buf, const uip_lladdr_t *a)
{
  if(outputfunc != NULL) {
    return outputfunc(buf, a);
  }
  UIP_LOG("tcpip_output: Use tcpip_set_outputfunc() to set an output function");
  return 0;
}

void
tcpip_set_outputfunc(uint8_t (*f)(struct net_buf *buf, const uip_lladdr_t *))
{
  outputfunc = f;
}

#if UIP_CONF_IP_FORWARD
unsigned char tcpip_is_forwarding; /* Forwarding right now? */
#endif /* UIP_CONF_IP_FORWARD */

struct net_buf;

PROCESS(tcpip_process, "TCP/IP stack");

/*---------------------------------------------------------------------------*/
#if UIP_TCP || UIP_CONF_IP_FORWARD
static void
start_periodic_tcp_timer(void)
{
  if(etimer_expired(&periodic)) {
    etimer_restart(&periodic);
  }
}
#endif /* UIP_TCP || UIP_CONF_IP_FORWARD */

/*---------------------------------------------------------------------------*/
static void
check_for_tcp_syn(struct net_buf *buf)
{
#if UIP_TCP || UIP_CONF_IP_FORWARD
  /* This is a hack that is needed to start the periodic TCP timer if
     an incoming packet contains a SYN: since uIP does not inform the
     application if a SYN arrives, we have no other way of starting
     this timer.  This function is called for every incoming IP packet
     to check for such SYNs. */
#define TCP_SYN 0x02
  if(UIP_IP_BUF(buf)->proto == UIP_PROTO_TCP &&
     (UIP_TCP_BUF(buf)->flags & TCP_SYN) == TCP_SYN) {
    start_periodic_tcp_timer();
  }
#endif /* UIP_TCP || UIP_CONF_IP_FORWARD */
}
/*---------------------------------------------------------------------------*/
static uint8_t
packet_input(struct net_buf *buf)
{
  uint8_t ret = 0;
#if UIP_CONF_IP_FORWARD
  if(uip_len > 0) {
    tcpip_is_forwarding = 1;
    if(uip_fw_forward() == UIP_FW_LOCAL) {
      tcpip_is_forwarding = 0;
      check_for_tcp_syn();
      uip_input();
      if(uip_len > 0) {
#if UIP_CONF_TCP_SPLIT
        uip_split_output();
#else /* UIP_CONF_TCP_SPLIT */
#if NETSTACK_CONF_WITH_IPV6
        tcpip_ipv6_output();
#else
	PRINTF("tcpip packet_input forward output len %d\n", uip_len);
        tcpip_output();
#endif
#endif /* UIP_CONF_TCP_SPLIT */
      }
    }
    tcpip_is_forwarding = 0;
  }
#else /* UIP_CONF_IP_FORWARD */
  if(uip_len(buf) > 0) {
    check_for_tcp_syn(buf);
    ret = uip_input(buf);
    if(ret && uip_len(buf) > 0) {
#if UIP_CONF_TCP_SPLIT
      uip_split_output(buf);
#else /* UIP_CONF_TCP_SPLIT */
#if NETSTACK_CONF_WITH_IPV6
      PRINTF("tcpip packet_input output len %d\n", uip_len(buf));
      ret = tcpip_ipv6_output(buf);
#else
      PRINTF("tcpip packet_input output len %d\n", uip_len(buf));
      ret = tcpip_output(buf, NULL);
#endif
#endif /* UIP_CONF_TCP_SPLIT */
    }
  }
#endif /* UIP_CONF_IP_FORWARD */
  return ret;
}
/*---------------------------------------------------------------------------*/
#if UIP_TCP
#if UIP_ACTIVE_OPEN
struct uip_conn *
tcp_connect(const uip_ipaddr_t *ripaddr, uint16_t port, void *appstate,
	    struct process *process, struct net_buf *buf)
{
  struct uip_conn *c;
  
  c = uip_connect(ripaddr, port);
  if(c == NULL) {
    return NULL;
  }

  c->appstate.p = process;
  c->appstate.state = appstate;
  
  tcpip_poll_tcp(c, buf);
  
  return c;
}
#endif /* UIP_ACTIVE_OPEN */
/*---------------------------------------------------------------------------*/
void
tcp_unlisten(uint16_t port, struct process *handler)
{
  static unsigned char i;
  struct listenport *l;

  l = s.listenports;
  for(i = 0; i < UIP_LISTENPORTS; ++i) {
    if(l->port == port &&
       l->p == handler) {
      l->port = 0;
      uip_unlisten(port);
      break;
    }
    ++l;
  }
}
/*---------------------------------------------------------------------------*/
void
tcp_listen(uint16_t port, struct process *handler)
{
  static unsigned char i;
  struct listenport *l;

  l = s.listenports;
  for(i = 0; i < UIP_LISTENPORTS; ++i) {
    if(l->port == 0) {
      l->port = port;
      l->p = handler;
      uip_listen(port);
      break;
    }
    ++l;
  }
}
/*---------------------------------------------------------------------------*/
void
tcp_attach(struct uip_conn *conn,
	   void *appstate)
{
  uip_tcp_appstate_t *s;

  s = &conn->appstate;
  s->p = PROCESS_CURRENT();
  s->state = appstate;
}

#endif /* UIP_TCP */
/*---------------------------------------------------------------------------*/
#if UIP_UDP
void
udp_attach(struct uip_udp_conn *conn,
	   void *appstate)
{
  uip_udp_appstate_t *s;

  s = &conn->appstate;
  s->p = PROCESS_CURRENT();
  s->state = appstate;
}
/*---------------------------------------------------------------------------*/
struct uip_udp_conn *
udp_new(const uip_ipaddr_t *ripaddr, uint16_t port, void *appstate)
{
  struct uip_udp_conn *c;
  uip_udp_appstate_t *s;
  
  c = uip_udp_new(ripaddr, port);
  if(c == NULL) {
    return NULL;
  }

  s = &c->appstate;
  s->p = PROCESS_CURRENT();
  s->state = appstate;

  return c;
}
/*---------------------------------------------------------------------------*/
struct uip_udp_conn *
udp_broadcast_new(uint16_t port, void *appstate)
{
  uip_ipaddr_t addr;
  struct uip_udp_conn *conn;

#if NETSTACK_CONF_WITH_IPV6
  uip_create_linklocal_allnodes_mcast(&addr);
#else
  uip_ipaddr(&addr, 255,255,255,255);
#endif /* NETSTACK_CONF_WITH_IPV6 */
  conn = udp_new(&addr, port, appstate);
  if(conn != NULL) {
    udp_bind(conn, port);
  }
  return conn;
}
#endif /* UIP_UDP */
/*---------------------------------------------------------------------------*/
#if UIP_CONF_ICMP6
uint8_t
icmp6_new(void *appstate) {
  if(uip_icmp6_conns.appstate.p == PROCESS_NONE) {
    uip_icmp6_conns.appstate.p = PROCESS_CURRENT();
    uip_icmp6_conns.appstate.state = appstate;
    return 0;
  }
  return 1;
}

void
tcpip_icmp6_call(uint8_t type)
{
  if(uip_icmp6_conns.appstate.p != PROCESS_NONE) {
    /* XXX: This is a hack that needs to be updated. Passing a pointer (&type)
       like this only works with process_post_synch. */
    process_post_synch(uip_icmp6_conns.appstate.p, tcpip_icmp6_event, &type,
		       NULL);
  }
  return;
}
#endif /* UIP_CONF_ICMP6 */
/*---------------------------------------------------------------------------*/
static void
eventhandler(process_event_t ev, process_data_t data, struct net_buf *buf)
{
#if UIP_TCP
  static unsigned char i;
  register struct listenport *l;
#endif /*UIP_TCP*/
  struct process *p;

  switch(ev) {
    case PROCESS_EVENT_EXITED:
      /* This is the event we get if a process has exited. We go through
         the TCP/IP tables to see if this process had any open
         connections or listening TCP ports. If so, we'll close those
         connections. */

      p = (struct process *)data;
#if UIP_TCP
      l = s.listenports;
      for(i = 0; i < UIP_LISTENPORTS; ++i) {
        if(l->p == p) {
          uip_unlisten(l->port);
          l->port = 0;
          l->p = PROCESS_NONE;
        }
        ++l;
      }
	 
      {
        struct uip_conn *cptr;
	    
        for(cptr = &uip_conns[0]; cptr < &uip_conns[UIP_CONNS]; ++cptr) {
          if(cptr->appstate.p == p) {
            cptr->appstate.p = PROCESS_NONE;
            cptr->tcpstateflags = UIP_CLOSED;
          }
        }
      }
#endif /* UIP_TCP */
#if UIP_UDP
      {
        struct uip_udp_conn *cptr;

        for(cptr = &uip_udp_conns[0];
            cptr < &uip_udp_conns[UIP_UDP_CONNS]; ++cptr) {
          if(cptr->appstate.p == p) {
            cptr->lport = 0;
          }
        }
      }
#endif /* UIP_UDP */
      break;

    case PROCESS_EVENT_TIMER:
      /* We get this event if one of our timers have expired. */
      {
        if(!buf)
          break;
        /* Check the clock so see if we should call the periodic uIP
           processing. */
        if(data == &periodic &&
           !etimer_is_triggered(&periodic)) {
	  etimer_set_triggered(&periodic);
#if UIP_TCP
          for(i = 0; i < UIP_CONNS; ++i) {
            if(uip_conn_active(i)) {
              /* Only restart the timer if there are active
                 connections. */
              etimer_restart(&periodic);
              uip_periodic(buf, i);
#if NETSTACK_CONF_WITH_IPV6
	      tcpip_ipv6_output(buf);
#else
              if(uip_len(buf) > 0) {
		PRINTF("tcpip_output from periodic len %d\n", uip_len(buf));
                tcpip_output(buf, NULL);
		PRINTF("tcpip_output after periodic len %d\n", uip_len(buf));
              }
#endif /* NETSTACK_CONF_WITH_IPV6 */
            }
          }
#endif /* UIP_TCP */
#if UIP_CONF_IP_FORWARD
          uip_fw_periodic();
#endif /* UIP_CONF_IP_FORWARD */
        }
        
#if NETSTACK_CONF_WITH_IPV6
#if UIP_CONF_IPV6_REASSEMBLY
        /*
         * check the timer for reassembly
         */
        if(data == &uip_reass_timer &&
           !etimer_is_triggered(&uip_reass_timer)) {
	  etimer_set_triggered(&uip_reass_timer);
          uip_reass_over();
          tcpip_ipv6_output(buf);
        }
#endif /* UIP_CONF_IPV6_REASSEMBLY */
        /*
         * check the different timers for neighbor discovery and
         * stateless autoconfiguration
         */
        /*if(data == &uip_ds6_timer_periodic &&
           etimer_expired(&uip_ds6_timer_periodic)) {
          uip_ds6_periodic();
          tcpip_ipv6_output();
        }*/
#if !UIP_CONF_ROUTER
        if(data == &uip_ds6_timer_rs &&
           !etimer_is_triggered(&uip_ds6_timer_rs)) {
	  etimer_set_triggered(&uip_ds6_timer_rs);
          uip_ds6_send_rs(buf);
	  tcpip_ipv6_output(buf);
        }
#endif /* !UIP_CONF_ROUTER */
        if(data == &uip_ds6_timer_periodic &&
           !etimer_is_triggered(&uip_ds6_timer_periodic)) {
	  etimer_set_triggered(&uip_ds6_timer_periodic);
          uip_ds6_periodic(buf);
	  tcpip_ipv6_output(buf);
        }
#endif /* NETSTACK_CONF_WITH_IPV6 */
      }
      break;
	 
#if UIP_TCP
    case TCP_POLL:
      if(data != NULL) {
        uint8_t ret = 0;
        uip_poll_conn(buf, data);
#if NETSTACK_CONF_WITH_IPV6
	if (!uip_len(buf) && buf->len) {
          /* Make sure we are sending something if needed. */
          uip_len(buf) = buf->len;
	}
        ret = tcpip_ipv6_output(buf);
#else /* NETSTACK_CONF_WITH_IPV6 */
        if(uip_len(buf) > 0) {
	  PRINTF("tcpip_output from tcp poll len %d\n", uip_len(buf));
          ret = tcpip_output(buf, NULL);
        }
#endif /* NETSTACK_CONF_WITH_IPV6 */
        /* Start the periodic polling, if it isn't already active. */
        start_periodic_tcp_timer();

	if (!ret) {
          /* Packet was not sent properly */
          ip_buf_unref(buf);
	}
      }
      break;
#endif /* UIP_TCP */
#if UIP_UDP
    case UDP_POLL:
      if(data != NULL) {
        uip_udp_periodic_conn(buf, data);
#if NETSTACK_CONF_WITH_IPV6
        tcpip_ipv6_output(buf);
#else
        if(uip_len(buf) > 0) {
          tcpip_output(buf, NULL);
        }
#endif /* UIP_UDP */
      }
      break;
#endif /* UIP_UDP */

    case PACKET_INPUT:
      if (!packet_input(buf)) {
        PRINTF("Packet %p was not sent and must be discarded by caller\n", buf);
      } else {
        PRINTF("Packet %p was sent ok\n", buf);
      }
      break;
  };
}
/*---------------------------------------------------------------------------*/
uint8_t
tcpip_input(struct net_buf *buf)
{
  process_post_synch(&tcpip_process, PACKET_INPUT, NULL, buf);
  if (uip_len(buf) == 0) {
    /* This indicates that there was a parsing/other error
     * in packet.
     */
    return 0;
  }

  uip_len(buf) = 0;
#if NETSTACK_CONF_WITH_IPV6
  uip_ext_len(buf) = 0;
#endif /*NETSTACK_CONF_WITH_IPV6*/

  return 1;
}
/*---------------------------------------------------------------------------*/
#if NETSTACK_CONF_WITH_IPV6
uint8_t
tcpip_ipv6_output(struct net_buf *buf)
{
  uip_ds6_nbr_t *nbr = NULL;
  uip_ipaddr_t *nexthop;
  uint8_t ret = 0; /* return value 0 == failed, 1 == ok */

  if(!buf || uip_len(buf) == 0) {
    return 0;
  }

  PRINTF("%s(): buf %p len %d\n", __FUNCTION__, buf, uip_len(buf));

  if(uip_len(buf) > UIP_LINK_MTU) {
    UIP_LOG("tcpip_ipv6_output: Packet too big");
    uip_len(buf) = 0;
    uip_ext_len(buf) = 0;
    return 0;
  }

  if(uip_is_addr_unspecified(&UIP_IP_BUF(buf)->destipaddr)){
    UIP_LOG("tcpip_ipv6_output: Destination address unspecified");
    uip_len(buf) = 0;
    uip_ext_len(buf) = 0;
    return 0;
  }

  if(!uip_is_addr_mcast(&UIP_IP_BUF(buf)->destipaddr)) {
    /* Next hop determination */
    nbr = NULL;

    /* We first check if the destination address is on our immediate
       link. If so, we simply use the destination address as our
       nexthop address. */
    if(uip_ds6_is_addr_onlink(&UIP_IP_BUF(buf)->destipaddr)){
      nexthop = &UIP_IP_BUF(buf)->destipaddr;
    } else {
      uip_ds6_route_t *route;
      /* Check if we have a route to the destination address. */
      route = uip_ds6_route_lookup(&UIP_IP_BUF(buf)->destipaddr);

      /* No route was found - we send to the default route instead. */
      if(route == NULL) {
        PRINTF("tcpip_ipv6_output: no route found, using default route\n");
        nexthop = uip_ds6_defrt_choose();
        if(nexthop == NULL) {
#ifdef UIP_FALLBACK_INTERFACE
	  PRINTF("FALLBACK: removing ext hdrs & setting proto %d %d\n", 
		 uip_ext_len(buf), *((uint8_t *)UIP_IP_BUF(buf) + 40));
	  if(uip_ext_len(buf) > 0) {
	    extern void remove_ext_hdr(void);
	    uint8_t proto = *((uint8_t *)UIP_IP_BUF(buf) + 40);
	    remove_ext_hdr();
	    /* This should be copied from the ext header... */
	    UIP_IP_BUF(buf)->proto = proto;
	  }
	  UIP_FALLBACK_INTERFACE.output();
#else
          PRINTF("tcpip_ipv6_output: Destination off-link but no route\n");
#endif /* !UIP_FALLBACK_INTERFACE */
          uip_len(buf) = 0;
          uip_ext_len(buf) = 0;
          return 0;
        }

      } else {
        /* A route was found, so we look up the nexthop neighbor for
           the route. */
        nexthop = uip_ds6_route_nexthop(route);

        /* If the nexthop is dead, for example because the neighbor
           never responded to link-layer acks, we drop its route. */
        if(nexthop == NULL) {
#if UIP_CONF_IPV6_RPL
          /* If we are running RPL, and if we are the root of the
             network, we'll trigger a global repair berfore we remove
             the route. */
          rpl_dag_t *dag;
          rpl_instance_t *instance;

          dag = (rpl_dag_t *)route->state.dag;
          if(dag != NULL) {
            instance = dag->instance;

            rpl_repair_root(instance->instance_id);
          }
#endif /* UIP_CONF_IPV6_RPL */
          uip_ds6_route_rm(route);

          /* We don't have a nexthop to send the packet to, so we drop
             it. */
          return 0;
        }
      }
#if TCPIP_CONF_ANNOTATE_TRANSMISSIONS
      if(nexthop != NULL) {
        static uint8_t annotate_last;
        static uint8_t annotate_has_last = 0;

        if(annotate_has_last) {
          printf("#L %u 0; red\n", annotate_last);
        }
        printf("#L %u 1; red\n", nexthop->u8[sizeof(uip_ipaddr_t) - 1]);
        annotate_last = nexthop->u8[sizeof(uip_ipaddr_t) - 1];
        annotate_has_last = 1;
      }
#endif /* TCPIP_CONF_ANNOTATE_TRANSMISSIONS */
    }

    /* End of next hop determination */

#if UIP_CONF_IPV6_RPL
    if(rpl_update_header_final(buf, nexthop)) {
      uip_len(buf) = 0;
      uip_ext_len(buf) = 0;
      return 0;
    }
#endif /* UIP_CONF_IPV6_RPL */
    nbr = uip_ds6_nbr_lookup(nexthop);
    if(nbr == NULL) {
#if UIP_ND6_SEND_NA
      if((nbr = uip_ds6_nbr_add(nexthop, NULL, 0, NBR_INCOMPLETE)) == NULL) {
	PRINTF("IP packet buf %p len %d discarded because cannot "
	       "add neighbor\n", buf, uip_len(buf));
        uip_len(buf) = 0;
        uip_ext_len(buf) = 0;
        return 0;
      } else {
#if UIP_CONF_IPV6_QUEUE_PKT
        /* Copy outgoing pkt in the queuing buffer for later transmit. */
        if(uip_packetqueue_alloc(buf, &nbr->packethandle, UIP_DS6_NBR_PACKET_LIFETIME) != NULL) {
          memcpy(uip_packetqueue_buf(&nbr->packethandle), UIP_IP_BUF(buf), uip_len(buf));
          uip_packetqueue_set_buflen(&nbr->packethandle, uip_len(buf));
        }
#else
	PRINTF("IP packet buf %p len %d discarded because NS is "
	       "being sent\n", buf, uip_len(buf));
#endif
      /* RFC4861, 7.2.2:
       * "If the source address of the packet prompting the solicitation is the
       * same as one of the addresses assigned to the outgoing interface, that
       * address SHOULD be placed in the IP Source Address of the outgoing
       * solicitation.  Otherwise, any one of the addresses assigned to the
       * interface should be used."*/
       if(uip_ds6_is_my_addr(&UIP_IP_BUF(buf)->srcipaddr)){
          uip_nd6_ns_output(NULL, &UIP_IP_BUF(buf)->srcipaddr, NULL, &nbr->ipaddr);
        } else {
          uip_nd6_ns_output(NULL, NULL, NULL, &nbr->ipaddr);
        }

        stimer_set(&nbr->sendns, uip_ds6_if.retrans_timer / 1000);
        nbr->nscount = 1;
      }

      uip_len(buf) = 0;
      uip_ext_len(buf) = 0;
      return 0; /* packet was discarded */
#else /* UIP_ND6_SEND_NA */
      int uiplen = uip_len(buf);
      int extlen = uip_ext_len(buf);

      /* Neighbor discovery is not there. Just try to send the packet. */
      ret = tcpip_output(buf, NULL);
      if (ret) {
        /* We must set the length back as these were overwritten
	 * by other part of the stack. If we do not do this then
	 * there will be a double memory free in the caller.
	 * FIXME properly later!
	 */
        uip_len(buf) = uiplen;
	uip_ext_len(buf) = extlen;
      } else {
        uip_len(buf) = 0;
        uip_ext_len(buf) = 0;
      }
      return ret;

#endif /* UIP_ND6_SEND_NA */
    } else {
#if UIP_ND6_SEND_NA
      if(nbr->state == NBR_INCOMPLETE) {
        PRINTF("tcpip_ipv6_output: nbr cache entry incomplete\n");
#if UIP_CONF_IPV6_QUEUE_PKT
        /* Copy outgoing pkt in the queuing buffer for later transmit and set
           the destination nbr to nbr. */
        if(uip_packetqueue_alloc(buf, &nbr->packethandle, UIP_DS6_NBR_PACKET_LIFETIME) != NULL) {
          memcpy(uip_packetqueue_buf(&nbr->packethandle), UIP_IP_BUF(buf), uip_len(buf));
          uip_packetqueue_set_buflen(&nbr->packethandle, uip_len(buf));
        } else {
          PRINTF("IP packet buf %p len %d discarded because no space "
                 "in the queue\n", buf, uip_len(buf));
        }
#else
	PRINTF("IP packet buf %p len %d discarded because neighbor info is "
	       "not yet received\n", buf, uip_len(buf));
#endif /*UIP_CONF_IPV6_QUEUE_PKT*/
        uip_len(buf) = 0;
        uip_ext_len(buf) = 0;
        return 0;
      }
      /* Send in parallel if we are running NUD (nbc state is either STALE,
         DELAY, or PROBE). See RFC 4861, section 7.3.3 on node behavior. */
      if(nbr->state == NBR_STALE) {
        nbr->state = NBR_DELAY;
        stimer_set(&nbr->reachable, UIP_ND6_DELAY_FIRST_PROBE_TIME);
        nbr->nscount = 0;
        PRINTF("tcpip_ipv6_output: nbr cache entry stale moving to delay\n");
      }
#endif /* UIP_ND6_SEND_NA */

      ret = tcpip_output(buf, uip_ds6_nbr_get_ll(nbr));

#if UIP_CONF_IPV6_QUEUE_PKT
      /*
       * Send the queued packets from here, may not be 100% perfect though.
       * This happens in a few cases, for example when instead of receiving a
       * NA after sendiong a NS, you receive a NS with SLLAO: the entry moves
       * to STALE, and you must both send a NA and the queued packet.
       */
      if(uip_packetqueue_buflen(&nbr->packethandle) != 0) {
        bool allocated_here = false;
        if (ret != 0) {
	  /* The IP buf was freed because the send succeed so we need to
           * allocate a new one here.
           */
          buf = ip_buf_get_reserve_tx(0);
          if (!buf) {
            PRINTF("%s(): Cannot send queued packet, no net buffers\n", __FUNCTION__);
            uip_packetqueue_free(&nbr->packethandle);
	    goto no_buf;
	  }
          allocated_here = true;
        }

        uip_len(buf) = buf->len = uip_packetqueue_buflen(&nbr->packethandle);
        memcpy(UIP_IP_BUF(buf), uip_packetqueue_buf(&nbr->packethandle),
               uip_len(buf));
        uip_packetqueue_free(&nbr->packethandle);
        ret = tcpip_output(buf, uip_ds6_nbr_get_ll(nbr));
        if (allocated_here && !ret) {
          /* There was a sending error and the buffer was not released.
           * We cannot return the buffer to upper layers so just release
           * it here.
           */
          ip_buf_unref(buf);
          ret = 1; /* This will tell caller that buf is released. */
        }
      }
    no_buf:
#endif /*UIP_CONF_IPV6_QUEUE_PKT*/

      if (ret == 0) {
        uip_len(buf) = 0;
        uip_ext_len(buf) = 0;
      }

      return ret;
    }
    return 0; /* discard packet */
  }
  /* Multicast IP destination address. */
  ret = tcpip_output(buf, NULL);
  uip_len(buf) = 0;
  uip_ext_len(buf) = 0;
  return ret;
}
#endif /* NETSTACK_CONF_WITH_IPV6 */
/*---------------------------------------------------------------------------*/
#if UIP_UDP
void
tcpip_poll_udp(struct uip_udp_conn *conn)
{
  process_post(&tcpip_process, UDP_POLL, conn);
}
#endif /* UIP_UDP */
/*---------------------------------------------------------------------------*/
#if UIP_TCP
void
tcpip_poll_tcp(struct uip_conn *conn, struct net_buf *data_buf)
{
  /* We are sending here the initial SYN */
  struct net_buf *buf = ip_buf_get_tx(conn->appstate.state);
  uip_set_conn(data_buf) = conn;

  /* The conn->buf will be freed after we have established the connection,
   * sent the message and received an ack to it. This will happen in
   * net_core.c:net_send().
   */
  conn->buf = ip_buf_ref(data_buf);

  process_post_synch(&tcpip_process, TCP_POLL, conn, buf);
}

#if UIP_ACTIVE_OPEN
void tcpip_resend_syn(struct uip_conn *conn, struct net_buf *buf)
{
  /* The network driver will unref the buf so in order not to loose the
   * buffer, we need to ref it here.
   */
  ip_buf_ref(buf);

  /* We are re-sending here the SYN */
  process_post_synch(&tcpip_process, TCP_POLL, conn, buf);
}
#endif /* UIP_ACTIVE_OPEN */
#endif /* UIP_TCP */
/*---------------------------------------------------------------------------*/
void
tcpip_uipcall(struct net_buf *buf)
{
  struct tcpip_uipstate *ts;
  
#if UIP_UDP
  if(uip_conn(buf) != NULL) {
    ts = &uip_conn(buf)->appstate;
  } else {
    ts = &uip_udp_conn(buf)->appstate;
  }
#else /* UIP_UDP */
  ts = &uip_conn(buf)->appstate;
#endif /* UIP_UDP */

#if UIP_TCP
 {
   static unsigned char i;
   struct listenport *l;
   
   /* If this is a connection request for a listening port, we must
      mark the connection with the right process ID. */
   if(uip_connected(buf)) {
     l = &s.listenports[0];
     for(i = 0; i < UIP_LISTENPORTS; ++i) {
       if(l->port == uip_conn(buf)->lport &&
	  l->p != PROCESS_NONE) {
	 ts->p = l->p;
	 ts->state = NULL;
	 break;
       }
       ++l;
     }
     
     /* Start the periodic polling, if it isn't already active. */
     start_periodic_tcp_timer();
   }
 }
#endif /* UIP_TCP */

#ifdef CONFIG_DHCP
  if(msg_for_dhcpc(buf)) {
    PRINTF("msg for dhcpc\n");
    dhcpc_appcall(tcpip_event, buf);
    return;
  }
#endif

  if(ts->p != NULL) {
    process_post_synch(ts->p, tcpip_event, ts->state, buf);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tcpip_process, ev, data, buf, user_data)
{
  PROCESS_BEGIN();
  
#if UIP_TCP
 {
   static unsigned char i;
   
   for(i = 0; i < UIP_LISTENPORTS; ++i) {
     s.listenports[i].port = 0;
   }
   s.p = PROCESS_CURRENT();
 }
#endif

  tcpip_event = process_alloc_event();
#if UIP_CONF_ICMP6
  tcpip_icmp6_event = process_alloc_event();
#endif /* UIP_CONF_ICMP6 */
  etimer_set(&periodic, CLOCK_SECOND / 2, &tcpip_process);

  uip_init();
#ifdef UIP_FALLBACK_INTERFACE
  UIP_FALLBACK_INTERFACE.init();
#endif
/* initialize RPL if configured for using RPL */
#if NETSTACK_CONF_WITH_IPV6 && UIP_CONF_IPV6_RPL
  rpl_init();
#endif /* UIP_CONF_IPV6_RPL */

  while(1) {
    PROCESS_YIELD();
    eventhandler(ev, data, buf);
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
