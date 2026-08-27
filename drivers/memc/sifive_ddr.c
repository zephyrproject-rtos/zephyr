/*
 * (C) Copyright 2020-2021 SiFive, Inc.
 * (C) Copyright 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Based on implementation of fsbl in:
 * https://github.com/sifive/freedom-u540-c000-bootloader
 */

#define DT_DRV_COMPAT sifive_fu740_c000_ddr

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <soc.h>
#include "sifive_ddrregs.h"

LOG_MODULE_REGISTER(sifive_ddr);

#define DRAM_CLASS_OFFSET		8
#define DRAM_CLASS_DDR4			0xA
#define OPTIMAL_RMODW_EN		BIT(0)
#define DISABLE_RD_INTERLEAVE		BIT(16)
#define OUT_OF_RANGE			BIT(1)
#define MULTIPLE_OUT_OF_RANGE		BIT(2)
#define PORT_COMMAND_CHANNEL_ERROR	BIT(7)
#define MC_INIT_COMPLETE		BIT(8)
#define LEVELING_OPERATION_COMPLETED	BIT(22)
#define DFI_PHY_WRLELV_MODE		BIT(24)
#define DFI_PHY_RDLVL_MODE		BIT(24)
#define DFI_PHY_RDLVL_GATE_MODE		BIT(0)
#define VREF_EN				BIT(24)
#define PORT_ADDR_PROTECTION_EN		BIT(0)
#define AXI0_ADDRESS_RANGE_ENABLE	BIT(8)
#define AXI0_RANGE_PROT_BITS_0		(BIT(24) | BIT(25));
#define RDLVL_EN			BIT(16)
#define RDLVL_GATE_EN			BIT(24)
#define WRLVL_EN			BIT(0)

#define PHY_RX_CAL_DQ0_0_OFFSET		0
#define PHY_RX_CAL_DQ1_0_OFFSET		16

#define DDR_CTL_REG(d, i) (*(d->ddrctl + i))
#define DDR_PHY_REG(d, i) (*(d->ddrphy + i))

struct ddr_ctrl_data {
	volatile uint32_t *const ddrctl;
	volatile uint32_t *const ddrphy;
	volatile uint32_t *const ddr_physical_filter;

	volatile uint32_t *const ddr_start;
	const size_t ddr_size;
};

static inline void phy_reset(struct ddr_ctrl_data *ddr_ctrl)
{
	unsigned int i;

	for (i = 1152; i <= 1214; i++) {
		DDR_PHY_REG(ddr_ctrl, i) = ddr_phy_settings[i];
	}
	for (i = 0; i <= 1151; i++) {
		DDR_PHY_REG(ddr_ctrl, i) = ddr_phy_settings[i];
	}
}

static inline void ddr_writeregmap(struct ddr_ctrl_data *ddr_ctrl)
{
	unsigned int i;

	for (i = 0; i <= 264; i++) {
		DDR_CTL_REG(ddr_ctrl, i) = ddr_ctl_settings[i];
	}
	phy_reset(ddr_ctrl);
}

static inline uint32_t ddr_getdramclass(struct ddr_ctrl_data *ddr_ctrl)
{
	return ((DDR_CTL_REG(ddr_ctrl, 0) >> DRAM_CLASS_OFFSET) & 0xF);
}

static inline void check_errata(uint32_t regbase, uint32_t updownreg)
{
	uint64_t fails = 0;
	uint32_t bit, dq;

	for (bit = 0, dq = 0; bit < 2; bit++, dq++) {
		uint32_t phy_rx_cal_dqn_0_offset;

		if (bit == 0) {
			phy_rx_cal_dqn_0_offset = PHY_RX_CAL_DQ0_0_OFFSET;
		} else {
			phy_rx_cal_dqn_0_offset = PHY_RX_CAL_DQ1_0_OFFSET;
		}

		uint32_t down = (updownreg >> phy_rx_cal_dqn_0_offset) & 0x3F;
		uint32_t up = (updownreg >> (phy_rx_cal_dqn_0_offset + 6)) & 0x3F;

		uint8_t failc0 = ((down == 0) && (up == 0x3F));
		uint8_t failc1 = ((up == 0) && (down == 0x3F));

		/* print error message on failure */
		if (failc0 || failc1) {
			if (fails == 0) {
				LOG_ERR("DDR error in fixing up");
			}
			char slicelsc = '0';
			char slicemsc = '0';

			fails |= (1 << dq);
			slicelsc += (dq % 10);
			slicemsc += (dq / 10);

			LOG_ERR("S %c%c%c", slicemsc, slicelsc, failc0 ? 'U' : 'D');
		}
	}
}

static inline uint64_t ddr_phy_fixup(struct ddr_ctrl_data *ddr_ctrl)
{
	/* return bitmask of failed lanes */
	uint32_t slicebase = 0;
	uint32_t updownreg;

	/* check errata condition */
	for (uint32_t slice = 0; slice < 8; slice++) {
		uint32_t regbase = slicebase + 34;

		for (uint32_t reg = 0 ; reg < 4; reg++) {
			updownreg = DDR_PHY_REG(ddr_ctrl, (regbase + reg));
			check_errata(regbase, updownreg);
		}
		slicebase += 128;
	}
	return (0);
}

static int ddr_init(const struct device *dev)
{
	struct ddr_ctrl_data *ddr_ctrl = dev->data;

	LOG_DBG("start: 0x%lx", (uintptr_t)ddr_ctrl->ddr_start);
	LOG_DBG("size:  0x%lx", ddr_ctrl->ddr_size);

	ddr_writeregmap(ddr_ctrl);

	DDR_CTL_REG(ddr_ctrl, 120) |= DISABLE_RD_INTERLEAVE;
	DDR_CTL_REG(ddr_ctrl, 21)  &= ~OPTIMAL_RMODW_EN;
	DDR_CTL_REG(ddr_ctrl, 170) |= WRLVL_EN | DFI_PHY_WRLELV_MODE;
	DDR_CTL_REG(ddr_ctrl, 181) |= DFI_PHY_RDLVL_MODE;
	DDR_CTL_REG(ddr_ctrl, 260) |= RDLVL_EN;
	DDR_CTL_REG(ddr_ctrl, 260) |= RDLVL_GATE_EN;
	DDR_CTL_REG(ddr_ctrl, 182) |= DFI_PHY_RDLVL_GATE_MODE;

	if (ddr_getdramclass(ddr_ctrl) == DRAM_CLASS_DDR4) {
		DDR_CTL_REG(ddr_ctrl, 184) |= VREF_EN;
	}

	DDR_CTL_REG(ddr_ctrl, 136) |= LEVELING_OPERATION_COMPLETED;
	DDR_CTL_REG(ddr_ctrl, 136) |= MC_INIT_COMPLETE;
	DDR_CTL_REG(ddr_ctrl, 136) |= OUT_OF_RANGE | MULTIPLE_OUT_OF_RANGE;

	/* Setup range protection */
	size_t end_addr_16Kblocks = ((ddr_ctrl->ddr_size >> 14) & 0x7FFFFF) - 1;

	DDR_CTL_REG(ddr_ctrl, 209) = 0x0;
	DDR_CTL_REG(ddr_ctrl, 210) = ((uint32_t) end_addr_16Kblocks);
	DDR_CTL_REG(ddr_ctrl, 212) = 0x0;
	DDR_CTL_REG(ddr_ctrl, 214) = 0x0;
	DDR_CTL_REG(ddr_ctrl, 216) = 0x0;
	DDR_CTL_REG(ddr_ctrl, 224) |= AXI0_RANGE_PROT_BITS_0;
	DDR_CTL_REG(ddr_ctrl, 225) = 0xFFFFFFFF;
	DDR_CTL_REG(ddr_ctrl, 208) |= AXI0_ADDRESS_RANGE_ENABLE;
	DDR_CTL_REG(ddr_ctrl, 208) |= PORT_ADDR_PROTECTION_EN;

	/* Mask port command error interrupt */
	DDR_CTL_REG(ddr_ctrl, 136) |= PORT_COMMAND_CHANNEL_ERROR;

	DDR_CTL_REG(ddr_ctrl, 0) |= 1;
	/* WAIT for initialization complete : bit 8 of INT_STATUS (DENALI_CTL_132) 0x210 */
	while ((DDR_CTL_REG(ddr_ctrl, 132) & MC_INIT_COMPLETE) != 0) {
		;
	}

	uint64_t ddr_end = (uint64_t)ddr_ctrl->ddr_start + ddr_ctrl->ddr_size;

	/* Disable the BusBlocker in front of the controller AXI slave ports */
	volatile uint64_t *filterreg = (volatile uint64_t *)ddr_ctrl->ddr_physical_filter;

	filterreg[0] = 0x0f00000000000000UL | (ddr_end >> 2);
	/*                ^^ RWX + TOR */

	ddr_phy_fixup(ddr_ctrl);
	return 0;
}

#define DDRCTL_NODE DT_NODELABEL(dmc)

static struct ddr_ctrl_data ddrctl_private_data = {
	.ddrctl = (uint32_t *)DT_REG_ADDR_BY_IDX(DDRCTL_NODE, 0),
	.ddrphy = (uint32_t *)DT_REG_ADDR_BY_IDX(DDRCTL_NODE, 1),
	.ddr_physical_filter = (uint32_t *)DT_REG_ADDR_BY_IDX(DDRCTL_NODE, 2),

	.ddr_start = (uint32_t *)DT_REG_ADDR(DT_NODELABEL(ram0)),
	.ddr_size = DT_REG_SIZE(DT_NODELABEL(ram0)),
};

DEVICE_DT_INST_DEFINE(0, ddr_init, NULL, &ddrctl_private_data, NULL, POST_KERNEL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);
