/*
 * Copyright (c) 2025, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_uart

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>

/* ambiq-sdk includes */
#include <soc.h>

LOG_MODULE_REGISTER(uart_ambiq, CONFIG_UART_LOG_LEVEL);

#define UART_AMBIQ_RSR_ERROR_MASK                                                                  \
	(UART0_RSR_FESTAT_Msk | UART0_RSR_PESTAT_Msk | UART0_RSR_BESTAT_Msk | UART0_RSR_OESTAT_Msk)

#define UART_IO_RESUME_DELAY_US 100

#ifdef CONFIG_UART_ASYNC_API
struct uart_ambiq_async_tx {
	const uint8_t *buf;
	size_t len;
	int32_t timeout;
	struct k_work_delayable timeout_work;
	bool enabled;
};

struct uart_ambiq_async_rx {
	uint8_t *buf;
	size_t len;
	size_t offset;
	size_t counter;
	uint8_t *next_buf;
	size_t next_len;
	int32_t timeout;
	struct k_work_delayable timeout_work;
	bool enabled;
};

struct uart_ambiq_async_data {
	const struct device *uart_dev;
	struct uart_ambiq_async_tx tx;
	struct uart_ambiq_async_rx rx;
	uart_callback_t cb;
	void *user_data;
	volatile bool dma_rdy;
};
#endif

struct uart_ambiq_config {
	uint32_t base;
	int size;
	int inst_idx;
	uint32_t clk_src;
	const struct pinctrl_dev_config *pincfg;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	void (*irq_config_func)(const struct device *dev);
#endif
};

/* Device data structure */
struct uart_ambiq_data {
	am_hal_uart_config_t hal_cfg;
	struct uart_config uart_cfg;
	void *uart_handler;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	volatile bool sw_call_txdrdy;
	uart_irq_callback_user_data_t irq_cb;
	struct k_spinlock irq_cb_lock;
	void *irq_cb_data;
#endif
#ifdef CONFIG_UART_ASYNC_API
	struct uart_ambiq_async_data async;
#endif
	bool tx_poll_trans_on;
	bool tx_int_trans_on;
	bool pm_policy_state_on;
};

static void uart_ambiq_pm_policy_state_lock_get_unconditional(void)
{
	if (IS_ENABLED(CONFIG_PM)) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	}
}

static void uart_ambiq_pm_policy_state_lock_get(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_PM)) {
		struct uart_ambiq_data *data = dev->data;

		if (!data->pm_policy_state_on) {
			data->pm_policy_state_on = true;
			uart_ambiq_pm_policy_state_lock_get_unconditional();
		}
	}
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
static void uart_ambiq_pm_policy_state_lock_put_unconditional(void)
{
	if (IS_ENABLED(CONFIG_PM)) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	}
}

static void uart_ambiq_pm_policy_state_lock_put(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_PM)) {
		struct uart_ambiq_data *data = dev->data;

		if (data->pm_policy_state_on) {
			data->pm_policy_state_on = false;
			uart_ambiq_pm_policy_state_lock_put_unconditional();
		}
	}
}
#endif

static int uart_ambiq_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_ambiq_config *config = dev->config;
	struct uart_ambiq_data *data = dev->data;

	data->hal_cfg.eTXFifoLevel = AM_HAL_UART_FIFO_LEVEL_16;
	data->hal_cfg.eRXFifoLevel = AM_HAL_UART_FIFO_LEVEL_16;
	data->hal_cfg.ui32BaudRate = cfg->baudrate;

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		data->hal_cfg.eDataBits = AM_HAL_UART_DATA_BITS_5;
		break;
	case UART_CFG_DATA_BITS_6:
		data->hal_cfg.eDataBits = AM_HAL_UART_DATA_BITS_6;
		break;
	case UART_CFG_DATA_BITS_7:
		data->hal_cfg.eDataBits = AM_HAL_UART_DATA_BITS_7;
		break;
	case UART_CFG_DATA_BITS_8:
		data->hal_cfg.eDataBits = AM_HAL_UART_DATA_BITS_8;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		data->hal_cfg.eStopBits = AM_HAL_UART_ONE_STOP_BIT;
		break;
	case UART_CFG_STOP_BITS_2:
		data->hal_cfg.eStopBits = AM_HAL_UART_TWO_STOP_BITS;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		data->hal_cfg.eFlowControl = AM_HAL_UART_FLOW_CTRL_NONE;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		data->hal_cfg.eFlowControl = AM_HAL_UART_FLOW_CTRL_RTS_CTS;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		data->hal_cfg.eParity = AM_HAL_UART_PARITY_NONE;
		break;
	case UART_CFG_PARITY_EVEN:
		data->hal_cfg.eParity = AM_HAL_UART_PARITY_EVEN;
		break;
	case UART_CFG_PARITY_ODD:
		data->hal_cfg.eParity = AM_HAL_UART_PARITY_ODD;
		break;
	default:
		return -ENOTSUP;
	}

	switch (config->clk_src) {
	case 0:
		data->hal_cfg.eClockSrc = AM_HAL_UART_CLOCK_SRC_HFRC;
		break;
	case 1:
		data->hal_cfg.eClockSrc = AM_HAL_UART_CLOCK_SRC_SYSPLL;
		break;
	default:
		return -EINVAL;
	}

	if (am_hal_uart_configure(data->uart_handler, &data->hal_cfg) != AM_HAL_STATUS_SUCCESS) {
		return -EINVAL;
	}

	data->uart_cfg = *cfg;

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_ambiq_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_ambiq_data *data = dev->data;

	*cfg = data->uart_cfg;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static bool uart_ambiq_is_readable(const struct device *dev)
{
	const struct uart_ambiq_config *config = dev->config;
	struct uart_ambiq_data *data = dev->data;
	uint32_t flag = 0;

	if (!(UARTn(config->inst_idx)->CR & UART0_CR_UARTEN_Msk) ||
	    !(UARTn(config->inst_idx)->CR & UART0_CR_RXE_Msk)) {
		return false;
	}
	am_hal_uart_flags_get(data->uart_handler, &flag);

	return (flag & UART0_FR_RXFE_Msk) == 0U;
}

static int uart_ambiq_poll_in(const struct device *dev, unsigned char *c)
{
	struct uart_ambiq_data *data = dev->data;
	uint32_t flag = 0;

	if (!uart_ambiq_is_readable(dev)) {
		return -1;
	}

	/* got a character */
	am_hal_uart_fifo_read(data->uart_handler, c, 1, NULL);
	am_hal_uart_flags_get(data->uart_handler, &flag);
	return flag & UART_AMBIQ_RSR_ERROR_MASK;
}

static void uart_ambiq_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_ambiq_data *data = dev->data;
	uint32_t flag = 0;
	unsigned int key;

	/* Wait for space in FIFO */
	do {
		am_hal_uart_flags_get(data->uart_handler, &flag);
	} while (flag & UART0_FR_TXFF_Msk);

	key = irq_lock();

	/* If an interrupt transmission is in progress, the pm constraint is already managed by the
	 * call of uart_ambiq_irq_tx_[en|dis]able
	 */
	if (!data->tx_poll_trans_on && !data->tx_int_trans_on) {
		data->tx_poll_trans_on = true;

		/* Don't allow system to suspend until
		 * transmission has completed
		 */
		uart_ambiq_pm_policy_state_lock_get(dev);
		am_hal_uart_interrupt_enable(data->uart_handler, AM_HAL_UART_INT_TXCMP);
	}

	/* Send a character */
	am_hal_uart_fifo_write(data->uart_handler, &c, 1, NULL);

	irq_unlock(key);
}

static int uart_ambiq_err_check(const struct device *dev)
{
	const struct uart_ambiq_config *cfg = dev->config;
	int errors = 0;

	if (UARTn(cfg->inst_idx)->RSR & AM_HAL_UART_RSR_OESTAT) {
		errors |= UART_ERROR_OVERRUN;
	}

	if (UARTn(cfg->inst_idx)->RSR & AM_HAL_UART_RSR_BESTAT) {
		errors |= UART_BREAK;
	}

	if (UARTn(cfg->inst_idx)->RSR & AM_HAL_UART_RSR_PESTAT) {
		errors |= UART_ERROR_PARITY;
	}

	if (UARTn(cfg->inst_idx)->RSR & AM_HAL_UART_RSR_FESTAT) {
		errors |= UART_ERROR_FRAMING;
	}

	return errors;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_ambiq_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	struct uart_ambiq_data *data = dev->data;
	int num_tx = 0U;
	unsigned int key;

	/* Lock interrupts to prevent nested interrupts or thread switch */
	key = irq_lock();

	am_hal_uart_fifo_write(data->uart_handler, (uint8_t *)tx_data, len, &num_tx);

	irq_unlock(key);

	return num_tx;
}

static int uart_ambiq_fifo_read(const struct device *dev, uint8_t *rx_data, const int len)
{
	struct uart_ambiq_data *data = dev->data;
	int num_rx = 0U;

	am_hal_uart_fifo_read(data->uart_handler, rx_data, len, &num_rx);

	return num_rx;
}

static void uart_ambiq_irq_tx_enable(const struct device *dev)
{
	const struct uart_ambiq_config *cfg = dev->config;
	struct uart_ambiq_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
	data->tx_poll_trans_on = false;
	data->tx_int_trans_on = true;
	uart_ambiq_pm_policy_state_lock_get(dev);

	am_hal_uart_interrupt_enable(data->uart_handler,
				     (AM_HAL_UART_INT_TX | AM_HAL_UART_INT_TXCMP));

	irq_unlock(key);

	if (!data->sw_call_txdrdy) {
		return;
	}
	data->sw_call_txdrdy = false;

	/*
	 * Verify if the callback has been registered. Due to HW limitation, the
	 * first TX interrupt should be triggered by the software.
	 *
	 * PL011 TX interrupt is based on a transition through a level, rather
	 * than on the level itself[1]. So that, enable TX interrupt can not
	 * trigger TX interrupt if no data was filled to TX FIFO at the
	 * beginning.
	 *
	 * [1]: PrimeCell UART (PL011) Technical Reference Manual
	 *      functional-overview/interrupts
	 */
	if (!data->irq_cb) {
		return;
	}

	/*
	 * Execute callback while TX interrupt remains enabled. If
	 * uart_fifo_fill() is called with small amounts of data, the 1/8 TX
	 * FIFO threshold may never be reached, and the hardware TX interrupt
	 * will never trigger.
	 */
	while (UARTn(cfg->inst_idx)->IER & AM_HAL_UART_INT_TX) {
		K_SPINLOCK(&data->irq_cb_lock) {
			data->irq_cb(dev, data->irq_cb_data);
		}
	}
}

static void uart_ambiq_irq_tx_disable(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;
	unsigned int key;

	key = irq_lock();

	data->sw_call_txdrdy = true;
	am_hal_uart_interrupt_disable(data->uart_handler,
				      (AM_HAL_UART_INT_TX | AM_HAL_UART_INT_TXCMP));
	data->tx_int_trans_on = false;
	uart_ambiq_pm_policy_state_lock_put(dev);

	irq_unlock(key);
}

static int uart_ambiq_irq_tx_complete(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;
	uint32_t flag = 0;
	/* Check for UART is busy transmitting data. */
	am_hal_uart_flags_get(data->uart_handler, &flag);
	return ((flag & AM_HAL_UART_FR_BUSY) == 0);
}

static int uart_ambiq_irq_tx_ready(const struct device *dev)
{
	const struct uart_ambiq_config *cfg = dev->config;
	struct uart_ambiq_data *data = dev->data;
	uint32_t status, flag, ier = 0;

	if (!(UARTn(cfg->inst_idx)->CR & UART0_CR_TXE_Msk)) {
		return false;
	}

	/* Check for TX interrupt status is set or TX FIFO is empty. */
	am_hal_uart_interrupt_status_get(data->uart_handler, &status, false);
	am_hal_uart_flags_get(data->uart_handler, &flag);
	am_hal_uart_interrupt_enable_get(data->uart_handler, &ier);
	return ((ier & AM_HAL_UART_INT_TX) &&
		((status & UART0_IES_TXRIS_Msk) || (flag & AM_HAL_UART_FR_TX_EMPTY)));
}

static void uart_ambiq_irq_rx_enable(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;

	am_hal_uart_interrupt_enable(data->uart_handler,
				     (AM_HAL_UART_INT_RX | AM_HAL_UART_INT_RX_TMOUT));
}

static void uart_ambiq_irq_rx_disable(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;

	am_hal_uart_interrupt_disable(data->uart_handler,
				      (AM_HAL_UART_INT_RX | AM_HAL_UART_INT_RX_TMOUT));
}

static int uart_ambiq_irq_rx_ready(const struct device *dev)
{
	const struct uart_ambiq_config *cfg = dev->config;
	struct uart_ambiq_data *data = dev->data;
	uint32_t flag = 0;
	uint32_t ier = 0;

	if (!(UARTn(cfg->inst_idx)->CR & UART0_CR_RXE_Msk)) {
		return false;
	}

	am_hal_uart_flags_get(data->uart_handler, &flag);
	am_hal_uart_interrupt_enable_get(data->uart_handler, &ier);
	return ((ier & AM_HAL_UART_INT_RX) && (!(flag & AM_HAL_UART_FR_RX_EMPTY)));
}

static void uart_ambiq_irq_err_enable(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;
	/* enable framing, parity, break, and overrun */
	am_hal_uart_interrupt_enable(data->uart_handler,
				     (AM_HAL_UART_INT_FRAME_ERR | AM_HAL_UART_INT_PARITY_ERR |
				      AM_HAL_UART_INT_BREAK_ERR | AM_HAL_UART_INT_OVER_RUN));
}

static void uart_ambiq_irq_err_disable(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;

	am_hal_uart_interrupt_disable(data->uart_handler,
				      (AM_HAL_UART_INT_FRAME_ERR | AM_HAL_UART_INT_PARITY_ERR |
				       AM_HAL_UART_INT_BREAK_ERR | AM_HAL_UART_INT_OVER_RUN));
}

static int uart_ambiq_irq_is_pending(const struct device *dev)
{
	return uart_ambiq_irq_rx_ready(dev) || uart_ambiq_irq_tx_ready(dev);
}

static int uart_ambiq_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_ambiq_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_ambiq_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = cb_data;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
static void async_user_callback(const struct device *dev, struct uart_event *evt);
static void uart_ambiq_async_tx_timeout(struct k_work *work);
static void uart_ambiq_async_rx_timeout(struct k_work *work);
#endif /* CONFIG_UART_ASYNC_API */

static int uart_ambiq_init(const struct device *dev)
{
	const struct uart_ambiq_config *config = dev->config;
	struct uart_ambiq_data *data = dev->data;
	int ret = 0;

	if (AM_HAL_STATUS_SUCCESS !=
	    am_hal_uart_initialize(config->inst_idx, &data->uart_handler)) {
		LOG_ERR("Fail to initialize UART\n");
		return -ENXIO;
	}

	ret = am_hal_uart_power_control(data->uart_handler, AM_HAL_SYSCTRL_WAKE, false);

	ret |= uart_ambiq_configure(dev, &data->uart_cfg);
	if (ret < 0) {
		LOG_ERR("Fail to config UART\n");
		goto end;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Fail to config UART pins\n");
		goto end;
	}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	config->irq_config_func(dev);
	data->sw_call_txdrdy = true;
#endif

#ifdef CONFIG_UART_ASYNC_API
	data->async.uart_dev = dev;
	k_work_init_delayable(&data->async.tx.timeout_work, uart_ambiq_async_tx_timeout);
	k_work_init_delayable(&data->async.rx.timeout_work, uart_ambiq_async_rx_timeout);
	data->async.rx.len = 0;
	data->async.rx.offset = 0;
	data->async.dma_rdy = true;
#endif

end:
	if (ret < 0) {
		am_hal_uart_deinitialize(data->uart_handler);
	}
	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int uart_ambiq_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct uart_ambiq_config *config = dev->config;
	struct uart_ambiq_data *data = dev->data;
	am_hal_sysctrl_power_state_e status;
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Set pins to active state */
		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			return err;
		}
		k_busy_wait(UART_IO_RESUME_DELAY_US);
		status = AM_HAL_SYSCTRL_WAKE;
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		am_hal_uart_tx_flush(data->uart_handler);
		/* Move pins to sleep state */
		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_SLEEP);
		if ((err < 0) && (err != -ENOENT)) {
			/*
			 * If returning -ENOENT, no pins where defined for sleep mode :
			 * Do not output on console (might sleep already) when going to sleep,
			 * "(LP)UART pinctrl sleep state not available"
			 * and don't block PM suspend.
			 * Else return the error.
			 */
			return err;
		}
		status = AM_HAL_SYSCTRL_DEEPSLEEP;
		break;
	default:
		return -ENOTSUP;
	}

	err = am_hal_uart_power_control(data->uart_handler, status, true);

	if (err != AM_HAL_STATUS_SUCCESS) {
		return -EPERM;
	} else {
		return 0;
	}
}
#endif /*CONFIG_PM_DEVICE*/

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
void uart_ambiq_isr(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;
	uint32_t status = 0;

	am_hal_uart_interrupt_status_get(data->uart_handler, &status, false);
	am_hal_uart_interrupt_clear(data->uart_handler, status);

	if (status & AM_HAL_UART_INT_TXCMP) {
		if (data->tx_poll_trans_on) {
			/* A poll transmission just completed,
			 * allow system to suspend
			 */
			am_hal_uart_interrupt_disable(data->uart_handler, AM_HAL_UART_INT_TXCMP);
			data->tx_poll_trans_on = false;
			uart_ambiq_pm_policy_state_lock_put(dev);
		}
		/* Transmission was either async or IRQ based,
		 * constraint will be released at the same time TXCMP IT
		 * is disabled
		 */
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Verify if the callback has been registered */
	if (data->irq_cb) {
		K_SPINLOCK(&data->irq_cb_lock) {
			data->irq_cb(dev, data->irq_cb_data);
		}
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
	am_hal_uart_interrupt_service(data->uart_handler, status);

	if (status & AM_HAL_UART_INT_TXCMP) {
		if (data->tx_int_trans_on) {
			struct uart_event tx_done = {
				.type = UART_TX_DONE,
				.data.tx.buf = data->async.tx.buf,
				.data.tx.len = data->async.tx.len,
			};
			async_user_callback(dev, &tx_done);
			data->tx_int_trans_on = false;
			data->async.dma_rdy = true;
			uart_ambiq_pm_policy_state_lock_put_unconditional();
		}
	}

	if (data->async.rx.timeout != SYS_FOREVER_US && data->async.rx.timeout != 0 &&
	    (status & AM_HAL_UART_INT_RX)) {
		k_work_reschedule(&data->async.rx.timeout_work, K_USEC(data->async.rx.timeout));
	}
#endif /* CONFIG_UART_ASYNC_API */
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#if defined(CONFIG_UART_ASYNC_API)
static inline void async_timer_start(struct k_work_delayable *work, int32_t timeout)
{
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		k_work_reschedule(work, K_USEC(timeout));
	}
}

static void async_user_callback(const struct device *dev, struct uart_event *evt)
{
	const struct uart_ambiq_data *data = dev->data;

	if (data->async.cb) {
		data->async.cb(dev, evt, data->async.user_data);
	}
}

static void uart_ambiq_async_tx_callback(uint32_t status, void *user_data)
{
	const struct device *dev = user_data;
	const struct uart_ambiq_config *config = dev->config;
	struct uart_ambiq_data *data = dev->data;
	struct uart_ambiq_async_tx *tx = &data->async.tx;

	unsigned int key = irq_lock();

	/* Skip callback if no DMA interrupt */
	if ((UARTn(config->inst_idx)->RSR_b.DMACPL == 0) &&
	    (UARTn(config->inst_idx)->RSR_b.DMAERR == 0)) {
		irq_unlock(key);
		return;
	}

	k_work_cancel_delayable(&tx->timeout_work);
	am_hal_uart_dma_transfer_complete(data->uart_handler);

	irq_unlock(key);
}

static int uart_ambiq_async_callback_set(const struct device *dev, uart_callback_t callback,
					 void *user_data)
{
	struct uart_ambiq_data *data = dev->data;

	data->async.cb = callback;
	data->async.user_data = user_data;

	return 0;
}

static int uart_ambiq_async_tx(const struct device *dev, const uint8_t *buf, size_t len,
			       int32_t timeout)
{
	struct uart_ambiq_data *data = dev->data;
	am_hal_uart_transfer_t uart_tx = {0};
	int ret = 0;

	if (!data->async.dma_rdy) {
		LOG_WRN("UART DMA busy");
		return -EBUSY;
	}
	data->async.dma_rdy = false;

#ifdef CONFIG_UART_AMBIQ_HANDLE_CACHE
	if (!buf_in_nocache((uintptr_t)buf, len)) {
		/* Clean Dcache before DMA write */
		sys_cache_data_flush_range((void *)buf, len);
	}
#endif /* CONFIG_UART_AMBIQ_HANDLE_CACHE */

	unsigned int key = irq_lock();

	data->async.tx.buf = buf;
	data->async.tx.len = len;
	data->async.tx.timeout = timeout;

	/* Do not allow system to suspend until transmission has completed */
	uart_ambiq_pm_policy_state_lock_get_unconditional();

	/* Enable interrupt so we can signal correct TX done */
	am_hal_uart_interrupt_enable(
		data->uart_handler,
		(AM_HAL_UART_INT_TXCMP | AM_HAL_UART_INT_DMACPRIS | AM_HAL_UART_INT_DMAERIS));

	uart_tx.eDirection = AM_HAL_UART_TX;
	uart_tx.ui32NumBytes = len;
	uart_tx.pui32TxBuffer = (uint32_t *)buf;
	uart_tx.pfnCallback = uart_ambiq_async_tx_callback;
	uart_tx.pvContext = (void *)dev;

	if (am_hal_uart_dma_transfer(data->uart_handler, &uart_tx) != AM_HAL_STATUS_SUCCESS) {
		ret = -EINVAL;
		LOG_ERR("Error starting Tx DMA (%d)", ret);
		irq_unlock(key);
		return ret;
	}
	data->tx_poll_trans_on = false;
	data->tx_int_trans_on = true;

	async_timer_start(&data->async.tx.timeout_work, timeout);

	irq_unlock(key);

	return ret;
}

static int uart_ambiq_async_tx_abort(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;
	const struct uart_ambiq_config *config = dev->config;
	size_t bytes_sent;

	unsigned int key = irq_lock();

	k_work_cancel_delayable(&data->async.tx.timeout_work);

	am_hal_uart_tx_abort(data->uart_handler);
	data->async.dma_rdy = true;

	bytes_sent = data->async.tx.len - UARTn(config->inst_idx)->COUNT_b.TOTCOUNT;

	irq_unlock(key);

	struct uart_event tx_aborted = {
		.type = UART_TX_ABORTED,
		.data.tx.buf = data->async.tx.buf,
		.data.tx.len = bytes_sent,
	};
	async_user_callback(dev, &tx_aborted);
	data->tx_int_trans_on = false;

	return 0;
}

static void uart_ambiq_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_ambiq_async_tx *tx =
		CONTAINER_OF(dwork, struct uart_ambiq_async_tx, timeout_work);
	struct uart_ambiq_async_data *async = CONTAINER_OF(tx, struct uart_ambiq_async_data, tx);
	struct uart_ambiq_data *data = CONTAINER_OF(async, struct uart_ambiq_data, async);

	uart_ambiq_async_tx_abort(data->async.uart_dev);

	LOG_DBG("tx: async timeout");
}

static int uart_ambiq_async_rx_disable(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;
	struct uart_event disabled_event = {.type = UART_RX_DISABLED};

	if (!data->async.rx.enabled) {
		async_user_callback(dev, &disabled_event);
		return -EFAULT;
	}

	unsigned int key = irq_lock();

	k_work_cancel_delayable(&data->async.rx.timeout_work);

	am_hal_uart_rx_abort(data->uart_handler);
	data->async.rx.enabled = false;
	data->async.dma_rdy = true;

	irq_unlock(key);

	/* Release current buffer event */
	struct uart_event rel_event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->async.rx.buf,
	};
	async_user_callback(dev, &rel_event);

	/* Disable RX event */
	async_user_callback(dev, &disabled_event);

	data->async.rx.buf = NULL;
	data->async.rx.len = 0;
	data->async.rx.counter = 0;
	data->async.rx.offset = 0;

	if (data->async.rx.next_buf) {
		/* Release next buffer event */
		struct uart_event next_rel_event = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = data->async.rx.next_buf,
		};
		async_user_callback(dev, &next_rel_event);
		data->async.rx.next_buf = NULL;
		data->async.rx.next_len = 0;
	}

	LOG_DBG("rx: disabled");

	return 0;
}

static void uart_ambiq_async_rx_callback(uint32_t status, void *user_data)
{
	const struct device *dev = user_data;
	const struct uart_ambiq_config *config = dev->config;
	struct uart_ambiq_data *data = dev->data;
	struct uart_ambiq_async_data *async = &data->async;
	size_t total_rx;

	total_rx = async->rx.len - UARTn(config->inst_idx)->COUNT_b.TOTCOUNT;

#if CONFIG_UART_AMBIQ_HANDLE_CACHE
	if (!buf_in_nocache((uintptr_t)async->rx.buf, total_rx)) {
		/* Invalidate Dcache after DMA read */
		sys_cache_data_invd_range((void *)async->rx.buf, total_rx);
	}
#endif /* CONFIG_UART_AMBIQ_HANDLE_CACHE */

	unsigned int key = irq_lock();

	am_hal_uart_interrupt_disable(data->uart_handler,
				      (AM_HAL_UART_INT_DMACPRIS | AM_HAL_UART_INT_DMAERIS));

	irq_unlock(key);

	if (total_rx > async->rx.offset) {
		async->rx.counter = total_rx - async->rx.offset;
		struct uart_event rdy_event = {
			.type = UART_RX_RDY,
			.data.rx.buf = async->rx.buf,
			.data.rx.len = async->rx.counter,
			.data.rx.offset = async->rx.offset,
		};
		async_user_callback(dev, &rdy_event);
	}

	if (async->rx.next_buf) {
		async->rx.offset = 0;
		async->rx.counter = 0;

		struct uart_event rel_event = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = async->rx.buf,
		};
		async_user_callback(dev, &rel_event);

		async->rx.buf = async->rx.next_buf;
		async->rx.len = async->rx.next_len;

		async->rx.next_buf = NULL;
		async->rx.next_len = 0;
		struct uart_event req_event = {
			.type = UART_RX_BUF_REQUEST,
		};
		async_user_callback(dev, &req_event);

		am_hal_uart_transfer_t uart_rx = {0};

		uart_rx.eDirection = AM_HAL_UART_RX;
		uart_rx.ui32NumBytes = async->rx.next_len;
		uart_rx.pui32RxBuffer = (uint32_t *)async->rx.next_buf;
		uart_rx.pfnCallback = uart_ambiq_async_rx_callback;
		uart_rx.pvContext = user_data;

		am_hal_uart_interrupt_enable(data->uart_handler,
					     (AM_HAL_UART_INT_DMACPRIS | AM_HAL_UART_INT_DMAERIS));

		am_hal_uart_dma_transfer(data->uart_handler, &uart_rx);

		async_timer_start(&async->rx.timeout_work, async->rx.timeout);
	} else {
		uart_ambiq_async_rx_disable(dev);
	}
}

static int uart_ambiq_async_rx_enable(const struct device *dev, uint8_t *buf, size_t len,
				      int32_t timeout)
{
	struct uart_ambiq_data *data = dev->data;
	am_hal_uart_transfer_t uart_rx = {0};
	int ret = 0;

	if (!data->async.dma_rdy) {
		LOG_WRN("UART DMA busy");
		return -EBUSY;
	}
	if (data->async.rx.enabled) {
		LOG_WRN("RX was already enabled");
		return -EBUSY;
	}

	unsigned int key = irq_lock();

	data->async.dma_rdy = false;
	data->async.rx.enabled = true;
	data->async.rx.buf = buf;
	data->async.rx.len = len;
	data->async.rx.timeout = timeout;

	uart_rx.eDirection = AM_HAL_UART_RX;
	uart_rx.ui32NumBytes = len;
	uart_rx.pui32RxBuffer = (uint32_t *)buf;
	uart_rx.pfnCallback = uart_ambiq_async_rx_callback;
	uart_rx.pvContext = (void *)dev;

	/* Disable RX interrupts to let DMA to handle it */
	uart_ambiq_irq_rx_disable(dev);
	am_hal_uart_interrupt_enable(data->uart_handler,
				     (AM_HAL_UART_INT_DMACPRIS | AM_HAL_UART_INT_DMAERIS));

	if (am_hal_uart_dma_transfer(data->uart_handler, &uart_rx) != AM_HAL_STATUS_SUCCESS) {
		ret = -EINVAL;
		LOG_ERR("Error starting Rx DMA (%d)", ret);
		irq_unlock(key);
		return ret;
	}

	async_timer_start(&data->async.rx.timeout_work, timeout);

	struct uart_event buf_req = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(dev, &buf_req);

	irq_unlock(key);

	LOG_DBG("async rx enabled");

	return ret;
}

static int uart_ambiq_async_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_ambiq_data *data = dev->data;
	unsigned int key;
	int ret = 0;

	LOG_DBG("replace buffer (%d)", len);

	key = irq_lock();

	if (data->async.rx.next_buf != NULL) {
		ret = -EBUSY;
	} else if (!data->async.rx.enabled) {
		ret = -EACCES;
	} else {
		data->async.rx.next_buf = buf;
		data->async.rx.next_len = len;
	}

	irq_unlock(key);

	return ret;
}

static void uart_ambiq_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_ambiq_async_rx *rx =
		CONTAINER_OF(dwork, struct uart_ambiq_async_rx, timeout_work);
	struct uart_ambiq_async_data *async = CONTAINER_OF(rx, struct uart_ambiq_async_data, rx);
	struct uart_ambiq_data *data = CONTAINER_OF(async, struct uart_ambiq_data, async);
	const struct uart_ambiq_config *config = data->async.uart_dev->config;
	uint32_t total_rx;

	LOG_DBG("rx timeout");

	unsigned int key = irq_lock();

	am_hal_uart_interrupt_disable(data->uart_handler,
				      (AM_HAL_UART_INT_DMACPRIS | AM_HAL_UART_INT_DMAERIS));

	k_work_cancel_delayable(&data->async.rx.timeout_work);

	irq_unlock(key);

	total_rx = async->rx.len - UARTn(config->inst_idx)->COUNT_b.TOTCOUNT;

	if (total_rx > async->rx.offset) {
		async->rx.counter = total_rx - async->rx.offset;
		struct uart_event rdy_event = {
			.type = UART_RX_RDY,
			.data.rx.buf = async->rx.buf,
			.data.rx.len = async->rx.counter,
			.data.rx.offset = async->rx.offset,
		};
		async_user_callback(async->uart_dev, &rdy_event);
		async->dma_rdy = true;
	}
	async->rx.offset += async->rx.counter;
	async->rx.counter = 0;

	am_hal_uart_interrupt_enable(data->uart_handler,
				     (AM_HAL_UART_INT_DMACPRIS | AM_HAL_UART_INT_DMAERIS));
}

#endif

static DEVICE_API(uart, uart_ambiq_driver_api) = {
	.poll_in = uart_ambiq_poll_in,
	.poll_out = uart_ambiq_poll_out,
	.err_check = uart_ambiq_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_ambiq_configure,
	.config_get = uart_ambiq_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_ambiq_fifo_fill,
	.fifo_read = uart_ambiq_fifo_read,
	.irq_tx_enable = uart_ambiq_irq_tx_enable,
	.irq_tx_disable = uart_ambiq_irq_tx_disable,
	.irq_tx_ready = uart_ambiq_irq_tx_ready,
	.irq_rx_enable = uart_ambiq_irq_rx_enable,
	.irq_rx_disable = uart_ambiq_irq_rx_disable,
	.irq_tx_complete = uart_ambiq_irq_tx_complete,
	.irq_rx_ready = uart_ambiq_irq_rx_ready,
	.irq_err_enable = uart_ambiq_irq_err_enable,
	.irq_err_disable = uart_ambiq_irq_err_disable,
	.irq_is_pending = uart_ambiq_irq_is_pending,
	.irq_update = uart_ambiq_irq_update,
	.irq_callback_set = uart_ambiq_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_ambiq_async_callback_set,
	.tx = uart_ambiq_async_tx,
	.tx_abort = uart_ambiq_async_tx_abort,
	.rx_enable = uart_ambiq_async_rx_enable,
	.rx_buf_rsp = uart_ambiq_async_rx_buf_rsp,
	.rx_disable = uart_ambiq_async_rx_disable,
#endif /* CONFIG_UART_ASYNC_API */
};

#define UART_AMBIQ_DECLARE_CFG(n, IRQ_FUNC_INIT)                                                   \
	static const struct uart_ambiq_config uart_ambiq_cfg_##n = {                               \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.size = DT_INST_REG_SIZE(n),                                                       \
		.inst_idx = (DT_INST_REG_ADDR(n) - UART0_BASE) / (UART1_BASE - UART0_BASE),        \
		.clk_src = DT_INST_PROP(n, clk_src),                                               \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		IRQ_FUNC_INIT}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
#define UART_AMBIQ_CONFIG_FUNC(n)                                                                  \
	static void uart_ambiq_irq_config_func_##n(const struct device *dev)                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_ambiq_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#define UART_AMBIQ_IRQ_CFG_FUNC_INIT(n) .irq_config_func = uart_ambiq_irq_config_func_##n
#define UART_AMBIQ_INIT_CFG(n)          UART_AMBIQ_DECLARE_CFG(n, UART_AMBIQ_IRQ_CFG_FUNC_INIT(n))
#else
#define UART_AMBIQ_CONFIG_FUNC(n)
#define UART_AMBIQ_IRQ_CFG_FUNC_INIT
#define UART_AMBIQ_INIT_CFG(n) UART_AMBIQ_DECLARE_CFG(n, UART_AMBIQ_IRQ_CFG_FUNC_INIT)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_AMBIQ_INIT(n)                                                                         \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct uart_ambiq_data uart_ambiq_data_##n = {                                      \
		.uart_cfg =                                                                        \
			{                                                                          \
				.baudrate = DT_INST_PROP(n, current_speed),                        \
				.parity = UART_CFG_PARITY_NONE,                                    \
				.stop_bits = UART_CFG_STOP_BITS_1,                                 \
				.data_bits = UART_CFG_DATA_BITS_8,                                 \
				.flow_ctrl = DT_INST_PROP(n, hw_flow_control)                      \
						     ? UART_CFG_FLOW_CTRL_RTS_CTS                  \
						     : UART_CFG_FLOW_CTRL_NONE,                    \
			},                                                                         \
	};                                                                                         \
	static const struct uart_ambiq_config uart_ambiq_cfg_##n;                                  \
	PM_DEVICE_DT_INST_DEFINE(n, uart_ambiq_pm_action);                                         \
	DEVICE_DT_INST_DEFINE(n, uart_ambiq_init, PM_DEVICE_DT_INST_GET(n), &uart_ambiq_data_##n,  \
			      &uart_ambiq_cfg_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,      \
			      &uart_ambiq_driver_api);                                             \
	UART_AMBIQ_CONFIG_FUNC(n)                                                                  \
	UART_AMBIQ_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(UART_AMBIQ_INIT)
