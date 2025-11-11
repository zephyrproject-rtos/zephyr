/*
 * Copyright (c) 2020 Piotr Mienkowski
 * Copyright (c) 2020 Linaro Limited
 * Copyright (c) 2022 Georgij Cernysiov
 * Copyright (c) 2025 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_spixf_nor

#include <errno.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <soc.h>
#include <string.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>

#define MAX32_QSPI_NODE DT_INST_PARENT(0)

/* Get the base address of the flash from the DTS node */
#define MAX32_QSPI_BASE_ADDRESS DT_INST_REG_ADDR(0)

#define MAX32_QSPI_HAS_JEDEC_ID DT_INST_NODE_HAS_PROP(0, jedec_id)

#define MAX32_QSPI_RESET_GPIO DT_INST_NODE_HAS_PROP(0, reset_gpios)
#define MAX32_QSPI_RESET_CMD  DT_INST_PROP(0, reset_cmd)

#define MAX32_QSPI_UNKNOWN_MODE (0xFF)

#if DT_INST_NODE_HAS_PROP(0, spi_bus_width) && DT_INST_PROP(0, spi_bus_width) == 4
#define MAX32_QSPI_USE_QUAD_IO 1
#else
#define MAX32_QSPI_USE_QUAD_IO 0
#endif

/* Number of times to try verifying the write enable succeeded */
#define WRITE_ENABLE_MAX_RETRIES 4

#include <spixf.h>

#include "spi_nor.h"
#include "jesd216.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(flash_max32_spixf_nor, CONFIG_FLASH_LOG_LEVEL);

struct flash_max32_spixf_nor_req_wrapper {
	const struct device *dev;
	mxc_spixf_req_t req;
};

struct flash_max32_spixf_nor_config {
	const struct device *clock;
	const struct max32_perclk *perclkens;
	size_t perclkens_len;
	size_t flash_size;
	uint32_t max_frequency;
	uint32_t spixf_base_addr;
	const struct pinctrl_dev_config *pcfg;
#if MAX32_QSPI_RESET_GPIO
	const struct gpio_dt_spec reset;
#endif
#if !defined(CONFIG_FLASH_ADI_MAX32_SPIXF_SFDP_RUNTIME)
	/* Length of BFP structure, in 32-bit words. */
	uint8_t bfp_len;

	/* Pointer to the BFP table as read from the device
	 * (little-endian stored words), from sfdp-bfp property
	 */
	const struct jesd216_bfp *bfp;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
#endif
#if MAX32_QSPI_HAS_JEDEC_ID
	uint8_t jedec_id[DT_INST_PROP_LEN(0, jedec_id)];
#endif /* jedec_id */
	bool force_quad_addr_writes;
};

struct flash_max32_spixf_nor_data {
	struct k_sem sem;
	struct k_sem sync;
#if defined(CONFIG_FLASH_PAGE_LAYOUT) && defined(CONFIG_FLASH_ADI_MAX32_SPIXF_SFDP_RUNTIME)
	struct flash_pages_layout layout;
#endif
	struct jesd216_erase_type erase_types[JESD216_NUM_ERASE_TYPES];
	/* Number of bytes per page */
	uint16_t page_size;
	enum jesd216_dw15_qer_type qer_type;
	enum jesd216_mode_type mode;
	int cmd_status;
	uint8_t qspi_write_cmd;
	uint8_t qspi_read_cmd;
	uint8_t qspi_read_cmd_latency;
	/*
	 * If set addressed operations should use 32-bit rather than
	 * 24-bit addresses.
	 */
	bool flag_access_32bit: 1;
};

static inline void qspi_lock_thread(const struct device *dev)
{
	struct flash_max32_spixf_nor_data *dev_data = dev->data;

	k_sem_take(&dev_data->sem, K_FOREVER);
}

static inline void qspi_unlock_thread(const struct device *dev)
{
	struct flash_max32_spixf_nor_data *dev_data = dev->data;

	k_sem_give(&dev_data->sem);
}

static int qspi_write_enable(const struct device *dev);

static void qspi_send_req_cb(mxc_spixf_req_t *req, int resp)
{
	struct flash_max32_spixf_nor_req_wrapper *req_wrapper =
		CONTAINER_OF(req, struct flash_max32_spixf_nor_req_wrapper, req);
	struct flash_max32_spixf_nor_data *dev_data = req_wrapper->dev->data;

	dev_data->cmd_status = resp;

	k_sem_give(&dev_data->sync);
}

static inline int qspi_copy_addr(uint8_t *dest, off_t addr, bool addr_32bit)
{
	if (addr_32bit) {
		dest[0] = (addr >> 24) & 0xFF;
		dest[1] = (addr >> 16) & 0xFF;
		dest[2] = (addr >> 8) & 0xFF;
		dest[3] = addr & 0xFF;
		return 4;
	}

	dest[0] = (addr >> 16) & 0xFF;
	dest[1] = (addr >> 8) & 0xFF;
	dest[2] = addr & 0xFF;
	return 3;
}

/*
 * Send a command over QSPI bus.
 */
static int qspi_send_req(const struct device *dev, struct flash_max32_spixf_nor_req_wrapper *req)
{
	struct flash_max32_spixf_nor_data *dev_data = dev->data;
	int r;

	dev_data->cmd_status = 0;
	req->dev = dev;
	req->req.callback = qspi_send_req_cb;

	if (req->req.tx_data) {
		LOG_DBG("Command 0x%x", req->req.tx_data[0]);
	}

	r = MXC_SPIXF_TransactionAsync(&req->req);

	if (r < 0) {
		LOG_ERR("Failed to send QSPI request (%d)", r);
		return -EIO;
	}

	k_sem_take(&dev_data->sync, K_FOREVER);

	LOG_DBG("Command status %d", dev_data->cmd_status);

	return dev_data->cmd_status;
}

/*
 * Perform a read access over QSPI bus.
 */
static int qspi_read_access(const struct device *dev, uint8_t cmd, uint8_t *data, size_t size,
			    uint8_t dummy_bytes)
{
	struct flash_max32_spixf_nor_req_wrapper req = {.req.width = MXC_SPIXF_WIDTH_1};
	int ret;

	req.req.tx_data = &cmd;
	req.req.len = 1;

	ret = qspi_send_req(dev, &req);
	if (ret < 0) {
		LOG_ERR("Failed to send read command (%d)", ret);
		return ret;
	}

	if (dummy_bytes) {
		MXC_SPIXF_Clocks(dummy_bytes, 0);
	}

	req.req.tx_data = NULL;
	req.req.rx_data = data;
	req.req.len = size;
	req.req.deass = 1;

	ret = qspi_send_req(dev, &req);
	if (ret < 0) {
		LOG_ERR("Failed to read data (%d)", ret);
	}

	return ret;
}

static int qspi_read_status_register(const struct device *dev, uint8_t reg_num, uint8_t *reg)
{
	uint8_t cmd;

	switch (reg_num) {
	case 1U:
		cmd = SPI_NOR_CMD_RDSR;
		break;
	case 2U:
		cmd = SPI_NOR_CMD_RDSR2;
		break;
	case 3U:
		cmd = SPI_NOR_CMD_RDSR3;
		break;
	default:
		return -EINVAL;
	}

	return qspi_read_access(dev, cmd, reg, 1, 0);
}

static int qspi_write_status_register(const struct device *dev, uint8_t reg_num, uint8_t reg)
{
	struct flash_max32_spixf_nor_data *dev_data = dev->data;
	size_t size;
	uint8_t payload[4] = {0};
	int ret;
	struct flash_max32_spixf_nor_req_wrapper req = {0};

	if (reg_num == 1) {
		payload[0] = SPI_NOR_CMD_WRSR;
		size = 1U;
		payload[1] = reg;
		/* 1 byte write clears SR2, write SR2 as well */
		if (dev_data->qer_type == JESD216_DW15_QER_S2B1v1) {
			ret = qspi_read_status_register(dev, 2, &payload[2]);
			if (ret < 0) {
				return ret;
			}
			size = 2U;
		}
	} else if (reg_num == 2) {
		payload[0] = SPI_NOR_CMD_WRSR2;

		/* if SR2 write needs SR1 */
		if ((dev_data->qer_type == JESD216_DW15_QER_VAL_S2B1v1) ||
		    (dev_data->qer_type == JESD216_DW15_QER_VAL_S2B1v4) ||
		    (dev_data->qer_type == JESD216_DW15_QER_VAL_S2B1v5)) {
			ret = qspi_read_status_register(dev, 1, &payload[1]);
			if (ret < 0) {
				return ret;
			}

			payload[0] = SPI_NOR_CMD_WRSR;
			payload[2] = reg;
			size = 2U;
		} else {
			size = 1U;
			payload[1] = reg;
		}
	} else if (reg_num == 3) {
		payload[0] = SPI_NOR_CMD_WRSR3;
		size = 1U;
		payload[1] = reg;
	} else {
		return -EINVAL;
	}

	req.req.tx_data = payload;
	req.req.len = size + 1;
	req.req.width = MXC_SPIXF_WIDTH_1;
	req.req.deass = 1;

	return qspi_send_req(dev, &req);
}

#if defined(CONFIG_FLASH_JESD216_API) || MAX32_QSPI_HAS_JEDEC_ID

static int qspi_read_jedec_id_priv(const struct device *dev, uint8_t *id)
{
	int ret;

	ret = qspi_read_access(dev, JESD216_CMD_READ_ID, id, JESD216_READ_ID_LEN, 8);
	if (ret < 0) {
		LOG_ERR("Failed to read ID (%d)", ret);
		return ret;
	}

	LOG_DBG("Read JESD216-ID");

	return ret;
}

#if defined(CONFIG_FLASH_JESD216_API)

static int qspi_read_jedec_id(const struct device *dev, uint8_t *id)
{
	int ret = 0;

	qspi_lock_thread(dev);
	MXC_SPIXF_Enable();

	ret = qspi_read_jedec_id_priv(dev, id);

	MXC_SPIXF_Disable();
	qspi_unlock_thread(dev);

	return ret;
}

#endif

#endif /* CONFIG_FLASH_JESD216_API */

static int qspi_send_write_enable(const struct device *dev)
{
	const uint8_t write_en = SPI_NOR_CMD_WREN;

	/* clang-format off */
	struct flash_max32_spixf_nor_req_wrapper wrap = {
		.req = {
				.deass = 1,
				.tx_data = &write_en,
				.len = 1,
		},
	};
	/* clang-format on */

	return qspi_send_req(dev, &wrap);
}

static int qspi_write_unprotect(const struct device *dev)
{
	uint8_t cmd = SPI_NOR_CMD_ULBPR;
	int ret = 0;
	struct flash_max32_spixf_nor_req_wrapper req = {
		.req.tx_data = &cmd,
		.req.len = 1,
		.req.width = MXC_SPIXF_WIDTH_1,
	};

	if (IS_ENABLED(DT_INST_PROP(0, requires_ulbpr))) {
		ret = qspi_write_enable(dev);

		if (ret != 0) {
			return ret;
		}

		ret = qspi_send_req(dev, &req);
	}

	return ret;
}

#if IS_ENABLED(CONFIG_FLASH_JESD216_API) || IS_ENABLED(CONFIG_FLASH_ADI_MAX32_SPIXF_SFDP_RUNTIME)
/*
 * Read Serial Flash Discovery Parameter
 */
static int qspi_read_sfdp_priv(const struct device *dev, off_t addr, void *data, size_t size)
{
	struct flash_max32_spixf_nor_req_wrapper req = {.req.width = MXC_SPIXF_WIDTH_1};
	uint8_t tx_payload[4] = {JESD216_CMD_READ_SFDP, 0};
	int ret;

	__ASSERT(data != NULL, "null destination");

	LOG_INF("Reading SFDP");

	/* Send the command and 24-bit address */
	qspi_copy_addr(&tx_payload[1], addr, false);

	req.req.tx_data = tx_payload;
	req.req.len = ARRAY_SIZE(tx_payload);

	ret = qspi_send_req(dev, &req);
	if (ret < 0) {
		LOG_ERR("Failed to send the read SFDP command (%d)", ret);
		return ret;
	}

	/* Clock the dummy bytes */
	MXC_SPIXF_Clocks(8, 0);

	/* Read the data */

	req.req.tx_data = NULL;
	req.req.rx_data = data;
	req.req.len = size;
	req.req.deass = 1;

	ret = qspi_send_req(dev, &req);
	if (ret < 0) {
		LOG_ERR("Failed to read SFDP data (%d)", ret);
	}

	return ret;
}

#endif

#if IS_ENABLED(CONFIG_FLASH_JESD216_API)

static int qspi_read_sfdp(const struct device *dev, off_t addr, void *data, size_t size)
{
	int ret;

	qspi_lock_thread(dev);
	MXC_SPIXF_Enable();

	ret = qspi_read_sfdp_priv(dev, addr, data, size);

	MXC_SPIXF_Disable();
	qspi_unlock_thread(dev);

	return ret;
}

#endif /* IS_ENABLED(CONFIG_FLASH_JESD216_API) */

static bool qspi_address_is_valid(const struct device *dev, off_t addr, size_t size)
{
	const struct flash_max32_spixf_nor_config *dev_cfg = dev->config;
	size_t flash_size = dev_cfg->flash_size;

	return (addr >= 0) && ((uint64_t)addr + (uint64_t)size <= flash_size);
}

static int flash_max32_spixf_nor_read(const struct device *dev, off_t addr, void *data, size_t size)
{
	const struct flash_max32_spixf_nor_config *dev_cfg = dev->config;

	/* read non-zero size */
	if (size == 0) {
		return 0;
	}

	if (!qspi_address_is_valid(dev, addr, size)) {
		LOG_DBG("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu",
			(long)addr, size);
		return -EINVAL;
	}

	qspi_lock_thread(dev);

	memcpy(data, (uint8_t *)dev_cfg->spixf_base_addr + addr, size);

	qspi_unlock_thread(dev);

	return 0;
}

static int qspi_wait_until_ready(const struct device *dev)
{
	uint8_t reg;
	int ret;

	do {
		ret = qspi_read_status_register(dev, 1U, &reg);
	} while (!ret && (reg & SPI_NOR_WIP_BIT));

	return ret;
}

static int flash_max32_spixf_clear_read_cache(const struct device *dev)
{
	struct flash_max32_spixf_nor_data *dev_data = dev->data;
	uint8_t read_data;
	int ret;

	/* Read two pages to flush the cache */
	ret = flash_max32_spixf_nor_read(dev, 0, &read_data, 1);
	if (ret) {
		LOG_ERR("Failed to read first page to clear the read cache (%d)", ret);
		return ret;
	}

	ret = flash_max32_spixf_nor_read(dev, dev_data->page_size, &read_data, 1);
	if (ret) {
		LOG_ERR("Failed to read second page to clear the read cache (%d)", ret);
		return ret;
	}

	return 0;
}

static int flash_max32_spixf_nor_write(const struct device *dev, off_t addr, const void *data,
				       size_t size)
{
	const struct flash_max32_spixf_nor_config *dev_cfg = dev->config;
	struct flash_max32_spixf_nor_data *dev_data = dev->data;
	int ret = 0;
	uint8_t addr_payload[4];
	uint8_t pp_cmd;
	struct flash_max32_spixf_nor_req_wrapper req = {0};
	uint8_t addr_width, data_width;

	/* write non-zero size */
	if (size == 0) {
		return 0;
	}

	if (!qspi_address_is_valid(dev, addr, size)) {
		LOG_DBG("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu",
			(long)addr, size);
		return -EINVAL;
	}

	if (IS_ENABLED(MAX32_QSPI_USE_QUAD_IO)) {
		pp_cmd = dev_data->qspi_write_cmd;
		data_width = MXC_SPIXF_WIDTH_4;
		addr_width = (pp_cmd == SPI_NOR_CMD_PP_1_1_4 && !dev_cfg->force_quad_addr_writes)
				     ? MXC_SPIXF_WIDTH_1
				     : MXC_SPIXF_WIDTH_4;
	} else {
		pp_cmd = SPI_NOR_CMD_PP;
		addr_width = MXC_SPIXF_WIDTH_1;
		data_width = MXC_SPIXF_WIDTH_1;
	}

	qspi_lock_thread(dev);
	MXC_SPIXF_Enable();

	while (size > 0) {
		size_t to_write = size;

		/* Don't write more than a page. */
		if (to_write >= SPI_NOR_PAGE_SIZE) {
			to_write = SPI_NOR_PAGE_SIZE;
		}

		/* Don't write across a page boundary */
		if (((addr + to_write - 1U) / SPI_NOR_PAGE_SIZE) != (addr / SPI_NOR_PAGE_SIZE)) {
			to_write = SPI_NOR_PAGE_SIZE - (addr % SPI_NOR_PAGE_SIZE);
		}

		ret = qspi_write_enable(dev);
		if (ret != 0) {
			break;
		}

		req.req.deass = 0;
		req.req.tx_data = &pp_cmd;
		req.req.len = 1;
		req.req.width = MXC_SPIXF_WIDTH_1;

		ret = qspi_send_req(dev, &req);
		if (ret < 0) {
			LOG_ERR("Failed to send command byte (%d)", ret);
			break;
		}

		req.req.tx_data = addr_payload;
		req.req.len = qspi_copy_addr(addr_payload, addr, dev_data->flag_access_32bit);
		req.req.width = addr_width;

		ret = qspi_send_req(dev, &req);
		if (ret < 0) {
			LOG_ERR("Failed to send write address (%d)", ret);
			break;
		}

		req.req.tx_data = data;
		req.req.len = to_write;
		req.req.width = data_width;
		req.req.deass = 1;

		ret = qspi_send_req(dev, &req);
		if (ret < 0) {
			LOG_ERR("Failed to send write data (%d)", ret);
			break;
		}

		size -= to_write;
		data = (const uint8_t *)data + to_write;
		addr += to_write;

		ret = qspi_wait_until_ready(dev);
		if (ret != 0) {
			break;
		}
	}
	goto end;

end:
	MXC_SPIXF_Disable();
	qspi_unlock_thread(dev);

	/* TODO: ICC Flush */

	if (ret >= 0) {
		ret = flash_max32_spixf_clear_read_cache(dev);
	}

	return ret;
}

static int flash_max32_spixf_nor_erase(const struct device *dev, off_t addr, size_t size)
{
	const struct flash_max32_spixf_nor_config *dev_cfg = dev->config;
	struct flash_max32_spixf_nor_data *dev_data = dev->data;
	struct flash_max32_spixf_nor_req_wrapper req = {.req.deass = 1};
	int ret = 0;

	/* erase non-zero size */
	if (size == 0) {
		return 0;
	}

	if (!qspi_address_is_valid(dev, addr, size)) {
		LOG_DBG("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu",
			(long)addr, size);
		return -EINVAL;
	}

	qspi_lock_thread(dev);
	MXC_SPIXF_Enable();

	while ((size > 0) && (ret == 0)) {
		qspi_write_enable(dev);

		if (size == dev_cfg->flash_size) {
			/* chip erase */
			uint8_t cmd = SPI_NOR_CMD_CE;

			req.req.tx_data = &cmd;
			req.req.len = 1;
			req.req.width = MXC_SPIXF_WIDTH_1;

			ret = qspi_send_req(dev, &req);
			if (ret < 0) {
				LOG_ERR("Failed to do a chip erase (%d)", ret);
				break;
			}
			size -= dev_cfg->flash_size;
		} else {
			const struct jesd216_erase_type *erase_types = dev_data->erase_types;
			const struct jesd216_erase_type *bet = NULL;

			uint8_t payload[5];

			req.req.tx_data = payload;
			req.req.len =
				qspi_copy_addr(&payload[1], addr, dev_data->flag_access_32bit) + 1;
			req.req.width = MXC_SPIXF_WIDTH_1;

			for (uint8_t ei = 0; ei < JESD216_NUM_ERASE_TYPES; ++ei) {
				const struct jesd216_erase_type *etp = &erase_types[ei];

				if ((etp->exp != 0) && SPI_NOR_IS_ALIGNED(addr, etp->exp) &&
				    SPI_NOR_IS_ALIGNED(size, etp->exp) &&
				    ((bet == NULL) || (etp->exp > bet->exp))) {
					bet = etp;
				}
			}
			if (bet != NULL) {
				payload[0] = bet->cmd;
				ret = qspi_send_req(dev, &req);
				if (ret < 0) {
					LOG_ERR("Failed to do a erase (%d)", ret);
					break;
				}
				addr += BIT(bet->exp);
				size -= BIT(bet->exp);
			} else {
				LOG_ERR("Can't erase %zu at 0x%lx", size, (long)addr);
				ret = -EINVAL;
			}
		}
		qspi_wait_until_ready(dev);
	}
	goto end;

end:
	MXC_SPIXF_Disable();
	qspi_unlock_thread(dev);

	/* TODO: ICC Flush */

	if (ret >= 0) {
		ret = flash_max32_spixf_clear_read_cache(dev);
	}

	return ret;
}

static const struct flash_parameters flash_max32_spixf_nor_parameters = {.write_block_size = 1,
									 .erase_value = 0xff};

static const struct flash_parameters *flash_max32_spixf_nor_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_max32_spixf_nor_parameters;
}

static void flash_max32_spixf_nor_isr(const struct device *dev)
{
	ARG_UNUSED(dev);

	MXC_SPIXF_Handler();
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_max32_spixf_nor_pages_layout(const struct device *dev,
					       const struct flash_pages_layout **layout,
					       size_t *layout_size)
{
	struct flash_max32_spixf_nor_data *dev_data = dev->data;
	const struct flash_max32_spixf_nor_config *dev_cfg = dev->config;

	ARG_UNUSED(dev_data);
	ARG_UNUSED(dev_cfg);

	*layout = COND_CODE_1(IS_ENABLED(CONFIG_FLASH_ADI_MAX32_SPIXF_SFDP_RUNTIME),
						  (&dev_data->layout), (&dev_cfg->layout));
	*layout_size = 1;
}
#endif

static DEVICE_API(flash, flash_max32_spixf_nor_driver_api) = {
	.read = flash_max32_spixf_nor_read,
	.write = flash_max32_spixf_nor_write,
	.erase = flash_max32_spixf_nor_erase,
	.get_parameters = flash_max32_spixf_nor_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_max32_spixf_nor_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = qspi_read_sfdp,
	.read_jedec_id = qspi_read_jedec_id,
#endif /* CONFIG_FLASH_JESD216_API */
};

#if defined(CONFIG_FLASH_PAGE_LAYOUT) && defined(CONFIG_FLASH_ADI_MAX32_SPIXF_SFDP_RUNTIME)
static int setup_pages_layout(const struct device *dev)
{
	const struct flash_max32_spixf_nor_config *dev_cfg = dev->config;
	struct flash_max32_spixf_nor_data *data = dev->data;
	const size_t flash_size = dev_cfg->flash_size;
	uint32_t layout_page_size = data->page_size;
	uint8_t exponent = 0;
	int rv = 0;

	/* Find the smallest erase size. */
	for (size_t i = 0; i < ARRAY_SIZE(data->erase_types); ++i) {
		const struct jesd216_erase_type *etp = &data->erase_types[i];

		if ((etp->cmd != 0) && ((exponent == 0) || (etp->exp < exponent))) {
			exponent = etp->exp;
		}
	}

	if (exponent == 0) {
		return -ENOTSUP;
	}

	uint32_t erase_size = BIT(exponent);

	/* We need layout page size to be compatible with erase size */
	if ((layout_page_size % erase_size) != 0) {
		LOG_DBG("layout page %u not compatible with erase size %u", layout_page_size,
			erase_size);
		LOG_DBG("erase size will be used as layout page size");
		layout_page_size = erase_size;
	}

	/* Warn but accept layout page sizes that leave inaccessible
	 * space.
	 */
	if ((flash_size % layout_page_size) != 0) {
		LOG_INF("layout page %u wastes space with device size %zu", layout_page_size,
			flash_size);
	}

	data->layout.pages_size = layout_page_size;
	data->layout.pages_count = flash_size / layout_page_size;
	LOG_DBG("layout %u x %u By pages", data->layout.pages_count, data->layout.pages_size);

	return rv;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int qspi_program_addr_4b(const struct device *dev, bool write_enable)
{
	static const uint8_t cmd = SPI_NOR_CMD_4BA;
	struct flash_max32_spixf_nor_req_wrapper req = {0};
	int ret;

	/* Send write enable command, if required */
	if (write_enable) {
		ret = qspi_write_enable(dev);
		if (ret != 0) {
			return ret;
		}
	}

	req.req.tx_data = &cmd;
	req.req.len = 1;
	req.req.width = MXC_SPIXF_WIDTH_1;
	req.req.deass = 1;

	/*
	 * No need to Read control register afterwards to verify if 4byte addressing mode
	 * is enabled as the effect of the command is immediate
	 * and the SPI_NOR_CMD_RDCR is vendor-specific :
	 * SPI_NOR_4BYTE_BIT is BIT 5 for Macronix and 0 for Micron or Windbond
	 * Moreover bit value meaning is also vendor-specific
	 */

	return qspi_send_req(dev, &req);
}

#define WRITE_ENABLE_MAX_RETRIES 4

static int qspi_write_enable(const struct device *dev)
{
	uint8_t reg;
	int ret;
	int retries = 0;

	ret = qspi_send_write_enable(dev);
	if (ret) {
		return ret;
	}

	do {
		ret = qspi_read_status_register(dev, 1U, &reg);
	} while (!ret && !(reg & SPI_NOR_WEL_BIT) && (retries++) < WRITE_ENABLE_MAX_RETRIES);

	if (retries >= WRITE_ENABLE_MAX_RETRIES) {
		LOG_WRN("Timed-out waiting for write-enabled status");

		return -ETIMEDOUT;
	}

	return ret;
}

static int qspi_program_quad_io(const struct device *dev)
{
	struct flash_max32_spixf_nor_data *data = dev->data;
	uint8_t qe_reg_num;
	uint8_t qe_bit;
	uint8_t reg;
	int ret;

	switch (data->qer_type) {
	case JESD216_DW15_QER_NONE:
		/* no QE bit, device detects reads based on opcode */
		return 0;
	case JESD216_DW15_QER_S1B6:
		qe_reg_num = 1U;
		qe_bit = BIT(6U);
		break;
	case JESD216_DW15_QER_S2B7:
		qe_reg_num = 2U;
		qe_bit = BIT(7U);
		break;
	case JESD216_DW15_QER_S2B1v1:
		__fallthrough;
	case JESD216_DW15_QER_S2B1v4:
		__fallthrough;
	case JESD216_DW15_QER_S2B1v5:
		__fallthrough;
	case JESD216_DW15_QER_S2B1v6:
		qe_reg_num = 2U;
		qe_bit = BIT(1U);
		break;
	default:
		return -ENOTSUP;
	}

	ret = qspi_read_status_register(dev, qe_reg_num, &reg);
	if (ret < 0) {
		return ret;
	}

	/* exit early if QE bit is already set */
	if ((reg & qe_bit) != 0U) {
		return 0;
	}

	reg |= qe_bit;

	ret = qspi_write_enable(dev);
	if (ret < 0) {
		LOG_DBG("Failed to enable writing to the flash: %d", ret);
		return ret;
	}

	ret = qspi_write_status_register(dev, qe_reg_num, reg);
	if (ret < 0) {
		LOG_DBG("Failed to set QE bit: %d", ret);
		return ret;
	}

	ret = qspi_wait_until_ready(dev);
	if (ret < 0) {
		LOG_DBG("Flash failed to become ready after writing QE bit: %d", ret);
		return ret;
	}

	/* validate that QE bit is set */
	ret = qspi_read_status_register(dev, qe_reg_num, &reg);
	if (ret < 0) {
		LOG_DBG("Failed to fetch QE register after setting it: %d", ret);
		return ret;
	}

	if ((reg & qe_bit) == 0U) {
		LOG_ERR("Status Register %u [0x%02x] not set", qe_reg_num, reg);
		return -EIO;
	}

	return ret;
}

static int spi_nor_process_bfp(const struct device *dev, const struct jesd216_param_header *php,
			       const struct jesd216_bfp *bfp)
{
	const struct flash_max32_spixf_nor_config *dev_cfg = dev->config;
	struct flash_max32_spixf_nor_data *data = dev->data;
	struct jesd216_erase_type *etp = data->erase_types;
	uint8_t addr_mode;
	const size_t flash_size = (jesd216_bfp_density(bfp) / 8U);
	int rc;

	if (flash_size != dev_cfg->flash_size) {
		LOG_ERR("Unexpected flash size: %u", flash_size);
		return -EINVAL;
	}

	LOG_INF("%s: %u MiBy flash", dev->name, (uint32_t)(flash_size >> 20));

	/* Copy over the erase types, preserving their order.  (The
	 * Sector Map Parameter table references them by index.)
	 */
	memset(data->erase_types, 0, sizeof(data->erase_types));
	for (uint8_t ti = 1; ti <= ARRAY_SIZE(data->erase_types); ++ti) {
		if (jesd216_bfp_erase(bfp, ti, etp) == 0) {
			LOG_DBG("Erase %u with %02x", (uint32_t)BIT(etp->exp), etp->cmd);
		}
		++etp;
	}

	data->page_size = jesd216_bfp_page_size(php, bfp);

	LOG_DBG("Page size %u bytes", data->page_size);
	LOG_DBG("Flash size %u bytes", flash_size);

	addr_mode = jesd216_bfp_addrbytes(bfp);
	if (addr_mode == JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B4B) {
		struct jesd216_bfp_dw16 dw16;

		if (jesd216_bfp_decode_dw16(php, bfp, &dw16) == 0) {
			/*
			 * According to JESD216, the bit0 of dw16.enter_4ba
			 * portion of flash description register 16 indicates
			 * if it is enough to use 0xB7 instruction without
			 * write enable to switch to 4 bytes addressing mode.
			 * If bit 1 is set, a write enable is needed.
			 */
			if (dw16.enter_4ba & 0x3) {
				rc = qspi_program_addr_4b(dev, dw16.enter_4ba & 2);
				if (rc == 0) {
					data->flag_access_32bit = true;
					LOG_INF("Flash - address mode: 4B");
				} else {
					LOG_ERR("Unable to enter 4B mode: %d\n", rc);
					return rc;
				}
			}
		}
	}
	if (addr_mode == JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_4B) {
		data->flag_access_32bit = true;
		LOG_INF("Flash - address mode: 4B");
	}

	/*
	 * Only check if the 1-4-4 (i.e. 4READ) or 1-1-4 (QREAD)
	 * is supported - other modes are not.
	 */
	if (IS_ENABLED(MAX32_QSPI_USE_QUAD_IO)) {
		const enum jesd216_mode_type supported_modes[] = {JESD216_MODE_114,
								  JESD216_MODE_144};
		struct jesd216_bfp_dw15 dw15;
		struct jesd216_instr res;

		/* reset active mode */
		data->mode = MAX32_QSPI_UNKNOWN_MODE;

		/* query supported read modes, begin from the slowest */
		for (size_t i = 0; i < ARRAY_SIZE(supported_modes); ++i) {
			rc = jesd216_bfp_read_support(php, bfp, supported_modes[i], &res);
			if (rc >= 0) {
				LOG_INF("Quad read mode %d instr [0x%x] supported",
					supported_modes[i], res.instr);

				data->mode = supported_modes[i];
				data->qspi_read_cmd = res.instr;
				data->qspi_read_cmd_latency = res.wait_states;

				if (res.mode_clocks) {
					data->qspi_read_cmd_latency += res.mode_clocks;
				}
			}
		}

		/* don't continue when there is no supported mode */
		if (data->mode == MAX32_QSPI_UNKNOWN_MODE) {
			LOG_ERR("No supported flash read mode found");
			return -ENOTSUP;
		}

		LOG_INF("Quad read mode %d instr [0x%x] will be used", data->mode, res.instr);

		/* try to decode QE requirement type */
		rc = jesd216_bfp_decode_dw15(php, bfp, &dw15);
		if (rc < 0) {
			/* will use QER from DTS or default (refer to device data) */
			LOG_WRN("Unable to decode QE requirement [DW15]: %d", rc);
		} else {
			/* bypass DTS QER value */
			data->qer_type = dw15.qer;
		}

		LOG_INF("QE requirement mode: %x", data->qer_type);

		/* enable QE */
		rc = qspi_program_quad_io(dev);
		if (rc < 0) {
			LOG_ERR("Failed to enable Quad mode: %d", rc);
			return rc;
		}

		LOG_INF("Quad mode enabled");
	}

	return 0;
}

#if MAX32_QSPI_RESET_GPIO
static void flash_max32_spixf_nor_gpio_reset(const struct device *dev)
{
	const struct flash_max32_spixf_nor_config *dev_cfg = dev->config;

	/* Generate RESETn pulse for the flash memory */
	gpio_pin_configure_dt(&dev_cfg->reset, GPIO_OUTPUT_ACTIVE);
	k_msleep(DT_INST_PROP(0, reset_gpios_duration));
	gpio_pin_set_dt(&dev_cfg->reset, 0);
}
#endif

#if MAX32_QSPI_RESET_CMD
static int flash_max32_spixf_nor_send_reset(const struct device *dev)
{
	struct flash_max32_spixf_nor_req_wrapper req = {.req = {
								.deass = 1,
								.width = MXC_SPIXF_WIDTH_1,
								.len = 1,
							}};
	uint8_t cmd;
	int ret;

	req.req.tx_data = &cmd;

	cmd = SPI_NOR_CMD_RESET_EN;

	ret = qspi_send_req(dev, &req);
	if (ret != 0) {
		LOG_ERR("%d: Failed to send RESET_EN", ret);
		return ret;
	}

	cmd = SPI_NOR_CMD_RESET_MEM;
	ret = qspi_send_req(dev, &req);
	if (ret != 0) {
		LOG_ERR("%d: Failed to send RESET_MEM", ret);
		return ret;
	}

	LOG_DBG("Sent Reset command");

	return 0;
}
#endif

#if MAX32_QSPI_HAS_JEDEC_ID
static int flash_max32_spixf_nor_check_jedec_id(const struct device *dev)
{
	const struct flash_max32_spixf_nor_config *dev_cfg = dev->config;
	uint8_t id[SPI_NOR_MAX_ID_LEN];
	int ret;

	ret = qspi_read_jedec_id_priv(dev, id);
	if (ret < 0) {
		LOG_ERR("Failed to read the JEDEC ID (%d)", ret);
		return ret;
	}

	if (memcmp(dev_cfg->jedec_id, id, MIN(DT_INST_PROP_LEN(0, jedec_id), SPI_NOR_MAX_ID_LEN)) !=
	    0) {
		LOG_ERR("JEDEC id [%02x %02x %02x] expect [%02x %02x %02x]", id[0], id[1], id[2],
			dev_cfg->jedec_id[0], dev_cfg->jedec_id[1], dev_cfg->jedec_id[2]);
		return -EINVAL;
	}

	return 0;
}
#endif

static void flash_max32_spixf_nor_irq_config(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(MAX32_QSPI_NODE), DT_IRQ(MAX32_QSPI_NODE, priority),
		    flash_max32_spixf_nor_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_IRQN(MAX32_QSPI_NODE));
}

static int flash_max32_spixf_nor_fetch_jesd216_details(const struct device *dev)
{
	int ret = 0;
#if defined(CONFIG_FLASH_ADI_MAX32_SPIXF_SFDP_RUNTIME)
	const uint8_t decl_nph = 2;
	union {
		/* We only process BFP so use one parameter block */
		uint8_t raw[JESD216_SFDP_SIZE(decl_nph)];
		struct jesd216_sfdp_header sfdp;
	} u;
	const struct jesd216_sfdp_header *hp = &u.sfdp;

	ret = qspi_read_sfdp_priv(dev, 0, u.raw, sizeof(u.raw));
	if (ret != 0) {
		LOG_ERR("SFDP read failed: %d", ret);
		return ret;
	}

	uint32_t magic = jesd216_sfdp_magic(hp);

	if (magic != JESD216_SFDP_MAGIC) {
		LOG_ERR("SFDP magic %08x invalid", magic);
		return -EINVAL;
	}

	LOG_INF("%s: SFDP v %u.%u AP %x with %u PH", dev->name, hp->rev_major, hp->rev_minor,
		hp->access, 1 + hp->nph);

	const struct jesd216_param_header *php = hp->phdr;
	const struct jesd216_param_header *phpe = php + MIN(decl_nph, 1 + hp->nph);

	while (php != phpe) {
		uint16_t id = jesd216_param_id(php);

		LOG_INF("PH%u: %04x rev %u.%u: %u DW @ %x", (php - hp->phdr), id, php->rev_major,
			php->rev_minor, php->len_dw, jesd216_param_addr(php));

		if (id == JESD216_SFDP_PARAM_ID_BFP) {
			union {
				uint32_t dw[20];
				struct jesd216_bfp bfp;
			} u2;
			const struct jesd216_bfp *bfp = &u2.bfp;

			ret = qspi_read_sfdp_priv(
				dev, jesd216_param_addr(php), (uint8_t *)u2.dw,
				MIN(sizeof(uint32_t) * php->len_dw, sizeof(u2.dw)));
			if (ret == 0) {
				ret = spi_nor_process_bfp(dev, php, bfp);
			}

			if (ret != 0) {
				LOG_ERR("SFDP BFP failed: %d", ret);
				break;
			}
		}
		++php;
	}
#else
	/* Synthesize a header and process the version from DTS */
	const struct flash_max32_spixf_nor_config *dev_cfg = dev->config;
	struct jesd216_param_header bfp_hdr = {
		.len_dw = dev_cfg->bfp_len,
	};

	ret = spi_nor_process_bfp(dev, &bfp_hdr, dev_cfg->bfp);
#endif

	return ret;
}

static void flash_max32_spixf_update_read_settings(uint8_t cmd_read, uint8_t read_latency)
{
	MXC_SPIXF_SetMode(MXC_SPIXF_MODE_0);
	MXC_SPIXF_SetSSPolActiveLow();
	MXC_SPIXF_SetSSActiveTime(MXC_SPIXF_SYS_CLOCKS_2);
	MXC_SPIXF_SetSSInactiveTime(MXC_SPIXF_SYS_CLOCKS_3);

	MXC_SPIXF_SetCmdValue(cmd_read);
	MXC_SPIXF_SetAddrWidth(MXC_SPIXF_QUAD_SDIO);
	MXC_SPIXF_SetDataWidth(MXC_SPIXF_WIDTH_4);
	MXC_SPIXF_SetModeClk(read_latency);

	MXC_SPIXF_Set3ByteAddr();
	MXC_SPIXF_SCKFeedbackEnable();
	MXC_SPIXF_SetSCKNonInverted();
}

static int flash_max32_spixf_nor_init(const struct device *dev)
{
	const struct flash_max32_spixf_nor_config *dev_cfg = dev->config;
	struct flash_max32_spixf_nor_data *dev_data = dev->data;
	int ret;

	/* Signals configuration */
	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("SPIXF pinctrl setup failed (%d)", ret);
		return ret;
	}

#if MAX32_QSPI_RESET_GPIO
	flash_max32_spixf_nor_gpio_reset(dev);
#endif

	if (dev_cfg->clock != NULL) {
		for (size_t i = 0; i < dev_cfg->perclkens_len; i++) {
			/* enable clock */
			ret = clock_control_on(dev_cfg->clock,
					       (clock_control_subsys_t)&dev_cfg->perclkens[i]);
			if (ret != 0) {
				LOG_ERR("cannot enable GPIO clock");
				return ret;
			}
		}
	}

	ret = MXC_SPIXF_Init(SPI_NOR_CMD_2READ, dev_cfg->max_frequency);
	if (ret < 0) {
		LOG_ERR("Failed to init the SPIXF peripheral (%d)", ret);
		return ret;
	}

	MXC_SPIXF_Enable();

	/* Initialize semaphores */
	k_sem_init(&dev_data->sem, 1, 1);
	k_sem_init(&dev_data->sync, 0, 1);

	/* Run IRQ init */
	flash_max32_spixf_nor_irq_config(dev);

#if MAX32_QSPI_RESET_CMD
	flash_max32_spixf_nor_send_reset(dev);
	k_busy_wait(DT_INST_PROP(0, reset_cmd_wait));
#endif

	/* Run NOR init */
	ret = flash_max32_spixf_nor_fetch_jesd216_details(dev);
	if (ret < 0) {
		LOG_WRN("Loading initial flash table details failed (%d)", ret);
		return -ENODEV;
	}

#if MAX32_QSPI_HAS_JEDEC_ID
	ret = flash_max32_spixf_nor_check_jedec_id(dev);
	if (ret < 0) {
		return -ENODEV;
	}
#endif

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
#if defined(CONFIG_FLASH_ADI_MAX32_SPIXF_SFDP_RUNTIME)
	ret = setup_pages_layout(dev);
	if (ret != 0) {
		LOG_ERR("layout setup failed: %d", ret);
		return -ENODEV;
	}
#else
	LOG_INF("Default page layout is %d by %d", dev_cfg->layout.pages_count,
		dev_cfg->layout.pages_size);
#endif
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

	ret = qspi_write_unprotect(dev);
	if (ret != 0) {
		LOG_ERR("write unprotect failed: %d", ret);
		return -ENODEV;
	}

	LOG_INF("NOR quad-flash at 0x%lx (0x%x bytes)", (long)(MAX32_QSPI_BASE_ADDRESS),
		dev_cfg->flash_size);

	MXC_SPIXF_Disable();

	/* Update our SPIXF main controller settings based on the fetched jesd216 details */
	flash_max32_spixf_update_read_settings(dev_data->qspi_read_cmd,
					       dev_data->qspi_read_cmd_latency);

	return 0;
}

#define QSPI_FLASH_MODULE(drv_id, flash_id) (DT_DRV_INST(drv_id), qspi_nor_flash_##flash_id)

#define DT_WRITEOC_PROP_OR(inst, default_value)                                                    \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, writeoc),                                    \
		    (_CONCAT(SPI_NOR_CMD_, DT_STRING_TOKEN(DT_DRV_INST(inst), writeoc))),    \
		    ((default_value)))

#define DT_QER_PROP_OR(inst, default_value)                                                        \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, quad_enable_requirements),                   \
		    (_CONCAT(JESD216_DW15_QER_VAL_,                                          \
			     DT_STRING_TOKEN(DT_DRV_INST(inst), quad_enable_requirements))), \
		    ((default_value)))

PINCTRL_DT_DEFINE(MAX32_QSPI_NODE);

#define CLOCK_CFG(node_id, prop, idx)                                                              \
	{.bus = DT_CLOCKS_CELL_BY_IDX(node_id, idx, offset),                                       \
	 .bit = DT_CLOCKS_CELL_BY_IDX(node_id, idx, bit)}

/* clang-format off */

static const struct max32_perclk perclkens[] = {
	DT_FOREACH_PROP_ELEM_SEP(MAX32_QSPI_NODE, clocks, CLOCK_CFG, (,))};

/* clang-format on */

#if MAX32_QSPI_HAS_JEDEC_ID
BUILD_ASSERT(DT_INST_PROP_LEN(0, jedec_id) >= 3, "jedec-id must be at least 3 bytes.");
#endif

#if !defined(CONFIG_FLASH_ADI_MAX32_SPIXF_SFDP_RUNTIME)
static const __aligned(4) uint8_t bfp_data[] = DT_INST_PROP(0, sfdp_bfp);
#endif

static const struct flash_max32_spixf_nor_config flash_max32_spixf_nor_cfg = {
	.clock = DEVICE_DT_GET_OR_NULL(DT_CLOCKS_CTLR(MAX32_QSPI_NODE)),
	.perclkens = perclkens,
	.perclkens_len = ARRAY_SIZE(perclkens),
	.flash_size = DT_INST_REG_SIZE(0),
	.max_frequency = DT_INST_PROP(0, qspi_max_frequency),
	.spixf_base_addr = DT_INST_REG_ADDR(0),
	.pcfg = PINCTRL_DT_DEV_CONFIG_GET(MAX32_QSPI_NODE),
	.force_quad_addr_writes = DT_INST_PROP_OR(0, force_quad_address_write, false),
#if !defined(CONFIG_FLASH_ADI_MAX32_SPIXF_SFDP_RUNTIME)
	.bfp_len = sizeof(bfp_data) / 4,
	.bfp = (const struct jesd216_bfp *)bfp_data,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.layout.pages_size = DT_INST_PROP(0, page_size),
	.layout.pages_count = DT_INST_REG_SIZE(0) / DT_INST_PROP(0, page_size),
#endif
#endif
#if MAX32_QSPI_RESET_GPIO
	.reset = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
#endif
#if MAX32_QSPI_HAS_JEDEC_ID
	.jedec_id = DT_INST_PROP(0, jedec_id),
#endif /* jedec_id */
};

static struct flash_max32_spixf_nor_data flash_max32_spixf_nor_dev_data = {
	.qer_type = DT_QER_PROP_OR(0, JESD216_DW15_QER_VAL_S1B6),
	.qspi_write_cmd = DT_WRITEOC_PROP_OR(0, SPI_NOR_CMD_PP_1_4_4),
};

DEVICE_DT_INST_DEFINE(0, &flash_max32_spixf_nor_init, NULL, &flash_max32_spixf_nor_dev_data,
		      &flash_max32_spixf_nor_cfg, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,
		      &flash_max32_spixf_nor_driver_api);
