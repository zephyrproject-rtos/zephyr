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

#include "nrf.h"
#include "hal_irq.h"

void irq_enable(uint8_t irq)
{
	NVIC_EnableIRQ((IRQn_Type) irq);
}

void irq_disable(uint8_t irq)
{
	NVIC_DisableIRQ((IRQn_Type) irq);
}

void irq_pending_set(uint8_t irq)
{
	NVIC_SetPendingIRQ((IRQn_Type) irq);
}

uint8_t irq_enabled(uint8_t irq)
{
	return ((NVIC->ISER[0] & (1 << ((uint32_t) (irq) & 0x1F))) != 0);
}

uint8_t irq_priority_equal(uint8_t irq)
{
	uint32_t irq_current = SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk;
	uint32_t irq_priority_current = 4;

	if (irq_current > 16) {
		irq_priority_current =
		    NVIC_GetPriority((IRQn_Type) (irq_current - 16)) & 0xFF;
	}

	return ((NVIC_GetPriority((IRQn_Type) irq) & 0xFF) ==
		irq_priority_current);
}
