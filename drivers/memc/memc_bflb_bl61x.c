/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_bl61x_psram

#include <zephyr/device.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/bflb_bl61x_clock.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memc_bflb_bl61x, CONFIG_MEMC_LOG_LEVEL);

#include <bouffalolab/bl61x/bflb_soc.h>
#include <bouffalolab/bl61x/glb_reg.h>
#include <bouffalolab/bl61x/psram_reg.h>

#define EFUSE_DEV_INFOS_OFFSET 0x18
#define EFUSE_PSRAM_SIZE_POS 24
#define EFUSE_PSRAM_SIZE_MSK 3
#define EFUSE_FLASH_SIZE_POS 26
#define EFUSE_FLASH_SIZE_MSK 7

#define EFUSE_PSRAM_TRIM_OFFSET 0xE8
#define EFUSE_PSRAM_TRIM_EN_POS 12
#define EFUSE_PSRAM_TRIM_PARITY_POS 11
#define EFUSE_PSRAM_TRIM_POS 0
#define EFUSE_PSRAM_TRIM_MSK 0x7FF

#define PSRAM_CONFIG_WAIT 4096

struct memc_bflb_bl61x_data {
	uint32_t psram_size;
	uint32_t flash_size;
};

struct memc_bflb_bl61x_config {
	uint32_t psram_clock_divider;
	uintptr_t base;
};

/* source:
 * 0: WIFIPLL div1 (320Mhz)
 * 1: AUPLL div1
 */
static void memc_bflb_bl61x_init_psram_clock(const struct device *dev, uint8_t source)
{
	const struct memc_bflb_bl61x_config *cfg = dev->config;
	uint32_t tmp;

	/* Turn off clock*/
	tmp = sys_read32(GLB_BASE + GLB_PSRAM_CFG0_OFFSET);
	tmp &= GLB_REG_PSRAMB_CLK_EN_UMSK;
	sys_write32(tmp, GLB_BASE + GLB_PSRAM_CFG0_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_PSRAM_CFG0_OFFSET);
	tmp &= GLB_REG_PSRAMB_CLK_SEL_UMSK;
	tmp &= GLB_REG_PSRAMB_CLK_DIV_UMSK;
	tmp |= source << GLB_REG_PSRAMB_CLK_SEL_POS;
	tmp |= (cfg->psram_clock_divider - 1) << GLB_REG_PSRAMB_CLK_DIV_POS;
	sys_write32(tmp, GLB_BASE + GLB_PSRAM_CFG0_OFFSET);

	/* Turn on clock*/
	tmp = sys_read32(GLB_BASE + GLB_PSRAM_CFG0_OFFSET);
	tmp |= GLB_REG_PSRAMB_CLK_EN_MSK;
	sys_write32(tmp, GLB_BASE + GLB_PSRAM_CFG0_OFFSET);
}

/* This initializes internal pins that are not exposed via pinctrl */
static void memc_bflb_bl61x_init_gpio(void)
{
	uint32_t tmp;

	for (uint8_t i = 0; i < 12; i++) {
		tmp = GLB_REG_GPIO_41_IE_MSK | GLB_REG_GPIO_41_SMT_MSK;
		sys_write32(tmp, GLB_BASE + GLB_GPIO_CFG41_OFFSET + i * 4);
	}
}

static int memc_bflb_bl61x_get_psram_ctrl(const struct device *dev)
{
	const struct memc_bflb_bl61x_config *cfg = dev->config;
	uint32_t tmp;
	volatile int wait_ack = PSRAM_CONFIG_WAIT;

	tmp = sys_read32(cfg->base + PSRAM_CONFIGURE_OFFSET);
	tmp &= PSRAM_REG_CONFIG_REQ_UMSK;
	tmp |= 1 << PSRAM_REG_CONFIG_REQ_POS;
	sys_write32(tmp, cfg->base + PSRAM_CONFIGURE_OFFSET);

	do {
		tmp = sys_read32(cfg->base + PSRAM_CONFIGURE_OFFSET);
		wait_ack--;
	} while (!(tmp & PSRAM_REG_CONFIG_GNT_MSK) && wait_ack > 0);

	if (wait_ack <= 0) {
		return -ETIMEDOUT;
	}
	return 0;
}

static void memc_bflb_bl61x_release_psram_ctrl(const struct device *dev)
{
	const struct memc_bflb_bl61x_config *cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->base + PSRAM_CONFIGURE_OFFSET);
	tmp &= PSRAM_REG_CONFIG_REQ_UMSK;
	sys_write32(tmp, cfg->base + PSRAM_CONFIGURE_OFFSET);
}

/* reg possible values for winbond:
 * ID0: 0
 * ID1: 1
 * CR0: 2
 * CR1: 3
 * CR2: 4
 * CR3: 5
 * CR4: 6
 */
static int memc_bflb_bl61x_get_psram_reg(const struct device *dev, uint8_t reg, uint16_t *out)
{
	const struct memc_bflb_bl61x_config *cfg = dev->config;
	uint32_t tmp;
	volatile int wait_ack = PSRAM_CONFIG_WAIT;
	int err;

	err = memc_bflb_bl61x_get_psram_ctrl(dev);
	if (err < 0) {
		LOG_ERR("Get PSRAM control timed out");
		return err;
	}

	tmp = sys_read32(cfg->base + PSRAM_CONFIGURE_OFFSET);
	tmp &= PSRAM_REG_WB_REG_SEL_UMSK;
	tmp |= reg << PSRAM_REG_WB_REG_SEL_POS;
	sys_write32(tmp, cfg->base + PSRAM_CONFIGURE_OFFSET);

	/* Start read from PSRAM */
	tmp = sys_read32(cfg->base + PSRAM_CONFIGURE_OFFSET);
	tmp &= PSRAM_REG_CONFIG_R_PUSLE_UMSK;
	tmp |= 1 << PSRAM_REG_CONFIG_R_PUSLE_POS;
	sys_write32(tmp, cfg->base + PSRAM_CONFIGURE_OFFSET);

	do {
		tmp = sys_read32(cfg->base + PSRAM_CONFIGURE_OFFSET);
		wait_ack--;
	} while (!(tmp & PSRAM_STS_CONFIG_R_DONE_MSK) && wait_ack > 0);

	if (wait_ack <= 0) {
		return -ETIMEDOUT;
	}

	tmp = sys_read32(cfg->base + PSRAM_MANUAL_CONTROL_OFFSET);
	*out = (uint16_t)(tmp >> 16);

	memc_bflb_bl61x_release_psram_ctrl(dev);

	return 0;
}

static const uint16_t dqs_delay_trims[16] = {
	0x8000,
	0xC000,
	0xE000,
	0xF000,
	0xF800,
	0xFC00,
	0xFE00,
	0xFF00,
	0xFF80,
	0xFFC0,
	0xFFE0,
	0xFFF0,
	0xFFF8,
	0xFFFC,
	0xFFFE,
	0xFFFF,
};

/* There is only one configuration sold with internal PSRAM */
static int memc_bflb_bl61x_init_psram(const struct device *dev)
{
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);
	const struct memc_bflb_bl61x_config *cfg = dev->config;
	uint32_t psram_trim, psram_parity, a, b;
	uint16_t dqs_d_t;
	uint16_t check_dat;
	int err;
	uint32_t tmp;
	volatile int wait_ack = PSRAM_CONFIG_WAIT;

	err = syscon_read_reg(efuse, EFUSE_PSRAM_TRIM_OFFSET, &psram_trim);
	if (err < 0) {
		LOG_ERR("Error: Couldn't read efuses: err: %d.\n", err);
		return err;
	}
	if (!((psram_trim >> EFUSE_PSRAM_TRIM_EN_POS) & 1)) {
		LOG_WRN("No PSRAM trim");
		return -ENOTSUP;
	}
	psram_parity = (psram_trim >> EFUSE_PSRAM_TRIM_PARITY_POS) & 1;
	psram_trim = (psram_trim >> EFUSE_PSRAM_TRIM_POS) & EFUSE_PSRAM_TRIM_MSK;
	if (psram_parity != (POPCOUNT(psram_trim) & 1)) {
		LOG_ERR("Bad trim Parity");
		return -EINVAL;
	}
	a = psram_trim & 0xf;
	b = (psram_trim & 0xf0) >> 4;
	dqs_d_t = (a + b) >> 1;
	dqs_d_t = dqs_d_t > ARRAY_SIZE(dqs_delay_trims) - 1 ?
		ARRAY_SIZE(dqs_delay_trims) - 1 : dqs_d_t;
	dqs_d_t = dqs_delay_trims[dqs_d_t];

	err = memc_bflb_bl61x_get_psram_ctrl(dev);
	if (err < 0) {
		LOG_ERR("Get PSRAM control timed out");
		return err;
	}

	tmp = sys_read32(cfg->base + PSRAM_ROUGH_DELAY_CTRL5_OFFSET);
	tmp &= PSRAM_REG_ROUGH_SEL_I_DQS0_UMSK;
	tmp |= dqs_d_t << PSRAM_REG_ROUGH_SEL_I_DQS0_POS;
	sys_write32(tmp, cfg->base + PSRAM_ROUGH_DELAY_CTRL5_OFFSET);

	tmp = sys_read32(cfg->base + PSRAM_CONFIGURE_OFFSET);
	tmp &= PSRAM_REG_PCK_S_DIV_UMSK;
	tmp |= 1 << PSRAM_REG_PCK_S_DIV_POS;
	tmp &= PSRAM_REG_VENDOR_SEL_UMSK;
	/* Winbond */
	tmp |= 1 << PSRAM_REG_VENDOR_SEL_POS;
	/* X8 I/O */
	tmp &= PSRAM_REG_X16_MODE_UMSK;
	sys_write32(tmp, cfg->base + PSRAM_CONFIGURE_OFFSET);

	tmp = sys_read32(cfg->base + PSRAM_MANUAL_CONTROL2_OFFSET);
	tmp &= PSRAM_REG_ADDR_MASK_UMSK;
	/* 4MB PSRAM */
	tmp |= 0x3 << PSRAM_REG_ADDR_MASK_POS;
	tmp &= PSRAM_REG_DQS_REL_VAL_UMSK;
	tmp |= 0x1f << PSRAM_REG_DQS_REL_VAL_POS;
	sys_write32(tmp, cfg->base + PSRAM_MANUAL_CONTROL2_OFFSET);

	/* Prepare Winbond setup to send to PSRAM */
	/* 6 CLK 166MHz Latency */
	tmp = 1 << PSRAM_REG_WB_LATENCY_POS;
	/* 35 Ohms for 4M */
	tmp |= 1 << PSRAM_REG_WB_DRIVE_ST_POS;
	/* Wrapped burst */
	tmp |= 1 << PSRAM_REG_WB_HYBRID_EN_POS;
	/* 64 bytes */
	tmp |= 5 << PSRAM_REG_WB_BURST_LENGTH_POS;
	tmp |= 0 << PSRAM_REG_WB_FIX_LATENCY_POS;
	/* Disable power down mode? */
	tmp |= 1 << PSRAM_REG_WB_DPD_DIS_POS;
	/* Partial refresh is no, full */
	tmp |= 0 << PSRAM_REG_WB_PASR_POS;
	/* Hybrid sleep off */
	tmp |= 0 << PSRAM_REG_WB_HYBRID_SLP_POS;
	/* Input power down off */
	tmp |= 0 << PSRAM_REG_WB_IPD_POS;
	/* Differential input clock */
	tmp |= 0 << PSRAM_REG_WB_MCLK_TYPE_POS;
	/* Linear mode disabled */
	tmp |= 1 << PSRAM_REG_WB_LINEAR_DIS_POS;
	/* Software reset Disabled */
	tmp |= 0 << PSRAM_REG_WB_SW_RST_POS;
	sys_write32(tmp, cfg->base + PSRAM_WINBOND_PSRAM_CONFIGURE_OFFSET);

	/* Select CR0 reg */
	tmp = sys_read32(cfg->base + PSRAM_CONFIGURE_OFFSET);
	tmp &= PSRAM_REG_WB_REG_SEL_UMSK;
	tmp |= 2 << PSRAM_REG_WB_REG_SEL_POS;
	sys_write32(tmp, cfg->base + PSRAM_CONFIGURE_OFFSET);

	/* Send configuration to PSRAM */
	tmp = sys_read32(cfg->base + PSRAM_CONFIGURE_OFFSET);
	tmp &= PSRAM_REG_CONFIG_W_PUSLE_UMSK;
	tmp |= 1 << PSRAM_REG_CONFIG_W_PUSLE_POS;
	sys_write32(tmp, cfg->base + PSRAM_CONFIGURE_OFFSET);

	/* Wait for ack */
	do {
		tmp = sys_read32(cfg->base + PSRAM_CONFIGURE_OFFSET);
		wait_ack--;
	} while (!(tmp & PSRAM_STS_CONFIG_W_DONE_MSK) && wait_ack > 0);

	memc_bflb_bl61x_release_psram_ctrl(dev);

	/* Check it worked by reading PSRAM ID */
	err = memc_bflb_bl61x_get_psram_reg(dev, 0, &check_dat);
	if (err < 0) {
		LOG_ERR("PSRAM check failed");
		return err;
	}
	LOG_INF("PSRAM ID: %x", check_dat);

	return 0;
}

static int memc_bflb_bl61x_init(const struct device *dev)
{
	struct memc_bflb_bl61x_data *data = dev->data;
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);
	const struct device *clock_dev =  DEVICE_DT_GET_ANY(bflb_clock_controller);
	uint32_t dev_infos;
	int err;
	uint32_t psram_size, flash_size;

	err = syscon_read_reg(efuse, EFUSE_DEV_INFOS_OFFSET, &dev_infos);
	if (err < 0) {
		LOG_ERR("Error: Couldn't read efuses: err: %d.\n", err);
		return err;
	}
	psram_size = (dev_infos >> EFUSE_PSRAM_SIZE_POS) & EFUSE_PSRAM_SIZE_MSK;
	if (psram_size) {
		switch (psram_size) {
		case 1:
			data->psram_size = MB(4);
		break;
		case 2:
			data->psram_size = MB(8);
		break;
		case 3:
			data->psram_size = MB(16);
		break;
		default:
			data->psram_size = 0;
			LOG_WRN("Unknown PSRAM size");
		break;
		}
		LOG_INF("Built-in PSRAM Present, size: 0x%x bytes", data->psram_size);
	} else {
		LOG_INF("No Built-in PSRAM");
	}
	flash_size = (dev_infos >> EFUSE_FLASH_SIZE_POS) & EFUSE_FLASH_SIZE_MSK;
	if (flash_size) {
		switch (flash_size) {
		case 1:
			data->flash_size = MB(2);
		break;
		case 2:
			data->flash_size = MB(4);
		break;
		case 3:
			data->flash_size = MB(6);
		break;
		case 4:
			data->flash_size = MB(8);
		break;
		default:
			data->flash_size = 0;
			LOG_WRN("Unknown Flash size");
		break;
		}
		LOG_INF("Built-in Flash Present, size: 0x%x bytes", data->flash_size);
	} else {
		LOG_INF("No Built-in Flash");
	}

	if (!psram_size) {
		return 0;
	}

	/* TODO: configurable values for other PSRAM sizes (QCC74x) */
	if (data->psram_size != 0x400000) {
		LOG_ERR("Only existing 4MB Winbond X8 PSRAM Config is supported");
		return -ENOTSUP;
	}

	if (clock_control_get_status(clock_dev, (clock_control_subsys_t)BL61X_CLKID_CLK_WIFIPLL)
		== CLOCK_CONTROL_STATUS_ON) {
		memc_bflb_bl61x_init_psram_clock(dev, 0);
	} else if (clock_control_get_status(clock_dev,
		(clock_control_subsys_t)BL61X_CLKID_CLK_AUPLL) == CLOCK_CONTROL_STATUS_ON) {
		memc_bflb_bl61x_init_psram_clock(dev, 1);
	} else {
		LOG_ERR("WIFIPLL or AUPLL must be enabled to use PSRAM");
		return -ENOTSUP;
	}

	memc_bflb_bl61x_init_gpio();
	err = memc_bflb_bl61x_init_psram(dev);

	return err;
}

static struct memc_bflb_bl61x_data data = {
};

static const struct memc_bflb_bl61x_config config = {
	.psram_clock_divider = DT_INST_PROP(0, clock_divider),
	.base = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, memc_bflb_bl61x_init, NULL, &data,
	      &config, POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY, NULL);
