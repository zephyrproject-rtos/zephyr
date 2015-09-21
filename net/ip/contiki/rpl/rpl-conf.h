/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 * \file
 *	Public configuration and API declarations for ContikiRPL.
 * \author
 *	Joakim Eriksson <joakime@sics.se> & Nicolas Tsiftes <nvt@sics.se>
 *
 */

#ifndef RPL_CONF_H
#define RPL_CONF_H

#include "contiki-conf.h"

/* Set to 1 to enable RPL statistics */
#ifndef RPL_CONF_STATS
#define RPL_CONF_STATS 0
#endif /* RPL_CONF_STATS */

/* 
 * Select routing metric supported at runtime. This must be a valid
 * DAG Metric Container Object Type (see below). Currently, we only 
 * support RPL_DAG_MC_ETX and RPL_DAG_MC_ENERGY.
 * When MRHOF (RFC6719) is used with ETX, no metric container must
 * be used; instead the rank carries ETX directly.
 */
#ifdef RPL_CONF_DAG_MC
#define RPL_DAG_MC RPL_CONF_DAG_MC
#else
#define RPL_DAG_MC RPL_DAG_MC_NONE
#endif /* RPL_CONF_DAG_MC */

/*
 * The objective function used by RPL is configurable through the 
 * RPL_CONF_OF parameter. This should be defined to be the name of an 
 * rpl_of object linked into the system image, e.g., rpl_of0.
 */
#ifdef RPL_CONF_OF
#define RPL_OF RPL_CONF_OF
#else
/* ETX is the default objective function. */
#define RPL_OF rpl_mrhof
#endif /* RPL_CONF_OF */

/* This value decides which DAG instance we should participate in by default. */
#ifdef RPL_CONF_DEFAULT_INSTANCE
#define RPL_DEFAULT_INSTANCE RPL_CONF_DEFAULT_INSTANCE
#else
#define RPL_DEFAULT_INSTANCE	       0x1e
#endif /* RPL_CONF_DEFAULT_INSTANCE */

/*
 * This value decides if this node must stay as a leaf or not
 * as allowed by draft-ietf-roll-rpl-19#section-8.5
 */
#ifdef RPL_CONF_LEAF_ONLY
#define RPL_LEAF_ONLY RPL_CONF_LEAF_ONLY
#else
#define RPL_LEAF_ONLY 0
#endif

/*
 * Maximum of concurent RPL instances.
 */
#ifdef RPL_CONF_MAX_INSTANCES
#define RPL_MAX_INSTANCES     RPL_CONF_MAX_INSTANCES
#else
#define RPL_MAX_INSTANCES     1
#endif /* RPL_CONF_MAX_INSTANCES */

/*
 * Maximum number of DAGs within an instance.
 */
#ifdef RPL_CONF_MAX_DAG_PER_INSTANCE
#define RPL_MAX_DAG_PER_INSTANCE     RPL_CONF_MAX_DAG_PER_INSTANCE
#else
#define RPL_MAX_DAG_PER_INSTANCE     2
#endif /* RPL_CONF_MAX_DAG_PER_INSTANCE */

/*
 * 
 */
#ifndef RPL_CONF_DAO_SPECIFY_DAG
  #if RPL_MAX_DAG_PER_INSTANCE > 1
    #define RPL_DAO_SPECIFY_DAG 1
  #else
    #define RPL_DAO_SPECIFY_DAG 0
  #endif /* RPL_MAX_DAG_PER_INSTANCE > 1 */
#else
  #define RPL_DAO_SPECIFY_DAG RPL_CONF_DAO_SPECIFY_DAG
#endif /* RPL_CONF_DAO_SPECIFY_DAG */

/*
 * The DIO interval (n) represents 2^n ms.
 *
 * According to the specification, the default value is 3 which
 * means 8 milliseconds. That is far too low when using duty cycling
 * with wake-up intervals that are typically hundreds of milliseconds.
 * ContikiRPL thus sets the default to 2^12 ms = 4.096 s.
 */
#ifdef RPL_CONF_DIO_INTERVAL_MIN
#define RPL_DIO_INTERVAL_MIN        RPL_CONF_DIO_INTERVAL_MIN
#else
#define RPL_DIO_INTERVAL_MIN        12
#endif

/*
 * Maximum amount of timer doublings.
 *
 * The maximum interval will by default be 2^(12+8) ms = 1048.576 s.
 * RFC 6550 suggests a default value of 20, which of course would be
 * unsuitable when we start with a minimum interval of 2^12.
 */
#ifdef RPL_CONF_DIO_INTERVAL_DOUBLINGS
#define RPL_DIO_INTERVAL_DOUBLINGS  RPL_CONF_DIO_INTERVAL_DOUBLINGS
#else
#define RPL_DIO_INTERVAL_DOUBLINGS  8
#endif

/*
 * DIO redundancy. To learn more about this, see RFC 6206.
 *
 * RFC 6550 suggests a default value of 10. It is unclear what the basis
 * of this suggestion is. Network operators might attain more efficient
 * operation by tuning this parameter for specific deployments.
 */
#ifdef RPL_CONF_DIO_REDUNDANCY
#define RPL_DIO_REDUNDANCY          RPL_CONF_DIO_REDUNDANCY
#else
#define RPL_DIO_REDUNDANCY          10
#endif

/*
 * Initial metric attributed to a link when the ETX is unknown
 */
#ifndef RPL_CONF_INIT_LINK_METRIC
#define RPL_INIT_LINK_METRIC        2
#else
#define RPL_INIT_LINK_METRIC        RPL_CONF_INIT_LINK_METRIC
#endif

/*
 * Default route lifetime unit. This is the granularity of time
 * used in RPL lifetime values, in seconds.
 */
#ifndef RPL_CONF_DEFAULT_LIFETIME_UNIT
#define RPL_DEFAULT_LIFETIME_UNIT       0xffff
#else
#define RPL_DEFAULT_LIFETIME_UNIT       RPL_CONF_DEFAULT_LIFETIME_UNIT
#endif

/*
 * Default route lifetime as a multiple of the lifetime unit.
 */
#ifndef RPL_CONF_DEFAULT_LIFETIME
#define RPL_DEFAULT_LIFETIME            0xff
#else
#define RPL_DEFAULT_LIFETIME            RPL_CONF_DEFAULT_LIFETIME
#endif

/*
 * DAG preference field
 */
#ifdef RPL_CONF_PREFERENCE
#define RPL_PREFERENCE              RPL_CONF_PREFERENCE
#else
#define RPL_PREFERENCE              0
#endif

/*
 * Hop-by-hop option
 * This option control the insertion of the RPL Hop-by-Hop extension header
 * into packets originating from this node. Incoming Hop-by-hop extension
 * header are still processed and forwarded.
 */
#ifdef RPL_CONF_INSERT_HBH_OPTION
#define RPL_INSERT_HBH_OPTION       RPL_CONF_INSERT_HBH_OPTION
#else
#define RPL_INSERT_HBH_OPTION       1
#endif

/*
 * RPL probing. When enabled, probes will be sent periodically to keep
 * parent link estimates up to date.
 * */
#ifdef RPL_CONF_WITH_PROBING
#define RPL_WITH_PROBING RPL_CONF_WITH_PROBING
#else
#define RPL_WITH_PROBING 1
#endif

/*
 * RPL probing interval.
 * */
#ifdef RPL_CONF_PROBING_INTERVAL
#define RPL_PROBING_INTERVAL RPL_CONF_PROBING_INTERVAL
#else
#define RPL_PROBING_INTERVAL (120 * CLOCK_SECOND)
#endif

/*
 * RPL probing expiration time.
 * */
#ifdef RPL_CONF_PROBING_EXPIRATION_TIME
#define RPL_PROBING_EXPIRATION_TIME RPL_CONF_PROBING_EXPIRATION_TIME
#else
#define RPL_PROBING_EXPIRATION_TIME (10 * 60 * CLOCK_SECOND)
#endif

/*
 * Function used to select the next parent to be probed.
 * */
#ifdef RPL_CONF_PROBING_SELECT_FUNC
#define RPL_PROBING_SELECT_FUNC RPL_CONF_PROBING_SELECT_FUNC
#else
#define RPL_PROBING_SELECT_FUNC(dag) get_probing_target((dag))
#endif

/*
 * Function used to send RPL probes.
 * To probe with DIO, use:
 * #define RPL_CONF_PROBING_SEND_FUNC(instance, addr) dio_output((instance), (addr))
 * To probe with DIS, use:
 * #define RPL_CONF_PROBING_SEND_FUNC(instance, addr) dis_output((addr))
 * Any other custom probing function is also acceptable.
 * */
#ifdef RPL_CONF_PROBING_SEND_FUNC
#define RPL_PROBING_SEND_FUNC RPL_CONF_PROBING_SEND_FUNC
#else
#define RPL_PROBING_SEND_FUNC(instance, addr) dio_output((instance), (addr))
#endif

/*
 * Function used to calculate next RPL probing interval
 * */
#ifdef RPL_CONF_PROBING_DELAY_FUNC
#define RPL_PROBING_DELAY_FUNC RPL_CONF_PROBING_DELAY_FUNC
#else
#define RPL_PROBING_DELAY_FUNC() ((RPL_PROBING_INTERVAL / 2) \
    + random_rand() % (RPL_PROBING_INTERVAL))
#endif

#endif /* RPL_CONF_H */
