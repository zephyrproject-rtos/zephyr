/*
 * Copyright 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <soc.h>

#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#define LOG_LEVEL CONFIG_XSPIM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stm32_xpsim);

#define DT_DRV_COMPAT st_stm32_xspim

/* Read-only driver configuration */
struct xspim_stm32_cfg {
	/* XSPIM instance. */
	XSPIM_TypeDef *base;
	/* XSPIM Clock configuration. */
	struct stm32_pclken pclken;
};

struct io_ports_dev_cfg {
	/* XSPIM instance. */
	XSPI_TypeDef *base;
	/* XSPIM Clock configuration. */
	int io_port;
};

#define XSPIM_IO_PORT_ENTRY(node_id, prop, idx, const_port)		\
	{								\
		.base = (XSPI_TypeDef *)DT_REG_ADDR(			\
			DT_PHANDLE_BY_IDX(node_id, prop, idx)),		\
		.io_port = (const_port),				\
	},

#define XSPIM_IO_PORT_DEV(port, value)					\
	DT_INST_FOREACH_PROP_ELEM_VARGS(0, port, XSPIM_IO_PORT_ENTRY, value)

static const struct io_ports_dev_cfg controllers_io_map[] = {
	XSPIM_IO_PORT_DEV(io_port_1, HAL_XSPIM_IOPORT_1)
	XSPIM_IO_PORT_DEV(io_port_2, HAL_XSPIM_IOPORT_2)
};

uint32_t ncs_override[] = {
	HAL_XSPI_CSSEL_OVR_DISABLED,
	HAL_XSPI_CSSEL_OVR_NCS1,
	HAL_XSPI_CSSEL_OVR_NCS2
};

static int xspim_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct xspim_stm32_cfg *cfg = dev->config;
	XSPIM_CfgTypeDef xspi_mgr_cfg;

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

	if (clock_control_on(clk, (clock_control_subsys_t) &cfg->pclken) != 0) {
		return -EIO;
	}

	xspi_mgr_cfg.nCSOverride = ncs_override[DT_INST_ENUM_IDX(0, ncs_override)];
	xspi_mgr_cfg.Req2AckTime = DT_INST_PROP(0, req2ack_time);

	/*
	 * XSPIM configuration requires to unclock all XSPI instance
	 * Hence it can't only be done at bootloader stage to avoid unclocking
	 * XSPI instance controlling the NOR the application it is run from.
	 *
	 * As we're running on bootloader build, don't trust the list of enabled
	 * controllers: it can be changed in application description.
	 * Instead use the list of controllers available in io-port-n properties
	 */
	 for (int i=0; i < ARRAY_SIZE(controllers_io_map) ; ++i) {
		XSPI_HandleTypeDef hxspi = {0};

		hxspi.Instance = controllers_io_map[i].base;

		xspi_mgr_cfg.IOPort = controllers_io_map[i].io_port;

		if (HAL_XSPIM_Config(&hxspi, &xspi_mgr_cfg,
				     HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
			LOG_ERR("XSPIM config failed for dev %p", controllers_io_map[i].base);
			return -EIO;
		}
	}

	return 0;
}

static const struct xspim_stm32_cfg xspim_stm32_cfg = {
	.base = (XSPIM_TypeDef *)DT_INST_REG_ADDR(0),
	.pclken = STM32_DT_INST_CLOCK_INFO(0),
};

DEVICE_DT_INST_DEFINE(0, &xspim_stm32_init, NULL,
		      NULL, &xspim_stm32_cfg,
		      PRE_KERNEL_2, 0, NULL);
