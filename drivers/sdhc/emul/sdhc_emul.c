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
	data->card.state = SDHC_STATE_IDLE;
	data->card.rca = 0;
	k_mutex_unlock(&data->lock);

	return 0;
}

static int sdhc_emul_request(const struct device *dev, struct sdhc_command *cmd,
			     struct sdhc_data *data)
{
	struct sdhc_emul_data *emul_data = dev->data;
	int ret;

	k_mutex_lock(&emul_data->lock, K_FOREVER);
	ret = sdhc_emul_core_request(&emul_data->card, cmd, data);
	k_mutex_unlock(&emul_data->lock);
	if (ret < 0) {
		LOG_ERR("CMD%d failed: %d", cmd->opcode, ret);
	}
	return ret;
}

static int sdhc_emul_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct sdhc_emul_data *data = dev->data;

	LOG_DBG("%s: set_io clock=%dHz bus_width=%d timing=%d voltage=%d power=%d",
		dev->name, ios->clock, ios->bus_width, ios->timing,
		ios->signal_voltage, ios->power_mode);

	k_mutex_lock(&data->lock, K_FOREVER);
	if (ios->bus_width != 0) {
		data->card.bus_width = (uint8_t)ios->bus_width;
	}
	data->card.hs200_mode = (ios->timing == SDHC_TIMING_HS200);
	data->card.hs400_mode = (ios->timing == SDHC_TIMING_HS400);
	k_mutex_unlock(&data->lock);

	return 0;
}

static int sdhc_emul_get_card_present(const struct device *dev)
{
	struct sdhc_emul_data *data = dev->data;

	LOG_DBG("%s: get_card_present -> %d", dev->name, data->card.card_present);

	return data->card.card_present ? 1 : 0;
}

static int sdhc_emul_execute_tuning(const struct device *dev)
{
	LOG_DBG("%s: tuning executed", dev->name);
	return 0;
}

static int sdhc_emul_card_busy(const struct device *dev)
{
	struct sdhc_emul_data *data = dev->data;
	int busy;

	k_mutex_lock(&data->lock, K_FOREVER);
	busy = (data->card.state == SDHC_STATE_PROGRAMMING) ? 1 : 0;
	k_mutex_unlock(&data->lock);

	if (busy) {
		LOG_DBG("%s: card_busy -> 1 (programming)", dev->name);
	}
	return busy;
}

static int sdhc_emul_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	const struct sdhc_emul_cfg *cfg = dev->config;

	memset(props, 0, sizeof(*props));

	props->f_min = cfg->f_min;
	props->f_max = cfg->f_max;
	props->power_delay = cfg->power_delay_ms;

	props->host_caps.high_spd_support = cfg->high_speed ? 1 : 0;
	props->host_caps.bus_8_bit_support = (cfg->bus_width >= 8) ? 1 : 0;
	props->host_caps.vol_330_support = cfg->voltage_330 ? 1 : 0;
	props->host_caps.vol_180_support = cfg->voltage_180 ? 1 : 0;

	props->bus_4_bit_support = (cfg->bus_width >= 4);
	props->hs200_support = cfg->hs200;
	props->hs400_support = cfg->hs400;
	props->hs400_enhanced_strobe_support = cfg->enhanced_strobe;
	props->is_spi = false;

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

	LOG_DBG("%s: inject fault on CMD%u (was CMD%u)", dev->name, cmd_index,
		data->card.inject_cmd);
	data->card.inject_cmd = cmd_index;
}

void sdhc_emul_clear_fault(const struct device *dev)
{
	struct sdhc_emul_data *data = dev->data;

	LOG_DBG("%s: clear fault (was CMD%u)", dev->name, data->card.inject_cmd);
	data->card.inject_cmd = 255;
}

void sdhc_emul_set_card_present(const struct device *dev, bool present)
{
	struct sdhc_emul_data *data = dev->data;

	LOG_DBG("%s: card_present %d -> %d", dev->name, data->card.card_present, present);
	data->card.card_present = present;
}

void sdhc_emul_trigger_sdio_irq(const struct device *dev, uint8_t fn)
{
	struct sdhc_emul_data *data = dev->data;

	if (data->card.irq_cb == NULL) {
		LOG_DBG("%s: SDIO IRQ fn%u dropped, no callback registered", dev->name, fn);
		return;
	}
	if ((data->card.irq_sources & SDHC_INT_SDIO) == 0) {
		LOG_DBG("%s: SDIO IRQ fn%u dropped, SDHC_INT_SDIO not enabled (sources 0x%x)",
			dev->name, fn, data->card.irq_sources);
		return;
	}

	LOG_DBG("%s: SDIO IRQ fn%u delivered", dev->name, fn);
	data->card.irq_cb(dev, SDHC_INT_SDIO, data->card.irq_user_data);
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
		.f_min           = DT_INST_PROP(n, min_bus_freq),        \
		.f_max           = DT_INST_PROP(n, max_bus_freq),        \
		.power_delay_ms  = DT_INST_PROP(n, power_delay_ms),      \
		.bus_width       = DT_INST_PROP(n, bus_width),           \
		.high_speed      = DT_INST_PROP(n, high_speed),          \
		.hs200           = DT_INST_PROP(n, mmc_hs200_1_8v),      \
		.hs400           = DT_INST_PROP(n, mmc_hs400_1_8v),      \
		.enhanced_strobe = DT_INST_PROP(n, mmc_hs400_enhanced_strobe), \
		.voltage_330     = DT_INST_PROP(n, voltage_330),         \
		.voltage_180     = DT_INST_PROP(n, voltage_180),         \
	};                                                               \
	DEVICE_DT_INST_DEFINE(n, sdhc_emul_init, NULL,                   \
		&sdhc_emul_data_##n, &sdhc_emul_cfg_##n,                 \
		POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &sdhc_emul_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_EMUL_DEFINE)
