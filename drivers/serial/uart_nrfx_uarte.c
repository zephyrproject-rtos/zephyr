/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Nordic Semiconductor nRF UARTE
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <hal/nrf_uarte.h>
#include <nrfx_timer.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <soc.h>
#include <dmm.h>
#include <helpers/nrfx_gppi.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_nrfx_uarte, CONFIG_UART_LOG_LEVEL);

#define RX_FLUSH_WORKAROUND 1

#define UARTE(idx)                DT_NODELABEL(uart##idx)
#define UARTE_HAS_PROP(idx, prop) DT_NODE_HAS_PROP(UARTE(idx), prop)
#define UARTE_PROP(idx, prop)     DT_PROP(UARTE(idx), prop)

#define UARTE_IS_CACHEABLE(idx) DMM_IS_REG_CACHEABLE(DT_PHANDLE(UARTE(idx), memory_regions))

/* Execute macro f(x) for all instances. */
#define UARTE_FOR_EACH_INSTANCE(f, sep, off_code, ...)                                             \
	NRFX_FOREACH_PRESENT(UARTE, f, sep, off_code, __VA_ARGS__)

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
#define UARTE_ANY_HW_ASYNC 1
#endif

/* Determine if any instance is using enhanced poll_out feature. */
#define IS_ENHANCED_POLL_OUT(unused, prefix, i, _) \
	IS_ENABLED(CONFIG_UART_##prefix##i##_ENHANCED_POLL_OUT)

#if UARTE_FOR_EACH_INSTANCE(IS_ENHANCED_POLL_OUT, (||), (0))
#define UARTE_ENHANCED_POLL_OUT 1
#endif

#define INSTANCE_PROP(unused, prefix, i, prop) UARTE_PROP(prefix##i, prop)
#define INSTANCE_PRESENT(unused, prefix, i, prop) 1

/* Driver supports case when all or none instances support that HW feature. */
#if	(UARTE_FOR_EACH_INSTANCE(INSTANCE_PROP, (+), (0), endtx_stoptx_supported)) == \
	(UARTE_FOR_EACH_INSTANCE(INSTANCE_PRESENT, (+), (0), endtx_stoptx_supported))
#define UARTE_HAS_ENDTX_STOPTX_SHORT 1
#endif

#if	(UARTE_FOR_EACH_INSTANCE(INSTANCE_PROP, (+), (0), frame_timeout_supported)) == \
	(UARTE_FOR_EACH_INSTANCE(INSTANCE_PRESENT, (+), (0), frame_timeout_supported))
#define UARTE_HAS_FRAME_TIMEOUT 1
#endif

#define INSTANCE_NEEDS_CACHE_MGMT(unused, prefix, i, prop) UARTE_IS_CACHEABLE(prefix##i)

#if UARTE_FOR_EACH_INSTANCE(INSTANCE_NEEDS_CACHE_MGMT, (+), (0), _)
#define UARTE_ANY_CACHE 1
#endif

#define IS_LOW_POWER(unused, prefix, i, _) IS_ENABLED(CONFIG_UART_##prefix##i##_NRF_ASYNC_LOW_POWER)

#if UARTE_FOR_EACH_INSTANCE(IS_LOW_POWER, (||), (0))
#define UARTE_ANY_LOW_POWER 1
#endif

#ifdef UARTE_ANY_CACHE
/* uart120 instance does not retain BAUDRATE register when ENABLE=0. When this instance
 * is used then baudrate must be set after enabling the peripheral and not before.
 * This approach works for all instances so can be generally applied when uart120 is used.
 * It is not default for all because it costs some resources. Since currently only uart120
 * needs cache, that is used to determine if workaround shall be applied.
 */
#define UARTE_BAUDRATE_RETENTION_WORKAROUND 1
#endif

/*
 * RX timeout is divided into time slabs, this define tells how many divisions
 * should be made. More divisions - higher timeout accuracy and processor usage.
 */
#define RX_TIMEOUT_DIV 5

/* Size of hardware fifo in RX path. */
#define UARTE_HW_RX_FIFO_SIZE 5

#ifdef UARTE_ANY_ASYNC

struct uarte_async_tx {
	struct k_timer timer;
	const uint8_t *buf;
	volatile size_t len;
	const uint8_t *xfer_buf;
	size_t xfer_len;
	size_t cache_offset;
	volatile int amount;
	bool pending;
};

struct uarte_async_rx {
	struct k_timer timer;
#ifdef CONFIG_HAS_NORDIC_DMM
	uint8_t *usr_buf;
	uint8_t *next_usr_buf;
#endif
	uint8_t *buf;
	size_t buf_len;
	size_t offset;
	uint8_t *next_buf;
	size_t next_buf_len;
#ifdef CONFIG_UART_NRFX_UARTE_ENHANCED_RX
#if !defined(UARTE_HAS_FRAME_TIMEOUT)
	uint32_t idle_cnt;
#endif
	k_timeout_t timeout;
#else
	uint32_t total_byte_cnt; /* Total number of bytes received */
	uint32_t total_user_byte_cnt; /* Total number of bytes passed to user */
	int32_t timeout_us; /* Timeout set by user */
	int32_t timeout_slab; /* rx_timeout divided by RX_TIMEOUT_DIV */
	int32_t timeout_left; /* Current time left until user callback */
	union {
		uint8_t ppi;
		uint32_t cnt;
	} cnt;
	/* Flag to ensure that RX timeout won't be executed during ENDRX ISR */
	volatile bool is_in_irq;
#endif /* CONFIG_UART_NRFX_UARTE_ENHANCED_RX */
	uint8_t flush_cnt;
	volatile bool enabled;
	volatile bool discard_fifo;
};

struct uarte_async_cb {
	uart_callback_t user_callback;
	void *user_data;
	struct uarte_async_rx rx;
	struct uarte_async_tx tx;
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
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	struct uart_config uart_config;
#ifdef UARTE_BAUDRATE_RETENTION_WORKAROUND
	nrf_uarte_baudrate_t nrf_baudrate;
#endif
#endif
#ifdef UARTE_INTERRUPT_DRIVEN
	struct uarte_nrfx_int_driven *int_driven;
#endif
#ifdef UARTE_ANY_ASYNC
	struct uarte_async_cb *async;
#endif
	atomic_val_t poll_out_lock;
	atomic_t flags;
#ifdef UARTE_ENHANCED_POLL_OUT
	uint8_t ppi_ch_endtx;
#endif
};

#define UARTE_FLAG_LOW_POWER_TX BIT(0)
#define UARTE_FLAG_LOW_POWER_RX BIT(1)
#define UARTE_FLAG_TRIG_RXTO BIT(2)
#define UARTE_FLAG_POLL_OUT BIT(3)

/* If enabled then ENDTX is PPI'ed to TXSTOP */
#define UARTE_CFG_FLAG_PPI_ENDTX   BIT(0)

/* If enabled then TIMER and PPI is used for byte counting. */
#define UARTE_CFG_FLAG_HW_BYTE_COUNTING   BIT(1)

/* If enabled then UARTE peripheral is disabled when not used. This allows
 * to achieve lowest power consumption in idle.
 */
#define UARTE_CFG_FLAG_LOW_POWER   BIT(2)

/* If enabled then UARTE peripheral is using memory which is cacheable. */
#define UARTE_CFG_FLAG_CACHEABLE BIT(3)

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

#define LOW_POWER_ENABLED(_config) \
	(IS_ENABLED(UARTE_ANY_LOW_POWER) && \
	 !IS_ENABLED(CONFIG_PM_DEVICE) && \
	 (_config->flags & UARTE_CFG_FLAG_LOW_POWER))
/**
 * @brief Structure for UARTE configuration.
 */
struct uarte_nrfx_config {
	NRF_UARTE_Type *uarte_regs; /* Instance address */
	uint32_t flags;
	bool disable_rx;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_HAS_NORDIC_DMM
	void *mem_reg;
#endif
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	/* None-zero in case of high speed instances. Baudrate is adjusted by that ratio. */
	uint32_t clock_freq;
#else
#ifdef UARTE_HAS_FRAME_TIMEOUT
	uint32_t baudrate;
#endif
	nrf_uarte_baudrate_t nrf_baudrate;
	nrf_uarte_config_t hw_config;
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef UARTE_ANY_ASYNC
	nrfx_timer_t timer;
	uint8_t *tx_cache;
	uint8_t *rx_flush_buf;
#endif
	uint8_t *poll_out_byte;
	uint8_t *poll_in_byte;
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
	struct uarte_nrfx_data *data = dev->data;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	/* If interrupt driven and asynchronous APIs are disabled then UART
	 * interrupt is still called to stop TX. Unless it is done using PPI.
	 */
	if (!IS_ENABLED(UARTE_HAS_ENDTX_STOPTX_SHORT) &&
	    nrf_uarte_int_enable_check(uarte, NRF_UARTE_INT_ENDTX_MASK) &&
		nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX)) {
		endtx_isr(dev);
	}

	bool txstopped = nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED);

	if (txstopped && (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME) || LOW_POWER_ENABLED(config))) {
		unsigned int key = irq_lock();

		if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME) &&
		    (data->flags & UARTE_FLAG_POLL_OUT)) {
			data->flags &= ~UARTE_FLAG_POLL_OUT;
			pm_device_runtime_put(dev);
		} else {
			nrf_uarte_disable(uarte);
		}

#ifdef UARTE_INTERRUPT_DRIVEN
		if (!data->int_driven || data->int_driven->fifo_fill_lock == 0)
#endif
		{
			nrf_uarte_int_disable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);
		}

		irq_unlock(key);
	}

#ifdef UARTE_INTERRUPT_DRIVEN
	if (!data->int_driven) {
		return;
	}

	if (txstopped) {
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

	if (nrf_baudrate == 0) {
		return -EINVAL;
	}

	/* scale baudrate setting */
	if (config->clock_freq > 0U) {
		nrf_baudrate /= config->clock_freq / NRF_UARTE_BASE_FREQUENCY_16MHZ;
	}

#ifdef UARTE_BAUDRATE_RETENTION_WORKAROUND
	struct uarte_nrfx_data *data = dev->data;

	data->nrf_baudrate = nrf_baudrate;
#else
	nrf_uarte_baudrate_set(get_uarte_instance(dev), nrf_baudrate);
#endif

	return 0;
}

static int uarte_nrfx_configure(const struct device *dev,
				const struct uart_config *cfg)
{
	struct uarte_nrfx_data *data = dev->data;
	nrf_uarte_config_t uarte_cfg;

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

#ifdef UARTE_HAS_FRAME_TIMEOUT
	uarte_cfg.frame_timeout = NRF_UARTE_FRAME_TIMEOUT_EN;
#endif
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
	bool ppi_endtx = config->flags & UARTE_CFG_FLAG_PPI_ENDTX ||
			 IS_ENABLED(UARTE_HAS_ENDTX_STOPTX_SHORT);

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

/* Using Macro instead of static inline function to handle NO_OPTIMIZATIONS case
 * where static inline fails on linking.
 */
#define HW_RX_COUNTING_ENABLED(config)    \
	(IS_ENABLED(UARTE_ANY_HW_ASYNC) ? \
	 (config->flags & UARTE_CFG_FLAG_HW_BYTE_COUNTING) : false)

static void uarte_periph_enable(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	const struct uarte_nrfx_config *config = dev->config;
	struct uarte_nrfx_data *data = dev->data;

	(void)data;
	nrf_uarte_enable(uarte);
#if UARTE_BAUDRATE_RETENTION_WORKAROUND
	nrf_uarte_baudrate_set(uarte,
		COND_CODE_1(CONFIG_UART_USE_RUNTIME_CONFIGURE,
			(data->nrf_baudrate), (config->nrf_baudrate)));
#endif

#ifdef UARTE_ANY_ASYNC
	if (data->async) {
		if (HW_RX_COUNTING_ENABLED(config)) {
			const nrfx_timer_t *timer = &config->timer;

			nrfx_timer_enable(timer);

			for (int i = 0; i < data->async->rx.flush_cnt; i++) {
				nrfx_timer_increment(timer);
			}
		}
		return;
	}
#endif

	if (IS_ENABLED(UARTE_ANY_NONE_ASYNC) && !config->disable_rx) {
		nrf_uarte_rx_buffer_set(uarte, config->poll_in_byte, 1);
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);
#if defined(UARTE_INTERRUPT_DRIVEN) && defined(CONFIG_PM_DEVICE)
		if (data->int_driven && data->int_driven->rx_irq_enabled) {
			nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ENDRX_MASK);
		}
#endif
	}
}

static void uarte_enable(const struct device *dev, uint32_t act_mask, uint32_t sec_mask)
{
	struct uarte_nrfx_data *data = dev->data;

	if (atomic_or(&data->flags, act_mask) & sec_mask) {
		/* Second direction already enabled so UARTE is enabled. */
		return;
	}

	uarte_periph_enable(dev);
}

/* At this point we should have irq locked and any previous transfer completed.
 * Transfer can be started, no need to wait for completion.
 */
static void tx_start(const struct device *dev, const uint8_t *buf, size_t len)
{
	const struct uarte_nrfx_config *config = dev->config;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

#if defined(CONFIG_PM_DEVICE) && !defined(CONFIG_PM_DEVICE_RUNTIME)
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		return;
	}
#endif

	if (IS_ENABLED(UARTE_ANY_CACHE) && (config->flags & UARTE_CFG_FLAG_CACHEABLE)) {
		sys_cache_data_flush_range((void *)buf, len);
	}

	nrf_uarte_tx_buffer_set(uarte, buf, len);
	if (!IS_ENABLED(UARTE_HAS_ENDTX_STOPTX_SHORT)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);
	}
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_TXSTOPPED);

	if (LOW_POWER_ENABLED(config)) {
		uarte_enable(dev, UARTE_FLAG_LOW_POWER_TX, UARTE_FLAG_LOW_POWER_RX);
	}

	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTTX);
}

#if defined(UARTE_ANY_ASYNC)
/** @brief Disable UARTE peripheral is not used by RX or TX.
 *
 * It must be called with interrupts locked so that deciding if no direction is
 * using the UARTE is atomically performed with UARTE peripheral disabling. Otherwise
 * it would be possible that after clearing flags we get preempted and UARTE is
 * enabled from the higher priority context and when we come back UARTE is disabled
 * here.
 * @param dev Device.
 * @param dis_mask Mask of direction (RX or TX) which now longer uses the UARTE instance.
 * @param sec_mask Mask of second direction which is used to check if it uses the UARTE.
 */
static void uarte_disable_locked(const struct device *dev, uint32_t dis_mask, uint32_t sec_mask)
{
	struct uarte_nrfx_data *data = dev->data;

	data->flags &= ~dis_mask;
	if (data->flags & sec_mask) {
		return;
	}

#if !defined(CONFIG_UART_NRFX_UARTE_ENHANCED_RX)
	const struct uarte_nrfx_config *config = dev->config;

	if (data->async && HW_RX_COUNTING_ENABLED(config)) {
		nrfx_timer_disable(&config->timer);
		/* Timer/counter value is reset when disabled. */
		data->async->rx.total_byte_cnt = 0;
		data->async->rx.total_user_byte_cnt = 0;
	}
#endif

	nrf_uarte_disable(get_uarte_instance(dev));
}
#endif

#ifdef UARTE_ANY_ASYNC

static void rx_timeout(struct k_timer *timer);
static void tx_timeout(struct k_timer *timer);

#if !defined(CONFIG_UART_NRFX_UARTE_ENHANCED_RX)
static void timer_handler(nrf_timer_event_t event_type, void *p_context) { }

static int uarte_nrfx_rx_counting_init(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;
	const struct uarte_nrfx_config *cfg = dev->config;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	int ret;

	if (HW_RX_COUNTING_ENABLED(cfg)) {
		nrfx_timer_config_t tmr_config = NRFX_TIMER_DEFAULT_CONFIG(
						NRF_TIMER_BASE_FREQUENCY_GET(cfg->timer.p_reg));
		uint32_t evt_addr = nrf_uarte_event_address_get(uarte, NRF_UARTE_EVENT_RXDRDY);
		uint32_t tsk_addr = nrfx_timer_task_address_get(&cfg->timer, NRF_TIMER_TASK_COUNT);

		tmr_config.mode = NRF_TIMER_MODE_COUNTER;
		tmr_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
		ret = nrfx_timer_init(&cfg->timer,
				      &tmr_config,
				      timer_handler);
		if (ret != NRFX_SUCCESS) {
			LOG_ERR("Timer already initialized");
			return -EINVAL;
		} else {
			nrfx_timer_clear(&cfg->timer);
		}

		ret = nrfx_gppi_channel_alloc(&data->async->rx.cnt.ppi);
		if (ret != NRFX_SUCCESS) {
			LOG_ERR("Failed to allocate PPI Channel");
			nrfx_timer_uninit(&cfg->timer);
			return -EINVAL;
		}

		nrfx_gppi_channel_endpoints_setup(data->async->rx.cnt.ppi, evt_addr, tsk_addr);
		nrfx_gppi_channels_enable(BIT(data->async->rx.cnt.ppi));
	} else {
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_RXDRDY_MASK);
	}

	return 0;
}
#endif /* !defined(CONFIG_UART_NRFX_UARTE_ENHANCED_RX) */

static int uarte_async_init(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	static const uint32_t rx_int_mask =
		NRF_UARTE_INT_ENDRX_MASK |
		NRF_UARTE_INT_RXSTARTED_MASK |
		NRF_UARTE_INT_ERROR_MASK |
		NRF_UARTE_INT_RXTO_MASK |
		((IS_ENABLED(CONFIG_UART_NRFX_UARTE_ENHANCED_RX) &&
		  !IS_ENABLED(UARTE_HAS_FRAME_TIMEOUT)) ? NRF_UARTE_INT_RXDRDY_MASK : 0);

#if !defined(CONFIG_UART_NRFX_UARTE_ENHANCED_RX)
	int ret = uarte_nrfx_rx_counting_init(dev);

	if (ret != 0) {
		return ret;
	}
#endif

	nrf_uarte_int_enable(uarte, rx_int_mask);

	k_timer_init(&data->async->rx.timer, rx_timeout, NULL);
	k_timer_user_data_set(&data->async->rx.timer, (void *)dev);
	k_timer_init(&data->async->tx.timer, tx_timeout, NULL);
	k_timer_user_data_set(&data->async->tx.timer, (void *)dev);

	return 0;
}

/* Attempt to start TX (asynchronous transfer). If hardware is not ready, then pending
 * flag is set. When current poll_out is completed, pending transfer is started.
 * Function must be called with interrupts locked.
 */
static void start_tx_locked(const struct device *dev, struct uarte_nrfx_data *data)
{
	nrf_uarte_int_enable(get_uarte_instance(dev), NRF_UARTE_INT_TXSTOPPED_MASK);
	if (!is_tx_ready(dev)) {
		/* Active poll out, postpone until it is completed. */
		data->async->tx.pending = true;
	} else {
		data->async->tx.pending = false;
		data->async->tx.amount = -1;
		tx_start(dev, data->async->tx.xfer_buf, data->async->tx.xfer_len);
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
	size_t remaining = data->async->tx.len - data->async->tx.cache_offset;

	if (!remaining) {
		return false;
	}

	size_t len = MIN(remaining, CONFIG_UART_ASYNC_TX_CACHE_SIZE);

	data->async->tx.xfer_len = len;
	data->async->tx.xfer_buf = config->tx_cache;
	memcpy(config->tx_cache, &data->async->tx.buf[data->async->tx.cache_offset], len);

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

	if (data->async->tx.len) {
		irq_unlock(key);
		return -EBUSY;
	}

	data->async->tx.len = len;
	data->async->tx.buf = buf;

	if (nrf_dma_accessible_check(uarte, buf)) {
		data->async->tx.xfer_buf = buf;
		data->async->tx.xfer_len = len;
	} else {
		data->async->tx.cache_offset = 0;
		(void)setup_tx_cache(dev);
	}

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(dev);
	}

	start_tx_locked(dev, data);

	irq_unlock(key);

	if (has_hwfc(dev) && timeout != SYS_FOREVER_US) {
		k_timer_start(&data->async->tx.timer, K_USEC(timeout), K_NO_WAIT);
	}
	return 0;
}

static int uarte_nrfx_tx_abort(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (data->async->tx.buf == NULL) {
		return -EFAULT;
	}

	data->async->tx.pending = false;
	k_timer_stop(&data->async->tx.timer);
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
		.data.rx.buf = data->async->rx.buf,
		.data.rx.len = len,
		.data.rx.offset = data->async->rx.offset
	};

	user_callback(dev, &evt);
}

static void rx_buf_release(const struct device *dev, uint8_t *buf)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = buf,
	};

	user_callback(dev, &evt);
}

static void notify_rx_disable(const struct device *dev)
{
	struct uart_event evt = {
		.type = UART_RX_DISABLED,
	};

	user_callback(dev, (struct uart_event *)&evt);
}

#ifdef UARTE_HAS_FRAME_TIMEOUT
static uint32_t us_to_bauds(uint32_t baudrate, int32_t timeout)
{
	uint64_t bauds = (uint64_t)baudrate * timeout / 1000000;

	return MIN((uint32_t)bauds, UARTE_FRAMETIMEOUT_COUNTERTOP_Msk);
}
#endif

static int uarte_nrfx_rx_enable(const struct device *dev, uint8_t *buf,
				size_t len,
				int32_t timeout)
{
	struct uarte_nrfx_data *data = dev->data;
	struct uarte_async_rx *async_rx = &data->async->rx;
	const struct uarte_nrfx_config *cfg = dev->config;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (cfg->disable_rx) {
		__ASSERT(false, "TX only UARTE instance");
		return -ENOTSUP;
	}

	/* Signal error if RX is already enabled or if the driver is waiting
	 * for the RXTO event after a call to uart_rx_disable() to discard
	 * data from the UARTE internal RX FIFO.
	 */
	if (async_rx->enabled || async_rx->discard_fifo) {
		return -EBUSY;
	}

#ifdef CONFIG_HAS_NORDIC_DMM
	uint8_t *dma_buf;
	int ret = 0;

	ret = dmm_buffer_in_prepare(cfg->mem_reg, buf, len, (void **)&dma_buf);
	if (ret < 0) {
		return ret;
	}

	async_rx->usr_buf = buf;
	buf = dma_buf;
#endif

#ifdef CONFIG_UART_NRFX_UARTE_ENHANCED_RX
#ifdef UARTE_HAS_FRAME_TIMEOUT
	if (timeout != SYS_FOREVER_US) {
		uint32_t baudrate = COND_CODE_1(CONFIG_UART_USE_RUNTIME_CONFIGURE,
			(data->uart_config.baudrate), (cfg->baudrate));

		async_rx->timeout = K_USEC(timeout);
		nrf_uarte_frame_timeout_set(uarte, us_to_bauds(baudrate, timeout));
		nrf_uarte_shorts_enable(uarte, NRF_UARTE_SHORT_FRAME_TIMEOUT_STOPRX);
	} else {
		async_rx->timeout = K_NO_WAIT;
	}
#else
	async_rx->timeout = (timeout == SYS_FOREVER_US) ?
		K_NO_WAIT : K_USEC(timeout / RX_TIMEOUT_DIV);
	async_rx->idle_cnt = 0;
#endif /* UARTE_HAS_FRAME_TIMEOUT */
#else
	async_rx->timeout_us = timeout;
	async_rx->timeout_slab = timeout / RX_TIMEOUT_DIV;
#endif

	async_rx->buf = buf;
	async_rx->buf_len = len;
	async_rx->offset = 0;
	async_rx->next_buf = NULL;
	async_rx->next_buf_len = 0;

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME) || LOW_POWER_ENABLED(cfg)) {
		if (async_rx->flush_cnt) {
			int cpy_len = MIN(len, async_rx->flush_cnt);

			if (IS_ENABLED(UARTE_ANY_CACHE) &&
			    (cfg->flags & UARTE_CFG_FLAG_CACHEABLE)) {
				sys_cache_data_invd_range(cfg->rx_flush_buf, cpy_len);
			}

			memcpy(buf, cfg->rx_flush_buf, cpy_len);

			if (IS_ENABLED(UARTE_ANY_CACHE) &&
			    (cfg->flags & UARTE_CFG_FLAG_CACHEABLE)) {
				sys_cache_data_flush_range(buf, cpy_len);
			}

			buf += cpy_len;
			len -= cpy_len;

			/* If flush content filled whole new buffer trigger interrupt
			 * to notify about received data and disabled RX from there.
			 */
			if (!len) {
				async_rx->flush_cnt -= cpy_len;
				memmove(cfg->rx_flush_buf, &cfg->rx_flush_buf[cpy_len],
						async_rx->flush_cnt);
				if (IS_ENABLED(UARTE_ANY_CACHE) &&
				    (cfg->flags & UARTE_CFG_FLAG_CACHEABLE)) {
					sys_cache_data_flush_range(cfg->rx_flush_buf,
								   async_rx->flush_cnt);
				}
				atomic_or(&data->flags, UARTE_FLAG_TRIG_RXTO);
				NRFX_IRQ_PENDING_SET(nrfx_get_irq_number(uarte));
				return 0;
			} else {
#ifdef CONFIG_UART_NRFX_UARTE_ENHANCED_RX
				if (!K_TIMEOUT_EQ(async_rx->timeout, K_NO_WAIT)) {
					nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXDRDY);
					k_timer_start(&async_rx->timer, async_rx->timeout,
							K_NO_WAIT);
				}
#endif
			}
		}
	}

	nrf_uarte_rx_buffer_set(uarte, buf, len);

	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);

	async_rx->enabled = true;

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(dev);
	} else if (LOW_POWER_ENABLED(cfg)) {
		unsigned int key = irq_lock();

		uarte_enable(dev, UARTE_FLAG_LOW_POWER_RX, UARTE_FLAG_LOW_POWER_TX);
		irq_unlock(key);
	}

	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);

	return 0;
}

static int uarte_nrfx_rx_buf_rsp(const struct device *dev, uint8_t *buf,
				 size_t len)
{
	struct uarte_nrfx_data *data = dev->data;
	struct uarte_async_rx *async_rx = &data->async->rx;
	int err;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	unsigned int key = irq_lock();

	if (async_rx->buf == NULL) {
		err = -EACCES;
	} else if (async_rx->next_buf == NULL) {
#ifdef CONFIG_HAS_NORDIC_DMM
		uint8_t *dma_buf;
		const struct uarte_nrfx_config *config = dev->config;

		err = dmm_buffer_in_prepare(config->mem_reg, buf, len, (void **)&dma_buf);
		if (err < 0) {
			return err;
		}
		async_rx->next_usr_buf = buf;
		buf = dma_buf;
#endif
		async_rx->next_buf = buf;
		async_rx->next_buf_len = len;
		nrf_uarte_rx_buffer_set(uarte, buf, len);
		/* If buffer is shorter than RX FIFO then there is a risk that due
		 * to interrupt handling latency ENDRX event is not handled on time
		 * and due to ENDRX_STARTRX short data will start to be overwritten.
		 * In that case short is not enabled and ENDRX event handler will
		 * manually start RX for that buffer. Thanks to RX FIFO there is
		 * 5 byte time for doing that. If interrupt latency is higher and
		 * there is no HWFC in both cases data will be lost or corrupted.
		 */
		if (len >= UARTE_HW_RX_FIFO_SIZE) {
			nrf_uarte_shorts_enable(uarte, NRF_UARTE_SHORT_ENDRX_STARTRX);
		}
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
	struct uarte_async_rx *async_rx = &data->async->rx;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	int key;

	if (async_rx->buf == NULL) {
		return -EFAULT;
	}

	k_timer_stop(&async_rx->timer);

	key = irq_lock();

	if (async_rx->next_buf != NULL) {
		nrf_uarte_shorts_disable(uarte, NRF_UARTE_SHORT_ENDRX_STARTRX);
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);
	}

	async_rx->enabled = false;
	async_rx->discard_fifo = true;

	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPRX);
	irq_unlock(key);

	return 0;
}

static void tx_timeout(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);
	(void) uarte_nrfx_tx_abort(dev);
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
	const struct device *dev = k_timer_user_data_get(timer);

#if  CONFIG_UART_NRFX_UARTE_ENHANCED_RX
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

#ifdef UARTE_HAS_FRAME_TIMEOUT
	if (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXDRDY)) {
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPRX);
	}
	return;
#else /* UARTE_HAS_FRAME_TIMEOUT */
	struct uarte_nrfx_data *data = dev->data;
	struct uarte_async_rx *async_rx = &data->async->rx;

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXDRDY)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXDRDY);
		async_rx->idle_cnt = 0;
	} else {
		async_rx->idle_cnt++;
		/* We compare against RX_TIMEOUT_DIV - 1 to get rather earlier timeout
		 * than late. idle_cnt is reset when last RX activity (RXDRDY event) is
		 * detected. It may happen that it happens when RX is inactive for whole
		 * RX timeout period (and it is the case when transmission is short compared
		 * to the timeout, for example timeout is 50 ms and transmission of few bytes
		 * takes less than 1ms). In that case if we compare against RX_TIMEOUT_DIV
		 * then RX notification would come after (RX_TIMEOUT_DIV + 1) * timeout.
		 */
		if (async_rx->idle_cnt == (RX_TIMEOUT_DIV - 1)) {
			nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPRX);
			return;
		}
	}

	k_timer_start(&async_rx->timer, async_rx->timeout, K_NO_WAIT);
#endif /* UARTE_HAS_FRAME_TIMEOUT */
#else /* CONFIG_UART_NRFX_UARTE_ENHANCED_RX */
	const struct uarte_nrfx_config *cfg = dev->config;
	struct uarte_nrfx_data *data = dev->data;
	struct uarte_async_rx *async_rx = &data->async->rx;
	uint32_t read;

	if (async_rx->is_in_irq) {
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
		read = async_rx->cnt.cnt;
	}

	/* Check if data was received since last function call */
	if (read != async_rx->total_byte_cnt) {
		async_rx->total_byte_cnt = read;
		async_rx->timeout_left = async_rx->timeout_us;
	}

	/* Check if there is data that was not sent to user yet
	 * Note though that 'len' is a count of data bytes received, but not
	 * necessarily the amount available in the current buffer
	 */
	int32_t len = async_rx->total_byte_cnt - async_rx->total_user_byte_cnt;

	if (!HW_RX_COUNTING_ENABLED(cfg) &&
	    (len < 0)) {
		/* Prevent too low value of rx_cnt.cnt which may occur due to
		 * latencies in handling of the RXRDY interrupt.
		 * At this point, the number of received bytes is at least
		 * equal to what was reported to the user.
		 */
		async_rx->cnt.cnt = async_rx->total_user_byte_cnt;
		len = 0;
	}

	/* Check for current buffer being full.
	 * if the UART receives characters before the ENDRX is handled
	 * and the 'next' buffer is set up, then the SHORT between ENDRX and
	 * STARTRX will mean that data will be going into to the 'next' buffer
	 * until the ENDRX event gets a chance to be handled.
	 */
	bool clipped = false;

	if (len + async_rx->offset > async_rx->buf_len) {
		len = async_rx->buf_len - async_rx->offset;
		clipped = true;
	}

	if (len > 0) {
		if (clipped || (async_rx->timeout_left < async_rx->timeout_slab)) {
			/* rx_timeout us elapsed since last receiving */
			if (async_rx->buf != NULL) {
				notify_uart_rx_rdy(dev, len);
				async_rx->offset += len;
				async_rx->total_user_byte_cnt += len;
			}
		} else {
			async_rx->timeout_left -= async_rx->timeout_slab;
		}

		/* If there's nothing left to report until the buffers are
		 * switched then the timer can be stopped
		 */
		if (clipped) {
			k_timer_stop(&async_rx->timer);
		}
	}

	nrf_uarte_int_enable(get_uarte_instance(dev),
			     NRF_UARTE_INT_ENDRX_MASK);
#endif /* CONFIG_UART_NRFX_UARTE_ENHANCED_RX */
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
	uint32_t err = nrf_uarte_errorsrc_get(uarte);
	struct uart_event evt = {
		.type = UART_RX_STOPPED,
		.data.rx_stop.reason = UARTE_ERROR_FROM_MASK(err),
	};

	/* For VPR cores read and write may be reordered - barrier needed. */
	nrf_barrier_r();
	nrf_uarte_errorsrc_clear(uarte, err);

	user_callback(dev, &evt);
	(void) uarte_nrfx_rx_disable(dev);
}

static void rxstarted_isr(const struct device *dev)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

#ifndef UARTE_HAS_FRAME_TIMEOUT
	struct uarte_nrfx_data *data = dev->data;
	struct uarte_async_rx *async_rx = &data->async->rx;

#ifdef CONFIG_UART_NRFX_UARTE_ENHANCED_RX
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (!K_TIMEOUT_EQ(async_rx->timeout, K_NO_WAIT)) {
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_RXDRDY_MASK);
	}
#else
	if (async_rx->timeout_us != SYS_FOREVER_US) {
		k_timeout_t timeout = K_USEC(async_rx->timeout_slab);

		async_rx->timeout_left = async_rx->timeout_us;
		k_timer_start(&async_rx->timer, timeout, timeout);
	}
#endif /* CONFIG_UART_NRFX_UARTE_ENHANCED_RX */
#endif /* !UARTE_HAS_FRAME_TIMEOUT */
	user_callback(dev, &evt);
}

static void endrx_isr(const struct device *dev)
{
	struct uarte_nrfx_data *data = dev->data;
	struct uarte_async_rx *async_rx = &data->async->rx;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

#if !defined(CONFIG_UART_NRFX_UARTE_ENHANCED_RX)
	async_rx->is_in_irq = true;
#endif

	/* ensure rx timer is stopped - it will be restarted in RXSTARTED
	 * handler if needed
	 */
	k_timer_stop(&async_rx->timer);

	/* this is the amount that the EasyDMA controller has copied into the
	 * buffer
	 */
	const int rx_amount = nrf_uarte_rx_amount_get(uarte) + async_rx->flush_cnt;

#ifdef CONFIG_HAS_NORDIC_DMM
	const struct uarte_nrfx_config *config = dev->config;
	int err =
		dmm_buffer_in_release(config->mem_reg, async_rx->usr_buf, rx_amount, async_rx->buf);

	(void)err;
	__ASSERT_NO_MSG(err == 0);
	async_rx->buf = async_rx->usr_buf;
#endif
	async_rx->flush_cnt = 0;

	/* The 'rx_offset' can be bigger than 'rx_amount', so it the length
	 * of data we report back the user may need to be clipped.
	 * This can happen because the 'rx_offset' count derives from RXRDY
	 * events, which can occur already for the next buffer before we are
	 * here to handle this buffer. (The next buffer is now already active
	 * because of the ENDRX_STARTRX shortcut)
	 */
	int rx_len = rx_amount - async_rx->offset;

	if (rx_len < 0) {
		rx_len = 0;
	}

#if !defined(CONFIG_UART_NRFX_UARTE_ENHANCED_RX)
	async_rx->total_user_byte_cnt += rx_len;
#endif

	/* Only send the RX_RDY event if there is something to send */
	if (rx_len > 0) {
		notify_uart_rx_rdy(dev, rx_len);
	}

	rx_buf_release(dev, async_rx->buf);
	async_rx->buf = async_rx->next_buf;
	async_rx->buf_len = async_rx->next_buf_len;
#ifdef CONFIG_HAS_NORDIC_DMM
	async_rx->usr_buf = async_rx->next_usr_buf;
#endif
	async_rx->next_buf = NULL;
	async_rx->next_buf_len = 0;
	async_rx->offset = 0;

	if (async_rx->enabled) {
		/* If there is a next buffer, then STARTRX will have already been
		 * invoked by the short (the next buffer will be filling up already)
		 * and here we just do the swap of which buffer the driver is following,
		 * the next rx_timeout() will update the rx_offset.
		 */
		unsigned int key = irq_lock();

		if (async_rx->buf) {
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
	}

#if !defined(CONFIG_UART_NRFX_UARTE_ENHANCED_RX)
	async_rx->is_in_irq = false;
#endif
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

static uint8_t rx_flush(const struct device *dev, uint8_t *buf)
{
	/* Flushing RX fifo requires buffer bigger than 4 bytes to empty fifo*/
	static const uint8_t dirty = CONFIG_UART_NRFX_UARTE_RX_FLUSH_MAGIC_BYTE;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	const struct uarte_nrfx_config *config = dev->config;
	uint32_t prev_rx_amount;
	uint32_t rx_amount;

	if (IS_ENABLED(RX_FLUSH_WORKAROUND)) {
		memset(buf, dirty, UARTE_HW_RX_FIFO_SIZE);
		if (IS_ENABLED(UARTE_ANY_CACHE) && (config->flags & UARTE_CFG_FLAG_CACHEABLE)) {
			sys_cache_data_flush_range(buf, UARTE_HW_RX_FIFO_SIZE);
		}
		prev_rx_amount = nrf_uarte_rx_amount_get(uarte);
	} else {
		prev_rx_amount = 0;
	}

	nrf_uarte_rx_buffer_set(uarte, buf, UARTE_HW_RX_FIFO_SIZE);
	/* Final part of handling RXTO event is in ENDRX interrupt
	 * handler. ENDRX is generated as a result of FLUSHRX task.
	 */
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_FLUSHRX);
	while (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		/* empty */
	}
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);

	rx_amount = nrf_uarte_rx_amount_get(uarte);
	if (!buf || !IS_ENABLED(RX_FLUSH_WORKAROUND)) {
		return rx_amount;
	}

	if (rx_amount != prev_rx_amount) {
		return rx_amount;
	}

	if (rx_amount > UARTE_HW_RX_FIFO_SIZE) {
		return 0;
	}

	if (IS_ENABLED(UARTE_ANY_CACHE) && (config->flags & UARTE_CFG_FLAG_CACHEABLE)) {
		sys_cache_data_invd_range(buf, UARTE_HW_RX_FIFO_SIZE);
	}

	for (int i = 0; i < rx_amount; i++) {
		if (buf[i] != dirty) {
			return rx_amount;
		}
	}

	return 0;
}

/* This handler is called when the receiver is stopped. If rx was aborted
 * data from fifo is flushed.
 */
static void rxto_isr(const struct device *dev)
{
	const struct uarte_nrfx_config *config = dev->config;
	struct uarte_nrfx_data *data = dev->data;
	struct uarte_async_rx *async_rx = &data->async->rx;

	if (async_rx->buf) {
#ifdef CONFIG_HAS_NORDIC_DMM
		(void)dmm_buffer_in_release(config->mem_reg, async_rx->usr_buf, 0, async_rx->buf);
		async_rx->buf = async_rx->usr_buf;
#endif
		rx_buf_release(dev, async_rx->buf);
		async_rx->buf = NULL;
	}

	/* This point can be reached in two cases:
	 * 1. RX is disabled because all provided RX buffers have been filled.
	 * 2. RX was explicitly disabled by a call to uart_rx_disable().
	 * In both cases, the rx_enabled flag is cleared, so that RX can be
	 * enabled again.
	 * In the second case, additionally, data from the UARTE internal RX
	 * FIFO need to be discarded.
	 */
	async_rx->enabled = false;
	if (async_rx->discard_fifo) {
		async_rx->discard_fifo = false;
#if !defined(CONFIG_UART_NRFX_UARTE_ENHANCED_RX)
		if (HW_RX_COUNTING_ENABLED(config)) {
			uint8_t buf[UARTE_HW_RX_FIFO_SIZE];

			/* It need to be included because TIMER+PPI got RXDRDY events
			 * and counted those flushed bytes.
			 */
			async_rx->total_user_byte_cnt += rx_flush(dev, buf);
			(void)buf;
		}
#endif
	} else {
		async_rx->flush_cnt = rx_flush(dev, config->rx_flush_buf);
	}

#ifdef CONFIG_UART_NRFX_UARTE_ENHANCED_RX
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
#ifdef UARTE_HAS_FRAME_TIMEOUT
	nrf_uarte_shorts_disable(uarte, NRF_UARTE_SHORT_FRAME_TIMEOUT_STOPRX);
#endif
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXDRDY);
#endif

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(dev);
	} else if (LOW_POWER_ENABLED(config)) {
		uint32_t key = irq_lock();

		uarte_disable_locked(dev, UARTE_FLAG_LOW_POWER_RX, UARTE_FLAG_LOW_POWER_TX);
		irq_unlock(key);
	}

	notify_rx_disable(dev);
}

static void txstopped_isr(const struct device *dev)
{
	const struct uarte_nrfx_config *config = dev->config;
	struct uarte_nrfx_data *data = dev->data;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	unsigned int key;

	key = irq_lock();

	size_t amount = (data->async->tx.amount >= 0) ?
			data->async->tx.amount : nrf_uarte_tx_amount_get(uarte);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		nrf_uarte_int_disable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);
		if (data->flags & UARTE_FLAG_POLL_OUT) {
			pm_device_runtime_put(dev);
			data->flags &= ~UARTE_FLAG_POLL_OUT;
		}
	} else if (LOW_POWER_ENABLED(config)) {
		nrf_uarte_int_disable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);
		uarte_disable_locked(dev, UARTE_FLAG_LOW_POWER_TX, UARTE_FLAG_LOW_POWER_RX);
	}

	irq_unlock(key);

	if (!data->async->tx.buf) {
		return;
	}

	/* If there is a pending tx request, it means that uart_tx()
	 * was called when there was ongoing uart_poll_out. Handling
	 * TXSTOPPED interrupt means that uart_poll_out has completed.
	 */
	if (data->async->tx.pending) {
		key = irq_lock();
		start_tx_locked(dev, data);
		irq_unlock(key);
		return;
	}

	/* Cache buffer is used because tx_buf wasn't in RAM. */
	if (data->async->tx.buf != data->async->tx.xfer_buf) {
		/* In that case setup next chunk. If that was the last chunk
		 * fall back to reporting TX_DONE.
		 */
		if (amount == data->async->tx.xfer_len) {
			data->async->tx.cache_offset += amount;
			if (setup_tx_cache(dev)) {
				key = irq_lock();
				start_tx_locked(dev, data);
				irq_unlock(key);
				return;
			}

			/* Amount is already included in cache_offset. */
			amount = data->async->tx.cache_offset;
		} else {
			/* TX was aborted, include cache_offset in amount. */
			amount += data->async->tx.cache_offset;
		}
	}

	k_timer_stop(&data->async->tx.timer);

	struct uart_event evt = {
		.data.tx.buf = data->async->tx.buf,
		.data.tx.len = amount,
	};
	if (amount == data->async->tx.len) {
		evt.type = UART_TX_DONE;
	} else {
		evt.type = UART_TX_ABORTED;
	}

	nrf_uarte_int_disable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);
	data->async->tx.buf = NULL;
	data->async->tx.len = 0;

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(dev);
	}

	user_callback(dev, &evt);
}

static void rxdrdy_isr(const struct device *dev)
{
#if !defined(UARTE_HAS_FRAME_TIMEOUT)
	struct uarte_nrfx_data *data = dev->data;

#if defined(CONFIG_UART_NRFX_UARTE_ENHANCED_RX)
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	data->async->rx.idle_cnt = 0;
	k_timer_start(&data->async->rx.timer, data->async->rx.timeout, K_NO_WAIT);
	nrf_uarte_int_disable(uarte, NRF_UARTE_INT_RXDRDY_MASK);
#else
	data->async->rx.cnt.cnt++;
#endif
#endif /* !UARTE_HAS_FRAME_TIMEOUT */
}

static bool event_check_clear(NRF_UARTE_Type *uarte, nrf_uarte_event_t event,
				uint32_t int_mask, uint32_t int_en_mask)
{
	if (nrf_uarte_event_check(uarte, event) && (int_mask & int_en_mask)) {
		nrf_uarte_event_clear(uarte, event);
		return true;
	}

	return false;
}

static void uarte_nrfx_isr_async(const void *arg)
{
	const struct device *dev = arg;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	const struct uarte_nrfx_config *config = dev->config;
	struct uarte_nrfx_data *data = dev->data;
	struct uarte_async_rx *async_rx = &data->async->rx;
	uint32_t imask = nrf_uarte_int_enable_check(uarte, UINT32_MAX);

	if (!(HW_RX_COUNTING_ENABLED(config) || IS_ENABLED(UARTE_HAS_FRAME_TIMEOUT))
	    && event_check_clear(uarte, NRF_UARTE_EVENT_RXDRDY, NRF_UARTE_INT_RXDRDY_MASK, imask)) {
		rxdrdy_isr(dev);

	}

	if (event_check_clear(uarte, NRF_UARTE_EVENT_ERROR, NRF_UARTE_INT_ERROR_MASK, imask)) {
		error_isr(dev);
	}

	if (event_check_clear(uarte, NRF_UARTE_EVENT_ENDRX, NRF_UARTE_INT_ENDRX_MASK, imask)) {
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
	if ((imask & NRF_UARTE_INT_RXSTARTED_MASK) &&
	    nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXSTARTED) &&
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
	if ((imask & NRF_UARTE_INT_RXTO_MASK) &&
	    nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXTO) &&
	    !nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXTO);
		rxto_isr(dev);
	}

	if (!IS_ENABLED(UARTE_HAS_ENDTX_STOPTX_SHORT) &&
	    (imask & NRF_UARTE_INT_ENDTX_MASK) &&
	    nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX)) {
		endtx_isr(dev);
	}

	if ((imask & NRF_UARTE_INT_TXSTOPPED_MASK) &&
	    nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED)) {
		txstopped_isr(dev);
	}

	if (atomic_and(&data->flags, ~UARTE_FLAG_TRIG_RXTO) & UARTE_FLAG_TRIG_RXTO) {
#ifdef CONFIG_HAS_NORDIC_DMM
		int ret;

		ret = dmm_buffer_in_release(config->mem_reg, async_rx->usr_buf, async_rx->buf_len,
					    async_rx->buf);

		(void)ret;
		__ASSERT_NO_MSG(ret == 0);
		async_rx->buf = async_rx->usr_buf;
#endif
		notify_uart_rx_rdy(dev, async_rx->buf_len);
		rx_buf_release(dev, async_rx->buf);
		async_rx->buf_len = 0;
		async_rx->buf = NULL;
		notify_rx_disable(dev);
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
	const struct uarte_nrfx_config *config = dev->config;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

#ifdef UARTE_ANY_ASYNC
	struct uarte_nrfx_data *data = dev->data;

	if (data->async) {
		return -ENOTSUP;
	}
#endif

	if (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		return -1;
	}

	if (IS_ENABLED(UARTE_ANY_CACHE) && (config->flags & UARTE_CFG_FLAG_CACHEABLE)) {
		sys_cache_data_invd_range(config->poll_in_byte, 1);
	}

	*c = *config->poll_in_byte;

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
	const struct uarte_nrfx_config *config = dev->config;
	bool isr_mode = k_is_in_isr() || k_is_pre_kernel();
	struct uarte_nrfx_data *data = dev->data;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	unsigned int key;

	if (isr_mode) {
		while (1) {
			key = irq_lock();
			if (is_tx_ready(dev)) {
#if UARTE_ANY_ASYNC
				if (data->async && data->async->tx.len &&
					data->async->tx.amount < 0) {
					data->async->tx.amount = nrf_uarte_tx_amount_get(uarte);
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

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		if (!(data->flags & UARTE_FLAG_POLL_OUT)) {
			data->flags |= UARTE_FLAG_POLL_OUT;
			pm_device_runtime_get(dev);
		}
	}

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME) || LOW_POWER_ENABLED(config)) {
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);
	}

	*config->poll_out_byte = c;
	tx_start(dev, config->poll_out_byte, 1);

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
	const struct uarte_nrfx_config *config = dev->config;

	if (size > 0 && nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		/* Clear the interrupt */
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);

		if (IS_ENABLED(UARTE_ANY_CACHE) && (config->flags & UARTE_CFG_FLAG_CACHEABLE)) {
			sys_cache_data_invd_range(config->poll_in_byte, 1);
		}

		/* Receive a character */
		rx_data[num_rx++] = *config->poll_in_byte;

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

	return ready ? data->int_driven->tx_buff_size : 0;
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

#ifdef UARTE_ENHANCED_POLL_OUT
static int endtx_stoptx_ppi_init(NRF_UARTE_Type *uarte,
				 struct uarte_nrfx_data *data)
{
	nrfx_err_t ret;

	ret = nrfx_gppi_channel_alloc(&data->ppi_ch_endtx);
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
#endif /* UARTE_ENHANCED_POLL_OUT */

/** @brief Pend until TX is stopped.
 *
 * There are 2 configurations that must be handled:
 * - ENDTX->TXSTOPPED PPI enabled - just pend until TXSTOPPED event is set
 * - disable ENDTX interrupt and manually trigger STOPTX, pend for TXSTOPPED
 */
static void wait_for_tx_stopped(const struct device *dev)
{
	const struct uarte_nrfx_config *config = dev->config;
	bool ppi_endtx = (config->flags & UARTE_CFG_FLAG_PPI_ENDTX) ||
			 IS_ENABLED(UARTE_HAS_ENDTX_STOPTX_SHORT);
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
			if (!IS_ENABLED(UARTE_HAS_ENDTX_STOPTX_SHORT)) {
				nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);
			}
			nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);
		}
	}

	NRFX_WAIT_FOR(nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED),
		      1000, 1, res);

	if (!ppi_endtx) {
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ENDTX_MASK);
	}
}

static void uarte_pm_resume(const struct device *dev)
{
	const struct uarte_nrfx_config *cfg = dev->config;

	(void)pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME) || !LOW_POWER_ENABLED(cfg)) {
		uarte_periph_enable(dev);
	}
}

static void uarte_pm_suspend(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	const struct uarte_nrfx_config *cfg = dev->config;
	struct uarte_nrfx_data *data = dev->data;

	(void)data;
#ifdef UARTE_ANY_ASYNC
	if (data->async) {
		/* Entering inactive state requires device to be no
		 * active asynchronous calls.
		 */
		__ASSERT_NO_MSG(!data->async->rx.enabled);
		__ASSERT_NO_MSG(!data->async->tx.len);
		if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
			/* If runtime PM is enabled then reference counting ensures that
			 * suspend will not occur when TX is active.
			 */
			__ASSERT_NO_MSG(nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED));
		} else {
			wait_for_tx_stopped(dev);
		}

#if !defined(CONFIG_UART_NRFX_UARTE_ENHANCED_RX)
		if (data->async && HW_RX_COUNTING_ENABLED(cfg)) {
			nrfx_timer_disable(&cfg->timer);
			/* Timer/counter value is reset when disabled. */
			data->async->rx.total_byte_cnt = 0;
			data->async->rx.total_user_byte_cnt = 0;
		}
#endif
	} else if (IS_ENABLED(UARTE_ANY_NONE_ASYNC))
#endif
	{
		if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXSTARTED)) {
#if defined(UARTE_INTERRUPT_DRIVEN) && defined(CONFIG_PM_DEVICE)
			if (data->int_driven) {
				data->int_driven->rx_irq_enabled =
						nrf_uarte_int_enable_check(uarte,
							NRF_UARTE_INT_ENDRX_MASK);
				if (data->int_driven->rx_irq_enabled) {
					nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ENDRX_MASK);
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
	}

	nrf_uarte_disable(uarte);

	(void)pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_SLEEP);
}

static int uarte_nrfx_pm_action(const struct device *dev, enum pm_device_action action)
{
	if (action == PM_DEVICE_ACTION_RESUME) {
		uarte_pm_resume(dev);
	} else if (IS_ENABLED(CONFIG_PM_DEVICE) && (action == PM_DEVICE_ACTION_SUSPEND)) {
		uarte_pm_suspend(dev);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int uarte_tx_path_init(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	const struct uarte_nrfx_config *cfg = dev->config;
	bool auto_endtx = false;

#ifdef UARTE_HAS_ENDTX_STOPTX_SHORT
	nrf_uarte_shorts_enable(uarte, NRF_UARTE_SHORT_ENDTX_STOPTX);
	auto_endtx = true;
#elif defined(UARTE_ENHANCED_POLL_OUT)
	if (cfg->flags & UARTE_CFG_FLAG_PPI_ENDTX) {
		struct uarte_nrfx_data *data = dev->data;
		int err;

		err = endtx_stoptx_ppi_init(uarte, data);
		if (err < 0) {
			return err;
		}
		auto_endtx = true;
	}
#endif

	/* Get to the point where TXSTOPPED event is set but TXSTOPPED interrupt is
	 * disabled. This trick is later on used to handle TX path and determine
	 * using HW if TX is active (TXSTOPPED event set means TX is inactive).
	 *
	 * Set TXSTOPPED event by requesting fake (zero-length) transfer.
	 * Pointer to RAM variable is set because otherwise such operation may
	 * result in HardFault or RAM corruption.
	 */
	nrf_uarte_enable(uarte);
	nrf_uarte_tx_buffer_set(uarte, cfg->poll_out_byte, 0);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTTX);
	if (!auto_endtx) {
		while (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX)) {
		}
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ENDTX_MASK);
	}
	while (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED)) {
	}
	nrf_uarte_disable(uarte);

	return 0;
}

static int uarte_instance_init(const struct device *dev,
			       uint8_t interrupts_active)
{
	int err;
	const struct uarte_nrfx_config *cfg = dev->config;

	if (IS_ENABLED(CONFIG_ARCH_POSIX)) {
		/* For simulation the DT provided peripheral address needs to be corrected */
		((struct pinctrl_dev_config *)cfg->pcfg)->reg = (uintptr_t)cfg->uarte_regs;
	}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	err = uarte_nrfx_configure(dev, &((struct uarte_nrfx_data *)dev->data)->uart_config);
	if (err) {
		return err;
	}
#else
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_baudrate_set(uarte, cfg->nrf_baudrate);
	nrf_uarte_configure(uarte, &cfg->hw_config);
#endif

#ifdef UARTE_ANY_ASYNC
	struct uarte_nrfx_data *data = dev->data;

	if (data->async) {
		err = uarte_async_init(dev);
		if (err < 0) {
			return err;
		}
	}
#endif

	err = uarte_tx_path_init(dev);
	if (err) {
		return err;
	}

	return pm_device_driver_init(dev, uarte_nrfx_pm_action);
}

#define UARTE_IRQ_CONFIGURE(idx, isr_handler)				       \
	do {								       \
		IRQ_CONNECT(DT_IRQN(UARTE(idx)), DT_IRQ(UARTE(idx), priority), \
			    isr_handler, DEVICE_DT_GET(UARTE(idx)), 0);	       \
		irq_enable(DT_IRQN(UARTE(idx)));			       \
	} while (false)

/* Low power mode is used when disable_rx is not defined or in async mode if
 * kconfig option is enabled.
 */
#define USE_LOW_POWER(idx)						       \
	COND_CODE_1(CONFIG_PM_DEVICE, (0),				       \
		(((!UARTE_PROP(idx, disable_rx) &&			       \
		COND_CODE_1(CONFIG_UART_##idx##_ASYNC,			       \
			(!IS_ENABLED(CONFIG_UART_##idx##_NRF_ASYNC_LOW_POWER)),\
			(1))) ? 0 : UARTE_CFG_FLAG_LOW_POWER)))

#define UARTE_DISABLE_RX_INIT(node_id) \
	.disable_rx = DT_PROP(node_id, disable_rx)

#define UARTE_GET_FREQ(idx) DT_PROP(DT_CLOCKS_CTLR(UARTE(idx)), clock_frequency)

#define UARTE_GET_BAUDRATE_DIV(idx)						\
	COND_CODE_1(DT_CLOCKS_HAS_IDX(UARTE(idx), 0),				\
		   ((UARTE_GET_FREQ(idx) / NRF_UARTE_BASE_FREQUENCY_16MHZ)), (1))

/* When calculating baudrate we need to take into account that high speed instances
 * must have baudrate adjust to the ratio between UARTE clocking frequency and 16 MHz.
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
		IF_ENABLED(UARTE_HAS_FRAME_TIMEOUT,				\
			(.frame_timeout = NRF_UARTE_FRAME_TIMEOUT_EN,))		\
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
			DMM_MEMORY_SECTION(UARTE(idx));			       \
		static uint8_t uarte##idx##_flush_buf[UARTE_HW_RX_FIFO_SIZE]   \
			DMM_MEMORY_SECTION(UARTE(idx));			       \
		struct uarte_async_cb uarte##idx##_async;))		       \
	static uint8_t uarte##idx##_poll_out_byte DMM_MEMORY_SECTION(UARTE(idx));\
	static uint8_t uarte##idx##_poll_in_byte DMM_MEMORY_SECTION(UARTE(idx)); \
	static struct uarte_nrfx_data uarte_##idx##_data = {		       \
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
		COND_CODE_1(CONFIG_UART_USE_RUNTIME_CONFIGURE,		       \
		    (IF_ENABLED(DT_CLOCKS_HAS_IDX(UARTE(idx), 0),	       \
			   (.clock_freq = UARTE_GET_FREQ(idx),))),	       \
		    (IF_ENABLED(UARTE_HAS_FRAME_TIMEOUT,		       \
			(.baudrate = UARTE_PROP(idx, current_speed),))	       \
		     .nrf_baudrate = UARTE_GET_BAUDRATE(idx),		       \
		     .hw_config = UARTE_NRF_CONFIG(idx),))		       \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(UARTE(idx)),		       \
		.uarte_regs = _CONCAT(NRF_UARTE, idx),                         \
		IF_ENABLED(CONFIG_HAS_NORDIC_DMM,			       \
				(.mem_reg = DMM_DEV_TO_REG(UARTE(idx)),))      \
		.flags =						       \
			(IS_ENABLED(CONFIG_UART_##idx##_ENHANCED_POLL_OUT) ?   \
				UARTE_CFG_FLAG_PPI_ENDTX : 0) |		       \
			(IS_ENABLED(CONFIG_UART_##idx##_NRF_HW_ASYNC) ?        \
				UARTE_CFG_FLAG_HW_BYTE_COUNTING : 0) |	       \
			(!IS_ENABLED(CONFIG_HAS_NORDIC_DMM) ? 0 :	       \
			  (UARTE_IS_CACHEABLE(idx) ?			       \
				UARTE_CFG_FLAG_CACHEABLE : 0)) |	       \
			USE_LOW_POWER(idx),				       \
		UARTE_DISABLE_RX_INIT(UARTE(idx)),			       \
		.poll_out_byte = &uarte##idx##_poll_out_byte,		       \
		.poll_in_byte = &uarte##idx##_poll_in_byte,		       \
		IF_ENABLED(CONFIG_UART_##idx##_ASYNC,			       \
				(.tx_cache = uarte##idx##_tx_cache,	       \
				 .rx_flush_buf = uarte##idx##_flush_buf,))     \
		IF_ENABLED(CONFIG_UART_##idx##_NRF_HW_ASYNC,		       \
			(.timer = NRFX_TIMER_INSTANCE(			       \
				CONFIG_UART_##idx##_NRF_HW_ASYNC_TIMER),))     \
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
	PM_DEVICE_DT_DEFINE(UARTE(idx), uarte_nrfx_pm_action,		       \
			    PM_DEVICE_ISR_SAFE);			       \
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
			DMM_MEMORY_SECTION(UARTE(idx));			       \
		 static struct uarte_nrfx_int_driven			       \
			uarte##idx##_int_driven = {			       \
				.tx_buffer = uarte##idx##_tx_buffer,	       \
				.tx_buff_size = sizeof(uarte##idx##_tx_buffer),\
			};))

#define COND_UART_NRF_UARTE_DEVICE(unused, prefix, i, _) \
	IF_ENABLED(CONFIG_HAS_HW_NRF_UARTE##prefix##i, (UART_NRF_UARTE_DEVICE(prefix##i);))

UARTE_FOR_EACH_INSTANCE(COND_UART_NRF_UARTE_DEVICE, (), ())
