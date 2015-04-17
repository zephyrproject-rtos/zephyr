/*
 * Copyright (c) 2011, Loughborough University - Computer Science
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
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \addtogroup uip6-multicast
 * @{
 */
/**
 * \file
 *    Multicast routing table manipulation
 *
 * \author
 *    George Oikonomou - <oikonomou@users.sourceforge.net>
 */

#include "contiki.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "net/ip/uip.h"
#include "net/ipv6/multicast/uip-mcast6-route.h"

#include <stdint.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
/* Size of the multicast routing table */
#ifdef UIP_MCAST6_ROUTE_CONF_ROUTES
#define UIP_MCAST6_ROUTE_ROUTES UIP_MCAST6_ROUTE_CONF_ROUTES
#else
#define UIP_MCAST6_ROUTE_ROUTES 1
#endif /* UIP_CONF_DS6_MCAST_ROUTES */
/*---------------------------------------------------------------------------*/
LIST(mcast_route_list);
MEMB(mcast_route_memb, uip_mcast6_route_t, UIP_MCAST6_ROUTE_ROUTES);

static uip_mcast6_route_t *locmcastrt;
/*---------------------------------------------------------------------------*/
uip_mcast6_route_t *
uip_mcast6_route_lookup(uip_ipaddr_t *group)
{
  locmcastrt = NULL;
  for(locmcastrt = list_head(mcast_route_list);
      locmcastrt != NULL;
      locmcastrt = list_item_next(locmcastrt)) {
    if(uip_ipaddr_cmp(&locmcastrt->group, group)) {
      return locmcastrt;
    }
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
uip_mcast6_route_t *
uip_mcast6_route_add(uip_ipaddr_t *group)
{
  /* _lookup must return NULL, i.e. the prefix does not exist in our table */
  locmcastrt = uip_mcast6_route_lookup(group);
  if(locmcastrt == NULL) {
    /* Allocate an entry and add the group to the list */
    locmcastrt = memb_alloc(&mcast_route_memb);
    if(locmcastrt == NULL) {
      return NULL;
    }
    list_add(mcast_route_list, locmcastrt);
  }

  /* Reaching here means we either found the prefix or allocated a new one */

  uip_ipaddr_copy(&(locmcastrt->group), group);

  return locmcastrt;
}
/*---------------------------------------------------------------------------*/
void
uip_mcast6_route_rm(uip_mcast6_route_t *route)
{
  /* Make sure it's actually in the list */
  for(locmcastrt = list_head(mcast_route_list);
      locmcastrt != NULL;
      locmcastrt = list_item_next(locmcastrt)) {
    if(locmcastrt == route) {
      list_remove(mcast_route_list, route);
      memb_free(&mcast_route_memb, route);
      return;
    }
  }
}
/*---------------------------------------------------------------------------*/
uip_mcast6_route_t *
uip_mcast6_route_list_head(void)
{
  return list_head(mcast_route_list);
}
/*---------------------------------------------------------------------------*/
int
uip_mcast6_route_count(void)
{
  return list_length(mcast_route_list);
}
/*---------------------------------------------------------------------------*/
void
uip_mcast6_route_init()
{
  memb_init(&mcast_route_memb);
  list_init(mcast_route_list);
}
/*---------------------------------------------------------------------------*/
/** @} */
