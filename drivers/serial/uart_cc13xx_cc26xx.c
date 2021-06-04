/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_uart

#include <device.h>
#include <errno.h>
#include <sys/__assert.h>
#include <pm/device.h>
#include <pm/pm.h>
#include <drivers/uart.h>

#include <driverlib/ioc.h>
#include <driverlib/prcm.h>
#include <driverlib/uart.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26X2.h>

#define GET_PIN(n, pin_name) \
	DT_INST_PROP_BY_IDX(n, pin_name, 0)
#define GET_PORT(n, pin_name) \
	DT_INST_PROP_BY_IDX(n, pin_name, 1)

struct uart_cc13xx_cc26xx_data {
	struct uart_config uart_config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *user_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_PM
	Power_NotifyObj postNotify;
	bool tx_constrained;
	bool rx_constrained;
#endif
#ifdef CONFIG_PM_DEVICE
	enum pm_device_state pm_state;
#endif
};

static inline struct uart_cc13xx_cc26xx_data *get_dev_data(const struct device *dev)
{
	return dev->data;
}

static inline const struct uart_device_config *get_dev_conf(const struct device *dev)
{
	return dev->config;
}

static int uart_cc13xx_cc26xx_poll_in(const struct device *dev,
				      unsigned char *c)
{
	if (!UARTCharsAvail(get_dev_conf(dev)->regs)) {
		return -1;
	}

	*c = UARTCharGetNonBlocking(get_dev_conf(dev)->regs);

	return 0;
}

static void uart_cc13xx_cc26xx_poll_out(const struct device *dev,
					unsigned char c)
{
	UARTCharPut(get_dev_conf(dev)->regs, c);
	/*
	 * Need to wait for character to be transmitted to ensure cpu does not
	 * enter standby when uart is busy
	 */
	while (UARTBusy(get_dev_conf(dev)->regs) == true) {
	}
}

static int uart_cc13xx_cc26xx_err_check(const struct device *dev)
{
	uint32_t flags = UARTRxErrorGet(get_dev_conf(dev)->regs);

	int error = (flags & UART_RXERROR_FRAMING ? UART_ERROR_FRAMING : 0) |
		    (flags & UART_RXERROR_PARITY ? UART_ERROR_PARITY : 0) |
		    (flags & UART_RXERROR_BREAK ? UART_BREAK : 0) |
		    (flags & UART_RXERROR_OVERRUN ? UART_ERROR_OVERRUN : 0);

	UARTRxErrorClear(get_dev_conf(dev)->regs);

	return error;
}

static int uart_cc13xx_cc26xx_configure(const struct device *dev,
					const struct uart_config *cfg)
{
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
	UARTConfigSetExpClk(get_dev_conf(dev)->regs,
			    get_dev_conf(dev)->sys_clk_freq, cfg->baudrate,
			    line_ctrl);

	/* Clear all UART interrupts */
	UARTIntClear(get_dev_conf(dev)->regs,
		UART_INT_OE | UART_INT_BE | UART_INT_PE |
		UART_INT_FE | UART_INT_RT | UART_INT_TX |
		UART_INT_RX | UART_INT_CTS);

	if (flow_ctrl) {
		UARTHwFlowControlEnable(get_dev_conf(dev)->regs);
	} else {
		UARTHwFlowControlDisable(get_dev_conf(dev)->regs);
	}

	/* Re-enable UART */
	UARTEnable(get_dev_conf(dev)->regs);

	/* Disabled FIFOs act as 1-byte-deep holding registers (character mode) */
	UARTFIFODisable(get_dev_conf(dev)->regs);

	get_dev_data(dev)->uart_config = *cfg;

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_cc13xx_cc26xx_config_get(const struct device *dev,
					 struct uart_config *cfg)
{
	*cfg = get_dev_data(dev)->uart_config;
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_cc13xx_cc26xx_fifo_fill(const struct device *dev,
					const uint8_t *buf,
					int len)
{
	int n = 0;

	while (n < len) {
		if (!UARTCharPutNonBlocking(get_dev_conf(dev)->regs, buf[n])) {
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
	int c, n;

	n = 0;
	while (n < len) {
		c = UARTCharGetNonBlocking(get_dev_conf(dev)->regs);
		if (c == -1) {
			break;
		}
		buf[n++] = c;
	}

	return n;
}

static void uart_cc13xx_cc26xx_irq_tx_enable(const struct device *dev)
{
#ifdef CONFIG_PM
	if (!get_dev_data(dev)->tx_constrained) {
		/*
		 * When tx irq is enabled, it is implicit that we are expecting
		 * to transmit using the uart, hence we should no longer go
		 * into standby.
		 *
		 * Instead of using device_busy_set(), which currently does
		 * not impact the PM policy, we specifically disable the
		 * standby mode instead, since it is the power state that
		 * would interfere with a transfer.
		 */
		pm_constraint_set(PM_STATE_STANDBY);
		get_dev_data(dev)->tx_constrained = true;
	}
#endif

	UARTIntEnable(get_dev_conf(dev)->regs, UART_INT_TX);
}

static void uart_cc13xx_cc26xx_irq_tx_disable(const struct device *dev)
{
	UARTIntDisable(get_dev_conf(dev)->regs, UART_INT_TX);

#ifdef CONFIG_PM
	if (get_dev_data(dev)->tx_constrained) {
		pm_constraint_release(PM_STATE_STANDBY);
		get_dev_data(dev)->tx_constrained = false;
	}
#endif
}

static int uart_cc13xx_cc26xx_irq_tx_ready(const struct device *dev)
{
	return UARTSpaceAvail(get_dev_conf(dev)->regs) ? 1 : 0;
}

static void uart_cc13xx_cc26xx_irq_rx_enable(const struct device *dev)
{
#ifdef CONFIG_PM
	/*
	 * When rx is enabled, it is implicit that we are expecting
	 * to receive from the uart, hence we can no longer go into
	 * standby.
	 */
	if (!get_dev_data(dev)->rx_constrained) {
		pm_constraint_set(PM_STATE_STANDBY);
		get_dev_data(dev)->rx_constrained = true;
	}
#endif

	UARTIntEnable(get_dev_conf(dev)->regs, UART_INT_RX);
}

static void uart_cc13xx_cc26xx_irq_rx_disable(const struct device *dev)
{
#ifdef CONFIG_PM
	if (get_dev_data(dev)->rx_constrained) {
		pm_constraint_release(PM_STATE_STANDBY);
		get_dev_data(dev)->rx_constrained = false;
	}
#endif

	UARTIntDisable(get_dev_conf(dev)->regs, UART_INT_RX);
}

static int uart_cc13xx_cc26xx_irq_tx_complete(const struct device *dev)
{
	return UARTBusy(get_dev_conf(dev)->regs) ? 0 : 1;
}

static int uart_cc13xx_cc26xx_irq_rx_ready(const struct device *dev)
{
	return UARTCharsAvail(get_dev_conf(dev)->regs) ? 1 : 0;
}

static void uart_cc13xx_cc26xx_irq_err_enable(const struct device *dev)
{
	return UARTIntEnable(get_dev_conf(dev)->regs,
			     UART_INT_OE | UART_INT_BE | UART_INT_PE |
				     UART_INT_FE);
}

static void uart_cc13xx_cc26xx_irq_err_disable(const struct device *dev)
{
	return UARTIntDisable(get_dev_conf(dev)->regs,
			      UART_INT_OE | UART_INT_BE | UART_INT_PE |
				      UART_INT_FE);
}

static int uart_cc13xx_cc26xx_irq_is_pending(const struct device *dev)
{
	uint32_t status = UARTIntStatus(get_dev_conf(dev)->regs, true);

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
	struct uart_cc13xx_cc26xx_data *data = get_dev_data(dev);

	data->callback = cb;
	data->user_data = user_data;
}

static void uart_cc13xx_cc26xx_isr(const struct device *dev)
{
	struct uart_cc13xx_cc26xx_data *data = get_dev_data(dev);

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
	int ret = Power_NOTIFYDONE;
	int16_t res_id;

	/* Reconfigure the hardware if returning from standby */
	if (eventType == PowerCC26XX_AWAKE_STANDBY) {
		if (get_dev_conf(dev)->regs ==
			DT_INST_REG_ADDR(0)) {
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
				&get_dev_data(dev)->uart_config) != 0) {
				ret = Power_NOTIFYERROR;
			}
		}
	}

	return (ret);
}
#endif

#ifdef CONFIG_PM_DEVICE
static int uart_cc13xx_cc26xx_set_power_state(const struct device *dev,
					      enum pm_device_state new_state)
{
	int ret = 0;

	if ((new_state == PM_DEVICE_STATE_ACTIVE) &&
		(new_state != get_dev_data(dev)->pm_state)) {
		if (get_dev_conf(dev)->regs ==
			DT_INST_REG_ADDR(0)) {
			Power_setDependency(PowerCC26XX_PERIPH_UART0);
		} else {
			Power_setDependency(PowerCC26X2_PERIPH_UART1);
		}
		/* Configure and enable UART */
		ret = uart_cc13xx_cc26xx_configure(dev,
			&get_dev_data(dev)->uart_config);
		if (ret == 0) {
			get_dev_data(dev)->pm_state = new_state;
		}
	} else {
		__ASSERT_NO_MSG(new_state == PM_DEVICE_STATE_LOW_POWER ||
			new_state == PM_DEVICE_STATE_SUSPEND ||
			new_state == PM_DEVICE_STATE_OFF);

		if (get_dev_data(dev)->pm_state == PM_DEVICE_STATE_ACTIVE) {
			UARTDisable(get_dev_conf(dev)->regs);
			/*
			 * Release power dependency - i.e. potentially power
			 * down serial domain.
			 */
			if (get_dev_conf(dev)->regs ==
			    DT_INST_REG_ADDR(0)) {
				Power_releaseDependency(
					PowerCC26XX_PERIPH_UART0);
			} else {
				Power_releaseDependency(
					PowerCC26X2_PERIPH_UART1);
			}
			get_dev_data(dev)->pm_state = new_state;
		}
	}

	return ret;
}

static int uart_cc13xx_cc26xx_pm_control(const struct device *dev,
					 uint32_t ctrl_command,
					 enum pm_device_state *state)
{
	int ret = 0;

	if (ctrl_command == PM_DEVICE_STATE_SET) {
		enum pm_device_state new_state = *state;

		if (new_state != get_dev_data(dev)->pm_state) {
			ret = uart_cc13xx_cc26xx_set_power_state(dev,
				new_state);
		}
	} else {
		__ASSERT_NO_MSG(ctrl_command == PM_DEVICE_STATE_GET);
		*state = get_dev_data(dev)->pm_state;
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
		get_dev_data(dev)->rx_constrained = false;		\
		get_dev_data(dev)->tx_constrained = false;		\
									\
		/* Set Power dependencies */				\
		if (DT_INST_REG_ADDR(n) == 0x40001000) {		\
			Power_setDependency(PowerCC26XX_PERIPH_UART0);	\
		} else {						\
			Power_setDependency(PowerCC26X2_PERIPH_UART1);	\
		}							\
									\
		/* Register notification function */			\
		Power_registerNotify(&get_dev_data(dev)->postNotify,	\
			PowerCC26XX_AWAKE_STANDBY,			\
			postNotifyFxn, (uintptr_t)dev);			\
	} while (0)
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
		while (PRCMPowerDomainStatus(domain) !=			     \
			PRCM_DOMAIN_POWER_ON) {				     \
			continue;					     \
		}							     \
	} while (0)
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_CC13XX_CC26XX_IRQ_CFG(n)					\
	do {								\
		UARTIntClear(get_dev_conf(dev)->regs, UART_INT_RX);	\
									\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
				DT_INST_IRQ(n, priority),		\
				uart_cc13xx_cc26xx_isr,			\
				DEVICE_DT_INST_GET(n),			\
				0);					\
		irq_enable(DT_INST_IRQN(n));				\
		/* Causes an initial TX ready INT when TX INT enabled */\
		UARTCharPutNonBlocking(get_dev_conf(dev)->regs, '\0');  \
	} while (0)

#define UART_CC13XX_CC26XX_INT_FIELDS					\
	.callback = NULL,						\
	.user_data = NULL,
#else
#define UART_CC13XX_CC26XX_IRQ_CFG(n)
#define UART_CC13XX_CC26XX_INT_FIELDS
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_CC13XX_CC26XX_DEVICE_DEFINE(n)				     \
	DEVICE_DT_INST_DEFINE(n,					     \
		uart_cc13xx_cc26xx_init_##n,				     \
		uart_cc13xx_cc26xx_pm_control,				     \
		&uart_cc13xx_cc26xx_data_##n, &uart_cc13xx_cc26xx_config_##n,\
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	     \
		&uart_cc13xx_cc26xx_driver_api)

#ifdef CONFIG_PM_DEVICE
#define UART_CC13XX_CC26XX_DEVICE_INIT(n)				\
	UART_CC13XX_CC26XX_DEVICE_DEFINE(n)

#define UART_CC13XX_CC26XX_INIT_PM_STATE				\
	do {								\
		get_dev_data(dev)->pm_state = PM_DEVICE_STATE_ACTIVE;	\
	} while (0)
#else
#define UART_CC13XX_CC26XX_INIT_PM_STATE
#endif

#define UART_CC13XX_CC26XX_INIT_FUNC(n)					    \
	static int uart_cc13xx_cc26xx_init_##n(const struct device *dev)	    \
	{								    \
		int ret;						    \
									    \
		UART_CC13XX_CC26XX_INIT_PM_STATE;			    \
									    \
		UART_CC13XX_CC26XX_POWER_UART(n);			    \
									    \
		/* Configure IOC module to map UART signals to pins */	    \
		IOCPortConfigureSet(GET_PIN(n, tx_pin), GET_PORT(n, tx_pin),\
			IOC_STD_OUTPUT);				    \
		IOCPortConfigureSet(GET_PIN(n, rx_pin), GET_PORT(n, rx_pin),\
			IOC_STD_INPUT);					    \
									    \
		/* Configure and enable UART */				    \
		ret = uart_cc13xx_cc26xx_configure(dev,			    \
			&get_dev_data(dev)->uart_config);		    \
									    \
		/* Enable interrupts */					    \
		UART_CC13XX_CC26XX_IRQ_CFG(n);				    \
									    \
		return ret;						    \
	}


#define UART_CC13XX_CC26XX_INIT(n)				     \
	UART_CC13XX_CC26XX_INIT_FUNC(n);			     \
								     \
	static const struct uart_device_config			     \
		uart_cc13xx_cc26xx_config_##n = {		     \
		.regs = DT_INST_REG_ADDR(n),			     \
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
		UART_CC13XX_CC26XX_INT_FIELDS			     \
	};							     \
								     \
	UART_CC13XX_CC26XX_DEVICE_DEFINE(n);

DT_INST_FOREACH_STATUS_OKAY(UART_CC13XX_CC26XX_INIT)
