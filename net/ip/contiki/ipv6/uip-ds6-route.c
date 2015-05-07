/*
 * Copyright (c) 2012, Thingsquare, http://www.thingsquare.com/.
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
 *
 */
/**
 * \addtogroup uip6
 * @{
 */

/**
 * \file
 *    Routing table manipulation
 */
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip.h"

#include "lib/list.h"
#include "lib/memb.h"
#include "net/nbr-table.h"

#include <string.h>

/* The nbr_routes holds a neighbor table to be able to maintain
   information about what routes go through what neighbor. This
   neighbor table is registered with the central nbr-table repository
   so that it will be maintained along with the rest of the neighbor
   tables in the system. */
NBR_TABLE(struct uip_ds6_route_neighbor_routes, nbr_routes);
MEMB(neighborroutememb, struct uip_ds6_route_neighbor_route, UIP_DS6_ROUTE_NB);

/* Each route is repressented by a uip_ds6_route_t structure and
   memory for each route is allocated from the routememb memory
   block. These routes are maintained on the routelist. */
LIST(routelist);
MEMB(routememb, uip_ds6_route_t, UIP_DS6_ROUTE_NB);

/* Default routes are held on the defaultrouterlist and their
   structures are allocated from the defaultroutermemb memory block.*/
LIST(defaultrouterlist);
MEMB(defaultroutermemb, uip_ds6_defrt_t, UIP_DS6_DEFRT_NB);

#if UIP_DS6_NOTIFICATIONS
LIST(notificationlist);
#endif

static int num_routes = 0;

#undef DEBUG
#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

static void rm_routelist_callback(nbr_table_item_t *ptr);
/*---------------------------------------------------------------------------*/
#if DEBUG != DEBUG_NONE
static void
assert_nbr_routes_list_sane(void)
{
  uip_ds6_route_t *r;
  int count;

  /* Check if the route list has an infinite loop. */
  for(r = uip_ds6_route_head(),
        count = 0;
      r != NULL &&
        count < UIP_DS6_ROUTE_NB * 2;
      r = uip_ds6_route_next(r),
        count++);

  if(count > UIP_DS6_ROUTE_NB) {
    printf("uip-ds6-route.c: assert_nbr_routes_list_sane route list is in infinite loop\n");
  }

  /* Make sure that the route list has as many entries as the
     num_routes vairable. */
  if(count < num_routes) {
    printf("uip-ds6-route.c: assert_nbr_routes_list_sane too few entries on route list: should be %d, is %d, max %d\n",
           num_routes, count, UIP_CONF_MAX_ROUTES);
  }
}
#endif /* DEBUG != DEBUG_NONE */
/*---------------------------------------------------------------------------*/
#if UIP_DS6_NOTIFICATIONS
static void
call_route_callback(int event, uip_ipaddr_t *route,
		    uip_ipaddr_t *nexthop)
{
  int num;
  struct uip_ds6_notification *n;
  for(n = list_head(notificationlist);
      n != NULL;
      n = list_item_next(n)) {
    if(event == UIP_DS6_NOTIFICATION_DEFRT_ADD ||
       event == UIP_DS6_NOTIFICATION_DEFRT_RM) {
      num = list_length(defaultrouterlist);
    } else {
      num = num_routes;
    }
    n->callback(event, route, nexthop, num);
  }
}
/*---------------------------------------------------------------------------*/
void
uip_ds6_notification_add(struct uip_ds6_notification *n,
			 uip_ds6_notification_callback c)
{
  if(n != NULL && c != NULL) {
    n->callback = c;
    list_add(notificationlist, n);
  }
}
/*---------------------------------------------------------------------------*/
void
uip_ds6_notification_rm(struct uip_ds6_notification *n)
{
  list_remove(notificationlist, n);
}
#endif
/*---------------------------------------------------------------------------*/
void
uip_ds6_route_init(void)
{
  memb_init(&routememb);
  list_init(routelist);
  nbr_table_register(nbr_routes,
                     (nbr_table_callback *)rm_routelist_callback);

  memb_init(&defaultroutermemb);
  list_init(defaultrouterlist);

#if UIP_DS6_NOTIFICATIONS
  list_init(notificationlist);
#endif
}
/*---------------------------------------------------------------------------*/
static uip_lladdr_t *
uip_ds6_route_nexthop_lladdr(uip_ds6_route_t *route)
{
  if(route != NULL) {
    return (uip_lladdr_t *)nbr_table_get_lladdr(nbr_routes,
                                                route->neighbor_routes);
  } else {
    return NULL;
  }
}
/*---------------------------------------------------------------------------*/
uip_ipaddr_t *
uip_ds6_route_nexthop(uip_ds6_route_t *route)
{
  if(route != NULL) {
    return uip_ds6_nbr_ipaddr_from_lladdr(uip_ds6_route_nexthop_lladdr(route));
  } else {
    return NULL;
  }
}
/*---------------------------------------------------------------------------*/
uip_ds6_route_t *
uip_ds6_route_head(void)
{
  return list_head(routelist);
}
/*---------------------------------------------------------------------------*/
uip_ds6_route_t *
uip_ds6_route_next(uip_ds6_route_t *r)
{
  if(r != NULL) {
    uip_ds6_route_t *n = list_item_next(r);
    return n;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
int
uip_ds6_route_num_routes(void)
{
  return num_routes;
}
/*---------------------------------------------------------------------------*/
uip_ds6_route_t *
uip_ds6_route_lookup(uip_ipaddr_t *addr)
{
  uip_ds6_route_t *r;
  uip_ds6_route_t *found_route;
  uint8_t longestmatch;

  PRINTF("uip-ds6-route: Looking up route for ");
  PRINT6ADDR(addr);
  PRINTF("\n");


  found_route = NULL;
  longestmatch = 0;
  for(r = uip_ds6_route_head();
      r != NULL;
      r = uip_ds6_route_next(r)) {
    if(r->length >= longestmatch &&
       uip_ipaddr_prefixcmp(addr, &r->ipaddr, r->length)) {
      longestmatch = r->length;
      found_route = r;
      /* check if total match - e.g. all 128 bits do match */
      if(longestmatch == 128) {
	break;
      }
    }
  }

  if(found_route != NULL) {
    PRINTF("uip-ds6-route: Found route: ");
    PRINT6ADDR(addr);
    PRINTF(" via ");
    PRINT6ADDR(uip_ds6_route_nexthop(found_route));
    PRINTF("\n");
  } else {
    PRINTF("uip-ds6-route: No route found\n");
  }

  if(found_route != NULL && found_route != list_head(routelist)) {
    /* If we found a route, we put it at the start of the routeslist
       list. The list is ordered by how recently we looked them up:
       the least recently used route will be at the end of the
       list - for fast lookups (assuming multiple packets to the same node). */

    list_remove(routelist, found_route);
    list_push(routelist, found_route);
  }

  return found_route;
}
/*---------------------------------------------------------------------------*/
uip_ds6_route_t *
uip_ds6_route_add(uip_ipaddr_t *ipaddr, uint8_t length,
		  uip_ipaddr_t *nexthop)
{
  uip_ds6_route_t *r;
  struct uip_ds6_route_neighbor_route *nbrr;

#if DEBUG != DEBUG_NONE
  assert_nbr_routes_list_sane();
#endif /* DEBUG != DEBUG_NONE */

  /* Get link-layer address of next hop, make sure it is in neighbor table */
  const uip_lladdr_t *nexthop_lladdr = uip_ds6_nbr_lladdr_from_ipaddr(nexthop);
  if(nexthop_lladdr == NULL) {
    PRINTF("uip_ds6_route_add: neighbor link-local address unknown for ");
    PRINT6ADDR(nexthop);
    PRINTF("\n");
    return NULL;
  }

  /* First make sure that we don't add a route twice. If we find an
     existing route for our destination, we'll delete the old
     one first. */
  r = uip_ds6_route_lookup(ipaddr);
  if(r != NULL) {
    uip_ipaddr_t *current_nexthop;
    current_nexthop = uip_ds6_route_nexthop(r);
    if(current_nexthop != NULL && uip_ipaddr_cmp(nexthop, current_nexthop)) {
      /* no need to update route - already correct! */
      return r;
    }
    PRINTF("uip_ds6_route_add: old route for ");
    PRINT6ADDR(ipaddr);
    PRINTF(" found, deleting it\n");

    uip_ds6_route_rm(r);
  }
  {
    struct uip_ds6_route_neighbor_routes *routes;
    /* If there is no routing entry, create one. We first need to
       check if we have room for this route. If not, we remove the
       least recently used one we have. */

    if(uip_ds6_route_num_routes() == UIP_DS6_ROUTE_NB) {
      /* Removing the oldest route entry from the route table. The
         least recently used route is the first route on the list. */
      uip_ds6_route_t *oldest;

      oldest = list_tail(routelist); /* uip_ds6_route_head(); */
      PRINTF("uip_ds6_route_add: dropping route to ");
      PRINT6ADDR(&oldest->ipaddr);
      PRINTF("\n");
      uip_ds6_route_rm(oldest);
    }


    /* Every neighbor on our neighbor table holds a struct
       uip_ds6_route_neighbor_routes which holds a list of routes that
       go through the neighbor. We add our route entry to this list.

       We first check to see if we already have this neighbor in our
       nbr_route table. If so, the neighbor already has a route entry
       list.
    */
    routes = nbr_table_get_from_lladdr(nbr_routes,
                                       (linkaddr_t *)nexthop_lladdr);

    if(routes == NULL) {
      /* If the neighbor did not have an entry in our neighbor table,
         we create one. The nbr_table_add_lladdr() function returns a
         pointer to a pointer that we may use for our own purposes. We
         initialize this pointer with the list of routing entries that
         are attached to this neighbor. */
      routes = nbr_table_add_lladdr(nbr_routes,
                                    (linkaddr_t *)nexthop_lladdr);
      if(routes == NULL) {
        /* This should not happen, as we explicitly deallocated one
           route table entry above. */
        PRINTF("uip_ds6_route_add: could not allocate neighbor table entry\n");
        return NULL;
      }
      LIST_STRUCT_INIT(routes, route_list);
    }

    /* Allocate a routing entry and populate it. */
    r = memb_alloc(&routememb);

    if(r == NULL) {
      /* This should not happen, as we explicitly deallocated one
         route table entry above. */
      PRINTF("uip_ds6_route_add: could not allocate route\n");
      return NULL;
    }

    /* add new routes first - assuming that there is a reason to add this
       and that there is a packet coming soon. */
    list_push(routelist, r);

    nbrr = memb_alloc(&neighborroutememb);
    if(nbrr == NULL) {
      /* This should not happen, as we explicitly deallocated one
         route table entry above. */
      PRINTF("uip_ds6_route_add: could not allocate neighbor route list entry\n");
      memb_free(&routememb, r);
      return NULL;
    }

    nbrr->route = r;
    /* Add the route to this neighbor */
    list_add(routes->route_list, nbrr);
    r->neighbor_routes = routes;
    num_routes++;

    PRINTF("uip_ds6_route_add num %d\n", num_routes);
  }

  uip_ipaddr_copy(&(r->ipaddr), ipaddr);
  r->length = length;

#ifdef UIP_DS6_ROUTE_STATE_TYPE
  memset(&r->state, 0, sizeof(UIP_DS6_ROUTE_STATE_TYPE));
#endif

  PRINTF("uip_ds6_route_add: adding route: ");
  PRINT6ADDR(ipaddr);
  PRINTF(" via ");
  PRINT6ADDR(nexthop);
  PRINTF("\n");
  ANNOTATE("#L %u 1;blue\n", nexthop->u8[sizeof(uip_ipaddr_t) - 1]);

#if UIP_DS6_NOTIFICATIONS
  call_route_callback(UIP_DS6_NOTIFICATION_ROUTE_ADD, ipaddr, nexthop);
#endif

#if DEBUG != DEBUG_NONE
  assert_nbr_routes_list_sane();
#endif /* DEBUG != DEBUG_NONE */
  return r;
}

/*---------------------------------------------------------------------------*/
void
uip_ds6_route_rm(uip_ds6_route_t *route)
{
  struct uip_ds6_route_neighbor_route *neighbor_route;
#if DEBUG != DEBUG_NONE
  assert_nbr_routes_list_sane();
#endif /* DEBUG != DEBUG_NONE */
  if(route != NULL && route->neighbor_routes != NULL) {

    PRINTF("uip_ds6_route_rm: removing route: ");
    PRINT6ADDR(&route->ipaddr);
    PRINTF("\n");

    /* Remove the route from the route list */
    list_remove(routelist, route);

    /* Find the corresponding neighbor_route and remove it. */
    for(neighbor_route = list_head(route->neighbor_routes->route_list);
        neighbor_route != NULL && neighbor_route->route != route;
        neighbor_route = list_item_next(neighbor_route));

    if(neighbor_route == NULL) {
      PRINTF("uip_ds6_route_rm: neighbor_route was NULL for ");
      uip_debug_ipaddr_print(&route->ipaddr);
      PRINTF("\n");
    }
    list_remove(route->neighbor_routes->route_list, neighbor_route);
    if(list_head(route->neighbor_routes->route_list) == NULL) {
      /* If this was the only route using this neighbor, remove the
         neibhor from the table */
      PRINTF("uip_ds6_route_rm: removing neighbor too\n");
      nbr_table_remove(nbr_routes, route->neighbor_routes->route_list);
    }
    memb_free(&routememb, route);
    memb_free(&neighborroutememb, neighbor_route);

    num_routes--;

    PRINTF("uip_ds6_route_rm num %d\n", num_routes);

#if UIP_DS6_NOTIFICATIONS
    call_route_callback(UIP_DS6_NOTIFICATION_ROUTE_RM,
        &route->ipaddr, uip_ds6_route_nexthop(route));
#endif
#if 0 //(DEBUG & DEBUG_ANNOTATE) == DEBUG_ANNOTATE
    /* we need to check if this was the last route towards "nexthop" */
    /* if so - remove that link (annotation) */
    uip_ds6_route_t *r;
    for(r = uip_ds6_route_head();
        r != NULL;
        r = uip_ds6_route_next(r)) {
      uip_ipaddr_t *nextr, *nextroute;
      nextr = uip_ds6_route_nexthop(r);
      nextroute = uip_ds6_route_nexthop(route);
      if(nextr != NULL &&
         nextroute != NULL &&
         uip_ipaddr_cmp(nextr, nextroute)) {
        /* we found another link using the specific nexthop, so keep the #L */
        return;
      }
    }
    ANNOTATE("#L %u 0\n", uip_ds6_route_nexthop(route)->u8[sizeof(uip_ipaddr_t) - 1]);
#endif
  }

#if DEBUG != DEBUG_NONE
  assert_nbr_routes_list_sane();
#endif /* DEBUG != DEBUG_NONE */
  return;
}
/*---------------------------------------------------------------------------*/
static void
rm_routelist(struct uip_ds6_route_neighbor_routes *routes)
{
#if DEBUG != DEBUG_NONE
  assert_nbr_routes_list_sane();
#endif /* DEBUG != DEBUG_NONE */
  PRINTF("uip_ds6_route_rm_routelist\n");
  if(routes != NULL && routes->route_list != NULL) {
    struct uip_ds6_route_neighbor_route *r;
    r = list_head(routes->route_list);
    while(r != NULL) {
      uip_ds6_route_rm(r->route);
      r = list_head(routes->route_list);
    }
    nbr_table_remove(nbr_routes, routes);
  }
#if DEBUG != DEBUG_NONE
  assert_nbr_routes_list_sane();
#endif /* DEBUG != DEBUG_NONE */
}
/*---------------------------------------------------------------------------*/
static void
rm_routelist_callback(nbr_table_item_t *ptr)
{
  rm_routelist((struct uip_ds6_route_neighbor_routes *)ptr);
}
/*---------------------------------------------------------------------------*/
void
uip_ds6_route_rm_by_nexthop(uip_ipaddr_t *nexthop)
{
  /* Get routing entry list of this neighbor */
  const uip_lladdr_t *nexthop_lladdr;
  struct uip_ds6_route_neighbor_routes *routes;

  nexthop_lladdr = uip_ds6_nbr_lladdr_from_ipaddr(nexthop);
  routes = nbr_table_get_from_lladdr(nbr_routes,
                                     (linkaddr_t *)nexthop_lladdr);
  rm_routelist(routes);
}
/*---------------------------------------------------------------------------*/
uip_ds6_defrt_t *
uip_ds6_defrt_add(uip_ipaddr_t *ipaddr, unsigned long interval)
{
  uip_ds6_defrt_t *d;

#if DEBUG != DEBUG_NONE
  assert_nbr_routes_list_sane();
#endif /* DEBUG != DEBUG_NONE */

  PRINTF("uip_ds6_defrt_add\n");
  d = uip_ds6_defrt_lookup(ipaddr);
  if(d == NULL) {
    d = memb_alloc(&defaultroutermemb);
    if(d == NULL) {
      PRINTF("uip_ds6_defrt_add: could not add default route to ");
      PRINT6ADDR(ipaddr);
      PRINTF(", out of memory\n");
      return NULL;
    } else {
      PRINTF("uip_ds6_defrt_add: adding default route to ");
      PRINT6ADDR(ipaddr);
      PRINTF("\n");
    }

    list_push(defaultrouterlist, d);
  }

  uip_ipaddr_copy(&d->ipaddr, ipaddr);
  if(interval != 0) {
    stimer_set(&d->lifetime, interval);
    d->isinfinite = 0;
  } else {
    d->isinfinite = 1;
  }

  ANNOTATE("#L %u 1\n", ipaddr->u8[sizeof(uip_ipaddr_t) - 1]);

#if UIP_DS6_NOTIFICATIONS
  call_route_callback(UIP_DS6_NOTIFICATION_DEFRT_ADD, ipaddr, ipaddr);
#endif

#if DEBUG != DEBUG_NONE
  assert_nbr_routes_list_sane();
#endif /* DEBUG != DEBUG_NONE */

  return d;
}
/*---------------------------------------------------------------------------*/
void
uip_ds6_defrt_rm(uip_ds6_defrt_t *defrt)
{
  uip_ds6_defrt_t *d;

#if DEBUG != DEBUG_NONE
  assert_nbr_routes_list_sane();
#endif /* DEBUG != DEBUG_NONE */

  /* Make sure that the defrt is in the list before we remove it. */
  for(d = list_head(defaultrouterlist);
      d != NULL;
      d = list_item_next(d)) {
    if(d == defrt) {
      PRINTF("Removing default route\n");
      list_remove(defaultrouterlist, defrt);
      memb_free(&defaultroutermemb, defrt);
      ANNOTATE("#L %u 0\n", defrt->ipaddr.u8[sizeof(uip_ipaddr_t) - 1]);
#if UIP_DS6_NOTIFICATIONS
      call_route_callback(UIP_DS6_NOTIFICATION_DEFRT_RM,
			  &defrt->ipaddr, &defrt->ipaddr);
#endif
      return;
    }
  }
#if DEBUG != DEBUG_NONE
  assert_nbr_routes_list_sane();
#endif /* DEBUG != DEBUG_NONE */

}
/*---------------------------------------------------------------------------*/
uip_ds6_defrt_t *
uip_ds6_defrt_lookup(uip_ipaddr_t *ipaddr)
{
  uip_ds6_defrt_t *d;
  for(d = list_head(defaultrouterlist);
      d != NULL;
      d = list_item_next(d)) {
    if(uip_ipaddr_cmp(&d->ipaddr, ipaddr)) {
      return d;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
uip_ipaddr_t *
uip_ds6_defrt_choose(void)
{
  uip_ds6_defrt_t *d;
  uip_ds6_nbr_t *bestnbr;
  uip_ipaddr_t *addr;

  addr = NULL;
  for(d = list_head(defaultrouterlist);
      d != NULL;
      d = list_item_next(d)) {
    PRINTF("Defrt, IP address ");
    PRINT6ADDR(&d->ipaddr);
    PRINTF("\n");
    bestnbr = uip_ds6_nbr_lookup(&d->ipaddr);
    if(bestnbr != NULL && bestnbr->state != NBR_INCOMPLETE) {
      PRINTF("Defrt found, IP address ");
      PRINT6ADDR(&d->ipaddr);
      PRINTF("\n");
      return &d->ipaddr;
    } else {
      addr = &d->ipaddr;
      PRINTF("Defrt INCOMPLETE found, IP address ");
      PRINT6ADDR(&d->ipaddr);
      PRINTF("\n");
    }
  }
  return addr;
}
/*---------------------------------------------------------------------------*/
void
uip_ds6_defrt_periodic(void)
{
  uip_ds6_defrt_t *d;
  d = list_head(defaultrouterlist);
  while(d != NULL) {
    if(!d->isinfinite &&
       stimer_expired(&d->lifetime)) {
      PRINTF("uip_ds6_defrt_periodic: defrt lifetime expired\n");
      uip_ds6_defrt_rm(d);
      d = list_head(defaultrouterlist);
    } else {
      d = list_item_next(d);
    }
  }
}
/*---------------------------------------------------------------------------*/
/** @} */
