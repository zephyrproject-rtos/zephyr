/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_uart

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>

#include <driverlib/prcm.h>
#include <driverlib/uart.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26X2.h>
#include <zephyr/irq.h>

struct uart_cc13xx_cc26xx_config {
	uint32_t reg;
	uint32_t sys_clk_freq;
};

enum uart_cc13xx_cc26xx_pm_locks {
	UART_CC13XX_CC26XX_PM_LOCK_TX,
	UART_CC13XX_CC26XX_PM_LOCK_RX,
	UART_CC13XX_CC26XX_PM_LOCK_COUNT,
};

struct uart_cc13xx_cc26xx_data {
	struct uart_config uart_config;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *user_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_PM
	Power_NotifyObj postNotify;
	ATOMIC_DEFINE(pm_lock, UART_CC13XX_CC26XX_PM_LOCK_COUNT);
#endif
};

static int uart_cc13xx_cc26xx_poll_in(const struct device *dev,
				      unsigned char *c)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;

	if (!UARTCharsAvail(config->reg)) {
		return -1;
	}

	*c = UARTCharGetNonBlocking(config->reg);

	return 0;
}

static void uart_cc13xx_cc26xx_poll_out(const struct device *dev,
					unsigned char c)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;

	UARTCharPut(config->reg, c);
	/*
	 * Need to wait for character to be transmitted to ensure cpu does not
	 * enter standby when uart is busy
	 */
	while (UARTBusy(config->reg) == true) {
	}
}

static int uart_cc13xx_cc26xx_err_check(const struct device *dev)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;

	uint32_t flags = UARTRxErrorGet(config->reg);

	int error = (flags & UART_RXERROR_FRAMING ? UART_ERROR_FRAMING : 0) |
		    (flags & UART_RXERROR_PARITY ? UART_ERROR_PARITY : 0) |
		    (flags & UART_RXERROR_BREAK ? UART_BREAK : 0) |
		    (flags & UART_RXERROR_OVERRUN ? UART_ERROR_OVERRUN : 0);

	UARTRxErrorClear(config->reg);

	return error;
}

static int uart_cc13xx_cc26xx_configure(const struct device *dev,
					const struct uart_config *cfg)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;
	struct uart_cc13xx_cc26xx_data *data = dev->data;
	uint32_t line_ctrl = 0;
	bool flow_ctrl;

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		line_ctrl |= UART_CONFIG_PAR_NONE;
		break;
	case UART_CFG_PARITY_ODD:
		line_ctrl |= UART_CONFIG_PAR_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		line_ctrl |= UART_CONFIG_PAR_EVEN;
		break;
	case UART_CFG_PARITY_MARK:
		line_ctrl |= UART_CONFIG_PAR_ONE;
		break;
	case UART_CFG_PARITY_SPACE:
		line_ctrl |= UART_CONFIG_PAR_ZERO;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		line_ctrl |= UART_CONFIG_STOP_ONE;
		break;
	case UART_CFG_STOP_BITS_2:
		line_ctrl |= UART_CONFIG_STOP_TWO;
		break;
	case UART_CFG_STOP_BITS_0_5:
	case UART_CFG_STOP_BITS_1_5:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		line_ctrl |= UART_CONFIG_WLEN_5;
		break;
	case UART_CFG_DATA_BITS_6:
		line_ctrl |= UART_CONFIG_WLEN_6;
		break;
	case UART_CFG_DATA_BITS_7:
		line_ctrl |= UART_CONFIG_WLEN_7;
		break;
	case UART_CFG_DATA_BITS_8:
		line_ctrl |= UART_CONFIG_WLEN_8;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		flow_ctrl = false;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		flow_ctrl = true;
		break;
	case UART_CFG_FLOW_CTRL_DTR_DSR:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	/* Disables UART before setting control registers */
	UARTConfigSetExpClk(config->reg,
			    config->sys_clk_freq, cfg->baudrate,
			    line_ctrl);

	/* Clear all UART interrupts */
	UARTIntClear(config->reg,
		UART_INT_OE | UART_INT_BE | UART_INT_PE |
		UART_INT_FE | UART_INT_RT | UART_INT_TX |
		UART_INT_RX | UART_INT_CTS);

	if (flow_ctrl) {
		UARTHwFlowControlEnable(config->reg);
	} else {
		UARTHwFlowControlDisable(config->reg);
	}

	/* Re-enable UART */
	UARTEnable(config->reg);

	/* Disabled FIFOs act as 1-byte-deep holding registers (character mode) */
	UARTFIFODisable(config->reg);

	data->uart_config = *cfg;

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_cc13xx_cc26xx_config_get(const struct device *dev,
					 struct uart_config *cfg)
{
	struct uart_cc13xx_cc26xx_data *data = dev->data;

	*cfg = data->uart_config;
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_cc13xx_cc26xx_fifo_fill(const struct device *dev,
					const uint8_t *buf,
					int len)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;
	int n = 0;

	while (n < len) {
		if (!UARTCharPutNonBlocking(config->reg, buf[n])) {
			break;
		}
		n++;
	}

	return n;
}

static int uart_cc13xx_cc26xx_fifo_read(const struct device *dev,
					uint8_t *buf,
					const int len)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;
	int c, n;

	n = 0;
	while (n < len) {
		c = UARTCharGetNonBlocking(config->reg);
		if (c == -1) {
			break;
		}
		buf[n++] = c;
	}

	return n;
}

static void uart_cc13xx_cc26xx_irq_tx_enable(const struct device *dev)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;

#ifdef CONFIG_PM
	struct uart_cc13xx_cc26xx_data *data = dev->data;

	if (!atomic_test_and_set_bit(data->pm_lock, UART_CC13XX_CC26XX_PM_LOCK_TX)) {
		/*
		 * When tx irq is enabled, it is implicit that we are expecting
		 * to transmit using the uart, hence we should no longer go
		 * into standby.
		 *
		 * Instead of using pm_device_busy_set(), which currently does
		 * not impact the PM policy, we specifically disable the
		 * standby mode instead, since it is the power state that
		 * would interfere with a transfer.
		 */
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
#endif

	UARTIntEnable(config->reg, UART_INT_TX);
}

static void uart_cc13xx_cc26xx_irq_tx_disable(const struct device *dev)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;

	UARTIntDisable(config->reg, UART_INT_TX);

#ifdef CONFIG_PM
	struct uart_cc13xx_cc26xx_data *data = dev->data;

	if (atomic_test_and_clear_bit(data->pm_lock, UART_CC13XX_CC26XX_PM_LOCK_TX)) {
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
#endif
}

static int uart_cc13xx_cc26xx_irq_tx_ready(const struct device *dev)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;

	return UARTSpaceAvail(config->reg) ? 1 : 0;
}

static void uart_cc13xx_cc26xx_irq_rx_enable(const struct device *dev)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;

#ifdef CONFIG_PM
	struct uart_cc13xx_cc26xx_data *data = dev->data;

	/*
	 * When rx is enabled, it is implicit that we are expecting
	 * to receive from the uart, hence we can no longer go into
	 * standby.
	 */
	if (!atomic_test_and_set_bit(data->pm_lock, UART_CC13XX_CC26XX_PM_LOCK_RX)) {
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
#endif

	UARTIntEnable(config->reg, UART_INT_RX);
}

static void uart_cc13xx_cc26xx_irq_rx_disable(const struct device *dev)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;

#ifdef CONFIG_PM
	struct uart_cc13xx_cc26xx_data *data = dev->data;

	if (atomic_test_and_clear_bit(data->pm_lock, UART_CC13XX_CC26XX_PM_LOCK_RX)) {
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
#endif

	UARTIntDisable(config->reg, UART_INT_RX);
}

static int uart_cc13xx_cc26xx_irq_tx_complete(const struct device *dev)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;

	return UARTBusy(config->reg) ? 0 : 1;
}

static int uart_cc13xx_cc26xx_irq_rx_ready(const struct device *dev)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;

	return UARTCharsAvail(config->reg) ? 1 : 0;
}

static void uart_cc13xx_cc26xx_irq_err_enable(const struct device *dev)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;

	return UARTIntEnable(config->reg,
			     UART_INT_OE | UART_INT_BE | UART_INT_PE |
				     UART_INT_FE);
}

static void uart_cc13xx_cc26xx_irq_err_disable(const struct device *dev)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;

	return UARTIntDisable(config->reg,
			      UART_INT_OE | UART_INT_BE | UART_INT_PE |
				      UART_INT_FE);
}

static int uart_cc13xx_cc26xx_irq_is_pending(const struct device *dev)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;
	uint32_t status = UARTIntStatus(config->reg, true);

	return status & (UART_INT_TX | UART_INT_RX) ? 1 : 0;
}

static int uart_cc13xx_cc26xx_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

static void uart_cc13xx_cc26xx_irq_callback_set(const struct device *dev,
						uart_irq_callback_user_data_t cb,
						void *user_data)
{
	struct uart_cc13xx_cc26xx_data *data = dev->data;

	data->callback = cb;
	data->user_data = user_data;
}

static void uart_cc13xx_cc26xx_isr(const struct device *dev)
{
	struct uart_cc13xx_cc26xx_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->user_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_PM
/*
 *  ======== postNotifyFxn ========
 *  Called by Power module when waking up the CPU from Standby, to support
 *  the case when PM is set but PM_DEVICE is
 *  not. The uart needs to be reconfigured afterwards unless Zephyr's device
 *  PM turned it off, in which case it'd be responsible for turning it back
 *  on and reconfiguring it.
 */
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
	uintptr_t clientArg)
{
	const struct device *dev = (const struct device *)clientArg;
	const struct uart_cc13xx_cc26xx_config *config = dev->config;
	struct uart_cc13xx_cc26xx_data *data = dev->data;
	int ret = Power_NOTIFYDONE;
	int16_t res_id;

	/* Reconfigure the hardware if returning from standby */
	if (eventType == PowerCC26XX_AWAKE_STANDBY) {
		if (config->reg == DT_INST_REG_ADDR(0)) {
			res_id = PowerCC26XX_PERIPH_UART0;
		} else { /* DT_INST_REG_ADDR(1) */
			res_id = PowerCC26X2_PERIPH_UART1;
		}

		if (Power_getDependencyCount(res_id) != 0) {
			/*
			 * Reconfigure and enable UART only if not
			 * actively powered down
			 */
			if (uart_cc13xx_cc26xx_configure(dev,
				&data->uart_config) != 0) {
				ret = Power_NOTIFYERROR;
			}
		}
	}

	return (ret);
}
#endif

#ifdef CONFIG_PM_DEVICE
static int uart_cc13xx_cc26xx_pm_action(const struct device *dev,
					enum pm_device_action action)
{
	const struct uart_cc13xx_cc26xx_config *config = dev->config;
	struct uart_cc13xx_cc26xx_data *data = dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (config->reg == DT_INST_REG_ADDR(0)) {
			Power_setDependency(PowerCC26XX_PERIPH_UART0);
		} else {
			Power_setDependency(PowerCC26X2_PERIPH_UART1);
		}
		/* Configure and enable UART */
		ret = uart_cc13xx_cc26xx_configure(dev, &data->uart_config);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		UARTDisable(config->reg);
		/*
		 * Release power dependency - i.e. potentially power
		 * down serial domain.
		 */
		if (config->reg == DT_INST_REG_ADDR(0)) {
			Power_releaseDependency(PowerCC26XX_PERIPH_UART0);
		} else {
			Power_releaseDependency(PowerCC26X2_PERIPH_UART1);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct uart_driver_api uart_cc13xx_cc26xx_driver_api = {
	.poll_in = uart_cc13xx_cc26xx_poll_in,
	.poll_out = uart_cc13xx_cc26xx_poll_out,
	.err_check = uart_cc13xx_cc26xx_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_cc13xx_cc26xx_configure,
	.config_get = uart_cc13xx_cc26xx_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_cc13xx_cc26xx_fifo_fill,
	.fifo_read = uart_cc13xx_cc26xx_fifo_read,
	.irq_tx_enable = uart_cc13xx_cc26xx_irq_tx_enable,
	.irq_tx_disable = uart_cc13xx_cc26xx_irq_tx_disable,
	.irq_tx_ready = uart_cc13xx_cc26xx_irq_tx_ready,
	.irq_rx_enable = uart_cc13xx_cc26xx_irq_rx_enable,
	.irq_rx_disable = uart_cc13xx_cc26xx_irq_rx_disable,
	.irq_tx_complete = uart_cc13xx_cc26xx_irq_tx_complete,
	.irq_rx_ready = uart_cc13xx_cc26xx_irq_rx_ready,
	.irq_err_enable = uart_cc13xx_cc26xx_irq_err_enable,
	.irq_err_disable = uart_cc13xx_cc26xx_irq_err_disable,
	.irq_is_pending = uart_cc13xx_cc26xx_irq_is_pending,
	.irq_update = uart_cc13xx_cc26xx_irq_update,
	.irq_callback_set = uart_cc13xx_cc26xx_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_PM
#define UART_CC13XX_CC26XX_POWER_UART(n)				\
	do {								\
		struct uart_cc13xx_cc26xx_data *dev_data = dev->data;	\
									\
		atomic_clear_bit(dev_data->pm_lock, UART_CC13XX_CC26XX_PM_LOCK_RX); \
		atomic_clear_bit(dev_data->pm_lock, UART_CC13XX_CC26XX_PM_LOCK_TX); \
									\
		/* Set Power dependencies */				\
		if (DT_INST_REG_ADDR(n) == 0x40001000) {		\
			Power_setDependency(PowerCC26XX_PERIPH_UART0);	\
		} else {						\
			Power_setDependency(PowerCC26X2_PERIPH_UART1);	\
		}							\
									\
		/* Register notification function */			\
		Power_registerNotify(&dev_data->postNotify,		\
			PowerCC26XX_AWAKE_STANDBY,			\
			postNotifyFxn, (uintptr_t)dev);			\
	} while (false)
#else
#define UART_CC13XX_CC26XX_POWER_UART(n)				\
	do {								\
		uint32_t domain, periph;					\
									\
		/* Enable UART power domain */				\
		if (DT_INST_REG_ADDR(n) == 0x40001000) {		\
			domain = PRCM_DOMAIN_SERIAL;			\
			periph = PRCM_PERIPH_UART0;			\
		} else {						\
			domain = PRCM_DOMAIN_PERIPH;			\
			periph = PRCM_PERIPH_UART1;			\
		}							\
		PRCMPowerDomainOn(domain);				\
									\
		/* Enable UART peripherals */				\
		PRCMPeripheralRunEnable(periph);			\
		PRCMPeripheralSleepEnable(periph);			\
									\
		/* Load PRCM settings */				\
		PRCMLoadSet();						\
		while (!PRCMLoadGet()) {				\
			continue;					\
		}							\
									     \
		/* UART should not be accessed until power domain is on. */  \
		while (PRCMPowerDomainsAllOn(domain) !=			     \
			PRCM_DOMAIN_POWER_ON) {				     \
			continue;					     \
		}							     \
	} while (false)
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_CC13XX_CC26XX_IRQ_CFG(n)					\
	do {								\
		const struct uart_cc13xx_cc26xx_config *config =	\
			dev->config;					\
									\
		UARTIntClear(config->reg, UART_INT_RX);			\
									\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
				DT_INST_IRQ(n, priority),		\
				uart_cc13xx_cc26xx_isr,			\
				DEVICE_DT_INST_GET(n),			\
				0);					\
		irq_enable(DT_INST_IRQN(n));				\
		/* Causes an initial TX ready INT when TX INT enabled */\
		UARTCharPutNonBlocking(config->reg, '\0');		\
	} while (false)

#define UART_CC13XX_CC26XX_INT_FIELDS					\
	.callback = NULL,						\
	.user_data = NULL,
#else
#define UART_CC13XX_CC26XX_IRQ_CFG(n)
#define UART_CC13XX_CC26XX_INT_FIELDS
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_CC13XX_CC26XX_DEVICE_DEFINE(n)				     \
	PM_DEVICE_DT_INST_DEFINE(n, uart_cc13xx_cc26xx_pm_action);	     \
									     \
	DEVICE_DT_INST_DEFINE(n,					     \
		uart_cc13xx_cc26xx_init_##n,				     \
		PM_DEVICE_DT_INST_GET(n),				     \
		&uart_cc13xx_cc26xx_data_##n, &uart_cc13xx_cc26xx_config_##n,\
		PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,		     \
		&uart_cc13xx_cc26xx_driver_api)

#define UART_CC13XX_CC26XX_INIT_FUNC(n)					    \
	static int uart_cc13xx_cc26xx_init_##n(const struct device *dev)	    \
	{								    \
		struct uart_cc13xx_cc26xx_data *data = dev->data;	    \
		int ret;						    \
									    \
		UART_CC13XX_CC26XX_POWER_UART(n);			    \
									    \
		ret = pinctrl_apply_state(data->pcfg, PINCTRL_STATE_DEFAULT);	\
		if (ret < 0) {	\
			return ret;	\
		}				    \
									    \
		/* Configure and enable UART */				    \
		ret = uart_cc13xx_cc26xx_configure(dev, &data->uart_config);\
									    \
		/* Enable interrupts */					    \
		UART_CC13XX_CC26XX_IRQ_CFG(n);				    \
									    \
		return ret;						    \
	}


#define UART_CC13XX_CC26XX_INIT(n)				     \
	PINCTRL_DT_INST_DEFINE(n); \
	UART_CC13XX_CC26XX_INIT_FUNC(n);			     \
								     \
	static const struct uart_cc13xx_cc26xx_config		     \
		uart_cc13xx_cc26xx_config_##n = {		     \
		.reg = DT_INST_REG_ADDR(n),			     \
		.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(n, clocks,   \
			clock_frequency)			     \
	};							     \
								     \
	static struct uart_cc13xx_cc26xx_data			     \
		uart_cc13xx_cc26xx_data_##n = {			     \
		.uart_config = {				     \
			.baudrate = DT_INST_PROP(n, current_speed),  \
			.parity = UART_CFG_PARITY_NONE,		     \
			.stop_bits = UART_CFG_STOP_BITS_1,	     \
			.data_bits = UART_CFG_DATA_BITS_8,	     \
			.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,	     \
		},						     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n), \
		UART_CC13XX_CC26XX_INT_FIELDS			     \
	};							     \
								     \
	UART_CC13XX_CC26XX_DEVICE_DEFINE(n);

DT_INST_FOREACH_STATUS_OKAY(UART_CC13XX_CC26XX_INIT)
