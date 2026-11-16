/*
 * Copyright (c) 2026 Chrispine Tinega <dev@chrispinetinega.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Remoteproc control messages for a TI K3 remote core.
 *
 * TI's Linux kernel stops a K3 remote core by sending RP_MBOX_SHUTDOWN over
 * the mailbox, then waits 3 s for RP_MBOX_SHUTDOWN_ACK before it asserts the
 * core's reset. Without the ACK the stop fails, and Linux has already removed
 * the rpmsg devices by then.
 *
 * This driver sits between the application and the IPM device that carries
 * the mailbox. It answers the request and halts the core, and passes every
 * other message through, so the application uses it as its IPM device and
 * needs no TI-specific code.
 */

#define DT_DRV_COMPAT ti_k3_rproc_ipm

#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ipm_ti_k3_rproc, CONFIG_IPM_LOG_LEVEL);

/* From TI's omap_remoteproc.h, which its K3 remoteproc drivers share. */
#define RP_MBOX_SHUTDOWN     0xFFFFFF14U
#define RP_MBOX_SHUTDOWN_ACK 0xFFFFFF15U

struct ipm_ti_k3_rproc_config {
	const struct device *parent;
};

struct ipm_ti_k3_rproc_data {
	ipm_callback_t callback;
	void *user_data;
};

/*
 * After the ACK, TI's kernel polls the core's status for 2 ms and resets it
 * only once it reports WFI; otherwise the stop fails with -ETIMEDOUT. So the
 * core must halt in WFI, which k_fatal_halt() does not do: its default spins.
 */
static FUNC_NORETURN void halt_for_host_reset(void)
{
	(void)irq_lock();
	for (;;) {
		__asm__ volatile("wfi");
	}
}

static void ipm_ti_k3_rproc_callback(const struct device *parent, void *user_data, uint32_t id,
				     volatile void *msg)
{
	const struct device *dev = user_data;
	struct ipm_ti_k3_rproc_data *data = dev->data;

	if (msg != NULL && *(volatile uint32_t *)msg == RP_MBOX_SHUTDOWN) {
		uint32_t ack = RP_MBOX_SHUTDOWN_ACK;

		if (ipm_send(parent, 0, id, &ack, sizeof(ack)) == 0) {
			/*
			 * The host tears down the vrings and resets this core
			 * once it has the ACK, so nothing more may run.
			 */
			halt_for_host_reset();
		}
		LOG_ERR("could not acknowledge the host's shutdown request");
		return;
	}

	if (data->callback != NULL) {
		data->callback(dev, data->user_data, id, msg);
	}
}

static int ipm_ti_k3_rproc_send(const struct device *dev, int wait, uint32_t id, const void *msg,
				int size)
{
	const struct ipm_ti_k3_rproc_config *config = dev->config;

	return ipm_send(config->parent, wait, id, msg, size);
}

static void ipm_ti_k3_rproc_register_callback(const struct device *dev, ipm_callback_t cb,
					      void *user_data)
{
	struct ipm_ti_k3_rproc_data *data = dev->data;

	data->callback = cb;
	data->user_data = user_data;
}

static int ipm_ti_k3_rproc_max_data_size_get(const struct device *dev)
{
	const struct ipm_ti_k3_rproc_config *config = dev->config;

	return ipm_max_data_size_get(config->parent);
}

static uint32_t ipm_ti_k3_rproc_max_id_val_get(const struct device *dev)
{
	const struct ipm_ti_k3_rproc_config *config = dev->config;

	return ipm_max_id_val_get(config->parent);
}

static int ipm_ti_k3_rproc_set_enabled(const struct device *dev, int enable)
{
	const struct ipm_ti_k3_rproc_config *config = dev->config;

	return ipm_set_enabled(config->parent, enable);
}

static int ipm_ti_k3_rproc_init(const struct device *dev)
{
	const struct ipm_ti_k3_rproc_config *config = dev->config;

	if (!device_is_ready(config->parent)) {
		LOG_ERR("IPM device %s is not ready", config->parent->name);
		return -ENODEV;
	}

	ipm_register_callback(config->parent, ipm_ti_k3_rproc_callback, (void *)dev);

	return 0;
}

static DEVICE_API(ipm, ipm_ti_k3_rproc_api) = {
	.send = ipm_ti_k3_rproc_send,
	.register_callback = ipm_ti_k3_rproc_register_callback,
	.max_data_size_get = ipm_ti_k3_rproc_max_data_size_get,
	.max_id_val_get = ipm_ti_k3_rproc_max_id_val_get,
	.set_enabled = ipm_ti_k3_rproc_set_enabled,
};

#define IPM_TI_K3_RPROC_DEFINE(n)                                                                  \
	static struct ipm_ti_k3_rproc_data ipm_ti_k3_rproc_data_##n;                               \
	static const struct ipm_ti_k3_rproc_config ipm_ti_k3_rproc_config_##n = {                  \
		.parent = DEVICE_DT_GET(DT_INST_PHANDLE(n, ipm_node)),                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ipm_ti_k3_rproc_init, NULL, &ipm_ti_k3_rproc_data_##n,            \
			      &ipm_ti_k3_rproc_config_##n, POST_KERNEL,                            \
			      CONFIG_IPM_TI_K3_RPROC_INIT_PRIORITY, &ipm_ti_k3_rproc_api);

DT_INST_FOREACH_STATUS_OKAY(IPM_TI_K3_RPROC_DEFINE)
