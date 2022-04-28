/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for the generic eSPI Host. This supports basic
 * host operations.
 */

#define DT_DRV_COMPAT zephyr_espi_emul_host

#define LOG_LEVEL CONFIG_ESPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(espi_host);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/espi/espi.h>
#include <zephyr/drivers/espi/espi_emul.h>
#include <zephyr/drivers/espi/espi_utils.h>

/** Declare the default state of virtual wires */
const static struct espi_emul_vw_data vw_state_default[] = {
	{
		.sig = ESPI_VWIRE_SIGNAL_SLP_S3,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_SLP_S4,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_SLP_S5,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_OOB_RST_WARN,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_PLTRST,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_SUS_STAT,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_NMIOUT,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_SMIOUT,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_HOST_RST_WARN,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_SLP_A,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_SUS_WARN,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_SLP_WLAN,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_SLP_LAN,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_HOST_C10,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_DNX_WARN,
		.level = 0,
	},
};

#define NUMBER_OF_VWIRES ARRAY_SIZE(vw_state_default)

/** Run-time data used by the emulator */
struct espi_host_emul_data {
	/** eSPI emulator detail */
	struct espi_emul emul;
	/** eSPI controller device */
	const struct device *espi;
	/** Callbacks */
	sys_slist_t callbacks;
	/** Virtual wires states */
	struct espi_emul_vw_data vw_state[NUMBER_OF_VWIRES];
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	/** ACPI Shared memory. */
	uint8_t shm_acpi_mmap[CONFIG_ESPI_EMUL_HOST_ACPI_SHM_REGION_SIZE];
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	/** Host commands shared memory. */
	uint8_t shm_host_cmd[CONFIG_ESPI_EMUL_HOST_CMD_SHM_REGION_SIZE];
#endif
};

/** Static configuration for the emulator */
struct espi_host_emul_cfg {
	/** Label of the emulated AP*/
	const char *label;
	/* eSPI chip-select of the emulated device */
	uint16_t chipsel;
};

/* Helpers */

/**
 * Initialize the state of virtual wires to default based on
 * the vw_state_default array.
 *
 * @param data Host emulator data with the vwire array
 */
static void host_espi_emul_init_vw_state(struct espi_host_emul_data *data)
{
	unsigned int i;

	for (i = 0; i < NUMBER_OF_VWIRES; i++) {
		data->vw_state[i] = vw_state_default[i];
	}
}

/**
 * Find a virtual wire in the array placed in the host data.
 *
 * @param data Host emulator data with the vwire array
 * @param vw Virtual wire signal to be found
 * @return index in the array
 * @return -1 if not found
 */
static int host_espi_emul_find_index(struct espi_host_emul_data *data, enum espi_vwire_signal vw)
{
	int idx;

	for (idx = 0; idx < NUMBER_OF_VWIRES; idx++) {
		if (data->vw_state[idx].sig == vw) {
			return idx;
		}
	}

	return -1;
}

/* API */

static int host_espi_api_config(const struct espi_emul *dev, struct espi_cfg *cfg)
{
	return 0;
}

static bool host_espi_api_get_channel_status(const struct espi_emul *dev, enum espi_channel ch)
{
	return ch == ESPI_CHANNEL_VWIRE;
}

/* Logical Channel 0 APIs */
static int host_espi_api_read_request(const struct espi_emul *dev, struct espi_request_packet *req)
{
	return -EIO;
}

static int host_espi_api_write_request(const struct espi_emul *dev, struct espi_request_packet *req)
{
	return -EIO;
}

static int host_espi_api_lpc_read_request(const struct espi_emul *dev,
					  enum lpc_peripheral_opcode op, uint32_t *value)
{
	struct espi_host_emul_data *data = CONTAINER_OF(dev, struct espi_host_emul_data, emul);

	switch (op) {
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	case EACPI_GET_SHARED_MEMORY:
		*value = (uint32_t)data->shm_acpi_mmap;
		break;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY:
		*value = (uint32_t)data->shm_host_cmd;
		break;
#endif
	default:
		return -EINVAL;
	}

	return 0;
}

static int host_espi_api_lpc_write_request(const struct espi_emul *dev,
					   enum lpc_peripheral_opcode op, uint32_t *value)
{
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	struct espi_host_emul_data *data = CONTAINER_OF(dev, struct espi_host_emul_data, emul);
	struct espi_event evt;
#endif

	switch (op) {
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	case ECUSTOM_HOST_CMD_SEND_RESULT:
		evt.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION;
		evt.evt_details = ESPI_PERIPHERAL_EC_HOST_CMD;
		evt.evt_data = *value;
		espi_send_callbacks(&data->callbacks, data->espi, evt);

		break;
#endif
	default:
		return -EINVAL;
	}

	return 0;
}

/* Logical Channel 1 APIs */
static int host_espi_api_send_vwire(const struct espi_emul *dev, enum espi_vwire_signal vw,
				    uint8_t level)
{
	struct espi_host_emul_data *data = CONTAINER_OF(dev, struct espi_host_emul_data, emul);
	struct espi_event evt;
	int idx;

	idx = host_espi_emul_find_index(data, vw);

	if (idx < 0) {
		return -EPERM;
	}

	data->vw_state[idx].level = (level > 0) ? 1 : 0;

	/* Send host callbacks */
	evt.evt_type = ESPI_BUS_EVENT_VWIRE_RECEIVED;
	evt.evt_details = vw;
	evt.evt_data = level;

	espi_emul_raise_event(data->espi, evt);

	return 0;
}

static int host_espi_api_receive_vwire(const struct espi_emul *dev, enum espi_vwire_signal vw,
				       uint8_t *level)
{
	struct espi_host_emul_data *data = CONTAINER_OF(dev, struct espi_host_emul_data, emul);
	struct espi_event evt;
	int idx;

	idx = host_espi_emul_find_index(data, vw);

	if (idx < 0) {
		return -EPERM;
	}

	*level = data->vw_state[idx].level;

	evt.evt_type = ESPI_BUS_EVENT_VWIRE_RECEIVED;
	evt.evt_details = vw;
	evt.evt_data = *level;

	espi_emul_raise_event(data->espi, evt);

	return 0;
}

/* Logical Channel 2 APIs */
static int host_espi_api_send_oob(const struct espi_emul *dev, struct espi_oob_packet *pckt)
{
	return -EIO;
}

static int host_espi_api_receive_oob(const struct espi_emul *dev, struct espi_oob_packet *pckt)
{
	return -EIO;
}

/* Logical Channel 3 APIs */
static int host_espi_api_flash_read(const struct espi_emul *dev, struct espi_flash_packet *pckt)
{
	return -EIO;
}

static int host_espi_api_flash_write(const struct espi_emul *dev, struct espi_flash_packet *pckt)
{
	return -EIO;
}

static int host_espi_api_flash_erase(const struct espi_emul *dev, struct espi_flash_packet *pckt)
{
	return -EIO;
}

/* Callbacks and traffic intercept */
static int host_espi_api_manage_callback(const struct espi_emul *dev,
					 struct espi_callback *callback, bool set)
{
	struct espi_host_emul_data *data = CONTAINER_OF(dev, struct espi_host_emul_data, emul);

	return espi_manage_callback(&data->callbacks, callback, set);
}

static int host_espi_api_raise_event(const struct espi_emul *dev, struct espi_event ev)
{
	struct espi_host_emul_data *data = CONTAINER_OF(dev, struct espi_host_emul_data, emul);

	espi_send_callbacks(&data->callbacks, data->espi, ev);

	return 0;
}

#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
static uintptr_t host_espi_api_get_acpi_shm(const struct espi_emul *dev)
{
	struct espi_host_emul_data *data = CONTAINER_OF(dev, struct espi_host_emul_data, emul);

	return (uintptr_t)data->shm_acpi_mmap;
}
#endif

static struct espi_emul_device_api host_espi_emul_api = {
	.config = host_espi_api_config,
	.get_channel_status = host_espi_api_get_channel_status,
	.read_request = host_espi_api_read_request,
	.write_request = host_espi_api_write_request,
	.read_lpc_request = host_espi_api_lpc_read_request,
	.write_lpc_request = host_espi_api_lpc_write_request,
	.send_vwire = host_espi_api_send_vwire,
	.receive_vwire = host_espi_api_receive_vwire,
	.send_oob = host_espi_api_send_oob,
	.receive_oob = host_espi_api_receive_oob,
	.flash_read = host_espi_api_flash_read,
	.flash_write = host_espi_api_flash_write,
	.flash_erase = host_espi_api_flash_erase,
	.manage_callback = host_espi_api_manage_callback,
	.raise_event = host_espi_api_raise_event,
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	.get_acpi_shm = host_espi_api_get_acpi_shm,
#endif
};

/**
 * Set up a new eSPI host emulator
 *
 * @param emul Emulation information
 * @param bus Device to emulated eSPI controller
 * @return 0 indicating success (always)
 */
static int host_espi_emul_init(const struct emul *target, const struct device *bus)
{
	const struct espi_host_emul_cfg *cfg = target->cfg;
	struct espi_host_emul_data *data = target->data;

	data->emul.api = &host_espi_emul_api;
	data->emul.chipsel = cfg->chipsel;
	data->emul.target = target;
	data->espi = bus;
	host_espi_emul_init_vw_state(data);

#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	memset(data->shm_acpi_mmap, 0, CONFIG_ESPI_EMUL_HOST_ACPI_SHM_REGION_SIZE);
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	memset(data->shm_host_cmd, 0, CONFIG_ESPI_EMUL_HOST_CMD_SHM_REGION_SIZE);
#endif

	return espi_emul_register(bus, &data->emul);
}

static int host_espi_emul_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

#define HOST_EMUL(n)                                                                               \
	static struct espi_host_emul_data host_espi_emul_data_##n;                                 \
	static const struct espi_host_emul_cfg host_espi_emul_cfg_##n = {                          \
		.chipsel = DT_INST_REG_ADDR(n),                                                    \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, host_espi_emul_init, &host_espi_emul_data_##n,                      \
			    &host_espi_emul_cfg_##n, &host_espi_emul_api);                         \
	DEVICE_DT_INST_DEFINE(n, &host_espi_emul_dev_init, NULL, NULL, NULL, POST_KERNEL,          \
			      CONFIG_ESPI_INIT_PRIORITY, NULL)

DT_INST_FOREACH_STATUS_OKAY(HOST_EMUL);
