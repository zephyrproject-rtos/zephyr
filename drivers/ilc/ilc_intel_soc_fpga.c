/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_ilc

/**
 * @file
 * @brief ILC driver for Intel FPGA Interrupt Latency Counter Core
 * Reference : Embedded Peripherals IP User Guide : 36. Intel FPGA ILC Core
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/shared_irq.h>
#include <zephyr/drivers/ilc.h>
#include <zephyr/logging/log.h>

/* ILC Control registers offset */
#define ILC_CTRL_REG_OFFSET            0x80
#define ILC_FREQ_REG_OFFSET            0x84
#define ILC_COUNTER_STOP_REG_OFFSET    0x88
#define ILC_READ_DATA_VALID_REG_OFFSET 0x8c
#define ILC_IRQ_ACTIVE_REG_OFFSET      0x90

/* ILC get number of port connected */
#define ILC_IRQ_PORT_COUNT(val) FIELD_GET(GENMASK(7, 2), val)

/* Read Interrupt type */
#define ILC_IRQ_TYPE_GET(val)     FIELD_GET(BIT(1), val)
/* Read ILC version */
#define ILC_CR_VER_READ(val)      FIELD_GET(GENMASK(31, 8), val)
/* Check data valid for given bit in registers */
#define ILC_GET_VLD_BIT(val, pos) FIELD_GET(BIT(pos), val)
/* Global enable bit position */
#define ILC_GLOBAL_ENABLE_BIT     BIT(0)
/* Interrupt pulse type */
#define ILC_IRQ_PULSE_SENSE       1
/* Maximum port count per ILC */
#define ILC_MAX_PORTS             32

LOG_MODULE_REGISTER(ilc_soft_ip_fpga, CONFIG_ILC_SOFT_IP_FPGA_LOG_LEVEL);

typedef void (*shared_irq_config_irq_t)(void);

struct shared_irq_config {
	uint32_t irq_num;
	shared_irq_config_irq_t config;
	uint32_t client_count;
};

struct intel_soc_fpga_ilc_config {
	DEVICE_MMIO_ROM;
	/* Shared irq */
	const struct device *shared_irq[ILC_MAX_PORTS];
	uint32_t port_config;
};

struct intel_soc_fpga_ilc_data {
	DEVICE_MMIO_RAM;
	/* Interrupt Type */
	bool irq_type;
	/* ILC driver enable bit */
	bool enabled;
	/* ILC driver register bit */
	bool ilc_registered;
	/* Active port counter */
	int8_t current_counter;
	/* Number of port connected*/
	uint32_t port_count;
	/* A structure used to submit work after a delay */
	struct k_work_delayable ilc_work_delay;
	/* ILC base address */
	uintptr_t ilc_base_address;
	/* Counter current value */
	uint32_t counter_value_data[ILC_MAX_PORTS];
	/* IRQ registered with ILC*/
	uint32_t irq_table_data[ILC_MAX_PORTS];
};

static int lookup_table_irq(const struct device *ilc_dev, uint32_t irq_number)
{
	struct intel_soc_fpga_ilc_data *ilc_data = ilc_dev->data;

	for (int index = 0; index < ILC_MAX_PORTS; index++) {
		if (ilc_data->irq_table_data[index] == irq_number) {
			return index;
		}
	}
	return -EPERM;
}

static int ilc_intel_soc_fpga_read_port(const struct device *ilc_dev, uint32_t *counter,
					uint8_t port)
{
	struct intel_soc_fpga_ilc_data *ilc_data = ilc_dev->data;

	if ((counter == NULL) || (port >= ilc_data->port_count)) {
		LOG_ERR("Invalid input parameters");
		return -EINVAL;
	}

	if (!ilc_data->enabled) {
		LOG_ERR("Device not enabled");
		return -ENODEV;
	}

	*counter = ilc_data->counter_value_data[port];

	return 0;
}

static int ilc_intel_soc_fpga_read_params(const struct device *ilc_dev, struct ilc_params *params)
{
	struct intel_soc_fpga_ilc_data *ilc_data = ilc_dev->data;
	uintptr_t ilc_base_address = DEVICE_MMIO_GET(ilc_dev);

	if (!ilc_data->enabled) {
		LOG_ERR("Device not enabled");
		return -ENODEV;
	}

	if (params == NULL) {
		LOG_ERR("Invalid parameter passed");
		return -EINVAL;
	}

	params->port_count = ilc_data->port_count;
	params->frequency = sys_read32(ilc_base_address + ILC_FREQ_REG_OFFSET);

	return 0;
}

static void ilc_delayed_work(struct k_work *work)
{
	uint32_t reg_status = 0;
	uint8_t active_counter = 0;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct intel_soc_fpga_ilc_data *ilc_data;

	/* To get structure base address using structure member */
	ilc_data = CONTAINER_OF(dwork, struct intel_soc_fpga_ilc_data, ilc_work_delay);

	uintptr_t ilc_base_address = ilc_data->ilc_base_address;

	/* To store active port counter */
	active_counter = ilc_data->current_counter;
	reg_status = sys_read32(ilc_base_address + ILC_READ_DATA_VALID_REG_OFFSET);
	if (ILC_GET_VLD_BIT(reg_status, active_counter)) {

		/* Reading counter value register*/
		ilc_data->counter_value_data[active_counter] =
			sys_read32(ilc_base_address + (active_counter * 4));
		reg_status = sys_read32(ilc_base_address + ILC_CTRL_REG_OFFSET);

		if (ILC_IRQ_TYPE_GET(reg_status) == ILC_IRQ_PULSE_SENSE) {
			/* Clear counter stop bit */
			sys_clear_bit(ilc_base_address + ILC_COUNTER_STOP_REG_OFFSET,
				      active_counter);
		}
		/* Make default value to active port counter */
		ilc_data->current_counter = -1;
		return;
	}
	/* Polling until get valid data */
	k_work_schedule(&ilc_data->ilc_work_delay, K_NO_WAIT);
}
static int ilc_interrupt_isr(const struct device *ilc_dev, uint32_t irq_number)

{
	uint32_t reg_status = 0;
	uint8_t active_counter_port = 0;
	struct intel_soc_fpga_ilc_data *ilc_data = ilc_dev->data;
	uintptr_t ilc_base_address = DEVICE_MMIO_GET(ilc_dev);

	reg_status = lookup_table_irq(ilc_dev, irq_number);

	if (reg_status != 0) {
		LOG_ERR("IRQ number not found");
		return reg_status;
	}
	/* update active port irq*/
	active_counter_port = reg_status;
	ilc_data->current_counter = active_counter_port;

	if (ilc_data->irq_type == ILC_IRQ_PULSE_SENSE) {
		/* Set counter stop register*/
		sys_set_bit(ilc_base_address + ILC_COUNTER_STOP_REG_OFFSET, active_counter_port);
	}
	/* Start workqueue to poll valid data */
	k_work_schedule(&ilc_data->ilc_work_delay, K_NO_WAIT);
	return 0;
}

int ilc_intel_soc_fpga_enable(const struct device *ilc_dev)
{
	uint32_t control_reg;
	int ret = 0;
	struct intel_soc_fpga_ilc_data *ilc_data = ilc_dev->data;
	const struct intel_soc_fpga_ilc_config *ilc_config = ilc_dev->config;
	uintptr_t ilc_base_address = DEVICE_MMIO_GET(ilc_dev);

	ilc_data->current_counter = -1;

	if (!ilc_data->enabled) {
		/* Read ILC control register */
		control_reg = sys_read32(ilc_base_address + ILC_CTRL_REG_OFFSET);
		ilc_data->irq_type = ILC_IRQ_TYPE_GET(control_reg);
		ilc_data->port_count = ILC_IRQ_PORT_COUNT(control_reg);

		if (ilc_config->port_config != ilc_data->port_count) {
			LOG_ERR("DT config port number and ILC config miss match");
			return -ENOENT;
		}
		if (!ilc_data->port_count) {
			LOG_ERR("No peripheral is connected to ILC");
			return -ENOENT;
		}

		for (int index = 0; index < ilc_data->port_count; index++) {
			ilc_data->counter_value_data[index] = 0;
			struct shared_irq_config *shared_irq_info =
				(struct shared_irq_config *)ilc_config->shared_irq[index]->config;
			ilc_data->irq_table_data[index] = shared_irq_info->irq_num;

			if (!ilc_data->ilc_registered) {
				if (!device_is_ready(ilc_config->shared_irq[index])) {
					LOG_ERR("Shared interrupt device not ready");
					return -ENODEV;
				}

				ret = shared_irq_isr_register(ilc_config->shared_irq[index],
							      &ilc_interrupt_isr, ilc_dev);
				if (ret != 0) {
					LOG_ERR("Shared Interrupt register failed for %d", index);
					return ret;
				}
			}

			ret = shared_irq_enable(ilc_config->shared_irq[index], ilc_dev);
			if (ret != 0) {
				LOG_ERR("Shared Interrupt enable failed for %d", index);
				return ret;
			}
		}
		/* enable shared irq register functionally */
		ilc_data->ilc_registered = true;
		/* Initialize delayed work queue */
		k_work_init_delayable(&ilc_data->ilc_work_delay, ilc_delayed_work);

		/* Enable ILC */
		sys_set_bit((ilc_base_address + ILC_CTRL_REG_OFFSET), ILC_GLOBAL_ENABLE_BIT);
		ilc_data->enabled = true;
	} else {
		LOG_WRN("Already enabled");
	}
	return ret;
}

int ilc_intel_soc_fpga_disable(const struct device *ilc_dev)
{
	int ret = 0;
	struct intel_soc_fpga_ilc_data *ilc_data = ilc_dev->data;
	const struct intel_soc_fpga_ilc_config *ilc_config = ilc_dev->config;
	uintptr_t ilc_base_address = DEVICE_MMIO_GET(ilc_dev);

	ilc_data->ilc_base_address = ilc_base_address;
	if (ilc_data->enabled) {
		for (int index = 0; index < ilc_data->port_count; index++) {
			ret = shared_irq_disable(ilc_config->shared_irq[index], ilc_dev);
			if (ret != 0) {
				LOG_ERR("Shared Interrupt disable failed for %d", index);
				return ret;
			}
		}

		/* Disable ILC */
		sys_clear_bit((ilc_base_address + ILC_CTRL_REG_OFFSET), ILC_GLOBAL_ENABLE_BIT);
		/* Disable ilc api functionally */
		ilc_data->enabled = false;
	} else {
		LOG_WRN("Already disabled");
	}
	return ret;
}

static const struct ilc_driver_api ilc_intel_soc_fpga_driver_api = {
	.enable = ilc_intel_soc_fpga_enable,
	.disable = ilc_intel_soc_fpga_disable,
	.read_params = ilc_intel_soc_fpga_read_params,
	.read_counter = ilc_intel_soc_fpga_read_port,
};

int ilc_intel_soc_fpga_init(const struct device *ilc_dev)
{
	DEVICE_MMIO_MAP(ilc_dev, K_MEM_CACHE_NONE);
	struct intel_soc_fpga_ilc_data *ilc_data = ilc_dev->data;

	/* Default disable ilc api functionally */
	ilc_data->enabled = false;
	/* Default disable shared irq register functionally */
	ilc_data->ilc_registered = false;
	return 0;
}

#define Z_DT_SHARED_IRQ_INIT(i, node_id)                                                           \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(DT_DRV_INST(node_id), shared_irqs, i))

#define ILC_SHARED_IRQ_INIT(node_id)                                                               \
	LISTIFY(DT_INST_PROP(node_id, shared_irq_count), Z_DT_SHARED_IRQ_INIT, (,), node_id)

#define INTEL_SOC_FPGA_ILC_INIT(inst)                                                              \
	static struct intel_soc_fpga_ilc_data intel_soc_fpga_ilc_data_##inst;                      \
	static const struct intel_soc_fpga_ilc_config intel_soc_fpga_ilc_config_##inst = {         \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                                           \
		.port_config = DT_INST_PROP(inst, shared_irq_count),                               \
		.shared_irq = {ILC_SHARED_IRQ_INIT(inst)}};                                        \
												   \
	DEVICE_DT_INST_DEFINE(inst, ilc_intel_soc_fpga_init, NULL,                                 \
			      &intel_soc_fpga_ilc_data_##inst, &intel_soc_fpga_ilc_config_##inst,  \
			      POST_KERNEL, CONFIG_ILC_INIT_PRIORITY,                               \
			      &ilc_intel_soc_fpga_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INTEL_SOC_FPGA_ILC_INIT);
