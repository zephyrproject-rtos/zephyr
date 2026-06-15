/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This driver creates a fake SDHC bus which can contain emulated card devices,
 * implemented by a separate emulation driver. The API between this driver and
 * its emulators is defined by struct sdhc_emul_api.
 */

#define DT_DRV_COMPAT zephyr_sdhc_emul_controller

#define LOG_LEVEL CONFIG_SDHC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sdhc_emul_ctlr);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/sdhc_emul.h>

/** Working data for the controller device */
struct sdhc_emul_ctlr_data {
	/* List of struct sdhc_emul associated with the device */
	sys_slist_t emuls;
	/* Interrupt callback */
	sdhc_interrupt_cb_t irq_cb;
	int irq_sources;
	void *irq_user_data;
};

struct sdhc_emul_ctlr_config {
	struct emul_list_for_bus emul_list;
};

/**
 * Find the first card emulator on this bus.
 * SDHC typically has one card per slot.
 */
struct sdhc_emul *sdhc_emul_get_card(const struct device *dev)
{
	struct sdhc_emul_ctlr_data *data = dev->data;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&data->emuls, node) {
		struct sdhc_emul *emul = CONTAINER_OF(node, struct sdhc_emul, node);

		return emul;
	}

	return NULL;
}

static int sdhc_emul_ctlr_reset(const struct device *dev)
{
	LOG_DBG("SDHC emul controller reset");
	return 0;
}

static int sdhc_emul_ctlr_request(const struct device *dev,
				  struct sdhc_command *cmd,
				  struct sdhc_data *data)
{
	struct sdhc_emul *emul;
	const struct sdhc_emul_api *api;
	int ret;

	emul = sdhc_emul_get_card(dev);
	if (!emul) {
		return -ENODEV;
	}

	api = emul->api;
	__ASSERT_NO_MSG(api);
	__ASSERT_NO_MSG(api->request);

	if (emul->mock_api != NULL && emul->mock_api->request != NULL) {
		ret = emul->mock_api->request(emul->target, cmd, data);
		if (ret != -ENOSYS) {
			return ret;
		}
	}

	return api->request(emul->target, cmd, data);
}

static int sdhc_emul_ctlr_set_io(const struct device *dev, struct sdhc_io *ios)
{
	return 0;
}

static int sdhc_emul_ctlr_get_card_present(const struct device *dev)
{
	struct sdhc_emul *emul = sdhc_emul_get_card(dev);

	if (emul != NULL && emul->api != NULL && emul->api->card_present != NULL) {
		return emul->api->card_present(emul->target);
	}

	return 0;
}

static int sdhc_emul_ctlr_execute_tuning(const struct device *dev)
{
	LOG_DBG("Tuning executed (emul)");
	return 0;
}

static int sdhc_emul_ctlr_card_busy(const struct device *dev)
{
	struct sdhc_emul *emul = sdhc_emul_get_card(dev);

	if (emul && emul->api && emul->api->card_busy) {
		return emul->api->card_busy(emul->target);
	}

	return 0;
}

static int sdhc_emul_ctlr_get_host_props(const struct device *dev,
					 struct sdhc_host_props *props)
{
	struct sdhc_emul *emul = sdhc_emul_get_card(dev);
	bool is_emmc = false;

	memset(props, 0, sizeof(*props));

	props->f_min = 400000;
	props->f_max = 200000000;
	props->bus_4_bit_support = true;
	props->host_caps.high_spd_support = 1;
	props->host_caps.vol_330_support = 1;

	/* Query card type to determine capabilities */
	if (emul && emul->target && emul->target->cfg) {
		const struct {
			uint32_t card_type;
		} *cfg = emul->target->cfg;

		/* SDHC_EMUL_TYPE_EMMC == 3 per DTS enum idx */
		if (cfg->card_type == 3) {
			is_emmc = true;
		}
	}

	if (is_emmc) {
		props->host_caps.bus_8_bit_support = 1;
		props->hs200_support = true;
		props->hs400_support = true;
	}

	return 0;
}

static int sdhc_emul_ctlr_enable_interrupt(const struct device *dev,
					   sdhc_interrupt_cb_t callback,
					   int sources, void *user_data)
{
	struct sdhc_emul_ctlr_data *data = dev->data;

	data->irq_cb = callback;
	data->irq_sources = sources;
	data->irq_user_data = user_data;
	return 0;
}

static int sdhc_emul_ctlr_disable_interrupt(const struct device *dev, int sources)
{
	struct sdhc_emul_ctlr_data *data = dev->data;

	data->irq_sources &= ~sources;
	if (data->irq_sources == 0) {
		data->irq_cb = NULL;
		data->irq_user_data = NULL;
	}
	return 0;
}

void sdhc_emul_raise_interrupt(const struct device *dev, int sources)
{
	struct sdhc_emul_ctlr_data *data = dev->data;

	if (data->irq_cb && (data->irq_sources & sources) != 0) {
		data->irq_cb(dev, data->irq_sources & sources, data->irq_user_data);
	}
}

int sdhc_emul_register(const struct device *dev, struct sdhc_emul *emul)
{
	struct sdhc_emul_ctlr_data *data = dev->data;

	sys_slist_append(&data->emuls, &emul->node);

	LOG_INF("Register emulator '%s' at SDHC slot %u",
		emul->target->dev->name, emul->slot);

	return 0;
}

static int sdhc_emul_ctlr_init(const struct device *dev)
{
	struct sdhc_emul_ctlr_data *data = dev->data;
	int rc;

	sys_slist_init(&data->emuls);

	rc = emul_init_for_bus(dev);

	return rc;
}

static DEVICE_API(sdhc, sdhc_emul_ctlr_api) = {
	.reset = sdhc_emul_ctlr_reset,
	.request = sdhc_emul_ctlr_request,
	.set_io = sdhc_emul_ctlr_set_io,
	.get_card_present = sdhc_emul_ctlr_get_card_present,
	.execute_tuning = sdhc_emul_ctlr_execute_tuning,
	.card_busy = sdhc_emul_ctlr_card_busy,
	.get_host_props = sdhc_emul_ctlr_get_host_props,
	.enable_interrupt = sdhc_emul_ctlr_enable_interrupt,
	.disable_interrupt = sdhc_emul_ctlr_disable_interrupt,
};

#define EMUL_LINK_AND_COMMA(node_id)                                        \
	{                                                                   \
		.dev = DEVICE_DT_GET(node_id),                              \
	},

#define SDHC_EMUL_CTLR_INIT(n)                                                          \
	static const struct emul_link_for_bus sdhc_emuls_##n[] = {                       \
		DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(n), EMUL_LINK_AND_COMMA)};     \
	static struct sdhc_emul_ctlr_config sdhc_emul_ctlr_cfg_##n = {                  \
		.emul_list = {                                                           \
			.children = sdhc_emuls_##n,                                      \
			.num_children = ARRAY_SIZE(sdhc_emuls_##n),                      \
		},                                                                       \
	};                                                                               \
	static struct sdhc_emul_ctlr_data sdhc_emul_ctlr_data_##n;                      \
	DEVICE_DT_INST_DEFINE(n, sdhc_emul_ctlr_init, NULL,                             \
			      &sdhc_emul_ctlr_data_##n, &sdhc_emul_ctlr_cfg_##n,        \
			      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY,                    \
			      &sdhc_emul_ctlr_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_EMUL_CTLR_INIT)
