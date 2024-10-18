/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_sdmmc

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/interrupt_controller/intc_ra_icu.h>

/* Renesas include */
#include "sdmmc_renesas_ra.h"
#include "r_sdhi.h"
#include "r_sdmmc_api.h"
#include "r_dtc.h"

LOG_MODULE_REGISTER(sdmmc_renesas_ra, CONFIG_SDMMC_LOG_LEVEL);

void sdhimmc_accs_isr(void);
void sdhimmc_card_isr(void);
void sdhimmc_dma_req_isr(void);

static volatile struct sdmmc_ra_event sdmmc_event = {
	.transfer_completed = false,
	.transfer_error = false,
};

struct ra_sdmmc_config {
	const struct pinctrl_dev_config *pcfg;
};

struct ra_sdmmc_priv {
	struct st_sdmmc_instance_ctrl sdmmc_ctrl;
	struct st_sdmmc_cfg fsp_config;
	struct gpio_dt_spec sdhi_en;
	uint8_t channel;
	struct k_sem thread_lock;
	uint8_t status;
	/* Transfer DTC */
	struct st_transfer_instance transfer;
	struct st_dtc_instance_ctrl transfer_ctrl;
	struct st_transfer_info transfer_info;
	struct st_transfer_cfg transfer_cfg;
	struct st_dtc_extended_cfg transfer_cfg_extend;
};

static inline int fsp_err_to_errno(fsp_err_t fsp_err)
{
	switch (fsp_err) {
	case FSP_ERR_INVALID_ARGUMENT:
		return -EINVAL;
	case FSP_ERR_NOT_OPEN:
		return -EIO;
	case FSP_ERR_IN_USE:
		return -EBUSY;
	case FSP_ERR_UNSUPPORTED:
		return -ENOTSUP;
	case 0:
		return 0;
	default:
		return -EINVAL;
	}
}

/* Delay function for SD subsystem with micro second*/
static inline void sdmmc_delay(unsigned int delay)
{
	k_usleep(delay);
}

/* The callback is called when a transfer completes. */
void r_sdhi_callback(sdmmc_callback_args_t *p_args)
{
	switch (p_args->event) {
	case SDMMC_EVENT_TRANSFER_COMPLETE:
		sdmmc_event.transfer_completed = true;
		break;
	default:
		break;
	}
}

/* Waits for SDMMC card to be ready for data. Returns 0 if card is ready */
int ra_sdmmc_wait_ready(const struct device *dev, int timeout)
{
	struct ra_sdmmc_priv *priv = dev->data;
	sdmmc_status_t status;

	while (timeout > 0) {
		/* Wait for transfer. */
		R_SDHI_StatusGet(&priv->sdmmc_ctrl, &status);
		if (!status.transfer_in_progress) {
			return 0;
		}
		/* Delay 125us before polling again */
		sdmmc_delay(125);
		timeout -= 125;
	}
	LOG_ERR("Card busy");
	return -EBUSY;
}
static int ra_media_init(const struct device *dev)
{
	struct ra_sdmmc_priv *priv = dev->data;
	fsp_err_t fsp_err;
	/* Initialize the SD card.  This should not be done until the card is plugged in for SD
	 * devices.
	 */
	fsp_err = R_SDHI_MediaInit(&priv->sdmmc_ctrl, NULL);
	if (FSP_SUCCESS != fsp_err) {
		LOG_INF("\r\nError in initializing R_SDHI_MediaInit: %d\r\n ", fsp_err);
		return -EIO; /* I/O error*/
	}
	return 0;
}
static int ra_sdmmc_access_init(struct disk_info *disk)
{
	const struct device *dev = disk->dev;
	struct ra_sdmmc_priv *priv = dev->data;
	struct st_sdmmc_instance_ctrl *p_ctrl = &priv->sdmmc_ctrl;
	fsp_err_t fsp_err;
	int ret = 0;
	int timeout = SDHI_PRV_ACCESS_TIMEOUT_US;
	sdmmc_status_t status;

	if ((uint32_t)(SDHI_PRV_OPEN) != p_ctrl->open) {
		fsp_err = R_SDHI_Open(&priv->sdmmc_ctrl, &priv->fsp_config);
	}

	fsp_err = R_SDHI_StatusGet(&priv->sdmmc_ctrl, &status);

	if (fsp_err != FSP_SUCCESS && !status.card_inserted) {
		priv->status = DISK_STATUS_NOMEDIA;
		LOG_INF("I/O error: %d", fsp_err);
		ret = -ENODEV; /* I/O error*/
		goto end;
	}
	k_sem_take(&priv->thread_lock, K_USEC(timeout));
	ret = ra_media_init(dev);

	if (ret < 0) {
		priv->status = DISK_STATUS_NOMEDIA;
		LOG_ERR("failed to init sdmmc media");
		goto end;
	}
	ra_sdmmc_wait_ready(dev, timeout);
	sdmmc_event.transfer_completed = false;

	/* Check if card is initial. */
	R_SDHI_StatusGet(&priv->sdmmc_ctrl, &status);
	if (!status.initialized) {
		priv->status = DISK_STATUS_UNINIT;
		ret = -EIO; /* I/O error*/
		goto end;
	}
	priv->status = DISK_STATUS_OK;
end:
	k_sem_give(&priv->thread_lock);
	return ret;
}

static int ra_sdmmc_access_deinit(struct ra_sdmmc_priv *priv)
{
	R_SDHI_Close(&priv->sdmmc_ctrl);

	priv->status = DISK_STATUS_UNINIT;
	return 0;
}

static int ra_sdmmc_access_status(struct disk_info *disk)
{
	const struct device *dev = disk->dev;
	struct ra_sdmmc_priv *priv = dev->data;

	return priv->status;
}

static int ra_sdmmc_access_read(struct disk_info *disk, uint8_t *data_buf, uint32_t start_sector,
				uint32_t num_sector)
{
	const struct device *dev = disk->dev;
	struct ra_sdmmc_priv *priv = dev->data;
	int timeout = SDHI_PRV_DATA_TIMEOUT_US * num_sector;
	fsp_err_t fsp_err;
	int ret = 0;

	if (start_sector + num_sector > priv->sdmmc_ctrl.device.sector_count) {
		ret = -EINVAL;
		goto end; /* I/O error*/
	}
	k_sem_take(&priv->thread_lock, K_USEC(timeout));

	fsp_err = R_SDHI_Read(&priv->sdmmc_ctrl, data_buf, start_sector, num_sector);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("R_SDHI_Read error: %d", fsp_err);
		ret = -EIO;
		goto end; /* I/O error*/
	}

	/* Verify card is back in transfer state after write */
	while (!sdmmc_event.transfer_completed) {
		/* Wait for transfer. */
		k_sleep(K_USEC(10));
	}
	sdmmc_event.transfer_completed = false;

end:
	k_sem_give(&priv->thread_lock);
	return ret;
}

static int ra_sdmmc_access_write(struct disk_info *disk, const uint8_t *buf, uint32_t start_sector,
				 uint32_t num_sector)
{
	const struct device *dev = disk->dev;
	struct ra_sdmmc_priv *priv = dev->data;
	int timeout = SDHI_PRV_DATA_TIMEOUT_US;
	int ret = 0;

	if (start_sector + num_sector > priv->sdmmc_ctrl.device.sector_count) {
		ret = -EINVAL;
		goto end; /* I/O error*/
	}
	k_sem_take(&priv->thread_lock, K_USEC(timeout));
	fsp_err_t fsp_err = R_SDHI_Write(&priv->sdmmc_ctrl, buf, start_sector, num_sector);

	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("R_SDHI_Write error: %d", fsp_err);
		ret = -EIO;
		goto end; /* I/O error*/
	}

	while (!sdmmc_event.transfer_completed) {
		/* Wait for transfer. */
		k_sleep(K_USEC(10));
	}
	sdmmc_event.transfer_completed = false;

end:
	k_sem_give(&priv->thread_lock);
	return ret;
}

static int ra_sdmmc_access_ioctl(struct disk_info *disk, uint8_t cmd, void *buf)
{
	const struct device *dev = disk->dev;
	struct ra_sdmmc_priv *priv = dev->data;

	int ret = 0;

	struct s_sdmmc_device *media_device = &priv->sdmmc_ctrl.device;

	switch (cmd) {
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(uint32_t *)buf = media_device->sector_count;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(uint32_t *)buf = media_device->sector_size_bytes;
		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		*(uint32_t *)buf = media_device->erase_sector_count;
		break;
	case DISK_IOCTL_CTRL_SYNC:
		break;
	case DISK_IOCTL_CTRL_INIT:
		return ra_sdmmc_access_init(disk);
	case DISK_IOCTL_CTRL_DEINIT:
		return ra_sdmmc_access_deinit(priv);
	default:
		ret = -EINVAL;
	}

	return ret;
}

static void ra_sdmmc_accs_isr(const void *parameter)
{
	sdhimmc_accs_isr();
}

static void ra_sdmmc_card_isr(const void *parameter)
{
	sdhimmc_card_isr();
}

static void ra_sdmmc_dma_req_isr(const void *parameter)
{
	sdhimmc_dma_req_isr();
}

static const struct disk_operations ra_sdmmc_ops = {
	.init = ra_sdmmc_access_init,
	.status = ra_sdmmc_access_status,
	.read = ra_sdmmc_access_read,
	.write = ra_sdmmc_access_write,
	.ioctl = ra_sdmmc_access_ioctl,
};

static struct disk_info ra_sdmmc_info = {
	.name = CONFIG_SDMMC_VOLUME_NAME,
	.ops = &ra_sdmmc_ops,
};

static int sdmmc_ra_init(const struct device *dev)
{
	const struct ra_sdmmc_config *config = dev->config;
	struct ra_sdmmc_priv *priv = dev->data;
	fsp_err_t fsp_err;
	int ret = 0;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
	if (priv->sdhi_en.port != NULL) {
		int err = gpio_pin_configure_dt(&priv->sdhi_en, GPIO_OUTPUT_HIGH);

		if (err) {
			return err;
		}
		k_sleep(K_MSEC(50));
	}

	k_sem_init(&priv->thread_lock, 1, 1);
	fsp_err = R_SDHI_Open(&priv->sdmmc_ctrl, &priv->fsp_config);

	if (fsp_err != FSP_SUCCESS) {
		LOG_INF("\r\nR_SDHI_Open error: %d\r\n", fsp_err);
		return -EIO; /* I/O error*/
	}
	sdmmc_delay(SDHI_PRV_ACCESS_TIMEOUT_US);
	ra_sdmmc_info.dev = dev;
	ret = disk_access_register(&ra_sdmmc_info);
	return ret;
}

#define _ELC_EVENT_SDMMC_ACCS(channel)    ELC_EVENT_SDHIMMC##channel##_ACCS
#define _ELC_EVENT_SDMMC_CARD(channel)    ELC_EVENT_SDHIMMC##channel##_CARD
#define _ELC_EVENT_SDMMC_DMA_REQ(channel) ELC_EVENT_SDHIMMC##channel##_DMA_REQ

#define ELC_EVENT_SDMMC_ACCS(channel)    _ELC_EVENT_SDMMC_ACCS(channel)
#define ELC_EVENT_SDMMC_CARD(channel)    _ELC_EVENT_SDMMC_CARD(channel)
#define ELC_EVENT_SDMMC_DMA_REQ(channel) _ELC_EVENT_SDMMC_DMA_REQ(channel)

#define RA_SDMMC_IRQ_CONFIG_INIT(index)                                                            \
	do {                                                                                       \
		ARG_UNUSED(dev);                                                                   \
                                                                                                   \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, accs, irq)] =                              \
			ELC_EVENT_SDMMC_ACCS(DT_INST_PROP(index, channel));                        \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, card, irq)] =                              \
			ELC_EVENT_SDMMC_CARD(DT_INST_PROP(index, channel));                        \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, dma_req, irq)] =                           \
			ELC_EVENT_SDMMC_DMA_REQ(DT_INST_PROP(index, channel));                     \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, accs, irq),                                 \
			    DT_INST_IRQ_BY_NAME(index, accs, priority), ra_sdmmc_accs_isr,         \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, card, irq),                                 \
			    DT_INST_IRQ_BY_NAME(index, card, priority), ra_sdmmc_card_isr,         \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, dma_req, irq),                              \
			    DT_INST_IRQ_BY_NAME(index, dma_req, priority), ra_sdmmc_dma_req_isr,   \
			    DEVICE_DT_INST_GET(index), 0);                                         \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(index, accs, irq));                                 \
		irq_enable(DT_INST_IRQ_BY_NAME(index, card, irq));                                 \
		irq_enable(DT_INST_IRQ_BY_NAME(index, dma_req, irq));                              \
	} while (0)

#define RA_SDHI_EN(index) .sdhi_en = GPIO_DT_SPEC_INST_GET_OR(index, enable_gpios, {0})

#define RA_SDMMC_DTC_INIT(index)                                                                   \
	ra_sdmmc_priv_##index.fsp_config.p_lower_lvl_transfer = &ra_sdmmc_priv_##index.transfer;

#define RA_SDMMC_DTC_STRUCT_INIT(index)                                                            \
	.transfer_info =                                                                           \
		{                                                                                  \
			.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,       \
			.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE,       \
			.transfer_settings_word_b.irq = TRANSFER_IRQ_END,                          \
			.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,       \
			.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,  \
			.transfer_settings_word_b.size = TRANSFER_SIZE_4_BYTE,                     \
			.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL,                     \
			.p_dest = (void *)NULL,                                                    \
			.p_src = (void const *)NULL,                                               \
			.num_blocks = 0,                                                           \
			.length = 128,                                                             \
	},                                                                                         \
	.transfer_cfg_extend = {.activation_source = DT_INST_IRQ_BY_NAME(index, dma_req, irq)},    \
	.transfer_cfg =                                                                            \
		{                                                                                  \
			.p_info = &ra_sdmmc_priv_##index.transfer_info,                            \
			.p_extend = &ra_sdmmc_priv_##index.transfer_cfg_extend,                    \
	},                                                                                         \
	.transfer = {                                                                              \
		.p_ctrl = &ra_sdmmc_priv_##index.transfer_ctrl,                                    \
		.p_cfg = &ra_sdmmc_priv_##index.transfer_cfg,                                      \
		.p_api = &g_transfer_on_dtc,                                                       \
	},

#define RA_SDMMC_INIT(index)                                                                       \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct ra_sdmmc_config ra_sdmmc_config_##index = {                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
	};                                                                                         \
                                                                                                   \
	static struct ra_sdmmc_priv ra_sdmmc_priv_##index = {                                      \
		.fsp_config =                                                                      \
			{                                                                          \
				.channel = DT_INST_PROP(index, channel),                           \
				.bus_width = DT_INST_PROP(index, bus_width),                       \
				.access_ipl = DT_INST_IRQ_BY_NAME(index, accs, priority),          \
				.access_irq = DT_INST_IRQ_BY_NAME(index, accs, irq),               \
				.card_ipl = DT_INST_IRQ_BY_NAME(index, card, priority),            \
				.card_irq = DT_INST_IRQ_BY_NAME(index, card, irq),                 \
				.dma_req_ipl = DT_INST_IRQ_BY_NAME(index, dma_req, priority),      \
				.dma_req_irq = DT_INST_IRQ_BY_NAME(index, dma_req, irq),           \
				.p_context = NULL,                                                 \
				.p_callback = r_sdhi_callback,                                     \
				.block_size = DT_INST_PROP(index, block_size),                     \
				.card_detect = DT_INST_PROP(index, card_detect),                   \
				.write_protect = DT_INST_PROP(index, write_protect),               \
				.p_extend = NULL,                                                  \
				.p_lower_lvl_transfer = &ra_sdmmc_priv_##index.transfer,           \
				.sdio_ipl = BSP_IRQ_DISABLED,                                      \
				.sdio_irq = FSP_INVALID_VECTOR,                                    \
			},                                                                         \
		RA_SDHI_EN(index),                                                                 \
		RA_SDMMC_DTC_STRUCT_INIT(index)};                                                  \
                                                                                                   \
	static int sdmmc_ra_init##index(const struct device *dev)                                  \
	{                                                                                          \
		RA_SDMMC_DTC_INIT(index);                                                          \
		RA_SDMMC_IRQ_CONFIG_INIT(index);                                                   \
		int err = sdmmc_ra_init(dev);                                                      \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, sdmmc_ra_init##index, NULL, &ra_sdmmc_priv_##index,           \
			      &ra_sdmmc_config_##index, POST_KERNEL, CONFIG_SD_INIT_PRIORITY,      \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(RA_SDMMC_INIT)
