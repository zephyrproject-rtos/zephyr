/*
 * Copyright (c) 2021-2025 ATL Electronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_uart

/**
 * @brief UART driver for Bouffalo Lab MCU family.
 */
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <soc.h>

#include <bflb_pinctrl.h>
#include <bflb_uart.h>
#include <bflb_glb.h>

#define UART_CTS_FLOWCONTROL_ENABLE (0)
#define UART_RTS_FLOWCONTROL_ENABLE (0)
#define UART_MSB_FIRST_ENABLE	    (0)
#define UART_DEFAULT_RTO_TIMEOUT    (255)
#define UART_CLOCK_DIV		    (0)

struct blfb_config {
	const struct pinctrl_dev_config *pinctrl_cfg;
	uint32_t periph_id;
	UART_CFG_Type uart_cfg;
	UART_FifoCfg_Type fifo_cfg;
};

static int uart_blfb_init(const struct device *dev)
{
	const struct blfb_config *cfg = dev->config;

	pinctrl_apply_state(cfg->pinctrl_cfg, PINCTRL_STATE_DEFAULT);

	GLB_Set_UART_CLK(1, HBN_UART_CLK_160M, UART_CLOCK_DIV);

	UART_IntMask(cfg->periph_id, UART_INT_ALL, 1);
	UART_Disable(cfg->periph_id, UART_TXRX);

	UART_Init(cfg->periph_id, (UART_CFG_Type *)&cfg->uart_cfg);
	UART_TxFreeRun(cfg->periph_id, 1);
	UART_SetRxTimeoutValue(cfg->periph_id, UART_DEFAULT_RTO_TIMEOUT);
	UART_FifoConfig(cfg->periph_id, (UART_FifoCfg_Type *)&cfg->fifo_cfg);
	UART_Enable(cfg->periph_id, UART_TXRX);

	return 0;
}

static int uart_blfb_poll_in(const struct device *dev, unsigned char *c)
{
	const struct blfb_config *cfg = dev->config;

	return UART_ReceiveData(cfg->periph_id, (uint8_t *)c, 1) ? 0 : -1;
}

static void uart_blfb_poll_out(const struct device *dev, unsigned char c)
{
	const struct blfb_config *cfg = dev->config;

	while (UART_GetTxFifoCount(cfg->periph_id) == 0) {
		;
	}

	(void)UART_SendData(cfg->periph_id, (uint8_t *)&c, 1);
}

#ifdef CONFIG_PM_DEVICE
static int uart_blfb_pm_control(const struct device *dev,
				enum pm_device_action action)
{
	const struct blfb_config *cfg = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		(void)pinctrl_apply_state(cfg->pinctrl_cfg, PINCTRL_STATE_DEFAULT);
		UART_Enable(cfg->periph_id, UART_TXRX);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		if (pinctrl_apply_state(cfg->pinctrl_cfg, PINCTRL_STATE_SLEEP)) {
			return -ENOTSUP;
		}
		UART_Disable(cfg->periph_id, UART_TXRX);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(uart, uart_blfb_driver_api) = {
	.poll_in = uart_blfb_poll_in,
	.poll_out = uart_blfb_poll_out,
};

#define BLFB_UART_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);						\
	PM_DEVICE_DT_INST_DEFINE(n, uart_blfb_pm_control);			\
	static const struct blfb_config blfb_uart##n##_config = {		\
		.pinctrl_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.periph_id = DT_INST_PROP(n, peripheral_id),			\
										\
		.uart_cfg.baudRate = DT_INST_PROP(n, current_speed),		\
		.uart_cfg.dataBits = UART_DATABITS_8,				\
		.uart_cfg.stopBits = UART_STOPBITS_1,				\
		.uart_cfg.parity = UART_PARITY_NONE,				\
		.uart_cfg.uartClk = SOC_BOUFFALOLAB_BL_PLL160_FREQ_HZ,		\
		.uart_cfg.ctsFlowControl = UART_CTS_FLOWCONTROL_ENABLE,		\
		.uart_cfg.rtsSoftwareControl = UART_RTS_FLOWCONTROL_ENABLE,	\
		.uart_cfg.byteBitInverse = UART_MSB_FIRST_ENABLE,		\
										\
		.fifo_cfg.txFifoDmaThreshold = 1,				\
		.fifo_cfg.rxFifoDmaThreshold = 1,				\
		.fifo_cfg.txFifoDmaEnable = 0,					\
		.fifo_cfg.rxFifoDmaEnable = 0,					\
	};									\
	DEVICE_DT_INST_DEFINE(n, &uart_blfb_init,				\
			      PM_DEVICE_DT_INST_GET(n),				\
			      NULL,						\
			      &blfb_uart##n##_config, PRE_KERNEL_1,		\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &uart_blfb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BLFB_UART_INIT)
