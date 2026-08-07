/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_zynqmp_ipi_mailbox

#include "ipm_xlnx_ipi.h"

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ipm_xlnx_ipi, CONFIG_IPM_LOG_LEVEL);

#define XLNX_IPI_MAX_BUF_SIZE_BYTES 32

struct xlnx_ipi_data {
	size_t len;
	void *user_data;
	uint8_t data[];
};

struct xlnx_ipi_reg_info {
	uint32_t ipi_ch_bit;
};

static const struct xlnx_ipi_reg_info xlnx_ipi_reg_info_zynqmp[] = {
	{.ipi_ch_bit = IPI_CH0_BIT},  /* IPI CH ID 0 - Default APU  */
	{.ipi_ch_bit = IPI_CH1_BIT},  /* IPI CH ID 1 - Default RPU0 */
	{.ipi_ch_bit = IPI_CH2_BIT},  /* IPI CH ID 2 - Default RPU1 */
	{.ipi_ch_bit = IPI_CH3_BIT},  /* IPI CH ID 3 - Default PMU0 */
	{.ipi_ch_bit = IPI_CH4_BIT},  /* IPI CH ID 4 - Default PMU1 */
	{.ipi_ch_bit = IPI_CH5_BIT},  /* IPI CH ID 5 - Default PMU2 */
	{.ipi_ch_bit = IPI_CH6_BIT},  /* IPI CH ID 6 - Default PMU3 */
	{.ipi_ch_bit = IPI_CH7_BIT},  /* IPI CH ID 7 - Default PL0 */
	{.ipi_ch_bit = IPI_CH8_BIT},  /* IPI CH ID 8 - Default PL1 */
	{.ipi_ch_bit = IPI_CH9_BIT},  /* IPI CH ID 9 - Default PL2 */
	{.ipi_ch_bit = IPI_CH10_BIT}, /* IPI CH ID 10 - Default PL3 */
};

struct xlnx_ipi_config {
	uint32_t ipi_ch_bit;
	uint32_t host_ipi_reg;
	int (*xlnx_ipi_config_func)(const struct device *dev);
	const struct device **cdev_list;
	int num_cdev;
};

struct xlnx_ipi_child_data {
	bool enabled;
	ipm_callback_t ipm_callback;
	void *user_data;
};

struct xlnx_ipi_child_config {
	const char *node_id;
	uint32_t local_request_region;
	uint32_t local_response_region;
	uint32_t remote_request_region;
	uint32_t remote_response_region;
	uint32_t host_ipi_reg;
	uint32_t remote_ipi_id;
	uint32_t remote_ipi_ch_bit;
};

static void xlnx_mailbox_rx_isr(const struct device *dev)
{
	const struct xlnx_ipi_config *config;
	const struct device **cdev_list;
	const struct xlnx_ipi_child_config *cdev_conf;
	const struct xlnx_ipi_child_data *cdev_data;
	uint8_t ipi_buf[XLNX_IPI_MAX_BUF_SIZE_BYTES + sizeof(struct xlnx_ipi_data)];
	int num_cdev;
	struct xlnx_ipi_data *msg;
	const struct device *cdev;
	uint32_t remote_ipi_ch_bit;
	int i, j;

	config = dev->config;
	cdev_list = config->cdev_list;
	num_cdev = config->num_cdev;
	msg = (struct xlnx_ipi_data *)ipi_buf;
	for (i = 0; i < num_cdev; i++) {
		cdev = cdev_list[i];
		cdev_conf = cdev->config;
		cdev_data = cdev->data;

		if (!cdev_data->enabled) {
			continue;
		}

		remote_ipi_ch_bit = cdev_conf->remote_ipi_ch_bit;
		if (!sys_test_bit(config->host_ipi_reg + IPI_ISR, remote_ipi_ch_bit)) {
			continue;
		}

		msg->len = XLNX_IPI_MAX_BUF_SIZE_BYTES;
		msg->user_data = cdev_data->user_data;
		for (j = 0; j < XLNX_IPI_MAX_BUF_SIZE_BYTES; j++) {
			msg->data[j] = sys_read8(cdev_conf->remote_request_region + j);
		}
		if (cdev_data->ipm_callback) {
			cdev_data->ipm_callback(cdev, cdev_data->user_data,
						cdev_conf->remote_ipi_id, msg);
		}
		sys_set_bit(config->host_ipi_reg + IPI_ISR, remote_ipi_ch_bit);
	}
}

static int xlnx_ipi_send(const struct device *ipmdev, int wait, uint32_t id, const void *data,
			 int size)
{
	const uint8_t *msg = (uint8_t *)data;
	const struct xlnx_ipi_child_config *config = ipmdev->config;
	unsigned int key;
	int i, obs_bit;

	ARG_UNUSED(id);

	if (size > XLNX_IPI_MAX_BUF_SIZE_BYTES) {
		return -EMSGSIZE;
	}

	key = irq_lock();
	if (msg) {
		/* Write buffer to send data */
		for (i = 0; i < size; i++) {
			sys_write8(msg[i], config->local_request_region + i);
		}
	}
	irq_unlock(key);

	sys_set_bit(config->host_ipi_reg + IPI_TRIG, config->remote_ipi_ch_bit);

	obs_bit = 0;
	do {
		obs_bit = sys_test_bit(config->host_ipi_reg + IPI_OBS, config->remote_ipi_ch_bit);
	} while (obs_bit && wait);

	return 0;
}

static void xlnx_ipi_register_callback(const struct device *port, ipm_callback_t cb,
				       void *user_data)
{
	struct xlnx_ipi_child_data *data = port->data;

	data->ipm_callback = cb;
	data->user_data = user_data;
}

static int xlnx_ipi_max_data_size_get(const struct device *ipmdev)
{
	return XLNX_IPI_MAX_BUF_SIZE_BYTES;
}

static uint32_t xlnx_ipi_max_id_val_get(const struct device *ipmdev)
{
	return UINT32_MAX;
}

static int xlnx_ipi_set_enabled(const struct device *ipmdev, int enable)
{
	const struct xlnx_ipi_child_config *config = ipmdev->config;
	struct xlnx_ipi_child_data *data = ipmdev->data;

	if (enable) {
		sys_set_bit(config->host_ipi_reg + IPI_IER, config->remote_ipi_ch_bit);
	} else {
		sys_set_bit(config->host_ipi_reg + IPI_IDR, config->remote_ipi_ch_bit);
	}

	/* If IPI channel bit in IPI Mask Register is not set, then interrupt is enabled */
	if (!sys_test_bit(config->host_ipi_reg + IPI_IMR, config->remote_ipi_ch_bit)) {
		data->enabled = enable;
		return 0;
	}

	return -EINVAL;
}

static int xlnx_ipi_init(const struct device *dev)
{
	const struct xlnx_ipi_config *conf = dev->config;

	/* disable all the interrupts */
	sys_write32(0xFFFFFFFF, conf->host_ipi_reg + IPI_IDR);

	/* clear status of any previous interrupts */
	sys_write32(0xFFFFFFFF, conf->host_ipi_reg + IPI_ISR);

	conf->xlnx_ipi_config_func(dev);

	return 0;
}

static DEVICE_API(ipm, xlnx_ipi_api) = {
	.send = xlnx_ipi_send,
	.register_callback = xlnx_ipi_register_callback,
	.max_data_size_get = xlnx_ipi_max_data_size_get,
	.max_id_val_get = xlnx_ipi_max_id_val_get,
	.set_enabled = xlnx_ipi_set_enabled,
};

#define GET_CHILD_DEV(node_id) DEVICE_DT_GET(node_id),

#define XLNX_IPI_CHILD(ch_node)                                                                    \
	struct xlnx_ipi_child_data xlnx_ipi_child_data##ch_node = {                                \
		.enabled = false,                                                                  \
		.ipm_callback = NULL,                                                              \
	};                                                                                         \
	struct xlnx_ipi_child_config xlnx_ipi_child_config##ch_node = {                            \
		.local_request_region = DT_REG_ADDR_BY_NAME(ch_node, local_request_region),        \
		.local_response_region = DT_REG_ADDR_BY_NAME(ch_node, local_response_region),      \
		.remote_request_region = DT_REG_ADDR_BY_NAME(ch_node, remote_request_region),      \
		.remote_response_region = DT_REG_ADDR_BY_NAME(ch_node, remote_response_region),    \
		.remote_ipi_id = DT_PROP(ch_node, remote_ipi_id),                                  \
		.remote_ipi_ch_bit =                                                               \
			xlnx_ipi_reg_info_zynqmp[DT_PROP(ch_node, remote_ipi_id)].ipi_ch_bit,      \
		.host_ipi_reg = DT_REG_ADDR_BY_NAME(DT_PARENT(ch_node), host_ipi_reg),             \
	};                                                                                         \
	DEVICE_DT_DEFINE(ch_node, NULL, NULL, &xlnx_ipi_child_data##ch_node,			   \
			 &xlnx_ipi_child_config##ch_node, POST_KERNEL,                             \
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &xlnx_ipi_api);

#define XLNX_IPI(inst)                                                                             \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, XLNX_IPI_CHILD);                                   \
	static const struct device *cdev##inst[] = {                                               \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, GET_CHILD_DEV)};                           \
	static int xlnx_ipi_config_func##inst(const struct device *dev);                           \
	struct xlnx_ipi_config xlnx_ipi_config##inst = {                                           \
		.host_ipi_reg = DT_INST_REG_ADDR_BY_NAME(inst, host_ipi_reg),                      \
		.xlnx_ipi_config_func = xlnx_ipi_config_func##inst,                                \
		.cdev_list = cdev##inst,                                                           \
		.num_cdev = ARRAY_SIZE(cdev##inst),                                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &xlnx_ipi_init, NULL, NULL, /* data */                         \
			      &xlnx_ipi_config##inst,           /* conf */                         \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);             \
	static int xlnx_ipi_config_func##inst(const struct device *dev)                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), xlnx_mailbox_rx_isr,  \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
		LOG_DBG("irq %d is enabled: %s\n", DT_INST_IRQN(inst),                             \
			irq_is_enabled(DT_INST_IRQN(inst)) ? "true" : "false");                    \
		return 0;                                                                          \
	}

DT_INST_FOREACH_STATUS_OKAY(XLNX_IPI)
