/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <board.h>
#include <device.h>
#include <kernel.h>
#include <init.h>
#include <can.h>

#define CAN_SAM_MCAN_MAX_PORT_COUNT 2

struct can_sam_mcan_config {
	int port_nr;
	void (*config_func)(void);
	struct stm32_pclken pclken;
};

struct can_sam_mcan_data {
	struct device	*clock;
};

#define DEV_CFG(dev) \
	((const struct can_sam_mcan_config * const)(dev)->config->config_info)

#define DEV_DATA(dev) \
	((struct can_sam_mcan_data * const)(dev)->driver_data)


static const struct can_driver_api can_sam_mcan_drv_api_funcs = {
	.set_mode			= 0,
	.set_bitrate			= 0,
	.get_error_counters		= 0,
	.send				= 0,
	.register_recv_callbacks	= 0,
	.register_error_callback	= 0,
};


#ifdef CONFIG_CAN_SAM_MCAN_PORT_1

static struct device DEVICE_NAME_GET(can_sam_mcan_port_1);

static void can_sam_mcan_irq_config_port_1(void)
{
}

static struct can_sam_mcan_data can_sam_mcan_dev_data_port_1 = {
};

static const struct can_sam_mcan_config can_sam_mcan_dev_cfg_port_1 = {
		.port_nr = 1,
		.config_func = can_sam_mcan_irq_config_port_1,
		.pclken = { .bus = STM32_CLOCK_BUS_APB1,
				.enr = LL_APB1_GRP1_PERIPH_CAN1 },
};

DEVICE_AND_API_INIT(can_sam_mcan_port_1, CONFIG_CAN_SAM_MCAN_PORT_1_DEV_NAME,
		can_sam_mcan_init,
		&can_sam_mcan_dev_data_port_1, &can_sam_mcan_dev_cfg_port_1,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&can_sam_mcan_drv_api_funcs);


#endif /* CONFIG_CAN_SAM_MCAN_PORT_1 */

#ifdef CONFIG_CAN_SAM_MCAN_PORT_2

static struct device DEVICE_NAME_GET(can_sam_mcan_port_2);

static void can_sam_mcan_irq_config_port_2(void)
{
}

static struct can_sam_mcan_data can_sam_mcan_dev_data_port_2 = {
		.can = CAN2,
};

static const struct can_sam_mcan_config can_sam_mcan_dev_cfg_port_2 = {
		.port_nr = 2,
		.config_func = can_sam_mcan_irq_config_port_2,
		.pclken = { .bus = STM32_CLOCK_BUS_APB1,
				.enr = LL_APB1_GRP1_PERIPH_CAN2 },
};

DEVICE_AND_API_INIT(can_sam_mcan_port_2, CONFIG_CAN_SAM_MCAN_PORT_2_DEV_NAME,
		can_sam_mcan_init,
		&can_sam_mcan_dev_data_port_2, &can_sam_mcan_dev_cfg_port_2,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&can_sam_mcan_drv_api_funcs);

#endif /* CONFIG_CAN_SAM_MCAN_PORT_2 */
