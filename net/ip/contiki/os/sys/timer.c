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
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/**
 * \file
 * Timer library implementation.
 * \author
 * Adam Dunkels <adam@sics.se>
 */

/**
 * \addtogroup timer
 * @{
 */

#include "contiki-conf.h"
#include "sys/clock.h"
#include "sys/timer.h"

#define DEBUG DEBUG_NONE
#include "contiki/ip/uip-debug.h"

/*
 * Code indicates that the timer has expired.
 * nano_timer_test(), called by timer_expired() returns it
 * on timer expiration instead of NULL if timer is still running
 */
#define TIMER_EXPIRED_CODE ((void *)0xfede0123)

static inline void do_init(struct timer *t)
{
  if (t && !t->init_done) {
    nano_timer_init(&t->nano_timer, TIMER_EXPIRED_CODE);
    t->init_done = true;
    t->expired = false;
  }
}

/*---------------------------------------------------------------------------*/
/**
 * Set a timer.
 *
 * This function is used to set a timer for a time sometime in the
 * future. The function timer_expired() will evaluate to true after
 * the timer has expired.
 *
 * \param t A pointer to the timer
 * \param interval The interval before the timer expires.
 *
 */
void
timer_set(struct timer *t, clock_time_t interval)
{
  do_init(t);

  if (t->started) {
    timer_stop(t);
  }

  switch (sys_execution_context_type_get()) {
  case NANO_CTX_FIBER:
    nano_fiber_timer_start(&t->nano_timer, interval);
    break;
  case NANO_CTX_TASK:
    nano_task_timer_start(&t->nano_timer, interval);
    break;
  default:
    return;
  }

  PRINTF("%s():%d timer %p started interval %d\n", __FUNCTION__, __LINE__,
	 t, interval);
  t->started = true;
  t->interval = interval;
  t->start = clock_time();
  t->triggered = false;
  t->expired = false;
}
/*---------------------------------------------------------------------------*/
/**
 * Reset the timer with the same interval.
 *
 * This function resets the timer with the same interval that was
 * given to the timer_set() function. The start point of the interval
 * is the exact time that the timer last expired. Therefore, this
 * function will cause the timer to be stable over time, unlike the
 * timer_restart() function.
 *
 * \note Must not be executed before timer expired
 *
 * \param t A pointer to the timer.
 * \sa timer_restart()
 */
void
timer_reset(struct timer *t)
{
  timer_stop(t);
  timer_set(t, t->interval);
}
/*---------------------------------------------------------------------------*/
/**
 * Restart the timer from the current point in time
 *
 * This function restarts a timer with the same interval that was
 * given to the timer_set() function. The timer will start at the
 * current time.
 *
 * \note A periodic timer will drift if this function is used to reset
 * it. For preioric timers, use the timer_reset() function instead.
 *
 * \param t A pointer to the timer.
 *
 * \sa timer_reset()
 */
void
timer_restart(struct timer *t)
{
  do_init(t);

  if (t->started) {
    timer_stop(t);
  }

  switch (sys_execution_context_type_get()) {
  case NANO_CTX_FIBER:
    nano_fiber_timer_start(&t->nano_timer, t->interval);
    break;
  case NANO_CTX_TASK:
    nano_task_timer_start(&t->nano_timer, t->interval);
    break;
  default:
    return;
  }
  t->started = true;
  t->start = clock_time();
  t->triggered = false;
  t->expired = false;
}
/*---------------------------------------------------------------------------*/
/**
 * Check if a timer has expired.
 *
 * This function tests if a timer has expired and returns true or
 * false depending on its status.
 *
 * \param t A pointer to the timer
 *
 * \return Non-zero if the timer has expired, zero otherwise.
 *
 */
int
timer_expired(struct timer *t)
{
  void *user_data;
  bool init_done = t->init_done;

  do_init(t);

  if (t->expired) {
    return true;
  }

  user_data = nano_timer_test(&t->nano_timer, TICKS_NONE);

  /* If user data is returned, then we are definitely expired. */
  if (user_data == TIMER_EXPIRED_CODE) {
    t->expired = true;
    return true;
  }

  /* If TICKS_NONE is used, then the system returns NULL when the
   * timer has not yet run. In this case we return true value so
   * that the caller can schedule the timer again.
   *
   * Note that it is absolutely mandatory that timer struct is
   * initialized before use. The init_done must reflect the real
   * status of the timer.
   */
  if (!init_done && user_data == NULL) {
    t->expired = true;
    return true;
  }

  return false;
}
/*---------------------------------------------------------------------------*/
bool timer_is_triggered(struct timer *t)
{
  return t->triggered;
}

void timer_set_triggered(struct timer *t)
{
  t->triggered = true;
}
/*---------------------------------------------------------------------------*/
/**
 * The time until the timer expires
 *
 * This function returns the time until the timer expires.
 *
 * \param t A pointer to the timer
 *
 * \return The time until the timer expires
 *
 */
clock_time_t
timer_remaining(struct timer *t)
{
  return nano_timer_ticks_remain(&t->nano_timer);
}
/*---------------------------------------------------------------------------*/
bool timer_stop(struct timer *t)
{
  if (!t->started) {
    return false;
  }

  switch (sys_execution_context_type_get()) {
  case NANO_CTX_FIBER:
    nano_fiber_timer_stop(&t->nano_timer);
    break;
  case NANO_CTX_TASK:
    nano_task_timer_stop(&t->nano_timer);
    break;
  default:
    return false;
  }

  PRINTF("%s():%d timer %p stopped\n", __FUNCTION__, __LINE__, t);

  t->started = false;
  t->triggered = false;
  return true;
}
/** @} */
