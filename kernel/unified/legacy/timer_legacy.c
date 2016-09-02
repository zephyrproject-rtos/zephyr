/*
 * Copyright (c) 2016 Wind River Systems, Inc.
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

#include <kernel.h>

void task_timer_start(ktimer_t timer, int32_t duration,
		      int32_t period, ksem_t sema)
{
	if (duration < 0 || period < 0 || (duration == 0 && period == 0)) {
		int key = irq_lock();

		if (timer->timeout.delta_ticks_from_prev != -1) {
			k_timer_stop(timer);
		}

		irq_unlock(key);

		return;
	}

	k_timer_start(timer, _ticks_to_ms(duration),
		      _ticks_to_ms(period),
		      (void(*)(void *))k_sem_give, sema, NULL, NULL);
}
