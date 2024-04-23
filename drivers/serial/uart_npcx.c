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
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	uart_irq_config_func_t irq_config_func;
#endif
	/* clock configuration */
	struct npcx_clk_cfg clk_cfg;
	/* int-mux configuration */
	const struct npcx_wui uart_rx_wui;
	/* pinmux configuration */
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_UART_ASYNC_API
	struct npcx_clk_cfg mdma_clk_cfg;
	struct mdma_reg *mdma_reg_base;
#endif
};

enum uart_pm_policy_state_flag {
	UART_PM_POLICY_STATE_TX_FLAG,
	UART_PM_POLICY_STATE_RX_FLAG,

	UART_PM_POLICY_STATE_FLAG_COUNT,
};

#ifdef CONFIG_UART_ASYNC_API
struct uart_npcx_rx_dma_params {
	uint8_t *buf;
	size_t buf_len;
	size_t offset;
	size_t counter;
	size_t timeout_us;
	struct k_work_delayable timeout_work;
	bool enabled;
};

struct uart_npcx_tx_dma_params {
	const uint8_t *buf;
	size_t buf_len;
	struct k_work_delayable timeout_work;
	size_t timeout_us;
};

struct uart_npcx_async_data {
	const struct device *uart_dev;
	uart_callback_t user_callback;
	void *user_data;
	struct uart_npcx_rx_dma_params rx_dma_params;
	struct uart_npcx_tx_dma_params tx_dma_params;
	uint8_t *next_rx_buffer;
	size_t next_rx_buffer_len;
	bool tx_in_progress;
};
#endif

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
#ifdef CONFIG_UART_ASYNC_API
	struct uart_npcx_async_data async;
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

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
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
	inst->UFTCTL &= ~(BIT(NPCX_UFTCTL_TEMPTY_LVL_EN) | BIT(NPCX_UFTCTL_TEMPTY_EN) |
			  BIT(NPCX_UFTCTL_NXMIP_EN));
}

static void uart_npcx_clear_rx_fifo(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;
	uint8_t scratch;

	/* Read all dummy bytes out from Rx FIFO */
	while (uart_npcx_rx_fifo_available(dev)) {
		scratch = inst->URBUF;
	}
}

#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_npcx_tx_fifo_ready(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

	/* True if the Tx FIFO is not completely full */
	return !(GET_FIELD(inst->UFTSTS, NPCX_UFTSTS_TEMPTY_LVL) == 0);
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

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->async.user_callback = NULL;
	data->async.user_data = NULL;
#endif
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
	while (!uart_npcx_fifo_fill(dev, &c, 1)) {
		continue;
	}
}

#else  /* !CONFIG_UART_INTERRUPT_DRIVEN */

/*
 * Poll-in implementation for byte mode config, read byte from URBUF if
 * available.
 */
static int uart_npcx_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;

	/* Rx single byte buffer is not full */
	if (!IS_BIT_SET(inst->UICTRL, NPCX_UICTRL_RBF)) {
		return -1;
	}

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
	while (!IS_BIT_SET(inst->UICTRL, NPCX_UICTRL_TBE)) {
		continue;
	}

	inst->UTBUF = c;
}
#endif /* !CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
static void async_user_callback(const struct device *dev, struct uart_event *evt)
{
	const struct uart_npcx_data *data = dev->data;

	if (data->async.user_callback) {
		data->async.user_callback(dev, evt, data->async.user_data);
	}
}

static void async_evt_rx_rdy(const struct device *dev)
{
	struct uart_npcx_data *data = dev->data;
	struct uart_npcx_rx_dma_params *rx_dma_params = &data->async.rx_dma_params;

	struct uart_event event = {.type = UART_RX_RDY,
				   .data.rx.buf = rx_dma_params->buf,
				   .data.rx.len = rx_dma_params->counter - rx_dma_params->offset,
				   .data.rx.offset = rx_dma_params->offset};

	LOG_DBG("RX Ready: (len: %d off: %d buf: %x)", event.data.rx.len, event.data.rx.offset,
		(uint32_t)event.data.rx.buf);

	/* Update the current pos for new data */
	rx_dma_params->offset = rx_dma_params->counter;

	/* Only send event for new data */
	if (event.data.rx.len > 0) {
		async_user_callback(dev, &event);
	}
}

static void async_evt_tx_done(const struct device *dev)
{
	struct uart_npcx_data *data = dev->data;

	(void)k_work_cancel_delayable(&data->async.tx_dma_params.timeout_work);

	LOG_DBG("TX done: %d", data->async.tx_dma_params.buf_len);

	struct uart_event event = {.type = UART_TX_DONE,
				   .data.tx.buf = data->async.tx_dma_params.buf,
				   .data.tx.len = data->async.tx_dma_params.buf_len};

	/* Reset TX Buffer */
	data->async.tx_dma_params.buf = NULL;
	data->async.tx_dma_params.buf_len = 0U;
	async_user_callback(dev, &event);
}

static void uart_npcx_async_rx_dma_get_status(const struct device *dev, size_t *pending_length)
{
	const struct uart_npcx_config *const config = dev->config;
	struct mdma_reg *const mdma_reg_base = config->mdma_reg_base;

	if (IS_BIT_SET(mdma_reg_base->MDMA_CTL0, NPCX_MDMA_CTL_MDMAEN)) {
		*pending_length = mdma_reg_base->MDMA_CTCNT0;
	} else {
		*pending_length = 0;
	}
}

static void uart_npcx_async_rx_flush(const struct device *dev)
{
	struct uart_npcx_data *data = dev->data;
	struct uart_npcx_rx_dma_params *rx_dma_params = &data->async.rx_dma_params;
	size_t curr_rcv_len, dma_pending_len;

	uart_npcx_async_rx_dma_get_status(dev, &dma_pending_len);
	curr_rcv_len = rx_dma_params->buf_len - dma_pending_len;

	if (curr_rcv_len > rx_dma_params->offset) {
		rx_dma_params->counter = curr_rcv_len;
		async_evt_rx_rdy(dev);
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
		k_timeout_t delay = K_MSEC(CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT);

		uart_npcx_pm_policy_state_lock_get(data, UART_PM_POLICY_STATE_RX_FLAG);
		k_work_reschedule(&data->rx_refresh_timeout_work, delay);
#endif
	}
}

static void async_evt_rx_buf_request(const struct device *dev)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(dev, &evt);
}

static int uart_npcx_async_callback_set(const struct device *dev, uart_callback_t callback,
					void *user_data)
{
	struct uart_npcx_data *data = dev->data;

	data->async.user_callback = callback;
	data->async.user_data = user_data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->user_cb = NULL;
	data->user_data = NULL;
#endif

	return 0;
}

static inline void async_timer_start(struct k_work_delayable *work, uint32_t timeout_us)
{
	if ((timeout_us != SYS_FOREVER_US) && (timeout_us != 0)) {
		LOG_DBG("async timer started for %d us", timeout_us);
		k_work_reschedule(work, K_USEC(timeout_us));
	}
}

static int uart_npcx_async_tx_dma_get_status(const struct device *dev, size_t *pending_length)
{
	const struct uart_npcx_config *const config = dev->config;
	struct mdma_reg *const mdma_reg_base = config->mdma_reg_base;

	if (IS_BIT_SET(mdma_reg_base->MDMA_CTL1, NPCX_MDMA_CTL_MDMAEN)) {
		*pending_length = mdma_reg_base->MDMA_CTCNT1;
	} else {
		*pending_length = 0;
		return -EBUSY;
	}

	return 0;
}

static int uart_npcx_async_tx(const struct device *dev, const uint8_t *buf, size_t len,
			      int32_t timeout)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;
	struct mdma_reg *const mdma_reg_base = config->mdma_reg_base;
	struct uart_npcx_data *data = dev->data;
	struct uart_npcx_tx_dma_params *tx_dma_params = &data->async.tx_dma_params;
	int key = irq_lock();

	if (buf == NULL || len == 0) {
		irq_unlock(key);
		return -EINVAL;
	}

	if (tx_dma_params->buf) {
		irq_unlock(key);
		return -EBUSY;
	}

	data->async.tx_in_progress = true;

	data->async.tx_dma_params.buf = buf;
	data->async.tx_dma_params.buf_len = len;
	data->async.tx_dma_params.timeout_us = timeout;

	mdma_reg_base->MDMA_SRCB1 = (uint32_t)buf;
	mdma_reg_base->MDMA_TCNT1 = len;

	async_timer_start(&data->async.tx_dma_params.timeout_work, timeout);
	mdma_reg_base->MDMA_CTL1 |= BIT(NPCX_MDMA_CTL_MDMAEN) | BIT(NPCX_MDMA_CTL_SIEN);

	inst->UMDSL |= BIT(NPCX_UMDSL_ETD);

#ifdef CONFIG_PM
	/* Do not allow system to suspend until transmission has completed */
	uart_npcx_pm_policy_state_lock_get(data, UART_PM_POLICY_STATE_TX_FLAG);
#endif
	irq_unlock(key);

	return 0;
}

static int uart_npcx_async_tx_abort(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_npcx_data *data = dev->data;
	struct mdma_reg *const mdma_reg_base = config->mdma_reg_base;
	size_t dma_pending_len, bytes_transmitted;
	int ret;

	k_work_cancel_delayable(&data->async.tx_dma_params.timeout_work);

	mdma_reg_base->MDMA_CTL1 &= ~BIT(NPCX_MDMA_CTL_MDMAEN);

	ret = uart_npcx_async_tx_dma_get_status(dev, &dma_pending_len);
	if (ret != 0) {
		bytes_transmitted = 0;
	} else {
		bytes_transmitted = data->async.tx_dma_params.buf_len - dma_pending_len;
	}

	struct uart_event tx_aborted_event = {
		.type = UART_TX_ABORTED,
		.data.tx.buf = data->async.tx_dma_params.buf,
		.data.tx.len = bytes_transmitted,
	};
	async_user_callback(dev, &tx_aborted_event);

	return ret;
}

static void uart_npcx_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_npcx_tx_dma_params *tx_params =
		CONTAINER_OF(dwork, struct uart_npcx_tx_dma_params, timeout_work);
	struct uart_npcx_async_data *async_data =
		CONTAINER_OF(tx_params, struct uart_npcx_async_data, tx_dma_params);
	const struct device *dev = async_data->uart_dev;

	LOG_ERR("Async Tx Timeout");
	uart_npcx_async_tx_abort(dev);
}

static int uart_npcx_async_rx_enable(const struct device *dev, uint8_t *buf, const size_t len,
				     const int32_t timeout_us)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;
	struct mdma_reg *const mdma_reg_base = config->mdma_reg_base;
	struct uart_npcx_data *data = dev->data;
	struct uart_npcx_rx_dma_params *rx_dma_params = &data->async.rx_dma_params;
	unsigned int key;

	LOG_DBG("Enable RX DMA, len:%d", len);

	key = irq_lock();

	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(len > 0);

	rx_dma_params->timeout_us = timeout_us;
	rx_dma_params->buf = buf;
	rx_dma_params->buf_len = len;

	rx_dma_params->offset = 0;
	rx_dma_params->counter = 0;

	SET_FIELD(inst->UFRCTL, NPCX_UFRCTL_RFULL_LVL_SEL, 1);

	mdma_reg_base->MDMA_DSTB0 = (uint32_t)buf;
	mdma_reg_base->MDMA_TCNT0 = len;
	mdma_reg_base->MDMA_CTL0 |= BIT(NPCX_MDMA_CTL_MDMAEN) | BIT(NPCX_MDMA_CTL_SIEN);

	inst->UMDSL |= BIT(NPCX_UMDSL_ERD);

	rx_dma_params->enabled = true;

	async_evt_rx_buf_request(dev);

	inst->UFRCTL |= BIT(NPCX_UFRCTL_RNEMPTY_EN);

	irq_unlock(key);

	return 0;
}

static void async_evt_rx_buf_release(const struct device *dev)
{
	struct uart_npcx_data *data = dev->data;
	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->async.rx_dma_params.buf,
	};

	async_user_callback(dev, &evt);
	data->async.rx_dma_params.buf = NULL;
	data->async.rx_dma_params.buf_len = 0U;
	data->async.rx_dma_params.offset = 0U;
	data->async.rx_dma_params.counter = 0U;
}

static int uart_npcx_async_rx_disable(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;
	struct uart_npcx_data *data = dev->data;
	struct mdma_reg *const mdma_reg_base = config->mdma_reg_base;
	struct uart_npcx_rx_dma_params *rx_dma_params = &data->async.rx_dma_params;
	unsigned int key;

	LOG_DBG("Async RX Disable");

	key = irq_lock();
	inst->UFRCTL &= ~(BIT(NPCX_UFRCTL_RNEMPTY_EN));

	k_work_cancel_delayable(&rx_dma_params->timeout_work);

	if (rx_dma_params->buf == NULL) {
		LOG_DBG("No buffers to release from RX DMA!");
	} else {
		uart_npcx_async_rx_flush(dev);
		async_evt_rx_buf_release(dev);
	}

	rx_dma_params->enabled = false;

	if (data->async.next_rx_buffer != NULL) {
		rx_dma_params->buf = data->async.next_rx_buffer;
		rx_dma_params->buf_len = data->async.next_rx_buffer_len;
		data->async.next_rx_buffer = NULL;
		data->async.next_rx_buffer_len = 0;
		/* Release the next buffer as well */
		async_evt_rx_buf_release(dev);
	}

	mdma_reg_base->MDMA_CTL0 &= ~BIT(NPCX_MDMA_CTL_MDMAEN);

	struct uart_event disabled_event = {.type = UART_RX_DISABLED};

	async_user_callback(dev, &disabled_event);

	irq_unlock(key);

	return 0;
}

static int uart_npcx_async_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_npcx_data *data = dev->data;

	if (data->async.next_rx_buffer != NULL) {
		return -EBUSY;
	} else if (data->async.rx_dma_params.enabled == false) {
		return -EACCES;
	}

	data->async.next_rx_buffer = buf;
	data->async.next_rx_buffer_len = len;

	LOG_DBG("Next RX buf rsp, new: %d", len);

	return 0;
}

static void uart_npcx_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_npcx_rx_dma_params *rx_params =
		CONTAINER_OF(dwork, struct uart_npcx_rx_dma_params, timeout_work);
	struct uart_npcx_async_data *async_data =
		CONTAINER_OF(rx_params, struct uart_npcx_async_data, rx_dma_params);
	const struct device *dev = async_data->uart_dev;

	LOG_DBG("Async RX timeout");
	uart_npcx_async_rx_flush(dev);
}

static void uart_npcx_async_dma_load_new_rx_buf(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;
	struct mdma_reg *const mdma_reg_base = config->mdma_reg_base;
	struct uart_npcx_data *data = dev->data;
	struct uart_npcx_rx_dma_params *rx_dma_params = &data->async.rx_dma_params;

	rx_dma_params->offset = 0;
	rx_dma_params->counter = 0;

	rx_dma_params->buf = data->async.next_rx_buffer;
	rx_dma_params->buf_len = data->async.next_rx_buffer_len;
	data->async.next_rx_buffer = NULL;
	data->async.next_rx_buffer_len = 0;

	mdma_reg_base->MDMA_DSTB0 = (uint32_t)rx_dma_params->buf;
	mdma_reg_base->MDMA_TCNT0 = rx_dma_params->buf_len;
	mdma_reg_base->MDMA_CTL0 |= BIT(NPCX_MDMA_CTL_MDMAEN) | BIT(NPCX_MDMA_CTL_SIEN);
	inst->UMDSL |= BIT(NPCX_UMDSL_ERD);
}

/* DMA rx reaches the terminal Count */
static void uart_npcx_async_dma_rx_complete(const struct device *dev)
{
	struct uart_npcx_data *data = dev->data;
	struct uart_npcx_rx_dma_params *rx_dma_params = &data->async.rx_dma_params;

	rx_dma_params->counter = rx_dma_params->buf_len;

	async_evt_rx_rdy(dev);

	/* A new buffer was available.  */
	if (data->async.next_rx_buffer != NULL) {
		async_evt_rx_buf_release(dev);
		uart_npcx_async_dma_load_new_rx_buf(dev);
		/* Request the next buffer */
		async_evt_rx_buf_request(dev);
		async_timer_start(&rx_dma_params->timeout_work, rx_dma_params->timeout_us);
	} else {
		/* Buffer full without valid next buffer, disable RX DMA */
		LOG_DBG("Disabled RX DMA, no valid next buffer ");
		uart_npcx_async_rx_disable(dev);
	}
}
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
static void uart_npcx_isr(const struct device *dev)
{
	struct uart_npcx_data *data = dev->data;
#if defined(CONFIG_PM) || defined(CONFIG_UART_ASYNC_API)
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;
#endif

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

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
#endif

#ifdef CONFIG_UART_ASYNC_API
	if (data->async.user_callback) {
		struct mdma_reg *const mdma_reg_base = config->mdma_reg_base;

		/*
		 * Check rx in any way because the RFIFO_NEMPTY_STS is not valid when MDMA mode is
		 * used. This is needed when the rx timeout_us is zero. In the case that the
		 * rx timeout_us is not zero, rx_flush is done in the tiemout_work callback.
		 */
		if (data->async.rx_dma_params.timeout_us == 0) {
			uart_npcx_async_rx_flush(dev);
		} else if (IS_BIT_SET(inst->UFRCTL, NPCX_UFRCTL_RNEMPTY_EN)) {
			async_timer_start(&data->async.rx_dma_params.timeout_work,
					  data->async.rx_dma_params.timeout_us);
		}

		/* MDMA rx end interrupt */
		if (IS_BIT_SET(mdma_reg_base->MDMA_CTL0, NPCX_MDMA_CTL_TC) &&
		    IS_BIT_SET(mdma_reg_base->MDMA_CTL0, NPCX_MDMA_CTL_SIEN)) {
			mdma_reg_base->MDMA_CTL0 &= ~BIT(NPCX_MDMA_CTL_SIEN);
			/* TC is write-0-clear bit */
			mdma_reg_base->MDMA_CTL0 &= ~BIT(NPCX_MDMA_CTL_TC);
			inst->UMDSL &= ~BIT(NPCX_UMDSL_ERD);
			uart_npcx_async_dma_rx_complete(dev);
			LOG_DBG("DMA Rx TC");
		}

		/* MDMA tx done interrupt */
		if (IS_BIT_SET(mdma_reg_base->MDMA_CTL1, NPCX_MDMA_CTL_TC) &&
		    IS_BIT_SET(mdma_reg_base->MDMA_CTL1, NPCX_MDMA_CTL_SIEN)) {
			mdma_reg_base->MDMA_CTL1 &= ~BIT(NPCX_MDMA_CTL_SIEN);
			/* TC is write-0-clear bit */
			mdma_reg_base->MDMA_CTL1 &= ~BIT(NPCX_MDMA_CTL_TC);

			/*
			 * MDMA tx is done (i.e. all data in the memory are moved to UART tx FIFO),
			 * but data in the tx FIFO are not completely sent to the bus.
			 */
			if (!IS_BIT_SET(inst->UFTSTS, NPCX_UFTSTS_NXMIP)) {
				k_spinlock_key_t key = k_spin_lock(&data->lock);

				inst->UFTCTL |= BIT(NPCX_UFTCTL_NXMIP_EN);
				k_spin_unlock(&data->lock, key);
			} else {
				data->async.tx_in_progress = false;
#ifdef CONFIG_PM
				uart_npcx_pm_policy_state_lock_put(data,
								   UART_PM_POLICY_STATE_TX_FLAG);
#endif /* CONFIG_PM */
				async_evt_tx_done(dev);
			}
		}
	}
#endif

#if defined(CONFIG_PM) || defined(CONFIG_UART_ASYNC_API)
	if (IS_BIT_SET(inst->UFTCTL, NPCX_UFTCTL_NXMIP_EN) &&
	    IS_BIT_SET(inst->UFTSTS, NPCX_UFTSTS_NXMIP)) {
		k_spinlock_key_t key = k_spin_lock(&data->lock);

		/* Disable NXMIP interrupt */
		inst->UFTCTL &= ~BIT(NPCX_UFTCTL_NXMIP_EN);
		k_spin_unlock(&data->lock, key);
#ifdef CONFIG_PM
		uart_npcx_pm_policy_state_lock_put(data, UART_PM_POLICY_STATE_TX_FLAG);
#endif
#ifdef CONFIG_UART_ASYNC_API
		if (data->async.tx_in_progress) {
			data->async.tx_in_progress = false;
			async_evt_tx_done(dev);
			LOG_DBG("Tx wait-empty done");
		}
#endif
	}
#endif
}
#endif

/* UART api functions */
static int uart_npcx_err_check(const struct device *dev)
{
	const struct uart_npcx_config *const config = dev->config;
	struct uart_reg *const inst = config->inst;
	uint32_t err = 0U;
	uint8_t stat = inst->USTAT;

	if (IS_BIT_SET(stat, NPCX_USTAT_DOE)) {
		err |= UART_ERROR_OVERRUN;
	}

	if (IS_BIT_SET(stat, NPCX_USTAT_PE)) {
		err |= UART_ERROR_PARITY;
	}

	if (IS_BIT_SET(stat, NPCX_USTAT_FE)) {
		err |= UART_ERROR_FRAMING;
	}

	return err;
}

static __unused void uart_npcx_rx_wk_isr(const struct device *dev, struct npcx_wui *wui)
{
	/*
	 * Set pm constraint to prevent the system enter suspend state within
	 * the CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT period.
	 */
	LOG_DBG("-->%s", dev->name);
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
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_npcx_async_callback_set,
	.tx = uart_npcx_async_tx,
	.tx_abort = uart_npcx_async_tx_abort,
	.rx_enable = uart_npcx_async_rx_enable,
	.rx_buf_rsp = uart_npcx_async_rx_buf_rsp,
	.rx_disable = uart_npcx_async_rx_disable,
#endif /* CONFIG_UART_ASYNC_API */
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

#ifdef CONFIG_UART_ASYNC_API
	ret = clock_control_on(clk_dev, (clock_control_subsys_t)&config->mdma_clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on UART MDMA clock fail %d", ret);
		return ret;
	}
#endif

	/*
	 * If apb2's clock is not 15MHz, we need to find the other optimized
	 * values of UPSR and UBAUD for baud rate 115200.
	 */
	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)&config->clk_cfg, &uart_rate);
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
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	/* Enable the UART FIFO mode */
	inst->UMDSL |= BIT(NPCX_UMDSL_FIFO_MD);

	/* Disable all UART tx FIFO interrupts */
	uart_npcx_dis_all_tx_interrupts(dev);

	/* Clear UART rx FIFO */
	uart_npcx_clear_rx_fifo(dev);

	/* Configure UART interrupts */
	config->irq_config_func(dev);
#endif

#ifdef CONFIG_UART_ASYNC_API
	data->async.next_rx_buffer = NULL;
	data->async.next_rx_buffer_len = 0;
	data->async.uart_dev = dev;
	k_work_init_delayable(&data->async.rx_dma_params.timeout_work, uart_npcx_async_rx_timeout);
	k_work_init_delayable(&data->async.tx_dma_params.timeout_work, uart_npcx_async_tx_timeout);
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

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
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

#define NPCX_UART_INIT(i)                                                                          \
	NPCX_UART_IRQ_CONFIG_FUNC_DECL(i);                                                         \
	                                                                                           \
	PINCTRL_DT_INST_DEFINE(i);                                                                 \
	                                                                                           \
	static const struct uart_npcx_config uart_npcx_cfg_##i = {                                 \
		.inst = (struct uart_reg *)DT_INST_REG_ADDR(i),                                    \
		.clk_cfg = NPCX_DT_CLK_CFG_ITEM(i),                                                \
		.uart_rx_wui = NPCX_DT_WUI_ITEM_BY_NAME(i, uart_rx),                               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(i),                                         \
		NPCX_UART_IRQ_CONFIG_FUNC_INIT(i)                                                  \
	                                                                                           \
		IF_ENABLED(CONFIG_UART_ASYNC_API, (                                                \
			.mdma_clk_cfg = NPCX_DT_CLK_CFG_ITEM_BY_IDX(i, 1),                         \
			.mdma_reg_base = (struct mdma_reg *)DT_INST_REG_ADDR_BY_IDX(i, 1),         \
		))                                                                                 \
	};                                                                                         \
	                                                                                           \
	static struct uart_npcx_data uart_npcx_data_##i = {                                        \
		.baud_rate = DT_INST_PROP(i, current_speed),                                       \
	};                                                                                         \
	                                                                                           \
	DEVICE_DT_INST_DEFINE(i, &uart_npcx_init, NULL, &uart_npcx_data_##i, &uart_npcx_cfg_##i,   \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_npcx_driver_api);   \
												   \
	NPCX_UART_IRQ_CONFIG_FUNC(i)

DT_INST_FOREACH_STATUS_OKAY(NPCX_UART_INIT)

#define ENABLE_MIWU_CRIN_IRQ(i)                                                                    \
	npcx_miwu_irq_get_and_clear_pending(&uart_npcx_cfg_##i.uart_rx_wui);                       \
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
