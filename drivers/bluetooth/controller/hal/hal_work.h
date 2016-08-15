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

#ifndef _HAL_WORK_H_
#define _HAL_WORK_H_

#include "nrf.h"

#define WORK_TICKER_WORKER0_IRQ	RTC0_IRQn
#define WORK_TICKER_JOB0_IRQ	SWI4_IRQn
#define WORK_TICKER_WORKER1_IRQ	SWI5_IRQn
#define WORK_TICKER_JOB1_IRQ	SWI5_IRQn

#define WORK_TICKER_WORKER0_IRQ_PRIORITY	0
#define WORK_TICKER_JOB0_IRQ_PRIORITY		0
#define WORK_TICKER_WORKER1_IRQ_PRIORITY	2
#define WORK_TICKER_JOB1_IRQ_PRIORITY		2

#endif /* _HAL_WORK_H_ */
