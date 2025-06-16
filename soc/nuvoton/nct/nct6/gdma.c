/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "gdma.h"

#define RAMFUNC __attribute__((section(".ramfunc"))) __attribute__((noinline))

/*--------------------------------------------------------------------------------------------*/
/*                                                                                            */
/*--------------------------------------------------------------------------------------------*/
RAMFUNC void gdma_memset_u8(uint8_t *dat, uint8_t set_val, uint32_t setlen)
{
	uint8_t val = set_val;

	if (setlen == 0) {
		return;
	}

	unsigned int key = irq_lock();

	GDMA_SRCB0 = (uint32_t)&val;
	GDMA_DSTB0 = (uint32_t)dat;
	GDMA_TCNT0 = setlen;
	GDMA_CTL0 = (GDMA_SOFTREQ_Msk | GDMA_SAFIX_Msk | GDMA_EN_Msk);

	while (GDMA_CTL0 & GDMA_EN_Msk);
	GDMA_CTL0 = 0;

	irq_unlock(key);
}

/*--------------------------------------------------------------------------------------------*/
/*                                                                                            */
/*--------------------------------------------------------------------------------------------*/
RAMFUNC void gdma_memcpy_u8(uint8_t *dst, uint8_t *src, uint32_t cpylen)
{
	if (cpylen == 0) {
		return;
	}

	unsigned int key = irq_lock();

	GDMA_SRCB0 = (uint32_t)src;
	GDMA_DSTB0 = (uint32_t)dst;
	GDMA_TCNT0 = cpylen;
	GDMA_CTL0 = (GDMA_SOFTREQ_Msk | GDMA_EN_Msk);

	while (GDMA_CTL0 & GDMA_EN_Msk);

	GDMA_CTL0 = 0;

	irq_unlock(key);
}

/*--------------------------------------------------------------------------------------------*/
/*                                                                                            */
/*--------------------------------------------------------------------------------------------*/
RAMFUNC void gdma_memcpy_u32(uint8_t *dst, uint8_t *src, uint32_t cpylen)
{
	uint8_t rlen = cpylen & 0x3;

	if (cpylen == 0) {
		return;
	}

	unsigned int key = irq_lock();

	GDMA_SRCB0 = (uint32_t)src;
	GDMA_DSTB0 = (uint32_t)dst;
	GDMA_TCNT0 = cpylen / 4;
	GDMA_CTL0 = (GDMA_SOFTREQ_Msk | (0x2<<GDMA_TWS_Pos) | GDMA_EN_Msk);

	while (GDMA_CTL0 & GDMA_EN_Msk);
	GDMA_CTL0 = 0;

	/* remain length */
    if(cpylen) {
        gdma_memcpy_u8((src - (cpylen & 0x3)), (dst- (cpylen & 0x3)), rlen);
    }

	irq_unlock(key);
}

/*-------------------------------------------------------------------------------------------*/
/*                                                                                            */
/*--------------------------------------------------------------------------------------------*/
RAMFUNC void gdma_memcpy_burst_u32(uint8_t *dst, uint8_t *src, uint32_t cpylen)
{
	uint32_t rlen = 0;

	if (cpylen == 0) {
		return;
	}

	unsigned int key = irq_lock();

	/* src and dst address must 16byte aligned */
	if (((uint32_t)src & 0x0F) || ((uint32_t)dst & 0xF)) {
		gdma_memcpy_u8(dst, src, cpylen);
		irq_unlock(key);
		return;
	}

	/* Enable FIU unlimited burst */
	if(((uint32_t)src & 0xF0000000) != 0)
	{
		rlen = cpylen & 0xFFFFFFF0;
		// Only for FIU, 0x60000000/0x70000000/0x80000000
		FIU0_BURST_CFG |= BIT(NCT_BURST_CFG_UNLIM_BURST);
	}

	if (rlen) {
		GDMA_SRCB0 = (uint32_t)src;
		GDMA_DSTB0 = (uint32_t)dst;
		GDMA_TCNT0 = rlen / 16;
		GDMA_CTL0 = (GDMA_SOFTREQ_Msk | (0x2<<GDMA_TWS_Pos) | GDMA_BME_Msk | GDMA_EN_Msk);;

		while (GDMA_CTL0 & GDMA_EN_Msk);
		GDMA_CTL0 = 0;

		src += rlen;
		dst += rlen;
		cpylen -= rlen;
	}

	/* Disable unlimited mode */
	if(((uint32_t)src & 0xF0000000) != 0)
	{
		// Only for FIU, 0x60000000/0x70000000/0x80000000
		FIU0_BURST_CFG &= ~BIT(NCT_BURST_CFG_UNLIM_BURST);
	}

	/* remain length */
	if (cpylen) {
		gdma_memcpy_u8(dst, src, cpylen);
	}

	irq_unlock(key);
}

/*--------------------------------------------------------------------------------------------*/
/*                                                                                            */
/*--------------------------------------------------------------------------------------------*/
RAMFUNC void gdma_memcpy_u32_dstfix(uint8_t *dst, uint8_t *src, uint32_t cpylen)
{
	if (cpylen == 0) {
		return;
	}

	unsigned int key = irq_lock();

	GDMA_SRCB0 = (uint32_t)src;
	GDMA_DSTB0 = (uint32_t)dst;
	GDMA_TCNT0 = cpylen / 4;
	GDMA_CTL0 = (GDMA_SOFTREQ_Msk | (0x2<<GDMA_TWS_Pos) | GDMA_DAFIX_Msk | GDMA_EN_Msk);

	while (GDMA_CTL0 & GDMA_EN_Msk);
	GDMA_CTL0 = 0;

	irq_unlock(key);
}

/*--------------------------------------------------------------------------------------------*/
/*                                                                                            */
/*--------------------------------------------------------------------------------------------*/
RAMFUNC void gdma_memcpy_u32_srcfix(uint8_t *dst, uint8_t *src, uint32_t cpylen)
{
	if (cpylen == 0) {
		return;
	}

	unsigned int key = irq_lock();

	GDMA_SRCB0 = (uint32_t)src;
	GDMA_DSTB0 = (uint32_t)dst;
	GDMA_TCNT0 = cpylen / 4;
	GDMA_CTL0 = (GDMA_SOFTREQ_Msk | (0x2<<GDMA_TWS_Pos) | GDMA_SAFIX_Msk | GDMA_EN_Msk);;

	while (GDMA_CTL0 & GDMA_EN_Msk);
	GDMA_CTL0 = 0;

	 irq_unlock(key);
}
