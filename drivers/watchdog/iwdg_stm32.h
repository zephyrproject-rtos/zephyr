/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
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

#ifndef _STM32_IWDG_H_
#define _STM32_IWDG_H_

#include <stdint.h>

/**
 * @brief Driver for Independent Watchdog (IWDG) for STM32 MCUs
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 19: Independent watchdog (IWDG)
 *
 */


/* counter reload trigger */
#define STM32_IWDG_KR_RELOAD 0xaaaa
/* magic value for unlocking write access to PR and RLR */
#define STM32_IWDG_KR_UNLOCK 0x5555
/* watchdog start */
#define STM32_IWDG_KR_START  0xcccc

/* 19.4.1 IWDG_KR */
union __iwdg_kr {
	uint32_t val;
	struct {
		uint16_t key;
		uint16_t rsvd;
	} bit;
};

/* 19.4.2 IWDG_PR */
union __iwdg_pr {
	uint32_t val;
	struct {
		uint32_t pr :3 __packed;
		uint32_t rsvd__3_31 :29 __packed;
	} bit;
};

/* 19.4.3 IWDG_RLR */
union __iwdg_rlr {
	uint32_t val;
	struct {
		uint32_t rl :12 __packed;
		uint32_t rsvd__12_31 :20 __packed;
	} bit;
};

/* 19.4.4 IWDG_SR */
union __iwdg_sr {
	uint32_t val;
	struct {
		uint32_t pvu :1 __packed;
		uint32_t rvu :1 __packed;
		uint32_t rsvd__2_31 :30 __packed;
	} bit;
};

/* 19.4.5 IWDG register map */
struct iwdg_stm32 {
	union __iwdg_kr kr;
	union __iwdg_pr pr;
	union __iwdg_rlr rlr;
	union __iwdg_sr sr;
};

#endif	/* _STM32_IWDG_H_ */
