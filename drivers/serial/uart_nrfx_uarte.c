/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Nordic Semiconductor nRF UARTE
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>
#include <hal/nrf_uarte.h>
#include <nrfx_timer.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <helpers/nrfx_gppi.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_nrfx_uarte, CONFIG_UART_LOG_LEVEL);

#include <zephyr/drivers/pinctrl.h>

/* Generalize PPI or DPPI channel management */
#if defined(PPI_PRESENT)
#include <nrfx_ppi.h>
#define gppi_channel_t nrf_ppi_channel_t
#define gppi_channel_alloc nrfx_ppi_channel_alloc
#define gppi_channel_enable nrfx_ppi_channel_enable
#elif defined(DPPI_PRESENT)
#include <nrfx_dppi.h>
#define gppi_channel_t uint8_t
#define gppi_channel_alloc nrfx_dppi_channel_alloc
#define gppi_channel_enable nrfx_dppi_channel_enable
#else
#error "No PPI or DPPI"
#endif

/* Execute macro f(x) for all instances. */
#define UARTE_FOR_EACH_INSTANCE(f, sep, off_code) \
	NRFX_FOREACH_PRESENT(UARTE, f, sep, off_code, _)

/* Determine if any instance is using interrupt driven API. */
#define IS_INT_DRIVEN(unused, prefix, i, _) \
	(IS_ENABLED(CONFIG_HAS_HW_NRF_UARTE##prefix##i) && \
	 IS_ENABLED(CONFIG_UART_##prefix##i##_INTERRUPT_DRIVEN))

#if UARTE_FOR_EACH_INSTANCE(IS_INT_DRIVEN, (||), (0))
	#define UARTE_INTERRUPT_DRIVEN	1
#endif

/* Determine if any instance is not using asynchronous API. */
#define IS_NOT_ASYNC(unused, prefix, i, _) \
	(IS_ENABLED(CONFIG_HAS_HW_NRF_UARTE##prefix##i) && \
	 !IS_ENABLED(CONFIG_UART_##prefix##i##_ASYNC))

#if UARTE_FOR_EACH_INSTANCE(IS_NOT_ASYNC, (||), (0))
#define UARTE_ANY_NONE_ASYNC 1
#endif

/* Determine if any instance is using asynchronous API. */
#define IS_ASYNC(unused, prefix, i, _) \
	(IS_ENABLED(CONFIG_HAS_HW_NRF_UARTE##prefix##i) && \
	 IS_ENABLED(CONFIG_UART_##prefix##i##_ASYNC))

#if UARTE_FOR_EACH_INSTANCE(IS_ASYNC, (||), (0))
#define UARTE_ANY_ASYNC 1
#endif

/* Determine if any instance is using asynchronous API with HW byte counting. */
#define IS_HW_ASYNC(unused, prefix, i, _) IS_ENABLED(CONFIG_UART_##prefix##i##_NRF_HW_ASYNC)

#if UARTE_FOR_EACH_INSTANCE(IS_HW_ASYNC, (||), (0))
#define UARTE_HW_ASYNC 1
#endif

/* Determine if any instance is using enhanced poll_out feature. */
#define IS_ENHANCED_POLL_OUT(unused, prefix, i, _) \
	IS_ENABLED(CONFIG_UART_##prefix##i##_ENHANCED_POLL_OUT)

#if UARTE_FOR_EACH_INSTANCE(IS_ENHANCED_POLL_OUT, (||), (0))
#define UARTE_ENHANCED_POLL_OUT 1
#endif

/*
 * RX timeout is divided into time slabs, this define tells how many divisions
 * should be made. More divisions - higher timeout accuracy and processor usage.
 */
#define RX_TIMEOUT_DIV 5

/* Size of hardware fifo in RX path. */
#define UARTE_HW_RX_FIFO_SIZE 5

#ifdef UARTE_ANY_ASYNC
struct uarte_async_cb {
	uart_callback_t user_callback;
	void *user_data;

	const uint8_t *tx_buf;
	volatile size_t tx_size;
	const uint8_t *xfer_buf;
	size_t xfer_len;

	size_t tx_cache_offset;

	struct k_timer tx_timeout_timer;

	uint8_t *rx_buf;
	size_t rx_buf_len;
	size_t rx_offset;
	uint8_t *rx_next_buf;
	size_t rx_next_buf_len;
	uint32_t rx_total_byte_cnt; /* Total number of bytes received */
	uint32_t rx_total_user_byte_cnt; /* Total number of bytes passed to user */
	int32_t rx_timeout; /* Timeout set by user */
	int32_t rx_timeout_slab; /* rx_timeout divided by RX_TIMEOUT_DIV */
	int32_t rx_timeout_left; /* Current time left until user callback */
	struct k_timer rx_timeout_timer;
	union {
		gppi_channel_t ppi;
		uint32_t cnt;
	} rx_cnt;
	volatile int tx_amount;

	atomic_t low_power_mask;
	uint8_t rx_flush_buffer[UARTE_HW_RX_FIFO_SIZE];
	uint8_t rx_flush_cnt;
	volatile bool rx_enabled;
	volatile bool discard_rx_fifo;
	bool pending_tx;
	/* Flag to ensure that RX timeout won't be executed during ENDRX ISR */
	volatile bool is_in_irq;
};
#endif /* UARTE_ANY_ASYNC */

#ifdef UARTE_INTERRUPT_DRIVEN
struct uarte_nrfx_int_driven {
	uart_irq_callback_user_data_t cb; /**< Callback function pointer */
	void *cb_data; /**< Callback function arg */
	uint8_t *tx_buffer;
	uint16_t tx_buff_size;
	volatile bool disable_tx_irq;
	bool tx_irq_enabled;
#ifdef CONFIG_PM_DEVICE
	bool rx_irq_enabled;
#endif
	atomic_t fifo_fill_lock;
};
#endif

/* Device data structure */
struct uarte_nrfx_data {
	const struct device *dev;
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	struct uart_config uart_config;
#endif
#ifdef UARTE_INTERRUPT_DRIVEN
	struct uarte_nrfx_int_driven *int_driven;
#endif
#ifdef UARTE_ANY_ASYNC
	struct uarte_async_cb *async;
#endif
	atomic_val_t poll_out_lock;
	uint8_t *char_out;
	uint8_t *rx_data;
	gppi_channel_t ppi_ch_endtx;
};

#define UARTE_LOW_POWER_TX BIT(0)
#define UARTE_LOW_POWER_RX BIT(1)

/* If enabled, pins are managed when going to low power mode. */
#define UARTE_CFG_FLAG_GPIO_MGMT   BIT(0)

/* If enabled then ENDTX is PPI'ed to TXSTOP */
#define UARTE_CFG_FLAG_PPI_ENDTX   BIT(1)

/* If enabled then TIMER and PPI is used for byte counting. */
#define UARTE_CFG_FLAG_HW_BYTE_COUNTING   BIT(2)

/* If enabled then UARTE peripheral is disabled when not used. This allows
 * to achieve lowest power consumption in idle.
 */
#define UARTE_CFG_FLAG_LOW_POWER   BIT(4)

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

/**
 * @brief Structure for UARTE configuration.
 */
struct uarte_nrfx_config {
	NRF_UARTE_Type *uarte_regs; /* Instance address */
	uint32_t clock_freq;
	uint32_t flags;
	bool disable_rx;
	const struct pinctrl_dev_config *pcfg;
#ifndef CONFIG_UART_USE_RUNTIME_CONFIGURE
	nrf_uarte_baudrate_t baudrate;
	nrf_uarte_config_t hw_config;
#endif
#ifdef UARTE_ANY_ASYNC
	nrfx_timer_t timer;
	uint8_t *tx_cache;
#endif
};

static inline NRF_UARTE_Type *get_uarte_instance(const struct device *dev)
{
	const struct uarte_nrfx_config *config = dev->config;

	return config->uarte_regs;
}

static void endtx_isr(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	unsigned int key = irq_lock();

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);
	}

	irq_unlock(key);

}

#ifdef UARTE_ANY_NONE_ASYNC
/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 */
static void uarte_nrfx_isr_int(const void *arg)
{
	const struct device *dev = arg;
	const struct uarte_nrfx_config *config = dev->config;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	/* If interrupt driven and asynchronous APIs are disabled then UART
	 * interrupt is still called to stop TX. Unless it is done using PPI.
	 */
	if (nrf_uarte_int_enable_check(uarte, NRF_UARTE_INT_ENDTX_MASK) &&
		nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX)) {
		endtx_isr(dev);
	}

	if (config->flags & UARTE_CFG_FLAG_LOW_POWER) {
		unsigned int key = irq_lock();

		if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED)) {
			nrf_uarte_disable(uarte);
		}

#ifdef UARTE_INTERRUPT_DRIVEN
		struct uarte_nrfx_data *data = dev->data;

		if (!data->int_driven || data->int_driven->fifo_fill_lock == 0)
#endif
		{
			nrf_uarte_int_disable(uarte,
					      NRF_UARTE_INT_TXSTOPPED_MASK);
		}

		irq_unlock(key);
	}

#ifdef UARTE_INTERRUPT_DRIVEN
	struct uarte_nrfx_data *data = dev->data;

	if (!data->int_driven) {
		return;
	}

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED)) {
		data->int_driven->fifo_fill_lock = 0;
		if (data->int_driven->disable_tx_irq) {
			nrf_uarte_int_disable(uarte,
					      NRF_UARTE_INT_TXSTOPPED_MASK);
			data->int_driven->disable_tx_irq = false;
			return;
		}

	}


	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ERROR)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ERROR);
	}

	if (data->int_driven->cb) {
		data->int_driven->cb(dev, data->int_driven->cb_data);
	}
#endif /* UARTE_INTERRUPT_DRIVEN */
}
#endif /* UARTE_ANY_NONE_ASYNC */

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
static int baudrate_set(const struct device *dev, uint32_t baudrate)
{
	const struct uarte_nrfx_config *config = dev->config;
	/* calculated baudrate divisor */
	nrf_uarte_baudrate_t nrf_baudrate = NRF_BAUDRATE(baudrate);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (nrf_baudrate == 0) {
		return -EINVAL;
	}

	/* scale baudrate setting */
	if (config->clock_freq > 0U) {
		nrf_baudrate /= config->clock_freq / NRF_UARTE_BASE_FREQUENCY_16MHZ;
	}

	nrf_uarte_baudrate_set(uarte, nrf_baudrate);

	return 0;
}

static int uarte_nrfx_configure(const struct device *dev,
				const struct uart_config *cfg)
{
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

	if (baudrate_set(dev, cfg->baudrate) != 0) {
		return -ENOTSUP;
	}

	nrf_uarte_configure(get_uarte_instance(dev), &uarte_cfg);

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


static int uarte_nrfx_err_check(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	/* register bitfields maps to the defines in uart.h */
	return nrf_uarte_errorsrc_get_and_clear(uarte);
}

/* Function returns true if new transfer can be started. Since TXSTOPPED
 * (and ENDTX) is cleared before triggering new transfer, TX is ready for new
 * transfer if any event is set.
 */
static bool is_tx_ready(const struct device *dev)
{
	const struct uarte_nrfx_config *config = dev->config;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	bool ppi_endtx = config->flags & UARTE_CFG_FLAG_PPI_ENDTX;

	return nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED) ||
		(!ppi_endtx ?
		       nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX) : 0);
}

/* Wait until the transmitter is in the idle state. When this function returns,
 * IRQ's are locked with the returned key.
 */
static int wait_tx_ready(const struct device *dev)
{
	unsigned int key;

	do {
		/* wait arbitrary time before back off. */
		bool res;

#if defined(CONFIG_ARCH_POSIX)
		NRFX_WAIT_FOR(is_tx_ready(dev), 33, 3, res);
#else
		NRFX_WAIT_FOR(is_tx_ready(dev), 100, 1, res);
#endif

		if (res) {
			key = irq_lock();
			if (is_tx_ready(dev)) {
				break;
			}

			irq_unlock(key);
		}
		if (IS_ENABLED(CONFIG_MULTITHREADING)) {
			k_msleep(1);
		}
	} while (1);

	return key;
}

#if defined(UARTE_ANY_ASYNC) || defined(CONFIG_PM_DEVICE)
static int pins_state_change(const struct device *dev, bool on)
{
	const struct uarte_nrfx_config *config = dev->config;

	if (config->flags & UARTE_CFG_FLAG_GPIO_MGMT) {
		return pinctrl_apply_state(config->pcfg,
				on ? PINCTRL_STATE_DEFAULT : PINCTRL_STATE_SLEEP);
	}

	return 0;
}
#endif

#ifdef UARTE_ANY_ASYNC

/* Using Macro instead of static inline function to handle NO_OPTIMIZATIONS case
 * where static inline fails on linking.
 */
#define HW_RX_COUNTING_ENABLED(config) \
	(IS_ENABLED(UARTE_HW_ASYNC) ? (config->flags & UARTE_CFG_FLAG_HW_BYTE_COUNTING) : false)

#endif /* UARTE_ANY_ASYNC */

static int uarte_enable(const struct device *dev, uint32_t mask)
{
#ifdef UARTE_ANY_ASYNC
	const struct uarte_nrfx_config *config = dev->config;
	struct uarte_nrfx_data *data = dev->data;

	if (data->async) {
		bool disabled = data->async->low_power_mask == 0;
		int ret;

		data->async->low_power_mask |= mask;
		ret = pins_state_change(dev, true);
		if (ret < 0) {
			return ret;
		}

		if (HW_RX_COUNTING_ENABLED(config) && disabled) {
			const nrfx_timer_t *timer = &config->timer;

			nrfx_timer_enable(timer);

			for (int i = 0; i < data->async->rx_flush_cnt; i++) {
				nrfx_timer_increment(timer);
			}
		}
	}
#endif
	nrf_uarte_enable(get_uarte_instance(dev));

	return 0;
}

/* At this point we should have irq locked and any previous transfer completed.
 * Transfer can be started, no need to wait for completion.
 */
static void tx_start(const struct device *dev, const uint8_t *buf, size_t len)
{
	const struct uarte_nrfx_config *config = dev->config;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		return;
	}
#endif
	nrf_uarte_tx_buffer_set(uarte, buf, len);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_TXSTOPPED);

	if (config->flags & UARTE_CFG_FLAG_LOW_POWER) {
		(void)uarte_enable(dev, UARTE_LOW_POWER_TX);
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);
	}

	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTTX);
}

#if defined(UARTE_ANY_ASYNC) || defined(CONFIG_PM_DEVICE)
static void uart_disable(const struct device *dev)
{
#ifdef UARTE_ANY_ASYNC
	const struct uarte_nrfx_config *config = dev->config;
	struct uarte_nrfx_data *data = dev->data;

	if (data->async && HW_RX_COUNTING_ENABLED(config)) {
		nrfx_timer_disable(&config->timer);
		/* Timer/counter value is reset when disabled. */
		data->async->rx_total_byte_cnt = 0;
		data->async->rx_total_user_byte_cnt = 0;
	}
#endif

	nrf_uarte_disable(get_uarte_instance(dev));
}
#endif

#ifdef UARTE_ANY_ASYNC

static void timer_handler(nrf_timer_event_t event_type, void *p_context) { }
static void rx_timeout(struct k_timer *timer);
static void tx_timeout(struct k_timer *timer);

static int uarte_nrfx_rx_counting_init(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;
	const struct uarte_nrfx_config *cfg = dev->config;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	int ret;

	if (HW_RX_COUNTING_ENABLED(cfg)) {
		nrfx_timer_config_t tmr_config = NRFX_TIMER_DEFAULT_CONFIG(
						NRF_TIMER_BASE_FREQUENCY_GET(cfg->timer.p_reg));

		tmr_config.mode = NRF_TIMER_MODE_COUNTER;
		tmr_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
		ret = nrfx_timer_init(&cfg->timer,
				      &tmr_config,
				      timer_handler);
		if (ret != NRFX_SUCCESS) {
			LOG_ERR("Timer already initialized");
			return -EINVAL;
		} else {
			nrfx_timer_enable(&cfg->timer);
			nrfx_timer_clear(&cfg->timer);
		}

		ret = gppi_channel_alloc(&data->async->rx_cnt.ppi);
		if (ret != NRFX_SUCCESS) {
			LOG_ERR("Failed to allocate PPI Channel");
			nrfx_timer_uninit(&cfg->timer);
			return -EINVAL;
		}

#if CONFIG_HAS_HW_NRF_PPI
		ret = nrfx_ppi_channel_assign(
			data->async->rx_cnt.ppi,
			nrf_uarte_event_address_get(uarte,
						    NRF_UARTE_EVENT_RXDRDY),
			nrfx_timer_task_address_get(&cfg->timer,
						    NRF_TIMER_TASK_COUNT));

		if (ret != NRFX_SUCCESS) {
			return -EIO;
		}
#else
		nrf_uarte_publish_set(uarte,
				      NRF_UARTE_EVENT_RXDRDY,
				      data->async->rx_cnt.ppi);
		nrf_timer_subscribe_set(cfg->timer.p_reg,
					NRF_TIMER_TASK_COUNT,
					data->async->rx_cnt.ppi);

#endif
		ret = gppi_channel_enable(data->async->rx_cnt.ppi);
		if (ret != NRFX_SUCCESS) {
			return -EIO;
		}
	} else {
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_RXDRDY_MASK);
	}

	return 0;
}

static int uarte_nrfx_init(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	int ret = uarte_nrfx_rx_counting_init(dev);

	if (ret != 0) {
		return ret;
	}

	data->async->low_power_mask = UARTE_LOW_POWER_TX;
	nrf_uarte_int_enable(uarte,
			     NRF_UARTE_INT_ENDRX_MASK |
			     NRF_UARTE_INT_RXSTARTED_MASK |
			     NRF_UARTE_INT_ERROR_MASK |
			     NRF_UARTE_INT_RXTO_MASK);
	nrf_uarte_enable(uarte);

	k_timer_init(&data->async->rx_timeout_timer, rx_timeout, NULL);
	k_timer_user_data_set(&data->async->rx_timeout_timer, data);
	k_timer_init(&data->async->tx_timeout_timer, tx_timeout, NULL);
	k_timer_user_data_set(&data->async->tx_timeout_timer, data);

	return 0;
}

/* Attempt to start TX (asynchronous transfer). If hardware is not ready, then pending
 * flag is set. When current poll_out is completed, pending transfer is started.
 * Function must be called with interrupts locked.
 */
static void start_tx_locked(const struct device *dev, struct uarte_nrfx_data *data)
{
	if (!is_tx_ready(dev)) {
		/* Active poll out, postpone until it is completed. */
		data->async->pending_tx = true;
	} else {
		data->async->pending_tx = false;
		data->async->tx_amount = -1;
		tx_start(dev, data->async->xfer_buf, data->async->xfer_len);
	}
}

/* Setup cache buffer (used for sending data outside of RAM memory).
 * During setup data is copied to cache buffer and transfer length is set.
 *
 * @return True if cache was set, false if no more data to put in cache.
 */
static bool setup_tx_cache(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;
	const struct uarte_nrfx_config *config = dev->config;
	size_t remaining = data->async->tx_size - data->async->tx_cache_offset;

	if (!remaining) {
		return false;
	}

	size_t len = MIN(remaining, CONFIG_UART_ASYNC_TX_CACHE_SIZE);

	data->async->xfer_len = len;
	data->async->xfer_buf = config->tx_cache;
	memcpy(config->tx_cache, &data->async->tx_buf[data->async->tx_cache_offset], len);

	return true;
}

static bool has_hwfc(const struct device *dev)
{
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	struct uarte_nrfx_data *data = dev->data;

	return data->uart_config.flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS;
#else
	const struct uarte_nrfx_config *config = dev->config;

	return config->hw_config.hwfc == NRF_UARTE_HWFC_ENABLED;
#endif
}

static int uarte_nrfx_tx(const struct device *dev, const uint8_t *buf,
			 size_t len,
			 int32_t timeout)
{
	struct uarte_nrfx_data *data = dev->data;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	unsigned int key = irq_lock();

	if (data->async->tx_size) {
		irq_unlock(key);
		return -EBUSY;
	}

	data->async->tx_size = len;
	data->async->tx_buf = buf;
	nrf_uarte_int_enable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);

	if (nrfx_is_in_ram(buf)) {
		data->async->xfer_buf = buf;
		data->async->xfer_len = len;
	} else {
		data->async->tx_cache_offset = 0;
		(void)setup_tx_cache(dev);
	}

	start_tx_locked(dev, data);

	irq_unlock(key);

	if (has_hwfc(dev) && timeout != SYS_FOREVER_US) {
		k_timer_start(&data->async->tx_timeout_timer, K_USEC(timeout), K_NO_WAIT);
	}
	return 0;
}

static int uarte_nrfx_tx_abort(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (data->async->tx_buf == NULL) {
		return -EFAULT;
	}

	data->async->pending_tx = false;
	k_timer_stop(&data->async->tx_timeout_timer);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);

	return 0;
}

static void user_callback(const struct device *dev, struct uart_event *evt)
{
	struct uarte_nrfx_data *data = dev->data;

	if (data->async->user_callback) {
		data->async->user_callback(dev, evt, data->async->user_data);
	}
}

static void notify_uart_rx_rdy(const struct device *dev, size_t len)
{
	struct uarte_nrfx_data *data = dev->data;
	struct uart_event evt = {
		.type = UART_RX_RDY,
		.data.rx.buf = data->async->rx_buf,
		.data.rx.len = len,
		.data.rx.offset = data->async->rx_offset
	};

	user_callback(dev, &evt);
}

static void rx_buf_release(const struct device *dev, uint8_t **buf)
{
	if (*buf) {
		struct uart_event evt = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = *buf,
		};

		user_callback(dev, &evt);
		*buf = NULL;
	}
}

static void notify_rx_disable(const struct device *dev)
{
	struct uart_event evt = {
		.type = UART_RX_DISABLED,
	};

	user_callback(dev, (struct uart_event *)&evt);
}

static int uarte_nrfx_rx_enable(const struct device *dev, uint8_t *buf,
				size_t len,
				int32_t timeout)
{
	struct uarte_nrfx_data *data = dev->data;
	const struct uarte_nrfx_config *cfg = dev->config;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	int ret = 0;

	if (cfg->disable_rx) {
		__ASSERT(false, "TX only UARTE instance");
		return -ENOTSUP;
	}

	/* Signal error if RX is already enabled or if the driver is waiting
	 * for the RXTO event after a call to uart_rx_disable() to discard
	 * data from the UARTE internal RX FIFO.
	 */
	if (data->async->rx_enabled || data->async->discard_rx_fifo) {
		return -EBUSY;
	}

	data->async->rx_timeout = timeout;
	data->async->rx_timeout_slab = timeout / RX_TIMEOUT_DIV;

	data->async->rx_buf = buf;
	data->async->rx_buf_len = len;
	data->async->rx_offset = 0;
	data->async->rx_next_buf = NULL;
	data->async->rx_next_buf_len = 0;

	if (cfg->flags & UARTE_CFG_FLAG_LOW_POWER) {
		if (data->async->rx_flush_cnt) {
			int cpy_len = MIN(len, data->async->rx_flush_cnt);

			memcpy(buf, data->async->rx_flush_buffer, cpy_len);
			buf += cpy_len;
			len -= cpy_len;

			/* If flush content filled whole new buffer complete the
			 * request and indicate rx being disabled.
			 */
			if (!len) {
				data->async->rx_flush_cnt -= cpy_len;
				notify_uart_rx_rdy(dev, cpy_len);
				rx_buf_release(dev, &data->async->rx_buf);
				notify_rx_disable(dev);
				return 0;
			}
		}
	}

	nrf_uarte_rx_buffer_set(uarte, buf, len);

	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);

	data->async->rx_enabled = true;
	if (cfg->flags & UARTE_CFG_FLAG_LOW_POWER) {
		unsigned int key = irq_lock();

		ret = uarte_enable(dev, UARTE_LOW_POWER_RX);
		irq_unlock(key);
	}

	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);

	return 0;
}

static int uarte_nrfx_rx_buf_rsp(const struct device *dev, uint8_t *buf,
				 size_t len)
{
	struct uarte_nrfx_data *data = dev->data;
	int err;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	unsigned int key = irq_lock();

	if (data->async->rx_buf == NULL) {
		err = -EACCES;
	} else if (data->async->rx_next_buf == NULL) {
		data->async->rx_next_buf = buf;
		data->async->rx_next_buf_len = len;
		nrf_uarte_rx_buffer_set(uarte, buf, len);
		nrf_uarte_shorts_enable(uarte, NRF_UARTE_SHORT_ENDRX_STARTRX);
		err = 0;
	} else {
		err = -EBUSY;
	}

	irq_unlock(key);

	return err;
}

static int uarte_nrfx_callback_set(const struct device *dev,
				   uart_callback_t callback,
				   void *user_data)
{
	struct uarte_nrfx_data *data = dev->data;

	if (!data->async) {
		return -ENOTSUP;
	}

	data->async->user_callback = callback;
	data->async->user_data = user_data;

	return 0;
}

static int uarte_nrfx_rx_disable(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (data->async->rx_buf == NULL) {
		return -EFAULT;
	}
	if (data->async->rx_next_buf != NULL) {
		nrf_uarte_shorts_disable(uarte, NRF_UARTE_SHORT_ENDRX_STARTRX);
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);
	}

	k_timer_stop(&data->async->rx_timeout_timer);
	data->async->rx_enabled = false;
	data->async->discard_rx_fifo = true;

	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPRX);

	return 0;
}

static void tx_timeout(struct k_timer *timer)
{
	struct uarte_nrfx_data *data = k_timer_user_data_get(timer);
	(void) uarte_nrfx_tx_abort(data->dev);
}

/**
 * Whole timeout is divided by RX_TIMEOUT_DIV into smaller units, rx_timeout
 * is executed periodically every rx_timeout_slab us. If between executions
 * data was received, then we start counting down time from start, if not, then
 * we subtract rx_timeout_slab from rx_timeout_left.
 * If rx_timeout_left is less than rx_timeout_slab it means that receiving has
 * timed out and we should tell user about that.
 */
static void rx_timeout(struct k_timer *timer)
{
	struct uarte_nrfx_data *data = k_timer_user_data_get(timer);
	const struct device *dev = data->dev;
	const struct uarte_nrfx_config *cfg = dev->config;
	uint32_t read;

	if (data->async->is_in_irq) {
		return;
	}

	/* Disable ENDRX ISR, in case ENDRX event is generated, it will be
	 * handled after rx_timeout routine is complete.
	 */
	nrf_uarte_int_disable(get_uarte_instance(dev),
			      NRF_UARTE_INT_ENDRX_MASK);

	if (HW_RX_COUNTING_ENABLED(cfg)) {
		read = nrfx_timer_capture(&cfg->timer, 0);
	} else {
		read = data->async->rx_cnt.cnt;
	}

	/* Check if data was received since last function call */
	if (read != data->async->rx_total_byte_cnt) {
		data->async->rx_total_byte_cnt = read;
		data->async->rx_timeout_left = data->async->rx_timeout;
	}

	/* Check if there is data that was not sent to user yet
	 * Note though that 'len' is a count of data bytes received, but not
	 * necessarily the amount available in the current buffer
	 */
	int32_t len = data->async->rx_total_byte_cnt
		    - data->async->rx_total_user_byte_cnt;

	if (!HW_RX_COUNTING_ENABLED(cfg) &&
	    (len < 0)) {
		/* Prevent too low value of rx_cnt.cnt which may occur due to
		 * latencies in handling of the RXRDY interrupt.
		 * At this point, the number of received bytes is at least
		 * equal to what was reported to the user.
		 */
		data->async->rx_cnt.cnt = data->async->rx_total_user_byte_cnt;
		len = 0;
	}

	/* Check for current buffer being full.
	 * if the UART receives characters before the ENDRX is handled
	 * and the 'next' buffer is set up, then the SHORT between ENDRX and
	 * STARTRX will mean that data will be going into to the 'next' buffer
	 * until the ENDRX event gets a chance to be handled.
	 */
	bool clipped = false;

	if (len + data->async->rx_offset > data->async->rx_buf_len) {
		len = data->async->rx_buf_len - data->async->rx_offset;
		clipped = true;
	}

	if (len > 0) {
		if (clipped ||
			(data->async->rx_timeout_left
				< data->async->rx_timeout_slab)) {
			/* rx_timeout us elapsed since last receiving */
			if (data->async->rx_buf != NULL) {
				notify_uart_rx_rdy(dev, len);
				data->async->rx_offset += len;
				data->async->rx_total_user_byte_cnt += len;
			}
		} else {
			data->async->rx_timeout_left -=
				data->async->rx_timeout_slab;
		}

		/* If there's nothing left to report until the buffers are
		 * switched then the timer can be stopped
		 */
		if (clipped) {
			k_timer_stop(&data->async->rx_timeout_timer);
		}
	}

	nrf_uarte_int_enable(get_uarte_instance(dev),
			     NRF_UARTE_INT_ENDRX_MASK);

}

#define UARTE_ERROR_FROM_MASK(mask)					\
	((mask) & NRF_UARTE_ERROR_OVERRUN_MASK ? UART_ERROR_OVERRUN	\
	 : (mask) & NRF_UARTE_ERROR_PARITY_MASK ? UART_ERROR_PARITY	\
	 : (mask) & NRF_UARTE_ERROR_FRAMING_MASK ? UART_ERROR_FRAMING	\
	 : (mask) & NRF_UARTE_ERROR_BREAK_MASK ? UART_BREAK		\
	 : 0)

static void error_isr(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	uint32_t err = nrf_uarte_errorsrc_get_and_clear(uarte);
	struct uart_event evt = {
		.type = UART_RX_STOPPED,
		.data.rx_stop.reason = UARTE_ERROR_FROM_MASK(err),
	};
	user_callback(dev, &evt);
	(void) uarte_nrfx_rx_disable(dev);
}

static void rxstarted_isr(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};
	user_callback(dev, &evt);
	if (data->async->rx_timeout != SYS_FOREVER_US) {
		data->async->rx_timeout_left = data->async->rx_timeout;
		k_timer_start(&data->async->rx_timeout_timer,
			      K_USEC(data->async->rx_timeout_slab),
			      K_USEC(data->async->rx_timeout_slab));
	}
}

static void endrx_isr(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	data->async->is_in_irq = true;

	/* ensure rx timer is stopped - it will be restarted in RXSTARTED
	 * handler if needed
	 */
	k_timer_stop(&data->async->rx_timeout_timer);

	/* this is the amount that the EasyDMA controller has copied into the
	 * buffer
	 */
	const int rx_amount = nrf_uarte_rx_amount_get(uarte) +
				data->async->rx_flush_cnt;

	data->async->rx_flush_cnt = 0;

	/* The 'rx_offset' can be bigger than 'rx_amount', so it the length
	 * of data we report back the user may need to be clipped.
	 * This can happen because the 'rx_offset' count derives from RXRDY
	 * events, which can occur already for the next buffer before we are
	 * here to handle this buffer. (The next buffer is now already active
	 * because of the ENDRX_STARTRX shortcut)
	 */
	int rx_len = rx_amount - data->async->rx_offset;

	if (rx_len < 0) {
		rx_len = 0;
	}

	data->async->rx_total_user_byte_cnt += rx_len;

	/* Only send the RX_RDY event if there is something to send */
	if (rx_len > 0) {
		notify_uart_rx_rdy(dev, rx_len);
	}

	if (!data->async->rx_enabled) {
		data->async->is_in_irq = false;
		return;
	}

	rx_buf_release(dev, &data->async->rx_buf);

	/* If there is a next buffer, then STARTRX will have already been
	 * invoked by the short (the next buffer will be filling up already)
	 * and here we just do the swap of which buffer the driver is following,
	 * the next rx_timeout() will update the rx_offset.
	 */
	unsigned int key = irq_lock();

	if (data->async->rx_next_buf) {
		data->async->rx_buf = data->async->rx_next_buf;
		data->async->rx_buf_len = data->async->rx_next_buf_len;
		data->async->rx_next_buf = NULL;
		data->async->rx_next_buf_len = 0;

		data->async->rx_offset = 0;
		/* Check is based on assumption that ISR handler handles
		 * ENDRX before RXSTARTED so if short was set on time, RXSTARTED
		 * event will be set.
		 */
		if (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXSTARTED)) {
			nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);
		}
		/* Remove the short until the subsequent next buffer is setup */
		nrf_uarte_shorts_disable(uarte, NRF_UARTE_SHORT_ENDRX_STARTRX);
	} else {
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPRX);
	}

	irq_unlock(key);

	data->async->is_in_irq = false;
}

/* Function for flushing internal RX fifo. Function can be called in case
 * flushed data is discarded or when data is valid and needs to be retrieved.
 *
 * However, UARTE does not update RXAMOUNT register if fifo is empty. Old value
 * remains. In certain cases it makes it impossible to distinguish between
 * case when fifo was empty and not. Function is trying to minimize chances of
 * error with following measures:
 * - RXAMOUNT is read before flushing and compared against value after flushing
 *   if they differ it indicates that data was flushed
 * - user buffer is dirtied and if RXAMOUNT did not changed it is checked if
 *   it is still dirty. If not then it indicates that data was flushed
 *
 * In other cases function indicates that fifo was empty. It means that if
 * number of bytes in the fifo equal last rx transfer length and data is equal
 * to dirty marker it will be discarded.
 *
 * @param dev Device.
 * @param buf Buffer for flushed data, null indicates that flushed data can be
 *	      dropped but we still want to get amount of data flushed.
 * @param len Buffer size, not used if @p buf is null.
 *
 * @return number of bytes flushed from the fifo.
 */
static uint8_t rx_flush(const struct device *dev, uint8_t *buf, uint32_t len)
{
	/* Flushing RX fifo requires buffer bigger than 4 bytes to empty fifo*/
	static const uint8_t dirty;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	uint32_t prev_rx_amount = nrf_uarte_rx_amount_get(uarte);
	uint8_t tmp_buf[UARTE_HW_RX_FIFO_SIZE];
	uint8_t *flush_buf = buf ? buf : tmp_buf;
	size_t flush_len = buf ? len : sizeof(tmp_buf);

	if (buf) {
		flush_buf = buf;
		flush_len = len;
	} else {
		flush_buf = tmp_buf;
		flush_len = sizeof(tmp_buf);
	}

	memset(flush_buf, dirty, flush_len);
	nrf_uarte_rx_buffer_set(uarte, flush_buf, flush_len);
	/* Final part of handling RXTO event is in ENDRX interrupt
	 * handler. ENDRX is generated as a result of FLUSHRX task.
	 */
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_FLUSHRX);
	while (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		/* empty */
	}
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);

	uint32_t rx_amount = nrf_uarte_rx_amount_get(uarte);

	if (rx_amount != prev_rx_amount) {
		return rx_amount;
	}

	for (int i = 0; i < flush_len; i++) {
		if (flush_buf[i] != dirty) {
			return rx_amount;
		}
	}

	return 0;
}

static void async_uart_release(const struct device *dev, uint32_t dir_mask)
{
	struct uarte_nrfx_data *data = dev->data;
	unsigned int key = irq_lock();

	data->async->low_power_mask &= ~dir_mask;
	if (!data->async->low_power_mask) {
		if (dir_mask == UARTE_LOW_POWER_RX) {
			data->async->rx_flush_cnt =
				rx_flush(dev, data->async->rx_flush_buffer,
					 sizeof(data->async->rx_flush_buffer));
		}

		uart_disable(dev);
		int err = pins_state_change(dev, false);

		(void)err;
		__ASSERT_NO_MSG(err == 0);
	}

	irq_unlock(key);
}

/* This handler is called when the receiver is stopped. If rx was aborted
 * data from fifo is flushed.
 */
static void rxto_isr(const struct device *dev)
{
	const struct uarte_nrfx_config *config = dev->config;
	struct uarte_nrfx_data *data = dev->data;

	rx_buf_release(dev, &data->async->rx_buf);
	rx_buf_release(dev, &data->async->rx_next_buf);

	/* This point can be reached in two cases:
	 * 1. RX is disabled because all provided RX buffers have been filled.
	 * 2. RX was explicitly disabled by a call to uart_rx_disable().
	 * In both cases, the rx_enabled flag is cleared, so that RX can be
	 * enabled again.
	 * In the second case, additionally, data from the UARTE internal RX
	 * FIFO need to be discarded.
	 */
	data->async->rx_enabled = false;
	if (data->async->discard_rx_fifo) {
		uint8_t flushed;

		data->async->discard_rx_fifo = false;
		flushed = rx_flush(dev, NULL, 0);
		if (HW_RX_COUNTING_ENABLED(config)) {
			/* It need to be included because TIMER+PPI got RXDRDY events
			 * and counted those flushed bytes.
			 */
			data->async->rx_total_user_byte_cnt += flushed;
		}
	}

	if (config->flags & UARTE_CFG_FLAG_LOW_POWER) {
		async_uart_release(dev, UARTE_LOW_POWER_RX);
	}

	notify_rx_disable(dev);
}

static void txstopped_isr(const struct device *dev)
{
	const struct uarte_nrfx_config *config = dev->config;
	struct uarte_nrfx_data *data = dev->data;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	unsigned int key;

	if (config->flags & UARTE_CFG_FLAG_LOW_POWER) {
		nrf_uarte_int_disable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);
		async_uart_release(dev, UARTE_LOW_POWER_TX);

		if (!data->async->tx_size) {
			return;
		}
	}

	if (!data->async->tx_buf) {
		return;
	}

	key = irq_lock();
	size_t amount = (data->async->tx_amount >= 0) ?
			data->async->tx_amount : nrf_uarte_tx_amount_get(uarte);

	irq_unlock(key);

	/* If there is a pending tx request, it means that uart_tx()
	 * was called when there was ongoing uart_poll_out. Handling
	 * TXSTOPPED interrupt means that uart_poll_out has completed.
	 */
	if (data->async->pending_tx) {
		key = irq_lock();
		start_tx_locked(dev, data);
		irq_unlock(key);
		return;
	}

	/* Cache buffer is used because tx_buf wasn't in RAM. */
	if (data->async->tx_buf != data->async->xfer_buf) {
		/* In that case setup next chunk. If that was the last chunk
		 * fall back to reporting TX_DONE.
		 */
		if (amount == data->async->xfer_len) {
			data->async->tx_cache_offset += amount;
			if (setup_tx_cache(dev)) {
				key = irq_lock();
				start_tx_locked(dev, data);
				irq_unlock(key);
				return;
			}

			/* Amount is already included in tx_cache_offset. */
			amount = data->async->tx_cache_offset;
		} else {
			/* TX was aborted, include tx_cache_offset in amount. */
			amount += data->async->tx_cache_offset;
		}
	}

	k_timer_stop(&data->async->tx_timeout_timer);

	struct uart_event evt = {
		.data.tx.buf = data->async->tx_buf,
		.data.tx.len = amount,
	};
	if (amount == data->async->tx_size) {
		evt.type = UART_TX_DONE;
	} else {
		evt.type = UART_TX_ABORTED;
	}

	nrf_uarte_int_disable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);
	data->async->tx_buf = NULL;
	data->async->tx_size = 0;

	user_callback(dev, &evt);
}

static void uarte_nrfx_isr_async(const void *arg)
{
	const struct device *dev = arg;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	const struct uarte_nrfx_config *config = dev->config;

	if (!HW_RX_COUNTING_ENABLED(config)
	    && nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXDRDY)) {
		struct uarte_nrfx_data *data = dev->data;

		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXDRDY);
		data->async->rx_cnt.cnt++;
		return;
	}

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ERROR)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ERROR);
		error_isr(dev);
	}

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)
	    && nrf_uarte_int_enable_check(uarte, NRF_UARTE_INT_ENDRX_MASK)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
		endrx_isr(dev);
	}

	/* RXSTARTED must be handled after ENDRX because it starts the RX timeout
	 * and if order is swapped then ENDRX will stop this timeout.
	 * Skip if ENDRX is set when RXSTARTED is set. It means that
	 * ENDRX occurred after check for ENDRX in isr which may happen when
	 * UARTE interrupt got preempted. Events are not cleared
	 * and isr will be called again. ENDRX will be handled first.
	 */
	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXSTARTED) &&
	    !nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);
		rxstarted_isr(dev);
	}

	/* RXTO must be handled after ENDRX which should notify the buffer.
	 * Skip if ENDRX is set when RXTO is set. It means that
	 * ENDRX occurred after check for ENDRX in isr which may happen when
	 * UARTE interrupt got preempted. Events are not cleared
	 * and isr will be called again. ENDRX will be handled first.
	 */
	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXTO) &&
	    !nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXTO);
		rxto_isr(dev);
	}

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX)
	    && nrf_uarte_int_enable_check(uarte, NRF_UARTE_INT_ENDTX_MASK)) {
		endtx_isr(dev);
	}

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED)
	    && nrf_uarte_int_enable_check(uarte,
					  NRF_UARTE_INT_TXSTOPPED_MASK)) {
		txstopped_isr(dev);
	}
}

#endif /* UARTE_ANY_ASYNC */

/**
 * @brief Poll the device for input.
 *
 * @param dev UARTE device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer is empty.
 */
static int uarte_nrfx_poll_in(const struct device *dev, unsigned char *c)
{

	const struct uarte_nrfx_data *data = dev->data;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

#ifdef UARTE_ANY_ASYNC
	if (data->async) {
		return -ENOTSUP;
	}
#endif

	if (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		return -1;
	}

	*c = *data->rx_data;

	/* clear the interrupt */
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);

	return 0;
}

/**
 * @brief Output a character in polled mode.
 *
 * @param dev UARTE device struct
 * @param c Character to send
 */
static void uarte_nrfx_poll_out(const struct device *dev, unsigned char c)
{
	struct uarte_nrfx_data *data = dev->data;
	bool isr_mode = k_is_in_isr() || k_is_pre_kernel();
	unsigned int key;

	if (isr_mode) {
		while (1) {
			key = irq_lock();
			if (is_tx_ready(dev)) {
#if UARTE_ANY_ASYNC
				if (data->async && data->async->tx_size &&
					data->async->tx_amount < 0) {
					data->async->tx_amount =
						nrf_uarte_tx_amount_get(
						      get_uarte_instance(dev));
				}
#endif
				break;
			}

			irq_unlock(key);
			Z_SPIN_DELAY(3);
		}
	} else {
		key = wait_tx_ready(dev);
	}

	*data->char_out = c;
	tx_start(dev, data->char_out, 1);

	irq_unlock(key);
}


#ifdef UARTE_INTERRUPT_DRIVEN
/** Interrupt driven FIFO fill function */
static int uarte_nrfx_fifo_fill(const struct device *dev,
				const uint8_t *tx_data,
				int len)
{
	struct uarte_nrfx_data *data = dev->data;

	len = MIN(len, data->int_driven->tx_buff_size);
	if (!atomic_cas(&data->int_driven->fifo_fill_lock, 0, 1)) {
		return 0;
	}

	/* Copy data to RAM buffer for EasyDMA transfer */
	memcpy(data->int_driven->tx_buffer, tx_data, len);

	unsigned int key = irq_lock();

	if (!is_tx_ready(dev)) {
		data->int_driven->fifo_fill_lock = 0;
		len = 0;
	} else {
		tx_start(dev, data->int_driven->tx_buffer, len);
	}

	irq_unlock(key);

	return len;
}

/** Interrupt driven FIFO read function */
static int uarte_nrfx_fifo_read(const struct device *dev,
				uint8_t *rx_data,
				const int size)
{
	int num_rx = 0;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	const struct uarte_nrfx_data *data = dev->data;

	if (size > 0 && nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		/* Clear the interrupt */
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);

		/* Receive a character */
		rx_data[num_rx++] = *data->rx_data;

		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);
	}

	return num_rx;
}

/** Interrupt driven transfer enabling function */
static void uarte_nrfx_irq_tx_enable(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	struct uarte_nrfx_data *data = dev->data;
	unsigned int key = irq_lock();

	data->int_driven->disable_tx_irq = false;
	data->int_driven->tx_irq_enabled = true;
	nrf_uarte_int_enable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);

	irq_unlock(key);
}

/** Interrupt driven transfer disabling function */
static void uarte_nrfx_irq_tx_disable(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;
	/* TX IRQ will be disabled after current transmission is finished */
	data->int_driven->disable_tx_irq = true;
	data->int_driven->tx_irq_enabled = false;
}

/** Interrupt driven transfer ready function */
static int uarte_nrfx_irq_tx_ready_complete(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	struct uarte_nrfx_data *data = dev->data;

	/* ENDTX flag is always on so that ISR is called when we enable TX IRQ.
	 * Because of that we have to explicitly check if ENDTX interrupt is
	 * enabled, otherwise this function would always return true no matter
	 * what would be the source of interrupt.
	 */
	bool ready = data->int_driven->tx_irq_enabled &&
		     nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED);

	if (ready) {
		data->int_driven->fifo_fill_lock = 0;
	}

	return ready;
}

static int uarte_nrfx_irq_rx_ready(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	return nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX);
}

/** Interrupt driven receiver enabling function */
static void uarte_nrfx_irq_rx_enable(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ENDRX_MASK);
}

/** Interrupt driven receiver disabling function */
static void uarte_nrfx_irq_rx_disable(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ENDRX_MASK);
}

/** Interrupt driven error enabling function */
static void uarte_nrfx_irq_err_enable(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ERROR_MASK);
}

/** Interrupt driven error disabling function */
static void uarte_nrfx_irq_err_disable(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ERROR_MASK);
}

/** Interrupt driven pending status function */
static int uarte_nrfx_irq_is_pending(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	return ((nrf_uarte_int_enable_check(uarte,
					    NRF_UARTE_INT_TXSTOPPED_MASK) &&
		 uarte_nrfx_irq_tx_ready_complete(dev))
		||
		(nrf_uarte_int_enable_check(uarte,
					    NRF_UARTE_INT_ENDRX_MASK) &&
		 uarte_nrfx_irq_rx_ready(dev)));
}

/** Interrupt driven interrupt update function */
static int uarte_nrfx_irq_update(const struct device *dev)
{
	return 1;
}

/** Set the callback function */
static void uarte_nrfx_irq_callback_set(const struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uarte_nrfx_data *data = dev->data;

	data->int_driven->cb = cb;
	data->int_driven->cb_data = cb_data;
}
#endif /* UARTE_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_nrfx_uarte_driver_api = {
	.poll_in		= uarte_nrfx_poll_in,
	.poll_out		= uarte_nrfx_poll_out,
	.err_check		= uarte_nrfx_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure              = uarte_nrfx_configure,
	.config_get             = uarte_nrfx_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef UARTE_ANY_ASYNC
	.callback_set		= uarte_nrfx_callback_set,
	.tx			= uarte_nrfx_tx,
	.tx_abort		= uarte_nrfx_tx_abort,
	.rx_enable		= uarte_nrfx_rx_enable,
	.rx_buf_rsp		= uarte_nrfx_rx_buf_rsp,
	.rx_disable		= uarte_nrfx_rx_disable,
#endif /* UARTE_ANY_ASYNC */
#ifdef UARTE_INTERRUPT_DRIVEN
	.fifo_fill		= uarte_nrfx_fifo_fill,
	.fifo_read		= uarte_nrfx_fifo_read,
	.irq_tx_enable		= uarte_nrfx_irq_tx_enable,
	.irq_tx_disable		= uarte_nrfx_irq_tx_disable,
	.irq_tx_ready		= uarte_nrfx_irq_tx_ready_complete,
	.irq_rx_enable		= uarte_nrfx_irq_rx_enable,
	.irq_rx_disable		= uarte_nrfx_irq_rx_disable,
	.irq_tx_complete	= uarte_nrfx_irq_tx_ready_complete,
	.irq_rx_ready		= uarte_nrfx_irq_rx_ready,
	.irq_err_enable		= uarte_nrfx_irq_err_enable,
	.irq_err_disable	= uarte_nrfx_irq_err_disable,
	.irq_is_pending		= uarte_nrfx_irq_is_pending,
	.irq_update		= uarte_nrfx_irq_update,
	.irq_callback_set	= uarte_nrfx_irq_callback_set,
#endif /* UARTE_INTERRUPT_DRIVEN */
};

static int endtx_stoptx_ppi_init(NRF_UARTE_Type *uarte,
				 struct uarte_nrfx_data *data)
{
	nrfx_err_t ret;

	ret = gppi_channel_alloc(&data->ppi_ch_endtx);
	if (ret != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate PPI Channel");
		return -EIO;
	}

	nrfx_gppi_channel_endpoints_setup(data->ppi_ch_endtx,
		nrf_uarte_event_address_get(uarte, NRF_UARTE_EVENT_ENDTX),
		nrf_uarte_task_address_get(uarte, NRF_UARTE_TASK_STOPTX));
	nrfx_gppi_channels_enable(BIT(data->ppi_ch_endtx));

	return 0;
}

static int uarte_instance_init(const struct device *dev,
			       uint8_t interrupts_active)
{
	int err;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	struct uarte_nrfx_data *data = dev->data;
	const struct uarte_nrfx_config *cfg = dev->config;

	nrf_uarte_disable(uarte);

	data->dev = dev;

#ifdef CONFIG_ARCH_POSIX
	/* For simulation the DT provided peripheral address needs to be corrected */
	((struct pinctrl_dev_config *)cfg->pcfg)->reg = (uintptr_t)cfg->uarte_regs;
#endif

	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	err = uarte_nrfx_configure(dev, &data->uart_config);
	if (err) {
		return err;
	}
#else
	nrf_uarte_baudrate_set(uarte, cfg->baudrate);
	nrf_uarte_configure(uarte, &cfg->hw_config);
#endif

	if (IS_ENABLED(UARTE_ENHANCED_POLL_OUT) &&
	    cfg->flags & UARTE_CFG_FLAG_PPI_ENDTX) {
		err = endtx_stoptx_ppi_init(uarte, data);
		if (err < 0) {
			return err;
		}
	}


#ifdef UARTE_ANY_ASYNC
	if (data->async) {
		err = uarte_nrfx_init(dev);
		if (err < 0) {
			return err;
		}
	} else
#endif
	{
		/* Enable receiver and transmitter */
		nrf_uarte_enable(uarte);

		if (!cfg->disable_rx) {
			nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);

			nrf_uarte_rx_buffer_set(uarte, data->rx_data, 1);
			nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);
		}
	}

	if (!(cfg->flags & UARTE_CFG_FLAG_PPI_ENDTX)) {
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ENDTX_MASK);
	}

	if (cfg->flags & UARTE_CFG_FLAG_LOW_POWER) {
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);
	}

	/* Set TXSTOPPED event by requesting fake (zero-length) transfer.
	 * Pointer to RAM variable (data->tx_buffer) is set because otherwise
	 * such operation may result in HardFault or RAM corruption.
	 */
	nrf_uarte_tx_buffer_set(uarte, data->char_out, 0);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTTX);

	/* switch off transmitter to save an energy */
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
/** @brief Pend until TX is stopped.
 *
 * There are 2 configurations that must be handled:
 * - ENDTX->TXSTOPPED PPI enabled - just pend until TXSTOPPED event is set
 * - disable ENDTX interrupt and manually trigger STOPTX, pend for TXSTOPPED
 */
static void wait_for_tx_stopped(const struct device *dev)
{
	const struct uarte_nrfx_config *config = dev->config;
	bool ppi_endtx = config->flags & UARTE_CFG_FLAG_PPI_ENDTX;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	bool res;

	if (!ppi_endtx) {
		/* We assume here that it can be called from any context,
		 * including the one that uarte interrupt will not preempt.
		 * Disable endtx interrupt to ensure that it will not be triggered
		 * (if in lower priority context) and stop TX if necessary.
		 */
		nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ENDTX_MASK);
		NRFX_WAIT_FOR(is_tx_ready(dev), 1000, 1, res);
		if (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED)) {
			nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);
			nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);
		}
	}

	NRFX_WAIT_FOR(nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED),
		      1000, 1, res);

	if (!ppi_endtx) {
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ENDTX_MASK);
	}
}


static int uarte_nrfx_pm_action(const struct device *dev,
				enum pm_device_action action)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
#if defined(UARTE_ANY_ASYNC) || defined(UARTE_INTERRUPT_DRIVEN)
	struct uarte_nrfx_data *data = dev->data;
#endif
	const struct uarte_nrfx_config *cfg = dev->config;
	int ret;

#ifdef UARTE_ANY_ASYNC
	/* If low power mode for asynchronous mode is used then there is nothing to do here.
	 * In low power mode UARTE is turned off whenever there is no activity.
	 */
	if (data->async && (cfg->flags & UARTE_CFG_FLAG_LOW_POWER)) {
		return 0;
	}
#endif

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:

		ret = pins_state_change(dev, true);
		if (ret < 0) {
			return ret;
		}

		nrf_uarte_enable(uarte);

#ifdef UARTE_ANY_ASYNC
		if (data->async) {
			if (HW_RX_COUNTING_ENABLED(cfg)) {
				nrfx_timer_enable(&cfg->timer);
			}

			return 0;
		}
#endif
		if (!cfg->disable_rx) {

			nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
			nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);
#ifdef UARTE_INTERRUPT_DRIVEN
			if (data->int_driven &&
			    data->int_driven->rx_irq_enabled) {
				nrf_uarte_int_enable(uarte,
						     NRF_UARTE_INT_ENDRX_MASK);
			}
#endif
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Disabling UART requires stopping RX, but stop RX event is
		 * only sent after each RX if async UART API is used.
		 */
#ifdef UARTE_ANY_ASYNC
		if (data->async) {
			/* Entering inactive state requires device to be no
			 * active asynchronous calls.
			 */
			__ASSERT_NO_MSG(!data->async->rx_enabled);
			__ASSERT_NO_MSG(!data->async->tx_size);

		}
#endif
		if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXSTARTED)) {
#ifdef UARTE_INTERRUPT_DRIVEN
			if (data->int_driven) {
				data->int_driven->rx_irq_enabled =
					nrf_uarte_int_enable_check(uarte,
						NRF_UARTE_INT_ENDRX_MASK);
				if (data->int_driven->rx_irq_enabled) {
					nrf_uarte_int_disable(uarte,
						NRF_UARTE_INT_ENDRX_MASK);
				}
			}
#endif
			nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPRX);
			while (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXTO)) {
				/* Busy wait for event to register */
				Z_SPIN_DELAY(2);
			}
			nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);
			nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXTO);
			nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
			nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ERROR);
		}

		wait_for_tx_stopped(dev);
		uart_disable(dev);

		ret = pins_state_change(dev, false);
		if (ret < 0) {
			return ret;
		}

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

#define UARTE(idx)			DT_NODELABEL(uart##idx)
#define UARTE_HAS_PROP(idx, prop)	DT_NODE_HAS_PROP(UARTE(idx), prop)
#define UARTE_PROP(idx, prop)		DT_PROP(UARTE(idx), prop)

#define UARTE_IRQ_CONFIGURE(idx, isr_handler)				       \
	do {								       \
		IRQ_CONNECT(DT_IRQN(UARTE(idx)), DT_IRQ(UARTE(idx), priority), \
			    isr_handler, DEVICE_DT_GET(UARTE(idx)), 0);	       \
		irq_enable(DT_IRQN(UARTE(idx)));			       \
	} while (false)

/* Low power mode is used when disable_rx is not defined or in async mode if
 * kconfig option is enabled.
 */
#define USE_LOW_POWER(idx) \
	((!UARTE_PROP(idx, disable_rx) &&				       \
	COND_CODE_1(CONFIG_UART_##idx##_ASYNC,				       \
		(!IS_ENABLED(CONFIG_UART_##idx##_NRF_ASYNC_LOW_POWER)),	       \
		(1))) ? 0 : UARTE_CFG_FLAG_LOW_POWER)

#define UARTE_DISABLE_RX_INIT(node_id) \
	.disable_rx = DT_PROP(node_id, disable_rx)

#define UARTE_GET_FREQ(idx) DT_PROP(DT_CLOCKS_CTLR(UARTE(idx)), clock_frequency)

#define UARTE_GET_BAUDRATE_DIV(idx)						\
	COND_CODE_1(DT_CLOCKS_HAS_IDX(UARTE(idx), 0),				\
		   ((UARTE_GET_FREQ(idx) / NRF_UARTE_BASE_FREQUENCY_16MHZ)), (1))

/* When calculating baudrate we need to take into account that some instances
 * must have baudrate adjusted to the ratio between UARTE clocking frequency and 16 MHz.
 */
#define UARTE_GET_BAUDRATE(idx) \
	(NRF_BAUDRATE(UARTE_PROP(idx, current_speed)) / UARTE_GET_BAUDRATE_DIV(idx))

/* Macro for setting nRF specific configuration structures. */
#define UARTE_NRF_CONFIG(idx) {							\
		.hwfc = (UARTE_PROP(idx, hw_flow_control) ==			\
			UART_CFG_FLOW_CTRL_RTS_CTS) ?				\
			NRF_UARTE_HWFC_ENABLED : NRF_UARTE_HWFC_DISABLED,	\
		.parity = IS_ENABLED(CONFIG_UART_##idx##_NRF_PARITY_BIT) ?	\
			NRF_UARTE_PARITY_INCLUDED : NRF_UARTE_PARITY_EXCLUDED,	\
		IF_ENABLED(UARTE_HAS_STOP_CONFIG, (.stop = NRF_UARTE_STOP_ONE,))\
		IF_ENABLED(UARTE_ODD_PARITY_ALLOWED,				\
			(.paritytype = NRF_UARTE_PARITYTYPE_EVEN,))		\
	}

/* Macro for setting zephyr specific configuration structures. */
#define UARTE_CONFIG(idx) {						       \
		.baudrate = UARTE_PROP(idx, current_speed),		       \
		.data_bits = UART_CFG_DATA_BITS_8,			       \
		.stop_bits = UART_CFG_STOP_BITS_1,			       \
		.parity = IS_ENABLED(CONFIG_UART_##idx##_NRF_PARITY_BIT)       \
			  ? UART_CFG_PARITY_EVEN			       \
			  : UART_CFG_PARITY_NONE,			       \
		.flow_ctrl = UARTE_PROP(idx, hw_flow_control)		       \
			     ? UART_CFG_FLOW_CTRL_RTS_CTS		       \
			     : UART_CFG_FLOW_CTRL_NONE,			       \
	}

#define UART_NRF_UARTE_DEVICE(idx)					       \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(UARTE(idx));		       \
	UARTE_INT_DRIVEN(idx);						       \
	PINCTRL_DT_DEFINE(UARTE(idx));					       \
	IF_ENABLED(CONFIG_UART_##idx##_ASYNC, (				       \
		static uint8_t						       \
			uarte##idx##_tx_cache[CONFIG_UART_ASYNC_TX_CACHE_SIZE] \
			UARTE_MEMORY_SECTION(idx);			       \
		struct uarte_async_cb uarte##idx##_async;))		       \
	static uint8_t uarte##idx##_char_out UARTE_MEMORY_SECTION(idx);	       \
	static uint8_t uarte##idx##_rx_data UARTE_MEMORY_SECTION(idx);	       \
	static struct uarte_nrfx_data uarte_##idx##_data = {		       \
		.char_out = &uarte##idx##_char_out,			       \
		.rx_data = &uarte##idx##_rx_data,			       \
		IF_ENABLED(CONFIG_UART_USE_RUNTIME_CONFIGURE,		       \
				(.uart_config = UARTE_CONFIG(idx),))	       \
		IF_ENABLED(CONFIG_UART_##idx##_ASYNC,			       \
			    (.async = &uarte##idx##_async,))		       \
		IF_ENABLED(CONFIG_UART_##idx##_INTERRUPT_DRIVEN,	       \
			    (.int_driven = &uarte##idx##_int_driven,))	       \
	};								       \
	COND_CODE_1(CONFIG_UART_USE_RUNTIME_CONFIGURE, (),		       \
		(BUILD_ASSERT(NRF_BAUDRATE(UARTE_PROP(idx, current_speed)) > 0,\
			  "Unsupported baudrate");))			       \
	static const struct uarte_nrfx_config uarte_##idx##z_config = {	       \
		COND_CODE_1(CONFIG_UART_USE_RUNTIME_CONFIGURE, (),	       \
		    (.baudrate = UARTE_GET_BAUDRATE(idx),		       \
		     .hw_config = UARTE_NRF_CONFIG(idx),))		       \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(UARTE(idx)),		       \
		.uarte_regs = _CONCAT(NRF_UARTE, idx),                         \
		.flags =						       \
			(IS_ENABLED(CONFIG_UART_##idx##_GPIO_MANAGEMENT) ?     \
				UARTE_CFG_FLAG_GPIO_MGMT : 0) |		       \
			(IS_ENABLED(CONFIG_UART_##idx##_ENHANCED_POLL_OUT) ?   \
				UARTE_CFG_FLAG_PPI_ENDTX : 0) |		       \
			(IS_ENABLED(CONFIG_UART_##idx##_NRF_HW_ASYNC) ?        \
				UARTE_CFG_FLAG_HW_BYTE_COUNTING : 0) |	       \
			USE_LOW_POWER(idx),				       \
		UARTE_DISABLE_RX_INIT(UARTE(idx)),			       \
		IF_ENABLED(CONFIG_UART_##idx##_ASYNC,			       \
				(.tx_cache = uarte##idx##_tx_cache,))	       \
		IF_ENABLED(CONFIG_UART_##idx##_NRF_HW_ASYNC,		       \
			(.timer = NRFX_TIMER_INSTANCE(			       \
				CONFIG_UART_##idx##_NRF_HW_ASYNC_TIMER),))     \
		IF_ENABLED(DT_CLOCKS_HAS_IDX(UARTE(idx), 0),		       \
			   (.clock_freq = DT_PROP(DT_CLOCKS_CTLR(UARTE(idx)),  \
						  clock_frequency),))	       \
	};								       \
	static int uarte_##idx##_init(const struct device *dev)		       \
	{								       \
		COND_CODE_1(CONFIG_UART_##idx##_ASYNC,			       \
			   (UARTE_IRQ_CONFIGURE(idx, uarte_nrfx_isr_async);),  \
			   (UARTE_IRQ_CONFIGURE(idx, uarte_nrfx_isr_int);))    \
		return uarte_instance_init(				       \
			dev,						       \
			IS_ENABLED(CONFIG_UART_##idx##_INTERRUPT_DRIVEN));     \
	}								       \
									       \
	PM_DEVICE_DT_DEFINE(UARTE(idx), uarte_nrfx_pm_action);		       \
									       \
	DEVICE_DT_DEFINE(UARTE(idx),					       \
		      uarte_##idx##_init,				       \
		      PM_DEVICE_DT_GET(UARTE(idx)),			       \
		      &uarte_##idx##_data,				       \
		      &uarte_##idx##z_config,				       \
		      PRE_KERNEL_1,					       \
		      CONFIG_SERIAL_INIT_PRIORITY,			       \
		      &uart_nrfx_uarte_driver_api)

#define UARTE_INT_DRIVEN(idx)						       \
	IF_ENABLED(CONFIG_UART_##idx##_INTERRUPT_DRIVEN,		       \
		(static uint8_t uarte##idx##_tx_buffer			       \
			[MIN(CONFIG_UART_##idx##_NRF_TX_BUFFER_SIZE,	       \
			     BIT_MASK(UARTE##idx##_EASYDMA_MAXCNT_SIZE))]      \
			UARTE_MEMORY_SECTION(idx);			       \
		 static struct uarte_nrfx_int_driven			       \
			uarte##idx##_int_driven = {			       \
				.tx_buffer = uarte##idx##_tx_buffer,	       \
				.tx_buff_size = sizeof(uarte##idx##_tx_buffer),\
			};))

#define UARTE_MEMORY_SECTION(idx)					       \
	COND_CODE_1(UARTE_HAS_PROP(idx, memory_regions),		       \
		(__attribute__((__section__(LINKER_DT_NODE_REGION_NAME(	       \
			DT_PHANDLE(UARTE(idx), memory_regions)))))),	       \
		())

#define COND_UART_NRF_UARTE_DEVICE(unused, prefix, i, _) \
	IF_ENABLED(CONFIG_HAS_HW_NRF_UARTE##prefix##i, (UART_NRF_UARTE_DEVICE(prefix##i);))

UARTE_FOR_EACH_INSTANCE(COND_UART_NRF_UARTE_DEVICE, (), ())
