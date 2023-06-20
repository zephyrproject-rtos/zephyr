/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @brief SDIO driver for Infineon CAT1 MCU family.
 *
 * This driver support only SDIO protocol of the SD interface for general
 * I/O functions.
 *
 * Refer to the SD Specifications Part 1 SDIO Specifications Version 4.10 for more
 * information on the SDIO protocol and specifications.
 *
 * Features
 * - Supports 4-bit interface
 * - Supports Ultra High Speed (UHS-I) mode
 * - Supports Default Speed (DS), High Speed (HS), SDR12, SDR25 and SDR50 speed modes
 * - Supports SDIO card interrupts in both 1-bit SD and 4-bit SD modes
 * - Supports Standard capacity (SDSC), High capacity (SDHC) and
 *   Extended capacity (SDXC) memory
 *
 * Note (limitations):
 * - current version of ifx_cat1_sdio supports only following set of commands:
 *   > GO_IDLE_STATE      (CMD0)
 *   > SEND_RELATIVE_ADDR (CMD3)
 *   > IO_SEND_OP_COND    (CMD5)
 *   > SELECT_CARD        (CMD7)
 *   > VOLTAGE_SWITCH     (CMD11)
 *   > GO_INACTIVE_STATE  (CMD15)
 *   > IO_RW_DIRECT       (CMD52)
 *   > IO_RW_EXTENDED     (CMD53)
 */

#define DT_DRV_COMPAT infineon_cat1_sdhc_sdio

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <zephyr/drivers/pinctrl.h>

#include <cyhal_sdhc.h>
#include <cyhal_sdio.h>
#include <cyhal_gpio.h>

LOG_MODULE_REGISTER(ifx_cat1_sdio, CONFIG_SDHC_LOG_LEVEL);

#include <zephyr/irq.h>

#define IFX_CAT1_SDIO_F_MIN (SDMMC_CLOCK_400KHZ)
#define IFX_CAT1_SDIO_F_MAX (SD_CLOCK_50MHZ)

struct ifx_cat1_sdio_config {
	const struct pinctrl_dev_config *pincfg;
	SDHC_Type *reg_addr;
	uint8_t irq_priority;
};

struct ifx_cat1_sdio_data {
	cyhal_sdio_t sdio_obj;
	cyhal_resource_inst_t hw_resource;
	cyhal_sdio_configurator_t cyhal_sdio_config;
	enum sdhc_clock_speed clock_speed;
	enum sdhc_bus_width bus_width;

	void *sdio_cb_user_data;
	sdhc_interrupt_cb_t sdio_cb;
};

static uint32_t sdio_rca;
static const cy_stc_sd_host_init_config_t host_config = {false, CY_SD_HOST_DMA_ADMA2, false};
static cy_en_sd_host_card_capacity_t sd_host_card_capacity = CY_SD_HOST_SDSC;
static cy_en_sd_host_card_type_t sd_host_card_type = CY_SD_HOST_NOT_EMMC;
static cy_stc_sd_host_sd_card_config_t sd_host_sd_card_config = {
	.lowVoltageSignaling = false,
	.busWidth = CY_SD_HOST_BUS_WIDTH_4_BIT,
	.cardType = &sd_host_card_type,
	.rca = &sdio_rca,
	.cardCapacity = &sd_host_card_capacity,
};

/* List of available SDHC instances */
static SDHC_Type *const IFX_CAT1_SDHC_BASE_ADDRESSES[CY_IP_MXSDHC_INSTANCES] = {
#ifdef SDHC0
	SDHC0,
#endif /* ifdef SDHC0 */

#ifdef SDHC1
	SDHC1,
#endif /* ifdef SDHC1 */
};

static int32_t _get_hw_block_num(SDHC_Type *reg_addr)
{
	uint32_t i;

	for (i = 0u; i < CY_IP_MXSDHC_INSTANCES; i++) {
		if (IFX_CAT1_SDHC_BASE_ADDRESSES[i] == reg_addr) {
			return i;
		}
	}

	return -EINVAL;
}

static int ifx_cat1_sdio_reset(const struct device *dev)
{
	struct ifx_cat1_sdio_data *dev_data = dev->data;

	cyhal_sdhc_software_reset((cyhal_sdhc_t *)&dev_data->sdio_obj);

	return 0;
}

static int ifx_cat1_sdio_set_io(const struct device *dev, struct sdhc_io *ios)
{
	cy_rslt_t ret;
	struct ifx_cat1_sdio_data *dev_data = dev->data;
	cyhal_sdio_cfg_t config = {.frequencyhal_hz = ios->clock};

	/* NOTE: Set bus width, set card power, set host signal voltage,
	 * set I/O timing does not support in current version of driver
	 */

	/* Set host clock */
	if ((dev_data->clock_speed != ios->clock) && (ios->clock != 0)) {

		if ((ios->clock > IFX_CAT1_SDIO_F_MAX) || (ios->clock < IFX_CAT1_SDIO_F_MIN)) {
			return -EINVAL;
		}

		ret = cyhal_sdio_configure(&dev_data->sdio_obj, &config);
		if (ret != CY_RSLT_SUCCESS) {
			return -ENOTSUP;
		}

		dev_data->clock_speed = ios->clock;
	}

	return 0;
}

static int ifx_cat1_sdio_card_busy(const struct device *dev)
{
	struct ifx_cat1_sdio_data *dev_data = dev->data;

	return cyhal_sdio_is_busy(&dev_data->sdio_obj) ? 1 : 0;
}

static int ifx_cat1_sdio_request(const struct device *dev, struct sdhc_command *cmd,
				 struct sdhc_data *data)
{
	struct ifx_cat1_sdio_data *dev_data = dev->data;
	int ret;

	switch (cmd->opcode) {
	case CYHAL_SDIO_CMD_GO_IDLE_STATE:
	case CYHAL_SDIO_CMD_SEND_RELATIVE_ADDR:
	case CYHAL_SDIO_CMD_IO_SEND_OP_COND:
	case CYHAL_SDIO_CMD_SELECT_CARD:
	case CYHAL_SDIO_CMD_VOLTAGE_SWITCH:
	case CYHAL_SDIO_CMD_GO_INACTIVE_STATE:
	case CYHAL_SDIO_CMD_IO_RW_DIRECT:
		ret = cyhal_sdio_send_cmd(&dev_data->sdio_obj, CYHAL_SDIO_XFER_TYPE_READ,
					  cmd->opcode, cmd->arg, cmd->response);
		if (ret != CY_RSLT_SUCCESS) {
			LOG_ERR("cyhal_sdio_send_cmd failed ret = %d \r\n", ret);
		}
		break;

	case CYHAL_SDIO_CMD_IO_RW_EXTENDED:
		cyhal_sdio_transfer_type_t direction;

		direction = (cmd->arg & BIT(SDIO_CMD_ARG_RW_SHIFT)) ? CYHAL_SDIO_XFER_TYPE_WRITE
								    : CYHAL_SDIO_XFER_TYPE_READ;

		ret = cyhal_sdio_bulk_transfer(&dev_data->sdio_obj, direction, cmd->arg, data->data,
					       data->blocks * data->block_size, cmd->response);

		if (ret != CY_RSLT_SUCCESS) {
			LOG_ERR("cyhal_sdio_bulk_transfer failed ret = %d \r\n", ret);
		}
		break;

	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int ifx_cat1_sdio_get_card_present(const struct device *dev)
{
	return 1;
}

static int ifx_cat1_sdio_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	memset(props, 0, sizeof(*props));
	props->f_max = IFX_CAT1_SDIO_F_MAX;
	props->f_min = IFX_CAT1_SDIO_F_MIN;
	props->host_caps.bus_4_bit_support = true;
	props->host_caps.high_spd_support = true;
	props->host_caps.sdr50_support = true;
	props->host_caps.sdio_async_interrupt_support = true;
	props->host_caps.vol_330_support = true;

	return 0;
}

static int ifx_cat1_sdio_enable_interrupt(const struct device *dev, sdhc_interrupt_cb_t callback,
					  int sources, void *user_data)
{
	struct ifx_cat1_sdio_data *data = dev->data;
	const struct ifx_cat1_sdio_config *cfg = dev->config;

	if (sources != SDHC_INT_SDIO) {
		return -ENOTSUP;
	}

	if (callback == NULL) {
		return -EINVAL;
	}

	/* Record SDIO callback parameters */
	data->sdio_cb = callback;
	data->sdio_cb_user_data = user_data;

	/* Enable CARD INTERRUPT event */
	cyhal_sdio_enable_event(&data->sdio_obj, CYHAL_SDIO_CARD_INTERRUPT,
				cfg->irq_priority, true);

	return 0;
}

static int ifx_cat1_sdio_disable_interrupt(const struct device *dev, int sources)
{
	struct ifx_cat1_sdio_data *data = dev->data;
	const struct ifx_cat1_sdio_config *cfg = dev->config;

	if (sources != SDHC_INT_SDIO) {
		return -ENOTSUP;
	}

	data->sdio_cb = NULL;
	data->sdio_cb_user_data = NULL;

	/* Disable CARD INTERRUPT event */
	cyhal_sdio_enable_event(&data->sdio_obj, CYHAL_SDIO_CARD_INTERRUPT,
				cfg->irq_priority, false);

	return 0;
}

static void ifx_cat1_sdio_event_callback(void *callback_arg, cyhal_sdio_event_t event)
{
	const struct device *dev = callback_arg;
	struct ifx_cat1_sdio_data *data = dev->data;

	if ((event == CYHAL_SDIO_CARD_INTERRUPT) && (data->sdio_cb != NULL)) {
		data->sdio_cb(dev, SDHC_INT_SDIO, data->sdio_cb_user_data);
	}
}

static int ifx_cat1_sdio_init(const struct device *dev)
{
	cy_rslt_t ret;
	struct ifx_cat1_sdio_data *data = dev->data;
	const struct ifx_cat1_sdio_config *config = dev->config;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	/* Dedicate SDIO HW resource */
	data->hw_resource.type = CYHAL_RSC_SDIODEV;
	data->hw_resource.block_num = _get_hw_block_num(config->reg_addr);

	/* Initialize the SDIO peripheral */
	data->cyhal_sdio_config.resource = &data->hw_resource;
	data->cyhal_sdio_config.host_config = &host_config,
	data->cyhal_sdio_config.card_config = &sd_host_sd_card_config,

	ret = cyhal_sdio_init_cfg(&data->sdio_obj, &data->cyhal_sdio_config);
	if (ret != CY_RSLT_SUCCESS) {
		LOG_ERR("cyhal_sdio_init_cfg failed ret = %d \r\n", ret);
		return ret;
	}

	/* Register callback for SDIO events */
	cyhal_sdio_register_callback(&data->sdio_obj, ifx_cat1_sdio_event_callback, (void *)dev);

	return 0;
}

static const struct sdhc_driver_api ifx_cat1_sdio_api = {
	.reset = ifx_cat1_sdio_reset,
	.request = ifx_cat1_sdio_request,
	.set_io = ifx_cat1_sdio_set_io,
	.get_card_present = ifx_cat1_sdio_get_card_present,
	.card_busy = ifx_cat1_sdio_card_busy,
	.get_host_props = ifx_cat1_sdio_get_host_props,
	.enable_interrupt = ifx_cat1_sdio_enable_interrupt,
	.disable_interrupt = ifx_cat1_sdio_disable_interrupt,
};

#define IFX_CAT1_SDHC_INIT(n)                                                                      \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct ifx_cat1_sdio_config ifx_cat1_sdio_##n##_config = {                    \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.reg_addr = (SDHC_Type *)DT_INST_REG_ADDR(n),                                      \
		.irq_priority = DT_INST_IRQ(n, priority)};                                         \
                                                                                                   \
	static struct ifx_cat1_sdio_data ifx_cat1_sdio_##n##_data;                                 \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ifx_cat1_sdio_init, NULL, &ifx_cat1_sdio_##n##_data,             \
			      &ifx_cat1_sdio_##n##_config, POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, \
			      &ifx_cat1_sdio_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_CAT1_SDHC_INIT)
