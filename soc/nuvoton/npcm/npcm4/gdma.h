/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __GDMA_H__
#define __GDMA_H__

#include <soc.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define GDMA_BASE_ADDR (0x40011000)
#define FIU0_BASE_ADDR (0x40020000)

#define FIU0_BURST_CFG M8(FIU0_BASE_ADDR + 0x01)

#define GDMA_CTL0 M32(GDMA_BASE_ADDR + 0x00)
#define GDMA_SRCB0 M32(GDMA_BASE_ADDR + 0x04)
#define GDMA_DSTB0 M32(GDMA_BASE_ADDR + 0x08)
#define GDMA_TCNT0 M32(GDMA_BASE_ADDR + 0x0C)
#define GDMA_CSRC0 M32(GDMA_BASE_ADDR + 0x10)
#define GDMA_CDST0 M32(GDMA_BASE_ADDR + 0x14)
#define GDMA_CTCNT0 M32(GDMA_BASE_ADDR + 0x18)

#define GDMA_CTL1 M32(GDMA_BASE_ADDR + 0x20)
#define GDMA_SRCB1 M32(GDMA_BASE_ADDR + 0x24)
#define GDMA_DSTB1 M32(GDMA_BASE_ADDR + 0x28)
#define GDMA_TCNT1 M32(GDMA_BASE_ADDR + 0x2C)
#define GDMA_CSRC1 M32(GDMA_BASE_ADDR + 0x30)
#define GDMA_CDST1 M32(GDMA_BASE_ADDR + 0x34)
#define GDMA_CTCNT1 M32(GDMA_BASE_ADDR + 0x38)


#ifdef __cplusplus
}
#endif

void gdma_memset_u8(uint8_t *dat, uint8_t set_val, uint32_t setlen);
void gdma_memcpy_u8(uint8_t *dst, uint8_t *src, uint32_t cpylen);
void gdma_memcpy_u32(uint8_t *dst, uint8_t *src, uint32_t cpylen);
void gdma_memcpy_u32_dstfix(uint8_t *dst, uint8_t *src, uint32_t cpylen);
void gdma_memcpy_u32_srcfix(uint8_t *dst, uint8_t *src, uint32_t cpylen);
void gdma_memcpy_burst_u32(uint8_t *dst, uint8_t *src, uint32_t cpylen);

#endif /* _GDMA_H */
