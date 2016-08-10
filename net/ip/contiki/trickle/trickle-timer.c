/*
 * Copyright (c) 2012, George Oikonomou - <oikonomou@users.sourceforge.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
 * \file
 *   Trickle timer library implementation.
 * \author
 *   George Oikonomou - <oikonomou@users.sourceforge.net>
 */

/**
 * \addtogroup trickle-timer
 * @{
 */

#include <net/buf.h>

#include "contiki-conf.h"
#include "trickle-timer.h"
#include "sys/ctimer.h"
#include "sys/cc.h"
#include "lib/random.h"
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_TRICKLE
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

/*---------------------------------------------------------------------------*/
/**
 * \brief Wide randoms for platforms using a 4-byte wide clock
 * (see ::TRICKLE_TIMER_WIDE_RAND)
 */
#if TRICKLE_TIMER_WIDE_RAND
#define tt_rand() wide_rand()
#else
#define tt_rand() random_rand()
#endif
/*---------------------------------------------------------------------------*/
/* Declarations of variables of local interest */
/*---------------------------------------------------------------------------*/
static struct trickle_timer *loctt;   /* Pointer to a struct for local use */
static clock_time_t loc_clock; /* A local, general-purpose placeholder */

static void fire(struct net_buf *buf, void *ptr);
static void double_interval(struct net_buf *buf, void *ptr);
/*---------------------------------------------------------------------------*/
/* Local utilities and functions to be used as ctimer callbacks */
/*---------------------------------------------------------------------------*/
#if TRICKLE_TIMER_WIDE_RAND
/* Returns a 4-byte wide, unsigned random number */
static uint32_t
wide_rand(void)
{
  return ((uint32_t)random_rand() << 16 | random_rand());
}
#endif
/*---------------------------------------------------------------------------*/
/*
 * Returns the maximum sane Imax value for a given Imin
 *
 * This function is a variant of a fairly standard 'Count Leading Zeros'. It
 * has three flavours. The most suitable one for a specific platform can be
 * configured by changing the value of TRICKLE_TIMER_CONF_MAX_IMAX_WIDTH
 * in the platform's contiki-conf.h
 */
#if TRICKLE_TIMER_ERROR_CHECKING
static uint8_t
max_imax(clock_time_t value)
{
  uint8_t zeros = 0;
#if (TRICKLE_TIMER_MAX_IMAX_WIDTH==TRICKLE_TIMER_MAX_IMAX_GENERIC)
  uint8_t i;
  clock_time_t mask = 0xFFFF;

  value--;

  for(i = sizeof(clock_time_t) << 2; i > 0; i >>= 1) {
    if((value & (mask <<= i)) == 0) {
      zeros += i;
      value <<= i;
    }
  }

#elif (TRICKLE_TIMER_MAX_IMAX_WIDTH==TRICKLE_TIMER_MAX_IMAX_16_BIT)
  if((value & 0xFF00) == 0) {
    zeros += 8;
    value <<= 8;
  }
  if((value & 0xF000) == 0) {
    zeros += 4;
    value <<= 4;
  }
  if((value & 0xC000) == 0) {
    zeros += 2;
    value <<= 2;
  }
  if((value & 0x8000) == 0) {
    zeros++;
  }
#elif (TRICKLE_TIMER_MAX_IMAX_WIDTH==TRICKLE_TIMER_MAX_IMAX_32_BIT)
  if((value & 0xFFFF0000) == 0) {
    zeros += 16;
    value <<= 16;
  }
  if((value & 0xFF000000) == 0) {
    zeros += 8;
    value <<= 8;
  }
  if((value & 0xF0000000) == 0) {
    zeros += 4;
    value <<= 4;
  }
  if((value & 0xC0000000) == 0) {
    zeros += 2;
    value <<= 2;
  }
  if((value & 0x80000000) == 0) {
    zeros += 1;
  }
#endif

  return zeros - 1; /* Always non-negative due to the range of 'value' */
}
#endif /* TRICKLE_TIMER_ERROR_CHECKING */
/*---------------------------------------------------------------------------*/
/* Returns a random time point t in [I/2 , I) */
static clock_time_t
get_t(clock_time_t i_cur)
{
  i_cur >>= 1;

  PRINTF("trickle_timer get t: [%lu, %lu)\n", (unsigned long)i_cur,
         (unsigned long)(i_cur << 1));

  return i_cur + (tt_rand() % i_cur);
}
/*---------------------------------------------------------------------------*/
static void
schedule_for_end(struct trickle_timer *tt)
{
  /* Reset our ctimer, schedule interval_end to run at time I */
  clock_time_t now = clock_time();

  loc_clock = TRICKLE_TIMER_INTERVAL_END(tt) - now;

  PRINTF("trickle_timer sched for end: at %lu, end in %ld\n",
         (unsigned long)clock_time(), (signed long)loc_clock);

  /* Interval's end will happen in loc_clock ticks. Make sure this isn't in
   * the past... */
  if(loc_clock > (TRICKLE_TIMER_CLOCK_MAX >> 1)) {
    loc_clock = 0; /* Interval ended in the past, schedule for in 0 */
    PRINTF("trickle_timer doubling: Was in the past. Compensating\n");
  }

  ctimer_set(NULL, &tt->ct, loc_clock, double_interval, tt);
}
/*---------------------------------------------------------------------------*/
/* This is used as a ctimer callback, thus its argument must be void *. ptr is
 * a pointer to the struct trickle_timer that fired */
static void
double_interval(struct net_buf *buf, void *ptr)
{
  clock_time_t last_end;

  /* 'cast' ptr to a struct trickle_timer */
  loctt = (struct trickle_timer *)ptr;

  loctt->c = 0;

  PRINTF("trickle_timer doubling: at %lu, (was for %lu), ",
         (unsigned long)clock_time(),
         (unsigned long)TRICKLE_TIMER_INTERVAL_END(loctt));

  /* Remember the previous interval's end (absolute time), before we double */
  last_end = TRICKLE_TIMER_INTERVAL_END(loctt);

  /* Double the interval if we have to */
  if(loctt->i_cur <= TRICKLE_TIMER_INTERVAL_MAX(loctt) >> 1) {
    /* If I <= Imax/2, we double */
    loctt->i_cur <<= 1;
    PRINTF("I << 1 = %lu\n", (unsigned long)loctt->i_cur);
  } else {
    /* We may have I > Imax/2 but I <> Imax, in which case we set to Imax
     * This will happen when I didn't start as Imin (before the first reset) */
    loctt->i_cur = TRICKLE_TIMER_INTERVAL_MAX(loctt);
    PRINTF("I = Imax = %lu\n", (unsigned long)loctt->i_cur);
  }

  /* Random t in [I/2, I) */
  loc_clock = get_t(loctt->i_cur);

  PRINTF("trickle_timer doubling: t=%lu\n", (unsigned long)loc_clock);

#if TRICKLE_TIMER_COMPENSATE_DRIFT
  /* Schedule for t ticks after the previous interval's end, not after now. If
   * that is in the past, schedule in 0 */
  loc_clock = (last_end + loc_clock) - clock_time();
  PRINTF("trickle_timer doubling: at %lu, in %ld ticks\n",
         (unsigned long)clock_time(), (signed long)loc_clock);
  if(loc_clock > (TRICKLE_TIMER_CLOCK_MAX >> 1)) {
    /* Oops, that's in the past */
    loc_clock = 0;
    PRINTF("trickle_timer doubling: Was in the past. Compensating\n");
  }
  ctimer_set(NULL, &loctt->ct, loc_clock, fire, loctt);

  /* Store the actual interval start (absolute time), we need it later.
   * We pretend that it started at the same time when the last one ended */
  loctt->i_start = last_end;
#else
  /* Assumed that the previous interval's end is 'now' and schedule in t ticks
   * after 'now', ignoring potential offsets */
  ctimer_set(NULL, &loctt->ct, loc_clock, fire, loctt);
  /* Store the actual interval start (absolute time), we need it later */
  loctt->i_start = loctt->ct.etimer.timer.start;
#endif

  PRINTF("trickle_timer doubling: Last end %lu, new end %lu, for %lu, I=%lu\n",
         (unsigned long)last_end,
         (unsigned long)TRICKLE_TIMER_INTERVAL_END(loctt),
         (unsigned long)(loctt->ct.etimer.timer.start +
                         loctt->ct.etimer.timer.interval),
         (unsigned long)(loctt->i_cur));
}
/*---------------------------------------------------------------------------*/
/* Called by the ctimer module at time t within the current interval. ptr is
 * a pointer to the struct trickle_timer of interest */
static void
fire(struct net_buf *buf, void *ptr)
{
  /* 'cast' c to a struct trickle_timer */
  loctt = (struct trickle_timer *)ptr;

  PRINTF("trickle_timer fire: at %lu (was for %lu)\n",
         (unsigned long)clock_time(),
         (unsigned long)(loctt->ct.etimer.timer.start +
                         loctt->ct.etimer.timer.interval));

  if(loctt->cb) {
    /*
     * Call the protocol's TX callback, with the suppression status as an
     * argument.
     */
    PRINTF("trickle_timer fire: Suppression Status %u (%u < %u)\n",
           TRICKLE_TIMER_PROTO_TX_ALLOW(loctt), loctt->c, loctt->k);
    loctt->cb(loctt->cb_arg, TRICKLE_TIMER_PROTO_TX_ALLOW(loctt));
  }

  if(trickle_timer_is_running(loctt)) {
    schedule_for_end(loctt);
  }
}
/*---------------------------------------------------------------------------*/
/* New trickle interval, either due to a newly set trickle timer or due to an
 * inconsistency. Schedule 'fire' to be called in t ticks. */
static void
new_interval(struct trickle_timer *tt)
{
  tt->c = 0;

  /* Random t in [I/2, I) */
  loc_clock = get_t(tt->i_cur);

  ctimer_set(NULL, &tt->ct, loc_clock, fire, tt);

  /* Store the actual interval start (absolute time), we need it later */
  tt->i_start = tt->ct.etimer.timer.start;
  PRINTF("trickle_timer new interval: at %lu, ends %lu, ",
         (unsigned long)clock_time(),
         (unsigned long)TRICKLE_TIMER_INTERVAL_END(tt));
  PRINTF("t=%lu, I=%lu\n", (unsigned long)loc_clock, (unsigned long)tt->i_cur);
}
/*---------------------------------------------------------------------------*/
/* Functions to be called by the protocol implementation */
/*---------------------------------------------------------------------------*/
void
trickle_timer_consistency(struct trickle_timer *tt)
{
  if(tt->c < 0xFF) {
    tt->c++;
  }
  PRINTF("trickle_timer consistency: c=%u\n", tt->c);
}
/*---------------------------------------------------------------------------*/
void
trickle_timer_inconsistency(struct trickle_timer *tt)
{
  /* "If I is equal to Imin when Trickle hears an "inconsistent" transmission,
   * Trickle does nothing." */
  if(tt->i_cur != tt->i_min) {
    PRINTF("trickle_timer inconsistency\n");
    tt->i_cur = tt->i_min;

    new_interval(tt);
  }
}
/*---------------------------------------------------------------------------*/
uint8_t
trickle_timer_config(struct trickle_timer *tt, clock_time_t i_min,
                     uint8_t i_max, uint8_t k)
{
#if TRICKLE_TIMER_ERROR_CHECKING
  /*
   * Although in theory Imin=1 is a valid value, it would break get_t() and
   * doesn't make sense anyway. Thus Valid Imin values are in the range:
   * 1 < Imin <= (TRICKLE_TIMER_CLOCK_MAX >> 1) + 1
   */
  if(TRICKLE_TIMER_IMIN_IS_BAD(i_min)) {
    PRINTF("trickle_timer config: Bad Imin value\n");
    return TRICKLE_TIMER_ERROR;
  }

  if(tt == NULL || i_max == 0 || k == 0) {
    PRINTF("trickle_timer config: Bad arguments\n");
    return TRICKLE_TIMER_ERROR;
  }

  /*
   * If clock_time_t is not wide enough to store Imin << Imax, we adjust Imax
   *
   * This means that 'we' are likely to have a different Imax than 'them'
   * See RFC 6206, sec 6.3 for the consequences of this situation
   */
  if(TRICKLE_TIMER_IPAIR_IS_BAD(i_min, i_max)) {
    PRINTF("trickle_timer config: %lu << %u would exceed clock boundaries. ",
           (unsigned long)i_min, i_max);

    /* For this Imin, get the maximum sane Imax */
    i_max = max_imax(i_min);
    PRINTF("trickle_timer config: Using Imax=%u\n", i_max);
  }
#endif

  tt->i_min = i_min;
  tt->i_max = i_max;
  tt->i_max_abs = i_min << i_max;
  tt->k = k;

  PRINTF("trickle_timer config: Imin=%lu, Imax=%u, k=%u\n",
         (unsigned long)tt->i_min, tt->i_max, tt->k);

  return TRICKLE_TIMER_SUCCESS;
}
/*---------------------------------------------------------------------------*/
uint8_t
trickle_timer_set(struct trickle_timer *tt, trickle_timer_cb_t proto_cb,
                  void *ptr)
{
#if TRICKLE_TIMER_ERROR_CHECKING
  /* Sanity checks */
  if(tt == NULL || proto_cb == NULL) {
    PRINTF("trickle_timer set: Bad arguments\n");
    return TRICKLE_TIMER_ERROR;
  }
#endif

  tt->cb = proto_cb;
  tt->cb_arg = ptr;

  /* Random I in [Imin , Imax] */
  tt->i_cur = tt->i_min +
    (tt_rand() % (TRICKLE_TIMER_INTERVAL_MAX(tt) - tt->i_min + 1));

  PRINTF("trickle_timer set: I=%lu in [%lu , %lu]\n", (unsigned long)tt->i_cur,
         (unsigned long)tt->i_min,
         (unsigned long)TRICKLE_TIMER_INTERVAL_MAX(tt));

  new_interval(tt);

  PRINTF("trickle_timer set: at %lu, ends %lu, t=%lu in [%lu , %lu)\n",
         (unsigned long)tt->i_start,
         (unsigned long)TRICKLE_TIMER_INTERVAL_END(tt),
         (unsigned long)tt->ct.etimer.timer.interval,
         (unsigned long)tt->i_cur >> 1, (unsigned long)tt->i_cur);

  return TRICKLE_TIMER_SUCCESS;
}
/*---------------------------------------------------------------------------*/
/** @} */
