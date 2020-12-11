/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <init.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(board, CONFIG_LOG_DEFAULT_LEVEL);

#include "board.h"

#define DAPLINK_QSPI_MUX_NODE DT_NODELABEL(daplink_qspi_mux)

#if DT_NODE_HAS_STATUS(DAPLINK_QSPI_MUX_NODE, okay)
int board_daplink_qspi_mux_select(enum board_daplink_qspi_mux_mode mode)
{
	const struct device *gpio;
	gpio_flags_t flags;
	int err;

	switch (mode) {
	case BOARD_DAPLINK_QSPI_MUX_MODE_XIP:
		flags = GPIO_OUTPUT_LOW;
		break;
	case BOARD_DAPLINK_QSPI_MUX_MODE_NORMAL:
		flags = GPIO_OUTPUT_HIGH;
		break;
	default:
		__ASSERT(0, "invalid mode");
		return -EINVAL;
	};

	gpio = device_get_binding(DT_GPIO_LABEL(DAPLINK_QSPI_MUX_NODE,
						mux_gpios));
	if (!gpio) {
		LOG_ERR("DAPLink QSPI MUX GPIO device '%s' not found",
			DT_GPIO_LABEL(DAPLINK_QSPI_MUX_NODE, mux_gpios));
		return -EINVAL;
	}

	err = gpio_config(gpio, DT_GPIO_PIN(DAPLINK_QSPI_MUX_NODE, mux_gpios),
			  DT_GPIO_FLAGS(DAPLINK_QSPI_MUX_NODE, mux_gpios) |
			  flags);
	if (err) {
		LOG_ERR("failed to configure DAPLink QSPI MUX GPIO (err %d)",
			err);
		return err;
	}

	return 0;
}

bool board_daplink_is_fitted(void)
{
	/*
	 * The DAPLINK_fitted_n signal is routed to an IRQ line. It is used as a
	 * level-detect non-interrupt signal to determine if the DAPLink shield
	 * is fitted.
	 */
	return !NVIC_GetPendingIRQ(DT_IRQN(DAPLINK_QSPI_MUX_NODE));
}

static int board_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * Automatically select normal mode unless the DAPLink shield is fitted
	 * in which case the CPU will have the off-board QSPI NOR flash
	 * memory-mapped at 0x0.
	 */
	if (!board_daplink_is_fitted()) {
		board_daplink_qspi_mux_select(
			BOARD_DAPLINK_QSPI_MUX_MODE_NORMAL);
	}

	return 0;
}

SYS_INIT(board_init, POST_KERNEL, CONFIG_BOARD_INIT_PRIORITY);
#endif /* DT_NODE_HAS_STATUS(DAPLINK_QSPI_MUX_NODE, okay) */
