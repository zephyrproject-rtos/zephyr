/*
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CAN_SAM_H_
#define ZEPHYR_DRIVERS_CAN_SAM_H_

#include "can_mcan.h"
#include <soc.h>

#define DEV_DATA(dev) ((struct can_sam_data *)(dev)->driver_data)
#define DEV_CFG(dev) ((const struct can_sam_config *)(dev)->config_info)

struct can_sam_config {
	struct can_mcan_config mcan_cfg;
	void (*config_irq)(void);
	struct soc_gpio_pin pin_list[2];
	u8_t pmc_id;
};

struct can_sam_data {
	struct can_mcan_data mcan_data;
	struct can_mcan_msg_sram msg_ram;
};

#endif /* ZEPHYR_DRIVERS_CAN_SAM_H_ */
