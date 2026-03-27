/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BL70x/BL70xL external SPI PSRAM initialization via SF_CTRL bank 2.
 *
 * Neither BL70x nor BL70xL has a dedicated memory controller — both flash
 * and PSRAM are managed through the SF_CTRL peripheral. Flash uses bank 1
 * (SF1 pad), PSRAM uses bank 2 (SF2 pad). This file initializes the PSRAM
 * chip and configures the IAHB cache paths so PSRAM is memory-mapped at
 * 0x26000000.
 *
 * Called from the flash controller driver during init.
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>

LOG_MODULE_DECLARE(flash_bflb, CONFIG_FLASH_LOG_LEVEL);

#if defined(CONFIG_SOC_SERIES_BL70XL)
#include <bouffalolab/bl70xl/bflb_soc.h>
#include <bouffalolab/bl70xl/sf_ctrl_reg.h>
#include <bouffalolab/bl70xl/l1c_reg.h>
#include <bouffalolab/bl70xl/glb_reg.h>
#else
#include <bouffalolab/bl70x/bflb_soc.h>
#include <bouffalolab/bl70x/sf_ctrl_reg.h>
#include <bouffalolab/bl70x/l1c_reg.h>
#include <bouffalolab/bl70x/glb_reg.h>
#endif

#include "flash_bflb_bl70x_l_psram.h"

/* SF controller owner */
#define SF_OWNER_SAHB 0U
#define SF_OWNER_IAHB 1U

/* SF controller pad/bank selection */
#define SF_PAD_SF1    0U
#define SF_PAD_SF2    1U
#define SF_BANK_FLASH 0U
#define SF_BANK_PSRAM 1U

/* SF controller IO modes */
#define SF_IO_NIO 0U
#define SF_IO_DO  1U
#define SF_IO_QO  2U
#define SF_IO_DIO 3U
#define SF_IO_QIO 4U

/* SF controller wrap length (512 bytes) */
#define SF_WRAP_LEN_512 6U

/* SF controller busy timeout */
#define SF_BUSY_TIMEOUT (5U * 160U * 1000U)

/* SF controller command direction */
#define CMD_READ  0U
#define CMD_WRITE 1U

/* GPIO CFGCTL register bit fields (per-pin, 16 bits each) */
#define GPIO_CFGCTL_IE_POS       0U
#define GPIO_CFGCTL_SMT_POS      1U
#define GPIO_CFGCTL_DRV_POS      2U
#define GPIO_CFGCTL_PU_POS       4U
#define GPIO_CFGCTL_PD_POS       5U
#define GPIO_CFGCTL_FUNC_SEL_POS 8U

/* GPIO function for flash/PSRAM alternate function */
#define GPIO_FUN_FLASH_PSRAM 2U

/* APMemory APS1604M-SQ SPI PSRAM command set */
#define PSRAM_CMD_READ_ID       0x9FU
#define PSRAM_CMD_READ_ID_DMY   0U
#define PSRAM_CMD_RESET_ENABLE  0x66U
#define PSRAM_CMD_RESET         0x99U
#define PSRAM_CMD_READ_REG      0xB5U
#define PSRAM_CMD_READ_REG_DMY  1U
#define PSRAM_CMD_WRITE_REG     0xB1U
#define PSRAM_CMD_QUAD_READ     0xEBU
#define PSRAM_CMD_QUAD_READ_DMY 3U
#define PSRAM_CMD_QUAD_WRITE    0x38U

/* PSRAM register bit fields */
#define PSRAM_REG_DRV_MSK   0x03U
#define PSRAM_REG_BURST_POS 5U
#define PSRAM_REG_BURST_MSK (0x03U << PSRAM_REG_BURST_POS)

/* Target drive strength: 50 ohm (value 0) */
#define PSRAM_DRIVE_STRENGTH 0U
/* Target burst length: 512 bytes (value 3) */
#define PSRAM_BURST_LENGTH   3U

/* PSRAM GPIO pins (SF2 pad set) */
static const uint8_t psram_pins[] = {27U, 26U, 23U, 25U, 24U, 28U};

/* SF_CTRL base address, set during init */
static uint32_t sf_base;

static bool sf_ctrl_is_busy(void)
{
	uint32_t val = sys_read32(sf_base + SF_CTRL_SF_IF_SAHB_0_OFFSET);

	return (val & SF_CTRL_SF_IF_BUSY_MSK) != 0U;
}

static void sf_ctrl_wait_idle(void)
{
	uint32_t timeout = SF_BUSY_TIMEOUT;

	while (sf_ctrl_is_busy() && (timeout > 0U)) {
		timeout--;
	}
}

static void psram_gpio_init(void)
{
	const uint32_t pin_cfg = BIT(GPIO_CFGCTL_IE_POS) | BIT(GPIO_CFGCTL_SMT_POS) |
				 BIT(GPIO_CFGCTL_DRV_POS) | BIT(GPIO_CFGCTL_PU_POS) |
				 (GPIO_FUN_FLASH_PSRAM << GPIO_CFGCTL_FUNC_SEL_POS);

	for (uint32_t i = 0U; i < ARRAY_SIZE(psram_pins); i++) {
		uint8_t pin = psram_pins[i];
		uint32_t addr = GLB_BASE + GLB_GPIO_CFGCTL0_OFFSET + ((uint32_t)pin / 2U) * 4U;
		uint32_t val = sys_read32(addr);

		if ((pin % 2U) == 0U) {
			val = (val & 0xFFFF0000U) | pin_cfg;
		} else {
			val = (val & 0x0000FFFFU) | (pin_cfg << 16U);
		}

		sys_write32(val, addr);
	}
}

static void sf_ctrl_select_pad(uint32_t sel)
{
	uint32_t val = sys_read32(sf_base + SF_CTRL_2_OFFSET);

	val = (val & SF_CTRL_SF_IF_PAD_SEL_UMSK) | (sel << SF_CTRL_SF_IF_PAD_SEL_POS);
	sys_write32(val, sf_base + SF_CTRL_2_OFFSET);
}

static void sf_ctrl_select_bank(uint32_t sel)
{
	uint32_t val = sys_read32(sf_base + SF_CTRL_2_OFFSET);

	if (sel == SF_BANK_FLASH) {
		val &= SF_CTRL_SF_IF_0_BK_SEL_UMSK;
	} else {
		val |= SF_CTRL_SF_IF_0_BK_SEL_MSK;
	}

	sys_write32(val, sf_base + SF_CTRL_2_OFFSET);
}

static void sf_ctrl_set_owner(uint32_t owner)
{
	uint32_t val;

	sf_ctrl_wait_idle();

	val = sys_read32(sf_base + SF_CTRL_1_OFFSET);
	val = (val & SF_CTRL_SF_IF_FN_SEL_UMSK) | (owner << SF_CTRL_SF_IF_FN_SEL_POS);

	if (owner == SF_OWNER_IAHB) {
		val |= SF_CTRL_SF_AHB2SIF_EN_MSK;
	} else {
		val &= SF_CTRL_SF_AHB2SIF_EN_UMSK;
	}

	sys_write32(val, sf_base + SF_CTRL_1_OFFSET);
}

static void sf_ctrl_send_cmd(uint32_t cmd_buf0, uint32_t cmd_buf1, uint32_t rw_flag,
			     uint32_t addr_size, uint32_t dummy_clks, uint32_t nb_data,
			     uint32_t addr_mode, uint32_t data_mode, bool qpi)
{
	uint32_t val;

	sf_ctrl_wait_idle();

	val = sys_read32(sf_base + SF_CTRL_1_OFFSET);
	if (((val & SF_CTRL_SF_IF_FN_SEL_MSK) >> SF_CTRL_SF_IF_FN_SEL_POS) != SF_OWNER_SAHB) {
		return;
	}

	val = sys_read32(sf_base + SF_CTRL_SF_IF_SAHB_0_OFFSET);
	val &= SF_CTRL_SF_IF_0_TRIG_UMSK;
	sys_write32(val, sf_base + SF_CTRL_SF_IF_SAHB_0_OFFSET);

	sys_write32(cmd_buf0, sf_base + SF_CTRL_SF_IF_SAHB_1_OFFSET);
	sys_write32(cmd_buf1, sf_base + SF_CTRL_SF_IF_SAHB_2_OFFSET);

	/* QPI mode */
	val = (val & SF_CTRL_SF_IF_0_QPI_MODE_EN_UMSK) |
	      ((qpi ? 1U : 0U) << SF_CTRL_SF_IF_0_QPI_MODE_EN_POS);

	/* IO mode */
	if (addr_mode == SF_IO_NIO) {
		if (data_mode == SF_IO_NIO) {
			val = (val & SF_CTRL_SF_IF_0_SPI_MODE_UMSK) |
			      (SF_IO_NIO << SF_CTRL_SF_IF_0_SPI_MODE_POS);
		} else if (data_mode == SF_IO_DO) {
			val = (val & SF_CTRL_SF_IF_0_SPI_MODE_UMSK) |
			      (SF_IO_DO << SF_CTRL_SF_IF_0_SPI_MODE_POS);
		} else if (data_mode == SF_IO_QIO) {
			val = (val & SF_CTRL_SF_IF_0_SPI_MODE_UMSK) |
			      (SF_IO_QO << SF_CTRL_SF_IF_0_SPI_MODE_POS);
		} else {
			/* No change for other data modes with NIO address */
		}
	} else if (addr_mode == SF_IO_DIO) {
		val = (val & SF_CTRL_SF_IF_0_SPI_MODE_UMSK) |
		      (SF_IO_DIO << SF_CTRL_SF_IF_0_SPI_MODE_POS);
	} else if (addr_mode == SF_IO_QIO) {
		val = (val & SF_CTRL_SF_IF_0_SPI_MODE_UMSK) |
		      (SF_IO_QIO << SF_CTRL_SF_IF_0_SPI_MODE_POS);
	} else {
		/* No change for other address modes */
	}

	/* Command: always enabled, 1 byte */
	val |= SF_CTRL_SF_IF_0_CMD_EN_MSK;
	val &= SF_CTRL_SF_IF_0_CMD_BYTE_UMSK;

	/* Address */
	if (addr_size != 0U) {
		val |= SF_CTRL_SF_IF_0_ADR_EN_MSK;
		val = (val & SF_CTRL_SF_IF_0_ADR_BYTE_UMSK) |
		      ((addr_size - 1U) << SF_CTRL_SF_IF_0_ADR_BYTE_POS);
	} else {
		val &= SF_CTRL_SF_IF_0_ADR_EN_UMSK;
		val &= SF_CTRL_SF_IF_0_ADR_BYTE_UMSK;
	}

	/* Dummy clocks */
	if (dummy_clks != 0U) {
		val |= SF_CTRL_SF_IF_0_DMY_EN_MSK;
		val = (val & SF_CTRL_SF_IF_0_DMY_BYTE_UMSK) |
		      ((dummy_clks - 1U) << SF_CTRL_SF_IF_0_DMY_BYTE_POS);
	} else {
		val &= SF_CTRL_SF_IF_0_DMY_EN_UMSK;
		val &= SF_CTRL_SF_IF_0_DMY_BYTE_UMSK;
	}

	/* Data */
	if (nb_data != 0U) {
		val |= SF_CTRL_SF_IF_0_DAT_EN_MSK;
		val = (val & SF_CTRL_SF_IF_0_DAT_BYTE_UMSK) |
		      ((nb_data - 1U) << SF_CTRL_SF_IF_0_DAT_BYTE_POS);
	} else {
		val &= SF_CTRL_SF_IF_0_DAT_EN_UMSK;
		val &= SF_CTRL_SF_IF_0_DAT_BYTE_UMSK;
	}

	/* Read/write flag */
	val = (val & SF_CTRL_SF_IF_0_DAT_RW_UMSK) | (rw_flag << SF_CTRL_SF_IF_0_DAT_RW_POS);
	sys_write32(val, sf_base + SF_CTRL_SF_IF_SAHB_0_OFFSET);

	/* Trigger */
	val |= SF_CTRL_SF_IF_0_TRIG_MSK;
	sys_write32(val, sf_base + SF_CTRL_SF_IF_SAHB_0_OFFSET);
}

static void sf_ctrl_send_spi_cmd(uint32_t cmd, uint32_t addr_size, uint32_t dummy_clks,
				 uint32_t nb_data, uint32_t rw_flag)
{
	sf_ctrl_send_cmd(cmd << 24U, 0U, rw_flag, addr_size, dummy_clks, nb_data, SF_IO_NIO,
			 SF_IO_NIO, false);
}

static void sf_ctrl_psram_init(void)
{
	uint32_t val;

	sf_ctrl_select_pad(SF_PAD_SF1);
	sf_ctrl_select_bank(SF_BANK_PSRAM);

	/* Enable PSRAM dual bank mode */
	val = sys_read32(sf_base + SF_CTRL_2_OFFSET);
	val |= SF_CTRL_SF_IF_BK2_EN_MSK;
	val |= SF_CTRL_SF_IF_BK2_MODE_MSK;
	sys_write32(val, sf_base + SF_CTRL_2_OFFSET);

	/* Configure PSRAM clock delay */
	val = sys_read32(sf_base + SF_CTRL_SF_IF_IAHB_12_OFFSET);
	val |= SF_CTRL_SF2_CLK_SF_RX_INV_SRC_MSK;
	val &= SF_CTRL_SF2_CLK_SF_RX_INV_SEL_UMSK;
	val |= SF_CTRL_SF2_IF_READ_DLY_SRC_MSK;
	val |= SF_CTRL_SF2_IF_READ_DLY_EN_MSK;
	val &= SF_CTRL_SF2_IF_READ_DLY_N_UMSK;
	sys_write32(val, sf_base + SF_CTRL_SF_IF_IAHB_12_OFFSET);

	/* Enable AHB access */
	val = sys_read32(sf_base + SF_CTRL_1_OFFSET);
	val |= SF_CTRL_SF_AHB2SRAM_EN_MSK;
	val |= SF_CTRL_SF_IF_EN_MSK;
	sys_write32(val, sf_base + SF_CTRL_1_OFFSET);

	sf_ctrl_set_owner(SF_OWNER_SAHB);
}

static void sf_ctrl_cmds_set(void)
{
	uint32_t val = sys_read32(sf_base + SF_CTRL_3_OFFSET);

	val |= SF_CTRL_SF_CMDS_2_EN_MSK;
	val &= SF_CTRL_SF_CMDS_2_WRAP_MODE_UMSK;
	val = (val & SF_CTRL_SF_CMDS_2_WRAP_LEN_UMSK) |
	      (SF_WRAP_LEN_512 << SF_CTRL_SF_CMDS_2_WRAP_LEN_POS);

	sys_write32(val, sf_base + SF_CTRL_3_OFFSET);
}

static void sf_ctrl_burst_toggle_set(void)
{
	uint32_t val;

	val = sys_read32(sf_base + SF_CTRL_3_OFFSET);
	val |= SF_CTRL_SF_CMDS_2_BT_EN_MSK;
	sys_write32(val, sf_base + SF_CTRL_3_OFFSET);

	val = sys_read32(sf_base + SF_CTRL_SF_IF_IAHB_6_OFFSET);
	val &= SF_CTRL_SF_IF_3_QPI_MODE_EN_UMSK;
	sys_write32(val, sf_base + SF_CTRL_SF_IF_IAHB_6_OFFSET);
}

static uint8_t psram_read_reg(void)
{
	sf_ctrl_send_spi_cmd(PSRAM_CMD_READ_REG, 3U, PSRAM_CMD_READ_REG_DMY, 1U, CMD_READ);

	while (sf_ctrl_is_busy()) {
		/* Wait for register read to complete */
	}

	return (uint8_t)sys_read32(SF_CTRL_BUF_BASE);
}

static void psram_write_reg(uint8_t regval)
{
	sys_write32((uint32_t)regval, SF_CTRL_BUF_BASE);

	sf_ctrl_send_spi_cmd(PSRAM_CMD_WRITE_REG, 3U, 0U, 1U, CMD_WRITE);
}

static int psram_set_drive_strength(void)
{
	uint8_t reg = psram_read_reg();

	if ((reg & PSRAM_REG_DRV_MSK) == PSRAM_DRIVE_STRENGTH) {
		return 0;
	}

	psram_write_reg((reg & (uint8_t)~PSRAM_REG_DRV_MSK) | (uint8_t)PSRAM_DRIVE_STRENGTH);
	reg = psram_read_reg();

	return ((reg & PSRAM_REG_DRV_MSK) == PSRAM_DRIVE_STRENGTH) ? 0 : -EIO;
}

static int psram_set_burst_wrap(void)
{
	uint8_t reg = psram_read_reg();
	uint8_t burst_val = (reg & (uint8_t)PSRAM_REG_BURST_MSK) >> PSRAM_REG_BURST_POS;

	if (burst_val == PSRAM_BURST_LENGTH) {
		return 0;
	}

	psram_write_reg((reg & (uint8_t)~PSRAM_REG_BURST_MSK) |
			(uint8_t)(PSRAM_BURST_LENGTH << PSRAM_REG_BURST_POS));
	reg = psram_read_reg();
	burst_val = (reg & (uint8_t)PSRAM_REG_BURST_MSK) >> PSRAM_REG_BURST_POS;

	return (burst_val == PSRAM_BURST_LENGTH) ? 0 : -EIO;
}

static int psram_chip_init(void)
{
	int ret;

	sf_ctrl_psram_init();
	sf_ctrl_cmds_set();
	sf_ctrl_burst_toggle_set();

	ret = psram_set_drive_strength();
	if (ret != 0) {
		return ret;
	}

	return psram_set_burst_wrap();
}

static void delay_us(uint32_t us)
{
	uint32_t start = csr_read(mcycle);
	uint32_t cycles = us * 32U;

	while ((csr_read(mcycle) - start) < cycles) {
		/* Busy-wait */
	}
}

static void psram_software_reset(void)
{
	sf_ctrl_send_spi_cmd(PSRAM_CMD_RESET_ENABLE, 0U, 0U, 0U, CMD_READ);
	while (sf_ctrl_is_busy()) {
		/* Wait for reset enable command */
	}

	sf_ctrl_send_spi_cmd(PSRAM_CMD_RESET, 0U, 0U, 0U, CMD_READ);
	while (sf_ctrl_is_busy()) {
		/* Wait for reset command */
	}

	delay_us(50U);
}

static bool psram_id_valid(const uint8_t *id)
{
	if (((id[0] == 0x00U) && (id[1] == 0x00U)) || ((id[0] == 0xFFU) && (id[1] == 0xFFU))) {
		return false;
	}

	return true;
}

static void psram_read_id(uint8_t *data)
{
	sf_ctrl_send_spi_cmd(PSRAM_CMD_READ_ID, 3U, PSRAM_CMD_READ_ID_DMY, 8U, CMD_READ);

	while (sf_ctrl_is_busy()) {
		/* Wait for ID read */
	}

	sys_put_le32(sys_read32(SF_CTRL_BUF_BASE), &data[0]);
	sys_put_le32(sys_read32(SF_CTRL_BUF_BASE + 4U), &data[4]);
}

static void sf_ctrl_psram_read_icache_set(uint32_t cmd, uint32_t dummy_clks, uint32_t addr_mode,
					  uint32_t data_mode)
{
	uint32_t val;

	sf_ctrl_wait_idle();

	val = sys_read32(sf_base + SF_CTRL_1_OFFSET);
	if (((val & SF_CTRL_SF_IF_FN_SEL_MSK) >> SF_CTRL_SF_IF_FN_SEL_POS) != SF_OWNER_IAHB) {
		return;
	}

	sys_write32(cmd << 24U, sf_base + SF_CTRL_SF_IF_IAHB_10_OFFSET);
	sys_write32(0U, sf_base + SF_CTRL_SF_IF_IAHB_11_OFFSET);

	val = sys_read32(sf_base + SF_CTRL_SF_IF_IAHB_9_OFFSET);

	val &= SF_CTRL_SF_IF_1_QPI_MODE_EN_UMSK;

	if (addr_mode == SF_IO_NIO) {
		if (data_mode == SF_IO_QIO) {
			val = (val & SF_CTRL_SF_IF_1_SPI_MODE_UMSK) |
			      (SF_IO_QO << SF_CTRL_SF_IF_1_SPI_MODE_POS);
		} else {
			val &= SF_CTRL_SF_IF_1_SPI_MODE_UMSK;
		}
	} else if (addr_mode == SF_IO_QIO) {
		val = (val & SF_CTRL_SF_IF_1_SPI_MODE_UMSK) |
		      (SF_IO_QIO << SF_CTRL_SF_IF_1_SPI_MODE_POS);
	} else {
		/* No change for other address modes */
	}

	val |= SF_CTRL_SF_IF_1_CMD_EN_MSK;
	val &= SF_CTRL_SF_IF_1_CMD_BYTE_UMSK;

	val |= SF_CTRL_SF_IF_1_ADR_EN_MSK;
	val = (val & SF_CTRL_SF_IF_1_ADR_BYTE_UMSK) | (2U << SF_CTRL_SF_IF_1_ADR_BYTE_POS);

	if (dummy_clks != 0U) {
		val |= SF_CTRL_SF_IF_1_DMY_EN_MSK;
		val = (val & SF_CTRL_SF_IF_1_DMY_BYTE_UMSK) |
		      ((dummy_clks - 1U) << SF_CTRL_SF_IF_1_DMY_BYTE_POS);
	} else {
		val &= SF_CTRL_SF_IF_1_DMY_EN_UMSK;
		val &= SF_CTRL_SF_IF_1_DMY_BYTE_UMSK;
	}

	val |= SF_CTRL_SF_IF_1_DAT_EN_MSK;
	val &= SF_CTRL_SF_IF_1_DAT_RW_UMSK;

	sys_write32(val, sf_base + SF_CTRL_SF_IF_IAHB_9_OFFSET);
}

static void sf_ctrl_psram_write_icache_set(uint32_t cmd, uint32_t addr_mode, uint32_t data_mode)
{
	uint32_t val;

	sf_ctrl_wait_idle();

	val = sys_read32(sf_base + SF_CTRL_1_OFFSET);
	if (((val & SF_CTRL_SF_IF_FN_SEL_MSK) >> SF_CTRL_SF_IF_FN_SEL_POS) != SF_OWNER_IAHB) {
		return;
	}

	sys_write32(cmd << 24U, sf_base + SF_CTRL_SF_IF_IAHB_4_OFFSET);
	sys_write32(0U, sf_base + SF_CTRL_SF_IF_IAHB_5_OFFSET);

	val = sys_read32(sf_base + SF_CTRL_SF_IF_IAHB_3_OFFSET);

	val &= SF_CTRL_SF_IF_2_QPI_MODE_EN_UMSK;

	if (addr_mode == SF_IO_NIO) {
		if (data_mode == SF_IO_QIO) {
			val = (val & SF_CTRL_SF_IF_2_SPI_MODE_UMSK) |
			      (SF_IO_QO << SF_CTRL_SF_IF_2_SPI_MODE_POS);
		} else {
			val &= SF_CTRL_SF_IF_2_SPI_MODE_UMSK;
		}
	} else if (addr_mode == SF_IO_QIO) {
		val = (val & SF_CTRL_SF_IF_2_SPI_MODE_UMSK) |
		      (SF_IO_QIO << SF_CTRL_SF_IF_2_SPI_MODE_POS);
	} else {
		/* No change for other address modes */
	}

	val |= SF_CTRL_SF_IF_2_CMD_EN_MSK;
	val &= SF_CTRL_SF_IF_2_CMD_BYTE_UMSK;

	val |= SF_CTRL_SF_IF_2_ADR_EN_MSK;
	val = (val & SF_CTRL_SF_IF_2_ADR_BYTE_UMSK) | (2U << SF_CTRL_SF_IF_2_ADR_BYTE_POS);

	val &= SF_CTRL_SF_IF_2_DMY_EN_UMSK;
	val &= SF_CTRL_SF_IF_2_DMY_BYTE_UMSK;

	val |= SF_CTRL_SF_IF_2_DAT_EN_MSK;
	val |= SF_CTRL_SF_IF_2_DAT_RW_MSK;

	sys_write32(val, sf_base + SF_CTRL_SF_IF_IAHB_3_OFFSET);
}

static void psram_enable_cache(void)
{
	uint32_t val;

	sf_ctrl_set_owner(SF_OWNER_IAHB);

	sf_ctrl_psram_read_icache_set(PSRAM_CMD_QUAD_READ, PSRAM_CMD_QUAD_READ_DMY, SF_IO_QIO,
				      SF_IO_QIO);

	sf_ctrl_psram_write_icache_set(PSRAM_CMD_QUAD_WRITE, SF_IO_QIO, SF_IO_QIO);

	val = sys_read32(L1C_BASE + L1C_CONFIG_OFFSET);
	val |= L1C_WT_EN_MSK;
	val &= L1C_WB_EN_UMSK;
	val |= L1C_WA_EN_MSK;
	val |= L1C_FORCE_BURST_0_MSK;
	sys_write32(val, L1C_BASE + L1C_CONFIG_OFFSET);
}

int flash_bflb_bl70x_l_psram_init(uint32_t sf_ctrl_base)
{
	uint8_t psram_id[8] = {0};
	unsigned int locker;
	int ret;

	locker = irq_lock();

	sf_base = sf_ctrl_base;

	psram_gpio_init();

	ret = psram_chip_init();
	if (ret != 0) {
		goto restore;
	}

	psram_software_reset();
	psram_read_id(psram_id);

	if (!psram_id_valid(psram_id)) {
		ret = -ENODEV;
	}

restore:
	/* Always restore SF_CTRL to flash bank and enable cache paths,
	 * even on failure, so flash XIP continues working.
	 */
	sf_ctrl_select_bank(SF_BANK_FLASH);
	psram_enable_cache();

	irq_unlock(locker);

	/*
	 * Log after SF_CTRL is back in IAHB mode — string literals live
	 * in flash XIP and are inaccessible while SAHB owns the bus.
	 */
	if (ret == -ENODEV) {
		LOG_ERR("PSRAM not detected");
	} else if (ret != 0) {
		LOG_ERR("PSRAM chip configuration failed: %d", ret);
	} else {
		LOG_INF("PSRAM ID: %02x %02x %02x %02x %02x %02x %02x %02x", psram_id[0],
			psram_id[1], psram_id[2], psram_id[3], psram_id[4], psram_id[5],
			psram_id[6], psram_id[7]);
	}

	return ret;
}
