/*
 * Copyright (c) 2016 BayLibre, SAS
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

#ifndef _STM32L4X_SYSCFG_REGISTERS_H_
#define _STM32L4X_SYSCFG_REGISTERS_H_

#define STM32L4X_SYSCFG_EXTICR_PIN_MASK		7

/* SYSCFG registers */
struct stm32l4x_syscfg {
	uint32_t memrmp;
	uint32_t cfgr1;
	uint32_t exticr1;
	uint32_t exticr2;
	uint32_t exticr3;
	uint32_t exticr4;
	uint32_t scsr;
	uint32_t cfgr2;
	uint32_t swpr;
	uint32_t skr;
};

#endif /* _STM32L4X_SYSCFG_REGISTERS_H_ */
