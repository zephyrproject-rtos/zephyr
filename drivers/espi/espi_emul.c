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
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(espi_emul_ctlr);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/espi/espi.h>
#include <zephyr/drivers/espi/espi_emul.h>
#include <zephyr/drivers/espi/espi_utils.h>

/** Declare the default state of virtual wires */
static const struct espi_emul_vw_data vw_state_default[] = {
	{
		.sig = ESPI_VWIRE_SIGNAL_OOB_RST_ACK,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_WAKE,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_PME,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_ERR_FATAL,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_ERR_NON_FATAL,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_SLV_BOOT_STS,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_SCI,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_SMI,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_RST_CPU_INIT,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_HOST_RST_ACK,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_SUS_ACK,
		.level = 0,
	},
	{
		.sig = ESPI_VWIRE_SIGNAL_DNX_ACK,
		.level = 0,
	},
};

#define NUMBER_OF_VWIRES ARRAY_SIZE(vw_state_default)

/** Working data for the eSPI peripheral emulator */
struct espi_emul_data {
	/**
	 * This is a pointer to the eSPI host emulator structure, to which the emulated peripheral
	 * is connected. This peripheral eSPI emulator doesn't emulate whole bus, instead, it
	 * emulates the eSPI as seen by the MCU, being peripheral connected to the host controller.
	 */
	struct espi_emul *host_emul;
	/** eSPI host configuration */
	struct espi_cfg cfg;
	/** List of eSPI callbacks */
	sys_slist_t callbacks;
	/** Interrupts state */
	bool interrupts_en;
	/** Virtual wires states */
	struct espi_emul_vw_data vw_state[NUMBER_OF_VWIRES];
	/** Bit field of enabled channels on eSPI bus */
	enum espi_channel channels_enabled;
	/** Keyboard controller properties */
#ifdef CONFIG_ESPI_EMUL_KBC8042
	uint8_t kbc8042_dbbin;
	uint8_t kbc8042_dbbout;
	uint8_t kbc8042_status;
	bool kbc8042_irq_enabled;
	bool kbc8042_obe_irq_pending;
#endif
	/** Variables used by LPC ACPI commands */
	uint8_t acpi_status;
};

/**
 * Initialize the state of virtual wires to default based on
 * the vw_state_default array.
 *
 * @param data Host emulator data with the vwire array
 */
static void espi_emul_init_vw_state(struct espi_emul_data *data)
{
	for (unsigned int i = 0; i < NUMBER_OF_VWIRES; i++) {
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
static int espi_emul_host_find_index(struct espi_emul_data *data, enum espi_vwire_signal vw)
{
	for (unsigned int i = 0; i < NUMBER_OF_VWIRES; i++) {
		if (data->vw_state[i].sig == vw) {
			return i;
		}
	}

	return -1;
}

#ifdef CONFIG_ESPI_EMUL_KBC8042
static void espi_emul_send_ibf_irq(const struct device *dev)
{
	struct espi_emul_data *data = dev->data;
	struct espi_evt_data_kbc *kbc_evt;
	struct espi_event ev;

	if (data->kbc8042_irq_enabled == false ||
	    (data->kbc8042_status & KBC8042_STATUS_IBF) == 0) {
		return;
	}

	ev.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION;
	ev.evt_details = ESPI_PERIPHERAL_8042_KBC;

	kbc_evt = (struct espi_evt_data_kbc *)&ev.evt_data;
	kbc_evt->evt = HOST_KBC_EVT_IBF;
	kbc_evt->data = data->kbc8042_dbbin;
	kbc_evt->type = (data->kbc8042_status & KBC8042_STATUS_A2);

	espi_send_callbacks(&data->callbacks, dev, ev);

	/* Normal MCU clears the flag and register after reading from it.
	 * We should follow this behavior.
	 */
	data->kbc8042_dbbin = 0;
	data->kbc8042_status &= ~(KBC8042_STATUS_A2 | KBC8042_STATUS_IBF);
}

static void espi_emul_send_obe_irq(const struct device *dev)
{
	struct espi_emul_data *data = dev->data;
	struct espi_evt_data_kbc *kbc_evt;
	struct espi_event ev;

	if (data->kbc8042_irq_enabled == false || data->kbc8042_obe_irq_pending == false) {
		return;
	}

	data->kbc8042_obe_irq_pending = false;

	ev.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION;
	ev.evt_details = ESPI_PERIPHERAL_8042_KBC;

	kbc_evt = (struct espi_evt_data_kbc *)&ev.evt_data;
	kbc_evt->evt = HOST_KBC_EVT_OBE;
	kbc_evt->data = 0;
	kbc_evt->type = 0;

	espi_send_callbacks(&data->callbacks, dev, ev);
}
#endif /* CONFIG_ESPI_EMUL_KBC8042 */

/** Controller API **/

static int espi_emul_api_config(const struct device *dev, struct espi_cfg *cfg)
{
	struct espi_emul_data *data = dev->data;

	data->channels_enabled = cfg->channel_caps;

	return 0;
}

static bool espi_emul_api_get_channel_status(const struct device *dev, enum espi_channel ch)
{
	struct espi_emul_data *data = dev->data;

	return (data->channels_enabled & ch);
}

/* Logical Channel 0 APIs */
static int espi_emul_api_read_request(const struct device *dev, struct espi_request_packet *req)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(req);

	return -EIO;
}

static int espi_emul_api_write_request(const struct device *dev, struct espi_request_packet *req)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(req);

	return -EIO;
}

static int espi_emul_api_lpc_read_request(const struct device *dev, enum lpc_peripheral_opcode op,
					  uint32_t *value)
{
	struct espi_emul_data *data = dev->data;
	struct espi_emul *host_emul = data->host_emul;

	if ((data->channels_enabled & ESPI_CHANNEL_PERIPHERAL) == 0) {
		return -EIO;
	}

	switch (op) {
#ifdef CONFIG_ESPI_EMUL_KBC8042
	case E8042_OBF_HAS_CHAR:
		*value = (data->kbc8042_status & KBC8042_STATUS_OBF) != 0 ? 1 : 0;
		break;
	case E8042_IBF_HAS_CHAR:
		*value = (data->kbc8042_status & KBC8042_STATUS_IBF) != 0 ? 1 : 0;
		break;
	case E8042_READ_KB_STS:
		*value = data->kbc8042_status;
		break;
#endif
	/* ACPI status transactions */
	case EACPI_READ_STS:
		*value = data->acpi_status;
		break;
	default:
		if (host_emul == NULL) {
			return -EIO;
		}

		return host_emul->api->read_lpc_request(host_emul, op, value);
	}

	return 0;
}

static int espi_emul_api_lpc_write_request(const struct device *dev, enum lpc_peripheral_opcode op,
					   uint32_t *value)
{
	struct espi_emul_data *data = dev->data;
	struct espi_emul *host_emul = data->host_emul;

	if ((data->channels_enabled & ESPI_CHANNEL_PERIPHERAL) == 0) {
		return -EIO;
	}

	switch (op) {
		/* Write transactions */
#ifdef CONFIG_ESPI_EMUL_KBC8042
	case E8042_WRITE_KB_CHAR:
		data->kbc8042_dbbout = *value;
		data->kbc8042_status &= ~KBC8042_STATUS_A2;
		data->kbc8042_status |= KBC8042_STATUS_OBF;
		break;
	case E8042_WRITE_MB_CHAR:
		data->kbc8042_dbbout = *value;
		data->kbc8042_status |= KBC8042_STATUS_A2;
		data->kbc8042_status |= KBC8042_STATUS_OBF;
		break;
#endif
		/* Write transactions without input parameters */
#ifdef CONFIG_ESPI_EMUL_KBC8042
	case E8042_RESUME_IRQ:
		data->kbc8042_irq_enabled = true;
		espi_emul_send_ibf_irq(dev);
		espi_emul_send_obe_irq(dev);
		break;
	case E8042_PAUSE_IRQ:
		data->kbc8042_irq_enabled = false;
		break;
	case E8042_CLEAR_OBF:
		data->kbc8042_status &= ~KBC8042_STATUS_OBF;
		break;
	case E8042_READ_KB_STS:
		break;
	case E8042_SET_FLAG:
		data->kbc8042_status |= *value;
		break;
	case E8042_CLEAR_FLAG:
		data->kbc8042_status &= ~(*value);
		break;
#endif
	case EACPI_WRITE_CHAR:
		break;
	/* ACPI status transactions */
	case EACPI_WRITE_STS:
		data->acpi_status = *value;
		break;
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	case ECUSTOM_HOST_SUBS_INTERRUPT_EN:
		data->interrupts_en = *value;
		break;
#endif
	default:
		if (host_emul == NULL) {
			return -EIO;
		}

		return host_emul->api->write_lpc_request(host_emul, op, value);
	}

	return 0;
}

/* Logical Channel 1 APIs */
static int espi_emul_api_send_vwire(const struct device *dev, enum espi_vwire_signal vw,
				    uint8_t level)
{
	struct espi_emul_data *data = dev->data;
	struct espi_emul *host_emul = data->host_emul;
	struct espi_event evt;
	int idx;

	if ((data->channels_enabled & ESPI_CHANNEL_VWIRE) == 0) {
		return -EIO;
	}

	idx = espi_emul_host_find_index(data, vw);
	if (idx == -1) {
		if (host_emul == NULL) {
			return -EIO;
		}

		return host_emul->api->send_vwire(host_emul, vw, level);
	}

	data->vw_state[idx].level = level;

	if (host_emul != NULL) {
		evt.evt_type = ESPI_BUS_EVENT_VWIRE_RECEIVED;
		evt.evt_details = vw;
		evt.evt_data = level;

		host_emul->api->raise_event(host_emul, evt);
	}

	return 0;
}

static int espi_emul_api_receive_vwire(const struct device *dev, enum espi_vwire_signal vw,
				       uint8_t *level)
{
	struct espi_emul_data *data = dev->data;
	struct espi_emul *host_emul = data->host_emul;
	int idx;

	if ((data->channels_enabled & ESPI_CHANNEL_VWIRE) == 0) {
		return -EIO;
	}

	idx = espi_emul_host_find_index(data, vw);
	if (idx == -1) {
		if (host_emul == NULL) {
			return -EIO;
		}

		return host_emul->api->receive_vwire(host_emul, vw, level);
	}

	*level = data->vw_state[idx].level;

	return 0;
}

/* Logical Channel 2 APIs */
static int espi_emul_api_send_oob(const struct device *dev, struct espi_oob_packet *pckt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pckt);

	return -EIO;
}

static int espi_emul_api_receive_oob(const struct device *dev, struct espi_oob_packet *pckt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pckt);

	return -EIO;
}

/* Logical Channel 3 APIs */
static int espi_emul_api_flash_read(const struct device *dev, struct espi_flash_packet *pckt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pckt);

	return -EIO;
}

static int espi_emul_api_flash_write(const struct device *dev, struct espi_flash_packet *pckt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pckt);

	return -EIO;
}

static int espi_emul_api_flash_erase(const struct device *dev, struct espi_flash_packet *pckt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pckt);

	return -EIO;
}

/* Callbacks and traffic intercept */
static int espi_emul_api_manage_callback(const struct device *dev, struct espi_callback *callback,
					 bool set)
{
	struct espi_emul_data *data = dev->data;

	return espi_manage_callback(&data->callbacks, callback, set);
}

/** Host API */

int espi_emul_api_host_io_read(const struct device *dev, uint8_t length, uint16_t addr,
			       uint32_t *reg)
{
#ifdef CONFIG_ESPI_EMUL_KBC8042
	struct espi_emul_data *data = dev->data;

	if (addr == ESPI_EMUL_KBC8042_PORT_IN_DATA) {
		*reg = data->kbc8042_dbbout;
		data->kbc8042_status &= ~(KBC8042_STATUS_OBF);
	} else if (addr == ESPI_EMUL_KBC8042_PORT_IN_STATUS) {
		*reg = data->kbc8042_status;
	} else {
		return -EINVAL;
	}

	data->kbc8042_dbbout = 0;
	data->kbc8042_status &= ~(KBC8042_STATUS_A2 | KBC8042_STATUS_OBF);
	data->kbc8042_obe_irq_pending = true;

	espi_emul_send_obe_irq(dev);

	return 0;
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(length);
	ARG_UNUSED(addr);
	ARG_UNUSED(reg);

	return -EIO;
#endif
}

int espi_emul_api_host_io_write(const struct device *dev, uint8_t length, uint16_t addr,
				uint32_t reg)
{
#ifdef CONFIG_ESPI_EMUL_KBC8042
	struct espi_emul_data *data = dev->data;

	if (addr == ESPI_EMUL_KBC8042_PORT_OUT_DATA) {
		data->kbc8042_dbbin = reg;
		data->kbc8042_status &= ~(KBC8042_STATUS_A2);
		data->kbc8042_status |= KBC8042_STATUS_IBF;
	} else if (addr == ESPI_EMUL_KBC8042_PORT_OUT_CMD) {
		data->kbc8042_dbbin = reg;
		data->kbc8042_status |= KBC8042_STATUS_A2;
		data->kbc8042_status |= KBC8042_STATUS_IBF;
	} else {
		return -EINVAL;
	}

	espi_emul_send_ibf_irq(dev);

	return 0;
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(length);
	ARG_UNUSED(addr);
	ARG_UNUSED(reg);

	return -EIO;
#endif
}

/**
 * Set up a new emulator and add it to the list
 *
 * @param dev eSPI emulation controller device
 */
static int espi_emul_init(const struct device *dev)
{
	struct espi_emul_data *data = dev->data;

	sys_slist_init(&data->callbacks);

	espi_emul_init_vw_state(data);

	data->channels_enabled = 0;

#ifdef CONFIG_ESPI_EMUL_KBC8042
	data->kbc8042_dbbin = 0;
	data->kbc8042_dbbout = 0;
	data->kbc8042_status = 0;
	data->kbc8042_irq_enabled = false;
	data->kbc8042_obe_irq_pending = false;
#endif

	data->acpi_status = 0;

	return emul_init_for_bus(dev);
}

int espi_emul_register(const struct device *dev, struct espi_emul *host_emul)
{
	struct espi_emul_data *data = dev->data;

	LOG_INF("Registering eSPI host emulator: %d (current = %p)\n", host_emul->chipsel,
		data->host_emul);
	__ASSERT_NO_MSG(data->host_emul == NULL);
	__ASSERT_NO_MSG(host_emul != NULL);
	__ASSERT_NO_MSG(host_emul->api != NULL);
	__ASSERT_NO_MSG(host_emul->api->read_lpc_request != NULL);
	__ASSERT_NO_MSG(host_emul->api->write_lpc_request != NULL);
	__ASSERT_NO_MSG(host_emul->api->receive_vwire != NULL);
	__ASSERT_NO_MSG(host_emul->api->send_vwire != NULL);
	__ASSERT_NO_MSG(host_emul->api->manage_callback != NULL);
	__ASSERT_NO_MSG(host_emul->api->raise_event != NULL);

#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	__ASSERT_NO_MSG(host_emul->api->get_acpi_shm != NULL);
#endif

	data->host_emul = host_emul;
	LOG_INF("Registered eSPI host emulator\n");

	return 0;
}

int espi_emul_host_io_read(const struct device *dev, uint8_t length, uint16_t addr, uint32_t *reg)
{
	struct espi_emul_driver_api *api = (struct espi_emul_driver_api *)dev->api;
	struct espi_emul_data *data = dev->data;
	struct espi_emul *host_emul = data->host_emul;

	if (host_emul == NULL) {
		return -EIO;
	}

	return api->host_io_read(dev, length, addr, reg);
}

int espi_emul_host_io_write(const struct device *dev, uint8_t length, uint16_t addr, uint32_t reg)
{
	struct espi_emul_driver_api *api = (struct espi_emul_driver_api *)dev->api;
	struct espi_emul_data *data = dev->data;
	struct espi_emul *host_emul = data->host_emul;

	if (host_emul == NULL) {
		return -EIO;
	}

	return api->host_io_write(dev, length, addr, reg);
}

int espi_emul_host_set_vwire(const struct device *dev, enum espi_vwire_signal vw, uint8_t level)
{
	struct espi_emul_data *data = dev->data;
	struct espi_emul *host_emul = data->host_emul;

	if ((data->channels_enabled & ESPI_CHANNEL_VWIRE) == 0 || host_emul == NULL) {
		return -EIO;
	}

	return host_emul->api->send_vwire(host_emul, vw, level);
}

#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
uintptr_t espi_emul_host_get_acpi_shm(const struct device *dev)
{
	struct espi_emul_data *data = dev->data;
	struct espi_emul *host_emul = data->host_emul;

	__ASSERT_NO_MSG(host_emul != NULL);

	return host_emul->api->get_acpi_shm(host_emul);
}
#endif

int espi_emul_host_manage_callback(const struct device *dev, struct espi_callback *callback,
				   bool set)
{
	struct espi_emul_data *data = dev->data;
	struct espi_emul *host_emul = data->host_emul;

	if (host_emul == NULL) {
		return -EIO;
	}

	return host_emul->api->manage_callback(host_emul, callback, set);
}

int espi_emul_raise_event(const struct device *dev, struct espi_event ev)
{
	struct espi_emul_data *data = dev->data;

	if (data->interrupts_en) {
		espi_send_callbacks(&data->callbacks, dev, ev);
	}

	return 0;
}

/* Device instantiation */
static struct espi_emul_driver_api api = {
	.espi_api = {
			.config = espi_emul_api_config,
			.get_channel_status = espi_emul_api_get_channel_status,
			.read_request = espi_emul_api_read_request,
			.write_request = espi_emul_api_write_request,
			.read_lpc_request = espi_emul_api_lpc_read_request,
			.write_lpc_request = espi_emul_api_lpc_write_request,
			.send_vwire = espi_emul_api_send_vwire,
			.receive_vwire = espi_emul_api_receive_vwire,
			.send_oob = espi_emul_api_send_oob,
			.receive_oob = espi_emul_api_receive_oob,
			.flash_read = espi_emul_api_flash_read,
			.flash_write = espi_emul_api_flash_write,
			.flash_erase = espi_emul_api_flash_erase,
			.manage_callback = espi_emul_api_manage_callback,
		},
	.host_io_read = espi_emul_api_host_io_read,
	.host_io_write = espi_emul_api_host_io_write,
};

BUILD_ASSERT(offsetof(struct espi_emul_driver_api, espi_api) == 0, "Invalid offset of espi_api");

#define EMUL_LINK_AND_COMMA(node_id)                                                               \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
	},

#define ESPI_EMUL_INIT(n)                                                                          \
	static const struct emul_link_for_bus emuls_##n[] = {                                      \
		DT_FOREACH_CHILD(DT_DRV_INST(n), EMUL_LINK_AND_COMMA)};                            \
	static struct emul_list_for_bus espi_emul_cfg_##n = {                                      \
		.children = emuls_##n,                                                             \
		.num_children = ARRAY_SIZE(emuls_##n),                                             \
	};                                                                                         \
	static struct espi_emul_data espi_emul_data_##n;                                           \
	DEVICE_DT_INST_DEFINE(n, &espi_emul_init, NULL, &espi_emul_data_##n, &espi_emul_cfg_##n,   \
			      POST_KERNEL, CONFIG_ESPI_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(ESPI_EMUL_INIT)
