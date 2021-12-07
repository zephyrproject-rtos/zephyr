/*
 * Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stddef.h>
#include <string.h>
#include "mmc_dw_ll.h"
#include <socfpga_reset_manager.h>
#include <socfpga_system_manager.h>

#define DWMMC_CTRL			(0x00)
#define CTRL_IDMAC_EN			BIT(25)
#define CTRL_DMA_EN			BIT(5)
#define CTRL_INT_EN			BIT(4)
#define CTRL_DMA_RESET			BIT(2)
#define CTRL_FIFO_RESET			BIT(1)
#define CTRL_RESET			BIT(0)
#define CTRL_RESET_ALL			(CTRL_DMA_RESET | CTRL_FIFO_RESET | \
					 CTRL_RESET)

#define DWMMC_PWREN			(0x04)
#define DWMMC_CLKDIV			(0x08)
#define DWMMC_CLKSRC			(0x0c)
#define DWMMC_CLKENA			(0x10)
#define DWMMC_TMOUT			(0x14)
#define DWMMC_CTYPE			(0x18)
#define CTYPE_8BIT			BIT(16)
#define CTYPE_4BIT			(1)
#define CTYPE_1BIT			(0)

#define DWMMC_BLKSIZ			(0x1c)
#define DWMMC_BYTCNT			(0x20)
#define DWMMC_INTMASK			(0x24)
#define INT_EBE				BIT(15)
#define INT_SBE				BIT(13)
#define INT_HLE				BIT(12)
#define INT_FRUN			BIT(11)
#define INT_DRT				BIT(9)
#define INT_RTO				BIT(8)
#define INT_DCRC			BIT(7)
#define INT_RCRC			BIT(6)
#define INT_RXDR			BIT(5)
#define INT_TXDR			BIT(4)
#define INT_DTO				BIT(3)
#define INT_CMD_DONE			BIT(2)
#define INT_RE				BIT(1)

#define DWMMC_CMDARG			(0x28)
#define DWMMC_CMD			(0x2c)
#define CMD_START			BIT(31)
#define CMD_USE_HOLD_REG		BIT(29)	/* 0 if SDR50/100 */
#define CMD_UPDATE_CLK_ONLY		BIT(21)
#define CMD_SEND_INIT			BIT(15)
#define CMD_STOP_ABORT_CMD		BIT(14)
#define CMD_WAIT_PRVDATA_COMPLETE	BIT(13)
#define CMD_WRITE			BIT(10)
#define CMD_DATA_TRANS_EXPECT		BIT(9)
#define CMD_CHECK_RESP_CRC		BIT(8)
#define CMD_RESP_LEN			BIT(7)
#define CMD_RESP_EXPECT			BIT(6)
#define CMD(x)				FIELD_GET(0x3f, x)

#define DWMMC_RESP0			(0x30)
#define DWMMC_RESP1			(0x34)
#define DWMMC_RESP2			(0x38)
#define DWMMC_RESP3			(0x3c)
#define DWMMC_RINTSTS			(0x44)
#define DWMMC_STATUS			(0x48)
#define STATUS_DATA_BUSY		BIT(9)

#define DWMMC_FIFOTH			(0x4c)
#define FIFOTH_TWMARK(x)		FIELD_GET(0xfff, x)
#define FIFOTH_RWMARK(x)		(FIELD_GET(0x1ff, x) << 16)
#define FIFOTH_DMA_BURST_SIZE(x)	(FIELD_GET(0x7, x) << 28)

#define DWMMC_DEBNCE			(0x64)
#define DWMMC_BMOD			(0x80)
#define BMOD_ENABLE			BIT(7)
#define BMOD_FB				BIT(1)
#define BMOD_SWRESET			BIT(0)

#define DWMMC_DBADDR			(0x88)
#define DWMMC_IDSTS			(0x8c)
#define DWMMC_IDINTEN			(0x90)
#define DWMMC_CARDTHRCTL		(0x100)
#define CARDTHRCTL_RD_THR(x)		(FIELD_GET(0xfff, x) << 16)
#define CARDTHRCTL_RD_THR_EN		BIT(0)

#define IDMAC_DES0_DIC			BIT(1)
#define IDMAC_DES0_LD			BIT(2)
#define IDMAC_DES0_FS			BIT(3)
#define IDMAC_DES0_CH			BIT(4)
#define IDMAC_DES0_ER			BIT(5)
#define IDMAC_DES0_CES			BIT(30)
#define IDMAC_DES0_OWN			BIT(31)
#define IDMAC_DES1_BS1(x)		FIELD_GET(0x1fff, x)
#define IDMAC_DES2_BS2(x)		(FIELD_GET(0x1fff, x) << 13)

#define DWMMC_DMA_MAX_BUFFER_SIZE	(512 * 8)

#define DWMMC_8BIT_MODE			BIT(6)

#define DWMMC_ADDRESS_MASK		U(0x0f)

#define TIMEOUT				100000

static void dw_init(void);
static int dw_send_cmd(struct mmc_cmd *cmd);
static int dw_set_ios(unsigned int clk, unsigned int width);
static int dw_prepare(int lba, uintptr_t buf, size_t size);
static int dw_read(int lba, uintptr_t buf, size_t size);
static int dw_write(int lba, uintptr_t buf, size_t size);

static const struct mmc_ops dw_mmc_ops = {
	.init		= dw_init,
	.send_cmd	= dw_send_cmd,
	.set_ios	= dw_set_ios,
	.prepare	= dw_prepare,
	.read		= dw_read,
	.write		= dw_write,
};

static dw_mmc_params_t dw_params;

static void dw_pwr_on(void)
{
	sys_write32(0x1, dw_params.reg_base + DWMMC_PWREN);
}

static void dw_pwr_off(void)
{
	sys_write32(0x0, dw_params.reg_base + DWMMC_PWREN);
}

static void dw_reset(void)
{
	/* Turn off power */
	dw_pwr_off();

	/* Reset sdmmc by reset manager */

	/* set bit per0 modrst addr sdmmc */
	sys_set_bits(SOCFPGA_RSTMGR(PER0MODRST),
			RSTMGR_FIELD(PER0, SDMMCOCP));
	sys_set_bits(SOCFPGA_RSTMGR(PER0MODRST),
			RSTMGR_FIELD(PER0, SDMMC));

	k_busy_wait(100);

	/* clr bit per0 modrst addr sdmmc */
	sys_clear_bits(SOCFPGA_RSTMGR(PER0MODRST),
			RSTMGR_FIELD(PER0, SDMMCOCP));
	sys_clear_bits(SOCFPGA_RSTMGR(PER0MODRST),
			RSTMGR_FIELD(PER0, SDMMC));

	/* Turn on power */
	dw_pwr_on();
}

static void dw_update_clk(void)
{
	unsigned int data;

	sys_write32(CMD_WAIT_PRVDATA_COMPLETE | CMD_UPDATE_CLK_ONLY |
		    CMD_START, dw_params.reg_base + DWMMC_CMD);
	while (1) {
		data = sys_read32(dw_params.reg_base + DWMMC_CMD);
		if ((data & CMD_START) == 0)
			break;
		data = sys_read32(dw_params.reg_base + DWMMC_RINTSTS);
		__ASSERT_NO_MSG((data & INT_HLE) == 0);
	}
}

static void dw_set_clk(int clk)
{
	unsigned int data;
	int div;

	__ASSERT_NO_MSG(clk > 0);

	for (div = 1; div < 256; div++) {
		if ((dw_params.clk_rate / (2 * div)) <= clk) {
			break;
		}
	}
	__ASSERT_NO_MSG(div < 256);

	/* wait until controller is idle */
	do {
		data = sys_read32(dw_params.reg_base + DWMMC_STATUS);
	} while (data & STATUS_DATA_BUSY);

	/* disable clock before change clock rate */
	sys_write32(0, dw_params.reg_base + DWMMC_CLKENA);
	dw_update_clk();

	sys_write32(div, dw_params.reg_base + DWMMC_CLKDIV);
	dw_update_clk();

	/* enable clock */
	sys_write32(1, dw_params.reg_base + DWMMC_CLKENA);
	sys_write32(0, dw_params.reg_base + DWMMC_CLKSRC);
	dw_update_clk();
}

static void dw_init(void)
{
	unsigned int data;
	uintptr_t base;

	__ASSERT_NO_MSG((dw_params.reg_base & MMC_BLOCK_MASK) == 0);

	base = dw_params.reg_base;
	sys_write32(1, base + DWMMC_PWREN);
	sys_write32(CTRL_RESET_ALL, base + DWMMC_CTRL);
	do {
		data = sys_read32(base + DWMMC_CTRL);
	} while (data);

	/* enable DMA in CTRL */
	data = CTRL_INT_EN | CTRL_DMA_EN | CTRL_IDMAC_EN;
	sys_write32(data, base + DWMMC_CTRL);
	sys_write32(~0, base + DWMMC_RINTSTS);
	sys_write32(0, base + DWMMC_INTMASK);
	sys_write32(~0, base + DWMMC_TMOUT);
	sys_write32(~0, base + DWMMC_IDINTEN);
	sys_write32(MMC_BLOCK_SIZE, base + DWMMC_BLKSIZ);
	sys_write32(256 * 1024, base + DWMMC_BYTCNT);
	sys_write32(0x00ffffff, base + DWMMC_DEBNCE);
	sys_write32(BMOD_SWRESET, base + DWMMC_BMOD);
	do {
		data = sys_read32(base + DWMMC_BMOD);
	} while (data & BMOD_SWRESET);
	/* enable DMA in BMOD */
	data |= BMOD_ENABLE | BMOD_FB;
	sys_write32(data, base + DWMMC_BMOD);

	k_busy_wait(100);
	dw_set_clk(MMC_BOOT_CLK_RATE);
	k_busy_wait(100);
}

static int dw_send_cmd(struct mmc_cmd *cmd)
{
	unsigned int op, data, err_mask;
	uintptr_t base;
	int timeout;

	__ASSERT_NO_MSG(cmd);

	base = dw_params.reg_base;

	switch (cmd->cmd_idx) {
	case 0:
		op = CMD_SEND_INIT;
		break;
	case 12:
		op = CMD_STOP_ABORT_CMD;
		break;
	case 13:
		op = CMD_WAIT_PRVDATA_COMPLETE;
		break;
	case 8:
		if (dw_params.mmc_dev_type == MMC_IS_EMMC)
			op = CMD_DATA_TRANS_EXPECT | CMD_WAIT_PRVDATA_COMPLETE;
		else
			op = CMD_WAIT_PRVDATA_COMPLETE;
		break;
	case 17:
	case 18:
		op = CMD_DATA_TRANS_EXPECT | CMD_WAIT_PRVDATA_COMPLETE;
		break;
	case 24:
	case 25:
		op = CMD_WRITE | CMD_DATA_TRANS_EXPECT |
		     CMD_WAIT_PRVDATA_COMPLETE;
		break;
	case 51:
		op = CMD_DATA_TRANS_EXPECT;
		break;
	default:
		op = 0;
		break;
	}
	op |= CMD_USE_HOLD_REG | CMD_START;
	switch (cmd->resp_type) {
	case 0:
		break;
	case MMC_RESPONSE_R2:
		op |= CMD_RESP_EXPECT | CMD_CHECK_RESP_CRC |
		      CMD_RESP_LEN;
		break;
	case MMC_RESPONSE_R3:
		op |= CMD_RESP_EXPECT;
		break;
	default:
		op |= CMD_RESP_EXPECT | CMD_CHECK_RESP_CRC;
		break;
	}
	timeout = TIMEOUT;
	do {
		k_busy_wait(500);
		data = sys_read32(base + DWMMC_STATUS);
		if (--timeout <= 0)
			k_panic();
	} while (data & STATUS_DATA_BUSY);

	sys_write32(~0, base + DWMMC_RINTSTS);
	sys_write32(cmd->cmd_arg, base + DWMMC_CMDARG);
	sys_write32(op | cmd->cmd_idx, base + DWMMC_CMD);

	err_mask = INT_EBE | INT_HLE | INT_RTO | INT_RCRC | INT_RE |
		   INT_DCRC | INT_DRT | INT_SBE;
	timeout = TIMEOUT;
	do {
		k_busy_wait(500);
		data = sys_read32(base + DWMMC_RINTSTS);
		if (data & err_mask)
			return -EIO;
		if (data & INT_DTO)
			break;
		if (--timeout == 0) {
			printk("ERROR: %s, RINTSTS:0x%x\n", __func__, data);
			k_panic();
		}
	} while (!(data & INT_CMD_DONE));

	if (op & CMD_RESP_EXPECT) {
		cmd->resp_data[0] = sys_read32(base + DWMMC_RESP0);
		if (op & CMD_RESP_LEN) {
			cmd->resp_data[1] = sys_read32(base + DWMMC_RESP1);
			cmd->resp_data[2] = sys_read32(base + DWMMC_RESP2);
			cmd->resp_data[3] = sys_read32(base + DWMMC_RESP3);
		}
	}
	return 0;
}

static int dw_set_ios(unsigned int clk, unsigned int width)
{
	switch (width) {
	case MMC_BUS_WIDTH_1:
		sys_write32(CTYPE_1BIT, dw_params.reg_base + DWMMC_CTYPE);
		break;
	case MMC_BUS_WIDTH_4:
		sys_write32(CTYPE_4BIT, dw_params.reg_base + DWMMC_CTYPE);
		break;
	case MMC_BUS_WIDTH_8:
		sys_write32(CTYPE_8BIT, dw_params.reg_base + DWMMC_CTYPE);
		break;
	default:
		__ASSERT_NO_MSG(0);
		break;
	}
	dw_set_clk(clk);
	return 0;
}

static int dw_prepare(int lba, uintptr_t buf, size_t size)
{
	struct dw_idmac_desc *desc;
	int desc_cnt, i, last;
	uintptr_t base;

	__ASSERT_NO_MSG(((buf & DWMMC_ADDRESS_MASK) == 0) &&
	       (dw_params.desc_size > 0) &&
	       ((dw_params.reg_base & MMC_BLOCK_MASK) == 0) &&
	       ((dw_params.desc_base & MMC_BLOCK_MASK) == 0) &&
	       ((dw_params.desc_size & MMC_BLOCK_MASK) == 0));

	arch_dcache_range((void *)buf, size, K_CACHE_WB);

	desc_cnt = (size + DWMMC_DMA_MAX_BUFFER_SIZE - 1) /
		   DWMMC_DMA_MAX_BUFFER_SIZE;
	__ASSERT_NO_MSG(desc_cnt * sizeof(struct dw_idmac_desc) < dw_params.desc_size);

	base = dw_params.reg_base;
	desc = (struct dw_idmac_desc *)dw_params.desc_base;
	sys_write32(size, base + DWMMC_BYTCNT);

	if (size < MMC_BLOCK_SIZE)
		sys_write32(size, base + DWMMC_BLKSIZ);
	else
		sys_write32(MMC_BLOCK_SIZE, base + DWMMC_BLKSIZ);

	sys_write32(~0, base + DWMMC_RINTSTS);
	for (i = 0; i < desc_cnt; i++) {
		desc[i].des0 = IDMAC_DES0_OWN | IDMAC_DES0_CH | IDMAC_DES0_DIC;
		desc[i].des1 = IDMAC_DES1_BS1(DWMMC_DMA_MAX_BUFFER_SIZE);
		desc[i].des2 = buf + DWMMC_DMA_MAX_BUFFER_SIZE * i;
		desc[i].des3 = dw_params.desc_base +
			(sizeof(struct dw_idmac_desc)) * (i + 1);
	}
	/* first descriptor */
	desc->des0 |= IDMAC_DES0_FS;
	/* last descriptor */
	last = desc_cnt - 1;
	(desc + last)->des0 |= IDMAC_DES0_LD;
	(desc + last)->des0 &= ~(IDMAC_DES0_DIC | IDMAC_DES0_CH);
	(desc + last)->des1 = IDMAC_DES1_BS1(size - (last *
				  DWMMC_DMA_MAX_BUFFER_SIZE));
	/* set next descriptor address as 0 */
	(desc + last)->des3 = 0;

	sys_write32(dw_params.desc_base, base + DWMMC_DBADDR);
	arch_dcache_range((void *)dw_params.desc_base,
			   desc_cnt * DWMMC_DMA_MAX_BUFFER_SIZE,
			   K_CACHE_WB);

	return 0;
}

static int dw_read(int lba, uintptr_t buf, size_t size)
{
	uint32_t data = 0;
	int timeout = TIMEOUT;

	do {
		data = sys_read32(dw_params.reg_base + DWMMC_RINTSTS);
		k_busy_wait(50);
	} while (!(data & INT_DTO) && timeout-- > 0);

	arch_dcache_range((void *)buf, size, K_CACHE_INVD);
	return 0;
}

static int dw_write(int lba, uintptr_t buf, size_t size)
{
	return 0;
}

void dw_mmc_init(dw_mmc_params_t *params, struct mmc_device_info *info)
{
	__ASSERT_NO_MSG((params != 0) &&
	       ((params->reg_base & MMC_BLOCK_MASK) == 0) &&
	       ((params->desc_base & MMC_BLOCK_MASK) == 0) &&
	       ((params->desc_size & MMC_BLOCK_MASK) == 0) &&
	       (params->desc_size > 0) &&
	       (params->clk_rate > 0) &&
	       ((params->bus_width == MMC_BUS_WIDTH_1) ||
		(params->bus_width == MMC_BUS_WIDTH_4) ||
		(params->bus_width == MMC_BUS_WIDTH_8)));

	memcpy(&dw_params, params, sizeof(dw_mmc_params_t));
	dw_params.mmc_dev_type = info->mmc_dev_type;

	dw_reset();
	mmc_init(&dw_mmc_ops, params->clk_rate, params->bus_width,
		 params->flags, info);

}
