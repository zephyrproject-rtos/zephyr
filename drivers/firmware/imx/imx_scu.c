/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/firmware/imx_scu.h>

/* Used for driver binding */
#define DT_DRV_COMPAT nxp_imx_scu

struct imx_scu_data {
	/* used for communicating with the SCFW */
	sc_ipc_t ipc_handle;
};

struct imx_scu_config {
	uint32_t mu_base_phys;
	uint32_t mu_size;
};

sc_ipc_t imx_scu_get_ipc_handle(const struct device *dev)
{
	return ((struct imx_scu_data*)dev->data)->ipc_handle;
}

static int imx_scu_init(const struct device *dev)
{
	const struct imx_scu_config *cfg;
	struct imx_scu_data *data;
	mm_reg_t mu_base;
	int ret;

	cfg = dev->config;
	data = dev->data;

	device_map(&mu_base, cfg->mu_base_phys, cfg->mu_size, K_MEM_CACHE_NONE);

	/* this will set data->ipc_handle */
	ret = sc_ipc_open(&data->ipc_handle, mu_base);
	if (ret != SC_ERR_NONE)
		return -ENODEV;

	return 0;
}

struct imx_scu_data imx_scu_data;
struct imx_scu_config imx_scu_config = {
	.mu_base_phys = DT_PROP_BY_IDX(DT_NODELABEL(scu), mbox, 0),
	.mu_size = DT_PROP_BY_IDX(DT_NODELABEL(scu), mbox, 1),
};

/* there can only be one system controller node */
DEVICE_DT_INST_DEFINE(0,
		&imx_scu_init,
		NULL,
		&imx_scu_data, &imx_scu_config,
		PRE_KERNEL_1, CONFIG_FIRMWARE_INIT_PRIORITY,
		NULL);
