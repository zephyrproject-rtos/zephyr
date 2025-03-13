/*
 * Copyright (c) 2024 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CACHE_CACHE_ANDES_L2_H_
#define ZEPHYR_DRIVERS_CACHE_CACHE_ANDES_L2_H_

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/syscon.h>

#define L2C_BASE		DT_REG_ADDR_BY_IDX(DT_INST(0, andestech_l2c), 0)

/* L2 cache Register Offset */
#define L2C_CONFIG		(L2C_BASE + 0x00)
#define L2C_CTRL		(L2C_BASE + 0x08)
#define L2C_CCTLCMD(hart_id)	\
	(L2C_BASE + 0x40 + (hart_id * l2_cache_cfg.cmd_offset))
#define L2C_CCTLACC(hart_id)	\
	(L2C_BASE + 0x48 + (hart_id * l2_cache_cfg.cmd_offset))
#define L2C_CCTLST(hart_id)	\
	(L2C_BASE + 0x80 + (hart_id * l2_cache_cfg.status_offset))

/* L2 cache config registers bitfields */
#define L2C_CONFIG_SIZE_SHIFT		7
#define L2C_CONFIG_MAP			BIT(20)
#define L2C_CONFIG_VERSION_SHIFT	24

/* L2 cache control registers bitfields */
#define L2C_CTRL_CEN		BIT(0)
#define L2C_CTRL_IPFDPT_3	GENMASK(4, 3)
#define L2C_CTRL_DPFDPT_8	GENMASK(7, 6)

/* L2 cache CCTL Access Line registers bitfields */
#define L2C_CCTLACC_WAY_SHIFT	28

/* L2 CCTL Command */
#define CCTL_L2_IX_INVAL	0x00
#define CCTL_L2_IX_WB		0x01
#define CCTL_L2_PA_INVAL	0x08
#define CCTL_L2_PA_WB		0x09
#define CCTL_L2_PA_WBINVAL	0x0a
#define CCTL_L2_WBINVAL_ALL	0x12

#define K_CACHE_WB		BIT(0)
#define K_CACHE_INVD		BIT(1)
#define K_CACHE_WB_INVD		(K_CACHE_WB | K_CACHE_INVD)

struct nds_l2_cache_config {
	uint32_t size;
	uint32_t cmd_offset;
	uint32_t status_offset;
	uint16_t status_shift;
	uint8_t version;
	uint8_t line_size;
};

static struct nds_l2_cache_config l2_cache_cfg;

static ALWAYS_INLINE int nds_l2_cache_is_inclusive(void)
{
	return IS_ENABLED(CONFIG_L2C_INCLUSIVE_POLICY) &&
			(l2_cache_cfg.version > 15);

}

static ALWAYS_INLINE void nds_l2_cache_wait_status(uint8_t hart_id)
{
	uint32_t status;

	do {
		status = sys_read32(L2C_CCTLST(hart_id));
		status >>= hart_id * l2_cache_cfg.status_shift;
		status &= BIT_MASK(4);
	} while (status == 1);
}

static ALWAYS_INLINE int nds_l2_cache_all(int op)
{
	unsigned long ways, sets, index, cmd;
	uint8_t hart_id;
	unsigned long status = csr_read(mstatus);

	if (!l2_cache_cfg.size) {
		return -ENOTSUP;
	} else if (l2_cache_cfg.size >= 128 * 1024) {
		ways = 16;
	} else {
		ways = 8;
	}

	if (csr_read(NDS_MMSC_CFG) & MMSC_CFG_VCCTL_2) {
		if ((status & MSTATUS_MPRV) && !(status & MSTATUS_MPP)) {
			if (!nds_l2_cache_is_inclusive()) {
				return -ENOTSUP;
			}
		}
	}

	switch (op) {
	case K_CACHE_WB:
		cmd = CCTL_L2_IX_WB;
		break;
	case K_CACHE_INVD:
		cmd = CCTL_L2_IX_INVAL;
		break;
	case K_CACHE_WB_INVD:
		cmd = CCTL_L2_WBINVAL_ALL;
		break;
	default:
		return -ENOTSUP;
	}

	hart_id = arch_proc_id();

	if (op == K_CACHE_WB_INVD) {
		sys_write32(CCTL_L2_WBINVAL_ALL, L2C_CCTLCMD(hart_id));

		/* Wait L2 CCTL Commands finished */
		nds_l2_cache_wait_status(hart_id);
	} else {
		sets = l2_cache_cfg.size / (ways * l2_cache_cfg.line_size);
		/* Invalidate all cache line by each way and each set */
		for (int j = 0; j < ways; j++) {
			/* Index of way */
			index = j << L2C_CCTLACC_WAY_SHIFT;
			for (int i = 0; i < sets; i++) {
				/* Index of set */
				index += l2_cache_cfg.line_size;

				/* Invalidate each cache line */
				sys_write32(index, L2C_CCTLACC(hart_id));
				sys_write32(cmd, L2C_CCTLCMD(hart_id));

				/* Wait L2 CCTL Commands finished */
				nds_l2_cache_wait_status(hart_id);
			}
		}
	}

	return 0;
}

static ALWAYS_INLINE int nds_l2_cache_range(void *addr, size_t size, int op)
{
	unsigned long last_byte, align_addr, cmd;
	uint8_t hart_id;

	if (!l2_cache_cfg.size) {
		return -ENOTSUP;
	}

	switch (op) {
	case K_CACHE_WB:
		cmd = CCTL_L2_PA_WB;
		break;
	case K_CACHE_INVD:
		cmd = CCTL_L2_PA_INVAL;
		break;
	case K_CACHE_WB_INVD:
		cmd = CCTL_L2_PA_WBINVAL;
		break;
	default:
		return -ENOTSUP;
	}

	last_byte = (unsigned long)addr + size - 1;
	align_addr = ROUND_DOWN(addr, l2_cache_cfg.line_size);
	hart_id = arch_proc_id();

	while (align_addr <= last_byte) {
		sys_write32(align_addr, L2C_CCTLACC(hart_id));
		sys_write32(cmd, L2C_CCTLCMD(hart_id));
		align_addr += l2_cache_cfg.line_size;

		/* Wait L2 CCTL Commands finished */
		nds_l2_cache_wait_status(hart_id);
	}

	return 0;
}

static ALWAYS_INLINE void nds_l2_cache_enable(void)
{
	if (l2_cache_cfg.size) {
		uint32_t l2c_ctrl = sys_read32(L2C_CTRL);

		if (!(l2c_ctrl & L2C_CTRL_CEN)) {
			WRITE_BIT(l2c_ctrl, 0, true);
			sys_write32(l2c_ctrl, L2C_CTRL);
		}
	}
}

static ALWAYS_INLINE void nds_l2_cache_disable(void)
{
	if (l2_cache_cfg.size) {
		uint32_t l2c_ctrl = sys_read32(L2C_CTRL);

		if (l2c_ctrl & L2C_CTRL_CEN) {
			WRITE_BIT(l2c_ctrl, 0, false);
			sys_write32(l2c_ctrl, L2C_CTRL);
		}
	}
}

static ALWAYS_INLINE int nds_l2_cache_init(uint8_t line_size)
{
	unsigned long size;
	uint32_t l2c_ctrl;

#if defined(CONFIG_SYSCON)
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(syscon), andestech_atcsmu100, okay)
	uint32_t system_cfg;
	const struct device *syscon_dev = DEVICE_DT_GET(DT_NODELABEL(syscon));

	if (device_is_ready(syscon_dev)) {
		/* Check L2 cache feature from SMU */
		syscon_read_reg(syscon_dev, 0x08, &system_cfg);

		/* Platform doesn't support L2 cache controller */
		if (!(system_cfg & BIT(8))) {
			l2_cache_cfg.size = 0;
			return 0;
		}
	} else {
		LOG_ERR("Andes cache driver should be initialized after "
			"syscon driver initialization");
		return 0;
	}
#endif /* andestech_atcsmu100 dts node status okay */
#endif /* defined(CONFIG_SYSCON) */

	l2_cache_cfg.line_size = line_size;

	size = (sys_read32(L2C_CONFIG) >> L2C_CONFIG_SIZE_SHIFT) & BIT_MASK(7);
	l2_cache_cfg.size = size * 128 * 1024;

	if (sys_read32(L2C_CONFIG) & L2C_CONFIG_MAP) {
		l2_cache_cfg.cmd_offset = 0x10;
		l2_cache_cfg.status_offset = 0;
		l2_cache_cfg.status_shift = 4;
	} else {
		l2_cache_cfg.cmd_offset = 0x1000;
		l2_cache_cfg.status_offset = 0x1000;
		l2_cache_cfg.status_shift = 0;
	}

	l2_cache_cfg.version = (sys_read32(L2C_CONFIG) >> L2C_CONFIG_VERSION_SHIFT) & BIT_MASK(8);

	/* Initializing L2 cache instruction, data prefetch depth */
	l2c_ctrl = sys_read32(L2C_CTRL);
	l2c_ctrl |= (L2C_CTRL_IPFDPT_3 | L2C_CTRL_DPFDPT_8);

	/* Writeback and invalidate all I/D-Cache before setting L2C */
	__asm__ volatile ("fence.i");
	sys_write32(l2c_ctrl, L2C_CTRL);

	if (IS_ENABLED(CONFIG_SMP)) {
		if (l2_cache_cfg.size) {
			l2c_ctrl = sys_read32(L2C_CTRL);

			if (!(l2c_ctrl & L2C_CTRL_CEN)) {
				WRITE_BIT(l2c_ctrl, 0, true);
				sys_write32(l2c_ctrl, L2C_CTRL);
			}
		}
	}

	return l2_cache_cfg.size;
}

#endif /* ZEPHYR_DRIVERS_CACHE_CACHE_ANDES_L2_H_ */
