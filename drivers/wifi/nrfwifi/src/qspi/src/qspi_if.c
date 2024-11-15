/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing QSPI device interface specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>

#include <soc.h>
#include <nrf_erratas.h>
#include <nrfx_qspi.h>
#include <hal/nrf_clock.h>
#include <hal/nrf_gpio.h>

#include "spi_nor.h"
#include "qspi_if.h"

/* The QSPI bus node which the NRF70 is on */
#define QSPI_IF_BUS_NODE DT_NODELABEL(qspi)

/* QSPI bus properties from the devicetree */
#define QSPI_IF_BUS_IRQN      DT_IRQN(QSPI_IF_BUS_NODE)
#define QSPI_IF_BUS_IRQ_PRIO  DT_IRQ(QSPI_IF_BUS_NODE, priority)
#define QSPI_IF_BUS_SCK_PIN   DT_PROP(QSPI_IF_BUS_NODE, sck_pin)
#define QSPI_IF_BUS_CSN_PIN   DT_PROP(QSPI_IF_BUS_NODE, csn_pins)
#define QSPI_IF_BUS_IO0_PIN   DT_PROP_BY_IDX(QSPI_IF_BUS_NODE, io_pins, 0)
#define QSPI_IF_BUS_IO1_PIN   DT_PROP_BY_IDX(QSPI_IF_BUS_NODE, io_pins, 1)
#define QSPI_IF_BUS_IO2_PIN   DT_PROP_BY_IDX(QSPI_IF_BUS_NODE, io_pins, 2)
#define QSPI_IF_BUS_IO3_PIN   DT_PROP_BY_IDX(QSPI_IF_BUS_NODE, io_pins, 3)

#define QSPI_IF_BUS_HAS_4_IO_PINS \
	(DT_PROP_LEN(QSPI_IF_BUS_NODE, io_pins) == 4)

#define QSPI_IF_BUS_PINCTRL_DT_DEV_CONFIG_GET \
	PINCTRL_DT_DEV_CONFIG_GET(QSPI_IF_BUS_NODE)

/* The NRF70 device node which is on the QSPI bus */
#define QSPI_IF_DEVICE_NODE DT_NODELABEL(nrf70)

/* NRF70 device QSPI properties */
#define QSPI_IF_DEVICE_FREQUENCY DT_PROP(QSPI_IF_DEVICE_NODE, qspi_frequency)
#define QSPI_IF_DEVICE_CPHA      DT_PROP(QSPI_IF_DEVICE_NODE, qspi_cpha)
#define QSPI_IF_DEVICE_CPOL      DT_PROP(QSPI_IF_DEVICE_NODE, qspi_cpol)
#define QSPI_IF_DEVICE_QUAD_MODE DT_PROP(QSPI_IF_DEVICE_NODE, qspi_quad_mode)
#define QSPI_IF_DEVICE_RX_DELAY  DT_PROP(QSPI_IF_DEVICE_NODE, qspi_rx_delay)

static struct qspi_config *qspi_cfg;
#if NRF_QSPI_HAS_XIP_ENC || NRF_QSPI_HAS_DMA_ENC
static unsigned int nonce_last_addr;
static unsigned int nonce_cnt;
#endif /*NRF_QSPI_HAS_XIP_ENC || NRF_QSPI_HAS_DMA_ENC*/

/* Main config structure */
static nrfx_qspi_config_t QSPIconfig;

/*
 * According to the respective specifications, the nRF52 QSPI supports clock
 * frequencies 2 - 32 MHz and the nRF53 one supports 6 - 96 MHz.
 */
BUILD_ASSERT(QSPI_IF_DEVICE_FREQUENCY >= (NRF_QSPI_BASE_CLOCK_FREQ / 16),
	     "Unsupported SCK frequency.");

/*
 * Determine a configuration value (INST_0_SCK_CFG) and, if needed, a divider
 * (BASE_CLOCK_DIV) for the clock from which the SCK frequency is derived that
 * need to be used to achieve the SCK frequency as close as possible (but not
 * higher) to the one specified in DT.
 */
#if defined(CONFIG_SOC_SERIES_NRF53X)
/*
 * On nRF53 Series SoCs, the default /4 divider for the HFCLK192M clock can
 * only be used when the QSPI peripheral is idle. When a QSPI operation is
 * performed, the divider needs to be changed to /1 or /2 (particularly,
 * the specification says that the peripheral "supports 192 MHz and 96 MHz
 * PCLK192M frequency"), but after that operation is complete, the default
 * divider needs to be restored to avoid increased current consumption.
 */
#if (QSPI_IF_DEVICE_FREQUENCY >= NRF_QSPI_BASE_CLOCK_FREQ)
/* For requested SCK >= 96 MHz, use HFCLK192M / 1 / (2*1) = 96 MHz */
#define BASE_CLOCK_DIV NRF_CLOCK_HFCLK_DIV_1
#define INST_0_SCK_CFG NRF_QSPI_FREQ_DIV1
/* If anomaly 159 is to be prevented, only /1 divider can be used. */
#elif NRF53_ERRATA_159_ENABLE_WORKAROUND
#define BASE_CLOCK_DIV NRF_CLOCK_HFCLK_DIV_1
#define INST_0_SCK_CFG (DIV_ROUND_UP(NRF_QSPI_BASE_CLOCK_FREQ, \
				     QSPI_IF_DEVICE_FREQUENCY) - 1)
#elif (QSPI_IF_DEVICE_FREQUENCY >= (NRF_QSPI_BASE_CLOCK_FREQ / 2))
/* For 96 MHz > SCK >= 48 MHz, use HFCLK192M / 2 / (2*1) = 48 MHz */
#define BASE_CLOCK_DIV NRF_CLOCK_HFCLK_DIV_2
#define INST_0_SCK_CFG NRF_QSPI_FREQ_DIV1
#elif (QSPI_IF_DEVICE_FREQUENCY >= (NRF_QSPI_BASE_CLOCK_FREQ / 3))
/* For 48 MHz > SCK >= 32 MHz, use HFCLK192M / 1 / (2*3) = 32 MHz */
#define BASE_CLOCK_DIV NRF_CLOCK_HFCLK_DIV_1
#define INST_0_SCK_CFG NRF_QSPI_FREQ_DIV3
#else
/* For requested SCK < 32 MHz, use divider /2 for HFCLK192M. */
#define BASE_CLOCK_DIV NRF_CLOCK_HFCLK_DIV_2
#define INST_0_SCK_CFG (DIV_ROUND_UP(NRF_QSPI_BASE_CLOCK_FREQ / 2, \
				     QSPI_IF_DEVICE_FREQUENCY) - 1)
#endif

#if BASE_CLOCK_DIV == NRF_CLOCK_HFCLK_DIV_1
/* For 8 MHz, use HFCLK192M / 1 / (2*12) */
#define INST_0_SCK_CFG_WAKE NRF_QSPI_FREQ_DIV12
#elif BASE_CLOCK_DIV == NRF_CLOCK_HFCLK_DIV_2
/* For 8 MHz, use HFCLK192M / 2 / (2*6) */
#define INST_0_SCK_CFG_WAKE NRF_QSPI_FREQ_DIV6
#else
#error "Unsupported base clock divider for wake-up frequency."
#endif

/* After the base clock divider is changed, some time is needed for the new
 * setting to take effect. This value specifies the delay (in microseconds)
 * to be applied to ensure that the clock is ready when the QSPI operation
 * starts. It was measured with a logic analyzer (unfortunately, the nRF5340
 * specification does not provide any numbers in this regard).
 */
/* FIXME: This has adverse impact on performance, ~3Mbps, so, for now, it is
 * disabled till further investigation.
 */
#define BASE_CLOCK_SWITCH_DELAY_US 0

#else
/*
 * On nRF52 Series SoCs, the base clock divider is not configurable,
 * so BASE_CLOCK_DIV is not defined.
 */
#if (QSPI_IF_DEVICE_FREQUENCY >= NRF_QSPI_BASE_CLOCK_FREQ)
#define INST_0_SCK_CFG NRF_QSPI_FREQ_DIV1
#else
#define INST_0_SCK_CFG (DIV_ROUND_UP(NRF_QSPI_BASE_CLOCK_FREQ, \
					 QSPI_IF_DEVICE_FREQUENCY) - 1)
#endif

/* For 8 MHz, use PCLK32M / 4 */
#define INST_0_SCK_CFG_WAKE NRF_QSPI_FREQ_DIV4

#endif /* defined(CONFIG_SOC_SERIES_NRF53X) */

static int qspi_device_init(const struct device *dev);
static void qspi_device_uninit(const struct device *dev);

#define WORD_SIZE 4

LOG_MODULE_DECLARE(wifi_nrf_bus, CONFIG_WIFI_NRF70_BUS_LOG_LEVEL);

/**
 * @brief QSPI buffer structure
 * Structure used both for TX and RX purposes.
 *
 * @param buf is a valid pointer to a data buffer.
 * Can not be NULL.
 * @param len is the length of the data to be handled.
 * If no data to transmit/receive - pass 0.
 */
struct qspi_buf {
	uint8_t *buf;
	size_t len;
};

/**
 * @brief QSPI command structure
 * Structure used for custom command usage.
 *
 * @param op_code is a command value (i.e 0x9F - get Jedec ID)
 * @param tx_buf structure used for TX purposes. Can be NULL if not used.
 * @param rx_buf structure used for RX purposes. Can be NULL if not used.
 */
struct qspi_cmd {
	uint8_t op_code;
	const struct qspi_buf *tx_buf;
	const struct qspi_buf *rx_buf;
};

/**
 * @brief Structure for defining the QSPI NOR access
 */
struct qspi_nor_data {
#ifdef CONFIG_MULTITHREADING
	/* The semaphore to control exclusive access on write/erase. */
	struct k_sem trans;
	/* The semaphore to control exclusive access to the device. */
	struct k_sem sem;
	/* The semaphore to indicate that transfer has completed. */
	struct k_sem sync;
	/* The semaphore to control driver init/uninit. */
	struct k_sem count;
#else /* CONFIG_MULTITHREADING */
	/* A flag that signals completed transfer when threads are
	 * not enabled.
	 */
	volatile bool ready;
#endif /* CONFIG_MULTITHREADING */
};

static inline int qspi_get_mode(bool cpol, bool cpha)
{
	register int ret = -EINVAL;

	if ((!cpol) && (!cpha)) {
		ret = 0;
	} else if (cpol && cpha) {
		ret = 1;
	}

	__ASSERT(ret != -EINVAL, "Invalid QSPI mode");

	return ret;
}

static inline bool qspi_write_is_quad(nrf_qspi_writeoc_t lines)
{
	switch (lines) {
	case NRF_QSPI_WRITEOC_PP4IO:
	case NRF_QSPI_WRITEOC_PP4O:
		return true;
	default:
		return false;
	}
}

static inline bool qspi_read_is_quad(nrf_qspi_readoc_t lines)
{
	switch (lines) {
	case NRF_QSPI_READOC_READ4IO:
	case NRF_QSPI_READOC_READ4O:
		return true;
	default:
		return false;
	}
}

static inline int qspi_get_lines_write(uint8_t lines)
{
	register int ret = -EINVAL;

	switch (lines) {
	case 3:
		ret = NRF_QSPI_WRITEOC_PP4IO;
		break;
	case 2:
		ret = NRF_QSPI_WRITEOC_PP4O;
		break;
	case 1:
		ret = NRF_QSPI_WRITEOC_PP2O;
		break;
	case 0:
		ret = NRF_QSPI_WRITEOC_PP;
		break;
	default:
		break;
	}

	__ASSERT(ret != -EINVAL, "Invalid QSPI write line");

	return ret;
}

static inline int qspi_get_lines_read(uint8_t lines)
{
	register int ret = -EINVAL;

	switch (lines) {
	case 4:
		ret = NRF_QSPI_READOC_READ4IO;
		break;
	case 3:
		ret = NRF_QSPI_READOC_READ4O;
		break;
	case 2:
		ret = NRF_QSPI_READOC_READ2IO;
		break;
	case 1:
		ret = NRF_QSPI_READOC_READ2O;
		break;
	case 0:
		ret = NRF_QSPI_READOC_FASTREAD;
		break;
	default:
		break;
	}

	__ASSERT(ret != -EINVAL, "Invalid QSPI read line");

	return ret;
}

nrfx_err_t _nrfx_qspi_read(void *p_rx_buffer, size_t rx_buffer_length, uint32_t src_address)
{
	return nrfx_qspi_read(p_rx_buffer, rx_buffer_length, src_address);
}

nrfx_err_t _nrfx_qspi_write(void const *p_tx_buffer, size_t tx_buffer_length, uint32_t dst_address)
{
	return nrfx_qspi_write(p_tx_buffer, tx_buffer_length, dst_address);
}

nrfx_err_t _nrfx_qspi_init(nrfx_qspi_config_t const *p_config, nrfx_qspi_handler_t handler,
			   void *p_context)
{
	NRF_QSPI_Type *p_reg = NRF_QSPI;

	nrfx_qspi_init(p_config, handler, p_context);

	/* RDC4IO = 4'hA (register IFTIMING), which means 10 Dummy Cycles for READ4. */
	p_reg->IFTIMING |= qspi_cfg->RDC4IO;

	/* LOG_DBG("%04x : IFTIMING", p_reg->IFTIMING & qspi_cfg->RDC4IO); */

	/* ACTIVATE task fails for slave bitfile so ignore it */
	return NRFX_SUCCESS;
}


/**
 * @brief Main configuration structure
 */
static struct qspi_nor_data qspi_nor_memory_data = {
#ifdef CONFIG_MULTITHREADING
	.trans = Z_SEM_INITIALIZER(qspi_nor_memory_data.trans, 1, 1),
	.sem = Z_SEM_INITIALIZER(qspi_nor_memory_data.sem, 1, 1),
	.sync = Z_SEM_INITIALIZER(qspi_nor_memory_data.sync, 0, 1),
	.count = Z_SEM_INITIALIZER(qspi_nor_memory_data.count, 0, K_SEM_MAX_LIMIT),
#endif /* CONFIG_MULTITHREADING */
};

NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(QSPI_IF_BUS_NODE);

IF_ENABLED(CONFIG_PINCTRL, (PINCTRL_DT_DEFINE(QSPI_IF_BUS_NODE)));

/**
 * @brief Converts NRFX return codes to the zephyr ones
 */
static inline int qspi_get_zephyr_ret_code(nrfx_err_t res)
{
	switch (res) {
	case NRFX_SUCCESS:
		return 0;
	case NRFX_ERROR_INVALID_PARAM:
	case NRFX_ERROR_INVALID_ADDR:
		return -EINVAL;
	case NRFX_ERROR_INVALID_STATE:
		return -ECANCELED;
#if NRF53_ERRATA_159_ENABLE_WORKAROUND
	case NRFX_ERROR_FORBIDDEN:
		LOG_ERR("nRF5340 anomaly 159 conditions detected");
		LOG_ERR("Set the CPU clock to 64 MHz before starting QSPI operation");
		return -ECANCELED;
#endif
	case NRFX_ERROR_BUSY:
	case NRFX_ERROR_TIMEOUT:
	default:
		return -EBUSY;
	}
}

static inline struct qspi_nor_data *get_dev_data(const struct device *dev)
{
	return dev->data;
}

static inline void qspi_lock(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct qspi_nor_data *dev_data = get_dev_data(dev);

	k_sem_take(&dev_data->sem, K_FOREVER);
#else /* CONFIG_MULTITHREADING */
	ARG_UNUSED(dev);
#endif /* CONFIG_MULTITHREADING */

	/*
	 * Change the base clock divider only for the time the driver is locked
	 * to perform a QSPI operation, otherwise the power consumption would be
	 * increased also when the QSPI peripheral is idle.
	 */
#if defined(CONFIG_SOC_SERIES_NRF53X)
	nrf_clock_hfclk192m_div_set(NRF_CLOCK, BASE_CLOCK_DIV);
	k_busy_wait(BASE_CLOCK_SWITCH_DELAY_US);
#endif
}

static inline void qspi_unlock(const struct device *dev)
{
#if defined(CONFIG_SOC_SERIES_NRF53X)
	/* Restore the default base clock divider to reduce power consumption.
	 */
	nrf_clock_hfclk192m_div_set(NRF_CLOCK, NRF_CLOCK_HFCLK_DIV_4);
	k_busy_wait(BASE_CLOCK_SWITCH_DELAY_US);
#endif

#ifdef CONFIG_MULTITHREADING
	struct qspi_nor_data *dev_data = get_dev_data(dev);

	k_sem_give(&dev_data->sem);
#else /* CONFIG_MULTITHREADING */
	ARG_UNUSED(dev);
#endif /* CONFIG_MULTITHREADING */
}

static inline void qspi_trans_lock(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct qspi_nor_data *dev_data = get_dev_data(dev);

	k_sem_take(&dev_data->trans, K_FOREVER);
#else /* CONFIG_MULTITHREADING */
	ARG_UNUSED(dev);
#endif /* CONFIG_MULTITHREADING */
}

static inline void qspi_trans_unlock(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct qspi_nor_data *dev_data = get_dev_data(dev);

	k_sem_give(&dev_data->trans);
#else /* CONFIG_MULTITHREADING */
	ARG_UNUSED(dev);
#endif /* CONFIG_MULTITHREADING */
}

static inline void qspi_wait_for_completion(const struct device *dev, nrfx_err_t res)
{
	struct qspi_nor_data *dev_data = get_dev_data(dev);

	if (res == NRFX_SUCCESS) {
#ifdef CONFIG_MULTITHREADING
		k_sem_take(&dev_data->sync, K_FOREVER);
#else /* CONFIG_MULTITHREADING */
		unsigned int key = irq_lock();

		while (!dev_data->ready) {
			k_cpu_atomic_idle(key);
			key = irq_lock();
		}

		dev_data->ready = false;
		irq_unlock(key);
#endif /* CONFIG_MULTITHREADING */
	}
}

static inline void qspi_complete(struct qspi_nor_data *dev_data)
{
#ifdef CONFIG_MULTITHREADING
	k_sem_give(&dev_data->sync);
#else /* CONFIG_MULTITHREADING */
	dev_data->ready = true;
#endif /* CONFIG_MULTITHREADING */
}

static inline void _qspi_complete(struct qspi_nor_data *dev_data)
{
	if (!qspi_cfg->easydma) {
		return;
	}

	qspi_complete(dev_data);
}
static inline void _qspi_wait_for_completion(const struct device *dev, nrfx_err_t res)
{
	if (!qspi_cfg->easydma) {
		return;
	}

	qspi_wait_for_completion(dev, res);
}

/**
 * @brief QSPI handler
 *
 * @param event Driver event type
 * @param p_context Pointer to context. Use in interrupt handler.
 * @retval None
 */
static void qspi_handler(nrfx_qspi_evt_t event, void *p_context)
{
	struct qspi_nor_data *dev_data = p_context;

	if (event == NRFX_QSPI_EVENT_DONE) {
		_qspi_complete(dev_data);
	}
}

static bool qspi_initialized;

static int qspi_device_init(const struct device *dev)
{
	struct qspi_nor_data *dev_data = get_dev_data(dev);
	nrfx_err_t res;
	int ret = 0;

	if (!IS_ENABLED(CONFIG_NRF70_QSPI_LOW_POWER)) {
		return 0;
	}

	qspi_lock(dev);

	/* In multithreading, driver can call qspi_device_init more than once
	 * before calling qspi_device_uninit. Keepping count, so QSPI is
	 * uninitialized only at the last call (count == 0).
	 */
#ifdef CONFIG_MULTITHREADING
	k_sem_give(&dev_data->count);
#endif

	if (!qspi_initialized) {
		res = nrfx_qspi_init(&QSPIconfig, qspi_handler, dev_data);
		ret = qspi_get_zephyr_ret_code(res);
		NRF_QSPI->IFTIMING |= qspi_cfg->RDC4IO;
		qspi_initialized = (ret == 0);
	}

	qspi_unlock(dev);

	return ret;
}

static void qspi_device_uninit(const struct device *dev)
{
	bool last = true;

	if (!IS_ENABLED(CONFIG_NRF70_QSPI_LOW_POWER)) {
		return;
	}

	qspi_lock(dev);

#ifdef CONFIG_MULTITHREADING
	struct qspi_nor_data *dev_data = get_dev_data(dev);

	/* The last thread to finish using the driver uninit the QSPI */
	(void)k_sem_take(&dev_data->count, K_NO_WAIT);
	last = (k_sem_count_get(&dev_data->count) == 0);
#endif

	if (last) {
		while (nrfx_qspi_mem_busy_check() != NRFX_SUCCESS) {
			if (IS_ENABLED(CONFIG_MULTITHREADING)) {
				k_msleep(50);
			} else {
				k_busy_wait(50000);
			}
		}

		nrfx_qspi_uninit();

#ifndef CONFIG_PINCTRL
		nrf_gpio_cfg_output(QSPI_PROP_AT(csn_pins, 0));
		nrf_gpio_pin_set(QSPI_PROP_AT(csn_pins, 0));
#endif

		qspi_initialized = false;
	}

	qspi_unlock(dev);
}

/* QSPI send custom command.
 *
 * If this is used for both send and receive the buffer sizes must be
 * equal and cover the whole transaction.
 */
static int qspi_send_cmd(const struct device *dev, const struct qspi_cmd *cmd, bool wren)
{
	/* Check input parameters */
	if (!cmd) {
		return -EINVAL;
	}

	const void *tx_buf = NULL;
	size_t tx_len = 0;
	void *rx_buf = NULL;
	size_t rx_len = 0;
	size_t xfer_len = sizeof(cmd->op_code);

	if (cmd->tx_buf) {
		tx_buf = cmd->tx_buf->buf;
		tx_len = cmd->tx_buf->len;
	}

	if (cmd->rx_buf) {
		rx_buf = cmd->rx_buf->buf;
		rx_len = cmd->rx_buf->len;
	}

	if ((rx_len != 0) && (tx_len != 0)) {
		if (rx_len != tx_len) {
			return -EINVAL;
		}

		xfer_len += tx_len;
	} else {
		/* At least one of these is zero. */
		xfer_len += tx_len + rx_len;
	}

	if (xfer_len > NRF_QSPI_CINSTR_LEN_9B) {
		LOG_WRN("cinstr %02x transfer too long: %zu", cmd->op_code, xfer_len);

		return -EINVAL;
	}

	nrf_qspi_cinstr_conf_t cinstr_cfg = {
		.opcode = cmd->op_code,
		.length = xfer_len,
		.io2_level = true,
		.io3_level = true,
		.wipwait = false,
		.wren = wren,
	};

	qspi_lock(dev);

	int res = nrfx_qspi_cinstr_xfer(&cinstr_cfg, tx_buf, rx_buf);

	qspi_unlock(dev);
	return qspi_get_zephyr_ret_code(res);
}

/* RDSR wrapper.  Negative value is error. */
static int qspi_rdsr(const struct device *dev)
{
	uint8_t sr = -1;
	const struct qspi_buf sr_buf = {
		.buf = &sr,
		.len = sizeof(sr),
	};
	struct qspi_cmd cmd = {
		.op_code = SPI_NOR_CMD_RDSR,
		.rx_buf = &sr_buf,
	};
	int ret = qspi_send_cmd(dev, &cmd, false);

	return (ret < 0) ? ret : sr;
}

/* Wait until RDSR confirms write is not in progress. */
static int qspi_wait_while_writing(const struct device *dev)
{
	int ret;

	do {
		ret = qspi_rdsr(dev);
	} while ((ret >= 0) && ((ret & SPI_NOR_WIP_BIT) != 0U));

	return (ret < 0) ? ret : 0;
}

/**
 * @brief Fills init struct
 *
 * @param config Pointer to the config struct provided by user
 * @param initstruct Pointer to the configuration struct
 * @retval None
 */
static inline void qspi_fill_init_struct(nrfx_qspi_config_t *initstruct)
{
	/* Configure XIP offset */
	initstruct->xip_offset = 0;

#ifdef CONFIG_PINCTRL
	initstruct->skip_gpio_cfg = true,
	initstruct->skip_psel_cfg = true,
#else
	/* Configure pins */
	initstruct->pins.sck_pin = QSPI_IF_BUS_SCK_PIN;
	initstruct->pins.csn_pin = QSPI_IF_BUS_CSN_PIN;
	initstruct->pins.io0_pin = QSPI_IF_BUS_IO0_PIN;
	initstruct->pins.io1_pin = QSPI_IF_BUS_IO1_PIN;
#if QSPI_IF_BUS_HAS_4_IO_PINS
	initstruct->pins.io2_pin = QSPI_IF_BUS_IO2_PIN;
	initstruct->pins.io3_pin = QSPI_IF_BUS_IO3_PIN;
#else
	initstruct->pins.io2_pin = NRF_QSPI_PIN_NOT_CONNECTED;
	initstruct->pins.io3_pin = NRF_QSPI_PIN_NOT_CONNECTED;
#endif
#endif /* CONFIG_PINCTRL */
	/* Configure Protocol interface */
	initstruct->prot_if.addrmode = NRF_QSPI_ADDRMODE_24BIT;

	initstruct->prot_if.dpmconfig = false;

	/* Configure physical interface */
	initstruct->phy_if.sck_freq = INST_0_SCK_CFG;

	/* Using MHZ fails checkpatch constant check */
	if (QSPI_IF_DEVICE_FREQUENCY >= 16000000) {
		qspi_cfg->qspi_slave_latency = 1;
	}
	initstruct->phy_if.sck_delay = QSPI_IF_DEVICE_RX_DELAY;
	initstruct->phy_if.spi_mode = qspi_get_mode(QSPI_IF_DEVICE_CPOL, QSPI_IF_DEVICE_CPHA);

	if (QSPI_IF_DEVICE_QUAD_MODE) {
		initstruct->prot_if.readoc = NRF_QSPI_READOC_READ4IO;
		initstruct->prot_if.writeoc = NRF_QSPI_WRITEOC_PP4IO;
	} else {
		initstruct->prot_if.readoc = NRF_QSPI_READOC_FASTREAD;
		initstruct->prot_if.writeoc = NRF_QSPI_WRITEOC_PP;
	}

	initstruct->phy_if.dpmen = false;
}

/* Configures QSPI memory for the transfer */
static int qspi_nrfx_configure(const struct device *dev)
{
	if (!dev) {
		return -ENXIO;
	}

	struct qspi_nor_data *dev_data = dev->data;

	qspi_fill_init_struct(&QSPIconfig);

#if defined(CONFIG_SOC_SERIES_NRF53X)
	/* When the QSPI peripheral is activated, during the nrfx_qspi driver
	 * initialization, it reads the status of the connected flash chip.
	 * Make sure this transaction is performed with a valid base clock
	 * divider.
	 */
	nrf_clock_hfclk192m_div_set(NRF_CLOCK, BASE_CLOCK_DIV);
	k_busy_wait(BASE_CLOCK_SWITCH_DELAY_US);
#endif

	nrfx_err_t res = _nrfx_qspi_init(&QSPIconfig, qspi_handler, dev_data);

#if defined(CONFIG_SOC_SERIES_NRF53X)
	/* Restore the default /4 divider after the QSPI initialization. */
	nrf_clock_hfclk192m_div_set(NRF_CLOCK, NRF_CLOCK_HFCLK_DIV_4);
	k_busy_wait(BASE_CLOCK_SWITCH_DELAY_US);
#endif

	int ret = qspi_get_zephyr_ret_code(res);

	if (ret == 0) {
		/* Set QE to match transfer mode. If not using quad
		 * it's OK to leave QE set, but doing so prevents use
		 * of WP#/RESET#/HOLD# which might be useful.
		 *
		 * Note build assert above ensures QER is S1B6.  Other
		 * options require more logic.
		 */
		ret = qspi_rdsr(dev);

		if (ret < 0) {
			LOG_ERR("RDSR failed: %d", ret);
			return ret;
		}

		uint8_t sr = (uint8_t)ret;
		bool qe_value = (qspi_write_is_quad(QSPIconfig.prot_if.writeoc)) ||
				(qspi_read_is_quad(QSPIconfig.prot_if.readoc));
		const uint8_t qe_mask = BIT(6); /* only S1B6 */
		bool qe_state = ((sr & qe_mask) != 0U);

		LOG_DBG("RDSR %02x QE %d need %d: %s", sr, qe_state, qe_value,
			(qe_state != qe_value) ? "updating" : "no-change");

		ret = 0;

		if (qe_state != qe_value) {
			const struct qspi_buf sr_buf = {
				.buf = &sr,
				.len = sizeof(sr),
			};
			struct qspi_cmd cmd = {
				.op_code = SPI_NOR_CMD_WRSR,
				.tx_buf = &sr_buf,
			};

			sr ^= qe_mask;
			ret = qspi_send_cmd(dev, &cmd, true);

			/* Writing SR can take some time, and further
			 * commands sent while it's happening can be
			 * corrupted.  Wait.
			 */
			if (ret == 0) {
				ret = qspi_wait_while_writing(dev);
			}
		}

		if (ret < 0) {
			LOG_ERR("QE %s failed: %d", qe_value ? "set" : "clear", ret);
		}
	}

	return ret;
}

static inline nrfx_err_t read_non_aligned(const struct device *dev, int addr, void *dest,
					  size_t size)
{
	uint8_t __aligned(WORD_SIZE) buf[WORD_SIZE * 2];
	uint8_t *dptr = dest;

	int flash_prefix = (WORD_SIZE - (addr % WORD_SIZE)) % WORD_SIZE;

	if (flash_prefix > size) {
		flash_prefix = size;
	}

	int dest_prefix = (WORD_SIZE - (int)dptr % WORD_SIZE) % WORD_SIZE;

	if (dest_prefix > size) {
		dest_prefix = size;
	}

	int flash_suffix = (size - flash_prefix) % WORD_SIZE;
	int flash_middle = size - flash_prefix - flash_suffix;
	int dest_middle = size - dest_prefix - (size - dest_prefix) % WORD_SIZE;

	if (flash_middle > dest_middle) {
		flash_middle = dest_middle;
		flash_suffix = size - flash_prefix - flash_middle;
	}

	nrfx_err_t res = NRFX_SUCCESS;

	/* read from aligned flash to aligned memory */
	if (flash_middle != 0) {
		res = _nrfx_qspi_read(dptr + dest_prefix, flash_middle, addr + flash_prefix);

		_qspi_wait_for_completion(dev, res);

		if (res != NRFX_SUCCESS) {
			return res;
		}

		/* perform shift in RAM */
		if (flash_prefix != dest_prefix) {
			memmove(dptr + flash_prefix, dptr + dest_prefix, flash_middle);
		}
	}

	/* read prefix */
	if (flash_prefix != 0) {
		res = _nrfx_qspi_read(buf, WORD_SIZE, addr - (WORD_SIZE - flash_prefix));

		_qspi_wait_for_completion(dev, res);

		if (res != NRFX_SUCCESS) {
			return res;
		}

		memcpy(dptr, buf + WORD_SIZE - flash_prefix, flash_prefix);
	}

	/* read suffix */
	if (flash_suffix != 0) {
		res = _nrfx_qspi_read(buf, WORD_SIZE * 2, addr + flash_prefix + flash_middle);

		_qspi_wait_for_completion(dev, res);

		if (res != NRFX_SUCCESS) {
			return res;
		}

		memcpy(dptr + flash_prefix + flash_middle, buf, flash_suffix);
	}

	return res;
}

static int qspi_nor_read(const struct device *dev, int addr, void *dest, size_t size)
{
	if (!dest) {
		return -EINVAL;
	}

	/* read size must be non-zero */
	if (!size) {
		return 0;
	}

	int rc = qspi_device_init(dev);

	if (rc != 0) {
		goto out;
	}

	qspi_lock(dev);

	nrfx_err_t res = read_non_aligned(dev, addr, dest, size);

	qspi_unlock(dev);

	rc = qspi_get_zephyr_ret_code(res);

out:
	qspi_device_uninit(dev);
	return rc;
}

/* addr aligned, sptr not null, slen less than 4 */
static inline nrfx_err_t write_sub_word(const struct device *dev, int addr, const void *sptr,
					size_t slen)
{
	uint8_t __aligned(4) buf[4];
	nrfx_err_t res;

	/* read out the whole word so that unchanged data can be
	 * written back
	 */
	res = _nrfx_qspi_read(buf, sizeof(buf), addr);
	_qspi_wait_for_completion(dev, res);

	if (res == NRFX_SUCCESS) {
		memcpy(buf, sptr, slen);
		res = _nrfx_qspi_write(buf, sizeof(buf), addr);
		_qspi_wait_for_completion(dev, res);
	}

	return res;
}

static int qspi_nor_write(const struct device *dev, int addr, const void *src, size_t size)
{
	if (!src) {
		return -EINVAL;
	}

	/* write size must be non-zero, less than 4, or a multiple of 4 */
	if ((size == 0) || ((size > 4) && ((size % 4U) != 0))) {
		return -EINVAL;
	}

	/* address must be 4-byte aligned */
	if ((addr % 4U) != 0) {
		return -EINVAL;
	}

	nrfx_err_t res = NRFX_SUCCESS;

	int rc = qspi_device_init(dev);

	if (rc != 0) {
		goto out;
	}

	qspi_trans_lock(dev);

	qspi_lock(dev);

	if (size < 4U) {
		res = write_sub_word(dev, addr, src, size);
	} else {
		res = _nrfx_qspi_write(src, size, addr);
		_qspi_wait_for_completion(dev, res);
	}

	qspi_unlock(dev);

	qspi_trans_unlock(dev);

	rc = qspi_get_zephyr_ret_code(res);
out:
	qspi_device_uninit(dev);
	return rc;
}

/**
 * @brief Configure the flash
 *
 * @param dev The flash device structure
 * @param info The flash info structure
 * @return 0 on success, negative errno code otherwise
 */
static int qspi_nor_configure(const struct device *dev)
{
	int ret = qspi_nrfx_configure(dev);

	if (ret != 0) {
		return ret;
	}

	qspi_device_uninit(dev);

	return 0;
}

/**
 * @brief Initialize and configure the flash
 *
 * @param name The flash name
 * @return 0 on success, negative errno code otherwise
 */
static int qspi_nor_init(const struct device *dev)
{
#ifdef CONFIG_PINCTRL
	int ret = pinctrl_apply_state(QSPI_IF_BUS_PINCTRL_DT_DEV_CONFIG_GET,
				      PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		return ret;
	}
#endif

	IRQ_CONNECT(QSPI_IF_BUS_IRQN,
		    QSPI_IF_BUS_IRQ_PRIO,
		    nrfx_isr,
		    nrfx_qspi_irq_handler,
		    0);

	return qspi_nor_configure(dev);
}

#if defined(CONFIG_SOC_SERIES_NRF53X)
static int qspi_cmd_encryption(const struct device *dev, nrf_qspi_encryption_t *p_cfg)
{
	const struct qspi_buf tx_buf = { .buf = (uint8_t *)&p_cfg->nonce[1],
					 .len = sizeof(p_cfg->nonce[1]) };
	const struct qspi_cmd cmd = {
		.op_code = 0x4f,
		.tx_buf = &tx_buf,
	};

	int ret = qspi_device_init(dev);

	if (ret == 0) {
		ret = qspi_send_cmd(dev, &cmd, false);
	}

	qspi_device_uninit(dev);

	if (ret < 0) {
		LOG_DBG("cmd_encryption failed %d", ret);
	}

	return ret;
}
#endif

int qspi_RDSR2(const struct device *dev, uint8_t *rdsr2)
{
	int ret = 0;
	uint8_t sr = 0;

	const struct qspi_buf sr_buf = {
		.buf = &sr,
		.len = sizeof(sr),
	};
	struct qspi_cmd cmd = {
		.op_code = 0x2f,
		.rx_buf = &sr_buf,
	};

	ret = qspi_device_init(dev);

	ret = qspi_send_cmd(dev, &cmd, false);

	qspi_device_uninit(dev);

	LOG_DBG("RDSR2 = 0x%x", sr);

	if (ret == 0) {
		*rdsr2 = sr;
	}

	return ret;
}

/* Wait until RDSR2 confirms RPU_WAKE write is successful */
int qspi_validate_rpu_wake_writecmd(const struct device *dev)
{
	int ret = 0;
	uint8_t rdsr2 = 0;

	for (int ii = 0; ii < 1; ii++) {
		ret = qspi_RDSR2(dev, &rdsr2);
		if (!ret && (rdsr2 & RPU_WAKEUP_NOW)) {
			return 0;
		}
	}

	return -1;
}


int qspi_RDSR1(const struct device *dev, uint8_t *rdsr1)
{
	int ret = 0;
	uint8_t sr = 0;

	const struct qspi_buf sr_buf = {
		.buf = &sr,
		.len = sizeof(sr),
	};
	struct qspi_cmd cmd = {
		.op_code = 0x1f,
		.rx_buf = &sr_buf,
	};

	ret = qspi_device_init(dev);

	ret = qspi_send_cmd(dev, &cmd, false);

	qspi_device_uninit(dev);

	LOG_DBG("RDSR1 = 0x%x", sr);

	if (ret == 0) {
		*rdsr1 = sr;
	}

	return ret;
}

/* Wait until RDSR1 confirms RPU_AWAKE/RPU_READY */
int qspi_wait_while_rpu_awake(const struct device *dev)
{
	int ret;
	uint8_t val = 0;

	for (int ii = 0; ii < 10; ii++) {
		ret = qspi_RDSR1(dev, &val);

		LOG_DBG("RDSR1 = 0x%x", val);

		if (!ret && (val & RPU_AWAKE_BIT)) {
			break;
		}

		k_msleep(1);
	}

	if (ret || !(val & RPU_AWAKE_BIT)) {
		LOG_ERR("RPU is not awake even after 10ms");
		return -1;
	}

	/* Restore QSPI clock frequency from DTS */
	QSPIconfig.phy_if.sck_freq = INST_0_SCK_CFG;

	return val;
}

int qspi_WRSR2(const struct device *dev, uint8_t data)
{
	const struct qspi_buf tx_buf = {
		.buf = &data,
		.len = sizeof(data),
	};
	const struct qspi_cmd cmd = {
		.op_code = 0x3f,
		.tx_buf = &tx_buf,
	};
	int ret = qspi_device_init(dev);

	if (ret == 0) {
		ret = qspi_send_cmd(dev, &cmd, false);
	}

	qspi_device_uninit(dev);

	if (ret < 0) {
		LOG_ERR("cmd_wakeup RPU failed %d", ret);
	}

	return ret;
}

int qspi_cmd_wakeup_rpu(const struct device *dev, uint8_t data)
{
	int ret;

	/* Waking RPU works reliably only with lowest frequency (8MHz) */
	QSPIconfig.phy_if.sck_freq = INST_0_SCK_CFG_WAKE;

	ret = qspi_WRSR2(dev, data);

	return ret;
}

struct device qspi_perip = {
	.data = &qspi_nor_memory_data,
};

int qspi_deinit(void)
{
	LOG_DBG("TODO : %s", __func__);

	return 0;
}

int qspi_init(struct qspi_config *config)
{
	unsigned int rc;

	qspi_cfg = config;

	config->readoc = config->quad_spi ? NRF_QSPI_READOC_READ4IO : NRF_QSPI_READOC_FASTREAD;
	config->writeoc = config->quad_spi ? NRF_QSPI_WRITEOC_PP4IO : NRF_QSPI_WRITEOC_PP;

	rc = qspi_nor_init(&qspi_perip);

	k_sem_init(&qspi_cfg->lock, 1, 1);

	return rc;
}

void qspi_update_nonce(unsigned int addr, int len, int hlread)
{
#if NRF_QSPI_HAS_XIP_ENC || NRF_QSPI_HAS_DMA_ENC

	NRF_QSPI_Type *p_reg = NRF_QSPI;

	if (!qspi_cfg->encryption) {
		return;
	}

	if (nonce_last_addr == 0 || hlread) {
		p_reg->DMA_ENC.NONCE2 = ++nonce_cnt;
	} else if ((nonce_last_addr + 4) != addr) {
		p_reg->DMA_ENC.NONCE2 = ++nonce_cnt;
	}

	nonce_last_addr = addr + len - 4;

#endif /*NRF_QSPI_HAS_XIP_ENC || NRF_QSPI_HAS_DMA_ENC*/
}

void qspi_addr_check(unsigned int addr, const void *data, unsigned int len)
{
	if ((addr % 4 != 0) || (((unsigned int)data) % 4 != 0) || (len % 4 != 0)) {
		LOG_ERR("%s : Unaligned address %x %x %d %x %x", __func__, addr,
		       (unsigned int)data, (addr % 4 != 0), (((unsigned int)data) % 4 != 0),
		       (len % 4 != 0));
	}
}

int qspi_write(unsigned int addr, const void *data, int len)
{
	int status;

	qspi_addr_check(addr, data, len);

	addr |= qspi_cfg->addrmask;

	k_sem_take(&qspi_cfg->lock, K_FOREVER);

	qspi_update_nonce(addr, len, 0);

	status = qspi_nor_write(&qspi_perip, addr, data, len);

	k_sem_give(&qspi_cfg->lock);

	return status;
}

int qspi_read(unsigned int addr, void *data, int len)
{
	int status;

	qspi_addr_check(addr, data, len);

	addr |= qspi_cfg->addrmask;

	k_sem_take(&qspi_cfg->lock, K_FOREVER);

	qspi_update_nonce(addr, len, 0);

	status = qspi_nor_read(&qspi_perip, addr, data, len);

	k_sem_give(&qspi_cfg->lock);

	return status;
}

int qspi_hl_readw(unsigned int addr, void *data)
{
	int status;
	uint8_t *rxb = NULL;
	uint32_t len = 4;

	len = len + (4 * qspi_cfg->qspi_slave_latency);

	rxb = k_malloc(len);

	if (rxb == NULL) {
		LOG_ERR("%s: ERROR ENOMEM line %d", __func__, __LINE__);
		return -ENOMEM;
	}

	memset(rxb, 0, len);

	k_sem_take(&qspi_cfg->lock, K_FOREVER);

	qspi_update_nonce(addr, 4, 1);

	status = qspi_nor_read(&qspi_perip, addr, rxb, len);

	k_sem_give(&qspi_cfg->lock);

	*(uint32_t *)data = *(uint32_t *)(rxb + (len - 4));

	k_free(rxb);

	return status;
}

int qspi_hl_read(unsigned int addr, void *data, int len)
{
	int count = 0;

	qspi_addr_check(addr, data, len);

	while (count < (len / 4)) {
		qspi_hl_readw(addr + (4 * count), ((char *)data + (4 * count)));
		count++;
	}

	return 0;
}

int qspi_cmd_sleep_rpu(const struct device *dev)
{
	uint8_t data = 0x0;

	/* printf("TODO : %s:", __func__); */
	const struct qspi_buf tx_buf = {
		.buf = &data,
		.len = sizeof(data),
	};

	const struct qspi_cmd cmd = {
		.op_code = 0x3f, /* 0x3f, //WRSR2(0x3F) WakeUP RPU. */
		.tx_buf = &tx_buf,
	};

	int ret = qspi_device_init(dev);

	if (ret == 0) {
		ret = qspi_send_cmd(dev, &cmd, false);
	}

	qspi_device_uninit(dev);

	if (ret < 0) {
		LOG_ERR("cmd_wakeup RPU failed: %d", ret);
	}

	return ret;
}

/* Encryption public API */

int qspi_enable_encryption(uint8_t *key)
{
#if defined(CONFIG_SOC_SERIES_NRF53X)
	int err = 0;

	if (qspi_cfg->encryption) {
		return -EALREADY;
	}

	int ret = qspi_device_init(&qspi_perip);

	if (ret != 0) {
		LOG_ERR("qspi_device_init failed: %d", ret);
		return -EIO;
	}

	memcpy(qspi_cfg->p_cfg.key, key, 16);

	err = nrfx_qspi_dma_encrypt(&qspi_cfg->p_cfg);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_qspi_dma_encrypt failed: %d", err);
		return -EIO;
	}

	err = qspi_cmd_encryption(&qspi_perip, &qspi_cfg->p_cfg);
	if (err != 0) {
		LOG_ERR("qspi_cmd_encryption failed: %d", err);
		return -EIO;
	}

	qspi_cfg->encryption = true;

	qspi_device_uninit(&qspi_perip);

	return 0;
#else
	return -ENOTSUP;
#endif
}
