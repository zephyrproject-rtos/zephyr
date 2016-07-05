/* wait queue for multiple fibers on nanokernel objects */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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



#include <wait_q.h>


/*
 * Remove first fiber from a wait queue and put it on the ready queue, knowing
 * that the wait queue is not empty.
 */

static struct tcs *_nano_wait_q_remove_no_check(struct _nano_queue *wait_q)
{
	struct tcs *tcs = wait_q->head;

	if (wait_q->tail == wait_q->head) {
		_nano_wait_q_reset(wait_q);
	} else {
		wait_q->head = tcs->link;
	}
	tcs->link = 0;

	_nano_fiber_ready(tcs);
	return tcs;
}

/*
 * Remove first fiber from a wait queue and put it on the ready queue.
 * Abort and return NULL if the wait queue is empty.
 */
struct tcs *_nano_wait_q_remove(struct _nano_queue *wait_q)
{
	return wait_q->head ? _nano_wait_q_remove_no_check(wait_q) : NULL;
}
