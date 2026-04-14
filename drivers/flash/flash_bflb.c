/*
 * Copyright (c) 2024-2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The bouffalolab serial flash controller provides 1 (BL60x only), or 2 banks of SPI controls.
 * There are two interactions modes: Memory-mapped (XIP) and direct, which are mutually exclusive.
 * Memory-mapping is achieved by providing read (and write) commands to the SF at the area
 * corresponding to the chosen bank, which will then automatically talk to the device.
 * The control is done via the Instruction bus which runs directly between the CPU ('s cache)
 * and the SF controller.
 * Direct mode enables the CPU to directly control the SF's interactions with the device by passing
 * commands and writing/reading data directly. The control is done via the System bus (normal bus
 * used to talk to peripherals).
 * Direct mode is achieved in different ways:
 * On E24 cpus SoCs (BL60x, BL70x/L), only one System bus interace is available. Switching is done
 * via a bank selection flag.
 * On e907 CPUs (BL61x/CL, BL808...), two system buses are available. The second bus is selected
 * by enabling it and turning on its selection flag, then enabling system bus mode on the interface.
 *
 * Devices are available via pads ('SF' registers), which indicate the mapping between pins and
 * interfaces. Some provide the ability to swap pins.
 */

#define DT_DRV_COMPAT bflb_sf_controller

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/cache.h>
#include <zephyr/sys/byteorder.h>

#include <soc.h>
#include <bflb_soc.h>
#include <glb_reg.h>
#include <sf_ctrl_reg.h>
#include <common_defines.h>
#include <hbn_reg.h>
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>

#include "spi_nor.h"
#include "jesd216.h"

#if defined(CONFIG_SOC_SERIES_BL60X) || defined(CONFIG_SOC_SERIES_BL70X) || \
	defined(CONFIG_SOC_SERIES_BL70XL)
#include <l1c_reg.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_bflb, CONFIG_FLASH_LOG_LEVEL);

#ifdef CONFIG_SOC_SERIES_BL60X
#define BFLB_XIP_BASE_BANK1	BL602_FLASH_XIP_BASE
#define BFLB_XIP_BASE_BANK2	-1
#define BFLB_XIP_SIZE		(BL602_FLASH_XIP_END - BL602_FLASH_XIP_BASE)
#define BFLB_SF_CLK_REG_OFF	GLB_CLK_CFG2_OFFSET
#define BFLB_HAS_IF2		0
#define BFLB_HAS_32B		0
#elif defined(CONFIG_SOC_SERIES_BL70X)
#define BFLB_XIP_BASE_BANK1	BL702_FLASH_XIP_BASE
#define BFLB_XIP_BASE_BANK2	BL702_PSRAM_XIP_BASE
#define BFLB_XIP_SIZE		(BL702_FLASH_XIP_END - BL702_FLASH_XIP_BASE)
#define BFLB_SF_CLK_REG_OFF	GLB_CLK_CFG2_OFFSET
#define BFLB_HAS_IF2		0
#define BFLB_HAS_32B		0
#elif defined(CONFIG_SOC_SERIES_BL70XL)
#define BFLB_XIP_BASE_BANK1	BL70XL_FLASH_XIP_BASE
#define BFLB_XIP_BASE_BANK2	BL70XL_PSRAM_XIP_BASE
#define BFLB_XIP_SIZE		(BL70XL_FLASH_XIP_END - BL70XL_FLASH_XIP_BASE)
#define BFLB_SF_CLK_REG_OFF	GLB_CLK_CFG2_OFFSET
#define BFLB_HAS_IF2		0
#define BFLB_HAS_32B		0
#elif defined(CONFIG_SOC_SERIES_BL61X)
#define BFLB_XIP_BASE_BANK1	BL616_FLASH_XIP_BASE
#define BFLB_XIP_BASE_BANK2	BL616_FLASH2_XIP_BUSREMAP_BASE
#define BFLB_XIP_SIZE		(BL616_FLASH_XIP_END - BL616_FLASH_XIP_BASE)
#define BFLB_SF_CLK_REG_OFF	GLB_SF_CFG0_OFFSET
#define BFLB_HAS_IF2		1
#define BFLB_HAS_32B		1
#endif

/* 'Roughly' x ms */
#define BFLB_FLASH_CONTROLLER_BUSY_TIMEOUT	8
#define BFLB_FLASH_CHIP_BUSY_TIMEOUT		100
#define BFLB_FLASH_CHIP_RESET_TIMEOUT		50
#define BFLB_FLASH_1RMS				16000
#define BFLB_FLASH_1RUS				16
#define BFLB_FLASH_1RUS21RMS			1000

#define BFLB_FLASH_SF_BUF_SIZE			256

#define BFLB_FLASH_ADDR_SIZE			3
#define BFLB_FLASH_ADDR_SIZE_32B		4
#define BFLB_FLASH_ADDR_SIZE_CONTREAD_ADD	1

#define ERASE_VALUE				0xFF
#define DUMMY_SECTOR_SIZE			4096
#define DUMMY_PAGE_SIZE				256
#define DUMMY_WRITE_ALIGN			4

#define FLASH_READ32(address)		(*((volatile uint32_t *)(address)))
#define FLASH_WRITE32(value, address)	(*((volatile uint32_t *)(address))) = value;

/* This correctly relocates this section to ITCM, fallback to ram if itcm is not set.
 * Prefer RAM on e24 platforms as ITCM is very limited.
 */
#if DT_HAS_CHOSEN(zephyr_itcm) && defined(CONFIG_SOC_SERIES_BL61X)
#define __nxipfunc			__GENERIC_SECTION(.itcm)
#else
#define __nxipfunc			__ramfunc
#endif

#include "flash_bflb.h"

#define ADDR_SIZE(_data) COND_CODE_1(BFLB_HAS_32B,						\
	(_data->controller->addr_32bits ? BFLB_FLASH_ADDR_SIZE_32B : BFLB_FLASH_ADDR_SIZE),	\
	(BFLB_FLASH_ADDR_SIZE))

typedef void (*flash_bflb_nxip_message)(uint32_t arg1, uint32_t arg2, uint32_t arg3);

static void flash_bflb_nxip_message_read_invalid(uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	LOG_WRN("Header read command (%x) doesn't match DTS read command (%x)", arg1, arg2);
}

static void flash_bflb_nxip_message_bad_bus_iahb(uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	LOG_WRN("Flash's Bus must be Instruction AHB and not System AHB");
}

static void flash_bflb_nxip_message_bad_bus_sahb(uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	LOG_WRN("Flash's Bus must be System AHB and not Instruction AHB");
}

static void flash_bflb_nxip_message_bad_qe(uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	LOG_ERR("Quad enable setup is not supported");
}

static void flash_bflb_nxip_message_busy(uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	LOG_ERR("Controller is busy!");
}

static void flash_bflb_nxip_message_busy_flash(uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	LOG_ERR("Flash is busy!");
}

static void flash_bflb_nxip_message_bad_sfdp(uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	LOG_ERR("Serial Flash Discovery Protocol error, code: %u %x", arg1, arg2);
}

static void flash_bflb_nxip_message_sad_sfdp(uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	LOG_WRN("Serial Flash Discovery Protocol unexpected, code: %u %x", arg1, arg2);
}

static void flash_bflb_nxip_message_notsup_sfdp(uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	LOG_ERR("Discovered flash cannot be supported, dw %u bits %x", arg1, arg2);
}

static void flash_bflb_nxip_message_sadsup_sfdp(uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	LOG_WRN("Discovered flash cannot be supported fully, dw %u bits %x", arg1, arg2);
}

static void flash_bflb_nxip_message_initseq_fail(uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	LOG_ERR("Initialization sequence failed, %x %x %x", arg1, arg2, arg3);
}

static void flash_bflb_nxip_message_sfdp_badsize(uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	LOG_WRN("Discovered flash does not have tree capacity:%d vs %d", arg1, arg2);
}

static const flash_bflb_nxip_message flash_bflb_nxip_messages[NXIP_MSG_MAX] = {
	[NXIP_MSG_READ_INVALID] = flash_bflb_nxip_message_read_invalid,
	[NXIP_MSG_BAD_BUS_IAHB] = flash_bflb_nxip_message_bad_bus_iahb,
	[NXIP_MSG_BAD_BUS_SAHB] = flash_bflb_nxip_message_bad_bus_sahb,
	[NXIP_MSG_BAD_QE] = flash_bflb_nxip_message_bad_qe,
	[NXIP_MSG_BUSY] = flash_bflb_nxip_message_busy,
	[NXIP_MSG_BUSY_FLASH] = flash_bflb_nxip_message_busy_flash,
	[NXIP_MSG_BAD_SFDP] = flash_bflb_nxip_message_bad_sfdp,
	[NXIP_MSG_SAD_SFDP] = flash_bflb_nxip_message_sad_sfdp,
	[NXIP_MSG_NOTSUP_SFDP] = flash_bflb_nxip_message_notsup_sfdp,
	[NXIP_MSG_SADSUP_SFDP] = flash_bflb_nxip_message_sadsup_sfdp,
	[NXIP_MSG_INITSEQ_FAIL] = flash_bflb_nxip_message_initseq_fail,
	[NXIP_MSG_SFDP_BADSIZE] = flash_bflb_nxip_message_sfdp_badsize,
};

static __nxipfunc void flash_bflb_nxip_message_set(struct flash_bflb_bank_data *data,
					enum flash_bflb_nxip_message_id id,
					uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	data->nxip_message = id >= NXIP_MSG_MAX ? NXIP_MSG_MAX - 1 : id;
	data->nxip_message_args[0] = arg1;
	data->nxip_message_args[1] = arg2;
	data->nxip_message_args[2] = arg3;
}

static void flash_bflb_nxip_message_clear(struct flash_bflb_bank_data *data)
{
	if (data->nxip_message > NXIP_MSG_NONE) {
		flash_bflb_nxip_messages[data->nxip_message](data->nxip_message_args[0],
							     data->nxip_message_args[1],
							     data->nxip_message_args[2]);
		data->nxip_message = NXIP_MSG_NONE;
	}
}

/* Will using function cause error ? */
static __nxipfunc bool flash_bflb_is_in_xip(struct flash_bflb_bank_data *data, void *func)
{
	if (((uint32_t)func >= (BFLB_XIP_BASE_BANK1)
	    && (uint32_t)func < (BFLB_XIP_BASE_BANK1 + BFLB_XIP_SIZE))
	    || ((uint32_t)func >= (BFLB_XIP_BASE_BANK2)
	    && (uint32_t)func < (BFLB_XIP_BASE_BANK2 + BFLB_XIP_SIZE))) {
		LOG_ERR("function at %p is in a SF controller region and will crash the device",
			func);
		return true;
	}

	return false;
}

/* Are we doing something that makes sense? ? */
static int flash_bflb_is_valid_range(struct flash_bflb_bank_data *data, off_t offset, size_t len)
{
	if (offset < 0) {
		LOG_WRN("0x%lx: before start of flash", (long)offset);
		return -EINVAL;
	}
	if ((data->cfg.size - offset) < len || len > data->cfg.size) {
		LOG_WRN("0x%lx: ends past the end of flash", (long)offset);
		return -EINVAL;
	}

	return 0;
}

static __nxipfunc void flash_bflb_settle_x(size_t cnt)
{
	for (size_t i = 0; i < cnt; i++) {
		__asm__ volatile (".rept 20 ; nop ; .endr");
	}
}

#if !DT_ANY_INST_HAS_BOOL_STATUS_OKAY(no_header)

static __nxipfunc void flash_bflb_set_default_read_header(struct flash_bflb_bank_data *data,
						struct bflb_header_flash_cfg *flash_header_cfg)
{
	switch (data->cfg.auto_spi_mode) {
	default:
	case BUS_NIO:
		data->cfg.cmd.auto_read = flash_header_cfg->fast_read_cmd;
		data->cfg.cmd.auto_read_dmycy = flash_header_cfg->fr_dmy_clk;
	break;
	case BUS_DO:
		data->cfg.cmd.auto_read = flash_header_cfg->fast_read_do_cmd;
		data->cfg.cmd.auto_read_dmycy = flash_header_cfg->fr_do_dmy_clk;
	break;
	case BUS_QO:
		data->cfg.cmd.auto_read = flash_header_cfg->fast_read_qo_cmd;
		data->cfg.cmd.auto_read_dmycy = flash_header_cfg->fr_qo_dmy_clk;
	break;
	case BUS_DIO:
		data->cfg.cmd.auto_read = flash_header_cfg->fast_read_dio_cmd;
		data->cfg.cmd.auto_read_dmycy = flash_header_cfg->fr_dio_dmy_clk;
	break;
	case BUS_QIO:
		if (data->cfg.use_qpi) {
			data->cfg.cmd.auto_read = flash_header_cfg->qpi_fast_read_qio_cmd;
			data->cfg.cmd.auto_read_dmycy = flash_header_cfg->qpi_fr_qio_dmy_clk;
		} else {
			data->cfg.cmd.auto_read = flash_header_cfg->fast_read_qio_cmd;
			data->cfg.cmd.auto_read_dmycy = flash_header_cfg->fr_qio_dmy_clk;
		}
	break;
	}
}

#endif

static __nxipfunc void flash_bflb_set_default_read_default(struct flash_bflb_bank_data *data)
{
	switch (data->cfg.auto_spi_mode) {
	default:
	case BUS_NIO:
		data->cfg.cmd.auto_read = SPI_NOR_CMD_READ_FAST;
		data->cfg.cmd.auto_read_dmycy = 1;
	break;
	case BUS_DO:
		data->cfg.cmd.auto_read = SPI_NOR_CMD_DREAD;
		data->cfg.cmd.auto_read_dmycy = 1;
	break;
	case BUS_QO:
		data->cfg.cmd.auto_read = SPI_NOR_CMD_QREAD;
		data->cfg.cmd.auto_read_dmycy = 1;
	break;
	case BUS_DIO:
		data->cfg.cmd.auto_read = SPI_NOR_CMD_2READ;
		data->cfg.cmd.auto_read_dmycy = 1;
	break;
	case BUS_QIO:
		data->cfg.cmd.auto_read = SPI_NOR_CMD_4READ;
		data->cfg.cmd.auto_read_dmycy = 1;
	break;
	}
}

#ifdef CONFIG_SOC_FLASH_BFLB_SFDP

static __nxipfunc enum flash_bflb_bus_mode flash_bflb_pick_next_worse_xip(
								enum flash_bflb_bus_mode mode)
{
	switch (mode) {
	default:
	case BUS_NIO:
		return BUS_NIO;
	break;
	case BUS_DO:
		return BUS_NIO;
	break;
	case BUS_DIO:
		return BUS_DO;
	break;
	case BUS_QO:
		return BUS_DIO;
	break;
	case BUS_QIO:
		return BUS_QO;
	break;
	}
}

#endif

#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL60X) || \
	defined(CONFIG_SOC_SERIES_BL70XL)

static __nxipfunc void flash_bflb_l1c_wrap(bool enable)
{
	uint32_t tmp;
	bool caching = false;

	tmp = FLASH_READ32(L1C_BASE + L1C_CONFIG_OFFSET);
	/* disable cache */
	if ((tmp & L1C_CACHEABLE_MSK) != 0) {
		caching = true;
		tmp &= ~(1 << L1C_CACHEABLE_POS);
		FLASH_WRITE32(tmp, L1C_BASE + L1C_CONFIG_OFFSET);
	}

	tmp = FLASH_READ32(L1C_BASE + L1C_CONFIG_OFFSET);

	if (enable) {
		tmp &= ~L1C_WRAP_DIS_MSK;
	} else {
		tmp |= L1C_WRAP_DIS_MSK;
	}

	FLASH_WRITE32(tmp, L1C_BASE + L1C_CONFIG_OFFSET);

	if (caching) {
		tmp |= (1 << L1C_CACHEABLE_POS);
		FLASH_WRITE32(tmp, L1C_BASE + L1C_CONFIG_OFFSET);
	}
}


#elif defined(CONFIG_SOC_SERIES_BL61X)

static __nxipfunc void flash_bflb_l1c_wrap(bool enable)
{
	/* Do nothing on Bl61x: no L1C */
	ARG_UNUSED(enable);
}

#endif

#if BFLB_HAS_IF2

static __nxipfunc void flash_bflb_if2_enable(struct flash_bflb_bank_data *data, bool enable)
{
	uint32_t tmp;

	if (data->bank != BANK2) {
		return;
	}
	if (enable) {
		tmp = FLASH_READ32(data->reg + SF_CTRL_SF_IF2_CTRL_1_OFFSET);
		tmp |= SF_CTRL_SF_IF2_EN_MSK;
		FLASH_WRITE32(tmp, data->reg + SF_CTRL_SF_IF2_CTRL_1_OFFSET);

		/* This sets IF2 as the IF currently controlled by SAHB, once setup IF2 can
		 * be disabled.
		 */
		tmp = FLASH_READ32(data->reg + SF_CTRL_SF_IF2_CTRL_1_OFFSET);
		tmp |= SF_CTRL_SF_IF2_FN_SEL_MSK;
		FLASH_WRITE32(tmp, data->reg + SF_CTRL_SF_IF2_CTRL_1_OFFSET);

		tmp = FLASH_READ32(data->reg + SF_CTRL_SF_IF2_CTRL_0_OFFSET);

		tmp &= ~(SF_CTRL_SF_IF2_REPLACE_SF1_MSK
				| SF_CTRL_SF_IF2_REPLACE_SF2_MSK
				| SF_CTRL_SF_IF2_REPLACE_SF3_MSK);
		if (data->cfg.pad.id == PAD1) {
			tmp |= SF_CTRL_SF_IF2_REPLACE_SF1_MSK;
		} else if (data->cfg.pad.id == PAD2) {
			tmp |= SF_CTRL_SF_IF2_REPLACE_SF2_MSK;
		} else {
			tmp |= SF_CTRL_SF_IF2_REPLACE_SF3_MSK;
		}

		tmp &= SF_CTRL_SF_IF2_PAD_SEL_UMSK;
		tmp |= (data->cfg.pad.id - 1U) << SF_CTRL_SF_IF2_PAD_SEL_POS;

		FLASH_WRITE32(tmp, data->reg + SF_CTRL_SF_IF2_CTRL_0_OFFSET);
	} else {
		tmp = FLASH_READ32(data->reg + SF_CTRL_SF_IF2_CTRL_1_OFFSET);
		tmp &= ~SF_CTRL_SF_IF2_FN_SEL_MSK;
		FLASH_WRITE32(tmp, data->reg + SF_CTRL_SF_IF2_CTRL_1_OFFSET);

		tmp = FLASH_READ32(data->reg + SF_CTRL_SF_IF2_CTRL_0_OFFSET);
		tmp &= ~(SF_CTRL_SF_IF2_REPLACE_SF1_MSK
			 | SF_CTRL_SF_IF2_REPLACE_SF2_MSK
			 | SF_CTRL_SF_IF2_REPLACE_SF3_MSK);
		FLASH_WRITE32(tmp, data->reg + SF_CTRL_SF_IF2_CTRL_0_OFFSET);

		tmp = FLASH_READ32(data->reg + SF_CTRL_SF_IF2_CTRL_1_OFFSET);
		tmp &= ~SF_CTRL_SF_IF2_EN_MSK;
		FLASH_WRITE32(tmp, data->reg + SF_CTRL_SF_IF2_CTRL_1_OFFSET);
	}
}

static __nxipfunc uintptr_t flash_bflb_set_sahb(struct flash_bflb_bank_data *data)
{
	if (data->bank == BANK2) {

		flash_bflb_if2_enable(data, true);

		return data->reg + SF_CTRL_SF_IF2_SAHB_0_OFFSET;
	}

	flash_bflb_if2_enable(data, false);

	return data->reg + SF_CTRL_SF_IF_SAHB_0_OFFSET;
}

static __nxipfunc void flash_bflb_release_sahb(struct flash_bflb_bank_data *data)
{
	if (data->bank == BANK2) {
		flash_bflb_if2_enable(data, false);
	}
}

static __nxipfunc uintptr_t flash_bflb_get_if(struct flash_bflb_bank_data *data)
{
	if (data->bank == BANK2) {
		return data->reg + SF_CTRL_SF_IF2_CTRL_0_OFFSET;
	} else {
		return data->reg;
	}
}

#else

static __nxipfunc uintptr_t flash_bflb_get_if(struct flash_bflb_bank_data *data)
{
	return data->reg;
}

#if defined(CONFIG_SOC_SERIES_BL60X)

static __nxipfunc uintptr_t flash_bflb_set_sahb(struct flash_bflb_bank_data *data)
{
	return data->reg + SF_CTRL_SF_IF_SAHB_0_OFFSET;
}

#else

static __nxipfunc uintptr_t flash_bflb_set_sahb(struct flash_bflb_bank_data *data)
{
	uint32_t tmp;

	tmp = FLASH_READ32(data->reg + SF_CTRL_2_OFFSET);
	if (data->bank == BANK2) {
		tmp |= SF_CTRL_SF_IF_0_BK_SEL_MSK;
	} else {
		tmp &= ~SF_CTRL_SF_IF_0_BK_SEL_MSK;
	}
	FLASH_WRITE32(tmp, data->reg + SF_CTRL_2_OFFSET);

	return data->reg + SF_CTRL_SF_IF_SAHB_0_OFFSET;
}

#endif

static __nxipfunc void flash_bflb_release_sahb(struct flash_bflb_bank_data *data)
{
	/* Nothing to do */
}

#endif

#if defined(CONFIG_SOC_SERIES_BL61X)

static __nxipfunc void flash_bflb_select_pads(struct flash_bflb_bank_data *data,
					      enum flash_bflb_pad bank1, enum flash_bflb_pad bank2)
{
	uint32_t tmp;

	if (bank1 == PAD1 || bank2 == PAD1) {
		tmp = FLASH_READ32(GLB_BASE + GLB_PARM_CFG0_OFFSET);
		tmp |= GLB_SEL_EMBEDDED_SFLASH_MSK;
		FLASH_WRITE32(tmp, GLB_BASE + GLB_PARM_CFG0_OFFSET);
	} else {
		tmp = FLASH_READ32(GLB_BASE + GLB_PARM_CFG0_OFFSET);
		tmp &= ~GLB_SEL_EMBEDDED_SFLASH_MSK;
		FLASH_WRITE32(tmp, GLB_BASE + GLB_PARM_CFG0_OFFSET);
	}

	tmp = FLASH_READ32(data->reg + SF_CTRL_2_OFFSET);
	tmp &= ~SF_CTRL_SF_IF_BK_SWAP_MSK;

	if (bank1 == PAD1 && bank2 == PAD2) {
		tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
		tmp |= 0 << SF_CTRL_SF_IF_PAD_SEL_POS;
	} else if (bank1 == PAD2 && bank2 == PAD3) {
		tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
		tmp |= 1U << SF_CTRL_SF_IF_PAD_SEL_POS;
	} else if (bank1 == PAD3 && bank2 == PAD1) {
		tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
		tmp |= 2U << SF_CTRL_SF_IF_PAD_SEL_POS;
	} else if (bank1 == PAD2 && bank2 == PAD1) {
		tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
		tmp |= 0U << SF_CTRL_SF_IF_PAD_SEL_POS;
		tmp |= SF_CTRL_SF_IF_BK_SWAP_MSK;
		tmp |= SF_CTRL_SF_IF_BK2_EN_MSK;
	} else if (bank1 == PAD3 && bank2 == PAD2) {
		tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
		tmp |= 1U << SF_CTRL_SF_IF_PAD_SEL_POS;
		tmp |= SF_CTRL_SF_IF_BK_SWAP_MSK;
	} else if (bank1 == PAD1 && bank2 == PAD3) {
		tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
		tmp |= 2U << SF_CTRL_SF_IF_PAD_SEL_POS;
		tmp |= SF_CTRL_SF_IF_BK_SWAP_MSK;
	}

	FLASH_WRITE32(tmp, data->reg + SF_CTRL_2_OFFSET);
}

#elif defined(CONFIG_SOC_SERIES_BL60X)

static __nxipfunc void flash_bflb_select_pads(struct flash_bflb_bank_data *data,
					      enum flash_bflb_pad bank1, enum flash_bflb_pad bank2)
{
	uint32_t tmp;

	tmp = FLASH_READ32(data->reg + SF_CTRL_2_OFFSET);
	tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
	tmp |= (bank1 - 1U) << SF_CTRL_SF_IF_PAD_SEL_POS;
	FLASH_WRITE32(tmp, data->reg + SF_CTRL_2_OFFSET);
}

#elif defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL70XL)

static __nxipfunc void flash_bflb_select_pads(struct flash_bflb_bank_data *data,
					      enum flash_bflb_pad bank1, enum flash_bflb_pad bank2)
{
	uint32_t tmp;

	tmp = FLASH_READ32(data->reg + SF_CTRL_2_OFFSET);
	tmp &= ~SF_CTRL_SF_IF_BK_SWAP_MSK;

	if (bank1 == PAD1 && bank2 == PAD2) {
		tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
		tmp |= 0 << SF_CTRL_SF_IF_PAD_SEL_POS;
	} else if (bank1 == PAD2 && bank2 == PAD3) {
		tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
		tmp |= 1U << SF_CTRL_SF_IF_PAD_SEL_POS;
	} else if (bank1 == PAD3 && bank2 == PAD1) {
		tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
		tmp |= 2U << SF_CTRL_SF_IF_PAD_SEL_POS;
	} else if (bank1 == PAD2 && bank2 == PAD1) {
		tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
		tmp |= 0U << SF_CTRL_SF_IF_PAD_SEL_POS;
		tmp |= SF_CTRL_SF_IF_BK_SWAP_MSK;
	} else if (bank1 == PAD3 && bank2 == PAD2) {
		tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
		tmp |= 1U << SF_CTRL_SF_IF_PAD_SEL_POS;
		tmp |= SF_CTRL_SF_IF_BK_SWAP_MSK;
	} else if (bank1 == PAD1 && bank2 == PAD3) {
		tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
		tmp |= 2U << SF_CTRL_SF_IF_PAD_SEL_POS;
		tmp |= SF_CTRL_SF_IF_BK_SWAP_MSK;
	} else if (bank1 == bank2 && bank1 == PAD2) {
		tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
		tmp |= 1U << SF_CTRL_SF_IF_PAD_SEL_POS;
	} else if (bank1 == bank2 && bank1 == PAD3) {
		tmp &= ~SF_CTRL_SF_IF_PAD_SEL_MSK;
		tmp |= 2U << SF_CTRL_SF_IF_PAD_SEL_POS;
	}

	FLASH_WRITE32(tmp, data->reg + SF_CTRL_2_OFFSET);
}
#endif

/* Memcpy will not be in ram */
static __nxipfunc void flash_bflb_xip_memcpy(volatile uint8_t *address_from,
					     volatile uint8_t *address_to, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		address_to[i] = address_from[i];
	}
}

static __nxipfunc bool flash_bflb_busy_wait(struct flash_bflb_bank_data *data)
{
	uint32_t counter = 0;

	while ((FLASH_READ32(flash_bflb_get_if(data) + SF_CTRL_SF_IF_SAHB_0_OFFSET)
		& SF_CTRL_SF_IF_BUSY_MSK) != 0
		&& counter < BFLB_FLASH_CONTROLLER_BUSY_TIMEOUT * BFLB_FLASH_1RUS21RMS) {
		flash_bflb_settle_x(BFLB_FLASH_1RUS);
		counter++;
	}

	if ((FLASH_READ32(flash_bflb_get_if(data) + SF_CTRL_SF_IF_SAHB_0_OFFSET)
		& SF_CTRL_SF_IF_BUSY_MSK) != 0) {
		flash_bflb_nxip_message_set(data, NXIP_MSG_BUSY, 0, 0, 0);
		return true;
	}

	return false;
}

/* Sets which AHB the flash controller is being talked to from
 * 0: System AHB (AHB connected to everything, E24 System Port)
 * 1: Instruction AHB (a dedicated bus between flash controller and L1C)
 */
static __nxipfunc int flash_bflb_set_bus(struct flash_bflb_bank_data *data, uint8_t bus)
{
	uint32_t tmp;

	if (flash_bflb_busy_wait(data)) {
		return -EBUSY;
	}

	tmp = FLASH_READ32(data->reg + SF_CTRL_1_OFFSET);

	if (bus == 1) {
		tmp |= SF_CTRL_SF_IF_FN_SEL_MSK;
		tmp |= SF_CTRL_SF_AHB2SIF_EN_MSK;
	} else {
		tmp &= ~SF_CTRL_SF_IF_FN_SEL_MSK;
		tmp &= ~SF_CTRL_SF_AHB2SIF_EN_MSK;
	}

	FLASH_WRITE32(tmp, data->reg + SF_CTRL_1_OFFSET);

	return 0;
}

static __nxipfunc int flash_bflb_set_command_iahb(struct flash_bflb_bank_data *data,
						  struct bflb_flash_command *command,
						  bool doing_cmd)
{
	uint32_t tmp;
#if defined(CONFIG_SOC_SERIES_BL60X)
	uintptr_t reg_off = SF_CTRL_SF_IF_IAHB_0_OFFSET;
#else
	uintptr_t reg_off = data->bank == BANK2 ?
		SF_CTRL_SF_IF_IAHB_9_OFFSET : SF_CTRL_SF_IF_IAHB_0_OFFSET;
#endif

	if (flash_bflb_busy_wait(data)) {
		return -EBUSY;
	}

	tmp = FLASH_READ32(data->reg + SF_CTRL_1_OFFSET);

	if ((tmp & SF_CTRL_SF_IF_FN_SEL_MSK) == 0) {
		flash_bflb_nxip_message_set(data, NXIP_MSG_BAD_BUS_IAHB, 0, 0, 0);
		return -EINVAL;
	}

#if defined(CONFIG_SOC_SERIES_BL60X)
	FLASH_WRITE32(command->cmd_buf[0], data->reg + SF_CTRL_SF_IF_IAHB_1_OFFSET);
	FLASH_WRITE32(command->cmd_buf[1], data->reg + SF_CTRL_SF_IF_IAHB_2_OFFSET);
#else
	if (data->bank == BANK2) {
		FLASH_WRITE32(command->cmd_buf[0], data->reg + SF_CTRL_SF_IF_IAHB_10_OFFSET);
		FLASH_WRITE32(command->cmd_buf[1], data->reg + SF_CTRL_SF_IF_IAHB_11_OFFSET);
	} else {
		FLASH_WRITE32(command->cmd_buf[0], data->reg + SF_CTRL_SF_IF_IAHB_1_OFFSET);
		FLASH_WRITE32(command->cmd_buf[1], data->reg + SF_CTRL_SF_IF_IAHB_2_OFFSET);
	}
#endif

	tmp = FLASH_READ32(data->reg + reg_off);

	/* 4 lines or 1 line commands */
	if (command->cmd_mode == 0) {
		tmp &= ~SF_CTRL_SF_IF_1_QPI_MODE_EN_MSK;
	} else {
		tmp |= SF_CTRL_SF_IF_1_QPI_MODE_EN_MSK;
	}

	/* set SPI mode*/
	tmp &= ~SF_CTRL_SF_IF_1_SPI_MODE_MSK;
	tmp |= command->spi_mode << SF_CTRL_SF_IF_1_SPI_MODE_POS;

	tmp &= ~SF_CTRL_SF_IF_1_CMD_BYTE_MSK;
	/* we are doing a command */
	if (doing_cmd) {
		tmp |= SF_CTRL_SF_IF_1_CMD_EN_MSK;
	} else {
		tmp &= ~SF_CTRL_SF_IF_1_CMD_EN_MSK;
	}

	/* configure address */
	tmp &= ~SF_CTRL_SF_IF_1_ADR_BYTE_MSK;
	if (command->addr_size != 0) {
		tmp |= SF_CTRL_SF_IF_1_ADR_EN_MSK;
		tmp |= ((command->addr_size - 1) << SF_CTRL_SF_IF_1_ADR_BYTE_POS);
	} else {
		tmp &= ~SF_CTRL_SF_IF_1_ADR_EN_MSK;
	}

	/* configure dummy */
	tmp &= ~SF_CTRL_SF_IF_1_DMY_BYTE_MSK;
	if (command->dummy_clks != 0) {
		tmp |= SF_CTRL_SF_IF_1_DMY_EN_MSK;
		tmp |= ((command->dummy_clks - 1) << SF_CTRL_SF_IF_1_DMY_BYTE_POS);
	} else {
		tmp &= ~SF_CTRL_SF_IF_1_DMY_EN_MSK;
	}

	/* configure data */
	if (command->nb_data != 0) {
		tmp |= SF_CTRL_SF_IF_1_DAT_EN_MSK;
	} else {
		tmp &= ~SF_CTRL_SF_IF_1_DAT_EN_MSK;
	}

	/* are we writing ? */
	if (command->rw) {
		tmp |= SF_CTRL_SF_IF_1_DAT_RW_MSK;
	} else {
		tmp &= ~SF_CTRL_SF_IF_1_DAT_RW_MSK;
	}

	FLASH_WRITE32(tmp, data->reg + reg_off);

	return 0;
}

static __nxipfunc int flash_bflb_set_command_iahb_write(struct flash_bflb_bank_data *data,
							struct bflb_flash_command *command,
							bool doing_cmd)
{
	uint32_t tmp;

	if (flash_bflb_busy_wait(data)) {
		return -EBUSY;
	}

	tmp = FLASH_READ32(data->reg + SF_CTRL_1_OFFSET);

	if ((tmp & SF_CTRL_SF_IF_FN_SEL_MSK) == 0) {
		flash_bflb_nxip_message_set(data, NXIP_MSG_BAD_BUS_IAHB, 0, 0, 0);
		return -EINVAL;
	}

	FLASH_WRITE32(command->cmd_buf[0], data->reg + SF_CTRL_SF_IF_IAHB_4_OFFSET);
	FLASH_WRITE32(command->cmd_buf[1], data->reg + SF_CTRL_SF_IF_IAHB_5_OFFSET);

	tmp = FLASH_READ32(data->reg + SF_CTRL_SF_IF_IAHB_3_OFFSET);

	/* 4 lines or 1 line commands */
	if (command->cmd_mode == 0) {
		tmp &= ~SF_CTRL_SF_IF_2_QPI_MODE_EN_MSK;
	} else {
		tmp |= SF_CTRL_SF_IF_2_QPI_MODE_EN_MSK;
	}

	/* set SPI mode*/
	tmp &= ~SF_CTRL_SF_IF_2_SPI_MODE_MSK;
	tmp |= command->spi_mode << SF_CTRL_SF_IF_2_SPI_MODE_POS;

	tmp &= ~SF_CTRL_SF_IF_2_CMD_BYTE_MSK;
	/* we are doing a command */
	if (doing_cmd) {
		tmp |= SF_CTRL_SF_IF_2_CMD_EN_MSK;
	} else {
		tmp &= ~SF_CTRL_SF_IF_2_CMD_EN_MSK;
	}

	/* configure address */
	tmp &= ~SF_CTRL_SF_IF_2_ADR_BYTE_MSK;
	if (command->addr_size != 0) {
		tmp |= SF_CTRL_SF_IF_2_ADR_EN_MSK;
		tmp |= ((command->addr_size - 1) << SF_CTRL_SF_IF_2_ADR_BYTE_POS);
	} else {
		tmp &= ~SF_CTRL_SF_IF_2_ADR_EN_MSK;
	}

	/* configure dummy */
	tmp &= ~SF_CTRL_SF_IF_2_DMY_BYTE_MSK;
	if (command->dummy_clks != 0) {
		tmp |= SF_CTRL_SF_IF_2_DMY_EN_MSK;
		tmp |= ((command->dummy_clks - 1) << SF_CTRL_SF_IF_2_DMY_BYTE_POS);
	} else {
		tmp &= ~SF_CTRL_SF_IF_2_DMY_EN_MSK;
	}

	/* configure data */
	if (command->nb_data != 0) {
		tmp |= SF_CTRL_SF_IF_2_DAT_EN_MSK;
	} else {
		tmp &= ~SF_CTRL_SF_IF_2_DAT_EN_MSK;
	}

	/* are we writing ? */
	if (command->rw) {
		tmp |= SF_CTRL_SF_IF_2_DAT_RW_MSK;
	} else {
		tmp &= ~SF_CTRL_SF_IF_2_DAT_RW_MSK;
	}

	FLASH_WRITE32(tmp, data->reg + SF_CTRL_SF_IF_IAHB_3_OFFSET);

	return 0;
}

static __nxipfunc int flash_bflb_set_command_sahb(struct flash_bflb_bank_data *data,
						  struct bflb_flash_command *command,
						  bool doing_cmd)
{
	uint32_t tmp;
	uint32_t bank_offset = flash_bflb_get_if(data) + SF_CTRL_SF_IF_SAHB_0_OFFSET;

	FLASH_WRITE32(command->cmd_buf[0], bank_offset + 0x4);
	FLASH_WRITE32(command->cmd_buf[1], bank_offset + 0x8);

	tmp = FLASH_READ32(bank_offset + 0);

	/* 4 lines or 1 line commands */
	if (command->cmd_mode == 0) {
		tmp &= ~SF_CTRL_SF_IF_0_QPI_MODE_EN_MSK;
	} else {
		tmp |= SF_CTRL_SF_IF_0_QPI_MODE_EN_MSK;
	}

	/* set SPI mode */
	tmp &= ~SF_CTRL_SF_IF_0_SPI_MODE_MSK;
	tmp |= command->spi_mode << SF_CTRL_SF_IF_0_SPI_MODE_POS;

	tmp &= ~SF_CTRL_SF_IF_0_CMD_BYTE_MSK;
	/* we are doing a command */
	if (doing_cmd) {
		tmp |= SF_CTRL_SF_IF_0_CMD_EN_MSK;
	} else {
		tmp &= ~SF_CTRL_SF_IF_0_CMD_EN_MSK;
	}

	/* configure address */
	tmp &= ~SF_CTRL_SF_IF_0_ADR_BYTE_MSK;
	if (command->addr_size != 0) {
		tmp |= SF_CTRL_SF_IF_0_ADR_EN_MSK;
		tmp |= ((command->addr_size - 1U) << SF_CTRL_SF_IF_0_ADR_BYTE_POS);
	} else {
		tmp &= ~SF_CTRL_SF_IF_0_ADR_EN_MSK;
	}

	/* configure dummy */
	tmp &= ~SF_CTRL_SF_IF_0_DMY_BYTE_MSK;
	if (command->dummy_clks != 0) {
		tmp |= SF_CTRL_SF_IF_0_DMY_EN_MSK;
		tmp |= ((command->dummy_clks - 1U) << SF_CTRL_SF_IF_0_DMY_BYTE_POS);
	} else {
		tmp &= ~SF_CTRL_SF_IF_0_DMY_EN_MSK;
	}

	/* configure data */
	tmp &= ~SF_CTRL_SF_IF_0_DAT_BYTE_MSK;
	if (command->nb_data != 0) {
		tmp |= SF_CTRL_SF_IF_0_DAT_EN_MSK;
		tmp |= ((command->nb_data - 1U) << SF_CTRL_SF_IF_0_DAT_BYTE_POS);
	} else {
		tmp &= ~SF_CTRL_SF_IF_0_DAT_EN_MSK;
	}

	/* are we writing ? */
	if (command->rw) {
		tmp |= SF_CTRL_SF_IF_0_DAT_RW_MSK;
	} else {
		tmp &= ~SF_CTRL_SF_IF_0_DAT_RW_MSK;
	}
	FLASH_WRITE32(tmp, bank_offset + 0);

	return 0;
}

static __nxipfunc int flash_bflb_send_command(struct flash_bflb_bank_data *data,
					      struct bflb_flash_command *command)
{
	uint32_t tmp;
	int ret;
	uint32_t bank_offset = flash_bflb_get_if(data) + SF_CTRL_SF_IF_SAHB_0_OFFSET;

	if (flash_bflb_is_in_xip(data, &flash_bflb_send_command)) {
		return -ENOTSUP;
	}

	if (flash_bflb_busy_wait(data)) {
		return -EBUSY;
	}

	tmp = FLASH_READ32(data->reg + SF_CTRL_1_OFFSET);
	if (tmp & SF_CTRL_SF_IF_FN_SEL_MSK) {
		flash_bflb_nxip_message_set(data, NXIP_MSG_BAD_BUS_SAHB, 0, 0, 0);
		return -EINVAL;
	}

	/* make sure command detriggered */
	tmp = FLASH_READ32(bank_offset + 0);
	tmp &= ~SF_CTRL_SF_IF_0_TRIG_MSK;
	FLASH_WRITE32(tmp, bank_offset + 0);

	ret = flash_bflb_set_command_sahb(data, command, true);
	if (ret != 0) {
		return ret;
	}

#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL60X)
	tmp = FLASH_READ32(data->reg + SF_CTRL_0_OFFSET);
	tmp |= SF_CTRL_SF_CLK_SAHB_SRAM_SEL_MSK;
	FLASH_WRITE32(tmp, data->reg + SF_CTRL_0_OFFSET);
#endif

	/* trigger command */
	tmp = FLASH_READ32(bank_offset + 0);
	tmp |= SF_CTRL_SF_IF_0_TRIG_MSK;
	FLASH_WRITE32(tmp, bank_offset + 0);

	if (flash_bflb_busy_wait(data)) {
		ret = -EBUSY;
	}

#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL60X)
	tmp = FLASH_READ32(data->reg + SF_CTRL_0_OFFSET);
	tmp &= ~SF_CTRL_SF_CLK_SAHB_SRAM_SEL_MSK;
	FLASH_WRITE32(tmp, data->reg + SF_CTRL_0_OFFSET);
#endif

	return ret;
}

static __nxipfunc int flash_bflb_flash_send_triplet(struct flash_bflb_bank_data *data, uint8_t cmd,
						    uint32_t cdata, uint8_t len)
{
	struct bflb_flash_command triplet = {0};

	flash_bflb_xip_memcpy((uint8_t *)&cdata, (uint8_t *)SF_CTRL_BUF_BASE, len);

	triplet.spi_mode = data->cfg.manual_spi_mode;
	triplet.cmd_buf[0] = (uint32_t)cmd << 24;
	triplet.nb_data = len;
	triplet.rw = 1;

	return flash_bflb_send_command(data, &triplet);
}

static __nxipfunc int flash_bflb_flash_read_register(struct flash_bflb_bank_data *data,
						     uint8_t index,
						     uint8_t *out, uint8_t len)
{
	struct bflb_flash_command read_reg = {0};
	int ret;

	read_reg.spi_mode = data->cfg.manual_spi_mode;
	read_reg.cmd_buf[0] = (data->cfg.cmd.read_reg[index]) << 24;
	read_reg.nb_data = len;
	ret = flash_bflb_send_command(data, &read_reg);
	if (ret != 0) {
		return ret;
	}

	if (flash_bflb_busy_wait(data)) {
		return -EBUSY;
	}

	flash_bflb_xip_memcpy((uint8_t *)SF_CTRL_BUF_BASE, out, len);

	return 0;
}

static __nxipfunc int flash_bflb_flash_write_register(struct flash_bflb_bank_data *data,
						      uint8_t index,
						      uint8_t *in, uint8_t len)
{
	struct bflb_flash_command write_reg = {0};

	flash_bflb_xip_memcpy(in, (uint8_t *)SF_CTRL_BUF_BASE, len);

	write_reg.spi_mode = data->cfg.manual_spi_mode;
	write_reg.cmd_buf[0] = (data->cfg.cmd.write_reg[index]) << 24;
	write_reg.nb_data = len;
	write_reg.rw = 1;

	return flash_bflb_send_command(data, &write_reg);
}

static __nxipfunc int flash_bflb_flash_disable_continuous_read(struct flash_bflb_bank_data *data)
{
	struct bflb_flash_command disable_continuous_read = {0};

	/* Effectively send the stop continuous read command 4 or 5 times.
	 * This should work for all possible contread setups.
	 */
	disable_continuous_read.spi_mode = data->cfg.manual_spi_mode;
	disable_continuous_read.addr_size = ADDR_SIZE(data);
	disable_continuous_read.cmd_buf[0] = data->cfg.cmd.contread_off << 24 |
	data->cfg.cmd.contread_off << 16 | data->cfg.cmd.contread_off << 8 |
	data->cfg.cmd.contread_off;
	disable_continuous_read.cmd_buf[1] = data->cfg.cmd.contread_off << 24 |
	data->cfg.cmd.contread_off << 16 | data->cfg.cmd.contread_off << 8 |
	data->cfg.cmd.contread_off;

	return flash_bflb_send_command(data, &disable_continuous_read);
}

static __nxipfunc int flash_bflb_enable_writable(struct flash_bflb_bank_data *data)
{
	struct bflb_flash_command write_enable = {0};
	int ret;
	uint32_t write_reg;

	write_enable.spi_mode = data->cfg.manual_spi_mode;
	write_enable.cmd_buf[0] = (data->cfg.cmd.write_enable) << 24;
	ret = flash_bflb_send_command(data, &write_enable);
	if (ret != 0) {
		return ret;
	}

	if (data->cfg.reg.write_enable_read_len) {
		/* check writable */
		ret = flash_bflb_flash_read_register(data, data->cfg.reg.write_enable_index,
			(uint8_t *)&write_reg, data->cfg.reg.write_enable_read_len);
		if (ret != 0) {
			return ret;
		}

		if ((((uint8_t *)&write_reg)[data->cfg.reg.write_enable_read_len - 1]
			& BIT(data->cfg.reg.write_enable_bit)) != 0) {
			return 0;
		}
	} else {
		return 0;
	}

	return -EIO;
}

static __nxipfunc int flash_bflb_flash_set_burst(struct flash_bflb_bank_data *data, bool yes)
{
	struct bflb_flash_command enable_burstwrap = {0};
	int ret;
	uint32_t tmp;

	flash_bflb_l1c_wrap(true);

	ret = flash_bflb_enable_writable(data);
	if (ret != 0) {
		return ret;
	}

	/* Burst wrap commands are usually in QO mode */
	enable_burstwrap.spi_mode = BUS_QIO;
	enable_burstwrap.dummy_clks = data->cfg.cmd.burstwrap_dmycy;
	enable_burstwrap.cmd_buf[0] = data->cfg.cmd.burstwrap << 24;
	enable_burstwrap.nb_data = 1;
	enable_burstwrap.rw = 1;
	if (yes) {
		tmp = data->cfg.cmd.burstwrap_on_data;
	} else {
		tmp = data->cfg.cmd.burstwrap_off_data;
	}
	FLASH_WRITE32(tmp, SF_CTRL_BUF_BASE);

	return flash_bflb_send_command(data, &enable_burstwrap);
}

#if BFLB_HAS_32B

static __nxipfunc void flash_bflb_set_32b_enabled(struct flash_bflb_bank_data *data, bool yes)
{
	uint32_t tmp;

	tmp = FLASH_READ32(data->reg + SF_CTRL_0_OFFSET);
	if (yes) {
		tmp |= SF_CTRL_SF_IF_32B_ADR_EN_MSK;
	} else {
		tmp &= ~SF_CTRL_SF_IF_32B_ADR_EN_MSK;
	}
	FLASH_WRITE32(tmp, data->reg + SF_CTRL_0_OFFSET);
}

static __nxipfunc int flash_bflb_enable_32baddr(struct flash_bflb_bank_data *data)
{
	struct bflb_flash_command enable_32baddr = {0};

	flash_bflb_set_32b_enabled(data, true);

	enable_32baddr.spi_mode = data->cfg.manual_spi_mode;
	enable_32baddr.cmd_buf[0] = data->cfg.cmd.enter_32bits_addr << 24;

	return flash_bflb_send_command(data, &enable_32baddr);
}

#endif

/* (!= QPI enable) */
static __nxipfunc int flash_bflb_enable_qspi(struct flash_bflb_bank_data *data)
{
	int ret;
	uint32_t tmp = 0;

	/* No Quad Enable */
	if (data->cfg.reg.quad_enable_read_len == 0) {
		return 0;
	}

	/* If read length is not the same as write length, write all registers.
	 * No cases where more than 2 registers must be written
	 */
	if (data->cfg.reg.quad_enable_write_len < 1 || data->cfg.reg.quad_enable_write_len > 2
	    || data->cfg.reg.quad_enable_index > 1 || data->cfg.reg.quad_enable_read_len != 1
	    || (data->cfg.reg.quad_enable_write_len > 1 && !(data->cfg.reg.quad_enable_index == 1))
	    || data->cfg.reg.quad_enable_bit > 7
	) {
		flash_bflb_nxip_message_set(data, NXIP_MSG_BAD_QE, 0, 0, 0);
		return -EINVAL;
	}

	/* writable command also enables writing to configuration registers, not just data*/
	ret = flash_bflb_enable_writable(data);
	if (ret != 0) {
		return ret;
	}

	/* get quad enable register value */
	ret = flash_bflb_flash_read_register(data, data->cfg.reg.quad_enable_index,
		(uint8_t *)&tmp, data->cfg.reg.quad_enable_read_len);
	if (ret != 0) {
		return ret;
	}

	/* qe is already enable*/
	if ((*(uint8_t *)&tmp & BIT(data->cfg.reg.quad_enable_bit)) != 0) {
		return 0;
	}

	if (data->cfg.reg.quad_enable_write_len > 1) {
		/* We assume quad enable is in second register.
		 * Other configurations are unsupported and not yet observed.
		 */

		/* First register (status) */
		ret = flash_bflb_flash_read_register(data, 0, (uint8_t *)&tmp, 1);
		if (ret != 0) {
			return ret;
		}
		/* Second register (configuration) */
		ret = flash_bflb_flash_read_register(data, 1, &(((uint8_t *)&tmp)[1]), 1);
		if (ret != 0) {
			return ret;
		}

		((uint8_t *)&tmp)[1] |= BIT(data->cfg.reg.quad_enable_bit);

	/* we only need to read and write the appropriate register (usually the second one) */
	} else {
		tmp |= BIT(data->cfg.reg.quad_enable_bit);
	}

	ret = flash_bflb_flash_write_register(data, data->cfg.reg.quad_enable_index,
					      (uint8_t *)&tmp, data->cfg.reg.quad_enable_write_len);
	if (ret != 0) {
		return ret;
	}

	ret = flash_bflb_flash_read_register(data, data->cfg.reg.quad_enable_index, (uint8_t *)&tmp,
					     data->cfg.reg.quad_enable_read_len);
	if (ret != 0) {
		return ret;
	}

	/* check Quad is Enabled */
	if ((*(uint8_t *)&tmp & BIT(data->cfg.reg.quad_enable_bit)) != 0) {
		return 0;
	}

	return -EIO;
}

static __nxipfunc int flash_bflb_reset(struct flash_bflb_bank_data *data)
{
	struct bflb_flash_command reset = {0};
	int ret;

	reset.spi_mode = BUS_NIO;
	reset.cmd_buf[0] = data->cfg.cmd.reset_enable << 24;

	ret = flash_bflb_send_command(data, &reset);
	if (ret < 0) {
		return ret;
	}

	reset.cmd_buf[0] = data->cfg.cmd.reset << 24;
	ret = flash_bflb_send_command(data, &reset);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static __nxipfunc int flash_bflb_flash_enable_qpi(struct flash_bflb_bank_data *data)
{
	struct bflb_flash_command enable_qpi = {0};

	if (data->cfg.cmd.enter_qpi == 0) {
		return 0;
	}

	enable_qpi.spi_mode = data->cfg.manual_spi_mode;
	enable_qpi.cmd_buf[0] = (data->cfg.cmd.enter_qpi) << 24;

	return flash_bflb_send_command(data, &enable_qpi);
}

static __nxipfunc int flash_bflb_flash_disable_qpi(struct flash_bflb_bank_data *data)
{
	struct bflb_flash_command disable_qpi = {0};

	if (data->cfg.cmd.exit_qpi == 0) {
		return 0;
	}

	/* Reset QPI exit */
	if (data->cfg.cmd.exit_qpi == SPI_NOR_CMD_RESET_EN) {
		flash_bflb_reset(data);
		/* Shorter wait as flash is not supposed to be doing anything */
		for (int i = 0; i < BFLB_FLASH_CHIP_RESET_TIMEOUT / 4; i++) {
			flash_bflb_settle_x(BFLB_FLASH_1RMS);
		}
		return 0;
	}

	disable_qpi.spi_mode = BUS_QIO;
	disable_qpi.cmd_mode = 1;
	disable_qpi.cmd_buf[0] = (data->cfg.cmd.exit_qpi) << 24;

	return flash_bflb_send_command(data, &disable_qpi);
}

/* ID0 for CPU 0, ID1 for cpu 1 */
static __nxipfunc uint32_t flash_bflb_get_offset(struct flash_bflb_bank_data *data)
{
	uint32_t tmp;
#if defined(CONFIG_SOC_SERIES_BL60X)
	uintptr_t reg = SF_CTRL_SF_ID0_OFFSET_OFFSET;
#else
	uintptr_t reg = data->bank == BANK2 ?
		SF_CTRL_SF_BK2_ID0_OFFSET_OFFSET : SF_CTRL_SF_ID0_OFFSET_OFFSET;
#endif

	tmp = FLASH_READ32(data->reg + reg);
	tmp &= SF_CTRL_SF_ID0_OFFSET_MSK;
	tmp = tmp >> SF_CTRL_SF_ID0_OFFSET_POS;

	return tmp;
}

static __nxipfunc void flash_bflb_set_offset(struct flash_bflb_bank_data *data, uintptr_t offset)
{
	uint32_t tmp;
#if defined(CONFIG_SOC_SERIES_BL60X)
	uintptr_t reg = SF_CTRL_SF_ID0_OFFSET_OFFSET;
#else
	uintptr_t reg = data->bank == BANK2 ?
		SF_CTRL_SF_BK2_ID0_OFFSET_OFFSET : SF_CTRL_SF_ID0_OFFSET_OFFSET;
#endif
	tmp = FLASH_READ32(data->reg + reg);
	tmp &= ~SF_CTRL_SF_ID0_OFFSET_MSK;
	tmp |= offset << SF_CTRL_SF_ID0_OFFSET_POS;
	FLASH_WRITE32(tmp, data->reg + reg);
}

static __nxipfunc int flash_bflb_save_xip_state(const struct device *dev)
{
	struct flash_bflb_bank_data *data = dev->data;
	int ret;
	uint32_t tmp;

	flash_bflb_set_sahb(data);

	/* Ensure IF1 is enabled */
	tmp = FLASH_READ32(data->reg + SF_CTRL_1_OFFSET);
	tmp |= SF_CTRL_SF_AHB2SRAM_EN_MSK;
	tmp |= SF_CTRL_SF_IF_EN_MSK;
	FLASH_WRITE32(tmp, data->reg + SF_CTRL_1_OFFSET);

	/* Bus to system AHB, effectively immediately disables XIP access *for all* */
	ret = flash_bflb_set_bus(data, 0);
	if (ret != 0) {
		goto exit_here;
	}

	/* Disable QPI */
	if (data->cfg.use_qpi) {
		ret = flash_bflb_flash_disable_qpi(data);
		if (ret != 0) {
			goto exit_here;
		}
	}

	/* Disable continuous read */
	if (data->cfg.cmd.contread_on != 0) {
		ret = flash_bflb_flash_disable_continuous_read(data);
		if (ret != 0) {
			goto exit_here;
		}
	}

	/* Disable burst with wrap*/
	if (data->cfg.cmd.burstwrap != 0 && data->cfg.auto_spi_mode == BUS_QIO) {
		ret = flash_bflb_flash_set_burst(data, false);
		if (ret != 0) {
			goto exit_here;
		}
	}

	/* enable quad previous command could've disabled it */
	if (data->cfg.manual_spi_mode == BUS_QIO || data->cfg.manual_spi_mode == BUS_QO) {
		ret = flash_bflb_enable_qspi(data);
		if (ret != 0) {
			goto exit_here;
		}
	}

#if BFLB_HAS_32B
	/* Samely */
	if (data->controller->addr_32bits && data->cfg.cmd.enter_32bits_addr) {
		flash_bflb_enable_32baddr(data);
	}
#endif

exit_here:
	if (ret != 0) {
		LOG_ERR("Failed to save XIP state: %d", ret);
		flash_bflb_nxip_message_clear(data);
	}
	return ret;
}

static __nxipfunc bool flash_bflb_flash_busy_wait(struct flash_bflb_bank_data *data)
{
	uint8_t tmp_bus = 0xFF;
	uint32_t counter = 0;

	/* Cannot check if busy */
	if (data->cfg.reg.busy_read_len == 0) {
		return false;
	}

	while ((tmp_bus & BIT(data->cfg.reg.busy_bit)) != 0 && counter <
		BFLB_FLASH_CHIP_BUSY_TIMEOUT) {
		flash_bflb_flash_read_register(data, data->cfg.reg.busy_index, &tmp_bus,
					       data->cfg.reg.busy_read_len);
		flash_bflb_settle_x(BFLB_FLASH_1RMS);
		counter++;
	}


	if ((tmp_bus & BIT(data->cfg.reg.busy_bit)) != 0) {
		flash_bflb_nxip_message_set(data, NXIP_MSG_BUSY_FLASH, 0, 0, 0);
		return true;
	}

	return false;
}

static __nxipfunc int flash_bflb_xip_init(struct flash_bflb_bank_data *data)
{
	struct bflb_flash_command xip_cmd = {0};
	struct bflb_flash_command cont_read_init_cmd = {0};
	bool is_command = true;
	uint32_t buf;
	int ret;

	xip_cmd.spi_mode = data->cfg.auto_spi_mode;
	xip_cmd.cmd_buf[0] = data->cfg.cmd.auto_read << 24;
	xip_cmd.dummy_clks = data->cfg.cmd.auto_read_dmycy;
	/* IAHB reads 32 bytes at once */
	xip_cmd.nb_data = 32;

	if (data->cfg.use_qpi && data->cfg.cmd.enter_qpi != 0) {
		xip_cmd.cmd_mode = 1;
	}

	xip_cmd.addr_size = ADDR_SIZE(data);

	if (data->cfg.quirk_bytes_read_len > 0) {
		flash_bflb_xip_memcpy(
		&(((uint8_t *)&(xip_cmd.cmd_buf[1]))
		[xip_cmd.addr_size - BFLB_FLASH_ADDR_SIZE]),
			data->cfg.quirk_bytes_read, data->cfg.quirk_bytes_read_len);
		xip_cmd.addr_size += data->cfg.quirk_bytes_read_len;
	}

	if (data->cfg.auto_spi_mode == BUS_QIO && data->cfg.cmd.contread_on != 0
	    && data->cfg.quirk_bytes_read_len == 0 && !data->cfg.use_qpi) {
		is_command = false;
		xip_cmd.addr_size += BFLB_FLASH_ADDR_SIZE_CONTREAD_ADD;
		if (data->controller->addr_32bits && BFLB_HAS_32B) {
			xip_cmd.cmd_buf[0] = 0;
			xip_cmd.cmd_buf[1] = data->cfg.cmd.contread_on << 24;
		} else {
			xip_cmd.cmd_buf[0] = data->cfg.cmd.contread_on;
		}

		flash_bflb_xip_memcpy((uint8_t *)&xip_cmd,
				      (uint8_t *)&cont_read_init_cmd, sizeof(xip_cmd));
		/* Align */
		cont_read_init_cmd.nb_data = 4;
		cont_read_init_cmd.cmd_buf[0] = data->cfg.cmd.auto_read << 24;
		if (data->controller->addr_32bits && BFLB_HAS_32B) {
			cont_read_init_cmd.cmd_buf[1] = data->cfg.cmd.contread_on << 16;
		} else {
			cont_read_init_cmd.cmd_buf[1] = data->cfg.cmd.contread_on << 24;
		}

		ret = flash_bflb_set_bus(data, 0);
		if (ret != 0) {
			return ret;
		}

		ret = flash_bflb_send_command(data, &cont_read_init_cmd);
		if (ret != 0) {
			return ret;
		}

		if (flash_bflb_busy_wait(data)) {
			return -EBUSY;
		}

		flash_bflb_xip_memcpy((uint8_t *)SF_CTRL_BUF_BASE, (uint8_t *)(&buf), 4);
	}

	/* Bus to instruction AHB */
	ret = flash_bflb_set_bus(data, 1);
	if (ret != 0) {
		return ret;
	}

	return flash_bflb_set_command_iahb(data, &xip_cmd, is_command);
}

static __nxipfunc int flash_bflb_autowrite_init(struct flash_bflb_bank_data *data)
{
	struct bflb_flash_command autowrite_cmd = {0};
	int ret;

	autowrite_cmd.spi_mode = data->cfg.auto_spi_mode;
	autowrite_cmd.cmd_buf[0] = data->cfg.cmd.auto_write << 24;
	autowrite_cmd.dummy_clks = data->cfg.cmd.auto_write_dmycy;
	autowrite_cmd.rw = 1;
	/* IAHB writes 32 bytes at once */
	autowrite_cmd.nb_data = 32;

	if (data->cfg.use_qpi && data->cfg.cmd.enter_qpi != 0) {
		autowrite_cmd.cmd_mode = 1;
	}

	/* 3 for 24 bits, 4 for 32 bits */
	autowrite_cmd.addr_size = ADDR_SIZE(data);

	if (data->cfg.quirk_bytes_write_len > 0) {
		flash_bflb_xip_memcpy(
			&(((uint8_t *)&(autowrite_cmd.cmd_buf[1]))
			[autowrite_cmd.addr_size - BFLB_FLASH_ADDR_SIZE]),
			data->cfg.quirk_bytes_write, data->cfg.quirk_bytes_write_len);
		autowrite_cmd.addr_size += data->cfg.quirk_bytes_write_len;
	}

	/* Bus to instruction AHB */
	ret = flash_bflb_set_bus(data, 1);
	if (ret != 0) {
		return ret;
	}

	return flash_bflb_set_command_iahb_write(data, &autowrite_cmd, true);
}

static __nxipfunc int flash_bflb_restore_xip_state(struct flash_bflb_bank_data *data)
{
	int ret;

#if BFLB_HAS_32B
	if (data->controller->addr_32bits && data->cfg.cmd.enter_32bits_addr != 0) {
		flash_bflb_enable_32baddr(data);
	}
#endif

	/* Enable quad if relevant */
	if (data->cfg.auto_spi_mode == BUS_QIO || data->cfg.auto_spi_mode == BUS_QO) {
		ret = flash_bflb_enable_qspi(data);
		if (ret != 0) {
			goto exit_here;
		}
	}

	/* Reenable burst read */
	if (data->cfg.cmd.burstwrap != 0  && data->cfg.auto_spi_mode == BUS_QIO) {
		ret = flash_bflb_flash_set_burst(data, true);
		if (ret != 0) {
			goto exit_here;
		}
	}

	if (data->cfg.use_qpi) {
		ret = flash_bflb_flash_enable_qpi(data);
		if (ret != 0) {
			goto exit_here;
		}
	}

	ret = flash_bflb_xip_init(data);
	if (ret != 0) {
		goto exit_here;
	}

	if (data->bank == BANK2 && data->cfg.cmd.auto_write != 0) {
		ret = flash_bflb_autowrite_init(data);
		if (ret != 0) {
			goto exit_here;
		}
	}

exit_here:
	if (ret != 0) {
		/* Attempt to restore still functional XIP bank (Fine if it is bank 2 failing) */
		flash_bflb_set_bus(data, 1);
	}

	flash_bflb_release_sahb(data);

	flash_bflb_nxip_message_clear(data);

	return ret;
}

#if defined(CONFIG_SOC_FLASH_BFLB_DIRECT_ACCESS)

static __nxipfunc int flash_bflb_read_sahb_do(struct flash_bflb_bank_data *data, off_t address,
					      void *buffer, size_t length)
{
	int ret;
	struct bflb_flash_command read_cmd = {0};
	size_t i, cur_len;


	read_cmd.spi_mode = data->cfg.manual_spi_mode;
	read_cmd.dummy_clks = data->cfg.cmd.manual_read_dmycy;
	read_cmd.cmd_buf[0] = data->cfg.cmd.manual_read << 24;

	read_cmd.addr_size = ADDR_SIZE(data);

	i = 0;
	while (i < length) {

		cur_len = BFLB_FLASH_SF_BUF_SIZE - ((address + i) % BFLB_FLASH_SF_BUF_SIZE);

		if (cur_len > length - i) {
			cur_len = length - i;
		}

		read_cmd.cmd_buf[0] &= ~0xFFFFFF;

		if (data->controller->addr_32bits && BFLB_HAS_32B) {
			read_cmd.cmd_buf[0] |= (address + i) >> 8;
			read_cmd.cmd_buf[1] = (address + i) << 24;
		} else {
			read_cmd.cmd_buf[0] |= (address + i);
		}

		read_cmd.nb_data = cur_len;

		ret = flash_bflb_send_command(data, &read_cmd);
		if (ret != 0) {
			return ret;
		}

		flash_bflb_xip_memcpy((uint8_t *)SF_CTRL_BUF_BASE, (uint8_t *)(buffer) + i,
				      cur_len);

		i += cur_len;

		if (flash_bflb_busy_wait(data)) {
			return -EBUSY;
		}

		if (flash_bflb_flash_busy_wait(data)) {
			return -EBUSY;
		}
	}

	return 0;
}

/* copies flash data using direct access */
static __nxipfunc int flash_bflb_read(const struct device *dev, off_t address, void *buffer,
				      size_t length)
{
	struct flash_bflb_bank_data *data = dev->data;
	unsigned int	locker;
	int ret;

	if (length == 0) {
		return 0;
	}

	ret = flash_bflb_is_valid_range(data, address, length);
	if (ret != 0) {
		return ret;
	}

	if (flash_bflb_is_in_xip(data, &flash_bflb_read)) {
		return -ENOTSUP;
	}

	/* interrupting would break, likely to access XIP*/
	locker = irq_lock();

	ret = flash_bflb_save_xip_state(dev);
	if (ret != 0) {
		/* Attempt to restore */
		flash_bflb_restore_xip_state(data);
		irq_unlock(locker);
		return ret;
	}

	ret = flash_bflb_read_sahb_do(data, address, buffer, length);

	if (ret != 0) {
		flash_bflb_restore_xip_state(data);
	} else {
		ret = flash_bflb_restore_xip_state(data);
	}
	irq_unlock(locker);

	return ret;
}

#else

/* copies flash data using XIP access */
static __nxipfunc int flash_bflb_read(const struct device *dev, off_t address, void *buffer,
				      size_t length)
{
	struct flash_bflb_bank_data *data = dev->data;
	uint32_t	img_offset;
	unsigned int	locker;
	int ret;

	if (length == 0) {
		return 0;
	}

	ret = flash_bflb_is_valid_range(data, address, length);
	if (ret != 0) {
		return ret;
	}

	if (flash_bflb_is_in_xip(data, &flash_bflb_read)) {
		return -ENOTSUP;
	}

	/* interrupting would break, likely to access XIP */
	locker = irq_lock();

	/* get XIP offset / where code really is in flash, usually 0x2000 */
	img_offset = flash_bflb_get_offset(data);

	/* need set offset to 0 to access? */
	if (address < img_offset) {

		sys_cache_data_flush_and_invd_all();

		/* set offset to 0 to access first (likely)0x2000 of flash */
		flash_bflb_set_offset(data, 0);

		/* copy data we need */
		flash_bflb_xip_memcpy((uint8_t *)(address + data->xip_base),
				      (uint8_t *)buffer, length);

		sys_cache_data_flush_and_invd_all();

		flash_bflb_set_offset(data, img_offset);
	} else {
		/* copy data we need */
		flash_bflb_xip_memcpy((uint8_t *)(address + data->xip_base - img_offset),
				      (uint8_t *)buffer, length);
	}

	/* done with interrupt breaking stuffs */
	irq_unlock(locker);

	return 0;
}

#endif

static __nxipfunc int flash_bflb_write(const struct device *dev, off_t address, const void *buffer,
				       size_t length)
{
	struct flash_bflb_bank_data *data = dev->data;
	unsigned int	locker;
	int		ret, rete;
	uint32_t	cur_len, i;
	struct bflb_flash_command write_cmd = {0};
	uint32_t	img_offset;

	if (length == 0) {
		return 0;
	}

	ret = flash_bflb_is_valid_range(data, address, length);
	if (ret != 0) {
		return ret;
	}

	if (flash_bflb_is_in_xip(data, &flash_bflb_write)) {
		return -ENOTSUP;
	}

	/* No need for commands if we have automatic write */
	if (data->cfg.cmd.auto_write != 0) {
		/* get XIP offset / where code really is in flash, usually 0x0 when writable */
		img_offset = flash_bflb_get_offset(data);

		/* need set offset to 0 to access? */
		if (address < img_offset) {
			sys_cache_data_flush_and_invd_all();

			/* set offset to 0 to access first (likely)0x2000 of flash */
			flash_bflb_set_offset(data, 0);

			/* copy data we need */
			flash_bflb_xip_memcpy((uint8_t *)buffer,
					(uint8_t *)(address + data->xip_base), length);

			sys_cache_data_flush_and_invd_all();

			flash_bflb_set_offset(data, img_offset);
		} else {
			/* copy data we need */
			flash_bflb_xip_memcpy((uint8_t *)buffer,
					      (uint8_t *)(address + data->xip_base - img_offset),
					      length);
		}

		return 0;
	}

	/* interrupting would break, likely to access XIP*/
	locker = irq_lock();

	ret = flash_bflb_save_xip_state(dev);
	if (ret != 0) {
		/* Attempt to restore */
		flash_bflb_restore_xip_state(data);
		irq_unlock(locker);
		return ret;
	}

	write_cmd.spi_mode = BUS_NIO;
	write_cmd.cmd_buf[0] = data->cfg.cmd.page_program << 24;
	write_cmd.rw = 1;
	write_cmd.addr_size = ADDR_SIZE(data);

	i = 0;
	while (i < length) {
		/* Write enable is needed for every write */
		ret = flash_bflb_enable_writable(data);
		if (ret != 0) {
			goto exit_here;
		}

		/* Get current position within page size,
		 * this assumes page_size <= CTRL_BUF_SIZE
		 */
		cur_len = data->cfg.page_size - ((address + i) % data->cfg.page_size);

		if (cur_len > length - i) {
			cur_len = length - i;
		}

		flash_bflb_xip_memcpy((uint8_t *)(buffer) +  i, (uint8_t *)SF_CTRL_BUF_BASE,
				      cur_len);

		write_cmd.cmd_buf[0] &= ~0xFFFFFF;

		if (data->controller->addr_32bits && BFLB_HAS_32B) {
			write_cmd.cmd_buf[0] |= (address + i) >> 8;
			write_cmd.cmd_buf[1] = (address + i) << 24;
		} else {
			write_cmd.cmd_buf[0] |= (address + i);
		}

		write_cmd.nb_data = cur_len;
		ret = flash_bflb_send_command(data, &write_cmd);
		if (ret != 0) {
			goto exit_here;
		}

		i += cur_len;

		flash_bflb_busy_wait(data);
		if (flash_bflb_flash_busy_wait(data)) {
			ret = -EBUSY;
			goto exit_here;
		}
	}

exit_here:
	rete = flash_bflb_restore_xip_state(data);
	sys_cache_data_flush_and_invd_all();
	irq_unlock(locker);

	return (ret != 0 ? ret : rete);
}

static __nxipfunc int flash_bflb_erase(const struct device *dev, off_t start, size_t len)
{
	struct flash_bflb_bank_data *data = dev->data;
	unsigned int	locker;
	int		ret, rete;
	struct bflb_flash_command erase_cmd = {0};
	uint32_t erase_start = start / data->cfg.sector_size;
	uint32_t erase_end = (len / data->cfg.sector_size)
		+ start / data->cfg.sector_size;

	if (len == 0) {
		return 0;
	}

	/* No explicit erase needed if auto-writing */
	if (data->cfg.cmd.auto_write != 0) {
		return 0;
	}

	ret = flash_bflb_is_valid_range(data, start, len);
	if (ret != 0) {
		return ret;
	}

	if (flash_bflb_is_in_xip(data, &flash_bflb_erase)) {
		return -ENOTSUP;
	}

	if ((len % data->cfg.sector_size) != 0) {
		LOG_WRN("Length is not a multiple of minimal erase block size");
		return -EINVAL;
	}

	if ((start % data->cfg.sector_size) != 0) {
		LOG_WRN("Start address is not a multiple of minimal erase block size");
		return -EINVAL;
	}
	/* interrupting would break, likely to access XIP*/
	locker = irq_lock();

	ret = flash_bflb_save_xip_state(dev);
	if (ret != 0) {
		/* Attempt to restore */
		flash_bflb_restore_xip_state(data);
		irq_unlock(locker);
		return ret;
	}

	erase_cmd.spi_mode = BUS_NIO;

	erase_cmd.addr_size = ADDR_SIZE(data);

	for (uint32_t i = erase_start; i < erase_end; i++) {
		/* Write enable is needed for every write */
		ret = flash_bflb_enable_writable(data);
		if (ret != 0) {
			goto exit_here;
		}

		erase_cmd.cmd_buf[0] = data->cfg.cmd.sector_erase << 24;
		if (data->controller->addr_32bits && BFLB_HAS_32B) {
			erase_cmd.cmd_buf[0] |= (i * data->cfg.sector_size) >> 8;
			erase_cmd.cmd_buf[1] = (i * data->cfg.sector_size) << 24;
		} else {
			erase_cmd.cmd_buf[0] |= (i * data->cfg.sector_size);
		}

		ret = flash_bflb_send_command(data, &erase_cmd);
		if (ret != 0) {
			goto exit_here;
		}

		flash_bflb_busy_wait(data);
		if (flash_bflb_flash_busy_wait(data)) {
			ret = -EBUSY;
			goto exit_here;
		}
	}

exit_here:
	rete = flash_bflb_restore_xip_state(data);
	sys_cache_data_flush_and_invd_all();
	irq_unlock(locker);

	return (ret != 0 ? ret : rete);
}


#ifdef CONFIG_FLASH_PAGE_LAYOUT
void flash_bflb_page_layout(const struct device *dev,
			    const struct flash_pages_layout **layout,
			    size_t *layout_size)
{
	struct flash_bflb_bank_data *data = dev->data;

	data->cfg.layout.pages_size = data->cfg.sector_size;
	data->cfg.layout.pages_count = data->cfg.size / data->cfg.sector_size;

	*layout = &data->cfg.layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *flash_bflb_get_parameters(const struct device *dev)
{
	struct flash_bflb_bank_data *data = dev->data;

	return &data->cfg.parameters;
}

static __nxipfunc int flash_bflb_get_jedec_id_internal(struct flash_bflb_bank_data *data,
						       uint8_t *out)
{
	struct bflb_flash_command get_jedecid = {0};
	int ret;
	int offset = 0;
	uint32_t tmp[3];

	get_jedecid.spi_mode = BUS_NIO;
	get_jedecid.cmd_buf[0] = SPI_NOR_CMD_RDID << 24;
	get_jedecid.addr_size = 0;
	get_jedecid.nb_data = 3;

#define COND_BAD (out[0] == 0xff || out[1] == 0xff || out[0] == 0x0 || out[1] == 0x0)

	while (1) {
		ret = flash_bflb_send_command(data, &get_jedecid);
		if (ret < 0) {
			return ret;
		}

		tmp[0] = FLASH_READ32(SF_CTRL_BUF_BASE);
		tmp[1] = FLASH_READ32(SF_CTRL_BUF_BASE);
		tmp[2] = FLASH_READ32(SF_CTRL_BUF_BASE);

		flash_bflb_xip_memcpy(&(((uint8_t *)tmp)[offset]), out, 3);

		/* Some devices want bits of address first
		 * 0xff is not a possible manufacturer ID
		 */
		if (COND_BAD && get_jedecid.addr_size == 0) {
			get_jedecid.addr_size = BFLB_FLASH_ADDR_SIZE;
		} else if (COND_BAD) {
			break;
		} else if (out[offset] == 0x7f) {
			offset++;
			get_jedecid.nb_data++;
		} else {
			break;
		}
	}

	if (COND_BAD) {
		return -EIO;
	}

#undef COND_BAD

	return 0;
}

#ifdef CONFIG_SOC_FLASH_BFLB_SFDP

int __nxipfunc flash_bflb_read_sfdp_internal(struct flash_bflb_bank_data *data, off_t offset,
				  uint8_t *o_data, size_t len)
{
	struct bflb_flash_command read_sfdp = {0};
	int ret;
	size_t read = 0;
	size_t cur_len;


	read_sfdp.spi_mode = BUS_NIO;
	read_sfdp.cmd_buf[0] = JESD216_CMD_READ_SFDP << 24;
	read_sfdp.addr_size = BFLB_FLASH_ADDR_SIZE;
	read_sfdp.dummy_clks = 1;

	while (read < len) {

		cur_len = BFLB_FLASH_SF_BUF_SIZE - ((offset + read) % BFLB_FLASH_SF_BUF_SIZE);

		if (cur_len > len - read) {
			cur_len = len - read;
		}

		read_sfdp.cmd_buf[0] &= ~0xFFFFFF;
		read_sfdp.nb_data = cur_len;
		read_sfdp.cmd_buf[0] |= (offset + read);

		ret = flash_bflb_send_command(data, &read_sfdp);
		if (ret < 0) {
			return ret;
		}

		flash_bflb_xip_memcpy((uint8_t *)SF_CTRL_BUF_BASE, o_data + read, cur_len);

		read += cur_len;

		if (flash_bflb_busy_wait(data)) {
			return -EBUSY;
		}

		if (flash_bflb_flash_busy_wait(data)) {
			return -EBUSY;
		}
	}

	return 0;
}

#if defined(CONFIG_FLASH_JESD216_API)

static __nxipfunc int flash_bflb_get_jedec_id(const struct device *dev, uint8_t *id)
{
	struct flash_bflb_bank_data *data = dev->data;
	unsigned int locker;
	int ret;

	if (flash_bflb_is_in_xip(data, &flash_bflb_get_jedec_id)) {
		return -ENOTSUP;
	}

	locker = irq_lock();

	ret = flash_bflb_save_xip_state(dev);
	if (ret != 0) {
		/* Attempt to restore */
		flash_bflb_restore_xip_state(data);
		irq_unlock(locker);
		return ret;
	}

	ret = flash_bflb_get_jedec_id_internal(data, id);

	flash_bflb_restore_xip_state(data);
	irq_unlock(locker);

	return ret;
}

static __nxipfunc int flash_bflb_read_sfdp(const struct device *dev, off_t offset, void *o_data,
					   size_t len)
{
	struct flash_bflb_bank_data *data = dev->data;
	unsigned int locker;
	int ret;

	if (len == 0) {
		return 0;
	}

	if ((offset + len) > 0xFFFFFF) {
		return -EINVAL;
	}

	if (flash_bflb_is_in_xip(data, &flash_bflb_read_sfdp)) {
		return -ENOTSUP;
	}

	locker = irq_lock();

	ret = flash_bflb_save_xip_state(dev);
	if (ret != 0) {
		/* Attempt to restore */
		flash_bflb_restore_xip_state(data);
		irq_unlock(locker);
		return ret;
	}

	ret = flash_bflb_read_sfdp_internal(data, offset, o_data, len);

	flash_bflb_restore_xip_state(data);
	irq_unlock(locker);

	return ret;
}

#endif

#endif /* CONFIG_SOC_FLASH_BFLB_SFDP */

#if !DT_ANY_INST_HAS_BOOL_STATUS_OKAY(no_header)

/* from SDK because there is no matching zephyr crc for the bflb flash crc, this is supposedly a
 * implementation of ZIP crc32
 */
static uint32_t bflb_soft_crc32(uint32_t initial, void *in, uint32_t len)
{
	uint32_t crc = ~initial;
	uint8_t *data = (uint8_t *)in;

	while (len--) {
		crc ^= *data++;
		for (uint8_t i = 0; i < 8; ++i) {
			if (crc & 1) {
				/* 0xEDB88320 = reverse 0x04C11DB7 */
				crc = (crc >> 1) ^ 0xEDB88320U;
			} else {
				crc = (crc >> 1);
			}
		}
	}

	return ~crc;
}

static __nxipfunc int flash_bflb_header_fetch(struct flash_bflb_bank_data *data,
					      struct bflb_header_flash_cfg *flash_header_cfg)
{
	uint32_t tmp;
	uint32_t img_offset;
	unsigned int locker;
	struct bflb_flash_header header;

	if (flash_bflb_is_in_xip(data, &flash_bflb_header_fetch)) {
		return -ENOTSUP;
	}

	/* get flash config using xip access */

	/* interrupting would break, likely to access XIP*/
	locker = irq_lock();
	/* get XIP offset / where code really is in flash, usually 0x2000 */
	img_offset = flash_bflb_get_offset(data);

	sys_cache_data_flush_and_invd_all();

	/* set offset to 0 to access first (likely)0x2000 of flash */
	flash_bflb_set_offset(data, 0);

	/* copy data we need */
	flash_bflb_xip_memcpy((uint8_t *)(data->xip_base),
			      (uint8_t *)&header, sizeof(struct bflb_flash_header));

	sys_cache_data_flush_and_invd_all();

	flash_bflb_set_offset(data, img_offset);

	/* done with interrupt breaking stuffs */
	irq_unlock(locker);

	/* magic */
	if (!(header.magic_2.magic[0] == BFLB_FLASH_MAGIC_2[0]
		&& header.magic_2.magic[1] == BFLB_FLASH_MAGIC_2[1]
		&& header.magic_2.magic[2] == BFLB_FLASH_MAGIC_2[2]
		&& header.magic_2.magic[3] == BFLB_FLASH_MAGIC_2[3])) {
		LOG_ERR("Flash data magic is incorrect");
		return -EINVAL;
	}

	tmp = bflb_soft_crc32(0, (uint8_t *)(&header.flash_cfg),
			      sizeof(struct bflb_header_flash_cfg));
	if (tmp != header.flash_cfg_crc) {
		LOG_ERR("Flash data crc is incorrect %d vs %d", tmp, header.flash_cfg_crc);
		return -EINVAL;
	}
	flash_bflb_xip_memcpy((uint8_t *)&(header.flash_cfg),
			      (uint8_t *)flash_header_cfg,
			      sizeof(struct bflb_header_flash_cfg));

	return 0;
}

/* The boot bank (bank1) has its settings provided to the bootrom via the boot header */
static int flash_bflb_init_bootbank(struct flash_bflb_bank_data *data)
{
	struct bflb_header_flash_cfg flash_header_cfg;
	int ret;

	ret = flash_bflb_header_fetch(data, &flash_header_cfg);
	if (ret < 0) {
		return ret;
	}

	if (!data->controller->override_bank1) {
		flash_bflb_set_default_read_header(data, &flash_header_cfg);
		data->cfg.page_size = flash_header_cfg.page_size;
		data->cfg.sector_size = flash_header_cfg.sector_size * 1024;
		data->cfg.cmd.enter_qpi = flash_header_cfg.enter_qpi;
		data->cfg.cmd.exit_qpi = flash_header_cfg.exit_qpi;
	}
	memcpy(data->cfg.cmd.read_reg, flash_header_cfg.read_reg_cmd, sizeof(uint8_t) * 4);
	memcpy(data->cfg.cmd.write_reg, flash_header_cfg.write_reg_cmd, sizeof(uint8_t) * 4);
	/* Continuous read supported */
	if ((flash_header_cfg.c_read_support & 0x1) != 0
	    /* Continuous read enabled */
	    && (flash_header_cfg.c_read_support & 0x2) == 0) {
		if (data->cfg.cmd.contread_on == 0) {
			data->cfg.cmd.contread_on = flash_header_cfg.c_read_mode;
		}
		data->cfg.cmd.contread_off = flash_header_cfg.c_rexit;
	}
	data->cfg.cmd.burstwrap = flash_header_cfg.burst_wrap_cmd;
	data->cfg.cmd.burstwrap_dmycy = flash_header_cfg.burst_wrap_cmd_dmy_clk;
	data->cfg.cmd.burstwrap_on_data = flash_header_cfg.burst_wrap_data;
	data->cfg.cmd.burstwrap_off_data = flash_header_cfg.de_burst_wrap_data;
	data->cfg.cmd.write_enable = flash_header_cfg.write_enable_cmd;
	data->cfg.cmd.page_program = flash_header_cfg.page_program_cmd;
	data->cfg.cmd.sector_erase = flash_header_cfg.sector_erase_cmd;
	data->cfg.cmd.block_erase = flash_header_cfg.blk32_erase_cmd;
#if BFLB_HAS_32B
	data->cfg.cmd.enter_32bits_addr = flash_header_cfg.enter_32bits_addr_cmd;
	data->cfg.cmd.exit_32bits_addr = flash_header_cfg.exit_32bits_addr_cmd;
#endif
	data->cfg.cmd.reset_enable = flash_header_cfg.reset_en_cmd;
	data->cfg.cmd.reset = flash_header_cfg.reset_cmd;
	data->cfg.cmd.release_powerdown = flash_header_cfg.release_powerdown;

	data->cfg.reg.write_enable_index = flash_header_cfg.wr_enable_index;
	data->cfg.reg.write_enable_bit = flash_header_cfg.wr_enable_bit;
	data->cfg.reg.write_enable_read_len = flash_header_cfg.wr_enable_read_reg_len;

	data->cfg.reg.quad_enable_index = flash_header_cfg.qe_index;
	data->cfg.reg.quad_enable_bit = flash_header_cfg.qe_bit;
	data->cfg.reg.quad_enable_read_len = flash_header_cfg.qe_read_reg_len;
	data->cfg.reg.quad_enable_write_len = flash_header_cfg.qe_write_reg_len;

	data->cfg.reg.busy_index = flash_header_cfg.busy_index;
	data->cfg.reg.busy_bit = flash_header_cfg.busy_bit;
	data->cfg.reg.busy_read_len = flash_header_cfg.busy_read_reg_len;

	return 0;
}

#endif /* !DT_ANY_INST_HAS_BOOL_STATUS_OKAY(no_header) */

/* The only delays used in the SDK are 'di' and 'cs'.
 * possibly each bit sets 3ns, or the value of 3 does, which is the maximum.
 * It used for final fine tuning.
 */
static __nxipfunc void flash_bflb_set_io_delays(struct flash_bflb_bank_data *data,
						uint8_t dod, uint8_t did, uint8_t oed,
						uint8_t csd, uint8_t clkd)
{
	uint32_t tmp = 0;
	uint32_t offset = 0;

	dod &= 0x3;
	did &= 0x3;
	oed &= 0x3;
	csd &= 0x3;
	clkd &= 0x3;

	if (data->cfg.pad.id == PAD1) {
		offset = data->reg + SF_CTRL_SF_IF_IO_DLY_0_OFFSET;
	} else if (data->cfg.pad.id == PAD2) {
		offset = data->reg + SF_CTRL_SF2_IF_IO_DLY_0_OFFSET;
	} else {
		offset = data->reg + SF_CTRL_SF3_IF_IO_DLY_0_OFFSET;
	}

	/* Set cs and clk delay */
	tmp = FLASH_READ32(offset);
	tmp &= ~SF_CTRL_SF_CS_DLY_SEL_MSK;
	tmp |= (uint32_t)csd << SF_CTRL_SF_CS_DLY_SEL_POS;
#if !defined(CONFIG_SOC_SERIES_BL60X)
	tmp &= ~SF_CTRL_SF_CS2_DLY_SEL_MSK;
	tmp |= (uint32_t)csd << SF_CTRL_SF_CS2_DLY_SEL_POS;
#endif
	tmp &= ~SF_CTRL_SF_CLK_OUT_DLY_SEL_MSK;
	tmp |= (uint32_t)clkd << SF_CTRL_SF_CLK_OUT_DLY_SEL_POS;
	FLASH_WRITE32(tmp, offset);

	/* Set do di and oe delay */
	tmp = FLASH_READ32(offset + 0x4);
	tmp &= ~SF_CTRL_SF_IO_0_DO_DLY_SEL_MSK;
	tmp |= (uint32_t)dod << SF_CTRL_SF_IO_0_DO_DLY_SEL_POS;
	tmp &= ~SF_CTRL_SF_IO_0_DI_DLY_SEL_MSK;
	tmp |= (uint32_t)did << SF_CTRL_SF_IO_0_DI_DLY_SEL_POS;
	tmp &= ~SF_CTRL_SF_IO_0_OE_DLY_SEL_MSK;
	tmp |= (uint32_t)oed << SF_CTRL_SF_IO_0_OE_DLY_SEL_POS;
	FLASH_WRITE32(tmp, offset + 0x4);

	tmp = FLASH_READ32(offset + 0x8);
	tmp &= ~SF_CTRL_SF_IO_0_DO_DLY_SEL_MSK;
	tmp |= (uint32_t)dod << SF_CTRL_SF_IO_0_DO_DLY_SEL_POS;
	tmp &= ~SF_CTRL_SF_IO_0_DI_DLY_SEL_MSK;
	tmp |= (uint32_t)did << SF_CTRL_SF_IO_0_DI_DLY_SEL_POS;
	tmp &= ~SF_CTRL_SF_IO_0_OE_DLY_SEL_MSK;
	tmp |= (uint32_t)oed << SF_CTRL_SF_IO_0_OE_DLY_SEL_POS;
	FLASH_WRITE32(tmp, offset + 0x8);

	tmp = FLASH_READ32(offset + 0xc);
	tmp &= ~SF_CTRL_SF_IO_0_DO_DLY_SEL_MSK;
	tmp |= (uint32_t)dod << SF_CTRL_SF_IO_0_DO_DLY_SEL_POS;
	tmp &= ~SF_CTRL_SF_IO_0_DI_DLY_SEL_MSK;
	tmp |= (uint32_t)did << SF_CTRL_SF_IO_0_DI_DLY_SEL_POS;
	tmp &= ~SF_CTRL_SF_IO_0_OE_DLY_SEL_MSK;
	tmp |= (uint32_t)oed << SF_CTRL_SF_IO_0_OE_DLY_SEL_POS;
	FLASH_WRITE32(tmp, offset + 0xc);

	tmp = FLASH_READ32(offset + 0x10);
	tmp &= ~SF_CTRL_SF_IO_0_DO_DLY_SEL_MSK;
	tmp |= (uint32_t)dod << SF_CTRL_SF_IO_0_DO_DLY_SEL_POS;
	tmp &= ~SF_CTRL_SF_IO_0_DI_DLY_SEL_MSK;
	tmp |= (uint32_t)did << SF_CTRL_SF_IO_0_DI_DLY_SEL_POS;
	tmp &= ~SF_CTRL_SF_IO_0_OE_DLY_SEL_MSK;
	tmp |= (uint32_t)oed << SF_CTRL_SF_IO_0_OE_DLY_SEL_POS;
	FLASH_WRITE32(tmp, offset + 0x10);
}


#if defined(CONFIG_SOC_SERIES_BL61X)

/* Set buffer mode and alignment ?
 * len in bytes
 * 0: 8
 * 1: 16
 * 2: 32
 * 3: 64
 * 4: 128
 * 5: 256
 * 6: 512
 * 7: 1024
 * 8: 2048
 * 9: 4096
 * mode:
 * 0: `bypass wrap commands to macro, original mode`
 * 1: `handle wrap commands, original mode`
 * 2: `bypass wrap commands to macro, cmds force wrap16*4 split into two wrap8*4`
 * 3: `handle wrap commands, cmds force wrap16*4 split into two wrap8*4`
 *
 */
static __nxipfunc void flash_bflb_set_cmds(struct flash_bflb_bank_data *data, uint8_t mode,
					   uint8_t len)
{
	uint32_t tmp;

	tmp = FLASH_READ32(data->reg + SF_CTRL_3_OFFSET);
	tmp |= SF_CTRL_SF_CMDS_CORE_EN_MSK;
	tmp |= SF_CTRL_SF_CMDS_2_EN_MSK;
	tmp |= SF_CTRL_SF_CMDS_1_EN_MSK;
	if (data->bank == BANK2) {
		tmp &= ~SF_CTRL_SF_CMDS_2_WRAP_MODE_MSK;
		tmp &= ~SF_CTRL_SF_CMDS_2_WRAP_LEN_MSK;
		tmp &= ~SF_CTRL_SF_CMDS_2_BT_EN_MSK;
		tmp &= ~SF_CTRL_SF_CMDS_2_BT_DLY_MSK;
		tmp |= (uint32_t)mode << SF_CTRL_SF_CMDS_2_WRAP_MODE_POS;
		tmp |= (uint32_t)len << SF_CTRL_SF_CMDS_2_WRAP_LEN_POS;
	} else {
		tmp &= ~SF_CTRL_SF_CMDS_1_WRAP_MODE_MSK;
		tmp &= ~SF_CTRL_SF_CMDS_1_WRAP_LEN_MSK;
		tmp |= (uint32_t)mode << SF_CTRL_SF_CMDS_1_WRAP_MODE_POS;
		tmp |= (uint32_t)len << SF_CTRL_SF_CMDS_1_WRAP_LEN_POS;
	}
	FLASH_WRITE32(tmp, data->reg + SF_CTRL_3_OFFSET);
}

#endif

#ifdef CONFIG_SOC_FLASH_BFLB_SFDP

static __nxipfunc int flash_bflb_discovery(struct flash_bflb_bank_data *data, uint8_t jedec_id[3])
{
	struct jesd216_sfdp_header sfdp_header;
	struct jesd216_param_header cur_header;
	struct jesd216_param_header bfp_header;
	struct jesd216_param_header vendor_header;
	struct jesd216_param_header qio_header;
	bool got_bfp = false;
	bool got_qio = false;
	bool got_vendor = false;
	int ret;
	uint32_t tmp;
	uint32_t i_ph = 0;

	ret = flash_bflb_read_sfdp_internal(data, 0, (void *)&(sfdp_header),
					    sizeof(struct jesd216_sfdp_header));
	if (ret != 0) {
		return ret;
	}

	tmp = jesd216_sfdp_magic(&sfdp_header);

	if (tmp != JESD216_SFDP_MAGIC) {
		flash_bflb_nxip_message_set(data, NXIP_MSG_BAD_SFDP, 1, tmp, 0);
		return -EINVAL;
	}

	/* Set these jedec defaults since this indeed a flash */
	data->cfg.reg.write_enable_index = 0;
	data->cfg.reg.write_enable_bit = 1;
	data->cfg.reg.write_enable_read_len = 1;

	while (i_ph < sfdp_header.nph) {

		ret = flash_bflb_read_sfdp_internal(data, sizeof(struct jesd216_sfdp_header)
						    + i_ph * sizeof(struct jesd216_param_header),
						    (void *)&(cur_header),
						    sizeof(struct jesd216_param_header));
		if (ret != 0) {
			flash_bflb_nxip_message_set(data, NXIP_MSG_BAD_SFDP, 2, i_ph, 0);
			return ret;
		}

		tmp = jesd216_param_id(&cur_header);

		if (tmp == JESD216_SFDP_PARAM_ID_BFP) {
			got_bfp = true;
			flash_bflb_xip_memcpy((uint8_t *)&cur_header, (uint8_t *)&bfp_header,
					      sizeof(struct jesd216_param_header));
		} else if (tmp == JESD216_SFDP_PARAM_ID_4B_ADDR_INSTR) {
			got_qio = true;
			flash_bflb_xip_memcpy((uint8_t *)&cur_header, (uint8_t *)&qio_header,
					      sizeof(struct jesd216_param_header));
		} else if (tmp == jedec_id[0]) {
			got_vendor = true;
			flash_bflb_xip_memcpy((uint8_t *)&cur_header, (uint8_t *)&vendor_header,
					      sizeof(struct jesd216_param_header));
		}

		i_ph++;
	}

	if (got_bfp) {
		union {
			uint32_t dw[20];
			struct jesd216_bfp bfp;
		} bfp_u;
		const struct jesd216_bfp *bfp = &bfp_u.bfp;
		struct jesd216_bfp_dw14 dw14;
		struct jesd216_bfp_dw15 dw15;
		struct jesd216_instr instr;
		uint8_t dummy_div = 2;
		bool read_ok = false;

		ret = flash_bflb_read_sfdp_internal(data, jesd216_param_addr(&bfp_header),
						    (void *)bfp_u.dw,
						    MIN(sizeof(uint32_t) * bfp_header.len_dw,
							sizeof(bfp_u.dw)));
		if (ret != 0) {
			flash_bflb_nxip_message_set(data, NXIP_MSG_BAD_SFDP, 3, 0, 0);
			return ret;
		}

		if (jesd216_bfp_density(bfp) / 8U != data->cfg.size) {
			flash_bflb_nxip_message_set(data, NXIP_MSG_SFDP_BADSIZE,
						    jesd216_bfp_density(bfp) / 8U,
						    data->cfg.size, 0);
		}
		data->cfg.size = jesd216_bfp_density(bfp) / 8U;

		/* Pick best supported fast read */
		do {
			switch (data->cfg.auto_spi_mode) {
			default:
			case BUS_NIO:
				dummy_div = 8;
				read_ok = true;
				tmp = JESD216_MODE_111;
			break;
			case BUS_DO:
				if ((bfp_u.dw[0] & BIT(16)) == 0) {
					data->cfg.auto_spi_mode =
						flash_bflb_pick_next_worse_xip(
							data->cfg.auto_spi_mode);
					flash_bflb_nxip_message_set(data, NXIP_MSG_SADSUP_SFDP, 1,
								    BIT(16), 0);
				} else {
					dummy_div = 8;
					tmp = JESD216_MODE_112;
					read_ok = true;
				}
			break;
			case BUS_DIO:
				if ((bfp_u.dw[0] & BIT(20)) == 0) {
					data->cfg.auto_spi_mode =
						flash_bflb_pick_next_worse_xip(
							data->cfg.auto_spi_mode);
					flash_bflb_nxip_message_set(data, NXIP_MSG_SADSUP_SFDP, 1,
								    BIT(20), 0);
				} else {
					dummy_div = 4;
					tmp = JESD216_MODE_122;
					read_ok = true;
				}
			break;
			case BUS_QO:
				if ((bfp_u.dw[0] & BIT(22)) == 0) {
					data->cfg.auto_spi_mode =
						flash_bflb_pick_next_worse_xip(
							data->cfg.auto_spi_mode);
					flash_bflb_nxip_message_set(data, NXIP_MSG_SADSUP_SFDP, 1,
								    BIT(22), 0);
				} else {
					dummy_div = 8;
					tmp = JESD216_MODE_114;
					read_ok = true;
				}
			break;
			case BUS_QIO:
				if (data->cfg.use_qpi) {
					if ((bfp_u.dw[4] & BIT(4)) == 0) {
						data->cfg.use_qpi = false;
						flash_bflb_nxip_message_set(data,
									    NXIP_MSG_SADSUP_SFDP, 5,
									    BIT(4), 0);
					} else {
						dummy_div = 2;
						tmp = JESD216_MODE_444;
						read_ok = true;
					}
				} else {
					if ((bfp_u.dw[0] & BIT(21)) == 0) {
						data->cfg.auto_spi_mode =
							flash_bflb_pick_next_worse_xip(
								data->cfg.auto_spi_mode);
						flash_bflb_nxip_message_set(data,
									    NXIP_MSG_SADSUP_SFDP, 1,
									    BIT(21), 0);
					} else {
						dummy_div = 2;
						tmp = JESD216_MODE_144;
						read_ok = true;
					}
				}
			break;
			}
		} while (!read_ok);

		ret = jesd216_bfp_read_support(&bfp_header, bfp, tmp, &instr);
		if (ret < 0) {
			flash_bflb_nxip_message_set(data, NXIP_MSG_BAD_SFDP, 2, tmp, 0);
			return ret;
		}
		if (ret == 0) {
			if (data->cfg.cmd.auto_read == 0) {
				flash_bflb_set_default_read_default(data);
			}
		} else {
			ret = 0;
			if (data->cfg.cmd.auto_read == 0) {
				data->cfg.cmd.auto_read = instr.instr;
			}
			/* Dummy cycles come in package of 8 */
			if (data->cfg.cmd.auto_read_dmycy == 0) {
				data->cfg.cmd.auto_read_dmycy = instr.wait_states / dummy_div;

				if (instr.wait_states % dummy_div != 0) {
					flash_bflb_nxip_message_set(data, NXIP_MSG_SADSUP_SFDP,
								    3, 0xFF, 0);
				}
			}
		}

		/* Cannot support non-uniform erase */
		if ((bfp_u.dw[0] & GENMASK(1, 0)) == GENMASK(1, 0)) {
			flash_bflb_nxip_message_set(data, NXIP_MSG_SADSUP_SFDP, 1, 0x7, 0);
		} else {
			data->cfg.cmd.sector_erase =
				(bfp_u.dw[0] & JESD216_SFDP_BFP_DW1_4KERASEINSTR_MASK)
				>> JESD216_SFDP_BFP_DW1_4KERASEINSTR_SHFT;
		}

		data->cfg.page_size = jesd216_bfp_page_size(&bfp_header, bfp);

		/* Busy bit infos */
		ret = jesd216_bfp_decode_dw14(&bfp_header, bfp, &dw14);
		if (ret == 0) {
			if (dw14.poll_options & BIT(1)) {
				data->cfg.reg.busy_index = 3;
				data->cfg.cmd.read_reg[3] = SPI_NOR_CMD_RDFLSR;
				data->cfg.reg.busy_bit = 7;
			} else {
				data->cfg.reg.busy_bit = 0;
				data->cfg.reg.busy_read_len = 1;
				data->cfg.reg.busy_index = 0;
			}
		} else {
			flash_bflb_nxip_message_set(data, NXIP_MSG_SAD_SFDP,
						    JESD216_SFDP_PARAM_ID_BFP, 14, 0);
		}

		ret = jesd216_bfp_decode_dw15(&bfp_header, bfp, &dw15);
		if (ret == 0) {
			switch (dw15.qer) {
			case JESD216_DW15_QER_S2B1v5:
			case JESD216_DW15_QER_S2B1v4:
			case JESD216_DW15_QER_S2B1v1:
				data->cfg.reg.quad_enable_bit = 1;
				data->cfg.reg.quad_enable_index = 1;
				data->cfg.reg.quad_enable_write_len = 2;
				data->cfg.reg.quad_enable_read_len = 1;
			break;
			case JESD216_DW15_QER_S1B6:
				data->cfg.reg.quad_enable_bit = 6;
				data->cfg.reg.quad_enable_index = 0;
				data->cfg.reg.quad_enable_write_len = 1;
				data->cfg.reg.quad_enable_read_len = 1;
			break;
			case JESD216_DW15_QER_S2B1v6:
				data->cfg.reg.quad_enable_bit = 1;
				data->cfg.reg.quad_enable_index = 1;
				data->cfg.reg.quad_enable_write_len = 1;
				data->cfg.reg.quad_enable_read_len = 1;
			break;
			default:
			break;
			}
		} else {
			ret = 0;
			flash_bflb_nxip_message_set(data, NXIP_MSG_SAD_SFDP,
						    JESD216_SFDP_PARAM_ID_BFP, 15, 0);
		}

		if (dw15.support_044 && data->cfg.cmd.contread_on == 0) {
			if ((dw15.entry_044 & BIT(0)) != 0 || (dw15.entry_044 & BIT(2)) != 0) {
				data->cfg.cmd.contread_on = 0xA5;
			} else {
				flash_bflb_nxip_message_set(data, NXIP_MSG_SAD_SFDP,
						    JESD216_SFDP_PARAM_ID_BFP, 1544, 0);
				data->cfg.cmd.contread_on = 0;
			}
		} else {
			data->cfg.cmd.contread_on = 0;
		}

		if (data->cfg.use_qpi && data->cfg.cmd.enter_qpi == 0) {
			if ((dw15.enable_444 & GENMASK(1, 0)) != 0) {
				data->cfg.cmd.enter_qpi = 0x38;
			} else if ((dw15.enable_444 & BIT(2)) != 0) {
				data->cfg.cmd.enter_qpi = 0x35;
			} else {
				flash_bflb_nxip_message_set(data, NXIP_MSG_SAD_SFDP,
						    JESD216_SFDP_PARAM_ID_BFP, 154440, 0);
			}
			if ((dw15.disable_444 & BIT(0)) != 0) {
				data->cfg.cmd.exit_qpi = 0xff;
			} else if ((dw15.disable_444 & BIT(1)) != 0) {
				data->cfg.cmd.exit_qpi = 0xf5;
			} else if ((dw15.disable_444 & BIT(2)) != 0) {
				data->cfg.cmd.exit_qpi = SPI_NOR_CMD_RESET_EN;
			} else {
				flash_bflb_nxip_message_set(data, NXIP_MSG_SAD_SFDP,
						    JESD216_SFDP_PARAM_ID_BFP, 154441, 0);
				data->cfg.cmd.enter_qpi = 0;
			}
		}

		tmp = jesd216_bfp_addrbytes(bfp);
		if (tmp == JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B4B
		    || tmp == JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_4B) {
			struct jesd216_bfp_dw16 dw16;

			ret = jesd216_bfp_decode_dw16(&bfp_header, bfp, &dw16);
			if (ret != 0) {
				ret = 0;
				data->controller->addr_32bits = false;
				data->cfg.cmd.enter_32bits_addr = 0;
				flash_bflb_nxip_message_set(data, NXIP_MSG_SAD_SFDP,
						    JESD216_SFDP_PARAM_ID_BFP, 16, 0);
			} else {
				if ((dw16.enter_4ba & GENMASK(1, 0)) != 0) {
					data->cfg.cmd.enter_32bits_addr = SPI_NOR_CMD_4BA;
				} else {
					data->cfg.cmd.enter_32bits_addr = 0;
				}
				if ((dw16.exit_4ba & GENMASK(1, 0)) != 0) {
					data->cfg.cmd.exit_32bits_addr = 0xE9;
				} else {
					data->cfg.cmd.enter_32bits_addr = 0;
				}

				if ((dw16.sr1_interface & GENMASK(1, 0)) != 0
				    || (dw16.sr1_interface & GENMASK(4, 3)) != 0) {
					data->cfg.cmd.write_enable = SPI_NOR_CMD_WREN;
				} else if ((dw16.sr1_interface & BIT(2)) != 0) {
					data->cfg.cmd.write_enable = SPI_NOR_CMD_CLRFLSR;
				}
			}
		} else {
			data->controller->addr_32bits = false;
			data->cfg.cmd.enter_32bits_addr = 0;
		}

	} else {
		flash_bflb_nxip_message_set(data, NXIP_MSG_SAD_SFDP, 1,
					    JESD216_SFDP_PARAM_ID_BFP, 0);
	}

	return ret;
}

#endif /* CONFIG_SOC_FLASH_BFLB_SFDP */

static __nxipfunc void flash_bflb_configure_timings(struct flash_bflb_bank_data *data)
{
	uint32_t tmp;

	tmp = FLASH_READ32(GLB_BASE + BFLB_SF_CLK_REG_OFF);
	tmp &= GLB_SF_CLK_DIV_UMSK;
	tmp &= GLB_SF_CLK_EN_UMSK;
	tmp |= (data->controller->clk_divider - 1) << GLB_SF_CLK_DIV_POS;
	FLASH_WRITE32(tmp, GLB_BASE + BFLB_SF_CLK_REG_OFF);

	/* Reset IO delays */
	flash_bflb_set_io_delays(data, data->cfg.pad.dod, data->cfg.pad.did,
				 data->cfg.pad.oed, data->cfg.pad.csd, data->cfg.pad.clkd);

	/* Set IAHB delay */
	if (data->cfg.pad.id == PAD1) {
		tmp = FLASH_READ32(data->reg + SF_CTRL_0_OFFSET);
		if (data->cfg.pad.read_delay > 0) {
			tmp |= SF_CTRL_SF_IF_READ_DLY_EN_MSK;
			tmp &= ~SF_CTRL_SF_IF_READ_DLY_N_MSK;
			tmp |= (data->cfg.pad.read_delay - 1U) << SF_CTRL_SF_IF_READ_DLY_N_POS;
		} else {
			tmp &= ~SF_CTRL_SF_IF_READ_DLY_EN_MSK;
		}
		if (data->cfg.pad.clock_invert) {
			tmp &= ~SF_CTRL_SF_CLK_OUT_INV_SEL_MSK;
		} else {
			tmp |= SF_CTRL_SF_CLK_OUT_INV_SEL_MSK;
		}
		if (data->cfg.pad.rx_clock_invert) {
			tmp |= SF_CTRL_SF_CLK_SF_RX_INV_SEL_MSK;
		} else {
			tmp &= ~SF_CTRL_SF_CLK_SF_RX_INV_SEL_MSK;
		}
		FLASH_WRITE32(tmp, data->reg + SF_CTRL_0_OFFSET);
	}

#if !defined(CONFIG_SOC_SERIES_BL60X)
	if (data->cfg.pad.id == PAD2 || data->cfg.pad.id == PAD3) {
		tmp = FLASH_READ32(data->reg + SF_CTRL_SF_IF_IAHB_12_OFFSET);
		tmp |= SF_CTRL_SF2_IF_READ_DLY_SRC_MSK;
		if (data->cfg.pad.read_delay > 0) {
			tmp |= SF_CTRL_SF2_IF_READ_DLY_EN_MSK;
			tmp &= ~SF_CTRL_SF2_IF_READ_DLY_N_MSK;
			tmp |= (data->cfg.pad.read_delay - 1U) << SF_CTRL_SF_IF_READ_DLY_N_POS;
		} else {
			tmp &= ~SF_CTRL_SF2_IF_READ_DLY_EN_MSK;
		}
		if (data->cfg.pad.clock_invert) {
			tmp &= ~(data->cfg.pad.id == PAD3 ?
				SF_CTRL_SF3_CLK_OUT_INV_SEL_MSK : SF_CTRL_SF2_CLK_OUT_INV_SEL_MSK);
		} else {
			tmp |= (data->cfg.pad.id == PAD3 ?
				SF_CTRL_SF3_CLK_OUT_INV_SEL_MSK : SF_CTRL_SF2_CLK_OUT_INV_SEL_MSK);
		}
		if (data->cfg.pad.rx_clock_invert) {
			tmp |= SF_CTRL_SF2_CLK_SF_RX_INV_SEL_MSK;
		} else {
			tmp &= ~SF_CTRL_SF2_CLK_SF_RX_INV_SEL_MSK;
		}
		FLASH_WRITE32(tmp, data->reg + SF_CTRL_SF_IF_IAHB_12_OFFSET);
	}
#endif

#if defined(CONFIG_SOC_SERIES_BL61X)
	/* Set IF2 delay */
	if (data->bank == BANK2) {
		tmp = FLASH_READ32(data->reg + SF_CTRL_SF_IF2_CTRL_0_OFFSET);
		if (data->cfg.pad.read_delay > 0) {
			tmp |= SF_CTRL_SF_IF2_READ_DLY_EN_MSK;
			tmp &= ~SF_CTRL_SF_IF2_READ_DLY_N_MSK;
			tmp |= (data->cfg.pad.read_delay - 1U) << SF_CTRL_SF_IF2_READ_DLY_N_POS;
		} else {
			tmp &= ~SF_CTRL_SF_IF2_READ_DLY_EN_MSK;
		}
		if (data->cfg.pad.rx_clock_invert) {
			tmp |= SF_CTRL_SF_CLK_SF_IF2_RX_INV_SEL_MSK;
		} else {
			tmp &= ~SF_CTRL_SF_CLK_SF_IF2_RX_INV_SEL_MSK;
		}
		FLASH_WRITE32(tmp, data->reg + SF_CTRL_SF_IF2_CTRL_0_OFFSET);
	}
#endif

	tmp = FLASH_READ32(GLB_BASE + BFLB_SF_CLK_REG_OFF);
	tmp |= GLB_SF_CLK_EN_MSK;
	FLASH_WRITE32(tmp, GLB_BASE + BFLB_SF_CLK_REG_OFF);
}

static __nxipfunc void flash_bflb_apply_pads(struct flash_bflb_bank_data *data)
{
	enum flash_bflb_pad bank_a, bank_b;
	size_t other = 0;

	if (data->controller->banks[other] == data) {
		other = 1;
	}

	bank_a = data->cfg.pad.id;
	if (data->controller->bank_cnt > 1) {
		bank_b = data->controller->banks[other]->cfg.pad.id;
	} else {
		bank_b = data->cfg.pad.id + 1 >= PADMAX ? PAD1 : data->cfg.pad.id + 1;
	}

	if (data->bank == BANK1) {
		flash_bflb_select_pads(data, bank_a, bank_b);
	} else {
		flash_bflb_select_pads(data, bank_b, bank_a);
	}
}

static __nxipfunc int flash_bflb_init(const struct device *dev)
{
	const struct flash_bflb_bank_config *config = dev->config;
	struct flash_bflb_bank_data *data = dev->data;
	unsigned int locker;
	int ret, ret_jedec;
	uint8_t jedec_id[3];
	uint32_t disp_1, disp_2;

#ifdef CONFIG_SOC_SERIES_BL70X
	if (data->cfg.pad.id == PAD2 && data->cfg.pad.is_external) {
		/* Use external pads */
		sys_write32(0, GLB_BASE + GLB_GPIO_USE_PSRAM__IO_OFFSET);
	} else {
		/* Use internal pads */
		sys_write32(GLB_CFG_GPIO_USE_PSRAM_IO_MSK,
			GLB_BASE + GLB_GPIO_USE_PSRAM__IO_OFFSET);
	}
#endif

	if (config->pincfg->state_cnt != 0) {
		ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret != 0) {
			return ret;
		}
	}

#if !defined(CONFIG_SOC_SERIES_BL60X)
	if (data->controller->bank_cnt > 1) {
		uint32_t tmp;

		/* Enable bank2 as early as possible (If there is a swap it needs to be enabled) */
		tmp = FLASH_READ32(data->reg + SF_CTRL_2_OFFSET);
		tmp |= SF_CTRL_SF_IF_BK2_EN_MSK;
		/* On BL70x this describes shared pad, multi-CS mode */
		if (data->controller->banks[0]->cfg.pad.id == data->controller->banks[1]->cfg.pad.id
		    && IS_ENABLED(CONFIG_SOC_SERIES_BL70X)) {
			tmp &= ~SF_CTRL_SF_IF_BK2_MODE_MSK;
		} else {
			tmp |= SF_CTRL_SF_IF_BK2_MODE_MSK;
		}
		FLASH_WRITE32(tmp, data->reg + SF_CTRL_2_OFFSET);
	}
#endif

#if !DT_ANY_INST_HAS_BOOL_STATUS_OKAY(no_header)
	if (data->bank == BANK1) {
		ret = flash_bflb_init_bootbank(data);
		if (ret != 0) {
			return ret;
		}
	}
#endif

	LOG_DBG("%s: pad %d spi_modes: %d %d auto_read: %x",
		data->bank == BANK2 ? "bank 2" : "bank 1",
		data->cfg.pad.id,
		data->cfg.auto_spi_mode, data->cfg.manual_spi_mode,
		data->cfg.cmd.auto_read);

	/* Lock IRQs before doing any interaction with SF. On zephyr, irq_lock also emulates
	 * mono-CPU irqs, so this is fine for SMP as well.
	 */
	locker = irq_lock();

	flash_bflb_apply_pads(data);

	flash_bflb_configure_timings(data);

	ret = flash_bflb_save_xip_state(dev);
	if (ret != 0) {
		/* Attempt to restore */
		flash_bflb_restore_xip_state(data);
		irq_unlock(locker);
		return ret;
	}

	ret_jedec = flash_bflb_get_jedec_id_internal(data, jedec_id);
	if (ret_jedec != 0) {
		flash_bflb_flash_disable_qpi(data);
		/* Try a reset */
		flash_bflb_reset(data);
		/* Wait because we might not have enough information yet to check for busyness */
		for (int i = 0; i < BFLB_FLASH_CHIP_RESET_TIMEOUT; i++) {
			flash_bflb_settle_x(BFLB_FLASH_1RMS);
		}
		ret_jedec = flash_bflb_get_jedec_id_internal(data, jedec_id);
	}
	if (ret_jedec != 0) {
		goto exit_nxip_bad;
	}

	/* This is a writable device, it must be fully 1 byte addressable */
	if (data->cfg.cmd.auto_write) {
		data->cfg.page_size = 1;
		data->cfg.sector_size = 1;
	}

#ifdef CONFIG_SOC_FLASH_BFLB_SFDP
	if (data->cfg.use_sfdp) {
		ret = flash_bflb_discovery(data, jedec_id);
		if (ret != 0) {
			goto exit_nxip_bad;
		}
	}
#endif

	/* Still no command so set default */
	if (data->cfg.cmd.auto_read == 0
		&& data->cfg.cmd.auto_read_dmycy == 0) {
		flash_bflb_set_default_read_default(data);
	}

	for (uint32_t i = 0; i < data->cfg.init_seq_len; i += 3) {
		ret = flash_bflb_flash_send_triplet(data, data->cfg.init_seq[i],
						    data->cfg.init_seq[i + 1],
						    data->cfg.init_seq[i + 2]);
		flash_bflb_busy_wait(data);
		if (ret != 0) {
			flash_bflb_nxip_message_set(data, NXIP_MSG_INITSEQ_FAIL,
						    data->cfg.init_seq[i],
						    data->cfg.init_seq[i + 1],
						    data->cfg.init_seq[i + 2]);
			break;
		}
	}

#if defined(CONFIG_SOC_SERIES_BL61X)
	/* Copy what the SDK does here since the role is unclear
	 * but effects (it breaks when this is not good values) are apparent
	 */
	if (data->cfg.auto_spi_mode == BUS_QIO) {
		flash_bflb_set_cmds(data, 2, 2);
	} else {
		flash_bflb_set_cmds(data, 1, 6);
	}
#endif

exit_nxip_bad:
	ret = flash_bflb_restore_xip_state(data);

	sys_cache_data_flush_and_invd_all();

	irq_unlock(locker);

	if (ret_jedec != 0) {
		LOG_ERR("Could not get JEDEC ID for %s, unable to initialize (%d, %02x%02x%02x)",
			data->bank == BANK2 ? "bank 2" : "bank 1", ret_jedec,
			jedec_id[0], jedec_id[1], jedec_id[2]);
		return -EIO;
	}

	if (*data->cfg.jedec_id == 0) {
		flash_bflb_xip_memcpy(jedec_id, data->cfg.jedec_id, 3);
	}

	disp_1 = 0;
	disp_2 = 0;
	flash_bflb_xip_memcpy(data->cfg.jedec_id, (uint8_t *)&disp_1, 3);
	flash_bflb_xip_memcpy(jedec_id, (uint8_t *)&disp_2, 3);

	if (disp_1 != disp_2) {
		disp_1 = sys_be32_to_cpu(disp_1) >> 8;
		disp_2 = sys_be32_to_cpu(disp_2) >> 8;
		LOG_WRN("JEDEC ID (%x) does not match device's (%x)", disp_1, disp_2);
	}

#ifdef CONFIG_SOC_FLASH_BFLB_SFDP
	if (data->cfg.use_sfdp) {
		LOG_DBG("Discovered at %s: size %d spi_modes: %d %d auto_read: %x, %d dmycy,%s%s%s",
			data->bank == BANK2 ? "bank 2" : "bank 1",
			data->cfg.size,
			data->cfg.auto_spi_mode, data->cfg.manual_spi_mode,
			data->cfg.cmd.auto_read, data->cfg.cmd.auto_read_dmycy,
			data->cfg.cmd.enter_32bits_addr && data->cfg.use_sfdp ?
				" 4B addr support," : "",
			data->cfg.reg.quad_enable_read_len ? " QE bit" : "",
			data->cfg.cmd.contread_on ? " EnXIP" : "");

		if (data->cfg.reg.quad_enable_read_len) {
			LOG_DBG("QE: bit %d index %d read_len %d, write_len %d",
				data->cfg.reg.quad_enable_bit, data->cfg.reg.quad_enable_index,
				data->cfg.reg.quad_enable_read_len,
				data->cfg.reg.quad_enable_write_len);
		}
		if (data->cfg.cmd.contread_on) {
			LOG_DBG("Enhanced XIP %x %x",
				data->cfg.cmd.contread_on, data->cfg.cmd.contread_off);
		}
	}
#endif

	return ret;
}

static DEVICE_API(flash, flash_bflb_api) = {
	.read = flash_bflb_read,
	.write = flash_bflb_write,
	.erase = flash_bflb_erase,
	.get_parameters = flash_bflb_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_bflb_page_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = flash_bflb_read_sfdp,
	.read_jedec_id = flash_bflb_get_jedec_id,
#endif
};

#define FLASH_BFLB_DEVICE_SET_CFG(_n)							\
	.cfg.auto_spi_mode = DT_PROP(_n, spi_bus_mode),					\
	.cfg.manual_spi_mode = BUS_NIO,							\
	.cfg.use_qpi = DT_PROP_OR(_n, use_qpi, false),					\
	.cfg.cmd.auto_read = DT_PROP_OR(_n, read_command, 0),				\
	.cfg.cmd.auto_read_dmycy = DT_PROP_OR(_n, read_dummy_cycles, 0),		\
	.cfg.cmd.auto_write = DT_PROP_OR(_n, write_command, 0),				\
	.cfg.cmd.auto_write_dmycy = DT_PROP_OR(_n, write_dummy_cycles, 0),		\
	.cfg.cmd.manual_read = SPI_NOR_CMD_READ,					\
	.cfg.cmd.manual_read_dmycy = 0,							\
	.cfg.cmd.read_reg = { SPI_NOR_CMD_RDSR, SPI_NOR_CMD_RDSR2,			\
			      SPI_NOR_CMD_RDSR3, 0 },					\
	.cfg.cmd.write_reg = { SPI_NOR_CMD_WRSR, SPI_NOR_CMD_WRSR2,			\
			      SPI_NOR_CMD_WRSR3, 0 },					\
	.cfg.cmd.contread_on = DT_PROP_OR(_n, continuous_read_command, 0),		\
	.cfg.cmd.contread_off = 0xff,							\
	.cfg.cmd.burstwrap = 0,								\
	.cfg.cmd.write_enable = SPI_NOR_CMD_WREN,					\
	.cfg.cmd.page_program = SPI_NOR_CMD_PP,						\
	.cfg.cmd.sector_erase = SPI_NOR_CMD_SE,						\
	.cfg.cmd.block_erase = SPI_NOR_CMD_BE_32K,					\
	.cfg.cmd.enter_32bits_addr = SPI_NOR_CMD_4BA,					\
	.cfg.cmd.exit_32bits_addr = 0xe9,						\
	.cfg.cmd.enter_qpi = DT_PROP_OR(_n, enter_qpi_command, 0),			\
	.cfg.cmd.exit_qpi = DT_PROP_OR(_n, exit_qpi_command, 0),			\
	.cfg.cmd.reset_enable = SPI_NOR_CMD_RESET_EN,					\
	.cfg.cmd.reset = SPI_NOR_CMD_RESET_MEM,						\
	.cfg.cmd.powerdown = SPI_NOR_CMD_DPD,						\
	.cfg.cmd.release_powerdown = SPI_NOR_CMD_RDPD,					\
	.cfg.size = DT_REG_SIZE(_n),							\
	.cfg.page_size = DUMMY_PAGE_SIZE,						\
	.cfg.sector_size = DT_PROP_OR(_n, erase_block_size, DUMMY_SECTOR_SIZE),		\
	.cfg.block_size = KB(32),							\
	.cfg.jedec_id = DT_PROP_OR(_n, jedec_id, 0),					\
	.cfg.use_sfdp = DT_PROP_OR(_n, use_sfdp, false),				\
	.cfg.init_seq = (uint32_t[]) DT_PROP_OR(_n, initialization_sequence, {}),	\
	.cfg.init_seq_len = DT_PROP_LEN_OR(_n, initialization_sequence, 0),		\
	.cfg.quirk_bytes_write = (uint8_t[]) DT_PROP_OR(_n, quirk_bytes_write, {}),	\
	.cfg.quirk_bytes_write_len = DT_PROP_LEN_OR(_n, quirk_bytes_write, 0),		\
	.cfg.quirk_bytes_read = (uint8_t[]) DT_PROP_OR(_n, quirk_bytes_read, {}),	\
	.cfg.quirk_bytes_read_len = DT_PROP_LEN_OR(_n, quirk_bytes_read, 0),		\
	.cfg.layout.pages_count = DT_REG_SIZE(_n)					\
		/ DT_PROP_OR(_n, erase_block_size, DUMMY_SECTOR_SIZE),			\
	.cfg.layout.pages_size = DT_PROP_OR(_n, erase_block_size,			\
					DUMMY_SECTOR_SIZE),				\
	.cfg.parameters.write_block_size = DT_PROP_OR(_n, write_block_size,		\
							DUMMY_WRITE_ALIGN),		\
	.cfg.parameters.erase_value = ERASE_VALUE,					\
	.cfg.reg = {0}

#define FLASH_BFLB_PAD_SET_CFG(_n)							\
	.cfg.pad.id = DT_PROP(_n, sf_pad),						\
	.cfg.pad.is_external = DT_PROP(_n, sf_pad_is_external),				\
	.cfg.pad.read_delay = DT_PROP(_n, read_delay),					\
	.cfg.pad.clock_invert = DT_PROP(_n, clock_invert),				\
	.cfg.pad.rx_clock_invert = DT_PROP(_n, rx_clock_invert),			\
	.cfg.pad.dod = DT_PROP(_n, tune_do),						\
	.cfg.pad.did = DT_PROP(_n, tune_di),						\
	.cfg.pad.csd = DT_PROP(_n, tune_cs),						\
	.cfg.pad.clkd = DT_PROP(_n, tune_clk),						\
	.cfg.pad.oed = DT_PROP(_n, tune_oe)


#define FLASH_BFLB_BANK_IS_BANK2(_n) (DT_REG_ADDR(_n) == BFLB_XIP_BASE_BANK2)

#define FLASH_BFLB_BANK_DECLARE(_n)							\
	static struct flash_bflb_bank_data flash_bflb_bank_data_##_n;

#define FLASH_BFLB_BANK_PICKUP(_n) &flash_bflb_bank_data_##_n,

#define FLASH_DEVICE_CHECK_COMPAT(_n)							\
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(_n, bflb_sf_device)				\
		     || DT_NODE_HAS_COMPAT(_n, bflb_sf_flash),				\
		     "Child of bank must be a SF Device compatible");

#define FLASH_BFLB_BANK_DEFINE(_n, _controller_n)					\
	BUILD_ASSERT(DT_REG_ADDR(_n) == BFLB_XIP_BASE_BANK1				\
		     || FLASH_BFLB_BANK_IS_BANK2(_n),					\
		     "Device address must match one of the memory mappings");		\
	BUILD_ASSERT(DT_REG_SIZE(_n) <= BFLB_XIP_SIZE,					\
		     "Device size must fit in the memory mapping");			\
	BUILD_ASSERT(DT_REG_ADDR(_n) == BFLB_XIP_BASE_BANK1				\
		     ? DT_PROP_OR(_n, write_command, 0) == 0 : true,			\
		     "Bank 1 does not support writing.");				\
	BUILD_ASSERT(DT_PROP_LEN_OR(_n, initialization_sequence, 0) % 3 == 0,		\
		     "Initialization sequence must be command data datalen triplets");	\
	BUILD_ASSERT(DT_CHILD_NUM(_n) == 1,						\
		     "A bank can only handle one and must have one device");		\
	DT_FOREACH_CHILD_STATUS_OKAY(_n, FLASH_DEVICE_CHECK_COMPAT);			\
	PINCTRL_DT_DEFINE(_n);								\
	static struct flash_bflb_bank_data flash_bflb_bank_data_##_n = {		\
		.reg = DT_REG_ADDR(_controller_n),					\
		.bank = (DT_REG_ADDR(_n) == BFLB_XIP_BASE_BANK1 ? BANK1 : BANK2),	\
		.controller = &flash_bflb_controller_data_##_controller_n,		\
		DT_FOREACH_CHILD_STATUS_OKAY(_n, FLASH_BFLB_PAD_SET_CFG),		\
		DT_FOREACH_CHILD_STATUS_OKAY(_n, FLASH_BFLB_DEVICE_SET_CFG),		\
		.xip_base = DT_REG_ADDR(_n),						\
		.nxip_message = NXIP_MSG_NONE,						\
		.last_flash_offset = 0,							\
	};										\
	struct flash_bflb_bank_config flash_bflb_bank_config_##_n = {			\
		.pincfg = PINCTRL_DT_DEV_CONFIG_GET(_n),				\
	};										\
	DEVICE_DT_DEFINE(_n, flash_bflb_init, NULL,					\
			 &flash_bflb_bank_data_##_n,					\
			 &flash_bflb_bank_config_##_n, POST_KERNEL,			\
			 CONFIG_FLASH_INIT_PRIORITY,					\
			 &flash_bflb_api);

#define FLASH_BFLB_CONTROLLER_INIT(_n)							\
	DT_FOREACH_CHILD_STATUS_OKAY(_n, FLASH_BFLB_BANK_DECLARE)			\
	static struct flash_bflb_controller_data flash_bflb_controller_data_##_n = {	\
		.override_bank1 = DT_PROP(_n, override_bank1),				\
		.addr_32bits = DT_PROP(_n, use_32b_addresses),				\
		.banks = { DT_FOREACH_CHILD_STATUS_OKAY(_n, FLASH_BFLB_BANK_PICKUP) },	\
		.bank_cnt = DT_CHILD_NUM_STATUS_OKAY(_n),				\
		.clk_divider = DT_PROP(DT_INST(0, bflb_flash_clk), divider),		\
	};										\
	DT_FOREACH_CHILD_STATUS_OKAY_VARGS(_n, FLASH_BFLB_BANK_DEFINE, _n)		\
	BUILD_ASSERT(DT_CHILD_NUM(_n) <= 2, "Only 2 banks available");			\
	BUILD_ASSERT(DT_CHILD_NUM_STATUS_OKAY(_n) > 0, "Bank 1 must be configured");


BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "There must be only one sf-controller");

DT_FOREACH_STATUS_OKAY(DT_DRV_COMPAT, FLASH_BFLB_CONTROLLER_INIT)
