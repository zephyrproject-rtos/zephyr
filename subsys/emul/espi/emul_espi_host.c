/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for the generic eSPI Host. This supports basic
 * host operations.
 */

#define DT_DRV_COMPAT zephyr_espi_emul_espi_host

#define LOG_LEVEL CONFIG_ESPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(espi_host);

#include <device.h>
#include <emul.h>
#include <drivers/espi.h>
#include <drivers/espi_emul.h>

/** Data about the virtual wire */
struct vw_data {
	/* Virtual wire signal */
	enum espi_vwire_signal sig;
	/* The level(state) of the virtual wire */
	uint8_t level;
	/* The direction of the virtual wire. Possible values:
	 * ESPI_MASTER_TO_SLAVE or ESPI_SLAVE_TO_MASTER */
	uint8_t dir;
};

/** Declare the default state of virtual wires */
const static struct vw_data vw_state_default[] = {
	{ ESPI_VWIRE_SIGNAL_SLP_S3,        0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SLP_S4,        0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SLP_S5,        0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SUS_STAT,      0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_PLTRST,        0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_OOB_RST_WARN,  0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_OOB_RST_ACK,   0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_WAKE,          0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_PME,           0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_ERR_FATAL,     0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_ERR_NON_FATAL, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_SLV_BOOT_STS,  0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_SCI,           0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_SMI,           0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_RST_CPU_INIT,  0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_HOST_RST_ACK,  0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_HOST_RST_WARN, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SUS_ACK,       0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_DNX_ACK,       0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_SUS_WARN,      0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SLP_A,         0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SLP_LAN,       0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SLP_WLAN,      0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_HOST_C10,      0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_DNX_WARN,      0, ESPI_MASTER_TO_SLAVE },
};

#define NUMBER_OF_VWIRES ARRAY_SIZE(vw_state_default)

/** Run-time data used by the emulator */
struct espi_host_emul_data {
	/** eSPI emulator detail */
	struct espi_emul emul;
	/** eSPI controller device */
	const struct device *espi;
	/** Configuration information */
	const struct espi_host_emul_cfg *cfg;
	/** Virtual Wires states, for one slave only.
	 *  With multi-slaves config, the states should be saved per slave */
	struct vw_data vw_state[NUMBER_OF_VWIRES];
};

/** Static configuration for the emulator */
struct espi_host_emul_cfg {
	/** Label of the eSPI bus this emulator connects to */
	const char *espi_label;
	/** Label of the emulated AP*/
	const char *label;
	/** Pointer to run-time data */
	struct espi_host_emul_data *data;
	/* eSPI chip-select of the emulated device */
	uint16_t chipsel;
};

/**
 * Initialize the state of virtual wires to default based on
 * the vw_state_default array.
 *
 * @param data Host emulator data with the vwire array
 */
static void emul_host_init_vw_state(struct espi_host_emul_data *data)
{
	unsigned i;

	for (i = 0; i < NUMBER_OF_VWIRES; i++) {
		data->vw_state[i] = vw_state_default[i];
	}
	return;
}

/**
 * Find a virtual wire in the array placed in the host data.
 *
 * @param data Host emulator data with the vwire array
 * @param vw Virtual wire signal to be found
 * @return index in the array
 * @return -1 if not found
 */
static int emul_host_find_index(struct espi_host_emul_data *data,
				enum espi_vwire_signal vw)
{
	int idx;

	for (idx = 0; idx < NUMBER_OF_VWIRES; idx++) {
		if (data->vw_state[idx].sig == vw) {
			return idx;
		}
	}

	return -1;
}

static int emul_host_set_vw(struct espi_emul *emul, enum espi_vwire_signal vw,
			    uint8_t level)
{
	struct espi_host_emul_data *data;
	int idx;

	data = CONTAINER_OF(emul, struct espi_host_emul_data, emul);
	idx = emul_host_find_index(data, vw);

	if (idx < 0 || data->vw_state[idx].dir != ESPI_SLAVE_TO_MASTER) {
		LOG_ERR("%s: invalid vw: %d", __func__, vw);
		return -EPERM;
	}

	data->vw_state[idx].level = level;

	return 0;
}

static int emul_host_get_vw(struct espi_emul *emul, enum espi_vwire_signal vw,
			    uint8_t *level)
{
	struct espi_host_emul_data *data;
	int idx;

	data = CONTAINER_OF(emul, struct espi_host_emul_data, emul);
	idx = emul_host_find_index(data, vw);

	if (idx < 0 || data->vw_state[idx].dir != ESPI_MASTER_TO_SLAVE) {
		LOG_ERR("%s: invalid vw: %d", __func__, vw);
		return -EPERM;
	}

	*level = data->vw_state[idx].level;

	return 0;
}

int emul_espi_host_send_vw(const struct device *espi_dev, enum espi_vwire_signal vw,
			   uint8_t level)
{
	struct espi_emul *emul_espi;
	struct espi_event evt;
	struct espi_host_emul_data *data_host;
	struct emul_espi_driver_api *api;
	int idx;

	api = (struct emul_espi_driver_api *)espi_dev->api;

	__ASSERT_NO_MSG(api);
	__ASSERT_NO_MSG(api->trigger_event);
	__ASSERT_NO_MSG(api->find_emul);

	emul_espi = api->find_emul(espi_dev, EMUL_ESPI_HOST_CHIPSEL);
	data_host = CONTAINER_OF(emul_espi, struct espi_host_emul_data, emul);

	idx = emul_host_find_index(data_host, vw);
	if (idx < 0 || data_host->vw_state[idx].dir != ESPI_MASTER_TO_SLAVE) {
		LOG_ERR("%s: invalid vw: %d", __func__, vw);
		return -EPERM;
	}

	data_host->vw_state[idx].level = level;

	evt.evt_type = ESPI_BUS_EVENT_VWIRE_RECEIVED;
	evt.evt_details = vw;
	evt.evt_data = level;

	api->trigger_event(espi_dev, &evt);

	return 0;
}

int emul_espi_host_port80_write(const struct device *espi_dev, uint32_t data)
{
	struct espi_event evt;
	struct emul_espi_driver_api *api;

	api = (struct emul_espi_driver_api *)espi_dev->api;

	__ASSERT_NO_MSG(api);
	__ASSERT_NO_MSG(api->trigger_event);

	evt.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION;
	evt.evt_details = ESPI_PERIPHERAL_DEBUG_PORT80;
	evt.evt_data = data;

	api->trigger_event(espi_dev, &evt);

	return 0;
}

/* Device instantiation */
static struct emul_espi_device_api ap_emul_api = {
	.set_vw = emul_host_set_vw,
	.get_vw = emul_host_get_vw,
};

/**
 * Set up a new eSPI host emulator
 *
 * @param emul Emulation information
 * @param bus Device to emulated eSPI controller
 * @return 0 indicating success (always)
 */
static int emul_host_init(const struct emul *emul, const struct device *bus)
{
	const struct espi_host_emul_cfg *cfg = emul->cfg;
	struct espi_host_emul_data *data = cfg->data;

	data->emul.api = &ap_emul_api;
	data->emul.chipsel = cfg->chipsel;
	data->espi = bus;
	data->cfg = cfg;
	emul_host_init_vw_state(data);

	return espi_emul_register(bus, emul->dev_label, &data->emul);
}

#define HOST_EMUL(n)							  \
	static struct espi_host_emul_data espi_host_emul_data_##n;	  \
	static const struct espi_host_emul_cfg espi_host_emul_cfg_##n = { \
		.espi_label = DT_INST_BUS_LABEL(n),			  \
		.data = &espi_host_emul_data_##n,			  \
		.chipsel = DT_INST_REG_ADDR(n),				  \
	};								  \
	EMUL_DEFINE(emul_host_init, DT_DRV_INST(n), &espi_host_emul_cfg_##n)

DT_INST_FOREACH_STATUS_OKAY(HOST_EMUL)
