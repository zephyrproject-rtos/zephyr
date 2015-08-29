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


void nano_timer_init(struct nano_timer *timer, void *data)
{
	nano_lifo_init(&timer->lifo);
	timer->userData = data;
}


FUNC_ALIAS(_timer_start, nano_fiber_timer_start, void);

/**
 * @brief Start a nanokernel timer from a task
 *
 * This function starts a previously initialized nanokernel timer object.
 * The timer will expire in <ticks> system clock ticks.
 *
 * @return N/A
 */
FUNC_ALIAS(_timer_start, nano_task_timer_start, void);

/**
 *
 * @brief Start a nanokernel timer (generic implementation)
 *
 * This function starts a previously initialized nanokernel timer object.
 * The timer will expire in <ticks> system clock ticks.
 *
 * @return N/A
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

	imask = irq_lock();

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

	irq_unlock(imask);
}

/**
 * @brief Stop a nanokernel timer (generic implementation)
 *
 * This function stops a previously started nanokernel timer object.
 * @param timer Timer to stop
 * @return N/A
 */
static void _timer_stop(struct nano_timer *timer)
{
	unsigned int imask;
	struct nano_timer *cur;
	struct nano_timer *prev = NULL;

	imask = irq_lock();

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

	irq_unlock(imask);
}


void nano_fiber_timer_stop(struct nano_timer *timer)
{

	_timer_stop(timer);

	/* if there was a waiter, kick it */
	if (timer->lifo.wait_q.head) {
		nano_fiber_lifo_put(&timer->lifo, (void *)0);
	}
}

void nano_task_timer_stop(struct nano_timer *timer)
{

	_timer_stop(timer);

	/* if there was a waiter, kick it */
	if (timer->lifo.wait_q.head) {
		nano_task_lifo_put(&timer->lifo, (void *)0);
	}
}


void *nano_fiber_timer_test(struct nano_timer *timer)
{
	return nano_fiber_lifo_get(&timer->lifo);
}


void *nano_fiber_timer_wait(struct nano_timer *timer)
{
	return nano_fiber_lifo_get_wait(&timer->lifo);
}

void *nano_task_timer_test(struct nano_timer *timer)
{
	return nano_task_lifo_get(&timer->lifo);
}

void *nano_task_timer_wait(struct nano_timer *timer)
{
	return nano_task_lifo_get_wait(&timer->lifo);
}
