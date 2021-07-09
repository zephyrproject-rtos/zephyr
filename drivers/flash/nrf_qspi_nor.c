/*
 * Copyright (c) 2019-2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_qspi_nor

#include <errno.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/atomic.h>
#include <soc.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(qspi_nor, CONFIG_FLASH_LOG_LEVEL);

#include "spi_nor.h"
#include "jesd216.h"
#include "flash_priv.h"
#include <nrf_erratas.h>
#include <nrfx_qspi.h>
#include <hal/nrf_clock.h>
#include <hal/nrf_gpio.h>

struct qspi_nor_data {
#ifdef CONFIG_MULTITHREADING
	/* The semaphore to control exclusive access to the device. */
	struct k_sem sem;
	/* The semaphore to indicate that transfer has completed. */
	struct k_sem sync;
	/* A counter to control QSPI deactivation. */
	atomic_t usage_count;
#else /* CONFIG_MULTITHREADING */
	/* A flag that signals completed transfer when threads are
	 * not enabled.
	 */
	volatile bool ready;
#endif /* CONFIG_MULTITHREADING */
	bool xip_enabled;
};

struct qspi_nor_config {
	nrfx_qspi_config_t nrfx_cfg;

	/* Size from devicetree, in bytes */
	uint32_t size;

	/* JEDEC id from devicetree */
	uint8_t id[SPI_NOR_MAX_ID_LEN];

	const struct pinctrl_dev_config *pcfg;
};

/* Status register bits */
#define QSPI_SECTOR_SIZE SPI_NOR_SECTOR_SIZE
#define QSPI_BLOCK_SIZE SPI_NOR_BLOCK_SIZE

/* instance 0 flash size in bytes */
#if DT_INST_NODE_HAS_PROP(0, size_in_bytes)
#define INST_0_BYTES (DT_INST_PROP(0, size_in_bytes))
#elif DT_INST_NODE_HAS_PROP(0, size)
#define INST_0_BYTES (DT_INST_PROP(0, size) / 8)
#else
#error "No size specified. 'size' or 'size-in-bytes' must be set"
#endif

BUILD_ASSERT(!(DT_INST_NODE_HAS_PROP(0, size_in_bytes) && DT_INST_NODE_HAS_PROP(0, size)),
	     "Node " DT_NODE_PATH(DT_DRV_INST(0)) " has both size and size-in-bytes "
	     "properties; use exactly one");


#define INST_0_SCK_FREQUENCY DT_INST_PROP(0, sck_frequency)
/*
 * According to the respective specifications, the nRF52 QSPI supports clock
 * frequencies 2 - 32 MHz and the nRF53 one supports 6 - 96 MHz.
 */
BUILD_ASSERT(INST_0_SCK_FREQUENCY >= (NRF_QSPI_BASE_CLOCK_FREQ / 16),
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
#if (INST_0_SCK_FREQUENCY >= NRF_QSPI_BASE_CLOCK_FREQ)
/* For requested SCK >= 96 MHz, use HFCLK192M / 1 / (2*1) = 96 MHz */
#define BASE_CLOCK_DIV NRF_CLOCK_HFCLK_DIV_1
#define INST_0_SCK_CFG NRF_QSPI_FREQ_DIV1
/* If anomaly 159 is to be prevented, only /1 divider can be used. */
#elif NRF53_ERRATA_159_ENABLE_WORKAROUND
#define BASE_CLOCK_DIV NRF_CLOCK_HFCLK_DIV_1
#define INST_0_SCK_CFG (DIV_ROUND_UP(NRF_QSPI_BASE_CLOCK_FREQ, \
				     INST_0_SCK_FREQUENCY) - 1)
#elif (INST_0_SCK_FREQUENCY >= (NRF_QSPI_BASE_CLOCK_FREQ / 2))
/* For 96 MHz > SCK >= 48 MHz, use HFCLK192M / 2 / (2*1) = 48 MHz */
#define BASE_CLOCK_DIV NRF_CLOCK_HFCLK_DIV_2
#define INST_0_SCK_CFG NRF_QSPI_FREQ_DIV1
#elif (INST_0_SCK_FREQUENCY >= (NRF_QSPI_BASE_CLOCK_FREQ / 3))
/* For 48 MHz > SCK >= 32 MHz, use HFCLK192M / 1 / (2*3) = 32 MHz */
#define BASE_CLOCK_DIV NRF_CLOCK_HFCLK_DIV_1
#define INST_0_SCK_CFG NRF_QSPI_FREQ_DIV3
#else
/* For requested SCK < 32 MHz, use divider /2 for HFCLK192M. */
#define BASE_CLOCK_DIV NRF_CLOCK_HFCLK_DIV_2
#define INST_0_SCK_CFG (DIV_ROUND_UP(NRF_QSPI_BASE_CLOCK_FREQ / 2, \
				     INST_0_SCK_FREQUENCY) - 1)
#endif
/* After the base clock divider is changed, some time is needed for the new
 * setting to take effect. This value specifies the delay (in microseconds)
 * to be applied to ensure that the clock is ready when the QSPI operation
 * starts. It was measured with a logic analyzer (unfortunately, the nRF5340
 * specification does not provide any numbers in this regard).
 */
#define BASE_CLOCK_SWITCH_DELAY_US 7

#else
/*
 * On nRF52 Series SoCs, the base clock divider is not configurable,
 * so BASE_CLOCK_DIV is not defined.
 */
#if (INST_0_SCK_FREQUENCY >= NRF_QSPI_BASE_CLOCK_FREQ)
#define INST_0_SCK_CFG NRF_QSPI_FREQ_DIV1
#else
#define INST_0_SCK_CFG (DIV_ROUND_UP(NRF_QSPI_BASE_CLOCK_FREQ, \
					 INST_0_SCK_FREQUENCY) - 1)

#endif

#endif /* defined(CONFIG_SOC_SERIES_NRF53X) */

/* 0 for MODE0 (CPOL=0, CPHA=0), 1 for MODE3 (CPOL=1, CPHA=1). */
#define INST_0_SPI_MODE DT_INST_PROP(0, cpol)
BUILD_ASSERT(DT_INST_PROP(0, cpol) == DT_INST_PROP(0, cpha),
	     "Invalid combination of \"cpol\" and \"cpha\" properties.");

/* for accessing devicetree properties of the bus node */
#define QSPI_NODE DT_INST_BUS(0)
#define QSPI_PROP_AT(prop, idx) DT_PROP_BY_IDX(QSPI_NODE, prop, idx)
#define QSPI_PROP_LEN(prop) DT_PROP_LEN(QSPI_NODE, prop)

#define INST_0_QER _CONCAT(JESD216_DW15_QER_VAL_, \
			   DT_STRING_TOKEN(DT_DRV_INST(0), \
					   quad_enable_requirements))

#define IS_EQUAL(x, y) ((x) == (y))
#define SR1_WRITE_CLEARS_SR2 IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S2B1v1)

#define SR2_WRITE_NEEDS_SR1  (IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S2B1v1) || \
			      IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S2B1v4) || \
			      IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S2B1v5))

#define QER_IS_S2B1 (IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S2B1v1) || \
		     IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S2B1v4) || \
		     IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S2B1v5) || \
		     IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S2B1v6))

BUILD_ASSERT((IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_NONE)
	      || IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S1B6)
	      || IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S2B1v1)
	      || IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S2B1v4)
	      || IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S2B1v5)
	      || IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S2B1v6)),
	     "Driver only supports NONE, S1B6, S2B1v1, S2B1v4, S2B1v5 or S2B1v6 for quad-enable-requirements");

#define INST_0_4BA DT_INST_PROP_OR(0, enter_4byte_addr, 0)
#if (INST_0_4BA != 0)
BUILD_ASSERT(((INST_0_4BA & 0x03) != 0),
	     "Driver only supports command (0xB7) for entering 4 byte addressing mode");
BUILD_ASSERT(DT_INST_PROP(0, address_size_32),
	    "After entering 4 byte addressing mode, 4 byte addressing is expected");
#endif

void z_impl_nrf_qspi_nor_xip_enable(const struct device *dev, bool enable);
void z_vrfy_nrf_qspi_nor_xip_enable(const struct device *dev, bool enable);

#define WORD_SIZE 4

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

static int qspi_nor_write_protection_set(const struct device *dev,
					 bool write_protect);

static int exit_dpd(const struct device *const dev);

/**
 * @brief Test whether offset is aligned.
 */
#define QSPI_IS_SECTOR_ALIGNED(_ofs) (((_ofs) & (QSPI_SECTOR_SIZE - 1U)) == 0)
#define QSPI_IS_BLOCK_ALIGNED(_ofs) (((_ofs) & (QSPI_BLOCK_SIZE - 1U)) == 0)

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

static inline void qspi_lock(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct qspi_nor_data *dev_data = dev->data;

	k_sem_take(&dev_data->sem, K_FOREVER);
#endif
}

static inline void qspi_unlock(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct qspi_nor_data *dev_data = dev->data;

	k_sem_give(&dev_data->sem);
#endif
}

static inline void qspi_clock_div_change(void)
{
#ifdef CONFIG_SOC_SERIES_NRF53X
	/* Make sure the base clock divider is changed accordingly
	 * before a QSPI transfer is performed.
	 */
	nrf_clock_hfclk192m_div_set(NRF_CLOCK, BASE_CLOCK_DIV);
	k_busy_wait(BASE_CLOCK_SWITCH_DELAY_US);
#endif
}

static inline void qspi_clock_div_restore(void)
{
#ifdef CONFIG_SOC_SERIES_NRF53X
	/* Restore the default base clock divider to reduce power
	 * consumption when the QSPI peripheral is idle.
	 */
	nrf_clock_hfclk192m_div_set(NRF_CLOCK, NRF_CLOCK_HFCLK_DIV_4);
#endif
}

static void qspi_acquire(const struct device *dev)
{
	struct qspi_nor_data *dev_data = dev->data;
	int rc;

	rc = pm_device_runtime_get(dev);
	if (rc < 0) {
		LOG_ERR("pm_device_runtime_get failed: %d", rc);
	}
#if defined(CONFIG_MULTITHREADING)
	/* In multithreading, the driver can call qspi_acquire more than once
	 * before calling qspi_release. Keeping count, so QSPI is deactivated
	 * only at the last call (usage_count == 0).
	 */
	atomic_inc(&dev_data->usage_count);
#endif

	qspi_lock(dev);

	if (!dev_data->xip_enabled) {
		qspi_clock_div_change();

		pm_device_busy_set(dev);
	}
}

static void qspi_release(const struct device *dev)
{
	struct qspi_nor_data *dev_data = dev->data;
	bool deactivate = true;
	int rc;

#if defined(CONFIG_MULTITHREADING)
	/* The last thread to finish using the driver deactivates the QSPI */
	deactivate = atomic_dec(&dev_data->usage_count) == 1;
#endif

	if (!dev_data->xip_enabled) {
		qspi_clock_div_restore();

		if (deactivate) {
			(void) nrfx_qspi_deactivate();
		}

		pm_device_busy_clear(dev);
	}

	qspi_unlock(dev);

	rc = pm_device_runtime_put(dev);
	if (rc < 0) {
		LOG_ERR("pm_device_runtime_put failed: %d", rc);
	}
}

static inline void qspi_wait_for_completion(const struct device *dev,
					    nrfx_err_t res)
{
	struct qspi_nor_data *dev_data = dev->data;

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
		qspi_complete(dev_data);
	}
}

/* QSPI send custom command.
 *
 * If this is used for both send and receive the buffer sizes must be
 * equal and cover the whole transaction.
 */
static int qspi_send_cmd(const struct device *dev, const struct qspi_cmd *cmd,
			 bool wren)
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
		LOG_WRN("cinstr %02x transfer too long: %zu",
			cmd->op_code, xfer_len);
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

	int res = nrfx_qspi_cinstr_xfer(&cinstr_cfg, tx_buf, rx_buf);

	return qspi_get_zephyr_ret_code(res);
}

#if !IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_NONE)
/* RDSR.  Negative value is error. */
static int qspi_rdsr(const struct device *dev, uint8_t sr_num)
{
	uint8_t opcode = SPI_NOR_CMD_RDSR;

	if (sr_num > 2 || sr_num == 0) {
		return -EINVAL;
	}
	if (sr_num == 2) {
		opcode = SPI_NOR_CMD_RDSR2;
	}
	uint8_t sr = 0xFF;
	const struct qspi_buf sr_buf = {
		.buf = &sr,
		.len = sizeof(sr),
	};
	struct qspi_cmd cmd = {
		.op_code = opcode,
		.rx_buf = &sr_buf,
	};
	int rc = qspi_send_cmd(dev, &cmd, false);

	return (rc < 0) ? rc : sr;
}

/* Wait until RDSR confirms write is not in progress. */
static int qspi_wait_while_writing(const struct device *dev, k_timeout_t poll_period)
{
	int rc;

	do {
#ifdef CONFIG_MULTITHREADING
		if (!K_TIMEOUT_EQ(poll_period, K_NO_WAIT)) {
			k_sleep(poll_period);
		}
#endif
		rc = qspi_rdsr(dev, 1);
	} while ((rc >= 0)
		 && ((rc & SPI_NOR_WIP_BIT) != 0U));

	return (rc < 0) ? rc : 0;
}

static int qspi_wrsr(const struct device *dev, uint8_t sr_val, uint8_t sr_num)
{
	int rc = 0;
	uint8_t opcode = SPI_NOR_CMD_WRSR;
	uint8_t length = 1;
	uint8_t sr_array[2] = {0};

	if (sr_num > 2 || sr_num == 0) {
		return -EINVAL;
	}

	if (sr_num == 1) {
		sr_array[0] = sr_val;
#if SR1_WRITE_CLEARS_SR2
		/* Writing sr1 clears sr2. need to read/modify/write both. */
		rc = qspi_rdsr(dev, 2);
		if (rc < 0) {
			LOG_ERR("RDSR for WRSR failed: %d", rc);
			return rc;
		}
		sr_array[1] = rc;
		length = 2;
#endif
	} else { /* sr_num == 2 */

#if SR2_WRITE_NEEDS_SR1
		/* Writing sr2 requires writing sr1 as well.
		 * Uses standard WRSR opcode
		 */
		sr_array[1] = sr_val;
		rc = qspi_rdsr(dev, 1);
		if (rc < 0) {
			LOG_ERR("RDSR for WRSR failed: %d", rc);
			return rc;
		}
		sr_array[0] = rc;
		length = 2;
#elif IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S2B1v6)
		/* Writing sr2 uses a dedicated WRSR2 command */
		sr_array[0] = sr_val;
		opcode = SPI_NOR_CMD_WRSR2;
#else
		LOG_ERR("Attempted to write status register 2, but no known method to write sr2");
		return -EINVAL;
#endif
	}

	const struct qspi_buf sr_buf = {
		.buf = sr_array,
		.len = length,
	};
	struct qspi_cmd cmd = {
		.op_code = opcode,
		.tx_buf = &sr_buf,
	};

	rc = qspi_send_cmd(dev, &cmd, true);

	/* Writing SR can take some time, and further
	 * commands sent while it's happening can be
	 * corrupted.  Wait.
	 */
	if (rc == 0) {
		rc = qspi_wait_while_writing(dev, K_NO_WAIT);
	}

	return rc;
}
#endif /* !IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_NONE) */

/* QSPI erase */
static int qspi_erase(const struct device *dev, uint32_t addr, uint32_t size)
{
	const struct qspi_nor_config *params = dev->config;
	int rc, rc2;

	rc = qspi_nor_write_protection_set(dev, false);
	if (rc != 0) {
		return rc;
	}
	while (size > 0) {
		nrfx_err_t res = !NRFX_SUCCESS;
		uint32_t adj = 0;

		if (size == params->size) {
			/* chip erase */
			res = nrfx_qspi_chip_erase();
			adj = size;
		} else if ((size >= QSPI_BLOCK_SIZE) &&
			   QSPI_IS_BLOCK_ALIGNED(addr)) {
			/* 64 kB block erase */
			res = nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_64KB, addr);
			adj = QSPI_BLOCK_SIZE;
		} else if ((size >= QSPI_SECTOR_SIZE) &&
			   QSPI_IS_SECTOR_ALIGNED(addr)) {
			/* 4kB sector erase */
			res = nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_4KB, addr);
			adj = QSPI_SECTOR_SIZE;
		} else {
			/* minimal erase size is at least a sector size */
			LOG_ERR("unsupported at 0x%lx size %zu", (long)addr, size);
			res = NRFX_ERROR_INVALID_PARAM;
		}

		qspi_wait_for_completion(dev, res);
		if (res == NRFX_SUCCESS) {
			addr += adj;
			size -= adj;

			/* Erasing flash pages takes a significant period of time and the
			 * flash memory is unavailable to perform additional operations
			 * until done.
			 */
			rc = qspi_wait_while_writing(dev, K_MSEC(10));
			if (rc < 0) {
				LOG_ERR("wait error at 0x%lx size %zu", (long)addr, size);
				break;
			}
		} else {
			LOG_ERR("erase error at 0x%lx size %zu", (long)addr, size);
			rc = qspi_get_zephyr_ret_code(res);
			break;
		}
	}

	rc2 = qspi_nor_write_protection_set(dev, true);

	return rc != 0 ? rc : rc2;
}

static int configure_chip(const struct device *dev)
{
	const struct qspi_nor_config *dev_config = dev->config;
	int rc = 0;

	/* Set QE to match transfer mode.  If not using quad
	 * it's OK to leave QE set, but doing so prevents use
	 * of WP#/RESET#/HOLD# which might be useful.
	 *
	 * Note build assert above ensures QER is S1B6 or
	 * S2B1v1/4/5/6. Other options require more logic.
	 */
#if !IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_NONE)
		nrf_qspi_prot_conf_t const *prot_if =
			&dev_config->nrfx_cfg.prot_if;
		bool qe_value = (prot_if->writeoc == NRF_QSPI_WRITEOC_PP4IO) ||
				(prot_if->writeoc == NRF_QSPI_WRITEOC_PP4O)  ||
				(prot_if->readoc == NRF_QSPI_READOC_READ4IO) ||
				(prot_if->readoc == NRF_QSPI_READOC_READ4O)  ||
				(prot_if->readoc == NRF_QSPI_READOC_READ2IO);
		uint8_t sr_num = 0;
		uint8_t qe_mask = 0;

#if IS_EQUAL(INST_0_QER, JESD216_DW15_QER_VAL_S1B6)
		sr_num = 1;
		qe_mask = BIT(6);
#elif QER_IS_S2B1
		sr_num = 2;
		qe_mask = BIT(1);
#else
		LOG_ERR("Unsupported QER type");
		return -EINVAL;
#endif

		rc = qspi_rdsr(dev, sr_num);
		if (rc < 0) {
			LOG_ERR("RDSR failed: %d", rc);
			return rc;
		}

		uint8_t sr = (uint8_t)rc;
		bool qe_state = ((sr & qe_mask) != 0U);

		LOG_DBG("RDSR %02x QE %d need %d: %s", sr, qe_state, qe_value,
			(qe_state != qe_value) ? "updating" : "no-change");

		rc = 0;
		if (qe_state != qe_value) {
			sr ^= qe_mask;
			rc = qspi_wrsr(dev, sr, sr_num);
		}

		if (rc < 0) {
			LOG_ERR("QE %s failed: %d", qe_value ? "set" : "clear",
				rc);
			return rc;
		}
#endif

	if (INST_0_4BA != 0) {
		struct qspi_cmd cmd = {
			.op_code = SPI_NOR_CMD_4BA,
		};

		/* Call will send write enable before instruction if that
		 * requirement is encoded in INST_0_4BA.
		 */
		rc = qspi_send_cmd(dev, &cmd, (INST_0_4BA & 0x02));

		if (rc < 0) {
			LOG_ERR("E4BA cmd issue failed: %d.", rc);
		} else {
			LOG_DBG("E4BA cmd issued.");
		}
	}

	return rc;
}

static int qspi_rdid(const struct device *dev, uint8_t *id)
{
	const struct qspi_buf rx_buf = {
		.buf = id,
		.len = 3
	};
	const struct qspi_cmd cmd = {
		.op_code = SPI_NOR_CMD_RDID,
		.rx_buf = &rx_buf,
	};

	return qspi_send_cmd(dev, &cmd, false);
}

#if defined(CONFIG_FLASH_JESD216_API)

static int qspi_read_jedec_id(const struct device *dev, uint8_t *id)
{
	int rc;

	qspi_acquire(dev);

	rc = qspi_rdid(dev, id);

	qspi_release(dev);

	return rc;
}

static int qspi_sfdp_read(const struct device *dev, off_t offset,
			  void *data, size_t len)
{
	__ASSERT(data != NULL, "null destination");

	uint8_t addr_buf[] = {
		offset >> 16,
		offset >> 8,
		offset,
		0,		/* wait state */
	};
	nrf_qspi_cinstr_conf_t cinstr_cfg = {
		.opcode = JESD216_CMD_READ_SFDP,
		.length = NRF_QSPI_CINSTR_LEN_1B,
		.io2_level = true,
		.io3_level = true,
	};
	nrfx_err_t res;

	qspi_acquire(dev);

	res = nrfx_qspi_lfm_start(&cinstr_cfg);
	if (res != NRFX_SUCCESS) {
		LOG_DBG("lfm_start: %x", res);
		goto out;
	}

	res = nrfx_qspi_lfm_xfer(addr_buf, NULL, sizeof(addr_buf), false);
	if (res != NRFX_SUCCESS) {
		LOG_DBG("lfm_xfer addr: %x", res);
		goto out;
	}

	res = nrfx_qspi_lfm_xfer(NULL, data, len, true);
	if (res != NRFX_SUCCESS) {
		LOG_DBG("lfm_xfer read: %x", res);
		goto out;
	}

out:
	qspi_release(dev);

	return qspi_get_zephyr_ret_code(res);
}

#endif /* CONFIG_FLASH_JESD216_API */

static inline nrfx_err_t read_non_aligned(const struct device *dev,
					  off_t addr,
					  void *dest, size_t size)
{
	uint8_t __aligned(WORD_SIZE) buf[WORD_SIZE * 2];
	uint8_t *dptr = dest;

	off_t flash_prefix = (WORD_SIZE - (addr % WORD_SIZE)) % WORD_SIZE;

	if (flash_prefix > size) {
		flash_prefix = size;
	}

	off_t dest_prefix = (WORD_SIZE - (off_t)dptr % WORD_SIZE) % WORD_SIZE;

	if (dest_prefix > size) {
		dest_prefix = size;
	}

	off_t flash_suffix = (size - flash_prefix) % WORD_SIZE;
	off_t flash_middle = size - flash_prefix - flash_suffix;
	off_t dest_middle = size - dest_prefix -
			    (size - dest_prefix) % WORD_SIZE;

	if (flash_middle > dest_middle) {
		flash_middle = dest_middle;
		flash_suffix = size - flash_prefix - flash_middle;
	}

	nrfx_err_t res = NRFX_SUCCESS;

	/* read from aligned flash to aligned memory */
	if (flash_middle != 0) {
		res = nrfx_qspi_read(dptr + dest_prefix, flash_middle,
				     addr + flash_prefix);
		qspi_wait_for_completion(dev, res);
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
		res = nrfx_qspi_read(buf, WORD_SIZE, addr -
				     (WORD_SIZE - flash_prefix));
		qspi_wait_for_completion(dev, res);
		if (res != NRFX_SUCCESS) {
			return res;
		}
		memcpy(dptr, buf + WORD_SIZE - flash_prefix, flash_prefix);
	}

	/* read suffix */
	if (flash_suffix != 0) {
		res = nrfx_qspi_read(buf, WORD_SIZE * 2,
				     addr + flash_prefix + flash_middle);
		qspi_wait_for_completion(dev, res);
		if (res != NRFX_SUCCESS) {
			return res;
		}
		memcpy(dptr + flash_prefix + flash_middle, buf, flash_suffix);
	}

	return res;
}

static int qspi_nor_read(const struct device *dev, off_t addr, void *dest,
			 size_t size)
{
	const struct qspi_nor_config *params = dev->config;
	nrfx_err_t res;

	if (!dest) {
		return -EINVAL;
	}

	/* read size must be non-zero */
	if (!size) {
		return 0;
	}

	/* affected region should be within device */
	if (addr < 0 ||
	    (addr + size) > params->size) {
		LOG_ERR("read error: address or size "
			"exceeds expected values."
			"Addr: 0x%lx size %zu", (long)addr, size);
		return -EINVAL;
	}

	qspi_acquire(dev);

	res = read_non_aligned(dev, addr, dest, size);

	qspi_release(dev);

	return qspi_get_zephyr_ret_code(res);
}

/* addr aligned, sptr not null, slen less than 4 */
static inline nrfx_err_t write_sub_word(const struct device *dev, off_t addr,
					const void *sptr, size_t slen)
{
	uint8_t __aligned(4) buf[4];
	nrfx_err_t res;

	/* read out the whole word so that unchanged data can be
	 * written back
	 */
	res = nrfx_qspi_read(buf, sizeof(buf), addr);
	qspi_wait_for_completion(dev, res);

	if (res == NRFX_SUCCESS) {
		memcpy(buf, sptr, slen);
		res = nrfx_qspi_write(buf, sizeof(buf), addr);
		qspi_wait_for_completion(dev, res);
	}

	return res;
}

BUILD_ASSERT((CONFIG_NORDIC_QSPI_NOR_STACK_WRITE_BUFFER_SIZE % 4) == 0,
	     "NOR stack buffer must be multiple of 4 bytes");

/* If enabled write using a stack-allocated aligned SRAM buffer as
 * required for DMA transfers by QSPI peripheral.
 *
 * If not enabled return the error the peripheral would have produced.
 */
static nrfx_err_t write_through_buffer(const struct device *dev, off_t addr,
				       const void *sptr, size_t slen)
{
	nrfx_err_t res = NRFX_SUCCESS;

	if (CONFIG_NORDIC_QSPI_NOR_STACK_WRITE_BUFFER_SIZE > 0) {
		uint8_t __aligned(4) buf[CONFIG_NORDIC_QSPI_NOR_STACK_WRITE_BUFFER_SIZE];
		const uint8_t *sp = sptr;

		while ((slen > 0) && (res == NRFX_SUCCESS)) {
			size_t len = MIN(slen, sizeof(buf));

			memcpy(buf, sp, len);
			res = nrfx_qspi_write(buf, len, addr);
			qspi_wait_for_completion(dev, res);

			if (res == NRFX_SUCCESS) {
				slen -= len;
				sp += len;
				addr += len;
			}
		}
	} else {
		res = NRFX_ERROR_INVALID_ADDR;
	}
	return res;
}

static int qspi_nor_write(const struct device *dev, off_t addr,
			  const void *src,
			  size_t size)
{
	const struct qspi_nor_config *params = dev->config;
	int rc, rc2;

	if (!src) {
		return -EINVAL;
	}

	/* write size must be non-zero, less than 4, or a multiple of 4 */
	if ((size == 0)
	    || ((size > 4) && ((size % 4U) != 0))) {
		return -EINVAL;
	}
	/* address must be 4-byte aligned */
	if ((addr % 4U) != 0) {
		return -EINVAL;
	}

	/* affected region should be within device */
	if (addr < 0 ||
	    (addr + size) > params->size) {
		LOG_ERR("write error: address or size "
			"exceeds expected values."
			"Addr: 0x%lx size %zu", (long)addr, size);
		return -EINVAL;
	}

	qspi_acquire(dev);

	rc = qspi_nor_write_protection_set(dev, false);
	if (rc == 0) {
		nrfx_err_t res;

		if (size < 4U) {
			res = write_sub_word(dev, addr, src, size);
		} else if (!nrfx_is_in_ram(src) ||
			   !nrfx_is_word_aligned(src)) {
			res = write_through_buffer(dev, addr, src, size);
		} else {
			res = nrfx_qspi_write(src, size, addr);
			qspi_wait_for_completion(dev, res);
		}

		rc = qspi_get_zephyr_ret_code(res);
	}

	rc2 = qspi_nor_write_protection_set(dev, true);

	qspi_release(dev);

	return rc != 0 ? rc : rc2;
}

static int qspi_nor_erase(const struct device *dev, off_t addr, size_t size)
{
	const struct qspi_nor_config *params = dev->config;
	int rc;

	/* address must be sector-aligned */
	if ((addr % QSPI_SECTOR_SIZE) != 0) {
		return -EINVAL;
	}

	/* size must be a non-zero multiple of sectors */
	if ((size == 0) || (size % QSPI_SECTOR_SIZE) != 0) {
		return -EINVAL;
	}

	/* affected region should be within device */
	if (addr < 0 ||
	    (addr + size) > params->size) {
		LOG_ERR("erase error: address or size "
			"exceeds expected values."
			"Addr: 0x%lx size %zu", (long)addr, size);
		return -EINVAL;
	}

	qspi_acquire(dev);

	rc = qspi_erase(dev, addr, size);

	qspi_release(dev);

	return rc;
}

static int qspi_nor_write_protection_set(const struct device *dev,
					 bool write_protect)
{
	int rc = 0;
	struct qspi_cmd cmd = {
		.op_code = ((write_protect) ? SPI_NOR_CMD_WRDI : SPI_NOR_CMD_WREN),
	};

	if (qspi_send_cmd(dev, &cmd, false) != 0) {
		rc = -EIO;
	}

	return rc;
}

static int qspi_init(const struct device *dev)
{
	const struct qspi_nor_config *dev_config = dev->config;
	uint8_t id[SPI_NOR_MAX_ID_LEN];
	nrfx_err_t res;
	int rc;

	res = nrfx_qspi_init(&dev_config->nrfx_cfg, qspi_handler, dev->data);
	rc = qspi_get_zephyr_ret_code(res);
	if (rc < 0) {
		return rc;
	}

#if DT_INST_NODE_HAS_PROP(0, rx_delay)
	if (!nrf53_errata_121()) {
		nrf_qspi_iftiming_set(NRF_QSPI, DT_INST_PROP(0, rx_delay));
	}
#endif

	/* It may happen that after the flash chip was previously put into
	 * the DPD mode, the system was reset but the flash chip was not.
	 * Consequently, the flash chip can be in the DPD mode at this point.
	 * Some flash chips will just exit the DPD mode on the first CS pulse,
	 * but some need to receive the dedicated command to do it, so send it.
	 * This can be the case even if the current image does not have
	 * CONFIG_PM_DEVICE set to enter DPD mode, as a previously executing image
	 * (for example the main image if the currently executing image is the
	 * bootloader) might have set DPD mode before reboot. As a result,
	 * attempt to exit DPD mode regardless of whether CONFIG_PM_DEVICE is set.
	 */
	rc = exit_dpd(dev);
	if (rc < 0) {
		return rc;
	}

	/* Retrieve the Flash JEDEC ID and compare it with the one expected. */
	rc = qspi_rdid(dev, id);
	if (rc < 0) {
		return rc;
	}

	if (memcmp(dev_config->id, id, SPI_NOR_MAX_ID_LEN) != 0) {
		LOG_ERR("JEDEC id [%02x %02x %02x] expect [%02x %02x %02x]",
			id[0], id[1], id[2], dev_config->id[0],
			dev_config->id[1], dev_config->id[2]);
		return -ENODEV;
	}

	/* The chip is correct, it can be configured now. */
	return configure_chip(dev);
}

static int qspi_nor_init(const struct device *dev)
{
	const struct qspi_nor_config *dev_config = dev->config;
	int rc;

	rc = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		return rc;
	}

	IRQ_CONNECT(DT_IRQN(QSPI_NODE), DT_IRQ(QSPI_NODE, priority),
		    nrfx_isr, nrfx_qspi_irq_handler, 0);

	qspi_clock_div_change();

	rc = qspi_init(dev);

	qspi_clock_div_restore();

	if (!IS_ENABLED(CONFIG_NORDIC_QSPI_NOR_XIP) && nrfx_qspi_init_check()) {
		(void)nrfx_qspi_deactivate();
	}

#ifdef CONFIG_NORDIC_QSPI_NOR_XIP
	if (rc == 0) {
		/* Enable XIP mode for QSPI NOR flash, this will prevent the
		 * flash from being powered down
		 */
		z_impl_nrf_qspi_nor_xip_enable(dev, true);
	}
#endif

	return rc;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)

/* instance 0 page count */
#define LAYOUT_PAGES_COUNT (INST_0_BYTES / \
			    CONFIG_NORDIC_QSPI_NOR_FLASH_LAYOUT_PAGE_SIZE)

BUILD_ASSERT((CONFIG_NORDIC_QSPI_NOR_FLASH_LAYOUT_PAGE_SIZE *
	      LAYOUT_PAGES_COUNT)
	     == INST_0_BYTES,
	     "QSPI_NOR_FLASH_LAYOUT_PAGE_SIZE incompatible with flash size");

static const struct flash_pages_layout dev_layout = {
	.pages_count = LAYOUT_PAGES_COUNT,
	.pages_size = CONFIG_NORDIC_QSPI_NOR_FLASH_LAYOUT_PAGE_SIZE,
};
#undef LAYOUT_PAGES_COUNT

static void qspi_nor_pages_layout(const struct device *dev,
				  const struct flash_pages_layout **layout,
				  size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *
qspi_flash_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	static const struct flash_parameters qspi_flash_parameters = {
		.write_block_size = 4,
		.erase_value = 0xff,
	};

	return &qspi_flash_parameters;
}

int qspi_nor_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	*size = (uint64_t)(INST_0_BYTES);

	return 0;
}

static const struct flash_driver_api qspi_nor_api = {
	.read = qspi_nor_read,
	.write = qspi_nor_write,
	.erase = qspi_nor_erase,
	.get_parameters = qspi_flash_get_parameters,
	.get_size = qspi_nor_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = qspi_nor_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = qspi_sfdp_read,
	.read_jedec_id = qspi_read_jedec_id,
#endif /* CONFIG_FLASH_JESD216_API */
};

#ifdef CONFIG_PM_DEVICE
static int enter_dpd(const struct device *const dev)
{
	if (IS_ENABLED(DT_INST_PROP(0, has_dpd))) {
		struct qspi_cmd cmd = {
			.op_code = SPI_NOR_CMD_DPD,
		};
		uint32_t t_enter_dpd = DT_INST_PROP_OR(0, t_enter_dpd, 0);
		int rc;

		rc = qspi_send_cmd(dev, &cmd, false);
		if (rc < 0) {
			return rc;
		}

		if (t_enter_dpd) {
			uint32_t t_enter_dpd_us =
				DIV_ROUND_UP(t_enter_dpd, NSEC_PER_USEC);

			k_busy_wait(t_enter_dpd_us);
		}
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static int exit_dpd(const struct device *const dev)
{
	if (IS_ENABLED(DT_INST_PROP(0, has_dpd))) {
		nrf_qspi_pins_t pins;
		nrf_qspi_pins_t disconnected_pins = {
			.sck_pin = NRF_QSPI_PIN_NOT_CONNECTED,
			.csn_pin = NRF_QSPI_PIN_NOT_CONNECTED,
			.io0_pin = NRF_QSPI_PIN_NOT_CONNECTED,
			.io1_pin = NRF_QSPI_PIN_NOT_CONNECTED,
			.io2_pin = NRF_QSPI_PIN_NOT_CONNECTED,
			.io3_pin = NRF_QSPI_PIN_NOT_CONNECTED,
		};
		struct qspi_cmd cmd = {
			.op_code = SPI_NOR_CMD_RDPD,
		};
		uint32_t t_exit_dpd = DT_INST_PROP_OR(0, t_exit_dpd, 0);
		nrfx_err_t res;
		int rc;

		nrf_qspi_pins_get(NRF_QSPI, &pins);
		nrf_qspi_pins_set(NRF_QSPI, &disconnected_pins);
		res = nrfx_qspi_activate(true);
		nrf_qspi_pins_set(NRF_QSPI, &pins);

		if (res != NRFX_SUCCESS) {
			return -EIO;
		}

		rc = qspi_send_cmd(dev, &cmd, false);
		if (rc < 0) {
			return rc;
		}

		if (t_exit_dpd) {
			uint32_t t_exit_dpd_us =
				DIV_ROUND_UP(t_exit_dpd, NSEC_PER_USEC);

			k_busy_wait(t_exit_dpd_us);
		}
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int qspi_suspend(const struct device *dev)
{
	const struct qspi_nor_config *dev_config = dev->config;
	nrfx_err_t res;
	int rc;

	res = nrfx_qspi_mem_busy_check();
	if (res != NRFX_SUCCESS) {
		return -EBUSY;
	}

	rc = enter_dpd(dev);
	if (rc < 0) {
		return rc;
	}

	nrfx_qspi_uninit();

	return pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_SLEEP);
}

static int qspi_resume(const struct device *dev)
{
	const struct qspi_nor_config *dev_config = dev->config;
	nrfx_err_t res;
	int rc;

	rc = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		return rc;
	}

	res = nrfx_qspi_init(&dev_config->nrfx_cfg, qspi_handler, dev->data);
	if (res != NRFX_SUCCESS) {
		return -EIO;
	}

	return exit_dpd(dev);
}

static int qspi_nor_pm_action(const struct device *dev,
			      enum pm_device_action action)
{
	int rc;

	if (pm_device_is_busy(dev)) {
		return -EBUSY;
	}

	qspi_lock(dev);
	qspi_clock_div_change();

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		rc = qspi_suspend(dev);
		break;

	case PM_DEVICE_ACTION_RESUME:
		rc = qspi_resume(dev);
		break;

	default:
		rc = -ENOTSUP;
	}

	qspi_clock_div_restore();
	qspi_unlock(dev);

	return rc;
}
#endif /* CONFIG_PM_DEVICE */

void z_impl_nrf_qspi_nor_xip_enable(const struct device *dev, bool enable)
{
	struct qspi_nor_data *dev_data = dev->data;

	if (dev_data->xip_enabled == enable) {
		return;
	}

	qspi_acquire(dev);

#if NRF_QSPI_HAS_XIPEN
	nrf_qspi_xip_set(NRF_QSPI, enable);
#endif
	if (enable) {
		(void)nrfx_qspi_activate(false);
	}
	dev_data->xip_enabled = enable;

	qspi_release(dev);
}

#ifdef CONFIG_USERSPACE
#include <zephyr/internal/syscall_handler.h>

void z_vrfy_nrf_qspi_nor_xip_enable(const struct device *dev, bool enable)
{
	K_OOPS(K_SYSCALL_SPECIFIC_DRIVER(dev, K_OBJ_DRIVER_FLASH,
					 &qspi_nor_api));

	z_impl_nrf_qspi_nor_xip_enable(dev, enable);
}

#include <zephyr/syscalls/nrf_qspi_nor_xip_enable_mrsh.c>
#endif /* CONFIG_USERSPACE */

static struct qspi_nor_data qspi_nor_dev_data = {
#ifdef CONFIG_MULTITHREADING
	.sem = Z_SEM_INITIALIZER(qspi_nor_dev_data.sem, 1, 1),
	.sync = Z_SEM_INITIALIZER(qspi_nor_dev_data.sync, 0, 1),
#endif /* CONFIG_MULTITHREADING */
};

NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(QSPI_NODE);

PINCTRL_DT_DEFINE(QSPI_NODE);

static const struct qspi_nor_config qspi_nor_dev_config = {
	.nrfx_cfg.skip_gpio_cfg = true,
	.nrfx_cfg.skip_psel_cfg = true,
	.pcfg = PINCTRL_DT_DEV_CONFIG_GET(QSPI_NODE),
	.nrfx_cfg.prot_if = {
		.readoc = COND_CODE_1(DT_INST_NODE_HAS_PROP(0, readoc),
			(_CONCAT(NRF_QSPI_READOC_,
				 DT_STRING_UPPER_TOKEN(DT_DRV_INST(0),
						       readoc))),
			(NRF_QSPI_READOC_FASTREAD)),
		.writeoc = COND_CODE_1(DT_INST_NODE_HAS_PROP(0, writeoc),
			(_CONCAT(NRF_QSPI_WRITEOC_,
				 DT_STRING_UPPER_TOKEN(DT_DRV_INST(0),
						       writeoc))),
			(NRF_QSPI_WRITEOC_PP)),
		.addrmode = DT_INST_PROP(0, address_size_32)
			    ? NRF_QSPI_ADDRMODE_32BIT
			    : NRF_QSPI_ADDRMODE_24BIT,
	},
	.nrfx_cfg.phy_if = {
		.sck_freq = INST_0_SCK_CFG,
		.sck_delay = DT_INST_PROP(0, sck_delay),
		.spi_mode = INST_0_SPI_MODE,
	},
	.nrfx_cfg.timeout = CONFIG_NORDIC_QSPI_NOR_TIMEOUT_MS,

	.size = INST_0_BYTES,
	.id = DT_INST_PROP(0, jedec_id),
};

PM_DEVICE_DT_INST_DEFINE(0, qspi_nor_pm_action);

DEVICE_DT_INST_DEFINE(0, qspi_nor_init, PM_DEVICE_DT_INST_GET(0),
		      &qspi_nor_dev_data, &qspi_nor_dev_config,
		      POST_KERNEL, CONFIG_NORDIC_QSPI_NOR_INIT_PRIORITY,
		      &qspi_nor_api);
