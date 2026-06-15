/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SDHC emulator framework.
 *
 * Provides a generic emulation layer for SDHC controllers and cards.
 * Controllers discover cards via the emul framework, and cards expose
 * their functionality through the sdhc_emul_api interface.
 */

#define DT_DRV_COMPAT zephyr_sdhc_emul

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/sdhc/sdhc_emul.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "sdhc_emul_types.h"
#include "sdhc_emul_core.h"

#ifdef CONFIG_SDHC_EMUL_BACKEND_FILE
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

LOG_MODULE_REGISTER(sdhc_emul, CONFIG_SDHC_LOG_LEVEL);

/* Forward declarations */
static int sdhc_emul_init(const struct device *dev);

static int sdhc_emul_backend_init(struct sdhc_emul_data *data, const struct device *dev)
{
	struct sdhc_emul_card *card = &data->card;

#ifdef CONFIG_SDHC_EMUL_BACKEND_FILE
	size_t size = card->n_blocks * card->block_size;
	int fd = open(CONFIG_SDHC_EMUL_BACKEND_FILE_PATH, O_RDWR | O_CREAT, 0666);

	if (fd < 0) {
		LOG_ERR("Failed to open backing file %s", CONFIG_SDHC_EMUL_BACKEND_FILE_PATH);
		return -EIO;
	}
	if (ftruncate(fd, size) < 0) {
		close(fd);
		return -EIO;
	}
	card->storage = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd); /* mmap keeps a reference */
	if (card->storage == MAP_FAILED) {
		LOG_ERR("Failed to mmap backing file");
		return -ENOMEM;
	}
#else
	/* Storage is statically allocated per instance (see SDHC_EMUL_DEFINE) */
	if (!card->storage) {
		LOG_ERR("Static storage not assigned");
		return -ENOMEM;
	}
#endif
	return 0;
}

/* API implementation */
static int sdhc_emul_reset(const struct device *dev)
{
	struct sdhc_emul_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->card.state = SDHC_EMUL_STATE_IDLE;
	data->card.rca = 0;
	k_mutex_unlock(&data->lock);

	return 0;
}

static int sdhc_emul_request(const struct device *dev, struct sdhc_command *cmd,
			     struct sdhc_data *data)
{
	struct sdhc_emul_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = sdhc_emul_core_request(&data->card, cmd, data);
	k_mutex_unlock(&data->lock);
	if (ret < 0) {
		LOG_ERR("CMD%d failed: %d", cmd->opcode, ret);
	}
	return ret;
}

static int sdhc_emul_set_io(const struct device *dev, struct sdhc_io *ios)
{
	return 0;
}

static int sdhc_emul_get_card_present(const struct device *dev)
{
	struct sdhc_emul_data *data = dev->data;

	return data->card.card_present ? 1 : 0;
}

static int sdhc_emul_execute_tuning(const struct device *dev)
{
	LOG_DBG("Tuning executed");
	return 0;
}

static int sdhc_emul_card_busy(const struct device *dev)
{
	return 0;
}

static int sdhc_emul_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	struct sdhc_emul_data *data = dev->data;

	memset(props, 0, sizeof(*props));

	props->f_min = 400000;
	if (data->card.type == SDHC_EMUL_TYPE_EMMC) {
		props->f_max = 200000000;
		props->bus_4_bit_support = true;
		props->host_caps.bus_8_bit_support = 1;
		props->hs200_support = true;
		props->hs400_support = true;
	} else {
		props->f_max = 25000000;
		props->bus_4_bit_support = true;
		props->host_caps.high_spd_support = 1;
	}
	props->host_caps.vol_330_support = 1;
	return 0;
}

static int sdhc_emul_enable_interrupt(const struct device *dev,
				      sdhc_interrupt_cb_t callback, int sources,
				      void *user_data)
{
	struct sdhc_emul_data *data = dev->data;

	data->card.irq_cb = callback;
	data->card.irq_sources = sources;
	data->card.irq_user_data = user_data;
	return 0;
}

static int sdhc_emul_disable_interrupt(const struct device *dev, int sources)
{
	struct sdhc_emul_data *data = dev->data;

	data->card.irq_sources &= ~sources;
	if (data->card.irq_sources == 0) {
		data->card.irq_cb = NULL;
		data->card.irq_user_data = NULL;
	}
	return 0;
}

/* Public Test Accessors */
void sdhc_emul_set_fault(const struct device *dev, uint8_t cmd_index)
{
	struct sdhc_emul_data *data = dev->data;

	data->card.inject_cmd = cmd_index;
}

void sdhc_emul_clear_fault(const struct device *dev)
{
	struct sdhc_emul_data *data = dev->data;

	data->card.inject_cmd = 255;
}

void sdhc_emul_set_card_present(const struct device *dev, bool present)
{
	struct sdhc_emul_data *data = dev->data;

	data->card.card_present = present;
}

void sdhc_emul_trigger_sdio_irq(const struct device *dev, uint8_t fn)
{
	struct sdhc_emul_data *data = dev->data;

	if (data->card.irq_cb && (data->card.irq_sources & SDHC_INT_SDIO)) {
		data->card.irq_cb(dev, SDHC_INT_SDIO, data->card.irq_user_data);
	}
}

uint8_t *sdhc_emul_get_storage(const struct device *dev)
{
	struct sdhc_emul_data *data = dev->data;

	return data->card.storage;
}

uint32_t sdhc_emul_get_block_count(const struct device *dev)
{
	struct sdhc_emul_data *data = dev->data;

	return data->card.n_blocks;
}

static DEVICE_API(sdhc, sdhc_emul_api) = {
	.reset = sdhc_emul_reset,
	.request = sdhc_emul_request,
	.set_io = sdhc_emul_set_io,
	.get_card_present = sdhc_emul_get_card_present,
	.execute_tuning = sdhc_emul_execute_tuning,
	.card_busy = sdhc_emul_card_busy,
	.get_host_props = sdhc_emul_get_host_props,
	.enable_interrupt = sdhc_emul_enable_interrupt,
	.disable_interrupt = sdhc_emul_disable_interrupt,
};

static int sdhc_emul_init(const struct device *dev)
{
	struct sdhc_emul_data *data = dev->data;
	const struct sdhc_emul_cfg *cfg = dev->config;

	k_mutex_init(&data->lock);

	data->card.type = cfg->card_type;
	data->card.n_blocks = cfg->n_blocks;
	data->card.block_size = cfg->block_size;
	data->card.inject_cmd = cfg->inject_cmd;
	data->card.sdio_fn_count = cfg->sdio_fn_cnt;
	data->card.card_present = cfg->card_present;
	data->card.ocr = cfg->host_ocr;

	sdhc_emul_core_build_cid(&data->card, 1);
	sdhc_emul_core_build_csd(&data->card);
	sdhc_emul_core_build_ext_csd(&data->card);
	sdhc_emul_core_init_sdio_regs(&data->card);

	return sdhc_emul_backend_init(data, dev);
}

#define SDHC_EMUL_STORAGE_SIZE(n)                                        \
	(DT_INST_PROP_OR(n, capacity_blocks, 131072) *                   \
	 DT_INST_PROP_OR(n, sector_size, 512))

#define SDHC_EMUL_DEFINE(n)                                              \
	static uint8_t sdhc_emul_storage_##n                             \
		[SDHC_EMUL_STORAGE_SIZE(n)] __aligned(4);                \
	static struct sdhc_emul_data sdhc_emul_data_##n = {              \
		.card.storage = sdhc_emul_storage_##n,                   \
	};                                                               \
	static const struct sdhc_emul_cfg sdhc_emul_cfg_##n = {          \
		.card_type    = DT_INST_ENUM_IDX_OR(n, card_type,        \
						    SDHC_EMUL_TYPE_SDHC),\
		.n_blocks     = DT_INST_PROP_OR(n, capacity_blocks, 131072),\
		.block_size   = DT_INST_PROP_OR(n, sector_size, 512),    \
		.inject_cmd   = DT_INST_PROP_OR(n, inject_error_cmd, 255),\
		.sdio_fn_cnt  = DT_INST_PROP_OR(n, sdio_func_count, 1),  \
		.card_present = DT_INST_NODE_HAS_PROP(n, card_inserted), \
		.host_ocr     = DT_INST_PROP_OR(n, host_ocr, 0x00FF8000),\
	};                                                               \
	DEVICE_DT_INST_DEFINE(n, sdhc_emul_init, NULL,                   \
		&sdhc_emul_data_##n, &sdhc_emul_cfg_##n,                 \
		POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &sdhc_emul_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_EMUL_DEFINE)
