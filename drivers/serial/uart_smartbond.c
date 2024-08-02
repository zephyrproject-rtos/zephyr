/*
 * Copyright (c) 2022 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_smartbond_uart

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/byteorder.h>
#include <DA1469xAB.h>
#include <zephyr/irq.h>
#include <da1469x_pd.h>

#define IIR_NO_INTR		1
#define IIR_THR_EMPTY		2
#define IIR_RX_DATA		4
#define IIR_LINE_STATUS		5
#define IIR_BUSY		7
#define IIR_TIMEOUT		12

#define STOP_BITS_1	0
#define STOP_BITS_2	1

#define DATA_BITS_5	0
#define DATA_BITS_6	1
#define DATA_BITS_7	2
#define DATA_BITS_8	3

#define RX_FIFO_TRIG_1_CHAR		0
#define RX_FIFO_TRIG_1_4_FULL		1
#define RX_FIFO_TRIG_1_2_FULL		2
#define RX_FIFO_TRIG_MINUS_2_CHARS	3

#define TX_FIFO_TRIG_EMPTY		0
#define TX_FIFO_TRIG_2_CHARS		1
#define TX_FIFO_TRIG_1_4_FULL		2
#define TX_FIFO_TRIG_1_2_FULL		3

#define BAUDRATE_CFG_DLH(cfg)		(((cfg) >> 16) & 0xff)
#define BAUDRATE_CFG_DLL(cfg)		(((cfg) >> 8) & 0xff)
#define BAUDRATE_CFG_DLF(cfg)		((cfg) & 0xff)

struct uart_smartbond_baudrate_cfg {
	uint32_t baudrate;
	/* DLH=cfg[23:16] DLL=cfg[15:8] DLF=cfg[7:0] */
	uint32_t cfg;
};

static const struct uart_smartbond_baudrate_cfg uart_smartbond_baudrate_table[] = {
	{ 2000000, 0x00000100 },
	{ 1000000, 0x00000200 },
	{  921600, 0x00000203 },
	{  500000, 0x00000400 },
	{  230400, 0x0000080b },
	{  115200, 0x00001106 },
	{   57600, 0x0000220c },
	{   38400, 0x00003401 },
	{   28800, 0x00004507 },
	{   19200, 0x00006803 },
	{   14400, 0x00008a0e },
	{    9600, 0x0000d005 },
	{    4800, 0x0001a00b },
};

struct uart_smartbond_cfg {
	UART2_Type *regs;
	int periph_clock_config;
	const struct pinctrl_dev_config *pcfg;
	bool hw_flow_control_supported;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
#if CONFIG_PM_DEVICE
	int rx_wake_timeout;
	struct gpio_dt_spec rx_wake_gpio;
	struct gpio_dt_spec dtr_gpio;
#endif
};

struct uart_smartbond_runtime_cfg {
	uint32_t baudrate_cfg;
	uint32_t lcr_reg_val;
	uint8_t mcr_reg_val;
	uint8_t ier_reg_val;
};

struct uart_smartbond_data {
	struct uart_config current_config;
	struct uart_smartbond_runtime_cfg runtime_cfg;
	struct k_spinlock lock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
	uint32_t flags;
	uint8_t rx_enabled;
	uint8_t tx_enabled;
#if CONFIG_PM_DEVICE
	struct gpio_callback dtr_wake_cb;
	const struct device *dev;
	struct gpio_callback rx_wake_cb;
	int rx_wake_timeout;
	struct k_work_delayable rx_timeout_work;
#endif
#endif
};

#ifdef CONFIG_PM_DEVICE
static inline void uart_smartbond_pm_prevent_system_sleep(void)
{
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}

static inline void uart_smartbond_pm_allow_system_sleep(void)
{
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}

static void uart_smartbond_pm_policy_state_lock_get(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_runtime_get(dev);
#else
	ARG_UNUSED(dev);
	uart_smartbond_pm_prevent_system_sleep();
#endif
}

static void uart_smartbond_pm_policy_state_lock_put(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_runtime_put(dev);
#else
	ARG_UNUSED(dev);
	uart_smartbond_pm_allow_system_sleep();
#endif
}

static void uart_smartbond_rx_refresh_timeout(struct k_work *work)
{
	struct uart_smartbond_data *data = CONTAINER_OF(work, struct uart_smartbond_data,
							rx_timeout_work.work);

	uart_smartbond_pm_policy_state_lock_put(data->dev);
}
#endif

static int uart_smartbond_poll_in(const struct device *dev, unsigned char *p_char)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	pm_device_runtime_get(dev);

	if ((config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_RFNE_Msk) == 0) {
		pm_device_runtime_put(dev);
		k_spin_unlock(&data->lock, key);
		return -1;
	}

	*p_char = config->regs->UART2_RBR_THR_DLL_REG;

	pm_device_runtime_put(dev);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static void uart_smartbond_poll_out(const struct device *dev, unsigned char out_char)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	pm_device_runtime_get(dev);

	while (!(config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_TFNF_Msk)) {
		/* Wait until FIFO has free space */
	}

	config->regs->UART2_RBR_THR_DLL_REG = out_char;

	pm_device_runtime_put(dev);

	k_spin_unlock(&data->lock, key);
}

static void apply_runtime_config(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	CRG_COM->SET_CLK_COM_REG = config->periph_clock_config;

	config->regs->UART2_MCR_REG = data->runtime_cfg.mcr_reg_val;
	config->regs->UART2_SRR_REG = UART2_UART2_SRR_REG_UART_UR_Msk |
				      UART2_UART2_SRR_REG_UART_RFR_Msk |
				      UART2_UART2_SRR_REG_UART_XFR_Msk;

	/* Configure baudrate */
	config->regs->UART2_LCR_REG |= UART2_UART2_LCR_REG_UART_DLAB_Msk;
	config->regs->UART2_IER_DLH_REG = BAUDRATE_CFG_DLH(data->runtime_cfg.baudrate_cfg);
	config->regs->UART2_RBR_THR_DLL_REG = BAUDRATE_CFG_DLL(data->runtime_cfg.baudrate_cfg);
	config->regs->UART2_DLF_REG = BAUDRATE_CFG_DLF(data->runtime_cfg.baudrate_cfg);
	config->regs->UART2_LCR_REG &= ~UART2_UART2_LCR_REG_UART_DLAB_Msk;

	/* Configure frame */
	config->regs->UART2_LCR_REG = data->runtime_cfg.lcr_reg_val;

	/* Enable hardware FIFO */
	config->regs->UART2_SFE_REG = UART2_UART2_SFE_REG_UART_SHADOW_FIFO_ENABLE_Msk;

	config->regs->UART2_SRT_REG = RX_FIFO_TRIG_1_CHAR;
	config->regs->UART2_STET_REG = TX_FIFO_TRIG_1_2_FULL;
	config->regs->UART2_IER_DLH_REG = data->runtime_cfg.ier_reg_val;

	k_spin_unlock(&data->lock, key);
}

static int uart_smartbond_configure(const struct device *dev,
				    const struct uart_config *cfg)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;
	uint32_t baudrate_cfg = 0;
	uint32_t lcr_reg_val;
	int err;
	int i;

	if ((cfg->parity != UART_CFG_PARITY_NONE && cfg->parity != UART_CFG_PARITY_ODD &&
	     cfg->parity != UART_CFG_PARITY_EVEN) ||
	    (cfg->stop_bits != UART_CFG_STOP_BITS_1 && cfg->stop_bits != UART_CFG_STOP_BITS_2) ||
	    (cfg->data_bits != UART_CFG_DATA_BITS_5 && cfg->data_bits != UART_CFG_DATA_BITS_6 &&
	     cfg->data_bits != UART_CFG_DATA_BITS_7 && cfg->data_bits != UART_CFG_DATA_BITS_8) ||
	    (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE &&
	     cfg->flow_ctrl != UART_CFG_FLOW_CTRL_RTS_CTS)) {
		return -ENOTSUP;
	}

	/* Flow control is not supported on UART */
	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS &&
	    !config->hw_flow_control_supported) {
		return -ENOTSUP;
	}

	/* Lookup configuration for baudrate */
	for (i = 0; i < ARRAY_SIZE(uart_smartbond_baudrate_table); i++) {
		if (uart_smartbond_baudrate_table[i].baudrate == cfg->baudrate) {
			baudrate_cfg = uart_smartbond_baudrate_table[i].cfg;
			break;
		}
	}

	if (baudrate_cfg == 0) {
		return -ENOTSUP;
	}

	/* Calculate frame configuration register value */
	lcr_reg_val = 0;

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		break;
	case UART_CFG_PARITY_EVEN:
		lcr_reg_val |= UART2_UART2_LCR_REG_UART_EPS_Msk;
		/* no break */
	case UART_CFG_PARITY_ODD:
		lcr_reg_val |= UART2_UART2_LCR_REG_UART_PEN_Msk;
		break;
	}

	if (cfg->stop_bits == UART_CFG_STOP_BITS_2)  {
		lcr_reg_val |= STOP_BITS_2 << UART2_UART2_LCR_REG_UART_STOP_Pos;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_6:
		lcr_reg_val |= DATA_BITS_6 << UART2_UART2_LCR_REG_UART_DLS_Pos;
		break;
	case UART_CFG_DATA_BITS_7:
		lcr_reg_val |= DATA_BITS_7 << UART2_UART2_LCR_REG_UART_DLS_Pos;
		break;
	case UART_CFG_DATA_BITS_8:
		lcr_reg_val |= DATA_BITS_8 << UART2_UART2_LCR_REG_UART_DLS_Pos;
		break;
	}

	data->runtime_cfg.baudrate_cfg = baudrate_cfg;
	data->runtime_cfg.lcr_reg_val = lcr_reg_val;
	data->runtime_cfg.mcr_reg_val = cfg->flow_ctrl ? UART2_UART2_MCR_REG_UART_AFCE_Msk : 0;

	pm_device_runtime_get(dev);
	apply_runtime_config(dev);
	pm_device_runtime_put(dev);

	data->current_config = *cfg;

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_smartbond_config_get(const struct device *dev,
				   struct uart_config *cfg)
{
	struct uart_smartbond_data *data = dev->data;

	*cfg = data->current_config;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#if CONFIG_PM_DEVICE

static void uart_smartbond_wake_handler(const struct device *gpio, struct gpio_callback *cb,
					uint32_t pins)
{
	struct uart_smartbond_data *data = CONTAINER_OF(cb, struct uart_smartbond_data,
							rx_wake_cb);

	/* Disable interrupts on UART RX pin to avoid repeated interrupts. */
	(void)gpio_pin_interrupt_configure(gpio, (find_msb_set(pins) - 1),
					   GPIO_INT_DISABLE);
	/* Refresh console expired time */
	if (data->rx_wake_timeout) {
		uart_smartbond_pm_policy_state_lock_get(data->dev);
		k_work_reschedule(&data->rx_timeout_work, K_MSEC(data->rx_wake_timeout));
	}
}

static void uart_smartbond_dtr_handler(const struct device *gpio, struct gpio_callback *cb,
				       uint32_t pins)
{
	struct uart_smartbond_data *data = CONTAINER_OF(cb, struct uart_smartbond_data,
							dtr_wake_cb);
	int pin = find_lsb_set(pins) - 1;

	if (gpio_pin_get(gpio, pin) == 1) {
		uart_smartbond_pm_policy_state_lock_put(data->dev);
	} else {
		uart_smartbond_pm_policy_state_lock_get(data->dev);
	}
}

#endif

static int uart_smartbond_init(const struct device *dev)
{
	struct uart_smartbond_data *data = dev->data;
	int ret = 0;

#ifdef CONFIG_PM_DEVICE_RUNTIME
	/* Make sure device state is marked as suspended */
	pm_device_init_suspended(dev);

	ret = pm_device_runtime_enable(dev);
#else
	da1469x_pd_acquire(MCU_PD_DOMAIN_COM);
#endif

#ifdef CONFIG_PM_DEVICE
	int rx_wake_timeout;
	const struct uart_smartbond_cfg *config = dev->config;
	const struct device *uart_console_dev =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	data->dev = dev;
	/* All uarts can have wake time specified in device tree to keep
	 * device awake after receiving data
	 */
	rx_wake_timeout = config->rx_wake_timeout;
	if (dev == uart_console_dev) {
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
		/* For device configured as console wake time is taken from
		 * Kconfig same way it is configured for other platforms
		 */
		rx_wake_timeout = CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT;
#endif
	}
	/* If DTR pin is configured, use it for power management */
	if (config->dtr_gpio.port != NULL) {
		gpio_init_callback(&data->dtr_wake_cb, uart_smartbond_dtr_handler,
				   BIT(config->dtr_gpio.pin));
		ret = gpio_add_callback(config->dtr_gpio.port, &data->dtr_wake_cb);
		if (ret == 0) {
			ret = gpio_pin_interrupt_configure_dt(&config->dtr_gpio,
							      GPIO_INT_MODE_EDGE |
							      GPIO_INT_TRIG_BOTH);
			/* Check if DTR is already active (low), if so lock power state */
			if (gpio_pin_get(config->dtr_gpio.port, config->dtr_gpio.pin) == 0) {
				uart_smartbond_pm_policy_state_lock_get(dev);
			}
		}
	}
	if (rx_wake_timeout > 0 && config->rx_wake_gpio.port != NULL) {
		k_work_init_delayable(&data->rx_timeout_work,
				      uart_smartbond_rx_refresh_timeout);
		gpio_init_callback(&data->rx_wake_cb, uart_smartbond_wake_handler,
				   BIT(config->rx_wake_gpio.pin));

		ret = gpio_add_callback(config->rx_wake_gpio.port, &data->rx_wake_cb);
		if (ret == 0) {
			data->rx_wake_timeout = rx_wake_timeout;
		}
	}
#endif

	ret = uart_smartbond_configure(dev, &data->current_config);
#ifndef CONFIG_PM_DEVICE_RUNTIME
	if (ret < 0) {
		da1469x_pd_release(MCU_PD_DOMAIN_COM);
	}
#endif

	return ret;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static inline void irq_tx_enable(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;

	config->regs->UART2_IER_DLH_REG |= UART2_UART2_IER_DLH_REG_PTIME_DLH7_Msk |
					   UART2_UART2_IER_DLH_REG_ETBEI_DLH1_Msk;
	data->runtime_cfg.ier_reg_val = config->regs->UART2_IER_DLH_REG;
}

static inline void irq_tx_disable(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;

	config->regs->UART2_IER_DLH_REG &= ~(UART2_UART2_IER_DLH_REG_PTIME_DLH7_Msk |
					UART2_UART2_IER_DLH_REG_ETBEI_DLH1_Msk);
	data->runtime_cfg.ier_reg_val = config->regs->UART2_IER_DLH_REG;
}

static inline void irq_rx_enable(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;

	config->regs->UART2_IER_DLH_REG |= UART2_UART2_IER_DLH_REG_ERBFI_DLH0_Msk;
	data->runtime_cfg.ier_reg_val = config->regs->UART2_IER_DLH_REG;
}

static inline void irq_rx_disable(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;

	config->regs->UART2_IER_DLH_REG &= ~UART2_UART2_IER_DLH_REG_ERBFI_DLH0_Msk;
	data->runtime_cfg.ier_reg_val = config->regs->UART2_IER_DLH_REG;
}

static int uart_smartbond_fifo_fill(const struct device *dev,
				  const uint8_t *tx_data,
				  int len)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;
	int num_tx = 0;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	pm_device_runtime_get(dev);
	while ((len - num_tx > 0) &&
	       (config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_TFNF_Msk)) {
		config->regs->UART2_RBR_THR_DLL_REG = tx_data[num_tx++];
	}

	if (data->tx_enabled) {
		irq_tx_enable(dev);
	}

	pm_device_runtime_put(dev);
	k_spin_unlock(&data->lock, key);

	return num_tx;
}

static int uart_smartbond_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int size)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;
	int num_rx = 0;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	pm_device_runtime_get(dev);
	while ((size - num_rx > 0) &&
	       (config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_RFNE_Msk)) {
		rx_data[num_rx++] = config->regs->UART2_RBR_THR_DLL_REG;
	}

	if (data->rx_enabled) {
		irq_rx_enable(dev);
	}

#ifdef CONFIG_PM_DEVICE
	if (data->rx_wake_timeout) {
		k_work_reschedule(&data->rx_timeout_work, K_MSEC(data->rx_wake_timeout));
	}
#endif

	pm_device_runtime_put(dev);
	k_spin_unlock(&data->lock, key);

	return num_rx;
}

static void uart_smartbond_irq_tx_enable(const struct device *dev)
{
	struct uart_smartbond_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->tx_enabled = 1;
	irq_tx_enable(dev);

	k_spin_unlock(&data->lock, key);
}

static void uart_smartbond_irq_tx_disable(const struct device *dev)
{
	struct uart_smartbond_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	irq_tx_disable(dev);
	data->tx_enabled = 0;

	k_spin_unlock(&data->lock, key);
}

static int uart_smartbond_irq_tx_ready(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;

	bool ret = (config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_TFNF_Msk) != 0;

	return ret;
}

static void uart_smartbond_irq_rx_enable(const struct device *dev)
{
	struct uart_smartbond_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->rx_enabled = 1;
	irq_rx_enable(dev);

	k_spin_unlock(&data->lock, key);
}

static void uart_smartbond_irq_rx_disable(const struct device *dev)
{
	struct uart_smartbond_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	irq_rx_disable(dev);
	data->rx_enabled = 0;

	k_spin_unlock(&data->lock, key);
}

static int uart_smartbond_irq_tx_complete(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;

	bool ret = (config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_TFE_Msk) != 0;

	return ret;
}

static int uart_smartbond_irq_rx_ready(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;

	bool ret = (config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_RFNE_Msk) != 0;

	return ret;
}

static void uart_smartbond_irq_err_enable(const struct device *dev)
{
	k_panic();
}

static void uart_smartbond_irq_err_disable(const struct device *dev)
{
	k_panic();
}

static int uart_smartbond_irq_is_pending(const struct device *dev)
{
	k_panic();

	return 0;
}

static int uart_smartbond_irq_update(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;
	bool no_intr = false;

	while (!no_intr) {
		switch (config->regs->UART2_IIR_FCR_REG & 0x0F) {
		case IIR_NO_INTR:
			no_intr = true;
			break;
		case IIR_THR_EMPTY:
			irq_tx_disable(dev);
			break;
		case IIR_RX_DATA:
			irq_rx_disable(dev);
			break;
		case IIR_LINE_STATUS:
		case IIR_TIMEOUT:
			/* ignore */
			break;
		case IIR_BUSY:
			/* busy detect */
			/* fall-through */
		default:
			k_panic();
			break;
		}
	}

	return 1;
}

static void uart_smartbond_irq_callback_set(const struct device *dev,
					  uart_irq_callback_user_data_t cb,
					  void *cb_data)
{
	struct uart_smartbond_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void uart_smartbond_isr(const struct device *dev)
{
	struct uart_smartbond_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_PM_DEVICE
static int uart_disable(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;

	/* Store IER register in case UART will go to sleep */
	data->runtime_cfg.ier_reg_val = config->regs->UART2_IER_DLH_REG;

	if (config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_RFNE_Msk) {
		return -EBUSY;
	}
	while (!(config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_TFE_Msk) ||
	       (config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_BUSY_Msk)) {
		/* Wait until FIFO is empty and UART finished tx */
		if (config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_RFNE_Msk) {
			return -EBUSY;
		}
	}

	CRG_COM->RESET_CLK_COM_REG = config->periph_clock_config;
	da1469x_pd_release(MCU_PD_DOMAIN_COM);

	return 0;
}

static int uart_smartbond_pm_action(const struct device *dev,
				enum pm_device_action action)
{
	const struct uart_smartbond_cfg *config;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
#ifdef CONFIG_PM_DEVICE_RUNTIME
		uart_smartbond_pm_prevent_system_sleep();
#endif
		da1469x_pd_acquire(MCU_PD_DOMAIN_COM);
		apply_runtime_config(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		config = dev->config;
		ret = uart_disable(dev);
		if (ret == 0 && config->rx_wake_gpio.port != NULL) {
			ret = gpio_pin_interrupt_configure_dt(&config->rx_wake_gpio,
							      GPIO_INT_MODE_EDGE |
							      GPIO_INT_TRIG_LOW);
		}
#ifdef CONFIG_PM_DEVICE_RUNTIME
		uart_smartbond_pm_allow_system_sleep();
#endif
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct uart_driver_api uart_smartbond_driver_api = {
	.poll_in = uart_smartbond_poll_in,
	.poll_out = uart_smartbond_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_smartbond_configure,
	.config_get = uart_smartbond_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_smartbond_fifo_fill,
	.fifo_read = uart_smartbond_fifo_read,
	.irq_tx_enable = uart_smartbond_irq_tx_enable,
	.irq_tx_disable = uart_smartbond_irq_tx_disable,
	.irq_tx_ready = uart_smartbond_irq_tx_ready,
	.irq_rx_enable = uart_smartbond_irq_rx_enable,
	.irq_rx_disable = uart_smartbond_irq_rx_disable,
	.irq_tx_complete = uart_smartbond_irq_tx_complete,
	.irq_rx_ready = uart_smartbond_irq_rx_ready,
	.irq_err_enable = uart_smartbond_irq_err_enable,
	.irq_err_disable = uart_smartbond_irq_err_disable,
	.irq_is_pending = uart_smartbond_irq_is_pending,
	.irq_update = uart_smartbond_irq_update,
	.irq_callback_set = uart_smartbond_irq_callback_set,
#endif  /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_SMARTBOND_CONFIGURE(id)			\
	do {						\
		IRQ_CONNECT(DT_INST_IRQN(id),		\
			    DT_INST_IRQ(id, priority),	\
			    uart_smartbond_isr,		\
			    DEVICE_DT_INST_GET(id), 0);	\
							\
		irq_enable(DT_INST_IRQN(id));		\
	} while (0)
#else
#define UART_SMARTBOND_CONFIGURE(id)
#endif

#ifdef CONFIG_PM_DEVICE
#define UART_PM_WAKE_RX_TIMEOUT(n)	\
	.rx_wake_timeout = (DT_INST_PROP_OR(n, rx_wake_timeout, 0)),
#define UART_PM_WAKE_RX_PIN(n)	\
	.rx_wake_gpio = GPIO_DT_SPEC_INST_GET_OR(n, rx_wake_gpios, {0}),
#define UART_PM_WAKE_DTR_PIN(n)	\
	.dtr_gpio = GPIO_DT_SPEC_INST_GET_OR(n, dtr_gpios, {0}),
#else
#define UART_PM_WAKE_RX_PIN(n) /* Not used */
#define UART_PM_WAKE_RX_TIMEOUT(n) /* Not used */
#define UART_PM_WAKE_DTR_PIN(n) /* Not used */
#endif

#define UART_SMARTBOND_DEVICE(id)								\
	PINCTRL_DT_INST_DEFINE(id);								\
	static const struct uart_smartbond_cfg uart_smartbond_##id##_cfg = {			\
		.regs = (UART2_Type *)DT_INST_REG_ADDR(id),					\
		.periph_clock_config = DT_INST_PROP(id, periph_clock_config),			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),					\
		.hw_flow_control_supported = DT_INST_PROP(id, hw_flow_control_supported),	\
		UART_PM_WAKE_RX_TIMEOUT(id)							\
		UART_PM_WAKE_RX_PIN(id)								\
		UART_PM_WAKE_DTR_PIN(id)							\
	};											\
	static struct uart_smartbond_data uart_smartbond_##id##_data = {			\
		.current_config = {								\
			.baudrate = DT_INST_PROP(id, current_speed),				\
			.parity = UART_CFG_PARITY_NONE,						\
			.stop_bits = UART_CFG_STOP_BITS_1,					\
			.data_bits = UART_CFG_DATA_BITS_8,					\
			.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,					\
		},										\
	};											\
	static int uart_smartbond_##id##_init(const struct device *dev)				\
	{											\
		UART_SMARTBOND_CONFIGURE(id);							\
		return uart_smartbond_init(dev);						\
	}											\
	PM_DEVICE_DT_INST_DEFINE(id, uart_smartbond_pm_action);					\
	DEVICE_DT_INST_DEFINE(id,								\
			      uart_smartbond_##id##_init,					\
			      PM_DEVICE_DT_INST_GET(id),					\
			      &uart_smartbond_##id##_data,					\
			      &uart_smartbond_##id##_cfg,					\
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,			\
			      &uart_smartbond_driver_api);					\

DT_INST_FOREACH_STATUS_OKAY(UART_SMARTBOND_DEVICE)
