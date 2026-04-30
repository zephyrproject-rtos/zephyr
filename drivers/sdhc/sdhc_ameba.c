/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_ameba_sdhc

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <ameba_soc.h>
#include <soc.h>

#include <zephyr/cache.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/ameba_clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sdhc_ameba, CONFIG_SDHC_LOG_LEVEL);

typedef void (*irq_config_func_t)(void);

struct sdhc_ameba_config {
	unsigned int max_freq;             /* Max bus frequency in Hz */
	unsigned int min_freq;             /* Min bus frequency in Hz */
	uint8_t bus_width;                 /* Width of the SDIO bus */
	uint32_t power_delay_ms;           /* power delay prop for the host in milliseconds */
	uint32_t reg_addr;                 /* Base address of the SDIO peripheral register block */
	SD_HdlTypeDef *hsd;                /* Pointer to SDIO HAL handle */
	irq_config_func_t irq_config_func; /* IRQ config function */
	struct gpio_dt_spec cd_gpio;       /* Card detect GPIO pin */
	const struct pinctrl_dev_config *pcfg; /* Pointer to pin control configuration */
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
};

struct sdhc_ameba_data {
	struct k_mutex bus_mutex;     /* Sync between commands */
	struct sdhc_io host_io;       /* Input/Output host configuration */
	struct sdhc_host_props props; /* current host properties */
};

static int sdhc_ameba_activate(const struct device *dev)
{
	int ret;
	const struct sdhc_ameba_config *config = dev->config;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	RCC_PeriphClockSourceSet(SDH, SYS_PLL);
	RCC_PeriphClockDividerFENSet(SYS_PLL_SDH, 1);

	if (clock_control_on(config->clock_dev, config->clock_subsys)) {
		LOG_ERR("Could not enable SDHC clock");
		return -EIO;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int sdhc_ameba_sd_init(const struct device *dev)
{
	struct sdhc_ameba_data *data = dev->data;
	const struct sdhc_ameba_config *config = dev->config;
	SD_HdlTypeDef *hsd = config->hsd;

	data->host_io.bus_width = config->bus_width;
	hsd->Instance = (SDIOHOST_TypeDef *)config->reg_addr;

	if (SDIO_ResetAll(hsd->Instance) != HAL_OK) {
		LOG_ERR("Failed to de-initialize the SDIO device");
		return -EIO;
	}

	if (SDIO_CheckState(hsd->Instance) != HAL_OK) {
		LOG_ERR("Failed to de-initialize the SDIO device");
		return -EIO;
	}

	if (SDIOH_Init(hsd->Instance) != HAL_OK) {
		LOG_ERR("Failed to initialize the SDIO device");
		return -EIO;
	}

	return 0;
}

static void sdhc_ameba_init_props(const struct device *dev)
{
	const struct sdhc_ameba_config *sdhc_config = dev->config;
	struct sdhc_ameba_data *data = dev->data;
	struct sdhc_host_props *props = &data->props;

	memset(props, 0, sizeof(struct sdhc_host_props));

	props->f_min = sdhc_config->min_freq;
	props->f_max = sdhc_config->max_freq;
	props->power_delay = sdhc_config->power_delay_ms;
	props->host_caps.vol_330_support = true;
	props->host_caps.vol_180_support = false;
	props->host_caps.bus_8_bit_support = false;
	props->host_caps.high_spd_support = (sdhc_config->bus_width == SDHC_BUS_WIDTH4BIT);
	props->host_caps.bus_4_bit_support = (sdhc_config->bus_width == SDHC_BUS_WIDTH4BIT);
	props->is_spi = false;
}

static int sdhc_ameba_card_busy(const struct device *dev)
{
	const struct sdhc_ameba_config *config = dev->config;

	if (config->hsd->State == SD_STATE_BUSY) {
		LOG_DBG("busy");
	}

	return (config->hsd->State == SD_STATE_BUSY);
}

static int sdhc_ameba_reset(const struct device *dev)
{
	struct sdhc_ameba_data *data = dev->data;
	const struct sdhc_ameba_config *config = dev->config;
	uint32_t res;

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	/* Resetting Host controller */
	SDIO_PowerState_OFF(config->hsd->Instance);
	k_msleep(data->props.power_delay);
	SDIO_PowerState_ON(config->hsd->Instance);
	k_msleep(data->props.power_delay);

	/* Resetting card */
	res = SDIO_ResetAll(config->hsd->Instance);
	if (res != 0U) {
		LOG_ERR("Failed to reset sdhc!");
	}

	k_mutex_unlock(&data->bus_mutex);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(dev);

	return res == 0U ? 0 : -EIO;
}

static int sdhc_ameba_set_data_dir(const struct sdhc_command *cmd,
				   SDIO_DataInitTypeDef *data_config)
{
	switch (cmd->opcode) {
	case SD_SWITCH:                   /* CMD6 */
	case MMC_SEND_EXT_CSD:            /* CMD8 for MMC */
	case SD_READ_SINGLE_BLOCK:        /* CMD17 */
	case SD_READ_MULTIPLE_BLOCK:      /* CMD18 */
	case SD_APP_SEND_NUM_WRITTEN_BLK: /* ACMD22 */
	case SD_APP_SEND_SCR:             /* ACMD51 */
		data_config->TransDir = SDIO_TRANS_CARD_TO_HOST;
		break;
	case SD_WRITE_SINGLE_BLOCK:   /* CMD24 */
	case SD_WRITE_MULTIPLE_BLOCK: /* CMD25 */
		data_config->TransDir = SDIO_TRANS_HOST_TO_CARD;
		break;
	case SDIO_RW_EXTENDED: /* CMD53 */
		if (cmd->arg & BIT(SDIO_CMD_ARG_RW_SHIFT)) {
			data_config->TransDir = SDIO_TRANS_HOST_TO_CARD; /* write */
		} else {
			data_config->TransDir = SDIO_TRANS_CARD_TO_HOST; /* read */
		}
		break;
	default:
		LOG_ERR("Unsupported CMD%d!", cmd->opcode);
		return -ENOTSUP;
	}
	return 0;
}

static int sdhc_ameba_finish_data_transfer(const struct sdhc_ameba_config *config,
					   struct sdhc_data *data, uint8_t *align_buf,
					   bool align_buf_alloc)
{
	if (SD_WaitTransDone(config->hsd, data->timeout_ms * 1000) != HAL_OK) {
		LOG_ERR("WaitTransDone error!");
		return -EBUSY;
	}

	/* enable SDIOHOST_BIT_CARD_INT_SIGNAL_EN int */
	if (!align_buf_alloc) {
		LOG_DBG("cache_inva operation: 0x%x(%dbyte)", (uint32_t)data->data,
			data->block_size * data->blocks);
		DCache_Invalidate((uint32_t)data->data, data->block_size * data->blocks);
	} else {
		LOG_DBG("cache_inva operation: 0x%x(%dbyte)", (uint32_t)align_buf,
			data->block_size * data->blocks);
		DCache_Invalidate((uint32_t)align_buf, data->block_size * data->blocks);
		memcpy(data->data, align_buf, data->block_size * data->blocks);
	}

	return 0;
}

static void sdhc_ameba_read_resp(const struct sdhc_ameba_config *config, struct sdhc_command *cmd)
{
	cmd->response[0] = SDIO_GetResponse(config->hsd->Instance, SDIO_RESP0);
	cmd->response[1] = SDIO_GetResponse(config->hsd->Instance, SDIO_RESP1);
	cmd->response[2] = SDIO_GetResponse(config->hsd->Instance, SDIO_RESP2);
	cmd->response[3] = SDIO_GetResponse(config->hsd->Instance, SDIO_RESP3);

	if (cmd->opcode == SD_ALL_SEND_CID || cmd->opcode == SD_SEND_CSD) {
		cmd->response[3] = (cmd->response[3] << 8) | ((cmd->response[2] >> 24) & 0xFF);
		cmd->response[2] = (cmd->response[2] << 8) | ((cmd->response[1] >> 24) & 0xFF);
		cmd->response[1] = (cmd->response[1] << 8) | ((cmd->response[0] >> 24) & 0xFF);
		cmd->response[0] = cmd->response[0] << 8;
	}
}

static int sdhc_ameba_request(const struct device *dev, struct sdhc_command *cmd,
			      struct sdhc_data *data)
{
	LOG_DBG("Send CMD%d", cmd->opcode);

	int res = 0;
	struct sdhc_ameba_data *dev_data = dev->data;
	const struct sdhc_ameba_config *config = dev->config;
	SDIO_CmdInitTypeDef sdmmc_cmdinit;
	SDIO_DataInitTypeDef data_config;
	uint8_t align_buf[512] __aligned(CACHE_LINE_SIZE);
	bool align_buf_alloc = false;

	__ASSERT_NO_MSG(cmd != NULL);

	if (config->hsd->State != SD_STATE_READY) {
		LOG_ERR("SDHC is busy");
		return -ETIMEDOUT;
	}

	if (k_mutex_lock(&dev_data->bus_mutex, K_MSEC(cmd->timeout_ms))) {
		return -EBUSY;
	}

	(void)pm_device_runtime_get(dev);

	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	sdmmc_cmdinit.CmdIndex = cmd->opcode;
	sdmmc_cmdinit.Argument = cmd->arg;
	/* Native response types (lower 4 bits) */
	sdmmc_cmdinit.RespType = cmd->response_type & 0xF;
	if (cmd->opcode != SD_STOP_TRANSMISSION) {
		sdmmc_cmdinit.CmdType = SDMMC_CMD_NORMAL;
	} else {
		sdmmc_cmdinit.CmdType = SDMMC_CMD_ABORT;
	}
	if (!data) {
		sdmmc_cmdinit.DataPresent = SDIO_TRANS_NO_DATA;
	} else {
		sdmmc_cmdinit.DataPresent = SDIO_TRANS_WITH_DATA;
	}

	if (data) {
		data_config.AutoCmdEn = SDIO_TRANS_AUTO_DIS;
		data_config.BlockSize = data->block_size;
		data_config.BlockCnt = data->blocks;
		if (data->blocks > 1) {
			data_config.TransType = SDIO_TRANS_MULTI_BLK;
		} else {
			data_config.TransType = SDIO_TRANS_SINGLE_BLK;
		}
		data_config.DmaEn = SDIO_TRANS_DMA_EN;

		res = sdhc_ameba_set_data_dir(cmd, &data_config);
		if (res != 0) {
			LOG_ERR("CMD%d Failed!", cmd->opcode);
			goto out;
		}

		LOG_DBG("CACHE_LINE_SIZE %d", CACHE_LINE_SIZE);
		if ((((uintptr_t)data->data) & (CACHE_LINE_SIZE - 1)) != 0) {
			align_buf_alloc = true;
			if (data->block_size * data->blocks > 512) {
				LOG_ERR("buf size over 512!!!");
				res = -EINVAL;
				goto out;
			}
			DCache_CleanInvalidate((uint32_t)align_buf,
					       data->block_size * data->blocks);
			memcpy(align_buf, data->data, data->block_size * data->blocks);
		} else {
			DCache_CleanInvalidate((uint32_t)data->data,
					       data->block_size * data->blocks);
		}

		/* SD_TransPreCheck */
		/* disable all the normal int without err int */
		SDIO_ConfigNormIntSig(config->hsd->Instance, 0xFFFFFFFFU, DISABLE);
		/* clear old flags */
		SDIO_ClearNormSts(config->hsd->Instance, SDIO_GetNormSts(config->hsd->Instance));

		config->hsd->ErrorCode = SD_ERROR_NONE;
		config->hsd->State = SD_STATE_BUSY;
		config->hsd->Context = SD_CONTEXT_DMA;
		if (cmd->opcode == SD_READ_MULTIPLE_BLOCK) { /* CMD18 */
			config->hsd->Context |= SD_CONTEXT_READ_MULTIPLE_BLOCK;
		} else if (cmd->opcode == SD_WRITE_MULTIPLE_BLOCK) { /* CMD25 */
			config->hsd->Context |= SD_CONTEXT_WRITE_MULTIPLE_BLOCK;
		}

		SDIO_ConfigData(config->hsd->Instance, &data_config);
		if (!align_buf_alloc) {
			res = SDIO_ConfigDMA(config->hsd->Instance, SDIO_SDMA_MODE,
					     (uint32_t)data->data);
		} else {
			res = SDIO_ConfigDMA(config->hsd->Instance,
					     SDIO_SDMA_MODE, (uint32_t)align_buf);
		}
		if (res != 0) {
			LOG_ERR("CMD%d Failed!", cmd->opcode);
			goto out;
		}

		SD_PreDMATrans(config->hsd);
	}

	SDIO_SendCommand(config->hsd->Instance, &sdmmc_cmdinit);
	res = SDIO_WaitResp(config->hsd->Instance, sdmmc_cmdinit.RespType, cmd->timeout_ms * 1000);
	if (res == 0 && data) {

		res = sdhc_ameba_finish_data_transfer(config, data, align_buf, align_buf_alloc);
	}

	if (res != 0) {
		LOG_ERR("CMD%d Failed!", cmd->opcode);
	} else {
		sdhc_ameba_read_resp(config, cmd);
	}

out:
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(dev);
	k_mutex_unlock(&dev_data->bus_mutex);

	return res;
}

static int sdhc_ameba_get_card_present(const struct device *dev)
{
	int res = 0;
	struct sdhc_ameba_data *dev_data = dev->data;
	const struct sdhc_ameba_config *config = dev->config;

	/* If a CD pin is configured, use it for card detection */
	if (config->cd_gpio.port != NULL) {
		res = gpio_pin_get_dt(&config->cd_gpio);
		return res;
	}

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	k_mutex_lock(&dev_data->bus_mutex, K_FOREVER);

	/* Card is considered present if the read command did not time out */
	if (SDIO_CmdSendOpCond(config->hsd->Instance, 0) != 0) {
		res = -EIO;
	}
	k_mutex_unlock(&dev_data->bus_mutex);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(dev);

	return res == 0;
}

static int sdhc_ameba_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	struct sdhc_ameba_data *data = dev->data;

	memcpy(props, &data->props, sizeof(struct sdhc_host_props));
	return 0;
}

static int sdhc_ameba_set_io(const struct device *dev, struct sdhc_io *ios)
{
	int res = 0;
	struct sdhc_ameba_data *data = dev->data;
	const struct sdhc_ameba_config *config = dev->config;
	struct sdhc_io *host_io = &data->host_io;
	struct sdhc_host_props *props = &data->props;

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	if ((ios->clock != 0) && (host_io->clock != ios->clock)) {
		if ((ios->clock > props->f_max) || (ios->clock < props->f_min)) {
			LOG_ERR("Invalid clock frequency%d, domain (%u, %u)", ios->clock,
				props->f_min, props->f_max);
			res = -EINVAL;
			goto out;
		}
		if (SDIO_ConfigClock(config->hsd->Instance, (uint32_t)ios->clock / 1000) !=
		    HAL_OK) {
			LOG_ERR("Failed to set clock to %d", ios->clock);
			res = -EIO;
			goto out;
		}
		host_io->clock = ios->clock;
	}

	if (ios->power_mode == SDHC_POWER_OFF) {
		SDIO_PowerState_OFF(config->hsd->Instance);
		k_msleep(data->props.power_delay);
	} else {
		SDIO_PowerState_ON(config->hsd->Instance);
		k_msleep(data->props.power_delay);
	}

	if ((ios->bus_width != 0) && (host_io->bus_width != ios->bus_width)) {
		uint32_t bus_width_reg_value;

		if (ios->bus_width == SDHC_BUS_WIDTH4BIT) {
			bus_width_reg_value = SDIOH_BUS_WIDTH_4BIT;
		} else {
			bus_width_reg_value = SDIOH_BUS_WIDTH_1BIT;
		}

		SDIO_ConfigBusWidth(config->hsd->Instance, bus_width_reg_value);
		host_io->bus_width = ios->bus_width;
	}
out:
	k_mutex_unlock(&data->bus_mutex);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(dev);

	return res;
}

static void sdhc_ameba_event_isr(void *param)
{
	const struct device *dev = (struct device *)param;
	const struct sdhc_ameba_config *config = dev->config;

	(void)SD_IRQHandler((void *)config->hsd);
}

static int sdhc_ameba_init(const struct device *dev)
{
	int ret;
	struct sdhc_ameba_data *data = dev->data;
	const struct sdhc_ameba_config *config = dev->config;

	if (config->cd_gpio.port != NULL) {
		if (!device_is_ready(config->cd_gpio.port)) {
			LOG_ERR("Card detect GPIO device not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->cd_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Couldn't configure card-detect pin; (%d)", ret);
			return ret;
		}
	}

	ret = sdhc_ameba_activate(dev);
	if (ret != 0) {
		LOG_ERR("Clock and GPIO could not be initialized for the SDHC module, err=%d", ret);
		return ret;
	}

	ret = sdhc_ameba_sd_init(dev);
	if (ret != 0) {
		LOG_ERR("SDIO Init Failed");
		return ret;
	}

	config->hsd->State = SD_STATE_READY;

	sdhc_ameba_init_props(dev);

	/* register interrupt handler and enable interrupt */
	config->irq_config_func();

	k_mutex_init(&data->bus_mutex);

	return ret;
}

static DEVICE_API(sdhc, sdhc_ameba_driver_api) = {
	.reset = sdhc_ameba_reset,
	.request = sdhc_ameba_request,
	.set_io = sdhc_ameba_set_io,
	.get_card_present = sdhc_ameba_get_card_present,
	.card_busy = sdhc_ameba_card_busy,
	.get_host_props = sdhc_ameba_get_host_props,
};

#define AMEBA_SDHC_IRQ_HANDLER(index)                                                              \
	static void sdhc_ameba_irq_config_func_##index(void)                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority),                     \
			    sdhc_ameba_event_isr, DEVICE_DT_INST_GET(index), 0);                   \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}

#define SDHC_AMEBA_INIT(index)                                                                     \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	AMEBA_SDHC_IRQ_HANDLER(index)                                                              \
	static SD_HdlTypeDef hsd_##index;                                                          \
	static const struct sdhc_ameba_config sdhc_ameba_config_##index = {                        \
		.hsd = &hsd_##index,                                                               \
		.reg_addr = DT_INST_REG_ADDR(index),                                               \
		.irq_config_func = sdhc_ameba_irq_config_func_##index,                             \
		.bus_width = DT_INST_PROP(index, bus_width),                                       \
		.power_delay_ms = DT_INST_PROP(index, power_delay_ms),                             \
		.min_freq = DT_INST_PROP(index, min_bus_freq),                                     \
		.max_freq = DT_INST_PROP(index, max_bus_freq),                                     \
		.cd_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(index), cd_gpios, {0}),                 \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                            \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(index, idx),           \
	};                                                                                         \
	static struct sdhc_ameba_data sdhc_ameba_data_##index;                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, sdhc_ameba_init, NULL, &sdhc_ameba_data_##index,              \
			      &sdhc_ameba_config_##index, POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY,  \
			      &sdhc_ameba_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_AMEBA_INIT)
