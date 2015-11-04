/*
 * Copyright (c) 2010, Loughborough University - Computer Science
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
 * \addtogroup smrf-multicast
 * @{
 */
/**
 * \file
 *    This file implements 'Stateless Multicast RPL Forwarding' (SMRF)
 *
 * \author
 *    George Oikonomou - <oikonomou@users.sourceforge.net>
 */

#include <net/ip_buf.h>

#include "contiki.h"
#include "contiki-net.h"
#include "contiki/ipv6/multicast/uip-mcast6.h"
#include "contiki/ipv6/multicast/uip-mcast6-route.h"
#include "contiki/ipv6/multicast/uip-mcast6-stats.h"
#include "contiki/ipv6/multicast/smrf.h"
#include "contiki/rpl/rpl.h"
#include "contiki/netstack.h"
#include "contiki/os/lib/random.h"
#include <string.h>

#define DEBUG DEBUG_NONE
#include "contiki/ip/uip-debug.h"

/*---------------------------------------------------------------------------*/
/* Macros */
/*---------------------------------------------------------------------------*/
/* CCI */
#define SMRF_FWD_DELAY()  NETSTACK_RDC.channel_check_interval()
/* Number of slots in the next 500ms */
#define SMRF_INTERVAL_COUNT  ((CLOCK_SECOND >> 2) / fwd_delay)
/*---------------------------------------------------------------------------*/
/* Internal Data */
/*---------------------------------------------------------------------------*/
static struct ctimer mcast_periodic;
static uint8_t fwd_delay;
static uint8_t fwd_spread;

/*---------------------------------------------------------------------------*/
/* uIPv6 Pointers */
/*---------------------------------------------------------------------------*/
#define UIP_IP_BUF(buf)        ((struct uip_ip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])
/*---------------------------------------------------------------------------*/
static void
mcast_fwd(struct net_buf *buf, void *p)
{
  UIP_IP_BUF(buf)->ttl--;
  tcpip_output(buf, NULL);
  uip_len(buf) = 0;
  uip_ext_len(buf) = 0;
}
/*---------------------------------------------------------------------------*/
static uint8_t
in(struct net_buf *buf)
{
  rpl_dag_t *d;                 /* Our DODAG */
  uip_ipaddr_t *parent_ipaddr;  /* Our pref. parent's IPv6 address */
  const uip_lladdr_t *parent_lladdr;  /* Our pref. parent's LL address */
  struct net_buf *netbuf;

  /*
   * Fetch a pointer to the LL address of our preferred parent
   *
   * ToDo: This rpl_get_any_dag() call is a dirty replacement of the previous
   *   rpl_get_dag(RPL_DEFAULT_INSTANCE);
   * so that things can compile with the new RPL code. This needs updated to
   * read instance ID from the RPL HBHO and use the correct parent accordingly
   */
  d = rpl_get_any_dag();
  if(!d) {
    UIP_MCAST6_STATS_ADD(mcast_dropped);
    return UIP_MCAST6_DROP;
  }

  /* Retrieve our preferred parent's LL address */
  parent_ipaddr = rpl_get_parent_ipaddr(d->preferred_parent);
  parent_lladdr = uip_ds6_nbr_lladdr_from_ipaddr(parent_ipaddr);

  if(parent_lladdr == NULL) {
    UIP_MCAST6_STATS_ADD(mcast_dropped);
    return UIP_MCAST6_DROP;
  }

  /*
   * We accept a datagram if it arrived from our preferred parent, discard
   * otherwise.
   */
  if(memcmp(parent_lladdr, &ip_buf_ll_src(buf),
            UIP_LLADDR_LEN)) {
    PRINTF("SMRF: Routable in but SMRF ignored it\n");
    UIP_MCAST6_STATS_ADD(mcast_dropped);
    return UIP_MCAST6_DROP;
  }

  if(UIP_IP_BUF(buf)->ttl <= 1) {
    UIP_MCAST6_STATS_ADD(mcast_dropped);
    return UIP_MCAST6_DROP;
  }

  UIP_MCAST6_STATS_ADD(mcast_in_all);
  UIP_MCAST6_STATS_ADD(mcast_in_unique);

  /* If we have an entry in the mcast routing table, something with
   * a higher RPL rank (somewhere down the tree) is a group member */
  if(uip_mcast6_route_lookup(&UIP_IP_BUF(buf)->destipaddr)) {
    /* If we enter here, we will definitely forward */
    UIP_MCAST6_STATS_ADD(mcast_fwd);

    /*
     * Add a delay (D) of at least SMRF_FWD_DELAY() to compensate for how
     * contikimac handles broadcasts. We can't start our TX before the sender
     * has finished its own.
     */
    fwd_delay = SMRF_FWD_DELAY();

    /* Finalise D: D = min(SMRF_FWD_DELAY(), SMRF_MIN_FWD_DELAY) */
#if SMRF_MIN_FWD_DELAY
    if(fwd_delay < SMRF_MIN_FWD_DELAY) {
      fwd_delay = SMRF_MIN_FWD_DELAY;
    }
#endif

    if(fwd_delay == 0) {
      /* No delay required, send it, do it now, why wait? */
      UIP_IP_BUF(buf)->ttl--;
      tcpip_output(buf, NULL);
      UIP_IP_BUF(buf)->ttl++;        /* Restore before potential upstack delivery */
    } else {
      /* Randomise final delay in [D , D*Spread], step D */
      fwd_spread = SMRF_INTERVAL_COUNT;
      if(fwd_spread > SMRF_MAX_SPREAD) {
        fwd_spread = SMRF_MAX_SPREAD;
      }
      if(fwd_spread) {
        fwd_delay = fwd_delay * (1 + ((random_rand() >> 11) % fwd_spread));
      }

      netbuf = net_buf_clone(buf);
      if (netbuf) {
        memcpy(net_buf_user_data(netbuf), net_buf_user_data(buf),
               buf->user_data_size);
        ctimer_set(netbuf, &mcast_periodic, fwd_delay, mcast_fwd, NULL);
      } else {
        PRINTF("SMRF: cannot clone net buffer\n");
      }
    }
    PRINTF("SMRF: %u bytes: fwd in %u [%u]\n",
           uip_len(buf), fwd_delay, fwd_spread);
  }

  /* Done with this packet unless we are a member of the mcast group */
  if(!uip_ds6_is_my_maddr(&UIP_IP_BUF(buf)->destipaddr)) {
    PRINTF("SMRF: Not a group member. No further processing\n");
    return UIP_MCAST6_DROP;
  } else {
    PRINTF("SMRF: Ours. Deliver to upper layers\n");
    UIP_MCAST6_STATS_ADD(mcast_in_ours);
    return UIP_MCAST6_ACCEPT;
  }
}
/*---------------------------------------------------------------------------*/
static void
init()
{
  UIP_MCAST6_STATS_INIT(NULL);

  uip_mcast6_route_init();
}
/*---------------------------------------------------------------------------*/
static void
out()
{
  return;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief The SMRF engine driver
 */
const struct uip_mcast6_driver smrf_driver = {
  "SMRF",
  init,
  out,
  in,
};
/*---------------------------------------------------------------------------*/
/** @} */
