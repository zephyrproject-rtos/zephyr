/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>
#include <soc.h>
#include "soc_ecia.h"

/* EC Subsystem */
#define MCHP_XEC_ECS_REG_BASE      (mem_addr_t) DT_REG_ADDR(DT_NODELABEL(ecs))
#define MCHP_XEC_ECS_ICR_OFS       0x18u
#define MCHP_XEC_ECS_ICR_DM_EN_POS 0 /* direct mode enable */

/* EC Interrupt Aggregator. It is not real interrupt controller. */
#define MCHP_XEC_ECIA_REG_BASE        (mem_addr_t) DT_REG_ADDR(DT_NODELABEL(ecia))
#define MCHP_XEC_ECIA_GIRQ_SIZE       20u /* 5 32-bit registers */
#define MCHP_XEC_ECIA_GIRQ_ALL_MSK    GENMASK(MCHP_MEC_ECIA_GIRQ_LAST, MCHP_MEC_ECIA_GIRQ_FIRST)
#define MCHP_XEC_ECIA_GIRQ_DIRECT_MSK (GENMASK(21, 13) | BIT(23))
#define MCHP_XEC_ECIA_GIRQ_AGGR_MSK   (GENMASK(12, 8) | BIT(22) | GENMASK(26, 24))

#define MCHP_XEC_ECIA_ZGIRQ_OFS(zgirq) ((uint32_t)(zgirq) * MCHP_XEC_ECIA_GIRQ_SIZE)
#define MCHP_XEC_ECIA_GIRQ_OFS(girq)                                                               \
	MCHP_XEC_ECIA_ZGIRQ_OFS((uint32_t)(girq) - MCHP_MEC_ECIA_GIRQ_FIRST)

#define MCHP_XEC_ECIA_GIRQ_SRC_OFS    0
#define MCHP_XEC_ECIA_GIRQ_ENSET_OFS  4u
#define MCHP_XEC_ECIA_GIRQ_RESULT_OFS 8u
#define MCHP_XEC_ECIA_GIRQ_ENCLR_OFS  12u
/* offset 16 (0x10) is reserved read-only 0 */

/* aggregated enable set/clear registers */
#define MCHP_XEC_ECIA_AGGR_ENSET_OFS 0x200u /* r/w1s */
#define MCHP_XEC_ECIA_AGGR_ENCLR_OFS 0x204u /* r/w1c */
#define MCHP_XEC_ECIA_AGGR_ACTV_OFS  0x208u /* read-only */

#define MCHP_XEC_ECIA_GIRQ_REG_OFS(girq, regofs)                                                   \
	(MCHP_XEC_ECIA_ZGIRQ_OFS((uint32_t)(girq) - MCHP_MEC_ECIA_GIRQ_FIRST) + (uint32_t)regofs)

#define MCHP_XEC_ECIA_GIRQ_SRC_REG_OFS(girq)                                                       \
	MCHP_XEC_ECIA_GIRQ_REG_OFS(girq, MCHP_XEC_ECIA_GIRQ_SRC_OFS)

#define MCHP_XEC_ECIA_GIRQ_ENSET_REG_OFS(girq)                                                     \
	MCHP_XEC_ECIA_GIRQ_REG_OFS(girq, MCHP_XEC_ECIA_GIRQ_ENSET_OFS)

#define MCHP_XEC_ECIA_GIRQ_RESULT_REG_OFS(girq)                                                    \
	MCHP_XEC_ECIA_GIRQ_REG_OFS(girq, MCHP_XEC_ECIA_GIRQ_RESULT_OFS)

#define MCHP_XEC_ECIA_GIRQ_ENCLR_REG_OFS(girq)                                                     \
	MCHP_XEC_ECIA_GIRQ_REG_OFS(girq, MCHP_XEC_ECIA_GIRQ_ENCLR_OFS)

int soc_ecia_init(uint32_t aggr_girq_bm, uint32_t direct_girq_bm, uint32_t flags)
{
	mem_addr_t ecia_base = MCHP_XEC_ECIA_REG_BASE;
	mem_addr_t ecs_base = MCHP_XEC_ECS_REG_BASE;
	uint32_t amsk = 0, dmsk = 0, bm = 0, girq = 0, raddr = 0;

	amsk = aggr_girq_bm & MCHP_XEC_ECIA_GIRQ_AGGR_MSK;
	dmsk = direct_girq_bm & MCHP_XEC_ECIA_GIRQ_DIRECT_MSK;

	bm = aggr_girq_bm | direct_girq_bm;
	while (bm != 0) {
		girq = find_lsb_set(bm) - 1u;

		raddr = ecia_base + MCHP_XEC_ECIA_GIRQ_OFS(girq);

		if ((flags & MCHP_MEC_ECIA_INIT_CLR_ENABLES) != 0) { /* clear enables? */
			sys_write32(UINT32_MAX, raddr + MCHP_XEC_ECIA_GIRQ_ENCLR_OFS);
		}

		if ((flags & MCHP_MEC_ECIA_INIT_CLR_STATUS) != 0) { /* clear status */
			sys_write32(UINT32_MAX, raddr + MCHP_XEC_ECIA_GIRQ_SRC_OFS);
		}

		bm &= (uint32_t)~BIT(girq);
	}

	sys_write32(UINT32_MAX, ecia_base + MCHP_XEC_ECIA_AGGR_ENCLR_OFS);
	sys_write32(amsk, ecia_base + MCHP_XEC_ECIA_AGGR_ENSET_OFS);

	if (dmsk != 0) {
		sys_set_bit(ecs_base + MCHP_XEC_ECS_ICR_OFS, MCHP_XEC_ECS_ICR_DM_EN_POS);
	} else {
		sys_clear_bit(ecs_base + MCHP_XEC_ECS_ICR_OFS, MCHP_XEC_ECS_ICR_DM_EN_POS);
	}

	return 0;
}

int soc_ecia_girq_ctrl_bm(uint8_t girq, uint32_t bitmap, uint8_t enable)
{
	mem_addr_t raddr = MCHP_XEC_ECIA_REG_BASE;

	if ((girq < MCHP_MEC_ECIA_GIRQ_FIRST) || (girq > MCHP_MEC_ECIA_GIRQ_LAST)) {
		return -EINVAL;
	}

	raddr += MCHP_XEC_ECIA_GIRQ_OFS(girq);

	if (enable != 0) {
		raddr += MCHP_XEC_ECIA_GIRQ_ENSET_OFS;
	} else {
		raddr += MCHP_XEC_ECIA_GIRQ_ENCLR_OFS;
	}

	sys_write32(bitmap, raddr);

	return 0;
}

int soc_ecia_girq_ctrl(uint8_t girq, uint8_t srcpos, uint8_t enable)
{
	uint32_t bitmap = BIT(srcpos);

	return soc_ecia_girq_ctrl_bm(girq, bitmap, enable);
}

uint32_t soc_ecia_girq_get_enable_bm(uint8_t girq)
{
	mem_addr_t raddr = MCHP_XEC_ECIA_REG_BASE;

	if ((girq < MCHP_MEC_ECIA_GIRQ_FIRST) || (girq > MCHP_MEC_ECIA_GIRQ_LAST)) {
		return 0;
	}

	raddr += MCHP_XEC_ECIA_GIRQ_ENSET_REG_OFS(girq);

	return sys_read32(raddr);
}

int soc_ecia_girq_status_clear_bm(uint8_t girq, uint32_t bitmap)
{
	mem_addr_t raddr = MCHP_XEC_ECIA_REG_BASE;

	if ((girq < MCHP_MEC_ECIA_GIRQ_FIRST) || (girq > MCHP_MEC_ECIA_GIRQ_LAST)) {
		return -EINVAL;
	}

	raddr += MCHP_XEC_ECIA_GIRQ_SRC_REG_OFS(girq);

	sys_write32(bitmap, raddr);

	return 0;
}

int soc_ecia_girq_status_clear(uint8_t girq, uint8_t srcpos)
{
	uint32_t bitmap = BIT(srcpos);

	return soc_ecia_girq_status_clear_bm(girq, bitmap);
}

int soc_ecia_girq_status(uint8_t girq, uint32_t *status)
{
	mem_addr_t raddr = MCHP_XEC_ECIA_REG_BASE;

	if ((girq < MCHP_MEC_ECIA_GIRQ_FIRST) || (girq > MCHP_MEC_ECIA_GIRQ_LAST)) {
		return -EINVAL;
	}

	raddr += MCHP_XEC_ECIA_GIRQ_SRC_REG_OFS(girq);

	if (status != NULL) {
		*status = sys_read32(raddr);
	}

	return 0;
}

int soc_ecia_girq_result(uint8_t girq, uint32_t *result)
{
	mem_addr_t raddr = MCHP_XEC_ECIA_REG_BASE;

	if ((girq < MCHP_MEC_ECIA_GIRQ_FIRST) || (girq > MCHP_MEC_ECIA_GIRQ_LAST)) {
		return -EINVAL;
	}

	raddr += MCHP_XEC_ECIA_GIRQ_RESULT_REG_OFS(girq);

	if (result != NULL) {
		*result = sys_read32(raddr);
	}

	return 0;
}

int soc_ecia_girq_is_result(uint8_t girq, uint32_t bitmap)
{
	uint32_t result = 0;

	if (soc_ecia_girq_result(girq, &result) != 0) {
		return 0;
	}

	return (result & bitmap);
}

int soc_ecia_girq_aggr_ctrl_bm(uint32_t girq_bitmap, uint8_t enable)
{
	mem_addr_t raddr = MCHP_XEC_ECIA_REG_BASE;

	if (enable != 0) {
		raddr += MCHP_XEC_ECIA_AGGR_ENSET_OFS;
	} else {
		raddr += MCHP_XEC_ECIA_AGGR_ENCLR_OFS;
	}

	sys_write32(girq_bitmap, raddr);

	return 0;
}

int soc_ecia_girq_aggr_ctrl(uint8_t girq, uint8_t enable)
{
	uint32_t girq_bitmap = BIT(girq);

	return soc_ecia_girq_aggr_ctrl_bm(girq_bitmap, enable);
}
