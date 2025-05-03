/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5912_espi

#include <zephyr/kernel.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(espi, CONFIG_ESPI_LOG_LEVEL);

#include "espi_utils.h"
#include "reg/reg_espi.h"

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "support only one espi compatible node");

struct espi_rts5912_config {
	volatile struct espi_reg *const espi_reg;
	uint32_t espislv_clk_grp;
	uint32_t espislv_clk_idx;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pcfg;
};

struct espi_rts5912_data {
	sys_slist_t callbacks;
	uint32_t config_data;
};

/*
 * =========================================================================
 * ESPI common function and API
 * =========================================================================
 */

#define RTS5912_ESPI_MAX_FREQ_20 20
#define RTS5912_ESPI_MAX_FREQ_25 25
#define RTS5912_ESPI_MAX_FREQ_33 33
#define RTS5912_ESPI_MAX_FREQ_50 50
#define RTS5912_ESPI_MAX_FREQ_66 66

static int espi_rts5912_configure(const struct device *dev, struct espi_cfg *cfg)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	uint32_t gen_conf = 0;
	uint8_t io_mode = 0;

	/* Maximum Frequency Supported */
	switch (cfg->max_freq) {
	case RTS5912_ESPI_MAX_FREQ_20:
		gen_conf |= 0UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case RTS5912_ESPI_MAX_FREQ_25:
		gen_conf |= 1UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case RTS5912_ESPI_MAX_FREQ_33:
		gen_conf |= 2UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case RTS5912_ESPI_MAX_FREQ_50:
		gen_conf |= 3UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case RTS5912_ESPI_MAX_FREQ_66:
		gen_conf |= 4UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	default:
		return -EINVAL;
	}

	/* I/O Mode Supported */
	io_mode = cfg->io_caps >> 1;

	if (io_mode > 3) {
		return -EINVAL;
	}
	gen_conf |= io_mode << ESPI_ESPICFG_IOSUP_Pos;

	/* Channel Supported */
	if (cfg->channel_caps & ESPI_CHANNEL_PERIPHERAL) {
		gen_conf |= BIT(0) << ESPI_ESPICFG_CHSUP_Pos;
	}

	if (cfg->channel_caps & ESPI_CHANNEL_VWIRE) {
		gen_conf |= BIT(1) << ESPI_ESPICFG_CHSUP_Pos;
	}

	if (cfg->channel_caps & ESPI_CHANNEL_OOB) {
		gen_conf |= BIT(2) << ESPI_ESPICFG_CHSUP_Pos;
	}

	if (cfg->channel_caps & ESPI_CHANNEL_FLASH) {
		gen_conf |= BIT(3) << ESPI_ESPICFG_CHSUP_Pos;
	}

	espi_reg->ESPICFG = gen_conf;
	data->config_data = espi_reg->ESPICFG;

	return 0;
}

static bool espi_rts5912_channel_ready(const struct device *dev, enum espi_channel ch)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	switch (ch) {
	case ESPI_CHANNEL_PERIPHERAL:
		return espi_reg->EPCFG & ESPI_EPCFG_CHEN ? true : false;
	case ESPI_CHANNEL_VWIRE:
		return espi_reg->EVCFG & ESPI_EVCFG_CHEN ? true : false;
	case ESPI_CHANNEL_OOB:
		return espi_reg->EOCFG & ESPI_EOCFG_CHEN ? true : false;
	case ESPI_CHANNEL_FLASH:
		return espi_reg->EFCONF & ESPI_EFCONF_CHEN ? true : false;
	default:
		return false;
	}
}

static int espi_rts5912_manage_callback(const struct device *dev, struct espi_callback *callback,
					bool set)
{
	struct espi_rts5912_data *data = dev->data;

	return espi_manage_callback(&data->callbacks, callback, set);
}

static DEVICE_API(espi, espi_rts5912_driver_api) = {
	.config = espi_rts5912_configure,
	.get_channel_status = espi_rts5912_channel_ready,
	.manage_callback = espi_rts5912_manage_callback,
};

static void espi_rst_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *data = dev->data;

	struct espi_event evt = {.evt_type = ESPI_BUS_RESET, .evt_details = 0, .evt_data = 0};

	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint32_t status = espi_reg->ERSTCFG;

	espi_reg->ERSTCFG |= ESPI_ERSTCFG_RSTSTS;
	espi_reg->ERSTCFG ^= ESPI_ERSTCFG_RSTPOL;

	if (status & ESPI_ERSTCFG_RSTSTS) {
		if (status & ESPI_ERSTCFG_RSTPOL) {
			/* rst pin high go low trigger interrupt */
			evt.evt_data = 0;
		} else {
			/* rst pin low go high trigger interrupt */
			evt.evt_data = 1;
		}
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}

static void espi_bus_reset_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	espi_reg->ERSTCFG = ESPI_ERSTCFG_RSTINTEN;
	espi_reg->ERSTCFG = ESPI_ERSTCFG_RSTMONEN;

	if (espi_reg->ERSTCFG & ESPI_ERSTCFG_RSTSTS) {
		/* high to low */
		espi_reg->ERSTCFG =
			ESPI_ERSTCFG_RSTMONEN | ESPI_ERSTCFG_RSTPOL | ESPI_ERSTCFG_RSTINTEN;
	} else {
		/* low to high */
		espi_reg->ERSTCFG = ESPI_ERSTCFG_RSTMONEN | ESPI_ERSTCFG_RSTINTEN;
	}

	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), bus_rst, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), bus_rst, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), bus_rst, priority), espi_rst_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), bus_rst, irq));
}

static int espi_rts5912_init(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct rts5912_sccon_subsys sccon;

	int rc;

	/* Setup eSPI pins */
	rc = pinctrl_apply_state(espi_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		LOG_ERR("eSPI pinctrl setup failed (%d)", rc);
		return rc;
	}

	if (!device_is_ready(espi_config->clk_dev)) {
		LOG_ERR("eSPI clock not ready");
		return -ENODEV;
	}

	/* Enable eSPI clock */
	sccon.clk_grp = espi_config->espislv_clk_grp;
	sccon.clk_idx = espi_config->espislv_clk_idx;
	rc = clock_control_on(espi_config->clk_dev, (clock_control_subsys_t)&sccon);
	if (rc != 0) {
		LOG_ERR("eSPI clock control on failed");
		goto exit;
	}

	/* Setup eSPI bus reset */
	espi_bus_reset_setup(dev);
exit:
	return rc;
}

PINCTRL_DT_INST_DEFINE(0);

static struct espi_rts5912_data espi_rts5912_data_0;

static const struct espi_rts5912_config espi_rts5912_config = {
	.espi_reg = (volatile struct espi_reg *const)DT_INST_REG_ADDR_BY_NAME(0, espi_target),
	.espislv_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(0), espi_target, clk_grp),
	.espislv_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(0), espi_target, clk_idx),
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

DEVICE_DT_INST_DEFINE(0, &espi_rts5912_init, NULL, &espi_rts5912_data_0, &espi_rts5912_config,
		      PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY, &espi_rts5912_driver_api);
