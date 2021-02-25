/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_uart

#include <sys/__assert.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <drivers/clock_control.h>
#include <kernel.h>
#include <power/power.h>
#include <soc.h>
#include "soc_miwu.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(uart_npcx, LOG_LEVEL_ERR);

/* Driver config */
struct uart_npcx_config {
	struct uart_device_config uconf;
	/* clock configuration */
	struct npcx_clk_cfg clk_cfg;
	/* int-mux configuration */
	const struct npcx_wui uart_rx_wui;
	/* pinmux configuration */
	const uint8_t   alts_size;
	const struct npcx_alt *alts_list;
};

/* Driver data */
struct uart_npcx_data {
	/* Baud rate */
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif
#ifdef CONFIG_PM_DEVICE
	uint32_t pm_state;
#endif
};

/* Driver convenience defines */
#define DRV_CONFIG(dev) \
	((const struct uart_npcx_config *)(dev)->config)

#define DRV_DATA(dev) \
	((struct uart_npcx_data *)(dev)->data)

#define HAL_INSTANCE(dev) \
	(struct uart_reg *)(DRV_CONFIG(dev)->uconf.base)

/* UART local functions */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_npcx_tx_fifo_ready(const struct device *dev)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);

	/* True if the Tx FIFO is not completely full */
	return !(GET_FIELD(inst->UFTSTS, NPCX_UFTSTS_TEMPTY_LVL) == 0);
}

static int uart_npcx_rx_fifo_available(const struct device *dev)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);

	/* True if at least one byte is in the Rx FIFO */
	return IS_BIT_SET(inst->UFRSTS, NPCX_UFRSTS_RFIFO_NEMPTY_STS);
}

static void uart_npcx_dis_all_tx_interrupts(const struct device *dev)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);

	/* Disable all Tx interrupts */
	inst->UFTCTL &= ~(BIT(NPCX_UFTCTL_TEMPTY_LVL_EN) |
				BIT(NPCX_UFTCTL_TEMPTY_EN) |
				BIT(NPCX_UFTCTL_NXMIPEN));
}

static void uart_npcx_clear_rx_fifo(const struct device *dev)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);
	uint8_t scratch;

	/* Read all dummy bytes out from Rx FIFO */
	while (uart_npcx_rx_fifo_available(dev))
		scratch = inst->URBUF;
}

static int uart_npcx_fifo_fill(const struct device *dev,
				  const uint8_t *tx_data,
				  int size)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);
	uint8_t tx_bytes = 0U;

	/* If Tx FIFO is still ready to send */
	while ((size - tx_bytes > 0) && uart_npcx_tx_fifo_ready(dev)) {
		/* Put a character into Tx FIFO */
		inst->UTBUF = tx_data[tx_bytes++];
	}

	return tx_bytes;
}

static int uart_npcx_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int size)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);
	unsigned int rx_bytes = 0U;

	/* If least one byte is in the Rx FIFO */
	while ((size - rx_bytes > 0) && uart_npcx_rx_fifo_available(dev)) {
		/* Receive one byte from Rx FIFO */
		rx_data[rx_bytes++] = inst->URBUF;
	}

	return rx_bytes;
}

static void uart_npcx_irq_tx_enable(const struct device *dev)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);

	inst->UFTCTL |= BIT(NPCX_UFTCTL_TEMPTY_EN);
}

static void uart_npcx_irq_tx_disable(const struct device *dev)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);

	inst->UFTCTL &= ~(BIT(NPCX_UFTCTL_TEMPTY_EN));
}

static int uart_npcx_irq_tx_ready(const struct device *dev)
{
	return uart_npcx_tx_fifo_ready(dev);
}

static int uart_npcx_irq_tx_complete(const struct device *dev)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);

	/* Tx FIFO is empty or last byte is sending */
	return IS_BIT_SET(inst->UFTSTS, NPCX_UFTSTS_NXMIP);
}

static void uart_npcx_irq_rx_enable(const struct device *dev)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);

	inst->UFRCTL |= BIT(NPCX_UFRCTL_RNEMPTY_EN);
}

static void uart_npcx_irq_rx_disable(const struct device *dev)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);

	inst->UFRCTL &= ~(BIT(NPCX_UFRCTL_RNEMPTY_EN));
}

static int uart_npcx_irq_rx_ready(const struct device *dev)
{
	return uart_npcx_rx_fifo_available(dev);
}

static void uart_npcx_irq_err_enable(const struct device *dev)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);

	inst->UICTRL |= BIT(NPCX_UICTRL_EEI);
}

static void uart_npcx_irq_err_disable(const struct device *dev)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);

	inst->UICTRL &= ~(BIT(NPCX_UICTRL_EEI));
}

static int uart_npcx_irq_is_pending(const struct device *dev)
{
	return (uart_npcx_irq_tx_ready(dev)
		|| uart_npcx_irq_rx_ready(dev));
}

static int uart_npcx_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static void uart_npcx_irq_callback_set(const struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_npcx_data *data = DRV_DATA(dev);

	data->user_cb = cb;
	data->user_data = cb_data;
}

static void uart_npcx_isr(const struct device *dev)
{
	struct uart_npcx_data *data = DRV_DATA(dev);

	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
}

/*
 * Poll-in implementation for interrupt driven config, forward call to
 * uart_npcx_fifo_read().
 */
static int uart_npcx_poll_in(const struct device *dev, unsigned char *c)
{
	return uart_npcx_fifo_read(dev, c, 1) ? 0 : -1;
}

/*
 * Poll-out implementation for interrupt driven config, forward call to
 * uart_npcx_fifo_fill().
 */
static void uart_npcx_poll_out(const struct device *dev, unsigned char c)
{
	while (!uart_npcx_fifo_fill(dev, &c, 1))
		continue;
}

#else /* !CONFIG_UART_INTERRUPT_DRIVEN */

/*
 * Poll-in implementation for byte mode config, read byte from URBUF if
 * available.
 */
static int uart_npcx_poll_in(const struct device *dev, unsigned char *c)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);

	/* Rx single byte buffer is not full */
	if (!IS_BIT_SET(inst->UICTRL, NPCX_UICTRL_RBF))
		return -1;

	*c = inst->URBUF;
	return 0;
}

/*
 * Poll-out implementation for byte mode config, write byte to UTBUF if empty.
 */
static void uart_npcx_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);

	/* Wait while Tx single byte buffer is ready to send */
	while (!IS_BIT_SET(inst->UICTRL, NPCX_UICTRL_TBE))
		continue;

	inst->UTBUF = c;
}
#endif /* !CONFIG_UART_INTERRUPT_DRIVEN */

/* UART api functions */
static int uart_npcx_err_check(const struct device *dev)
{
	struct uart_reg *const inst = HAL_INSTANCE(dev);
	uint32_t err = 0U;
	uint8_t stat = inst->USTAT;

	if (IS_BIT_SET(stat, NPCX_USTAT_DOE))
		err |= UART_ERROR_OVERRUN;

	if (IS_BIT_SET(stat, NPCX_USTAT_PE))
		err |= UART_ERROR_PARITY;

	if (IS_BIT_SET(stat, NPCX_USTAT_FE))
		err |= UART_ERROR_FRAMING;

	return err;
}

/* UART driver registration */
static const struct uart_driver_api uart_npcx_driver_api = {
	.poll_in = uart_npcx_poll_in,
	.poll_out = uart_npcx_poll_out,
	.err_check = uart_npcx_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_npcx_fifo_fill,
	.fifo_read = uart_npcx_fifo_read,
	.irq_tx_enable = uart_npcx_irq_tx_enable,
	.irq_tx_disable = uart_npcx_irq_tx_disable,
	.irq_tx_ready = uart_npcx_irq_tx_ready,
	.irq_tx_complete = uart_npcx_irq_tx_complete,
	.irq_rx_enable = uart_npcx_irq_rx_enable,
	.irq_rx_disable = uart_npcx_irq_rx_disable,
	.irq_rx_ready = uart_npcx_irq_rx_ready,
	.irq_err_enable = uart_npcx_irq_err_enable,
	.irq_err_disable = uart_npcx_irq_err_disable,
	.irq_is_pending = uart_npcx_irq_is_pending,
	.irq_update = uart_npcx_irq_update,
	.irq_callback_set = uart_npcx_irq_callback_set,
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_npcx_init(const struct device *dev)
{
	const struct uart_npcx_config *const config = DRV_CONFIG(dev);
	const struct uart_npcx_data *const data = DRV_DATA(dev);
	struct uart_reg *const inst = HAL_INSTANCE(dev);
	const struct device *const clk_dev =
					device_get_binding(NPCX_CLK_CTRL_NAME);
	uint32_t uart_rate;
	int ret;

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t *)
							&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on UART clock fail %d", ret);
		return ret;
	}

	/*
	 * If apb2's clock is not 15MHz, we need to find the other optimized
	 * values of UPSR and UBAUD for baud rate 115200.
	 */
	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t *)
			&config->clk_cfg, &uart_rate);
	if (ret < 0) {
		LOG_ERR("Get UART clock rate error %d", ret);
		return ret;
	}
	__ASSERT(uart_rate == 15000000, "Unsupported apb2 clock for UART!");

	/* Fix baud rate to 115200 */
	if (data->baud_rate  == 115200) {
		inst->UPSR = 0x38;
		inst->UBAUD = 0x01;
	} else
		return -EINVAL;

	/*
	 * 8-N-1, FIFO enabled.  Must be done after setting
	 * the divisor for the new divisor to take effect.
	 */
	inst->UFRS = 0x00;

	/* Initialize UART FIFO if mode is interrupt driven */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Enable the UART FIFO mode */
	inst->UMDSL |= BIT(NPCX_UMDSL_FIFO_MD);

	/* Disable all UART tx FIFO interrupts */
	uart_npcx_dis_all_tx_interrupts(dev);

	/* Clear UART rx FIFO */
	uart_npcx_clear_rx_fifo(dev);

	/* Configure UART interrupts */
	config->uconf.irq_config_func(dev);
#endif

#if defined(CONFIG_PM)
	/*
	 * Configure the UART wake-up event triggered from a falling edge
	 * on CR_SIN pin. No need for callback function.
	 */
	npcx_miwu_interrupt_configure(&config->uart_rx_wui,
			NPCX_MIWU_MODE_EDGE, NPCX_MIWU_TRIG_LOW);

	/* Enable irq of interrupt-input module */
	npcx_miwu_irq_enable(&config->uart_rx_wui);
#endif

	/* Configure pin-mux for uart device */
	npcx_pinctrl_mux_configure(config->alts_list, config->alts_size, 1);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static inline bool uart_npcx_device_is_transmitting(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)) {
		/* The transmitted transaction is completed? */
		return !uart_npcx_irq_tx_complete(dev);
	}

	/* No need for polling mode */
	return 0;
}

static inline int uart_npcx_get_power_state(const struct device *dev,
							uint32_t *state)
{
	const struct uart_npcx_data *const data = DRV_DATA(dev);

	*state = data->pm_state;
	return 0;
}

static inline int uart_npcx_set_power_state(const struct device *dev,
							uint32_t next_state)
{
	struct uart_npcx_data *const data = DRV_DATA(dev);

	/* If next device power state is LOW or SUSPEND power state */
	if (next_state == DEVICE_PM_LOW_POWER_STATE ||
	    next_state == DEVICE_PM_SUSPEND_STATE) {
		/*
		 * If uart device is busy with transmitting, the driver will
		 * stay in while loop and wait for the transaction is completed.
		 */
		while (uart_npcx_device_is_transmitting(dev)) {
			continue;
		}
	}

	data->pm_state = next_state;
	return 0;
}

/* Implements the device power management control functionality */
static int uart_npcx_pm_control(const struct device *dev, uint32_t ctrl_command,
				 void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;

	switch (ctrl_command) {
	case DEVICE_PM_SET_POWER_STATE:
		ret = uart_npcx_set_power_state(dev, *((uint32_t *)context));
		break;
	case DEVICE_PM_GET_POWER_STATE:
		ret = uart_npcx_get_power_state(dev, (uint32_t *)context);
		break;
	default:
		ret = -EINVAL;
	}

	if (cb != NULL) {
		cb(dev, ret, context, arg);
	}
	return ret;
}
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define NPCX_UART_IRQ_CONFIG_FUNC_DECL(inst) \
	static void uart_npcx_irq_config_##inst(const struct device *dev)
#define NPCX_UART_IRQ_CONFIG_FUNC_INIT(inst) \
	.irq_config_func = uart_npcx_irq_config_##inst,
#define NPCX_UART_IRQ_CONFIG_FUNC(inst)	                                       \
	static void uart_npcx_irq_config_##inst(const struct device *dev)      \
	{	                                                               \
		IRQ_CONNECT(DT_INST_IRQN(inst),		                       \
			DT_INST_IRQ(inst, priority),                           \
			uart_npcx_isr,                                         \
			DEVICE_DT_INST_GET(inst),                              \
			0);                                                    \
		irq_enable(DT_INST_IRQN(inst));		                       \
	}
#else
#define NPCX_UART_IRQ_CONFIG_FUNC_DECL(inst)
#define NPCX_UART_IRQ_CONFIG_FUNC_INIT(inst)
#define NPCX_UART_IRQ_CONFIG_FUNC(inst)
#endif

#define NPCX_UART_INIT(inst)                                                   \
	NPCX_UART_IRQ_CONFIG_FUNC_DECL(inst);	                               \
									       \
	static const struct npcx_alt uart_alts##inst[] =		       \
					NPCX_DT_ALT_ITEMS_LIST(inst);	       \
									       \
	static const struct uart_npcx_config uart_npcx_cfg_##inst = {	       \
		.uconf = {                                                     \
			.base = (uint8_t *)DT_INST_REG_ADDR(inst),             \
			NPCX_UART_IRQ_CONFIG_FUNC_INIT(inst)                   \
		},                                                             \
		.clk_cfg = NPCX_DT_CLK_CFG_ITEM(inst),                         \
		.uart_rx_wui = NPCX_DT_WUI_ITEM_BY_NAME(0, uart_rx),           \
		.alts_size = ARRAY_SIZE(uart_alts##inst),                      \
		.alts_list = uart_alts##inst,                                  \
	};                                                                     \
									       \
	static struct uart_npcx_data uart_npcx_data_##inst = {                 \
		.baud_rate = DT_INST_PROP(inst, current_speed)                 \
	};                                                                     \
									       \
	DEVICE_DT_INST_DEFINE(inst,					       \
			&uart_npcx_init,                                       \
			uart_npcx_pm_control,				       \
			&uart_npcx_data_##inst, &uart_npcx_cfg_##inst,         \
			PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,      \
			&uart_npcx_driver_api);                                \
									       \
NPCX_UART_IRQ_CONFIG_FUNC(inst)

DT_INST_FOREACH_STATUS_OKAY(NPCX_UART_INIT)
