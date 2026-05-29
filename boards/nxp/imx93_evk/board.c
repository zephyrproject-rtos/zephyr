/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(board_control, CONFIG_BOARD_MIMX93_EVK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(imx93evk_exp_sel) && IS_ENABLED(CONFIG_BOARD_MIMX93_EVK_EXP_SEL_INIT)

#define BOARD_EXP_SEL_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(imx93evk_exp_sel)

#define BOARD_EXP_SEL_MUX_A (0U)
#define BOARD_EXP_SEL_MUX_B (1U)

static int board_init_exp_sel(void)
{
	int rc = 0;
	const struct gpio_dt_spec mux = GPIO_DT_SPEC_GET(BOARD_EXP_SEL_NODE, mux_gpios);
	uint32_t pin_state = DT_ENUM_IDX(BOARD_EXP_SEL_NODE, mux);

	if (!gpio_is_ready_dt(&mux)) {
		LOG_ERR("EXP_SEL Pin port is not ready");
		return -ENODEV;
	}

#if defined(CONFIG_CAN)
	if (pin_state != BOARD_EXP_SEL_MUX_A) {
		LOG_WRN("CAN is enabled, EXP_SEL overrides to A");
		pin_state = BOARD_EXP_SEL_MUX_A;
	}
#endif /* CONFIG_CAN */

	rc = gpio_pin_configure_dt(&mux, pin_state);
	if (rc) {
		LOG_ERR("Write EXP_SEL Pin error %d", rc);
		return rc;
	}
	LOG_INF("EXP_SEL mux %c with priority %d", pin_state ? 'B' : 'A',
		CONFIG_BOARD_MIMX93_EVK_EXP_SEL_INIT_PRIO);

	return 0;
}

SYS_INIT(board_init_exp_sel, POST_KERNEL, CONFIG_BOARD_MIMX93_EVK_EXP_SEL_INIT_PRIO);

#endif
/*
 * DT_HAS_COMPAT_STATUS_OKAY(imx93evk_exp_sel) && \
 * IS_ENABLED(CONFIG_BOARD_MIMX93_EVK_EXP_SEL_INIT)
 */
