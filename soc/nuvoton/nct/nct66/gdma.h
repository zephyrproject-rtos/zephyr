/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
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


/*------------------------------*/
/* GDMA_CTL0 / GDMA_CTL1 fields */
/*------------------------------*/
#define GDMA_ERR_Pos                                (20)
#define GDMA_ERR_Msk                                (0x1 << GDMA_ERR_Pos)

#define GDMA_TC_Pos                                 (18)
#define GDMA_TC_Msk                                 (0x1 << GDMA_TC_Pos)

#define GDMA_SOFTREQ_Pos                            (16)
#define GDMA_SOFTREQ_Msk                            (0x1 << GDMA_SOFTREQ_Pos)

#define GDMA_GPS_Pos                                (14)
#define GDMA_GPS_Msk                                (0x1 << GDMA_GPS_Pos)

#define GDMA_TWS_Pos                                (12)
#define GDMA_TWS_Msk                                (0x3 << GDMA_TWS_Pos)

#define GDMA_BME_Pos                                (9)
#define GDMA_BME_Msk                                (0x1 << GDMA_BME_Pos)

#define GDMA_SIEN_Pos                               (8)
#define GDMA_SIEN_Msk                               (0x1 << GDMA_SIEN_Pos)

#define GDMA_SAFIX_Pos                              (7)
#define GDMA_SAFIX_Msk                              (0x1 << GDMA_SAFIX_Pos)

#define GDMA_DAFIX_Pos                              (6)
#define GDMA_DAFIX_Msk                              (0x1 << GDMA_DAFIX_Pos)

#define GDMA_SADIR_Pos                              (5)
#define GDMA_SADIR_Msk                              (0x1 << GDMA_SADIR_Pos)

#define GDMA_DADIR_Pos                              (4)
#define GDMA_DADIR_Msk                              (0x1 << GDMA_DADIR_Pos)

#define GDMA_MS_Pos                                 (2)
#define GDMA_MS_Msk                                 (0x3 << GDMA_MS_Pos)

#define GDMA_GPD_Pos                                (1)
#define GDMA_GPD_Msk                                (0x1 << GDMA_GPD_Pos)

#define GDMA_EN_Pos                                 (0)
#define GDMA_EN_Msk                                 (0x1 << GDMA_EN_Pos)

/*-------------------------------*/
/* GDMA_TCNT / GDMA_CTCNT fields */
/*-------------------------------*/
#define GDMA_TFR_CNT_Pos                            (0)
#define GDMA_TFR_CNT_Msk                            (0x0000FFFFFF << GDMA_TFR_CNT_Pos)


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
