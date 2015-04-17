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
 *    Header file for multicast routing table manipulation
 *
 * \author
 *    George Oikonomou - <oikonomou@users.sourceforge.net>
 */
#ifndef UIP_MCAST6_ROUTE_H_
#define UIP_MCAST6_ROUTE_H_

#include "contiki.h"
#include "net/ip/uip.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
/** \brief An entry in the multicast routing table */
typedef struct uip_mcast6_route {
  struct uip_mcast6_route *next; /**< Routes are arranged in a linked list */
  uip_ipaddr_t group; /**< The multicast group */
  uint32_t lifetime; /**< Entry lifetime seconds */
  void *dag; /**< Pointer to an rpl_dag_t struct */
} uip_mcast6_route_t;
/*---------------------------------------------------------------------------*/
/** \name Multicast Routing Table Manipulation */
/** @{ */

/**
 * \brief Lookup a multicast route
 * \param group A pointer to the multicast group to be searched for
 * \return A pointer to the new routing entry, or NULL if the route could not
 *         be found
 */
uip_mcast6_route_t *uip_mcast6_route_lookup(uip_ipaddr_t *group);

/**
 * \brief Add a multicast route
 * \param group A pointer to the multicast group to be added
 * \return A pointer to the new route, or NULL if the route could not be added
 */
uip_mcast6_route_t *uip_mcast6_route_add(uip_ipaddr_t *group);

/**
 * \brief Remove a multicast route
 * \param route A pointer to the route to be removed
 */
void uip_mcast6_route_rm(uip_mcast6_route_t *route);

/**
 * \brief Retrieve the count of multicast routes
 * \return The number of multicast routes
 */
int uip_mcast6_route_count(void);

/**
 * \brief Retrieve a pointer to the start of the multicast routes list
 * \return A pointer to the start of the multicast routes
 *
 * If the multicast routes list is empty, this function will return NULL
 */
uip_mcast6_route_t *uip_mcast6_route_list_head(void);
/*---------------------------------------------------------------------------*/
/**
 * \brief Multicast routing table init routine
 *
 *        Multicast routing tables are not necessarily required by all
 *        multicast engines. For instance, trickle multicast does not rely on
 *        the existence of a routing table. Therefore, this function here
 *        should be invoked by each engine's init routine only if the relevant
 *        functionality is required. This is also why this function should not
 *        get hooked into the uip-ds6 core.
 */
void uip_mcast6_route_init(void);
/** @} */

#endif /* UIP_MCAST6_ROUTE_H_ */
/** @} */
