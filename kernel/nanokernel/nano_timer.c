/* nano_timer.c - timer for nanokernel-only systems */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
