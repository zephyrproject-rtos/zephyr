/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SDHC card peripheral emulator.
 *
 * Emulates SD/SDHC/SDXC/eMMC/SDIO card personalities. Registers as a
 * peripheral on the sdhc-emul-controller bus using the emul framework.
 */

#define DT_DRV_COMPAT zephyr_sdhc_emul_card

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/sdhc_emul.h>
#include <zephyr/drivers/sdhc/sdhc_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>
#include <string.h>

#include "sdhc_emul_types.h"
#include "sdhc_emul_core.h"

LOG_MODULE_REGISTER(sdhc_emul_card, CONFIG_SDHC_LOG_LEVEL);

/* Per-instance emulator data */
struct sdhc_emul_card_data {
	struct sdhc_emul_card card;
	struct k_mutex lock;
};

/* Per-instance config from DTS */
struct sdhc_emul_card_cfg {
	enum sdhc_emul_type card_type;
	uint32_t n_blocks;
	uint16_t block_size;
	uint8_t inject_cmd;
	uint8_t sdio_fn_cnt;
	bool card_present;
	uint32_t host_ocr;
};

static int sdhc_emul_card_request(const struct emul *target,
				  struct sdhc_command *cmd,
				  struct sdhc_data *data)
{
	struct sdhc_emul_card_data *emul_data = target->data;
	struct sdhc_emul_card *card = &emul_data->card;
	int ret;

	k_mutex_lock(&emul_data->lock, K_FOREVER);
	ret = sdhc_emul_core_request(card, cmd, data);
	k_mutex_unlock(&emul_data->lock);
	if (ret < 0) {
		LOG_ERR("CMD%d failed: %d", cmd->opcode, ret);
	}
	return ret;
}

static int sdhc_emul_card_busy(const struct emul *target)
{
	return 0;
}

static int sdhc_emul_card_present(const struct emul *target)
{
	struct sdhc_emul_card_data *data = target->data;

	return data->card.card_present ? 1 : 0;
}

/**
 * Helper to find the first card emul data from a controller device.
 * The controller stores a slist of sdhc_emul nodes. We walk to find
 * the first one and return its card data.
 */
static struct sdhc_emul_card_data *find_card_data(const struct device *dev)
{
	struct sdhc_emul *emul = sdhc_emul_get_card(dev);

	if (emul != NULL && emul->target != NULL) {
		return emul->target->data;
	}

	return NULL;
}

/* Public test-accessor API implementations */
void sdhc_emul_set_fault(const struct device *dev, uint8_t cmd_index)
{
	struct sdhc_emul_card_data *data = find_card_data(dev);

	if (data) {
		data->card.inject_cmd = cmd_index;
	}
}

void sdhc_emul_clear_fault(const struct device *dev)
{
	struct sdhc_emul_card_data *data = find_card_data(dev);

	if (data) {
		data->card.inject_cmd = 255;
	}
}

void sdhc_emul_set_card_present(const struct device *dev, bool present)
{
	struct sdhc_emul_card_data *data = find_card_data(dev);

	if (data) {
		data->card.card_present = present;
	}
}

void sdhc_emul_trigger_sdio_irq(const struct device *dev, uint8_t fn)
{
	ARG_UNUSED(fn);
	sdhc_emul_raise_interrupt(dev, SDHC_INT_SDIO);
}

uint8_t *sdhc_emul_get_storage(const struct device *dev)
{
	struct sdhc_emul_card_data *data = find_card_data(dev);

	if (data) {
		return data->card.storage;
	}
	return NULL;
}

uint32_t sdhc_emul_get_block_count(const struct device *dev)
{
	struct sdhc_emul_card_data *data = find_card_data(dev);

	if (data) {
		return data->card.n_blocks;
	}
	return 0;
}

static const struct sdhc_emul_api sdhc_emul_card_api_instance = {
	.request = sdhc_emul_card_request,
	.card_busy = sdhc_emul_card_busy,
	.card_present = sdhc_emul_card_present,
};

static int sdhc_emul_card_init(const struct emul *target, const struct device *parent)
{
	struct sdhc_emul_card_data *data = target->data;
	const struct sdhc_emul_card_cfg *cfg = target->cfg;

	k_mutex_init(&data->lock);

	data->card.type = cfg->card_type;
	data->card.n_blocks = cfg->n_blocks;
	data->card.block_size = cfg->block_size;
	data->card.inject_cmd = cfg->inject_cmd;
	data->card.sdio_fn_count = cfg->sdio_fn_cnt;
	data->card.card_present = cfg->card_present;
	data->card.ocr = cfg->host_ocr;
	data->card.state = SDHC_EMUL_STATE_IDLE;

	sdhc_emul_core_build_cid(&data->card, 1);
	sdhc_emul_core_build_csd(&data->card);
	sdhc_emul_core_build_ext_csd(&data->card);
	sdhc_emul_core_init_sdio_regs(&data->card);

	/* Storage is statically allocated per instance via the DEFINE macro */

	/* Set the card API on the bus emul node.
	 * target and registration are handled by emul_init_for_bus().
	 */
	struct sdhc_emul *bus_emul = target->bus.sdhc;

	bus_emul->api = &sdhc_emul_card_api_instance;

	return 0;
}

static int sdhc_emul_card_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

#define SDHC_EMUL_CARD_STORAGE_SIZE(n)                                                  \
	(DT_INST_PROP_OR(n, capacity_blocks, 131072) *                                  \
	 DT_INST_PROP_OR(n, sector_size, 512))

#define SDHC_EMUL_CARD_DEFINE(n)                                                        \
	static uint8_t sdhc_emul_card_storage_##n                                       \
		[SDHC_EMUL_CARD_STORAGE_SIZE(n)] __aligned(4);                          \
	static struct sdhc_emul_card_data sdhc_emul_card_data_##n = {                   \
		.card.storage = sdhc_emul_card_storage_##n,                             \
	};                                                                              \
	static const struct sdhc_emul_card_cfg sdhc_emul_card_cfg_##n = {               \
		.card_type = DT_INST_ENUM_IDX_OR(n, card_type, SDHC_EMUL_TYPE_SDHC),   \
		.n_blocks = DT_INST_PROP_OR(n, capacity_blocks, 131072),                \
		.block_size = DT_INST_PROP_OR(n, sector_size, 512),                     \
		.inject_cmd = DT_INST_PROP_OR(n, inject_error_cmd, 255),                \
		.sdio_fn_cnt = DT_INST_PROP_OR(n, sdio_func_count, 1),                 \
		.card_present = DT_INST_NODE_HAS_PROP(n, card_inserted),                \
		.host_ocr = DT_INST_PROP_OR(n, host_ocr, 0x00FF8000),                  \
	};                                                                              \
	DEVICE_DT_INST_DEFINE(n, sdhc_emul_card_dev_init, NULL,                         \
			      &sdhc_emul_card_data_##n,                                 \
			      &sdhc_emul_card_cfg_##n,                                  \
			      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, NULL);            \
	EMUL_DT_INST_DEFINE(n, sdhc_emul_card_init,                                     \
			    &sdhc_emul_card_data_##n,                                   \
			    &sdhc_emul_card_cfg_##n,                                    \
			    &sdhc_emul_card_api_instance, NULL)

DT_INST_FOREACH_STATUS_OKAY(SDHC_EMUL_CARD_DEFINE)
