/*
 * Copyright (c) 2024-2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_flash_controller

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/cache.h>

#include <soc.h>
#include <bflb_soc.h>
#include <glb_reg.h>
#include <sf_ctrl_reg.h>
#include <common_defines.h>
#include <hbn_reg.h>
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>

#if defined(CONFIG_SOC_SERIES_BL60X) || defined(CONFIG_SOC_SERIES_BL70X)
#include <l1c_reg.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_bflb, CONFIG_FLASH_LOG_LEVEL);

#define ERASE_VALUE	0xFF
#define WRITE_SIZE	DT_PROP(DT_CHOSEN(zephyr_flash), write_block_size)
#define ERASE_SIZE	DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size)
#define TOTAL_SIZE	DT_REG_SIZE(DT_CHOSEN(zephyr_flash))

#ifdef CONFIG_SOC_SERIES_BL60X
#define BFLB_XIP_BASE BL602_FLASH_XIP_BASE
#define BFLB_XIP_END  BL602_FLASH_XIP_END
#elif defined(CONFIG_SOC_SERIES_BL70X)
#define BFLB_XIP_BASE BL702_FLASH_XIP_BASE
#define BFLB_XIP_END  BL702_FLASH_XIP_END
#elif defined(CONFIG_SOC_SERIES_BL61X)
#define BFLB_XIP_BASE BL616_FLASH_XIP_BASE
#define BFLB_XIP_END  BL616_FLASH_XIP_END
#endif

#define BFLB_FLASH_CONTROLLER_BUSY_TIMEOUT_MS	200
#define BFLB_FLASH_CHIP_BUSY_TIMEOUT_MS		5000

#define BFLB_FLASH_FLASH_BLOCK_PROTECT_MSK 0x1C

#define FLASH_READ32(address)		(*((volatile uint32_t *)(address)))
#define FLASH_WRITE32(value, address)	(*((volatile uint32_t *)(address))) = value;

struct flash_bflb_config {
	uint32_t reg;
	void (*irq_config_func)(const struct device *dev);
};

#define BFLB_FLASH_MAGIC_1 "BFNP"
#define BFLB_FLASH_MAGIC_2 "FCFG"

struct bflb_flash_magic_1 {
	char magic[4];
	uint32_t revision;
} __packed;

struct bflb_flash_magic_2 {
	char magic[4];
} __packed;

/* Raw flash configuration data structure */
struct bflb_flash_cfg {
/* Serial flash interface mode, bit0-3:spi mode, bit4:unwrap, bit5:32-bits addr mode support */
	uint8_t  io_mode;
/* Support continuous read mode, bit0:continuous read mode support, bit1:read mode cfg */
	uint8_t  c_read_support;
/* SPI clock delay, bit0-3:delay,bit4-6:pad delay */
	uint8_t  clk_delay;
/* SPI clock phase invert, bit0:clck invert, bit1:rx invert, bit2-4:pad delay, bit5-7:pad delay */
	uint8_t  clk_invert;
/* Flash enable reset command */
	uint8_t  reset_en_cmd;
/* Flash reset command */
	uint8_t  reset_cmd;
/* Flash reset continuous read command */
	uint8_t  reset_c_read_cmd;
/* Flash reset continuous read command size */
	uint8_t  reset_c_read_cmd_size;
/* JEDEC ID command */
	uint8_t  jedec_id_cmd;
/* JEDEC ID command dummy clock */
	uint8_t  jedec_id_cmd_dmy_clk;
#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL60X)
/* QPI JEDEC ID command */
	uint8_t  qpi_jedec_id_cmd;
/* QPI JEDEC ID command dummy clock */
	uint8_t  qpi_jedec_id_cmd_dmy_clk;
#else
/* Enter 32-bits addr command */
	uint8_t  enter_32bits_addr_cmd;
/* Exit 32-bits addr command */
	uint8_t  exit_32bits_addr_cmd;
#endif
/* (x * 1024) bytes */
	uint8_t  sector_size;
/* Manufacturer ID */
	uint8_t  mid;
/* Page size */
	uint16_t page_size;
/* Chip erase cmd */
	uint8_t  chip_erase_cmd;
/* Sector erase command */
	uint8_t  sector_erase_cmd;
/* Block 32K erase command*/
	uint8_t  blk32_erase_cmd;
/* Block 64K erase command */
	uint8_t  blk64_erase_cmd;
/* Write enable command, needed before every erase or program, or register write */
	uint8_t  write_enable_cmd;
/* Page program command */
	uint8_t  page_program_cmd;
/* QIO page program cmd */
	uint8_t  qpage_program_cmd;
/* QIO page program address mode */
	uint8_t  qpp_addr_mode;
/* Fast read command */
	uint8_t  fast_read_cmd;
/* Fast read command dummy clock */
	uint8_t  fr_dmy_clk;
/* QPI fast read command */
	uint8_t  qpi_fast_read_cmd;
/* QPI fast read command dummy clock */
	uint8_t  qpi_fr_dmy_clk;
/* Fast read dual output command */
	uint8_t  fast_read_do_cmd;
/* Fast read dual output command dummy clock */
	uint8_t  fr_do_dmy_clk;
/* Fast read dual io command */
	uint8_t  fast_read_dio_cmd;
/* Fast read dual io command dummy clock */
	uint8_t  fr_dio_dmy_clk;
/* Fast read quad output command */
	uint8_t  fast_read_qo_cmd;
/* Fast read quad output command dummy clock */
	uint8_t  fr_qo_dmy_clk;
/* Fast read quad io command */
	uint8_t  fast_read_qio_cmd;
/* Fast read quad io command dummy clock */
	uint8_t  fr_qio_dmy_clk;
/* QPI fast read quad io command */
	uint8_t  qpi_fast_read_qio_cmd;
/* QPI fast read QIO dummy clock */
	uint8_t  qpi_fr_qio_dmy_clk;
/* QPI program command */
	uint8_t  qpi_page_program_cmd;
/* Enable write volatile reg */
	uint8_t  write_vreg_enable_cmd;
/* Write enable register index */
	uint8_t  wr_enable_index;
/* Quad mode enable register index */
	uint8_t  qe_index;
/* Busy status register index */
	uint8_t  busy_index;
/* Write enable register bit pos */
	uint8_t  wr_enable_bit;
/* Quad enable register bit pos */
	uint8_t  qe_bit;
/* Busy status register bit pos */
	uint8_t  busy_bit;
/* Register length of write enable */
	uint8_t  wr_enable_write_reg_len;
/* Register length of write enable status */
	uint8_t  wr_enable_read_reg_len;
/* Register length of quad enable */
	uint8_t  qe_write_reg_len;
/* Register length of quad enable status */
	uint8_t  qe_read_reg_len;
/* Release power down command */
	uint8_t  release_powerdown;
/* Register length of contain busy status */
	uint8_t  busy_read_reg_len;
/* Read register command buffer */
	uint8_t  read_reg_cmd[4];
/* Write register command buffer */
	uint8_t  write_reg_cmd[4];
/* Enter qpi command */
	uint8_t  enter_qpi;
/* Exit qpi command */
	uint8_t  exit_qpi;
/* Config data for continuous read mode */
	uint8_t  c_read_mode;
/* Config data for exit continuous read mode */
	uint8_t  c_rexit;
/* Enable burst wrap command */
	uint8_t  burst_wrap_cmd;
/* Enable burst wrap command dummy clock */
	uint8_t  burst_wrap_cmd_dmy_clk;
/* Data and address mode for this command */
	uint8_t  burst_wrap_data_mode;
/* Data to enable burst wrap */
	uint8_t  burst_wrap_data;
/* Disable burst wrap command */
	uint8_t  de_burst_wrap_cmd;
/* Disable burst wrap command dummy clock */
	uint8_t  de_burst_wrap_cmd_dmy_clk;
/* Data and address mode for this command */
	uint8_t  de_burst_wrap_data_mode;
/* Data to disable burst wrap */
	uint8_t  de_burst_wrap_data;
/* Typical 4K(usually) erase time */
	uint16_t time_e_sector;
/* Typical 32K erase time */
	uint16_t time_e_32k;
/* Typical 64K erase time */
	uint16_t time_e_64k;
/* Typical Page program time */
	uint16_t time_page_pgm;
/* Typical Chip erase time in ms */
	uint16_t time_ce;
/* Release power down command delay time for wake up */
	uint8_t  pd_delay;
/* QE set data */
	uint8_t  qe_data;
} __packed;

struct bflb_flash_header {
	struct bflb_flash_magic_1 magic_1;
	struct bflb_flash_magic_2 magic_2;
	struct bflb_flash_cfg flash_cfg;
	uint32_t flash_cfg_crc;
} __packed;

struct bflb_flash_command {
/* Read write 0: read 1 : write */
	uint8_t rw;
/* Command mode 0: 1 line, 1: 4 lines */
	uint8_t cmd_mode;
/* SPI mode 0: IO 1: DO 2: QO 3: DIO 4: QIO */
	uint8_t spi_mode;
/* Address size */
	uint8_t addr_size;
/* Dummy clocks */
	uint8_t dummy_clks;
/* Transfer number of bytes */
	uint32_t nb_data;
/* Command buffer */
	uint32_t cmd_buf[2];
};

struct flash_bflb_data {
	struct bflb_flash_cfg flash_cfg;
	uint32_t last_flash_offset;
	uint32_t reg_copy;
	uint32_t jedec_id;
};

/* Will using function cause error ? */
static bool flash_bflb_is_in_xip(void *func)
{
	if ((uint32_t)func > BFLB_XIP_BASE && (uint32_t)func < BFLB_XIP_END) {
		LOG_ERR("function at %p is in XIP and will crash the device", func);
		return true;
	}

	return false;
}

/* Are we doing something that makes sense? ? */
static int flash_bflb_is_valid_range(off_t offset, size_t len)
{
	if (offset < 0) {
		LOG_WRN("0x%lx: before start of flash", (long)offset);
		return -EINVAL;
	}
	if ((TOTAL_SIZE - offset) < len || len > TOTAL_SIZE) {
		LOG_WRN("0x%lx: ends past the end of flash", (long)offset);
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL60X)

static void flash_bflb_l1c_wrap(bool enable)
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

static void flash_bflb_l1c_wrap(bool enable)
{
	/* Do nothing on Bl61x: no L1C */
	ARG_UNUSED(enable);
}

#endif

/* Memcpy will not be in ram */
static void flash_bflb_xip_memcpy(volatile uint8_t *address_from, volatile uint8_t *address_to,
				  size_t size)
{
	for (size_t i = 0; i < size; i++) {
		address_to[i] = address_from[i];
	}
}

static bool flash_bflb_busy_wait(struct flash_bflb_data *data)
{
	uint32_t counter = 0;

	while ((FLASH_READ32(data->reg_copy + SF_CTRL_SF_IF_SAHB_0_OFFSET + 0)
		& SF_CTRL_SF_IF_BUSY_MSK) != 0
		&& counter < BFLB_FLASH_CONTROLLER_BUSY_TIMEOUT_MS * 20000) {
		clock_bflb_settle();
		counter++;
	}

	if ((FLASH_READ32(data->reg_copy + SF_CTRL_SF_IF_SAHB_0_OFFSET + 0)
		& SF_CTRL_SF_IF_BUSY_MSK) != 0) {
		return true;
	}

	return false;
}

/* Sets which AHB the flash controller is being talked to from
 * 0: System AHB (AHB connected to everything, E24 System Port)
 * 1: Instruction AHB (a dedicated bus between flash controller and L1C)
 */
static int flash_bflb_set_bus(struct flash_bflb_data *data, uint8_t bus)
{
	uint32_t tmp;

	if (flash_bflb_busy_wait(data)) {
		return -EBUSY;
	}

	tmp = FLASH_READ32(data->reg_copy + SF_CTRL_1_OFFSET);

	if (bus == 1) {
		tmp |= SF_CTRL_SF_IF_FN_SEL_MSK;
		tmp |= SF_CTRL_SF_AHB2SIF_EN_MSK;
	} else {
		tmp &= ~SF_CTRL_SF_IF_FN_SEL_MSK;
		tmp &= ~SF_CTRL_SF_AHB2SIF_EN_MSK;
	}

	FLASH_WRITE32(tmp, data->reg_copy + SF_CTRL_1_OFFSET);

	return 0;
}

static uint8_t flash_bflb_admode_to_spimode(uint8_t addr_mode, uint8_t data_mode)
{
	__ASSERT(addr_mode < 3, "addr_mode unhandled");
	__ASSERT(data_mode < 3, "data_mode unhandled");

	if (addr_mode == 0) {
		return data_mode;
	} else if (addr_mode == 1) {
		return 3;
	} else if (addr_mode == 2) {
		return 4;
	}

	return 0;
}

static int flash_bflb_set_command_iahb(struct flash_bflb_data *data,
				       struct bflb_flash_command *command,
				       bool doing_cmd)
{
	uint32_t tmp;
	uint32_t bank_offset = data->reg_copy + SF_CTRL_SF_IF_IAHB_0_OFFSET;

	if (flash_bflb_busy_wait(data)) {
		return -EBUSY;
	}

	tmp = FLASH_READ32(data->reg_copy + SF_CTRL_1_OFFSET);

	if ((tmp & SF_CTRL_SF_IF_FN_SEL_MSK) == 0) {
		LOG_ERR("Flash's Bus must be Instruction AHB and not System AHB");
		return -EINVAL;
	}

	FLASH_WRITE32(command->cmd_buf[0], bank_offset + 0x4);
	FLASH_WRITE32(command->cmd_buf[1], bank_offset + 0x8);

	tmp = FLASH_READ32(bank_offset + 0);

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

	FLASH_WRITE32(tmp, bank_offset + 0);

	return 0;
}

static int flash_bflb_set_command_sahb(struct flash_bflb_data *data,
				       struct bflb_flash_command *command,
				       bool doing_cmd)
{
	uint32_t tmp;
	uint32_t bank_offset = data->reg_copy + SF_CTRL_SF_IF_SAHB_0_OFFSET;

	if (flash_bflb_busy_wait(data)) {
		return -EBUSY;
	}

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
		tmp |= ((command->addr_size - 1) << SF_CTRL_SF_IF_0_ADR_BYTE_POS);
	} else {
		tmp &= ~SF_CTRL_SF_IF_0_ADR_EN_MSK;
	}

	/* configure dummy */
	tmp &= ~SF_CTRL_SF_IF_0_DMY_BYTE_MSK;
	if (command->dummy_clks != 0) {
		tmp |= SF_CTRL_SF_IF_0_DMY_EN_MSK;
		tmp |= ((command->dummy_clks - 1) << SF_CTRL_SF_IF_0_DMY_BYTE_POS);
	} else {
		tmp &= ~SF_CTRL_SF_IF_0_DMY_EN_MSK;
	}

	/* configure data */
	tmp &= ~SF_CTRL_SF_IF_0_DAT_BYTE_MSK;
	if (command->nb_data != 0) {
		tmp |= SF_CTRL_SF_IF_0_DAT_EN_MSK;
		tmp |= ((command->nb_data - 1) << SF_CTRL_SF_IF_0_DAT_BYTE_POS);
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

static int flash_bflb_send_command(struct flash_bflb_data *data, struct bflb_flash_command *command)
{
	uint32_t tmp;
	int ret;
	uint32_t bank_offset = data->reg_copy + SF_CTRL_SF_IF_SAHB_0_OFFSET;

	if (flash_bflb_is_in_xip(&flash_bflb_send_command)) {
		return -ENOTSUP;
	}

	if (flash_bflb_busy_wait(data)) {
		return -EBUSY;
	}

	tmp = FLASH_READ32(data->reg_copy + SF_CTRL_1_OFFSET);
	if (tmp & SF_CTRL_SF_IF_FN_SEL_MSK) {
		LOG_ERR("Flash's Bus must be System AHB and not Instruction AHB");
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
	tmp = FLASH_READ32(data->reg_copy + SF_CTRL_0_OFFSET);
	tmp |= SF_CTRL_SF_CLK_SAHB_SRAM_SEL_MSK;
	FLASH_WRITE32(tmp, data->reg_copy + SF_CTRL_0_OFFSET);
#endif

	/* trigger command */
	tmp = FLASH_READ32(bank_offset + 0);
	tmp |= SF_CTRL_SF_IF_0_TRIG_MSK;
	FLASH_WRITE32(tmp, bank_offset + 0);

	if (flash_bflb_busy_wait(data)) {
#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL60X)
		tmp = FLASH_READ32(data->reg_copy + SF_CTRL_0_OFFSET);
		tmp &= ~SF_CTRL_SF_CLK_SAHB_SRAM_SEL_MSK;
		FLASH_WRITE32(tmp, data->reg_copy + SF_CTRL_0_OFFSET);
#endif
		return -EBUSY;
	}

#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL60X)
	tmp = FLASH_READ32(data->reg_copy + SF_CTRL_0_OFFSET);
	tmp &= ~SF_CTRL_SF_CLK_SAHB_SRAM_SEL_MSK;
	FLASH_WRITE32(tmp, data->reg_copy + SF_CTRL_0_OFFSET);
#endif

	return 0;
}

static int flash_bflb_flash_read_register(struct flash_bflb_data *data, uint8_t index, uint8_t *out,
	uint8_t len)
{
	struct bflb_flash_command read_reg = {0};
	int ret;

	read_reg.cmd_buf[0] = (data->flash_cfg.read_reg_cmd[index]) << 24;
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


static int flash_bflb_flash_write_register(struct flash_bflb_data *data, uint8_t index, uint8_t *in,
	uint8_t len)
{
	struct bflb_flash_command write_reg = {0};

	flash_bflb_xip_memcpy(in, (uint8_t *)SF_CTRL_BUF_BASE, len);

	write_reg.cmd_buf[0] = (data->flash_cfg.write_reg_cmd[index]) << 24;
	write_reg.nb_data = len;
	write_reg.rw = 1;

	return flash_bflb_send_command(data, &write_reg);
}

static int flash_bflb_flash_disable_continuous_read(struct flash_bflb_data *data)
{
	struct bflb_flash_command disable_continuous_read = {0};

	disable_continuous_read.addr_size = data->flash_cfg.reset_c_read_cmd_size;
	disable_continuous_read.cmd_buf[0] = data->flash_cfg.reset_c_read_cmd << 24 |
	data->flash_cfg.reset_c_read_cmd << 16 | data->flash_cfg.reset_c_read_cmd << 8 |
	data->flash_cfg.reset_c_read_cmd;

	return flash_bflb_send_command(data, &disable_continuous_read);
}

static int flash_bflb_flash_disable_burst(struct flash_bflb_data *data)
{
	uint32_t tmp;
	struct bflb_flash_command disable_burstwrap = {0};

	disable_burstwrap.dummy_clks = data->flash_cfg.de_burst_wrap_cmd_dmy_clk;
	disable_burstwrap.spi_mode = flash_bflb_admode_to_spimode(
		data->flash_cfg.de_burst_wrap_data_mode,
		data->flash_cfg.de_burst_wrap_data_mode);
	disable_burstwrap.cmd_buf[0] = data->flash_cfg.de_burst_wrap_cmd << 24;
	disable_burstwrap.nb_data = 1;
	disable_burstwrap.rw = 1;
	tmp = data->flash_cfg.de_burst_wrap_data;
	FLASH_WRITE32(tmp, SF_CTRL_BUF_BASE);

	return flash_bflb_send_command(data, &disable_burstwrap);
}

static int flash_bflb_flash_enable_burst(struct flash_bflb_data *data)
{
	uint32_t tmp;
	struct bflb_flash_command enable_burstwrap = {0};

	enable_burstwrap.dummy_clks = data->flash_cfg.burst_wrap_cmd_dmy_clk;
	enable_burstwrap.spi_mode = flash_bflb_admode_to_spimode(
		data->flash_cfg.burst_wrap_data_mode,
		data->flash_cfg.burst_wrap_data_mode);
	enable_burstwrap.cmd_buf[0] = data->flash_cfg.burst_wrap_cmd << 24;
	enable_burstwrap.nb_data = 1;
	enable_burstwrap.rw = 1;
	tmp = data->flash_cfg.burst_wrap_data;
	FLASH_WRITE32(tmp, SF_CTRL_BUF_BASE);

	return flash_bflb_send_command(data, &enable_burstwrap);
}

static int flash_bflb_enable_writable(struct flash_bflb_data *data)
{
	struct bflb_flash_command write_enable = {0};
	int ret;
	uint32_t write_reg;

	write_enable.cmd_buf[0] = (data->flash_cfg.write_enable_cmd) << 24;
	ret = flash_bflb_send_command(data, &write_enable);
	if (ret != 0) {
		return ret;
	}

	/* check writable */
	ret = flash_bflb_flash_read_register(data, data->flash_cfg.wr_enable_index,
		(uint8_t *)&write_reg, data->flash_cfg.wr_enable_read_reg_len);
	if (ret != 0) {
		return ret;
	}

	if ((write_reg & (1 << data->flash_cfg.wr_enable_bit)) != 0) {
		return 0;
	}

	return -EINVAL;
}

/* /!\ UNTESTED (no relevant hardware) */
static int flash_bflb_enable_qspi(struct flash_bflb_data *data)
{
	int ret;
	uint32_t tmp = 0;
	/* qe = quad enable */

	/* writable command also enables writing to configuration registers, not just data*/
	ret = flash_bflb_enable_writable(data);
	if (ret != 0) {
		return ret;
	}

	if (data->flash_cfg.qe_read_reg_len == 0) {
		/* likely to write nothing (len = 0) */
		return flash_bflb_flash_write_register(data, data->flash_cfg.qe_index,
						       (uint8_t *)&tmp,
						       data->flash_cfg.qe_write_reg_len);
	}

	/* get quad enable register value */
	ret = flash_bflb_flash_read_register(data, data->flash_cfg.qe_index, (uint8_t *)&tmp,
		data->flash_cfg.qe_read_reg_len);
	if (ret != 0) {
		return ret;
	}

	/* qe is a bit */
	if (data->flash_cfg.qe_data == 0) {
		/* qe is already enable*/
		if ((tmp & (1 << data->flash_cfg.qe_bit)) != 0) {
			return 0;
		}
	/* qe is a specific value, not encountered in available flash chip configs  */
	} else {
		if (((tmp >> (data->flash_cfg.qe_bit & 0x08)) & 0xff) == data->flash_cfg.qe_data) {
			return 0;
		}
	}

	/* all status registers must be read and written */
	if (data->flash_cfg.qe_write_reg_len != 1) {
		ret = flash_bflb_flash_read_register(data, 0, (uint8_t *)&tmp, 1);
		if (ret != 0) {
			return ret;
		}
		ret = flash_bflb_flash_read_register(data, 1, (uint8_t *)&tmp + 1, 1);
		if (ret != 0) {
			return ret;
		}

		if (data->flash_cfg.qe_data == 0) {
			tmp |= (1 << (data->flash_cfg.qe_bit + 8 * data->flash_cfg.qe_index));
		} else {
			tmp = tmp & (~(0xff << (8 * data->flash_cfg.qe_index)));
			tmp |= (data->flash_cfg.qe_data << (8 * data->flash_cfg.qe_index));
		}

	/* we only need to read and write the appropriate register (usually the second one) */
	} else {
		if (data->flash_cfg.qe_data == 0) {
			tmp |= (1 << (data->flash_cfg.qe_bit % 8));
		} else {
			tmp = data->flash_cfg.qe_data;
		}
	}

	ret = flash_bflb_flash_write_register(data, data->flash_cfg.qe_index, (uint8_t *)&tmp,
					      data->flash_cfg.qe_write_reg_len);
	if (ret != 0) {
		return ret;
	}
	ret = flash_bflb_flash_read_register(data, data->flash_cfg.qe_index, (uint8_t *)&tmp,
					     data->flash_cfg.qe_write_reg_len);
	if (ret != 0) {
		return ret;
	}

	/* check Quad is Enabled */
	if (data->flash_cfg.qe_data == 0) {
		if ((tmp & (1 << data->flash_cfg.qe_bit)) != 0) {
			return 0;
		}
	} else {
		if (((tmp >> (data->flash_cfg.qe_bit & 0x08)) & 0xff) == data->flash_cfg.qe_data) {
			return 0;
		}
	}

	return -EINVAL;
}

static uint32_t flash_bflb_get_offset(uint32_t base_reg)
{
	uint32_t tmp;

	tmp = FLASH_READ32(base_reg + SF_CTRL_SF_ID0_OFFSET_OFFSET);
	tmp &= SF_CTRL_SF_ID0_OFFSET_MSK;
	tmp = tmp >> SF_CTRL_SF_ID0_OFFSET_POS;

	return tmp;
}

static void flash_bflb_set_offset(uint32_t base_reg, uint32_t offset)
{
	uint32_t tmp;

	tmp = FLASH_READ32(base_reg + SF_CTRL_SF_ID0_OFFSET_OFFSET);
	tmp &= ~SF_CTRL_SF_ID0_OFFSET_MSK;
	tmp |= offset << SF_CTRL_SF_ID0_OFFSET_POS;
	FLASH_WRITE32(tmp, base_reg + SF_CTRL_SF_ID0_OFFSET_OFFSET);
}

static int flash_bflb_save_xip_state(const struct device *dev)
{
	const struct flash_bflb_config *cfg = dev->config;
	struct flash_bflb_data *data = dev->data;
	int ret;

	data->reg_copy = cfg->reg;

	/* bus to system AHB */
	ret = flash_bflb_set_bus(data, 0);
	if (ret != 0) {
		return ret;
	}

	/* command to disable continuous read */
	ret = flash_bflb_flash_disable_continuous_read(data);
	if (ret != 0) {
		return ret;
	}

	/* disable burst with wrap*/
	ret = flash_bflb_flash_disable_burst(data);
	if (ret != 0) {
		return ret;
	}

	/* enable quad previous command could've disabled it
	 * 0: io 1: do 2: qo 3: dio 4: qio
	 */
	if ((data->flash_cfg.io_mode & 0xf) == 2 || (data->flash_cfg.io_mode & 0xf) == 4) {
		ret = flash_bflb_enable_qspi(data);
		if (ret != 0) {
			return ret;
		}
	}

	/* disable burst with wrap*/
	ret = flash_bflb_flash_disable_burst(data);
	if (ret != 0) {
		return ret;
	}

	data->last_flash_offset = flash_bflb_get_offset(data->reg_copy);
	sys_cache_data_flush_and_invd_all();
	flash_bflb_set_offset(data->reg_copy, 0);

	return 0;
}

static int flash_bflb_xip_init(struct flash_bflb_data *data)
{
	struct bflb_flash_command xip_cmd = {0};
	bool no_command = false;
	int ret;

	/* bus to instruction AHB */
	ret = flash_bflb_set_bus(data, 1);
	if (ret != 0) {
		return ret;
	}

	xip_cmd.spi_mode = data->flash_cfg.io_mode & 0xf;

	switch (data->flash_cfg.io_mode & 0xf) {
	default:
	case 0:
		xip_cmd.cmd_buf[0] = data->flash_cfg.fast_read_cmd << 24;
		xip_cmd.dummy_clks = data->flash_cfg.fr_dmy_clk;
	break;
	case 1:
		xip_cmd.cmd_buf[0] = data->flash_cfg.fast_read_do_cmd << 24;
		xip_cmd.dummy_clks = data->flash_cfg.fr_do_dmy_clk;
	break;
	case 2:
		xip_cmd.cmd_buf[0] = data->flash_cfg.fast_read_qo_cmd << 24;
		xip_cmd.dummy_clks = data->flash_cfg.fr_qo_dmy_clk;
	break;
	case 3:
		xip_cmd.cmd_buf[0] = data->flash_cfg.fast_read_dio_cmd << 24;
		xip_cmd.dummy_clks = data->flash_cfg.fr_dio_dmy_clk;
	break;
	case 4:
		xip_cmd.cmd_buf[0] = data->flash_cfg.fast_read_qio_cmd << 24;
		xip_cmd.dummy_clks = data->flash_cfg.fr_qio_dmy_clk;
	break;
	}
	xip_cmd.addr_size = 3;

	/* continuous read for qo and qio */
	if ((data->flash_cfg.io_mode & 0xf) == 2 || (data->flash_cfg.io_mode & 0xf) == 4) {

		if ((data->flash_cfg.c_read_support & 0x02) == 0) {
			if ((data->flash_cfg.c_read_support & 0x01) == 0) {
				/* "Not support cont read,but we still need set read mode(winbond
				 * 80dv)"
				 */
				xip_cmd.cmd_buf[1] = data->flash_cfg.c_read_mode << 24;
			} else {
				no_command = true;
				xip_cmd.cmd_buf[0] =  data->flash_cfg.c_read_mode;
			}
			xip_cmd.addr_size = 4;
		}
	}
	xip_cmd.nb_data = 32;

	return flash_bflb_set_command_iahb(data, &xip_cmd, !no_command);
}

static bool flash_bflb_flash_busy_wait(struct flash_bflb_data *data)
{
	uint8_t tmp_bus = 0xFF;
	uint32_t counter = 0;

	while ((tmp_bus & (1 << data->flash_cfg.busy_bit)) != 0 && counter <
		BFLB_FLASH_CHIP_BUSY_TIMEOUT_MS * 20000) {
		flash_bflb_flash_read_register(data, data->flash_cfg.busy_index, &tmp_bus,
					       data->flash_cfg.busy_read_reg_len);
		clock_bflb_settle();
		counter++;
	}


	if ((tmp_bus & (1 << data->flash_cfg.busy_bit)) != 0) {
		return true;
	}

	return false;
}

static int flash_bflb_restore_xip_state(struct flash_bflb_data *data)
{
	int ret;

	sys_cache_data_flush_and_invd_all();
	flash_bflb_set_offset(data->reg_copy, data->last_flash_offset);

	/* reenable burst read */
	if ((data->flash_cfg.io_mode & 0x10) != 0) {
		if ((data->flash_cfg.io_mode & 0xf) == 2 || (data->flash_cfg.io_mode & 0xf) == 4) {
			ret = flash_bflb_flash_enable_burst(data);
			if (ret != 0) {
				return ret;
			}
		}
	}

	ret = flash_bflb_xip_init(data);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

#if defined(CONFIG_SOC_FLASH_BFLB_DIRECT_ACCESS)

static int flash_bflb_read_sahb_do(struct flash_bflb_data *data, off_t address, void *buffer,
				   size_t length)
{
	int ret;
	struct bflb_flash_command read_cmd = {0};
	size_t i, cur_len;

	read_cmd.spi_mode = data->flash_cfg.io_mode & 0xf;

	switch (data->flash_cfg.io_mode & 0xf) {
	default:
	case 0:
		read_cmd.cmd_buf[0] = data->flash_cfg.fast_read_cmd << 24;
		read_cmd.dummy_clks = data->flash_cfg.fr_dmy_clk;
	break;
	case 1:
		read_cmd.cmd_buf[0] = data->flash_cfg.fast_read_do_cmd << 24;
		read_cmd.dummy_clks = data->flash_cfg.fr_do_dmy_clk;
	break;
	case 2:
		read_cmd.cmd_buf[0] = data->flash_cfg.fast_read_qo_cmd << 24;
		read_cmd.dummy_clks = data->flash_cfg.fr_qo_dmy_clk;
	break;
	case 3:
		read_cmd.cmd_buf[0] = data->flash_cfg.fast_read_dio_cmd << 24;
		read_cmd.dummy_clks = data->flash_cfg.fr_dio_dmy_clk;
	break;
	case 4:
		read_cmd.cmd_buf[0] = data->flash_cfg.fast_read_qio_cmd << 24;
		read_cmd.dummy_clks = data->flash_cfg.fr_qio_dmy_clk;
	break;
	}
	read_cmd.addr_size = 3;

	/* continuous read for qo and qio */
	if ((data->flash_cfg.io_mode & 0xf) == 2 || (data->flash_cfg.io_mode & 0xf) == 4) {

		if ((data->flash_cfg.c_read_support & 0x02) == 0) {
			if ((data->flash_cfg.c_read_support & 0x01) == 0) {
				/* "Not support cont read,but we still need set read mode(winbond
				 * 80dv)"
				 */
				read_cmd.cmd_buf[1] = data->flash_cfg.c_read_mode << 24;
			} else {
				read_cmd.cmd_buf[1] =  data->flash_cfg.c_read_mode << 24;
			}
			read_cmd.addr_size = 4;
		}
	}

	i = 0;
	while (i < length) {

		cur_len = data->flash_cfg.page_size - ((address + i) % data->flash_cfg.page_size);

		if (cur_len > length - i) {
			cur_len = length - i;
		}

		read_cmd.cmd_buf[0] &= ~0xFFFFFF;
		read_cmd.cmd_buf[0] |= (address + i);
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
static int flash_bflb_read(const struct device *dev, off_t address, void *buffer, size_t length)
{
	struct flash_bflb_data *data = dev->data;
	unsigned int	locker;
	int ret;

	if (length == 0) {
		return 0;
	}

	ret = flash_bflb_is_valid_range(address, length);
	if (ret != 0) {
		return ret;
	}

	if (flash_bflb_is_in_xip(&flash_bflb_read)) {
		return -ENOTSUP;
	}

	/* interrupting would break, likely to access XIP*/
	locker = irq_lock();

	ret = flash_bflb_save_xip_state(dev);
	if (ret != 0) {
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
static int flash_bflb_read(const struct device *dev, off_t address, void *buffer, size_t length)
{
	struct flash_bflb_data *data = dev->data;
	uint32_t	img_offset;
	unsigned int	locker;
	int ret;

	if (length == 0) {
		return 0;
	}

	ret = flash_bflb_is_valid_range(address, length);
	if (ret != 0) {
		return ret;
	}

	if (flash_bflb_is_in_xip(&flash_bflb_read)) {
		return -ENOTSUP;
	}

	/* interrupting would break, likely to access XIP*/
	locker = irq_lock();

	/* get XIP offset / where code really is in flash, usually 0x2000 */
	img_offset = flash_bflb_get_offset(data->reg_copy);

	/* need set offset to 0 to access? */
	if (address < img_offset) {

		sys_cache_data_flush_and_invd_all();

		/* set offset to 0 to access first (likely)0x2000 of flash */
		flash_bflb_set_offset(data->reg_copy, 0);

		/* copy data we need */
		flash_bflb_xip_memcpy((uint8_t *)(address + BFLB_XIP_BASE),
				      (uint8_t *)buffer, length);

		sys_cache_data_flush_and_invd_all();

		flash_bflb_set_offset(data->reg_copy, img_offset);
	} else {
		/* copy data we need */
		flash_bflb_xip_memcpy((uint8_t *)(address + BFLB_XIP_BASE - img_offset),
				      (uint8_t *)buffer, length);
	}

	/* done with interrupt breaking stuffs */
	irq_unlock(locker);

	return 0;
}

#endif

static int flash_bflb_write(const struct device *dev,
			     off_t address,
			     const void *buffer,
			     size_t length)
{
	struct flash_bflb_data *data = dev->data;
	uint32_t	tmp;
	unsigned int	locker;
	int		ret, rete;
	uint32_t	cur_len, i;
	struct bflb_flash_command write_cmd = {0};

	if (length == 0) {
		return 0;
	}

	ret = flash_bflb_is_valid_range(address, length);
	if (ret != 0) {
		return ret;
	}

	if (flash_bflb_is_in_xip(&flash_bflb_write)) {
		return -ENOTSUP;
	}

	/* interrupting would break, likely to access XIP*/
	locker = irq_lock();

	ret = flash_bflb_save_xip_state(dev);
	if (ret != 0) {
		irq_unlock(locker);
		return ret;
	}

	/* Check if the flash chip is OK to write to */
	ret = flash_bflb_flash_read_register(data, 0, (uint8_t *)(&tmp), 1);
	if (ret != 0) {
		goto exit_here;
	}
	if ((tmp & BFLB_FLASH_FLASH_BLOCK_PROTECT_MSK) != 0) {
		ret = -EINVAL;
		goto exit_here;
	}

	if ((data->flash_cfg.io_mode & 0xf) == 0
		|| (data->flash_cfg.io_mode & 0xf) == 1
		|| (data->flash_cfg.io_mode & 0xf) == 3) {
		write_cmd.cmd_buf[0] = data->flash_cfg.page_program_cmd << 24;
	/* quad mode */
	} else {
		write_cmd.cmd_buf[0] = data->flash_cfg.qpage_program_cmd << 24;
		write_cmd.spi_mode =
			flash_bflb_admode_to_spimode(data->flash_cfg.qpp_addr_mode, 2);
	}
	write_cmd.rw = 1;
	write_cmd.addr_size = 3;

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
		cur_len = data->flash_cfg.page_size - ((address + i) % data->flash_cfg.page_size);

		if (cur_len > length - i) {
			cur_len = length - i;
		}

		flash_bflb_xip_memcpy((uint8_t *)(buffer) +  i, (uint8_t *)SF_CTRL_BUF_BASE,
				      cur_len);
		write_cmd.cmd_buf[0] &= ~0xFFFFFF;
		write_cmd.cmd_buf[0] |= (address + i);
		write_cmd.nb_data = cur_len;
		ret = flash_bflb_send_command(data, &write_cmd);
		if (ret != 0) {
			goto exit_here;
		}

		i += cur_len;

		flash_bflb_busy_wait(data);
		flash_bflb_flash_busy_wait(data);
	}

exit_here:
	rete = flash_bflb_restore_xip_state(data);
	irq_unlock(locker);

	return (ret != 0 ? ret : rete);
}

static int flash_bflb_erase(const struct device *dev, off_t start, size_t len)
{
	struct flash_bflb_data *data = dev->data;
	unsigned int	locker;
	int		ret, rete;
	struct bflb_flash_command erase_cmd = {0};
	uint32_t erase_start = start / ERASE_SIZE;
	uint32_t erase_end = (len / ERASE_SIZE) + start / ERASE_SIZE;

	if (len == 0) {
		return 0;
	}

	ret = flash_bflb_is_valid_range(start, len);
	if (ret != 0) {
		return ret;
	}

	if (flash_bflb_is_in_xip(&flash_bflb_write)) {
		return -ENOTSUP;
	}

	if ((len % ERASE_SIZE) != 0) {
		LOG_WRN("Length is not a multiple of minimal erase block size");
		return -EINVAL;
	}

	if ((start % ERASE_SIZE) != 0) {
		LOG_WRN("Start address is not a multiple of minimal erase block size");
		return -EINVAL;
	}
	/* interrupting would break, likely to access XIP*/
	locker = irq_lock();

	ret = flash_bflb_save_xip_state(dev);
	if (ret != 0) {
		irq_unlock(locker);
		return ret;
	}

	erase_cmd.rw = 0;
	erase_cmd.addr_size = 3;

	for (uint32_t i = erase_start; i < erase_end; i++) {
		/* Write enable is needed for every write */
		ret = flash_bflb_enable_writable(data);
		if (ret != 0) {
			goto exit_here;
		}

		erase_cmd.cmd_buf[0] = data->flash_cfg.sector_erase_cmd << 24;
		erase_cmd.cmd_buf[0] |= (i * data->flash_cfg.sector_size * 1024);

		ret = flash_bflb_send_command(data, &erase_cmd);
		if (ret != 0) {
			goto exit_here;
		}

		flash_bflb_busy_wait(data);
		flash_bflb_flash_busy_wait(data);
	}

exit_here:
	rete = flash_bflb_restore_xip_state(data);
	irq_unlock(locker);

	return (ret != 0 ? ret : rete);
}


#if CONFIG_FLASH_PAGE_LAYOUT
static struct flash_pages_layout flash_bflb_pages_layout = {
	.pages_count = TOTAL_SIZE / ERASE_SIZE,
	.pages_size = ERASE_SIZE,
};

void flash_bflb_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	*layout = &flash_bflb_pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters flash_bflb_parameters = {
	.write_block_size = WRITE_SIZE,
	.erase_value = ERASE_VALUE,
};

static const struct flash_parameters *flash_bflb_get_parameters(const struct device *dev)
{
	return &flash_bflb_parameters;
}

static void flash_bflb_isr(const struct device *dev)
{
	/* No interrupts */
}

static DEVICE_API(flash, flash_bflb_api) = {
	.read = flash_bflb_read,
	.write = flash_bflb_write,
	.erase = flash_bflb_erase,
	.get_parameters = flash_bflb_get_parameters,
#if CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_bflb_page_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

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

/* /!\ this function cannot run from XIP! */
static int flash_bflb_config_init(const struct device *dev)
{
	struct flash_bflb_data *data = dev->data;
	const struct flash_bflb_config *cfg = dev->config;
	uint32_t tmp;
	uint32_t img_offset;
	unsigned int locker;
	struct bflb_flash_header header;

	if (flash_bflb_is_in_xip(&flash_bflb_config_init)) {
		return -ENOTSUP;
	}

	/* copy cfg reg to memory as cfg will not be in it and inaccessible */
	data->reg_copy = cfg->reg;

	/* get flash config using xip access */

	/* interrupting would break, likely to access XIP*/
	locker = irq_lock();
	/* get XIP offset / where code really is in flash, usually 0x2000 */
	img_offset = flash_bflb_get_offset(data->reg_copy);

	sys_cache_data_flush_and_invd_all();

	/* set offset to 0 to access first (likely)0x2000 of flash */
	flash_bflb_set_offset(data->reg_copy, 0);

	/* copy data we need */
	flash_bflb_xip_memcpy((uint8_t *)(BFLB_XIP_BASE),
			      (uint8_t *)&header, sizeof(struct bflb_flash_header));

	sys_cache_data_flush_and_invd_all();

	flash_bflb_set_offset(data->reg_copy, img_offset);

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

	tmp = bflb_soft_crc32(0, (uint8_t *)(&header.flash_cfg), sizeof(struct bflb_flash_cfg));
	if (tmp != header.flash_cfg_crc) {
		LOG_ERR("Flash data crc is incorrect %d vs %d", tmp, header.flash_cfg_crc);
		return -EINVAL;
	}
	flash_bflb_xip_memcpy((uint8_t *)&(header.flash_cfg),
			      (uint8_t *)&(data->flash_cfg), sizeof(struct bflb_flash_cfg));

	return 0;
}

/* grabs the id with byte order inverted (LSB to MSB) */
static int flash_bflb_get_jedecid_live(struct flash_bflb_data *data, uint32_t *output)
{
	struct bflb_flash_command get_jedecid = {0};
	int ret;

	get_jedecid.dummy_clks = data->flash_cfg.jedec_id_cmd_dmy_clk;
	get_jedecid.cmd_buf[0] = data->flash_cfg.jedec_id_cmd << 24;
	get_jedecid.nb_data = 3;

	ret = flash_bflb_send_command(data, &get_jedecid);
	if (ret < 0) {
		return ret;
	}

	*output = FLASH_READ32(SF_CTRL_BUF_BASE);

	return 0;
}

static int flash_bflb_init(const struct device *dev)
{
	const struct flash_bflb_config *cfg = dev->config;
	struct flash_bflb_data *data = dev->data;
	unsigned int	locker;
	int ret;

	ret = flash_bflb_config_init(dev);
	if (ret < 0) {
		return ret;
	}

	locker = irq_lock();

	ret = flash_bflb_save_xip_state(dev);
	if (ret != 0) {
		irq_unlock(locker);
		return ret;
	}

	flash_bflb_get_jedecid_live(data, &(data->jedec_id));

	/* operations done here in bflb driver but not here:
	 * - reenable qspi (already done in flash_bflb_save_xip_state in bflb driver too)
	 * - reenable flash-side burstwrap (already done in restore state, possibly need to be done
	 * before l1c wrap side)
	 */
	if ((data->flash_cfg.io_mode & 0x10) == 0) {
		flash_bflb_l1c_wrap(true);
	} else {
		flash_bflb_l1c_wrap(false);
	}

	ret = flash_bflb_restore_xip_state(data);
	if (ret != 0) {
		irq_unlock(locker);
		return ret;
	}

	irq_unlock(locker);

	cfg->irq_config_func(dev);

	return 0;
}

#define FLASH_BFLB_DEVICE(n)						\
	static void flash_bflb_irq_config_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    flash_bflb_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}								\
	static const struct flash_bflb_config flash_bflb_config_##n = {	\
		.reg = DT_INST_REG_ADDR_BY_IDX(n, 0),			\
		.irq_config_func = &flash_bflb_irq_config_##n,		\
	};								\
	static struct flash_bflb_data flash_bflb_data_##n;		\
	DEVICE_DT_INST_DEFINE(n, flash_bflb_init, NULL,			\
			      &flash_bflb_data_##n,			\
			      &flash_bflb_config_##n, POST_KERNEL,	\
			      CONFIG_FLASH_INIT_PRIORITY,		\
			      &flash_bflb_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_BFLB_DEVICE)
