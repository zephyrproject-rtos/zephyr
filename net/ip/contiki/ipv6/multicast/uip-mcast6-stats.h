/*
 * Copyright (c) 2014, University of Bristol - http://www.bris.ac.uk
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
 *     Header file for IPv6 multicast forwarding stats maintenance
 *
 * \author
 *     George Oikonomou - <oikonomou@users.sourceforge.net>
 */
#ifndef UIP_MCAST6_STATS_H_
#define UIP_MCAST6_STATS_H_
/*---------------------------------------------------------------------------*/
#include "contiki-conf.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
/* The platform can override the stats datatype */
#ifdef UIP_MCAST6_CONF_STATS_DATATYPE
#define UIP_MCAST6_STATS_DATATYPE UIP_MCAST6_CONF_STATS_DATATYPE
#else
#define UIP_MCAST6_STATS_DATATYPE uint16_t
#endif
/*---------------------------------------------------------------------------*/
#ifdef UIP_MCAST6_CONF_STATS
#define UIP_MCAST6_STATS UIP_MCAST6_CONF_STATS
#else
#define UIP_MCAST6_STATS 0
#endif
/*---------------------------------------------------------------------------*/
/* Stats datatype */
/*---------------------------------------------------------------------------*/
/**
 * \brief A data structure used to maintain multicast stats
 *
 * Each engine can extend this structure via the engine_stats field
 */
typedef struct uip_mcast6_stats {
  /** Count of unique datagrams received */
  UIP_MCAST6_STATS_DATATYPE mcast_in_unique;

  /** Count of all datagrams received */
  UIP_MCAST6_STATS_DATATYPE mcast_in_all;

  /** Count of datagrams received for a group that we have joined */
  UIP_MCAST6_STATS_DATATYPE mcast_in_ours;

  /** Count of datagrams forwarded by us but we are not the seed */
  UIP_MCAST6_STATS_DATATYPE mcast_fwd;

  /** Count of multicast datagrams originated by us */
  UIP_MCAST6_STATS_DATATYPE mcast_out;

  /** Count of malformed multicast datagrams seen by us */
  UIP_MCAST6_STATS_DATATYPE mcast_bad;

  /** Count of multicast datagrams correclty formed but dropped by us */
  UIP_MCAST6_STATS_DATATYPE mcast_dropped;

  /** Opaque pointer to an engine's additional stats */
  void *engine_stats;
} uip_mcast6_stats_t;
/*---------------------------------------------------------------------------*/
/* Access macros */
/*---------------------------------------------------------------------------*/
#if UIP_MCAST6_STATS
/* Don't access this variable directly, use the macros below */
extern uip_mcast6_stats_t uip_mcast6_stats;

#define UIP_MCAST6_STATS_ADD(x) uip_mcast6_stats.x++
#define UIP_MCAST6_STATS_GET(x) uip_mcast6_stats.x
#define UIP_MCAST6_STATS_INIT(s) uip_mcast6_stats_init(s)
#else /* UIP_MCAST6_STATS */
#define UIP_MCAST6_STATS_ADD(x)
#define UIP_MCAST6_STATS_GET(x) 0
#define UIP_MCAST6_STATS_INIT(s)
#endif /* UIP_MCAST6_STATS */
/*---------------------------------------------------------------------------*/
/**
 * \brief Initialise multicast stats
 * \param stats A pointer to a struct holding an engine's additional statistics
 */
void uip_mcast6_stats_init(void *stats);
/*---------------------------------------------------------------------------*/
#endif /* UIP_MCAST6_STATS_H_ */
/*---------------------------------------------------------------------------*/
/** @} */
