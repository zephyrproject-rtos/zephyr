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
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(espi_host);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/espi_emul.h>

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
	{ ESPI_VWIRE_SIGNAL_SLP_S3, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SLP_S4, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SLP_S5, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SUS_STAT, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_PLTRST, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_OOB_RST_WARN, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_OOB_RST_ACK, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_WAKE, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_PME, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_ERR_FATAL, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_ERR_NON_FATAL, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_SLV_BOOT_STS, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_SCI, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_SMI, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_RST_CPU_INIT, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_HOST_RST_ACK, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_HOST_RST_WARN, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SUS_ACK, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_DNX_ACK, 0, ESPI_SLAVE_TO_MASTER },
	{ ESPI_VWIRE_SIGNAL_SUS_WARN, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SLP_A, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SLP_LAN, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_SLP_WLAN, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_HOST_C10, 0, ESPI_MASTER_TO_SLAVE },
	{ ESPI_VWIRE_SIGNAL_DNX_WARN, 0, ESPI_MASTER_TO_SLAVE },
};

#define NUMBER_OF_VWIRES ARRAY_SIZE(vw_state_default)

/** Run-time data used by the emulator */
struct espi_host_emul_data {
	/** eSPI emulator detail */
	struct espi_emul emul;
	/** eSPI controller device */
	const struct device *espi;
	/** Virtual Wires states, for one slave only.
	 *  With multi-slaves config, the states should be saved per slave */
	struct vw_data vw_state[NUMBER_OF_VWIRES];
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	/** ACPI Shared memory. */
	uint8_t shm_acpi_mmap[CONFIG_EMUL_ESPI_HOST_ACPI_SHM_REGION_SIZE];
#endif
};

/** Static configuration for the emulator */
struct espi_host_emul_cfg {
	/** Label of the emulated AP*/
	const char *label;
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

static int emul_host_set_vw(const struct emul *target,
			    enum espi_vwire_signal vw, uint8_t level)
{
	struct espi_host_emul_data *data = target->data;
	int idx;

	idx = emul_host_find_index(data, vw);

	if (idx < 0 || data->vw_state[idx].dir != ESPI_SLAVE_TO_MASTER) {
		LOG_ERR("%s: invalid vw: %d", __func__, vw);
		return -EPERM;
	}

	data->vw_state[idx].level = level;

	return 0;
}

static int emul_host_get_vw(const struct emul *target,
			    enum espi_vwire_signal vw, uint8_t *level)
{
	struct espi_host_emul_data *data = target->data;
	int idx;

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
	data_host = espi_dev->data;

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

#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
static uintptr_t emul_espi_dev_get_acpi_shm(const struct emul *target)
{
	struct espi_host_emul_data *data = target->data;

	return (uintptr_t)data->shm_acpi_mmap;
}

uintptr_t emul_espi_host_get_acpi_shm(const struct device *espi_dev)
{
	uint32_t shm;
	int rc = espi_read_lpc_request(espi_dev, EACPI_GET_SHARED_MEMORY, &shm);

	__ASSERT_NO_MSG(rc == 0);

	return (uintptr_t) shm;
}
#endif

/* Device instantiation */
static struct emul_espi_device_api ap_emul_api = {
	.set_vw = emul_host_set_vw,
	.get_vw = emul_host_get_vw,
	.get_acpi_shm = emul_espi_dev_get_acpi_shm,
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
	struct espi_host_emul_data *data = emul->data;

	ARG_UNUSED(bus);

	emul_host_init_vw_state(data);

	return 0;
}

#define HOST_EMUL(n)                                                                               \
	static struct espi_host_emul_data espi_host_emul_data_##n;                                 \
	static const struct espi_host_emul_cfg espi_host_emul_cfg_##n = {                          \
		.chipsel = DT_INST_REG_ADDR(n),                                                    \
	};                                                                                         \
	EMUL_DEFINE(emul_host_init, DT_DRV_INST(n), &espi_host_emul_cfg_##n,                       \
		    &espi_host_emul_data_##n, &ap_emul_api)

DT_INST_FOREACH_STATUS_OKAY(HOST_EMUL)
