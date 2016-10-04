/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
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

#include <stdint.h>
#include <irq.h>

#include "work.h"

static struct work *_work_head;

static int _irq_is_priority_equal(unsigned int irq)
{
	unsigned int curr_ctx;
	int curr_prio;

	curr_ctx = _ScbActiveVectorGet();
	if (curr_ctx > 16) {
		/* Interrupts */
		curr_prio = _NvicIrqPrioGet(curr_ctx - 16);
	} else if (curr_ctx > 3) {
		/* Execeptions */
		curr_prio = _ScbExcPrioGet(curr_ctx);
	} else if (curr_ctx > 0) {
		/* Fixed Priority Exceptions: -3, -2, -1 priority */
		curr_prio = curr_ctx - 4;
	} else {
		/* Thread mode */
		curr_prio = 256;
	}

	return (_NvicIrqPrioGet(irq) == curr_prio);
}

void work_enable(uint8_t group)
{
	irq_enable(group);
}

void work_disable(uint8_t group)
{
	irq_disable(group);
}

uint32_t work_is_enabled(uint8_t group)
{
	return irq_is_enabled(group);
}

uint32_t work_schedule(struct work *w, uint8_t chain)
{
	uint32_t imask = irq_lock();
	struct work *prev;
	struct work *curr;

	/* Dequeue expired work at head */
	while ((_work_head)
	       && (_work_head->ack == _work_head->req)
	    ) {
		_work_head = _work_head->next;
	}

	/* Dequeue expired in between list and find last node */
	curr = _work_head;
	prev = curr;
	while (curr) {
		/* delete expired work */
		if (curr->ack == curr->req) {
			prev->next = curr->next;
		} else {
			prev = curr;
		}

		curr = curr->next;
	}

	/* chain, if explicitly requested, or if work not at current level */
	chain = chain || (!_irq_is_priority_equal(w->group))
	    || (!irq_is_enabled(w->group));

	/* Already in List */
	curr = _work_head;
	while (curr) {
		if (curr == w) {
			if (!chain) {
				break;
			}

			irq_unlock(imask);

			return 1;
		}

		curr = curr->next;
	}

	/* handle work(s) that can be inline */
	if (!chain) {
		w->req = w->ack;

		irq_unlock(imask);

		if (w->fp) {
			w->fp(w->params);
		}

		return 0;
	}

	/* New, add to the list */
	w->req = w->ack + 1;
	w->next = 0;
	if (prev == curr) {
		_work_head = w;
	} else {
		prev->next = w;
	}

	_NvicIrqPend(w->group);

	irq_unlock(imask);

	return 0;
}

void work_run(uint8_t group)
{
	uint32_t imask = irq_lock();
	struct work *curr = _work_head;

	while (curr) {
		if ((curr->group == group) && (curr->ack != curr->req)) {
			curr->ack = curr->req;

			if (curr->fp) {
				if (curr->next) {
					_NvicIrqPend(group);
				}

				irq_unlock(imask);

				curr->fp(curr->params);

				return;
			}
		}

		curr = curr->next;
	}

	irq_unlock(imask);
}
