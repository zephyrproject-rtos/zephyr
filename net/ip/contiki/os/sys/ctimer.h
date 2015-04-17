/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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

/**
 * \file
 *         Header file for the callback timer
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

/**
 * \addtogroup sys
 * @{
 */

/**
 * \defgroup ctimer Callback timer
 * @{
 *
 * The ctimer module provides a timer mechanism that calls a specified
 * C function when a ctimer expires.
 *
 */

#ifndef CTIMER_H_
#define CTIMER_H_

#include "sys/etimer.h"

struct ctimer {
  struct ctimer *next;
  struct etimer etimer;
  struct process *p;
  void (*f)(void *);
  void *ptr;
};

/**
 * \brief      Reset a callback timer with the same interval as was
 *             previously set.
 * \param c    A pointer to the callback timer.
 *
 *             This function resets the callback timer with the same
 *             interval that was given to the callback timer with the
 *             ctimer_set() function. The start point of the interval
 *             is the exact time that the callback timer last
 *             expired. Therefore, this function will cause the timer
 *             to be stable over time, unlike the ctimer_restart()
 *             function.
 *
 * \sa ctimer_restart()
 */
void ctimer_reset(struct ctimer *c);

/**
 * \brief      Restart a callback timer from the current point in time
 * \param c    A pointer to the callback timer.
 *
 *             This function restarts the callback timer with the same
 *             interval that was given to the ctimer_set()
 *             function. The callback timer will start at the current
 *             time.
 *
 *             \note A periodic timer will drift if this function is
 *             used to reset it. For periodic timers, use the
 *             ctimer_reset() function instead.
 *
 * \sa ctimer_reset()
 */
void ctimer_restart(struct ctimer *c);

/**
 * \brief      Set a callback timer.
 * \param c    A pointer to the callback timer.
 * \param t    The interval before the timer expires.
 * \param f    A function to be called when the timer expires.
 * \param ptr  An opaque pointer that will be supplied as an argument to the callback function.
 *
 *             This function is used to set a callback timer for a time
 *             sometime in the future. When the callback timer expires,
 *             the callback function f will be called with ptr as argument.
 *
 */
void ctimer_set(struct ctimer *c, clock_time_t t,
		void (*f)(void *), void *ptr);

/**
 * \brief      Stop a pending callback timer.
 * \param c    A pointer to the pending callback timer.
 *
 *             This function stops a callback timer that has previously
 *             been set with ctimer_set(), ctimer_reset(), or ctimer_restart().
 *             After this function has been called, the callback timer will be
 *             expired and will not call the callback function.
 *
 */
void ctimer_stop(struct ctimer *c);

/**
 * \brief      Check if a callback timer has expired.
 * \param c    A pointer to the callback timer
 * \return     Non-zero if the timer has expired, zero otherwise.
 *
 *             This function tests if a callback timer has expired and
 *             returns true or false depending on its status.
 */
int ctimer_expired(struct ctimer *c);

/**
 * \brief      Initialize the callback timer library.
 *
 *             This function initializes the callback timer library and
 *             should be called from the system boot up code.
 */
void ctimer_init(void);

#endif /* CTIMER_H_ */
/** @} */
/** @} */
