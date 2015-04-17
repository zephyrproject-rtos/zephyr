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
 * \addtogroup uip6-multicast
 * @{
 */
/**
 * \defgroup smrf-multicast 'Stateless Multicast RPL Forwarding' (SMRF)
 *
 * SMRF will only work in RPL networks in MOP 3 "Storing with Multicast"
 * @{
 */
/**
 * \file
 *    Header file for the SMRF forwarding engine
 *
 * \author
 *    George Oikonomou - <oikonomou@users.sourceforge.net>
 */

#ifndef SMRF_H_
#define SMRF_H_

#include "contiki-conf.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
/* Configuration */
/*---------------------------------------------------------------------------*/
/* Fmin */
#ifdef SMRF_CONF_MIN_FWD_DELAY
#define SMRF_MIN_FWD_DELAY SMRF_CONF_MIN_FWD_DELAY
#else
#define SMRF_MIN_FWD_DELAY 4
#endif

/* Max Spread */
#ifdef SMRF_CONF_MAX_SPREAD
#define SMRF_MAX_SPREAD SMRF_CONF_MAX_SPREAD
#else
#define SMRF_MAX_SPREAD 4
#endif
/*---------------------------------------------------------------------------*/
/* Stats datatype */
/*---------------------------------------------------------------------------*/
struct smrf_stats {
  uint16_t mcast_in_unique;
  uint16_t mcast_in_all;        /* At layer 3 */
  uint16_t mcast_in_ours;       /* Unique and we are a group member */
  uint16_t mcast_fwd;           /* Forwarded by us but we are not the seed */
  uint16_t mcast_out;           /* We are the seed */
  uint16_t mcast_bad;
  uint16_t mcast_dropped;
};
/*---------------------------------------------------------------------------*/
#endif /* SMRF_H_ */
/*---------------------------------------------------------------------------*/
/** @} */
/** @} */
