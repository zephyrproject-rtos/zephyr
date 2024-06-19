/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gdma.h"

/*-------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------*/
void gdma_memset_u8(uint8_t *dat, uint8_t set_val, uint32_t setlen)
{
	uint8_t val = set_val;

	if (setlen == 0) {
		return;
	}

	GDMA_SRCB0 = (uint32_t)&val;
	GDMA_DSTB0 = (uint32_t)dat;
	GDMA_TCNT0 = setlen;
	GDMA_CTL0 = 0x10081;

	while (GDMA_CTL0 & 0x1) {
	}
	GDMA_CTL0 = 0;
}

/*-------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------*/
void gdma_memcpy_u8(uint8_t *dst, uint8_t *src, uint32_t cpylen)
{
	if (cpylen == 0) {
		return;
	}

	GDMA_SRCB0 = (uint32_t)src;
	GDMA_DSTB0 = (uint32_t)dst;
	GDMA_TCNT0 = cpylen;
	GDMA_CTL0 = 0x10001;

	while (GDMA_CTL0 & 0x1) {
	}
	GDMA_CTL0 = 0;
}

/*-------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------*/
void gdma_memcpy_u32(uint8_t *dst, uint8_t *src, uint32_t cpylen)
{
	if (cpylen == 0) {
		return;
	}

	GDMA_SRCB0 = (uint32_t)src;
	GDMA_DSTB0 = (uint32_t)dst;
	GDMA_TCNT0 = cpylen / 4;
	GDMA_CTL0 = 0x12001;

	while (GDMA_CTL0 & 0x1) {
	}
	GDMA_CTL0 = 0;
}

/*-------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------*/
void gdma_memcpy_burst_u32(uint8_t *dst, uint8_t *src, uint32_t cpylen)
{
	uint32_t rlen;

	if (cpylen == 0) {
		return;
	}

	/* src and dst address must 16byte aligned */
	if (((uint32_t)src & 0x0F) || ((uint32_t)dst & 0xF)) {
		gdma_memcpy_u8(dst, src, cpylen);
		return;
	}

	/* aligned 64Byte length */
	rlen = cpylen & 0xFFFFFFC0;
	if (rlen) {
		FIU0_BURST_CFG = 0x0B;

		GDMA_SRCB0 = (uint32_t)src;
		GDMA_DSTB0 = (uint32_t)dst;
		GDMA_TCNT0 = rlen / 16;
		GDMA_CTL0 = 0x12201;

		while (GDMA_CTL0 & 0x1) {
		}
		GDMA_CTL0 = 0;

		FIU0_BURST_CFG = 0x03;

		src += rlen;
		dst += rlen;
		cpylen -= rlen;
	}

	/* remain length */
	if (cpylen) {
		gdma_memcpy_u8(dst, src, cpylen);
	}
}

/*-------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------*/
void gdma_memcpy_u32_dstfix(uint8_t *dst, uint8_t *src, uint32_t cpylen)
{
	if (cpylen == 0) {
		return;
	}

	GDMA_SRCB0 = (uint32_t)src;
	GDMA_DSTB0 = (uint32_t)dst;
	GDMA_TCNT0 = cpylen / 4;
	GDMA_CTL0 = 0x12041;

	while (GDMA_CTL0 & 0x1) {
	}
	GDMA_CTL0 = 0;
}


/*-------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------*/
void gdma_memcpy_u32_srcfix(uint8_t *dst, uint8_t *src, uint32_t cpylen)
{
	if (cpylen == 0) {
		return;
	}

	GDMA_SRCB0 = (uint32_t)src;
	GDMA_DSTB0 = (uint32_t)dst;
	GDMA_TCNT0 = cpylen / 4;
	GDMA_CTL0 = 0x12081;

	while (GDMA_CTL0 & 0x1) {
	}
	GDMA_CTL0 = 0;
}
