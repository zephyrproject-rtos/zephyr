/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/espi.h>
#include <power/ndsx_espi.h>
#include <power/x86_non_dsx.h>

#define LOG_LEVEL CONFIG_SYS_PM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(power);

static const struct device *espi_dev;
static struct espi_callback espi_bus_cb;
static struct espi_callback espi_chan_cb;
static struct espi_callback espi_vw_cb;

static void espi_vw_handler(struct espi_event *event)
{
	LOG_DBG("VW is triggered, event=%d, val=%d\n", event->evt_details,
			vw_get_level(event->evt_details));

	switch (event->evt_details) {
	case ESPI_VWIRE_SIGNAL_SLP_S3:
	case ESPI_VWIRE_SIGNAL_SLP_S4:
		/* k_thread_resume(&pwrseq_thread_id); */
		break;
	default:
		break;
	}
}

static void espi_bus_handler(const struct device *dev,
				struct espi_callback *cb,
				struct espi_event event)
{
	switch (event.evt_type) {
	case ESPI_BUS_RESET:
		LOG_DBG("ESPI bus reset");
		espi_bus_reset();
		break;
	case ESPI_BUS_EVENT_VWIRE_RECEIVED:
		LOG_DBG("ESPI VW received");
		espi_vw_handler(&event);
		break;
	case ESPI_BUS_EVENT_CHANNEL_READY:
		LOG_DBG("ESPI channel ready");
		break;
	default:
		break;
	}
}

uint8_t vw_get_level(enum espi_vwire_signal signal)
{
	uint8_t level;

	if (espi_receive_vwire(espi_dev, signal, &level))
		return 0;

	return level;
}

void ndsx_espi_configure(void)
{
	struct espi_cfg cfg = {
		.io_caps = ESPI_IO_MODE_SINGLE_LINE,
		.channel_caps = ESPI_CHANNEL_VWIRE |
			ESPI_CHANNEL_PERIPHERAL |
			ESPI_CHANNEL_OOB,
		.max_freq = 20, //ESPI_FREQ_MHZ,
	};

	espi_dev = device_get_binding("ESPI_0");
	if (!espi_dev) {
		LOG_ERR("Failed to get eSPI binding");
		return;
	}

	if (espi_config(espi_dev, &cfg)) {
		LOG_ERR("Failed to configure eSPI");
		return;
	}

	/* Configure handler for eSPI events */
	espi_init_callback(&espi_bus_cb, espi_bus_handler, ESPI_BUS_RESET);
	espi_add_callback(espi_dev, &espi_bus_cb);

	espi_init_callback(&espi_chan_cb, espi_bus_handler,
			ESPI_BUS_EVENT_CHANNEL_READY);
	espi_add_callback(espi_dev, &espi_chan_cb);

	espi_init_callback(&espi_vw_cb, espi_bus_handler,
			ESPI_BUS_EVENT_VWIRE_RECEIVED);
	espi_add_callback(espi_dev, &espi_vw_cb);
}
