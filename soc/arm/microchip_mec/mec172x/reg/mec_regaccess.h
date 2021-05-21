/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_REGACCESS_H
#define _MEC_REGACCESS_H

#include <stdint.h>

#define MMCR32(a)	*((volatile uint32_t *)(uintptr_t)(a))
#define MMCR16(a)	*((volatile uint16_t *)(uintptr_t)(a))
#define MMCR8(a)	*((volatile uint8_t *)(uintptr_t)(a))

#define MMCR_RD32(a, v)	v = *((volatile uint32_t *)(uintptr_t)(a))
#define MMCR_RD16(a, v)	v = *((volatile uint16_t *)(uintptr_t)(a))
#define MMCR_RD8(a, v)	v = *((volatile uint8_t *)(uintptr_t)(a))

#define MMCR_WR32(a, d)	*((volatile uint32_t *)(uintptr_t)(a)) = (uint32_t)(d)
#define MMCR_WR16(a, h)	*((volatile uint16_t *)(uintptr_t)(a)) = (uint16_t)(h)
#define MMCR_WR8(a, b)	*((volatile uint8_t *)(uintptr_t)(a)) = (uint8_t)(b)

#define REG32(a)	*((volatile uint32_t *)(uintptr_t)(a))
#define REG16(a)	*((volatile uint16_t *)(uintptr_t)(a))
#define REG8(a)		*((volatile uint8_t *)(uintptr_t)(a))

#define REG32W(a, d)	*((volatile uint32_t *)(uintptr_t)(a)) = (uint32_t)(d)
#define REG16W(a, h)	*((volatile uint16_t *)(uintptr_t)(a)) = (uint16_t)(h)
#define REG8W(a, b)	*((volatile uint8_t *)(uintptr_t)(a)) = (uint8_t)(b)

#define REG32R(a, d)	(d) = *(volatile uint32_t *)(uintptr_t)(a)
#define REG16R(a, h)	(h) = *(volatile uint16_t *)(uintptr_t)(a)
#define REG8R(a, b)	(b) = *(volatile uint8_t *)(uintptr_t)(a)

#define REG32_OFS(a, ofs) \
	*(volatile uint32_t *)((uintptr_t)(a) + (uintptr_t)(ofs))
#define REG16_OFS(a, ofs) \
	*(volatile uint16_t *)((uintptr_t)(a) + (uintptr_t)(ofs))
#define REG8_OFS(a, ofs) \
	*(volatile uint8_t *)((uintptr_t)(a) + (uintptr_t)(ofs))

#define REG32_BIT_SET(a, b)	*(volatile uint32_t *)(a) |= (1ul << (b))
#define REG32_BIT_CLR(a, b)	*(volatile uint32_t *)(a) &= ~(1ul << (b))

#define REG16_BIT_SET(a, b)	*(volatile uint16_t *)(a) |= (1ul << (b))
#define REG16_BIT_CLR(a, b)	*(volatile uint16_t *)(a) &= ~(1ul << (b))

#define REG8_BIT_SET(a, b)	*(volatile uint8_t *)(a) |= (1ul << (b))
#define REG8_BIT_CLR(a, b)	*(volatile uint8_t *)(a) &= ~(1ul << (b))

#endif /* _MEC_REGACCESS_H */
