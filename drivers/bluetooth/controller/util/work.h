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

#ifndef _WORK_H_
#define _WORK_H_

typedef void (*work_fp) (void *params);

struct work {
	void *next;
	uint8_t req;
	uint8_t ack;
	uint8_t group;
	work_fp fp;
	void *params;
};

void work_enable(uint8_t group);
void work_disable(uint8_t group);
uint32_t work_is_enabled(uint8_t group);
uint32_t work_schedule(struct work *w, uint8_t chain);
void work_run(uint8_t group);

#endif /* _WORK_H_ */
