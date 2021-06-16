/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This driver creates fake eSPI buses which can contain emulated devices
 * (mainly host), implemented by a separate emulation driver.
 * The API between this driver/controller and device emulators attached
 * to its bus is defined by struct emul_espi_device_api.
 */

#define DT_DRV_COMPAT zephyr_espi_emul_controller

#define LOG_LEVEL CONFIG_ESPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(espi_emul_ctlr);

#include <device.h>
#include <drivers/emul.h>
#include <drivers/espi.h>
#include <drivers/espi_emul.h>
#include "espi_utils.h"

/** Working data for the controller */
struct espi_emul_data {
	/* List of struct espi_emul associated with the device */
	sys_slist_t emuls;
	/* eSPI host configuration */
	struct espi_cfg cfg;
	/** List of eSPI callbacks */
	sys_slist_t callbacks;
};

static struct espi_emul *espi_emul_find(const struct device *dev,
					unsigned int chipsel)
{
	struct espi_emul_data *data = dev->data;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&data->emuls, node) {
		struct espi_emul *emul;

		emul = CONTAINER_OF(node, struct espi_emul, node);
		if (emul->chipsel == chipsel) {
			return emul;
		}
	}

	return NULL;
}

static int espi_emul_config(const struct device *dev, struct espi_cfg *cfg)
{
	struct espi_emul_data *data = dev->data;

	__ASSERT_NO_MSG(cfg);

	data->cfg = *cfg;

	return 0;
}


static int emul_espi_trigger_event(const struct device *dev,
				   struct espi_event *evt)
{
	struct espi_emul_data *data = dev->data;

	if (((evt->evt_type & ESPI_BUS_EVENT_VWIRE_RECEIVED) &&
	     !(data->cfg.channel_caps & ESPI_CHANNEL_VWIRE)) ||
	    ((evt->evt_type & ESPI_BUS_EVENT_OOB_RECEIVED) &&
	     !(data->cfg.channel_caps & ESPI_CHANNEL_OOB)) ||
	    ((evt->evt_type & ESPI_BUS_PERIPHERAL_NOTIFICATION)
	     && !(data->cfg.channel_caps & ESPI_CHANNEL_PERIPHERAL))) {
		return -EIO;
	}

	espi_send_callbacks(&data->callbacks, dev, *evt);

	return 0;
}

static bool espi_emul_get_channel_status(const struct device *dev, enum espi_channel ch)
{
	struct espi_emul_data *data = dev->data;

	return (data->cfg.channel_caps & ch);
}

static int espi_emul_send_vwire(const struct device *dev, enum espi_vwire_signal vw, uint8_t level)
{
	const struct emul_espi_device_api *api;
	struct espi_emul *emul;
	struct espi_emul_data *data = dev->data;

	if (!(data->cfg.channel_caps & ESPI_CHANNEL_VWIRE)) {
		return -EIO;
	}

	emul = espi_emul_find(dev, EMUL_ESPI_HOST_CHIPSEL);
	if (!emul) {
		LOG_DBG("espi_emul not found");
		return -EIO;
	}

	__ASSERT_NO_MSG(emul->api);
	__ASSERT_NO_MSG(emul->api->set_vw);
	api = emul->api;

	return api->set_vw(emul, vw, level);
}

static int espi_emul_receive_vwire(const struct device *dev, enum espi_vwire_signal vw, uint8_t *level)
{
	const struct emul_espi_device_api *api;
	struct espi_emul *emul;
	struct espi_emul_data *data = dev->data;

	if (!(data->cfg.channel_caps & ESPI_CHANNEL_VWIRE)) {
		return -EIO;
	}

	emul = espi_emul_find(dev, EMUL_ESPI_HOST_CHIPSEL);
	if (!emul) {
		LOG_INF("espi_emul not found");
		return -EIO;
	}

	__ASSERT_NO_MSG(emul->api);
	__ASSERT_NO_MSG(emul->api->get_vw);
	api = emul->api;

	return api->get_vw(emul, vw, level);
}

static int espi_emul_manage_callback(const struct device *dev, struct espi_callback *callback, bool set)
{
	struct espi_emul_data *data = dev->data;

	return espi_manage_callback(&data->callbacks, callback, set);
}

/**
 * Set up a new emulator and add it to the list
 *
 * @param dev eSPI emulation controller device
 */
static int espi_emul_init(const struct device *dev)
{
	struct espi_emul_data *data = dev->data;
	const struct emul_list_for_bus *list = dev->config;

	sys_slist_init(&data->emuls);

	return emul_init_for_bus_from_list(dev, list);
}

int espi_emul_register(const struct device *dev, const char *name,
		       struct espi_emul *emul)
{
	struct espi_emul_data *data = dev->data;

	sys_slist_append(&data->emuls, &emul->node);

	LOG_INF("Register emulator '%s' at cs %u\n", name, emul->chipsel);

	return 0;
}

/* Device instantiation */
static struct emul_espi_driver_api emul_espi_driver_api = {
	.espi_api = {
		.config = espi_emul_config,
		.get_channel_status = espi_emul_get_channel_status,
		.send_vwire = espi_emul_send_vwire,
		.receive_vwire = espi_emul_receive_vwire,
		.manage_callback = espi_emul_manage_callback
	},
	.trigger_event = emul_espi_trigger_event,
	.find_emul = espi_emul_find,
};


#define EMUL_LINK_AND_COMMA(node_id) {	    \
		.label = DT_LABEL(node_id), \
},

#define ESPI_EMUL_INIT(n)					      \
	static const struct emul_link_for_bus emuls_##n[] = {	      \
		DT_FOREACH_CHILD(DT_DRV_INST(n), EMUL_LINK_AND_COMMA) \
	};							      \
	static struct emul_list_for_bus espi_emul_cfg_##n = {	      \
		.children = emuls_##n,				      \
		.num_children = ARRAY_SIZE(emuls_##n),		      \
	};							      \
	static struct espi_emul_data espi_emul_data_##n;	      \
	DEVICE_DT_INST_DEFINE(n,				      \
			      &espi_emul_init,			      \
			      NULL,				      \
			      &espi_emul_data_##n,		      \
			      &espi_emul_cfg_##n,		      \
			      POST_KERNEL,			      \
			      CONFIG_ESPI_INIT_PRIORITY,	      \
			      &emul_espi_driver_api);


DT_INST_FOREACH_STATUS_OKAY(ESPI_EMUL_INIT)
