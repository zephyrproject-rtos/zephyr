/* nano_timer.c - timer for nanokernel-only systems */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nano_private.h>

struct nano_timer *_nano_timer_list = NULL;

/**
 *
 * nano_timer_init - initialize a nanokernel timer object
 *
 * This function initializes a nanokernel timer object structure.
 *
 * It may be called from either a fiber or task context.
 *
 * The <userData> passed to this function must have enough space for a pointer
 * in its first field, that may be overwritten when the timer expires, plus
 * whatever data the user wishes to store and recover when the timer expires.
 *
 * RETURNS: N/A
 *
 */

void nano_timer_init(struct nano_timer *timer, void *userData)
{
	nano_lifo_init(&timer->lifo);
	timer->userData = userData;
}

/**
 *
 * nano_fiber_timer_start - start a nanokernel timer from a fiber
 *
 * This function starts a previously initialized nanokernel timer object.
 * The timer will expire in <ticks> system clock ticks.
 *
 * RETURNS: N/A
 *
 */

FUNC_ALIAS(_timer_start, nano_fiber_timer_start, void);

/**
 *
 * nano_task_timer_start - start a nanokernel timer from a task
 *
 * This function starts a previously initialized nanokernel timer object.
 * The timer will expire in <ticks> system clock ticks.
 *
 * RETURNS: N/A
 *
 */

FUNC_ALIAS(_timer_start, nano_task_timer_start, void);

/**
 *
 * _timer_start - start a nanokernel timer (generic implementation)
 *
 * This function starts a previously initialized nanokernel timer object.
 * The timer will expire in <ticks> system clock ticks.
 *
 * RETURNS: N/A
 *
 * NOMANUAL
 */

void _timer_start(struct nano_timer *timer, /* timer to start */
				       int ticks /* number of system ticks
						    before expiry */
				       )
{
	unsigned int imask;
	struct nano_timer *cur;
	struct nano_timer *prev = NULL;

	timer->ticks = ticks;

	imask = irq_lock_inline();

	cur = _nano_timer_list;

	while (cur && (timer->ticks > cur->ticks)) {
		timer->ticks -= cur->ticks;
		prev = cur;
		cur = cur->link;
	}

	timer->link = cur;
	if (cur != NULL)
		cur->ticks -= timer->ticks;

	if (prev != NULL)
		prev->link = timer;
	else
		_nano_timer_list = timer;

	irq_unlock_inline(imask);
}

/**
 *
 * _timer_stop - stop a nanokernel timer (generic implementation)
 *
 * This function stops a previously started nanokernel timer object.
 *
 * RETURNS: N/A
 *
 * NOMANUAL
 */

static void _timer_stop(struct nano_timer *timer /* timer to stop */
				     )
{
	unsigned int imask;
	struct nano_timer *cur;
	struct nano_timer *prev = NULL;

	imask = irq_lock_inline();

	cur = _nano_timer_list;

	/* find prev */
	while (cur && cur != timer) {
		prev = cur;
		cur = cur->link;
	}

	/* if found it, remove it */
	if (cur) {
		/* if it was first */
		if (prev == NULL) {
			_nano_timer_list = timer->link;
			/* if not last */
			if (_nano_timer_list)
				_nano_timer_list->ticks += timer->ticks;
		} else {
			prev->link = timer->link;
			/* if not last */
			if (prev->link)
				prev->link->ticks += timer->ticks;
		}
	}

	/* now the timer can't expire since it is removed from the list */

	irq_unlock_inline(imask);
}

/**
 *
 * nano_fiber_timer_stop - stop a nanokernel timer from a fiber
 *
 * This function stops a previously started nanokernel timer object.
 *
 * RETURNS: N/A
 *
 */

void nano_fiber_timer_stop(struct nano_timer *timer /* timer to stop */
			)
{

	_timer_stop(timer);

	/* if there was a waiter, kick it */
	if (timer->lifo.wait_q.head) {
		nano_fiber_lifo_put(&timer->lifo, (void *)0);
	}
}

/**
 *
 * nano_task_timer_stop - stop a nanokernel timer from a task
 *
 * This function stops a previously started nanokernel timer object.
 *
 * RETURNS: N/A
 *
 */

void nano_task_timer_stop(struct nano_timer *timer /* timer to stop */
		       )
{

	_timer_stop(timer);

	/* if there was a waiter, kick it */
	if (timer->lifo.wait_q.head) {
		nano_task_lifo_put(&timer->lifo, (void *)0);
	}
}

/**
 *
 * nano_fiber_timer_test - make the current fiber check for a timer expiry
 *
 * This function will check if a timer has expired. The timer must
 * have been initialized by nano_timer_init() and started via either
 * nano_fiber_timer_start() or nano_task_timer_start() first.
 *
 * RETURNS: pointer to timer initialization data, or NULL if timer not expired
 *
 */

void *nano_fiber_timer_test(struct nano_timer *timer /* timer to check */
			)
{
	return nano_fiber_lifo_get(&timer->lifo);
}

/**
 *
 * nano_fiber_timer_wait - make the current fiber wait for a timer to expire
 *
 * This function will pend on a timer if it hasn't expired yet. The timer must
 * have been initialized by nano_timer_init() and started via either
 * nano_fiber_timer_start() or nano_task_timer_start() first and must not
 * have been stopped via nano_task_timer_stop() or nano_fiber_timer_stop().
 *
 * RETURNS: pointer to timer initialization data
 *
 */

void *nano_fiber_timer_wait(struct nano_timer *timer /* timer to pend on */
			 )
{
	return nano_fiber_lifo_get_wait(&timer->lifo);
}

/**
 *
 * nano_task_timer_test - make the current task check for a timer expiry
 *
 * This function will check if a timer has expired. The timer must
 * have been initialized by nano_timer_init() and started via either
 * nano_fiber_timer_start() or nano_task_timer_start() first.
 *
 * RETURNS: pointer to timer initialization data, or NULL if timer not expired
 *
 */

void *nano_task_timer_test(struct nano_timer *timer /* timer to check */
		       )
{
	return nano_task_lifo_get(&timer->lifo);
}

/**
 *
 * nano_task_timer_wait - make the current task wait for a timer to expire
 *
 * This function will pend on a timer if it hasn't expired yet. The timer must
 * have been initialized by nano_timer_init() and started via either
 * nano_fiber_timer_start() or nano_task_timer_start() first and must not
 * have been stopped via nano_task_timer_stop() or nano_fiber_timer_stop().
 *
 * RETURNS: pointer to timer initialization data
 *
 */

void *nano_task_timer_wait(struct nano_timer *timer /* timer to pend on */
			)
{
	return nano_task_lifo_get_wait(&timer->lifo);
}
