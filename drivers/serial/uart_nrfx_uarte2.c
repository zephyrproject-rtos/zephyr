/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Nordic Semiconductor nRF UARTE
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/serial/uart_async_to_irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/logging/log.h>
#include <nrfx_uarte.h>
#include <helpers/nrfx_gppi.h>
#include <haly/nrfy_uarte.h>
#define LOG_MODULE_NAME uarte
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_UART_LOG_LEVEL);

#define INSTANCE_INT_DRIVEN(periph, prefix, i, _) \
	 IS_ENABLED(CONFIG_UART_##prefix##i##_INTERRUPT_DRIVEN)

#define INSTANCE_ASYNC(periph, prefix, i, _) \
	 IS_ENABLED(CONFIG_UART_##prefix##i##_ASYNC)

#define INSTANCE_POLLING(periph, prefix, id, _) \
	UTIL_AND(CONFIG_HAS_HW_NRF_UARTE##prefix##id, \
	  UTIL_AND(COND_CODE_1(CONFIG_UART_##prefix##id##_INTERRUPT_DRIVEN, (0), (1)), \
		   COND_CODE_1(CONFIG_UART_##prefix##id##_ASYNC, (0), (1))))

#define INSTANCE_ENHANCED_POLL_OUT(periph, prefix, i, _) \
	 IS_ENABLED(CONFIG_UART_##prefix##i##_ENHANCED_POLL_OUT)

/* Macro determining if any instance is using interrupt driven API. */
#if (NRFX_FOREACH_ENABLED(UARTE, INSTANCE_INT_DRIVEN, (+), (0), _))
#define UARTE_ANY_INTERRUPT_DRIVEN 1
#else
#define UARTE_ANY_INTERRUPT_DRIVEN 0
#endif

/* Macro determining if any instance is enabled and using ASYNC API. */
#if (NRFX_FOREACH_ENABLED(UARTE, INSTANCE_ASYNC, (+), (0), _))
#define UARTE_ANY_ASYNC 1
#else
#define UARTE_ANY_ASYNC 0
#endif

/* Macro determining if any instance is using only polling API. */
#if (NRFX_FOREACH_ENABLED(UARTE, INSTANCE_POLLING, (+), (0), _))
#define UARTE_ANY_POLLING 1
#else
#define UARTE_ANY_POLLING 0
#endif

/* Macro determining if any instance is using interrupt driven API. */
#if (NRFX_FOREACH_ENABLED(UARTE, INSTANCE_ENHANCED_POLL_OUT, (+), (0), _))
#define UARTE_ENHANCED_POLL_OUT 1
#else
#define UARTE_ENHANCED_POLL_OUT 0
#endif

#if UARTE_ANY_INTERRUPT_DRIVEN || UARTE_ANY_ASYNC
#define UARTE_INT_ASYNC 1
#else
#define UARTE_INT_ASYNC 0
#endif

#if defined(UARTE_CONFIG_PARITYTYPE_Msk)
#define UARTE_ODD_PARITY_ALLOWED 1
#else
#define UARTE_ODD_PARITY_ALLOWED 0
#endif

/*
 * RX timeout is divided into time slabs, this define tells how many divisions
 * should be made. More divisions - higher timeout accuracy and processor usage.
 */
#define RX_TIMEOUT_DIV 5

/* Macro for converting numerical baudrate to register value. It is convenient
 * to use this approach because for constant input it can calculate nrf setting
 * at compile time.
 */
#define NRF_BAUDRATE(baudrate) ((baudrate) == 300 ? 0x00014000 :\
	(baudrate) == 600    ? 0x00027000 :			\
	(baudrate) == 1200   ? NRF_UARTE_BAUDRATE_1200 :	\
	(baudrate) == 2400   ? NRF_UARTE_BAUDRATE_2400 :	\
	(baudrate) == 4800   ? NRF_UARTE_BAUDRATE_4800 :	\
	(baudrate) == 9600   ? NRF_UARTE_BAUDRATE_9600 :	\
	(baudrate) == 14400  ? NRF_UARTE_BAUDRATE_14400 :	\
	(baudrate) == 19200  ? NRF_UARTE_BAUDRATE_19200 :	\
	(baudrate) == 28800  ? NRF_UARTE_BAUDRATE_28800 :	\
	(baudrate) == 31250  ? NRF_UARTE_BAUDRATE_31250 :	\
	(baudrate) == 38400  ? NRF_UARTE_BAUDRATE_38400 :	\
	(baudrate) == 56000  ? NRF_UARTE_BAUDRATE_56000 :	\
	(baudrate) == 57600  ? NRF_UARTE_BAUDRATE_57600 :	\
	(baudrate) == 76800  ? NRF_UARTE_BAUDRATE_76800 :	\
	(baudrate) == 115200 ? NRF_UARTE_BAUDRATE_115200 :	\
	(baudrate) == 230400 ? NRF_UARTE_BAUDRATE_230400 :	\
	(baudrate) == 250000 ? NRF_UARTE_BAUDRATE_250000 :	\
	(baudrate) == 460800 ? NRF_UARTE_BAUDRATE_460800 :	\
	(baudrate) == 921600 ? NRF_UARTE_BAUDRATE_921600 :	\
	(baudrate) == 1000000 ? NRF_UARTE_BAUDRATE_1000000 : 0)

#define UARTE_DATA_FLAG_TRAMPOLINE	BIT(0)
#define UARTE_DATA_FLAG_RX_ENABLED	BIT(1)

struct uarte_async_data {
	uart_callback_t user_callback;
	void *user_data;

	uint8_t *en_rx_buf;
	size_t en_rx_len;

	struct k_timer tx_timer;
	struct k_timer rx_timer;

	k_timeout_t rx_timeout;

	/* Keeps the most recent error mask. */
	uint32_t err;

	uint8_t idle_cnt;
};

/* Device data structure */
struct uarte_nrfx_data {
	struct uart_async_to_irq_data *a2i_data;
#if CONFIG_UART_USE_RUNTIME_CONFIGURE
	struct uart_config uart_config;
#endif
	struct uarte_async_data *async;
	atomic_t flags;
	uint8_t  rx_byte;
};
BUILD_ASSERT(offsetof(struct uarte_nrfx_data, a2i_data) == 0);

/* If set then pins are managed when going to low power mode. */
#define UARTE_CFG_FLAG_GPIO_MGMT		BIT(0)

/* If set then receiver is not used. */
#define UARTE_CFG_FLAG_NO_RX			BIT(1)

/* If set then instance is using interrupt driven API. */
#define UARTE_CFG_FLAG_INTERRUPT_DRIVEN_API	BIT(2)

/**
 * @brief Structure for UARTE configuration.
 */
struct uarte_nrfx_config {
	const struct uart_async_to_irq_config *a2i_config;
	nrfx_uarte_t nrfx_dev;
	nrfx_uarte_config_t nrfx_config;
	const struct pinctrl_dev_config *pcfg;
	uint32_t flags;

	LOG_INSTANCE_PTR_DECLARE(log);
};
BUILD_ASSERT(offsetof(struct uarte_nrfx_config, a2i_config) == 0);

#define UARTE_ERROR_FROM_MASK(mask)					\
	((mask) & NRF_UARTE_ERROR_OVERRUN_MASK ? UART_ERROR_OVERRUN	\
	 : (mask) & NRF_UARTE_ERROR_PARITY_MASK ? UART_ERROR_PARITY	\
	 : (mask) & NRF_UARTE_ERROR_FRAMING_MASK ? UART_ERROR_FRAMING	\
	 : (mask) & NRF_UARTE_ERROR_BREAK_MASK ? UART_BREAK		\
	 : 0)

/* Determine if the device has interrupt driven API enabled. */
#define IS_INT_DRIVEN_API(dev)						\
	(UARTE_ANY_INTERRUPT_DRIVEN &&					\
	 (((const struct uarte_nrfx_config *)dev->config)->flags &	\
	  UARTE_CFG_FLAG_INTERRUPT_DRIVEN_API))

/* Determine if the device supports only polling API. */
#define IS_POLLING_API(dev) \
	(!UARTE_INT_ASYNC || (((struct uarte_nrfx_data *)dev->data)->async == NULL))

/* Determine if the device supports asynchronous API. */
#define IS_ASYNC_API(dev) (!IS_INT_DRIVEN_API(dev) && !IS_POLLING_API(dev))

static inline const nrfx_uarte_t *get_nrfx_dev(const struct device *dev)
{
	const struct uarte_nrfx_config *config = dev->config;

	return &config->nrfx_dev;
}

static int callback_set(const struct device *dev, uart_callback_t callback, void *user_data)
{
	struct uarte_nrfx_data *data = dev->data;

	data->async->user_callback = callback;
	data->async->user_data = user_data;

	return 0;
}

#if UARTE_ANY_ASYNC
static int api_callback_set(const struct device *dev, uart_callback_t callback, void *user_data)
{
	if (!IS_ASYNC_API(dev)) {
		return -ENOTSUP;
	}

	return callback_set(dev, callback, user_data);
}
#endif

static void on_tx_done(const struct device *dev, const nrfx_uarte_event_t *event)
{
	struct uarte_nrfx_data *data = dev->data;
	struct uart_event evt = {
		.type = (event->data.tx.flags & NRFX_UARTE_TX_DONE_ABORTED) ?
			UART_TX_ABORTED : UART_TX_DONE,
		.data.tx.buf = event->data.tx.p_buffer,
		.data.tx.len = event->data.tx.length
	};
	bool hwfc;

#if CONFIG_UART_USE_RUNTIME_CONFIGURE
	hwfc = data->uart_config.flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS;
#else
	const struct uarte_nrfx_config *config = dev->config;

	hwfc = config->nrfx_config.config.hwfc == NRF_UARTE_HWFC_ENABLED;
#endif

	if (hwfc) {
		k_timer_stop(&data->async->tx_timer);
	}
	data->async->user_callback(dev, &evt, data->async->user_data);
}

static void on_rx_done(const struct device *dev, const nrfx_uarte_event_t *event)
{
	struct uarte_nrfx_data *data = dev->data;
	struct uart_event evt;

	if (data->async->err) {
		evt.type = UART_RX_STOPPED;
		evt.data.rx_stop.reason = UARTE_ERROR_FROM_MASK(data->async->err);
		evt.data.rx_stop.data.buf = event->data.rx.p_buffer;
		evt.data.rx_stop.data.len = event->data.rx.length;
		/* Keep error code for uart_err_check(). */
		if (!IS_INT_DRIVEN_API(dev)) {
			data->async->err = 0;
		}
		data->async->user_callback(dev, &evt, data->async->user_data);
	} else if (event->data.rx.length) {
		evt.type = UART_RX_RDY,
		evt.data.rx.buf = event->data.rx.p_buffer,
		evt.data.rx.len = event->data.rx.length,
		evt.data.rx.offset = 0;
		data->async->user_callback(dev, &evt, data->async->user_data);
	}

	evt.type = UART_RX_BUF_RELEASED;
	evt.data.rx_buf.buf = event->data.rx.p_buffer;

	data->async->user_callback(dev, &evt, data->async->user_data);
}

static void start_rx_timer(struct uarte_nrfx_data *data)
{
	struct uarte_async_data *adata = data->async;

	k_timer_start(&adata->rx_timer, adata->rx_timeout, K_NO_WAIT);
}

static void on_rx_byte(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;
	struct uarte_async_data *adata = data->async;
	const nrfx_uarte_t *nrfx_dev = get_nrfx_dev(dev);

	nrfx_uarte_rxdrdy_disable(nrfx_dev);
	adata->idle_cnt = RX_TIMEOUT_DIV;
	start_rx_timer(data);
}

static void on_rx_buf_req(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;
	struct uarte_async_data *adata = data->async;
	const nrfx_uarte_t *nrfx_dev = get_nrfx_dev(dev);

	/* If buffer is not null it indicates that event comes from RX enabling
	 * function context. We need to pass provided buffer to the driver.
	 */
	if (adata->en_rx_buf) {
		uint8_t *buf = adata->en_rx_buf;
		size_t len = adata->en_rx_len;
		nrfx_err_t err;

		adata->en_rx_buf = NULL;
		adata->en_rx_len = 0;

		err = nrfx_uarte_rx_buffer_set(nrfx_dev, buf, len);
		__ASSERT_NO_MSG(err == NRFX_SUCCESS);
		return;
	}

	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST
	};

	/* If counter reached zero that indicates that timeout was reached and
	 * reception of one buffer was terminated to restart another transfer.
	 */
	if (!K_TIMEOUT_EQ(adata->rx_timeout, K_NO_WAIT)) {
		nrfx_uarte_rxdrdy_enable(nrfx_dev);
	}
	data->async->user_callback(dev, &evt, data->async->user_data);
}

static void on_rx_disabled(const struct device *dev, struct uarte_nrfx_data *data)
{
	struct uart_event evt = {
		.type = UART_RX_DISABLED
	};

	atomic_and(&data->flags, ~UARTE_DATA_FLAG_RX_ENABLED);
	k_timer_stop(&data->async->rx_timer);

	data->async->user_callback(dev, &evt, data->async->user_data);
}

static void trigger_handler(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;

	if (UARTE_ANY_INTERRUPT_DRIVEN &&
	    atomic_and(&data->flags, ~UARTE_DATA_FLAG_TRAMPOLINE) &
	    UARTE_DATA_FLAG_TRAMPOLINE) {
		uart_async_to_irq_trampoline_cb(dev);
	}
}

static void evt_handler(nrfx_uarte_event_t const *event, void *context)
{
	const struct device *dev = context;
	struct uarte_nrfx_data *data = dev->data;

	switch (event->type) {
	case NRFX_UARTE_EVT_TX_DONE:
		on_tx_done(dev, event);
		break;
	case NRFX_UARTE_EVT_RX_DONE:
		on_rx_done(dev, event);
		break;
	case NRFX_UARTE_EVT_RX_BYTE:
		on_rx_byte(dev);
		break;
	case NRFX_UARTE_EVT_ERROR:
		data->async->err = event->data.error.error_mask;
		break;
	case NRFX_UARTE_EVT_RX_BUF_REQUEST:
		on_rx_buf_req(dev);
		break;
	case NRFX_UARTE_EVT_RX_DISABLED:
		on_rx_disabled(dev, data);
		break;
	case NRFX_UARTE_EVT_RX_BUF_TOO_LATE:
		/* No support */
		break;
	case NRFX_UARTE_EVT_TRIGGER:
		trigger_handler(dev);
		break;
	default:
		__ASSERT_NO_MSG(0);
	}
}

static int api_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout)
{
	struct uarte_nrfx_data *data = dev->data;
	const nrfx_uarte_t *nrfx_dev = get_nrfx_dev(dev);
	nrfx_err_t err;
	bool hwfc;

#if CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		return -ECANCELED;
	}
#endif

#if CONFIG_UART_USE_RUNTIME_CONFIGURE
	hwfc = data->uart_config.flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS;
#else
	const struct uarte_nrfx_config *config = dev->config;

	hwfc = config->nrfx_config.config.hwfc == NRF_UARTE_HWFC_ENABLED;
#endif

	err = nrfx_uarte_tx(nrfx_dev, buf, len, 0);
	if (err != NRFX_SUCCESS) {
		return (err == NRFX_ERROR_BUSY) ? -EBUSY : -EIO;
	}

	if (hwfc && timeout != SYS_FOREVER_US) {
		k_timer_start(&data->async->tx_timer, K_USEC(timeout), K_NO_WAIT);
	}

	return 0;
}

static int api_tx_abort(const struct device *dev)
{
	const nrfx_uarte_t *nrfx_dev = get_nrfx_dev(dev);
	nrfx_err_t err;

	err = nrfx_uarte_tx_abort(nrfx_dev, false);
	return (err == NRFX_SUCCESS) ? 0 : -EFAULT;
}

static void tx_timeout_handler(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);

	(void)api_tx_abort(dev);
}

static void rx_timeout_handler(struct k_timer *timer)
{
	const struct device *dev = (const struct device *)k_timer_user_data_get(timer);
	struct uarte_nrfx_data *data = dev->data;
	struct uarte_async_data *adata = data->async;
	const nrfx_uarte_t *nrfx_dev = get_nrfx_dev(dev);

	if (nrfx_uarte_rx_new_data_check(nrfx_dev)) {
		adata->idle_cnt = RX_TIMEOUT_DIV - 1;
	} else {
		adata->idle_cnt--;
		if (adata->idle_cnt == 0) {
			(void)nrfx_uarte_rx_abort(nrfx_dev, false, false);
			return;
		}
	}

	start_rx_timer(data);
}

/* Determine if RX FIFO content shall be kept when device is being disabled.
 * When flow-control is used then we expect to keep RX FIFO content since HWFC
 * enforces lossless communication. However, when HWFC is not used (by any instance
 * then RX FIFO handling feature is disabled in the nrfx_uarte to save space.
 * It is based on assumption that without HWFC it is expected that some data may
 * be lost and there are means to prevent that (keeping receiver always opened by
 * provided reception buffers on time).
 */
static inline uint32_t get_keep_fifo_content_flag(const struct device *dev)
{
#if CONFIG_UART_USE_RUNTIME_CONFIGURE
	struct uarte_nrfx_data *data = dev->data;

	if (data->uart_config.flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		return NRFX_UARTE_RX_ENABLE_KEEP_FIFO_CONTENT;
	}
#else
	const struct uarte_nrfx_config *config = dev->config;

	if (config->nrfx_config.config.hwfc == NRF_UARTE_HWFC_ENABLED) {
		return NRFX_UARTE_RX_ENABLE_KEEP_FIFO_CONTENT;
	}
#endif

	return 0;
}

static int api_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	nrfx_err_t err;
	const nrfx_uarte_t *nrfx_dev = get_nrfx_dev(dev);
	const struct uarte_nrfx_config *cfg = dev->config;
	struct uarte_nrfx_data *data = dev->data;
	struct uarte_async_data *adata = data->async;
	uint32_t flags = NRFX_UARTE_RX_ENABLE_CONT |
			 get_keep_fifo_content_flag(dev) |
			 (IS_ASYNC_API(dev) ? NRFX_UARTE_RX_ENABLE_STOP_ON_END : 0);

	if (cfg->flags & UARTE_CFG_FLAG_NO_RX) {
		return -ENOTSUP;
	}

	if (timeout != SYS_FOREVER_US) {
		adata->idle_cnt = RX_TIMEOUT_DIV + 1;
		adata->rx_timeout = K_USEC(timeout / RX_TIMEOUT_DIV);
		nrfx_uarte_rxdrdy_enable(nrfx_dev);
	} else {
		adata->rx_timeout = K_NO_WAIT;
	}

	/* Store the buffer. It will be passed to the driver in the event handler.
	 * We do that instead of calling nrfx_uarte_rx_buffer_set here to ensure
	 * that nrfx_uarte_rx_buffer_set is called when RX enable configuration
	 * flags are already known to the driver (e.g. if flushed data shall be
	 * kept or not).
	 */
	adata->err = 0;
	adata->en_rx_buf = buf;
	adata->en_rx_len = len;

	atomic_or(&data->flags, UARTE_DATA_FLAG_RX_ENABLED);

	err = nrfx_uarte_rx_enable(nrfx_dev, flags);
	if (err != NRFX_SUCCESS) {
		atomic_and(&data->flags, ~UARTE_DATA_FLAG_RX_ENABLED);
		return (err == NRFX_ERROR_BUSY) ? -EBUSY : -EIO;
	}

	return 0;
}

static int api_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	const nrfx_uarte_t *nrfx_dev = get_nrfx_dev(dev);
	struct uarte_nrfx_data *data = dev->data;
	nrfx_err_t err;

	if (!(data->flags & UARTE_DATA_FLAG_RX_ENABLED)) {
		return -EACCES;
	}

	err = nrfx_uarte_rx_buffer_set(nrfx_dev, buf, len);
	switch (err) {
	case NRFX_SUCCESS:
		return 0;
	case NRFX_ERROR_BUSY:
		return -EBUSY;
	default:
		return -EIO;
	}
}

static int api_rx_disable(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;

	k_timer_stop(&data->async->rx_timer);

	return (nrfx_uarte_rx_abort(get_nrfx_dev(dev), true, false) == NRFX_SUCCESS) ? 0 : -EFAULT;
}

static int api_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uarte_nrfx_config *cfg = dev->config;
	const nrfx_uarte_t *instance = &cfg->nrfx_dev;
	nrfx_err_t err;

	if (IS_INT_DRIVEN_API(dev)) {
		return uart_fifo_read(dev, c, 1) == 0 ? -1 : 0;
	}

	if (IS_ASYNC_API(dev)) {
		return -EBUSY;
	}

	err = nrfx_uarte_rx_ready(instance, NULL);
	if (err == NRFX_SUCCESS) {
		uint8_t *rx_byte = cfg->nrfx_config.rx_cache.p_buffer;

		*c = *rx_byte;
		err = nrfx_uarte_rx_buffer_set(instance, rx_byte, 1);
		__ASSERT_NO_MSG(err == NRFX_SUCCESS);

		return 0;
	}

	return -1;
}

static void api_poll_out(const struct device *dev, unsigned char out_char)
{
	const nrfx_uarte_t *nrfx_dev = get_nrfx_dev(dev);
	nrfx_err_t err;

#if CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		return;
	}
#endif

	do {
		/* When runtime PM is used we cannot use early return because then
		 * we have no information when UART is actually done with the
		 * transmission. It reduces UART performance however, polling in
		 * general is not power efficient and should be avoided in low
		 * power applications.
		 */
		err = nrfx_uarte_tx(nrfx_dev, &out_char, 1, NRFX_UARTE_TX_EARLY_RETURN);
		__ASSERT(err != NRFX_ERROR_INVALID_ADDR, "Invalid address of the buffer");

		if (err == NRFX_ERROR_BUSY) {
			if (IS_ENABLED(CONFIG_MULTITHREADING) && k_is_preempt_thread()) {
				k_msleep(1);
			} else {
				Z_SPIN_DELAY(3);
			}
		}
	} while (err == NRFX_ERROR_BUSY);
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
/**
 * @brief Set the baud rate
 *
 * This routine set the given baud rate for the UARTE.
 *
 * @param dev UARTE device struct
 * @param baudrate Baud rate
 *
 * @return 0 on success or error code
 */
static int baudrate_set(NRF_UARTE_Type *uarte, uint32_t baudrate)
{
	nrf_uarte_baudrate_t nrf_baudrate = NRF_BAUDRATE(baudrate);

	if (baudrate == 0) {
		return -EINVAL;
	}

	nrfy_uarte_baudrate_set(uarte, nrf_baudrate);

	return 0;
}

static int uarte_nrfx_configure(const struct device *dev,
				const struct uart_config *cfg)
{
	const nrfx_uarte_t *nrfx_dev = get_nrfx_dev(dev);
	struct uarte_nrfx_data *data = dev->data;
	nrf_uarte_config_t uarte_cfg;

#if NRF_UARTE_HAS_FRAME_TIMEOUT
	uarte_cfg.frame_timeout = NRF_UARTE_FRAME_TIMEOUT_DIS;
#endif

#if defined(UARTE_CONFIG_STOP_Msk)
	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		uarte_cfg.stop = NRF_UARTE_STOP_ONE;
		break;
	case UART_CFG_STOP_BITS_2:
		uarte_cfg.stop = NRF_UARTE_STOP_TWO;
		break;
	default:
		return -ENOTSUP;
	}
#else
	if (cfg->stop_bits != UART_CFG_STOP_BITS_1) {
		return -ENOTSUP;
	}
#endif

	if (cfg->data_bits != UART_CFG_DATA_BITS_8) {
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		uarte_cfg.hwfc = NRF_UARTE_HWFC_DISABLED;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		uarte_cfg.hwfc = NRF_UARTE_HWFC_ENABLED;
		break;
	default:
		return -ENOTSUP;
	}

#if defined(UARTE_CONFIG_PARITYTYPE_Msk)
	uarte_cfg.paritytype = NRF_UARTE_PARITYTYPE_EVEN;
#endif
	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		uarte_cfg.parity = NRF_UARTE_PARITY_EXCLUDED;
		break;
	case UART_CFG_PARITY_EVEN:
		uarte_cfg.parity = NRF_UARTE_PARITY_INCLUDED;
		break;
#if defined(UARTE_CONFIG_PARITYTYPE_Msk)
	case UART_CFG_PARITY_ODD:
		uarte_cfg.parity = NRF_UARTE_PARITY_INCLUDED;
		uarte_cfg.paritytype = NRF_UARTE_PARITYTYPE_ODD;
		break;
#endif
	default:
		return -ENOTSUP;
	}

	if (baudrate_set(nrfx_dev->p_reg, cfg->baudrate) != 0) {
		return -ENOTSUP;
	}

	nrfy_uarte_configure(nrfx_dev->p_reg, &uarte_cfg);

	data->uart_config = *cfg;

	return 0;
}

static int uarte_nrfx_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	struct uarte_nrfx_data *data = dev->data;

	*cfg = data->uart_config;
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#if UARTE_ANY_POLLING || UARTE_ANY_INTERRUPT_DRIVEN
static int api_err_check(const struct device *dev)
{
	if (IS_POLLING_API(dev)) {
		const struct uarte_nrfx_config *cfg = dev->config;
		const nrfx_uarte_t *instance = &cfg->nrfx_dev;
		uint32_t mask = nrfx_uarte_errorsrc_get(instance);

		return mask;
	}

	struct uarte_nrfx_data *data = dev->data;
	uint32_t rv = data->async->err;

	data->async->err = 0;

	return rv;
}
#endif

static const struct uart_async_to_irq_async_api a2i_api = {
	.callback_set		= callback_set,
	.tx			= api_tx,
	.tx_abort		= api_tx_abort,
	.rx_enable		= api_rx_enable,
	.rx_buf_rsp		= api_rx_buf_rsp,
	.rx_disable		= api_rx_disable,
};

static const struct uart_driver_api uart_nrfx_uarte_driver_api = {
	.poll_in	= api_poll_in,
	.poll_out	= api_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure	= uarte_nrfx_configure,
	.config_get	= uarte_nrfx_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#if UARTE_ANY_POLLING || UARTE_ANY_INTERRUPT_DRIVEN
	.err_check	= api_err_check,
#endif
#if UARTE_ANY_ASYNC
	.callback_set	= api_callback_set,
	.tx		= api_tx,
	.tx_abort	= api_tx_abort,
	.rx_enable	= api_rx_enable,
	.rx_buf_rsp	= api_rx_buf_rsp,
	.rx_disable	= api_rx_disable,
#endif /* UARTE_ANY_ASYNC */
#if UARTE_ANY_INTERRUPT_DRIVEN
	UART_ASYNC_TO_IRQ_API_INIT(),
#endif /* UARTE_ANY_INTERRUPT_DRIVEN */
};

static int endtx_stoptx_ppi_init(NRF_UARTE_Type *uarte)
{
	nrfx_err_t ret;
	uint8_t ch;

	ret = nrfx_gppi_channel_alloc(&ch);
	if (ret != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate PPI Channel");
		return -EIO;
	}

	nrfx_gppi_channel_endpoints_setup(ch,
		nrfy_uarte_event_address_get(uarte, NRF_UARTE_EVENT_ENDTX),
		nrfy_uarte_task_address_get(uarte, NRF_UARTE_TASK_STOPTX));
	nrfx_gppi_channels_enable(BIT(ch));

	return 0;
}

static int start_rx(const struct device *dev)
{
	const struct uarte_nrfx_config *cfg = dev->config;

	if (IS_INT_DRIVEN_API(dev)) {
		return uart_async_to_irq_rx_enable(dev);
	}

	__ASSERT_NO_MSG(IS_POLLING_API(dev));

	nrfx_err_t err;
	const nrfx_uarte_t *instance = &cfg->nrfx_dev;
	uint8_t *rx_byte = cfg->nrfx_config.rx_cache.p_buffer;

	err = nrfx_uarte_rx_buffer_set(instance, rx_byte, 1);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);

	err = nrfx_uarte_rx_enable(instance, 0);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS || err == NRFX_ERROR_BUSY);

	(void)err;

	return 0;
}

static void async_to_irq_trampoline(const struct device *dev)
{
	const struct uarte_nrfx_config *cfg = dev->config;
	struct uarte_nrfx_data *data = dev->data;
	uint32_t prev = atomic_or(&data->flags, UARTE_DATA_FLAG_TRAMPOLINE);

	if (!(prev & UARTE_DATA_FLAG_TRAMPOLINE)) {
		nrfx_uarte_int_trigger(&cfg->nrfx_dev);
	}
}

static int uarte_nrfx_init(const struct device *dev)
{
	int err;
	nrfx_err_t nerr;
	const nrfx_uarte_t *nrfx_dev = get_nrfx_dev(dev);
	const struct uarte_nrfx_config *cfg = dev->config;
	struct uarte_nrfx_data *data = dev->data;

#ifdef CONFIG_ARCH_POSIX
	/* For simulation the DT provided peripheral address needs to be corrected */
	((struct pinctrl_dev_config *)cfg->pcfg)->reg = (uintptr_t)nrfx_dev->p_reg;
#endif

	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	if (UARTE_ENHANCED_POLL_OUT && cfg->nrfx_config.tx_stop_on_end) {
		err = endtx_stoptx_ppi_init(nrfx_dev->p_reg);
		if (err < 0) {
			return err;
		}
	}

	if (UARTE_ANY_INTERRUPT_DRIVEN) {
		if (cfg->a2i_config) {
			err = uart_async_to_irq_init(data->a2i_data, cfg->a2i_config);
			if (err < 0) {
				return err;
			}
		}
	}

	if (IS_ENABLED(UARTE_INT_ASYNC) && data->async) {
		k_timer_init(&data->async->rx_timer, rx_timeout_handler, NULL);
		k_timer_user_data_set(&data->async->rx_timer, (void *)dev);
		k_timer_init(&data->async->tx_timer, tx_timeout_handler, NULL);
		k_timer_user_data_set(&data->async->tx_timer, (void *)dev);
	}

	nerr = nrfx_uarte_init(nrfx_dev, &cfg->nrfx_config,
			IS_ENABLED(UARTE_INT_ASYNC) ?
				(IS_POLLING_API(dev) ? NULL : evt_handler) : NULL);
	if (nerr == NRFX_SUCCESS && !IS_ASYNC_API(dev) && !(cfg->flags & UARTE_CFG_FLAG_NO_RX)) {
		err = start_rx(dev);
	}

	switch (nerr) {
	case NRFX_ERROR_INVALID_STATE:
		return -EBUSY;
	case NRFX_ERROR_BUSY:
		return -EACCES;
	case NRFX_ERROR_INVALID_PARAM:
		return -EINVAL;
	default:
		return 0;
	}
}

#ifdef CONFIG_PM_DEVICE
static int stop_rx(const struct device *dev)
{
	const struct uarte_nrfx_config *cfg = dev->config;

	if (IS_INT_DRIVEN_API(dev)) {
		return uart_async_to_irq_rx_disable(dev);
	}

	__ASSERT_NO_MSG(IS_POLLING_API(dev));
	nrfx_err_t err;
	const nrfx_uarte_t *instance = &cfg->nrfx_dev;

	err = nrfx_uarte_rx_abort(instance, true, true);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);

	return 0;
}

static int uarte_nrfx_pm_action(const struct device *dev,
				enum pm_device_action action)
{
	const struct uarte_nrfx_config *cfg = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (cfg->flags & UARTE_CFG_FLAG_GPIO_MGMT) {
			ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
			if (ret < 0) {
				return ret;
			}
		}
		if (!IS_ASYNC_API(dev) && !(cfg->flags & UARTE_CFG_FLAG_NO_RX)) {
			return start_rx(dev);
		}

		break;
	case PM_DEVICE_ACTION_SUSPEND:
		if (!IS_ASYNC_API(dev) && !(cfg->flags & UARTE_CFG_FLAG_NO_RX)) {
			stop_rx(dev);
		}

		if (cfg->flags & UARTE_CFG_FLAG_GPIO_MGMT) {
			ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_SLEEP);
			if (ret < 0) {
				return ret;
			}
		}

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#if defined(UARTE_CONFIG_STOP_Msk)
#define UARTE_HAS_STOP_CONFIG 1
#endif

#define UARTE(idx)			DT_NODELABEL(uart##idx)
#define UARTE_HAS_PROP(idx, prop)	DT_NODE_HAS_PROP(UARTE(idx), prop)
#define UARTE_PROP(idx, prop)		DT_PROP(UARTE(idx), prop)

/* Macro returning initial log level. Logs are off for UART used for console. */
#define GET_INIT_LOG_LEVEL(idx)					  \
	COND_CODE_1(DT_HAS_CHOSEN(zephyr_console),		  \
		(DT_SAME_NODE(UARTE(idx),			  \
			      DT_CHOSEN(zephyr_console)) ?	  \
			 LOG_LEVEL_NONE : CONFIG_UART_LOG_LEVEL), \
			(CONFIG_UART_LOG_LEVEL))

/* Macro puts buffers in dedicated section if device tree property is set. */
#define UARTE_MEMORY_SECTION(idx)					       \
	COND_CODE_1(UARTE_HAS_PROP(idx, memory_regions),		       \
		(__attribute__((__section__(LINKER_DT_NODE_REGION_NAME(	       \
			DT_PHANDLE(UARTE(idx), memory_regions)))))),	       \
		())

#define UART_NRF_UARTE_DEVICE(idx) \
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, idx, GET_INIT_LOG_LEVEL(idx));			\
	static uint8_t uarte##idx##_tx_cache[CONFIG_UART_##idx##_TX_CACHE_SIZE]			\
			UARTE_MEMORY_SECTION(idx) __aligned(4);					\
	static uint8_t uarte##idx##_rx_cache[CONFIG_UART_##idx##_RX_CACHE_SIZE]			\
			UARTE_MEMORY_SECTION(idx) __aligned(4);					\
	static nrfx_uarte_rx_cache_t uarte##idx##_rx_cache_scratch;				\
	IF_ENABLED(CONFIG_UART_##idx##_INTERRUPT_DRIVEN,					\
		(static uint8_t a2i_rx_buf##idx[CONFIG_UART_##idx##_A2I_RX_SIZE]		\
			UARTE_MEMORY_SECTION(idx) __aligned(4);))				\
	PINCTRL_DT_DEFINE(UARTE(idx));								\
	static const struct uart_async_to_irq_config uarte_a2i_config_##idx =			\
		UART_ASYNC_TO_IRQ_API_CONFIG_INITIALIZER(&a2i_api,				\
					  async_to_irq_trampoline,				\
					  UARTE_PROP(idx, current_speed),			\
					  uarte##idx##_tx_cache,				\
					  /* nrfx_uarte driver is using the last byte in the */	\
					  /* cache buffer for keeping a byte that is currently*/\
					  /* polled out so it cannot be used as a cache buffer*/\
					  /* by the adaptation layer. */			\
					  sizeof(uarte##idx##_tx_cache) - 1,			\
					  COND_CODE_1(CONFIG_UART_##idx##_INTERRUPT_DRIVEN,	\
						  (a2i_rx_buf##idx), (NULL)),			\
					  COND_CODE_1(CONFIG_UART_##idx##_INTERRUPT_DRIVEN,	\
						  (sizeof(a2i_rx_buf##idx)), (0)),		\
					  CONFIG_UART_##idx##_A2I_RX_BUF_COUNT,			\
					  LOG_INSTANCE_PTR(LOG_MODULE_NAME, idx));		\
	static const struct uarte_nrfx_config uarte_config_##idx = {				\
		.a2i_config = IS_ENABLED(CONFIG_UART_##idx## _INTERRUPT_DRIVEN) ?		\
			&uarte_a2i_config_##idx : NULL,						\
		.nrfx_dev = NRFX_UARTE_INSTANCE(idx),						\
		.nrfx_config = {								\
			.p_context = (void *)DEVICE_DT_GET(UARTE(idx)),				\
			.tx_cache = {								\
				.p_buffer = uarte##idx##_tx_cache,				\
				.length = CONFIG_UART_##idx##_TX_CACHE_SIZE			\
			},									\
			.rx_cache = {								\
				.p_buffer = uarte##idx##_rx_cache,				\
				.length = CONFIG_UART_##idx##_RX_CACHE_SIZE			\
			},									\
			.p_rx_cache_scratch = &uarte##idx##_rx_cache_scratch,			\
			.baudrate = NRF_BAUDRATE(UARTE_PROP(idx, current_speed)),		\
			.interrupt_priority = DT_IRQ(UARTE(idx), priority),			\
			.config = {								\
				.hwfc = (UARTE_PROP(idx, hw_flow_control) ==			\
					UART_CFG_FLOW_CTRL_RTS_CTS) ?				\
					NRF_UARTE_HWFC_ENABLED : NRF_UARTE_HWFC_DISABLED,	\
				.parity = IS_ENABLED(CONFIG_UART_##idx##_NRF_PARITY_BIT) ?	\
					NRF_UARTE_PARITY_INCLUDED : NRF_UARTE_PARITY_EXCLUDED,	\
				IF_ENABLED(UARTE_HAS_STOP_CONFIG, (.stop = NRF_UARTE_STOP_ONE,))\
				IF_ENABLED(UARTE_ODD_PARITY_ALLOWED,				\
					(.paritytype = NRF_UARTE_PARITYTYPE_EVEN,))		\
			},									\
			.tx_stop_on_end = IS_ENABLED(CONFIG_UART_##idx##_ENHANCED_POLL_OUT),	\
			.skip_psel_cfg = true,							\
			.skip_gpio_cfg = true,							\
		},										\
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(UARTE(idx)),					\
		.flags = (UARTE_PROP(idx, disable_rx) ? UARTE_CFG_FLAG_NO_RX : 0) |		\
			(IS_ENABLED(CONFIG_UART_##idx##_GPIO_MANAGEMENT) ?			\
				UARTE_CFG_FLAG_GPIO_MGMT : 0) |					\
			(IS_ENABLED(CONFIG_UART_##idx##_INTERRUPT_DRIVEN) ?			\
				UARTE_CFG_FLAG_INTERRUPT_DRIVEN_API : 0),			\
		LOG_INSTANCE_PTR_INIT(log, LOG_MODULE_NAME, idx)				\
	};											\
	static struct uart_async_to_irq_data uarte_a2i_data_##idx;				\
	static struct uarte_async_data uarte_async_##idx;					\
	static struct uarte_nrfx_data uarte_data_##idx = {					\
		.a2i_data = IS_ENABLED(CONFIG_UART_##idx##_INTERRUPT_DRIVEN) ?			\
			&uarte_a2i_data_##idx : NULL,						\
		IF_ENABLED(CONFIG_UART_USE_RUNTIME_CONFIGURE,					\
			(.uart_config = {							\
			.baudrate = UARTE_PROP(idx, current_speed),                             \
			.parity = IS_ENABLED(CONFIG_UART_##idx##_NRF_PARITY_BIT) ?              \
				  UART_CFG_PARITY_EVEN : UART_CFG_PARITY_NONE,                  \
			.stop_bits = UART_CFG_STOP_BITS_1,                                      \
			.data_bits = UART_CFG_DATA_BITS_8,                                      \
			.flow_ctrl = UARTE_PROP(idx, hw_flow_control) ?                         \
				     UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE,      \
			},))									\
		.async = (IS_ENABLED(CONFIG_UART_##idx##_INTERRUPT_DRIVEN) ||			\
			  IS_ENABLED(CONFIG_UART_##idx##_ASYNC)) ? &uarte_async_##idx : NULL	\
	};											\
	static int uarte_init_##idx(const struct device *dev)					\
	{											\
		COND_CODE_1(INSTANCE_POLLING(_, /*empty*/, idx, _), (),				\
			(									\
			IRQ_CONNECT(DT_IRQN(UARTE(idx)), DT_IRQ(UARTE(idx), priority),		\
				    nrfx_isr, nrfx_uarte_##idx##_irq_handler, 0);		\
			irq_enable(DT_IRQN(UARTE(idx)));					\
			)									\
		)										\
		return uarte_nrfx_init(dev);							\
	}											\
	PM_DEVICE_DT_DEFINE(UARTE(idx), uarte_nrfx_pm_action);					\
	DEVICE_DT_DEFINE(UARTE(idx),								\
		      uarte_init_##idx,								\
		      PM_DEVICE_DT_GET(UARTE(idx)),						\
		      &uarte_data_##idx,							\
		      &uarte_config_##idx,							\
		      PRE_KERNEL_1,								\
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,					\
		      &uart_nrfx_uarte_driver_api)

/* Macro creates device instance if it is enabled in devicetree. */
#define UARTE_DEVICE(periph, prefix, id, _) \
	IF_ENABLED(CONFIG_HAS_HW_NRF_UARTE##prefix##id, (UART_NRF_UARTE_DEVICE(prefix##id);))

/* Macro iterates over nrfx_uarte instances enabled in the nrfx_config.h. */
NRFX_FOREACH_ENABLED(UARTE, UARTE_DEVICE, (), (), _)
