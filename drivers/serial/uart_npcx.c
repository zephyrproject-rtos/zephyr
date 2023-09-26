/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_uart

#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <soc.h>
#include "soc_miwu.h"
#include "soc_power.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(uart_npcx, CONFIG_UART_LOG_LEVEL);

/* Driver config */
struct uart_npcx_config {
	struct uart_reg *inst;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif
	/* clock configuration */
	struct npcx_clk_cfg clk_cfg;
	/* int-mux configuration */
	const struct npcx_wui uart_rx_wui;
	/* pinmux configuration */
	const struct pinctrl_dev_config *pcfg;
};

enum uart_pm_policy_state_flag {
	UART_PM_POLICY_STATE_TX_FLAG,
	UART_PM_POLICY_STATE_RX_FLAG,

	UART_PM_POLICY_STATE_FLAG_COUNT,
};

/* Driver data */
struct uart_npcx_data {
	/* Baud rate */
	uint32_t baud_rate;
	struct miwu_callback uart_rx_cb;
	struct k_spinlock lock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif
#ifdef CONFIG_PM
	ATOMIC_DEFINE(pm_policy_state_flag, UART_PM_POLICY_STATE_FLAG_COUNT);
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
	struct k_work_delayable rx_refresh_timeout_work;
#endif
#endif
};

#ifdef CONFIG_PM
static void uart_npcx_pm_policy_state_lock_get(struct uart_npcx_data *data,
					       enum uart_pm_policy_state_flag flag)
{
	if (atomic_test_and_set_bit(data->pm_policy_state_flag, flag) == 0) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}

static void uart_npcx_pm_policy_state_lock_put(struct uart_npcx_data *data,
					    enum uart_pm_policy_state_flag flag)
{
	if (atomic_test_and_clear_bit(data->pm_policy_state_flag, flag) == 1) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}
#endif

/* UART local functions */
static int uart_set_npcx_baud_rate(struct uart_reg *const inst, int baud_rate, int src_clk)
{
	/* Fix baud rate to 115200 so far */
	if (baud_rate == 115200) {
		if (src_clk == 15000000) {
			inst->UPSR = 0x38;
			inst->UBAUD = 0x01;
		} else if (src_clk == 20000000) {
			inst->UPSR = 0x08;
			inst->UBAUD = 0x0a;
		} else {
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_npcx_tx_fifo_ready(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

	/* True if the Tx FIFO is not completely full */
	return !(GET_FIELD(inst->UFTSTS, NPCX_UFTSTS_TEMPTY_LVL) == 0);
}

static int uart_npcx_rx_fifo_available(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

	/* True if at least one byte is in the Rx FIFO */
	return IS_BIT_SET(inst->UFRSTS, NPCX_UFRSTS_RFIFO_NEMPTY_STS);
}

static void uart_npcx_dis_all_tx_interrupts(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

	/* Disable all Tx interrupts */
	inst->UFTCTL &= ~(BIT(NPCX_UFTCTL_TEMPTY_LVL_EN) |
			  BIT(NPCX_UFTCTL_TEMPTY_EN) |
			  BIT(NPCX_UFTCTL_NXMIP_EN));
}

static void uart_npcx_clear_rx_fifo(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;
	uint8_t scratch;

	/* Read all dummy bytes out from Rx FIFO */
	while (uart_npcx_rx_fifo_available(dev))
		scratch = inst->URBUF;
}

static int uart_npcx_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;
	struct uart_npcx_data *data = dev->data;
	uint8_t tx_bytes = 0U;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* If Tx FIFO is still ready to send */
	while ((size - tx_bytes > 0) && uart_npcx_tx_fifo_ready(dev)) {
		/* Put a character into Tx FIFO */
		inst->UTBUF = tx_data[tx_bytes++];
	}
#ifdef CONFIG_PM
	uart_npcx_pm_policy_state_lock_get(data, UART_PM_POLICY_STATE_TX_FLAG);
	/* Enable NXMIP interrupt in case ec enters deep sleep early */
	inst->UFTCTL |= BIT(NPCX_UFTCTL_NXMIP_EN);
#endif /* CONFIG_PM */
	k_spin_unlock(&data->lock, key);

	return tx_bytes;
}

static int uart_npcx_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;
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
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;
	struct uart_npcx_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	inst->UFTCTL |= BIT(NPCX_UFTCTL_TEMPTY_EN);
	k_spin_unlock(&data->lock, key);
}

static void uart_npcx_irq_tx_disable(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;
	struct uart_npcx_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	inst->UFTCTL &= ~(BIT(NPCX_UFTCTL_TEMPTY_EN));
	k_spin_unlock(&data->lock, key);
}

static bool uart_npcx_irq_tx_is_enabled(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

	return IS_BIT_SET(inst->UFTCTL, NPCX_UFTCTL_TEMPTY_EN);
}

static int uart_npcx_irq_tx_ready(const struct device *dev)
{
	return uart_npcx_tx_fifo_ready(dev) && uart_npcx_irq_tx_is_enabled(dev);
}

static int uart_npcx_irq_tx_complete(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

	/* Tx FIFO is empty or last byte is sending */
	return IS_BIT_SET(inst->UFTSTS, NPCX_UFTSTS_NXMIP);
}

static void uart_npcx_irq_rx_enable(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

	inst->UFRCTL |= BIT(NPCX_UFRCTL_RNEMPTY_EN);
}

static void uart_npcx_irq_rx_disable(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

	inst->UFRCTL &= ~(BIT(NPCX_UFRCTL_RNEMPTY_EN));
}

static bool uart_npcx_irq_rx_is_enabled(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

	return IS_BIT_SET(inst->UFRCTL, NPCX_UFRCTL_RNEMPTY_EN);
}

static int uart_npcx_irq_rx_ready(const struct device *dev)
{
	return uart_npcx_rx_fifo_available(dev);
}

static void uart_npcx_irq_err_enable(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

	inst->UICTRL |= BIT(NPCX_UICTRL_EEI);
}

static void uart_npcx_irq_err_disable(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

	inst->UICTRL &= ~(BIT(NPCX_UICTRL_EEI));
}

static int uart_npcx_irq_is_pending(const struct device *dev)
{
	return uart_npcx_irq_tx_ready(dev) ||
		(uart_npcx_irq_rx_ready(dev) && uart_npcx_irq_rx_is_enabled(dev));
}

static int uart_npcx_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static void uart_npcx_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_npcx_data *data = dev->data;

	data->user_cb = cb;
	data->user_data = cb_data;
}

static void uart_npcx_isr(const struct device *dev)
{
	struct uart_npcx_data *data = dev->data;

	/*
	 * Set pm constraint to prevent the system enter suspend state within
	 * the CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT period.
	 */
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
	if (uart_npcx_irq_rx_ready(dev)) {
		k_timeout_t delay = K_MSEC(CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT);

		uart_npcx_pm_policy_state_lock_get(data, UART_PM_POLICY_STATE_RX_FLAG);
		k_work_reschedule(&data->rx_refresh_timeout_work, delay);
	}
#endif

	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
#ifdef CONFIG_PM
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

	if (IS_BIT_SET(inst->UFTCTL, NPCX_UFTCTL_NXMIP_EN) &&
	    IS_BIT_SET(inst->UFTSTS, NPCX_UFTSTS_NXMIP)) {
		k_spinlock_key_t key = k_spin_lock(&data->lock);

		/* Disable NXMIP interrupt */
		inst->UFTCTL &= ~BIT(NPCX_UFTCTL_NXMIP_EN);
		k_spin_unlock(&data->lock, key);
		uart_npcx_pm_policy_state_lock_put(data, UART_PM_POLICY_STATE_TX_FLAG);
	}
#endif /* CONFIG_PM */
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
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

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
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

	/* Wait while Tx single byte buffer is ready to send */
	while (!IS_BIT_SET(inst->UICTRL, NPCX_UICTRL_TBE))
		continue;

	inst->UTBUF = c;
}
#endif /* !CONFIG_UART_INTERRUPT_DRIVEN */

/* UART api functions */
static int uart_npcx_err_check(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;
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

static __unused void uart_npcx_rx_wk_isr(const struct device *dev, struct npcx_wui *wui)
{
	/*
	 * Set pm constraint to prevent the system enter suspend state within
	 * the CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT period.
	 */
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
	struct uart_npcx_data *data = dev->data;
	k_timeout_t delay = K_MSEC(CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT);

	uart_npcx_pm_policy_state_lock_get(data, UART_PM_POLICY_STATE_RX_FLAG);
	k_work_reschedule(&data->rx_refresh_timeout_work, delay);
#endif

	/*
	 * Disable MIWU CR_SIN interrupt to avoid the other redundant interrupts
	 * after ec wakes up.
	 */
	npcx_uart_disable_access_interrupt();
}

#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
static void uart_npcx_rx_refresh_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_npcx_data *data =
		CONTAINER_OF(dwork, struct uart_npcx_data, rx_refresh_timeout_work);

	uart_npcx_pm_policy_state_lock_put(data, UART_PM_POLICY_STATE_RX_FLAG);
}
#endif

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
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_npcx_init(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_npcx_data *const data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	struct uart_reg *const inst = config->inst;
	uint32_t uart_rate;
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on UART clock fail %d", ret);
		return ret;
	}

	/*
	 * If apb2's clock is not 15MHz, we need to find the other optimized
	 * values of UPSR and UBAUD for baud rate 115200.
	 */
	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)&config->clk_cfg,
				     &uart_rate);
	if (ret < 0) {
		LOG_ERR("Get UART clock rate error %d", ret);
		return ret;
	}

	/* Configure baud rate */
	ret = uart_set_npcx_baud_rate(inst, data->baud_rate, uart_rate);
	if (ret < 0) {
		LOG_ERR("Set baud rate %d with unsupported apb clock %d failed", data->baud_rate,
			uart_rate);
		return ret;
	}

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
	config->irq_config_func(dev);
#endif

	if (IS_ENABLED(CONFIG_PM)) {
		/* Initialize a miwu device input and its callback function */
		npcx_miwu_init_dev_callback(&data->uart_rx_cb, &config->uart_rx_wui,
					    uart_npcx_rx_wk_isr, dev);
		npcx_miwu_manage_callback(&data->uart_rx_cb, true);
		/*
		 * Configure the UART wake-up event triggered from a falling
		 * edge on CR_SIN pin. No need for callback function.
		 */
		npcx_miwu_interrupt_configure(&config->uart_rx_wui, NPCX_MIWU_MODE_EDGE,
					      NPCX_MIWU_TRIG_LOW);

#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
		k_work_init_delayable(&data->rx_refresh_timeout_work, uart_npcx_rx_refresh_timeout);
#endif
	}

	/* Configure pin-mux for uart device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("UART pinctrl setup failed (%d)", ret);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define NPCX_UART_IRQ_CONFIG_FUNC_DECL(inst)                                                       \
	static void uart_npcx_irq_config_##inst(const struct device *dev)
#define NPCX_UART_IRQ_CONFIG_FUNC_INIT(inst) .irq_config_func = uart_npcx_irq_config_##inst,
#define NPCX_UART_IRQ_CONFIG_FUNC(inst)                                                            \
	static void uart_npcx_irq_config_##inst(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), uart_npcx_isr,        \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}
#else
#define NPCX_UART_IRQ_CONFIG_FUNC_DECL(inst)
#define NPCX_UART_IRQ_CONFIG_FUNC_INIT(inst)
#define NPCX_UART_IRQ_CONFIG_FUNC(inst)
#endif

#define NPCX_UART_INIT(i)                                                                       \
	NPCX_UART_IRQ_CONFIG_FUNC_DECL(i);                                                      \
												\
	PINCTRL_DT_INST_DEFINE(i);                                                              \
												\
	static const struct uart_npcx_config uart_npcx_cfg_##i = {                              \
		.inst = (struct uart_reg *)DT_INST_REG_ADDR(i),                                 \
		.clk_cfg = NPCX_DT_CLK_CFG_ITEM(i),                                             \
		.uart_rx_wui = NPCX_DT_WUI_ITEM_BY_NAME(0, uart_rx),                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(i),                                      \
		NPCX_UART_IRQ_CONFIG_FUNC_INIT(i)                                               \
	};                                                                                      \
												\
	static struct uart_npcx_data uart_npcx_data_##i = { .baud_rate = DT_INST_PROP(          \
								       i, current_speed) };     \
												\
	DEVICE_DT_INST_DEFINE(i, &uart_npcx_init, NULL, &uart_npcx_data_##i,                    \
			      &uart_npcx_cfg_##i, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,    \
			      &uart_npcx_driver_api);                                           \
												\
	NPCX_UART_IRQ_CONFIG_FUNC(i)

DT_INST_FOREACH_STATUS_OKAY(NPCX_UART_INIT)

#define ENABLE_MIWU_CRIN_IRQ(i)                                                                 \
	npcx_miwu_irq_get_and_clear_pending(&uart_npcx_cfg_##i.uart_rx_wui);                    \
	npcx_miwu_irq_enable(&uart_npcx_cfg_##i.uart_rx_wui);

#define DISABLE_MIWU_CRIN_IRQ(i) npcx_miwu_irq_disable(&uart_npcx_cfg_##i.uart_rx_wui);

void npcx_uart_enable_access_interrupt(void)
{
	DT_INST_FOREACH_STATUS_OKAY(ENABLE_MIWU_CRIN_IRQ)
}

void npcx_uart_disable_access_interrupt(void)
{
	DT_INST_FOREACH_STATUS_OKAY(DISABLE_MIWU_CRIN_IRQ)
}
