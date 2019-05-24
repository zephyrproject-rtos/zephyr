/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

#ifndef _REGACCESS_H
#define _REGACCESS_H

#include <stdint.h>

#define MMCR32(a)   *((volatile uint32_t *)(uintptr_t)(a))
#define MMCR16(a)   *((volatile uint16_t *)(uintptr_t)(a))
#define MMCR8(a)    *((volatile uint8_t *)(uintptr_t)(a))

#define MMCR_RD32(a, v)   v = *((volatile uint32_t *)(uintptr_t)(a))
#define MMCR_RD16(a, v)   v = *((volatile uint16_t *)(uintptr_t)(a))
#define MMCR_RD8(a, v)    v = *((volatile uint8_t *)(uintptr_t)(a))

#define MMCR_WR32(a, d)   *((volatile uint32_t *)(uintptr_t)(a)) = (uint32_t)(d)
#define MMCR_WR16(a, h)   *((volatile uint16_t *)(uintptr_t)(a)) = (uint16_t)(h)
#define MMCR_WR8(a, b)    *((volatile uint8_t *)(uintptr_t)(a)) = (uint8_t)(b)

#define REG32(a)    *((volatile uint32_t *)(uintptr_t)(a))
#define REG16(a)    *((volatile uint16_t *)(uintptr_t)(a))
#define REG8(a)     *((volatile uint8_t *)(uintptr_t)(a))

#define REG32W(a, d)    *((volatile uint32_t *)(uintptr_t)(a)) = (uint32_t)(d)
#define REG16W(a, h)    *((volatile uint16_t *)(uintptr_t)(a)) = (uint16_t)(h)
#define REG8W(a, b)     *((volatile uint8_t *)(uintptr_t)(a)) = (uint8_t)(b)

#define REG32R(a, d)    (d) = *(volatile uint32_t *)(uintptr_t)(a)
#define REG16R(a, h)    (h) = *(volatile uint16_t *)(uintptr_t)(a)
#define REG8R(a, b)     (b) = *(volatile uint8_t *)(uintptr_t)(a)

#define REG32_OFS(a, ofs)   *(volatile uint32_t *)((uintptr_t)(a) + (uintptr_t)(ofs))
#define REG16_OFS(a, ofs)   *(volatile uint16_t *)((uintptr_t)(a) + (uintptr_t)(ofs))
#define REG8_OFS(a, ofs)    *(volatile uint8_t *)((uintptr_t)(a) + (uintptr_t)(ofs))


#endif // #ifndef _REGACCESS_H
