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

#ifndef _MEM_H_
#define _MEM_H_

void mem_init(void *mem_pool, uint16_t mem_size, uint16_t mem_count,
	      void **mem_head);
void *mem_acquire(void **mem_head);
void mem_release(void *mem, void **mem_head);

uint16_t mem_free_count_get(void *mem_head);
void *mem_get(void *mem_pool, uint16_t mem_size, uint16_t index);
uint16_t mem_index_get(void *mem, void *mem_pool, uint16_t mem_size);

void mem_rcopy(uint8_t *dst, uint8_t const *src, uint16_t len);
uint8_t mem_is_zero(uint8_t *src, uint16_t len);

uint32_t mem_ut(void);

#endif /* _MEM_H_ */
