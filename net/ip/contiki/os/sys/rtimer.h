/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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
 *         Header file for the real-time timer module.
 * \author
 *         Adam Dunkels <adam@sics.se>
 *
 */

/** \addtogroup sys
 * @{ */

/**
 * \defgroup rt Real-time task scheduling
 *
 * The real-time module handles the scheduling and execution of
 * real-time tasks (with predictable execution times).
 *
 * @{
 */

#ifndef RTIMER_H_
#define RTIMER_H_

#include "contiki-conf.h"

#ifndef RTIMER_CLOCK_LT
typedef unsigned short rtimer_clock_t;
#define RTIMER_CLOCK_LT(a,b)     ((signed short)((a)-(b)) < 0)
#endif /* RTIMER_CLOCK_LT */

#include "rtimer-arch.h"

/**
 * \brief      Initialize the real-time scheduler.
 *
 *             This function initializes the real-time scheduler and
 *             must be called at boot-up, before any other functions
 *             from the real-time scheduler is called.
 */
void rtimer_init(void);

struct rtimer;
typedef void (* rtimer_callback_t)(struct rtimer *t, void *ptr);

/**
 * \brief      Representation of a real-time task
 *
 *             This structure represents a real-time task and is used
 *             by the real-time module and the architecture specific
 *             support module for the real-time module.
 */
struct rtimer {
  rtimer_clock_t time;
  rtimer_callback_t func;
  void *ptr;
};

enum {
  RTIMER_OK,
  RTIMER_ERR_FULL,
  RTIMER_ERR_TIME,
  RTIMER_ERR_ALREADY_SCHEDULED,
};

/**
 * \brief      Post a real-time task.
 * \param task A pointer to the task variable previously declared with RTIMER_TASK().
 * \param time The time when the task is to be executed.
 * \param duration Unused argument.
 * \param func A function to be called when the task is executed.
 * \param ptr An opaque pointer that will be supplied as an argument to the callback function.
 * \return     Non-zero (true) if the task could be scheduled, zero
 *             (false) if the task could not be scheduled.
 *
 *             This function schedules a real-time task at a specified
 *             time in the future.
 *
 */
int rtimer_set(struct rtimer *task, rtimer_clock_t time,
	       rtimer_clock_t duration, rtimer_callback_t func, void *ptr);

/**
 * \brief      Execute the next real-time task and schedule the next task, if any
 *
 *             This function is called by the architecture dependent
 *             code to execute and schedule the next real-time task.
 *
 */
void rtimer_run_next(void);

/**
 * \brief      Get the current clock time
 * \return     The current time
 *
 *             This function returns what the real-time module thinks
 *             is the current time. The current time is used to set
 *             the timeouts for real-time tasks.
 *
 * \hideinitializer
 */
#define RTIMER_NOW() rtimer_arch_now()

/**
 * \brief      Get the time that a task last was executed
 * \param task The task
 * \return     The time that a task last was executed
 *
 *             This function returns the time that the task was last
 *             executed. This typically is used to get a periodic
 *             execution of a task without clock drift.
 *
 * \hideinitializer
 */
#define RTIMER_TIME(task) ((task)->time)

void rtimer_arch_init(void);
void rtimer_arch_schedule(rtimer_clock_t t);
/*rtimer_clock_t rtimer_arch_now(void);*/

#define RTIMER_SECOND RTIMER_ARCH_SECOND

#endif /* RTIMER_H_ */

/** @} */
/** @} */
