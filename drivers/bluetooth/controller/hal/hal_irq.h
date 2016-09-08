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

#ifndef _IRQ_H_
#define _IRQ_H_

void irq_enable(uint8_t irq);
void irq_disable(uint8_t irq);
void irq_pending_set(uint8_t irq);
uint8_t irq_enabled(uint8_t irq);
uint8_t irq_priority_equal(uint8_t irq);

#endif /* _IRQ_H_ */
