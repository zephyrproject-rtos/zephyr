/*
 * Copyright (c) 2019-2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_qspi_nor

#include <errno.h>
#include <drivers/flash.h>
#include <init.h>
#include <pm/device.h>
#include <string.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(qspi_nor, CONFIG_FLASH_LOG_LEVEL);

#include "spi_nor.h"
#include "jesd216.h"
#include "flash_priv.h"
#include <nrfx_qspi.h>
#include <hal/nrf_clock.h>

struct qspi_nor_data {
#ifdef CONFIG_MULTITHREADING
	/* The semaphore to control exclusive access on write/erase. */
	struct k_sem trans;
	/* The semaphore to control exclusive access to the device. */
	struct k_sem sem;
	/* The semaphore to indicate that transfer has completed. */
	struct k_sem sync;
#if NRF52_ERRATA_122_PRESENT
	/* The semaphore to control driver init/uninit. */
	struct k_sem count;
#endif
#else /* CONFIG_MULTITHREADING */
	/* A flag that signals completed transfer when threads are
	 * not enabled.
	 */
	volatile bool ready;
#endif /* CONFIG_MULTITHREADING */
};

struct qspi_nor_config {
	nrfx_qspi_config_t nrfx_cfg;

	/* Size from devicetree, in bytes */
	uint32_t size;

	/* JEDEC id from devicetree */
	uint8_t id[SPI_NOR_MAX_ID_LEN];
};

/* Status register bits */
#define QSPI_SECTOR_SIZE SPI_NOR_SECTOR_SIZE
#define QSPI_BLOCK_SIZE SPI_NOR_BLOCK_SIZE

/* instance 0 flash size in bytes */
#define INST_0_BYTES (DT_INST_PROP(0, size) / 8)

#define INST_0_SCK_FREQUENCY DT_INST_PROP(0, sck_frequency)
BUILD_ASSERT(INST_0_SCK_FREQUENCY >= (NRF_QSPI_BASE_CLOCK_FREQ / 16),
	     "Unsupported SCK frequency.");

/* 0 for MODE0 (CPOL=0, CPHA=0), 1 for MODE3 (CPOL=1, CPHA=1). */
#define INST_0_SPI_MODE DT_INST_PROP(0, cpol)
BUILD_ASSERT(DT_INST_PROP(0, cpol) == DT_INST_PROP(0, cpha),
	     "Invalid combination of \"cpol\" and \"cpha\" properties.");

/* for accessing devicetree properties of the bus node */
#define QSPI_NODE DT_BUS(DT_DRV_INST(0))
#define QSPI_PROP_AT(prop, idx) DT_PROP_BY_IDX(QSPI_NODE, prop, idx)
#define QSPI_PROP_LEN(prop) DT_PROP_LEN(QSPI_NODE, prop)

#define INST_0_QER _CONCAT(JESD216_DW15_QER_, \
			   DT_STRING_TOKEN(DT_DRV_INST(0), \
					   quad_enable_requirements))

BUILD_ASSERT(((INST_0_QER == JESD216_DW15_QER_NONE)
	      || (INST_0_QER == JESD216_DW15_QER_S1B6)),
	     "Driver only supports NONE or S1B6 for quad-enable-requirements");

#if NRF52_ERRATA_122_PRESENT
#include <hal/nrf_gpio.h>
static int anomaly_122_init(const struct device *dev);
static void anomaly_122_uninit(const struct device *dev);

#define ANOMALY_122_INIT(dev)   anomaly_122_init(dev)
#define ANOMALY_122_UNINIT(dev) anomaly_122_uninit(dev)
#else
#define ANOMALY_122_INIT(dev) 0
#define ANOMALY_122_UNINIT(dev)
#endif

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
#else /* CONFIG_MULTITHREADING */
	ARG_UNUSED(dev);
#endif /* CONFIG_MULTITHREADING */
}

static inline void qspi_unlock(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct qspi_nor_data *dev_data = dev->data;

	k_sem_give(&dev_data->sem);
#else /* CONFIG_MULTITHREADING */
	ARG_UNUSED(dev);
#endif /* CONFIG_MULTITHREADING */
}

static inline void qspi_trans_lock(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct qspi_nor_data *dev_data = dev->data;

	k_sem_take(&dev_data->trans, K_FOREVER);
#else /* CONFIG_MULTITHREADING */
	ARG_UNUSED(dev);
#endif /* CONFIG_MULTITHREADING */
}

static inline void qspi_trans_unlock(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct qspi_nor_data *dev_data = dev->data;

	k_sem_give(&dev_data->trans);
#else /* CONFIG_MULTITHREADING */
	ARG_UNUSED(dev);
#endif /* CONFIG_MULTITHREADING */
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

#if NRF52_ERRATA_122_PRESENT
static bool qspi_initialized;

static int anomaly_122_init(const struct device *dev)
{
	struct qspi_nor_data *dev_data = dev->data;
	nrfx_err_t res;
	int ret = 0;

	if (!nrf52_errata_122()) {
		return 0;
	}

	qspi_lock(dev);

	/* In multithreading, driver can call anomaly_122_init more than once
	 * before calling anomaly_122_uninit. Keepping count, so QSPI is
	 * uninitialized only at the last call (count == 0).
	 */
#ifdef CONFIG_MULTITHREADING
	k_sem_give(&dev_data->count);
#endif

	if (!qspi_initialized) {
		const struct qspi_nor_config *dev_config = dev->config;

		res = nrfx_qspi_init(&dev_config->nrfx_cfg,
				     qspi_handler,
				     dev_data);
		ret = qspi_get_zephyr_ret_code(res);
		qspi_initialized = (ret == 0);
	}

	qspi_unlock(dev);

	return ret;
}

static void anomaly_122_uninit(const struct device *dev)
{
	bool last = true;

	if (!nrf52_errata_122()) {
		return;
	}

	qspi_lock(dev);

#ifdef CONFIG_MULTITHREADING
	struct qspi_nor_data *dev_data = dev->data;

	/* The last thread to finish using the driver uninit the QSPI */
	(void) k_sem_take(&dev_data->count, K_NO_WAIT);
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

		nrf_gpio_cfg_output(QSPI_PROP_AT(csn_pins, 0));
		nrf_gpio_pin_set(QSPI_PROP_AT(csn_pins, 0));

		nrfx_qspi_uninit();
		qspi_initialized = false;
	}

	qspi_unlock(dev);
}
#endif /* NRF52_ERRATA_122_PRESENT */


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
	} while ((ret >= 0)
		 && ((ret & SPI_NOR_WIP_BIT) != 0U));

	return (ret < 0) ? ret : 0;
}

/* QSPI erase */
static int qspi_erase(const struct device *dev, uint32_t addr, uint32_t size)
{
	/* address must be sector-aligned */
	if ((addr % QSPI_SECTOR_SIZE) != 0) {
		return -EINVAL;
	}

	/* size must be a non-zero multiple of sectors */
	if ((size == 0) || (size % QSPI_SECTOR_SIZE) != 0) {
		return -EINVAL;
	}

	int rv = 0;
	const struct qspi_nor_config *params = dev->config;

	rv = ANOMALY_122_INIT(dev);
	if (rv != 0) {
		goto out;
	}
	qspi_trans_lock(dev);
	rv = qspi_nor_write_protection_set(dev, false);
	if (rv != 0) {
		goto out_trans_unlock;
	}
	qspi_lock(dev);
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
		} else {
			LOG_ERR("erase error at 0x%lx size %zu", (long)addr, size);
			rv = qspi_get_zephyr_ret_code(res);
			break;
		}
	}
	qspi_unlock(dev);

	int rv2 = qspi_nor_write_protection_set(dev, true);

	if (!rv) {
		rv = rv2;
	}

out_trans_unlock:
	qspi_trans_unlock(dev);

out:
	ANOMALY_122_UNINIT(dev);
	return rv;
}

/* Configures QSPI memory for the transfer */
static int qspi_nrfx_configure(const struct device *dev)
{
	struct qspi_nor_data *dev_data = dev->data;
	const struct qspi_nor_config *dev_config = dev->config;

	nrfx_err_t res = nrfx_qspi_init(&dev_config->nrfx_cfg,
					qspi_handler,
					dev_data);
	int ret = qspi_get_zephyr_ret_code(res);
	if (ret < 0) {
		return ret;
	}

#if DT_INST_NODE_HAS_PROP(0, rx_delay)
	if (!nrf53_errata_121()) {
		nrf_qspi_iftiming_set(NRF_QSPI, DT_INST_PROP(0, rx_delay));
	}
#endif

	if (INST_0_QER != JESD216_DW15_QER_NONE) {
		/* Set QE to match transfer mode.  If not using quad
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
		nrf_qspi_prot_conf_t const *prot_if =
			&dev_config->nrfx_cfg.prot_if;
		bool qe_value = (prot_if->writeoc == NRF_QSPI_WRITEOC_PP4IO) ||
				(prot_if->writeoc == NRF_QSPI_WRITEOC_PP4O)  ||
				(prot_if->readoc == NRF_QSPI_READOC_READ4IO) ||
				(prot_if->readoc == NRF_QSPI_READOC_READ4O);
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
			LOG_ERR("QE %s failed: %d", qe_value ? "set" : "clear",
				ret);
			return ret;
		}
	}

	return 0;
}

static int qspi_read_jedec_id(const struct device *dev,
				     uint8_t *id)
{
	const struct qspi_buf rx_buf = {
		.buf = id,
		.len = 3
	};
	const struct qspi_cmd cmd = {
		.op_code = SPI_NOR_CMD_RDID,
		.rx_buf = &rx_buf,
	};

	int ret = ANOMALY_122_INIT(dev);

	if (ret == 0) {
		ret = qspi_send_cmd(dev, &cmd, false);
	}
	ANOMALY_122_UNINIT(dev);

	return ret;
}

#if defined(CONFIG_FLASH_JESD216_API)

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

	int ret = ANOMALY_122_INIT(dev);
	nrfx_err_t res = NRFX_SUCCESS;

	if (ret != 0) {
		LOG_DBG("ANOMALY_122_INIT: %d", ret);
		ANOMALY_122_UNINIT(dev);
		return ret;
	}

	qspi_lock(dev);
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
	qspi_unlock(dev);
	ANOMALY_122_UNINIT(dev);
	return qspi_get_zephyr_ret_code(res);
}

#endif /* CONFIG_FLASH_JESD216_API */

/**
 * @brief Retrieve the Flash JEDEC ID and compare it with the one expected
 *
 * @param dev The device structure
 * @return 0 on success, negative errno code otherwise
 */
static inline int qspi_nor_read_id(const struct device *dev)
{
	uint8_t id[SPI_NOR_MAX_ID_LEN];
	int ret = qspi_read_jedec_id(dev, id);

	if (ret != 0) {
		return -EIO;
	}

	const struct qspi_nor_config *qnc = dev->config;

	if (memcmp(qnc->id, id, SPI_NOR_MAX_ID_LEN) != 0) {
		LOG_ERR("JEDEC id [%02x %02x %02x] expect [%02x %02x %02x]",
			id[0], id[1], id[2],
			qnc->id[0], qnc->id[1], qnc->id[2]);
		return -ENODEV;
	}

	return 0;
}

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
	if (!dest) {
		return -EINVAL;
	}

	/* read size must be non-zero */
	if (!size) {
		return 0;
	}

	const struct qspi_nor_config *params = dev->config;

	/* affected region should be within device */
	if (addr < 0 ||
	    (addr + size) > params->size) {
		LOG_ERR("read error: address or size "
			"exceeds expected values."
			"Addr: 0x%lx size %zu", (long)addr, size);
		return -EINVAL;
	}

	int rc = ANOMALY_122_INIT(dev);

	if (rc != 0) {
		goto out;
	}

	qspi_lock(dev);

	nrfx_err_t res = read_non_aligned(dev, addr, dest, size);

	qspi_unlock(dev);

	rc = qspi_get_zephyr_ret_code(res);

out:
	ANOMALY_122_UNINIT(dev);
	return rc;
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

#define NVMC_WRITE_OK (CONFIG_NORDIC_QSPI_NOR_STACK_WRITE_BUFFER_SIZE > 0)

/* If enabled write using a stack-allocated aligned SRAM buffer as
 * required for DMA transfers by QSPI peripheral.
 *
 * If not enabled return the error the peripheral would have produced.
 */
static inline nrfx_err_t write_from_nvmc(const struct device *dev, off_t addr,
					 const void *sptr, size_t slen)
{
#if NVMC_WRITE_OK
	uint8_t __aligned(4) buf[CONFIG_NORDIC_QSPI_NOR_STACK_WRITE_BUFFER_SIZE];
	const uint8_t *sp = sptr;
	nrfx_err_t res = NRFX_SUCCESS;

	while ((slen > 0) && (res == NRFX_SUCCESS)) {
		size_t len = MIN(slen, sizeof(buf));

		memcpy(buf, sp, len);
		res = nrfx_qspi_write(buf, sizeof(buf),
				      addr);
		qspi_wait_for_completion(dev, res);

		if (res == NRFX_SUCCESS) {
			slen -= len;
			sp += len;
			addr += len;
		}
	}
#else /* NVMC_WRITE_OK */
	nrfx_err_t res = NRFX_ERROR_INVALID_ADDR;
#endif /* NVMC_WRITE_OK */
	return res;
}

static int qspi_nor_write(const struct device *dev, off_t addr,
			  const void *src,
			  size_t size)
{
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

	const struct qspi_nor_config *params = dev->config;

	/* affected region should be within device */
	if (addr < 0 ||
	    (addr + size) > params->size) {
		LOG_ERR("write error: address or size "
			"exceeds expected values."
			"Addr: 0x%lx size %zu", (long)addr, size);
		return -EINVAL;
	}

	nrfx_err_t res = NRFX_SUCCESS;

	int rc = ANOMALY_122_INIT(dev);

	if (rc != 0) {
		goto out;
	}

	qspi_trans_lock(dev);
	res = qspi_nor_write_protection_set(dev, false);
	qspi_lock(dev);
	if (!res) {
		if (size < 4U) {
			res = write_sub_word(dev, addr, src, size);
		} else if (!nrfx_is_in_ram(src)) {
			res = write_from_nvmc(dev, addr, src, size);
		} else {
			res = nrfx_qspi_write(src, size, addr);
			qspi_wait_for_completion(dev, res);
		}
	}
	qspi_unlock(dev);

	int res2 = qspi_nor_write_protection_set(dev, true);

	qspi_trans_unlock(dev);
	if (!res) {
		res = res2;
	}

	rc = qspi_get_zephyr_ret_code(res);
out:
	ANOMALY_122_UNINIT(dev);
	return rc;
}

static int qspi_nor_erase(const struct device *dev, off_t addr, size_t size)
{
	const struct qspi_nor_config *params = dev->config;

	/* affected region should be within device */
	if (addr < 0 ||
	    (addr + size) > params->size) {
		LOG_ERR("erase error: address or size "
			"exceeds expected values."
			"Addr: 0x%lx size %zu", (long)addr, size);
		return -EINVAL;
	}

	int ret = qspi_erase(dev, addr, size);

	return ret;
}

static int qspi_nor_write_protection_set(const struct device *dev,
					 bool write_protect)
{
	int ret = 0;
	struct qspi_cmd cmd = {
		.op_code = ((write_protect) ? SPI_NOR_CMD_WRDI : SPI_NOR_CMD_WREN),
	};

	if (qspi_send_cmd(dev, &cmd, false) != 0) {
		ret = -EIO;
	}

	return ret;
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

	ANOMALY_122_UNINIT(dev);

	/* now the spi bus is configured, we can verify the flash id */
	if (qspi_nor_read_id(dev) != 0) {
		return -ENODEV;
	}

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
#if defined(CONFIG_SOC_SERIES_NRF53X)
	/* Make sure the PCLK192M clock, from which the SCK frequency is
	 * derived, is not prescaled (the default setting after reset is
	 * "divide by 4").
	 */
	nrf_clock_hfclk192m_div_set(NRF_CLOCK, NRF_CLOCK_HFCLK_DIV_1);
#endif

	IRQ_CONNECT(DT_IRQN(QSPI_NODE), DT_IRQ(QSPI_NODE, priority),
		    nrfx_isr, nrfx_qspi_irq_handler, 0);
	return qspi_nor_configure(dev);
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

static const struct flash_driver_api qspi_nor_api = {
	.read = qspi_nor_read,
	.write = qspi_nor_write,
	.erase = qspi_nor_erase,
	.get_parameters = qspi_flash_get_parameters,
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
		int ret;

		ret = qspi_send_cmd(dev, &cmd, false);
		if (ret < 0) {
			return ret;
		}

		if (t_enter_dpd) {
			uint32_t t_enter_dpd_us =
				ceiling_fraction(t_enter_dpd, NSEC_PER_USEC);

			k_busy_wait(t_enter_dpd_us);
		}
	}

	return 0;
}

static int exit_dpd(const struct device *const dev)
{
	if (IS_ENABLED(DT_INST_PROP(0, has_dpd))) {
		struct qspi_cmd cmd = {
			.op_code = SPI_NOR_CMD_RDPD,
		};
		uint32_t t_exit_dpd = DT_INST_PROP_OR(0, t_exit_dpd, 0);
		int ret;

		ret = qspi_send_cmd(dev, &cmd, false);
		if (ret < 0) {
			return ret;
		}

		if (t_exit_dpd) {
			uint32_t t_exit_dpd_us =
				ceiling_fraction(t_exit_dpd, NSEC_PER_USEC);

			k_busy_wait(t_exit_dpd_us);
		}
	}

	return 0;
}

static int qspi_nor_pm_action(const struct device *dev,
			      enum pm_device_action action)
{
	struct qspi_nor_data *dev_data = dev->data;
	const struct qspi_nor_config *dev_config = dev->config;
	int ret;
	nrfx_err_t err;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = ANOMALY_122_INIT(dev);
		if (ret < 0) {
			return ret;
		}

		if (nrfx_qspi_mem_busy_check() != NRFX_SUCCESS) {
			return -EBUSY;
		}

		ret = enter_dpd(dev);
		if (ret < 0) {
			return ret;
		}

		nrfx_qspi_uninit();
		break;

	case PM_DEVICE_ACTION_RESUME:
		err = nrfx_qspi_init(&dev_config->nrfx_cfg,
				     qspi_handler,
				     dev_data);
		if (err != NRFX_SUCCESS) {
			return -EIO;
		}

		ret = exit_dpd(dev);
		if (ret < 0) {
			return ret;
		}

		ANOMALY_122_UNINIT(dev);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static struct qspi_nor_data qspi_nor_dev_data = {
#ifdef CONFIG_MULTITHREADING
	.trans = Z_SEM_INITIALIZER(qspi_nor_dev_data.trans, 1, 1),
	.sem = Z_SEM_INITIALIZER(qspi_nor_dev_data.sem, 1, 1),
	.sync = Z_SEM_INITIALIZER(qspi_nor_dev_data.sync, 0, 1),
#if NRF52_ERRATA_122_PRESENT
	.count = Z_SEM_INITIALIZER(qspi_nor_dev_data.count, 0, K_SEM_MAX_LIMIT),
#endif
#endif /* CONFIG_MULTITHREADING */
};

static const struct qspi_nor_config qspi_nor_dev_config = {
	.nrfx_cfg.pins = {
		.sck_pin = DT_PROP(QSPI_NODE, sck_pin),
		.csn_pin = QSPI_PROP_AT(csn_pins, 0),
		.io0_pin = QSPI_PROP_AT(io_pins, 0),
		.io1_pin = QSPI_PROP_AT(io_pins, 1),
#if QSPI_PROP_LEN(io_pins) > 2
		.io2_pin = QSPI_PROP_AT(io_pins, 2),
		.io3_pin = QSPI_PROP_AT(io_pins, 3),
#else
		.io2_pin = NRF_QSPI_PIN_NOT_CONNECTED,
		.io3_pin = NRF_QSPI_PIN_NOT_CONNECTED,
#endif
	},
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
		.sck_freq = (INST_0_SCK_FREQUENCY > NRF_QSPI_BASE_CLOCK_FREQ)
			    ? NRF_QSPI_FREQ_DIV1
			    : (NRF_QSPI_BASE_CLOCK_FREQ /
			       INST_0_SCK_FREQUENCY) - 1,
		.sck_delay = DT_INST_PROP(0, sck_delay),
		.spi_mode = INST_0_SPI_MODE,
	},

	.size = INST_0_BYTES,
	.id = DT_INST_PROP(0, jedec_id),
};

PM_DEVICE_DT_INST_DEFINE(0, qspi_nor_pm_action);

DEVICE_DT_INST_DEFINE(0, qspi_nor_init, PM_DEVICE_DT_INST_REF(0),
		      &qspi_nor_dev_data, &qspi_nor_dev_config,
		      POST_KERNEL, CONFIG_NORDIC_QSPI_NOR_INIT_PRIORITY,
		      &qspi_nor_api);
