/*
 * Copyright (c) 2023 Andes Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT andestech_qspi_nor

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <string.h>
#include <zephyr/logging/log.h>

#include "flash_andes_qspi.h"
#include "spi_nor.h"
#include "jesd216.h"
#include "flash_priv.h"

LOG_MODULE_REGISTER(flash_andes, CONFIG_FLASH_LOG_LEVEL);

/* Indicates that an access command includes bytes for the address.
 * If not provided the opcode is not followed by address bytes.
 */
#define ANDES_ACCESS_ADDRESSED BIT(0)

/* Indicates that an access command is performing a write.  If not
 * provided access is a read.
 */
#define ANDES_ACCESS_WRITE BIT(7)

#define flash_andes_qspi_cmd_read(dev, opcode, dest, length)                   \
	flash_andes_qspi_access(dev, opcode, 0, 0, dest, length)
#define flash_andes_qspi_cmd_addr_read(dev, opcode, addr, dest, length)        \
	flash_andes_qspi_access(dev, opcode, ANDES_ACCESS_ADDRESSED, addr,     \
				dest, length)
#define flash_andes_qspi_cmd_write(dev, opcode)                                \
	flash_andes_qspi_access(dev, opcode, ANDES_ACCESS_WRITE, 0, NULL, 0)
#define flash_andes_qspi_cmd_addr_write(dev, opcode, addr, src, length)        \
	flash_andes_qspi_access(dev, opcode,                                   \
				ANDES_ACCESS_WRITE | ANDES_ACCESS_ADDRESSED,   \
				addr, (void *)src, length)


typedef void (*flash_andes_qspi_config_func_t)(void);
struct flash_andes_qspi_config {
	flash_andes_qspi_config_func_t cfg_func;
	uint32_t base;
	uint32_t irq_num;
	struct flash_parameters parameters;
	bool xip;
#if defined(CONFIG_FLASH_ANDES_QSPI_SFDP_DEVICETREE)
	uint8_t jedec_id[SPI_NOR_MAX_ID_LEN];
	uint32_t flash_size;
	uint8_t bfp_len;
	const struct jesd216_bfp *bfp;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#endif /* CONFIG_FLASH_ANDES_QSPI_SFDP_DEVICETREE */
};

struct flash_andes_qspi_data {
	struct k_sem sem;
	struct k_sem device_sync_sem;
	uint32_t tx_fifo_size;
	uint32_t rx_fifo_size;
	uint8_t *tx_buf;
	uint8_t *rx_buf;
	uint32_t tx_len;
	uint32_t rx_len;
	uint32_t tx_ptr; /* write pointer */
	uint32_t rx_ptr; /* read pointer */
	struct jesd216_erase_type erase_types[JESD216_NUM_ERASE_TYPES];
	uint16_t page_size;
#ifdef CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME
	uint32_t flash_size;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#endif
};

static int flash_andes_qspi_write_protection_set(const struct device *dev,
						 bool write_protect);

/* Get pointer to array of supported erase types. */
static inline const struct jesd216_erase_type *
dev_erase_types(const struct device *dev)
{
	const struct flash_andes_qspi_data *dev_data = dev->data;

	return dev_data->erase_types;
}

/* Get the size of the flash device. */
static inline uint32_t dev_flash_size(const struct device *dev)
{
#ifdef CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME
	const struct flash_andes_qspi_data *dev_data = dev->data;

	return dev_data->flash_size;
#else /* CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME */
	const struct flash_andes_qspi_config *config = dev->config;

	return config->flash_size;
#endif /* CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME */
}

/* Get the flash device page size. */
static inline uint16_t dev_page_size(const struct device *dev)
{
	const struct flash_andes_qspi_data *dev_data = dev->data;

	return dev_data->page_size;
}

/*
 * @brief Send an SPI command
 *
 * @param dev Device struct
 * @param opcode The command to send
 * @param access flags that determine how the command is constructed.
 * @param addr The address to send
 * @param data The buffer to store or read the value
 * @param length The size of the buffer
 * @return 0 on success
 */
static int flash_andes_qspi_access(const struct device *const dev,
				   uint8_t opcode, uint8_t access, off_t addr,
				   void *data, size_t length)
{
	struct flash_andes_qspi_data *dev_data = dev->data;
	const struct flash_andes_qspi_config *config = dev->config;
	uint32_t base = config->base;

	bool is_addressed = (access & ANDES_ACCESS_ADDRESSED) != 0U;
	bool is_write = (access & ANDES_ACCESS_WRITE) != 0U;
	int ret = 0;

	uint32_t tctrl, int_msk;

	/* Command phase enable */
	tctrl = TCTRL_CMD_EN_MSK;
	if (is_addressed) {
		/* Enable and set ADDR len */
		sys_write32((sys_read32(QSPI_TFMAT(base)) |
			    (0x2 << TFMAT_ADDR_LEN_OFFSET)), QSPI_TFMAT(base));
		sys_write32(addr, QSPI_ADDR(base));
		/* Address phase enable */
		tctrl |= TCTRL_ADDR_EN_MSK;
	}

	if (length == 0) {
		if ((opcode == FLASH_ANDES_CMD_4PP) ||
		    (opcode == FLASH_ANDES_CMD_4READ)) {
			goto exit;
		}
		tctrl |= TRNS_MODE_NONE_DATA;
		int_msk = IEN_END_MSK;
	} else if (is_write) {
		dev_data->tx_ptr = 0;
		dev_data->tx_buf = (uint8_t *)data;
		dev_data->tx_len = length;

		tctrl |= (TRNS_MODE_WRITE_ONLY |
			 ((length - 1) << TCTRL_WR_TCNT_OFFSET));
		int_msk = IEN_TX_FIFO_MSK | IEN_END_MSK;
	} else {
		dev_data->rx_ptr = 0;
		dev_data->rx_buf = (uint8_t *)data;

		tctrl |= (TRNS_MODE_READ_ONLY |
			 ((length - 1) << TCTRL_RD_TCNT_OFFSET));
		int_msk = IEN_RX_FIFO_MSK | IEN_END_MSK;
	}

	switch (opcode) {
	case FLASH_ANDES_CMD_4PP:
		tctrl = ((tctrl & ~TCTRL_TRNS_MODE_MSK)	|
			DUAL_IO_MODE			|
			TCTRL_ADDR_FMT_MSK		|
			TCTRL_ADDR_EN_MSK		|
			TRNS_MODE_WRITE_ONLY);
		break;
	case FLASH_ANDES_CMD_4READ:
		tctrl = ((tctrl & ~TCTRL_TRNS_MODE_MSK)	|
			DUAL_IO_MODE			|
			TCTRL_ADDR_FMT_MSK		|
			TCTRL_ADDR_EN_MSK		|
			TRNS_MODE_DUMMY_READ		|
			DUMMY_CNT_3);
		break;
	case JESD216_CMD_READ_SFDP:
		tctrl = ((tctrl & ~TCTRL_TRNS_MODE_MSK)	|
			TCTRL_ADDR_EN_MSK		|
			TRNS_MODE_DUMMY_READ);
		break;
	default:
		break;
	}

	sys_write32(tctrl, QSPI_TCTRL(base));
	/* Enable TX/RX FIFO interrupts */
	sys_write32(int_msk, QSPI_INTEN(base));
	/* write CMD register to send command*/
	sys_write32(opcode, QSPI_CMD(base));
	k_sem_take(&dev_data->device_sync_sem, K_FOREVER);
exit:
	return ret;
}

/* Everything necessary to acquire owning access to the device. */
static void acquire_device(const struct device *dev)
{
	struct flash_andes_qspi_data *dev_data = dev->data;

	k_sem_take(&dev_data->sem, K_FOREVER);
}

/* Everything necessary to release access to the device. */
static void release_device(const struct device *dev)
{
	struct flash_andes_qspi_data *dev_data = dev->data;

	k_sem_give(&dev_data->sem);
}

/**
 * @brief Wait until the flash is ready
 *
 * @param dev The device structure
 * @return 0 on success, negative errno code otherwise
 */
static int flash_andes_qspi_wait_until_ready(const struct device *dev)
{
	int ret;
	uint8_t reg;

	do {
		ret = flash_andes_qspi_cmd_read(dev,
		FLASH_ANDES_CMD_RDSR, &reg, 1);
	} while (!ret && (reg & FLASH_ANDES_WIP_BIT));

	return ret;
}

#if defined(CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME) || \
	defined(CONFIG_FLASH_JESD216_API)
/*
 * @brief Read content from the SFDP hierarchy
 *
 * @note The device must be externally acquired before invoking this
 * function.
 *
 * @param dev Device struct
 * @param addr The address to send
 * @param data The buffer to store or read the value
 * @param length The size of the buffer
 * @return 0 on success, negative errno code otherwise
 */
static int read_sfdp(const struct device *const dev,
		     off_t addr, void *data, size_t length)
{
	/* READ_SFDP requires a 24-bit address followed by a single
	 * byte for a wait state.  This is effected by using 32-bit
	 * address by shifting the 24-bit address up 8 bits.
	 */
	return flash_andes_qspi_access(dev, JESD216_CMD_READ_SFDP,
				       ANDES_ACCESS_ADDRESSED,
				       addr, data, length);
}
#endif /* CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME */

/**
 * @brief Write the status register.
 *
 * @note The device must be externally acquired before invoking this
 * function.
 *
 * @param dev Device struct
 * @param sr The new value of the status register
 *
 * @return 0 on success or a negative error code.
 */
static int flash_andes_qspi_wrsr(const struct device *dev,
				 uint8_t sr)
{
	int ret = flash_andes_qspi_cmd_write(dev, FLASH_ANDES_CMD_WREN);

	if (ret == 0) {
		ret = flash_andes_qspi_access(dev, FLASH_ANDES_CMD_WRSR,
					      ANDES_ACCESS_WRITE, 0, &sr,
					      sizeof(sr));
		flash_andes_qspi_wait_until_ready(dev);
	}

	return ret;
}

static int flash_andes_qspi_read(const struct device *dev,
				 off_t addr, void *dest, size_t size)
{
	const size_t flash_size = dev_flash_size(dev);
	int ret;

	/* should be between 0 and flash size */
	if ((addr < 0 || addr >= flash_size || ((flash_size - addr) < size))) {
		return -EINVAL;
	}

	if (size == 0) {
		return 0;
	}

	acquire_device(dev);

	ret = flash_andes_qspi_cmd_addr_read(dev,
		FLASH_ANDES_CMD_4READ, addr, dest, size);

	release_device(dev);
	return ret;
}

static int flash_andes_qspi_write(const struct device *dev, off_t addr,
				  const void *src, size_t size)
{
	const size_t flash_size = dev_flash_size(dev);
	const uint16_t page_size = dev_page_size(dev);
	size_t to_write = size;
	int ret = 0;

	/* should be between 0 and flash size */
	if ((addr < 0 || addr >= flash_size || ((flash_size - addr) < size))) {
		return -EINVAL;
	}

	if (size == 0) {
		return 0;
	}

	acquire_device(dev);

	ret = flash_andes_qspi_write_protection_set(dev, false);

	if (ret != 0) {
		goto out;
	}

	do {
		/* Get the adequate size to send*/
		to_write = MIN(page_size - (addr % page_size), size);

		ret = flash_andes_qspi_cmd_addr_write(dev,
			FLASH_ANDES_CMD_4PP, addr, src, to_write);

		if (ret != 0) {
			break;
		}

		size -= to_write;
		src = (const uint8_t *)src + to_write;
		addr += to_write;

		flash_andes_qspi_wait_until_ready(dev);
	} while (size > 0);


	int ret2 = flash_andes_qspi_write_protection_set(dev, true);

	if (!ret) {
		ret = ret2;
	}

out:
	release_device(dev);
	return ret;
}

static int flash_andes_qspi_erase(const struct device *dev,
				  off_t addr, size_t size)
{
	const size_t flash_size = dev_flash_size(dev);
	int ret = 0;

	/* erase area must be subregion of device */
	if ((addr < 0 || addr >= flash_size || ((flash_size - addr) < size))) {
		return -EINVAL;
	}

	if (size == 0) {
		return 0;
	}

	/* address must be sector-aligned */
	if (!SPI_NOR_IS_SECTOR_ALIGNED(addr)) {
		return -EINVAL;
	}

	/* size must be a multiple of sectors */
	if ((size % SPI_NOR_SECTOR_SIZE) != 0) {
		return -EINVAL;
	}

	acquire_device(dev);

	ret = flash_andes_qspi_write_protection_set(dev, false);

	if (ret != 0) {
		goto out;
	}

	if (size == flash_size) {
		/* chip erase */
		flash_andes_qspi_cmd_write(dev, FLASH_ANDES_CMD_CE);
		size -= flash_size;
		flash_andes_qspi_wait_until_ready(dev);
	}

	while (size > 0) {
		const struct jesd216_erase_type *erase_types =
						dev_erase_types(dev);
		const struct jesd216_erase_type *bet = NULL;

		for (uint8_t ei = 0; ei < JESD216_NUM_ERASE_TYPES; ++ei) {
			const struct jesd216_erase_type *etp =
						&erase_types[ei];

			if ((etp->exp != 0) &&
				SPI_NOR_IS_ALIGNED(addr, etp->exp) &&
				SPI_NOR_IS_ALIGNED(size, etp->exp) &&
				((bet == NULL) || (etp->exp > bet->exp))) {
				bet = etp;
			}
		}

		if (bet != NULL) {
			flash_andes_qspi_cmd_addr_write(dev, bet->cmd,
							addr, NULL, 0);
			addr += BIT(bet->exp);
			size -= BIT(bet->exp);
		} else {
			LOG_DBG("Can't erase %zu at 0x%lx",
				size, (long)addr);
			ret = -EINVAL;
			break;
		}

		flash_andes_qspi_wait_until_ready(dev);
	}

	int ret2 = flash_andes_qspi_write_protection_set(dev, true);

	if (!ret) {
		ret = ret2;
	}

out:
	release_device(dev);

	return ret;
}

static int flash_andes_qspi_write_protection_set(const struct device *dev,
						 bool write_protect)
{
	return flash_andes_qspi_cmd_write(dev, (write_protect) ?
				FLASH_ANDES_CMD_WRDI : FLASH_ANDES_CMD_WREN);
}

#if defined(CONFIG_FLASH_JESD216_API)

static int flash_andes_qspi_sfdp_read(const struct device *dev, off_t addr,
				      void *dest, size_t size)
{
	acquire_device(dev);

	int ret = read_sfdp(dev, addr, dest, size);

	release_device(dev);

	return ret;
}

#endif /* CONFIG_FLASH_JESD216_API */

static int flash_andes_qspi_read_jedec_id(const struct device *dev,
					  uint8_t *id)
{
	if (id == NULL) {
		return -EINVAL;
	}

	acquire_device(dev);

	int ret = flash_andes_qspi_cmd_read(dev, FLASH_ANDES_CMD_RDID, id, 3);

	release_device(dev);

	return ret;
}

static int spi_nor_process_bfp(const struct device *dev,
			       const struct jesd216_param_header *php,
			       const struct jesd216_bfp *bfp)
{
	struct flash_andes_qspi_data *dev_data = dev->data;
	struct jesd216_erase_type *etp = dev_data->erase_types;
	const size_t flash_size = jesd216_bfp_density(bfp) / 8U;

	LOG_DBG("%s: %u MiBy flash", dev->name, (uint32_t)(flash_size >> 20));

	/* Copy over the erase types, preserving their order.  (The
	 * Sector Map Parameter table references them by index.)
	 */
	memset(dev_data->erase_types, 0, sizeof(dev_data->erase_types));
	for (uint8_t ti = 1; ti <= ARRAY_SIZE(dev_data->erase_types); ++ti) {
		if (jesd216_bfp_erase(bfp, ti, etp) == 0) {
			LOG_DBG("Erase %u with %02x",
				(uint32_t)BIT(etp->exp), etp->cmd);
		}
		++etp;
	}

	dev_data->page_size = jesd216_bfp_page_size(php, bfp);
#ifdef CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME
	dev_data->flash_size = flash_size;
#else /* CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME */
	if (flash_size != dev_flash_size(dev)) {
		LOG_ERR("BFP flash size mismatch with devicetree");
		return -EINVAL;
	}
#endif /* CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME */

	return 0;
}

static int spi_nor_process_sfdp(const struct device *dev)
{
	int ret;

#if defined(CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME)

	const uint8_t decl_nph = 2;
	union {
		/* We only process BFP so use one parameter block */
		uint8_t raw[JESD216_SFDP_SIZE(decl_nph)];
		struct jesd216_sfdp_header sfdp;
	} u_header;
	const struct jesd216_sfdp_header *hp = &u_header.sfdp;

	ret = read_sfdp(dev, 0, u_header.raw, sizeof(u_header.raw));
	if (ret != 0) {
		LOG_ERR("SFDP read failed: %d", ret);
		return ret;
	}

	uint32_t magic = jesd216_sfdp_magic(hp);

	if (magic != JESD216_SFDP_MAGIC) {
		LOG_ERR("SFDP magic %08x invalid", magic);
		return -EINVAL;
	}

	LOG_DBG("%s: SFDP v %u.%u AP %x with %u PH", dev->name,
		hp->rev_major, hp->rev_minor, hp->access, 1 + hp->nph);

	const struct jesd216_param_header *php = hp->phdr;
	const struct jesd216_param_header *phpe =
		php + MIN(decl_nph, 1 + hp->nph);

	while (php != phpe) {
		uint16_t id = jesd216_param_id(php);

		LOG_DBG("PH%zu: %04x rev %u.%u: %u DW @ %x",
			(php - hp->phdr), id, php->rev_major, php->rev_minor,
			php->len_dw, jesd216_param_addr(php));

		if (id == JESD216_SFDP_PARAM_ID_BFP) {
			union {
				uint32_t dw[MIN(php->len_dw, 20)];
				struct jesd216_bfp bfp;
			} u_param;
			const struct jesd216_bfp *bfp = &u_param.bfp;

			ret = read_sfdp(dev,
				jesd216_param_addr(php), u_param.dw, sizeof(u_param.dw));

			if (ret != 0) {
				break;
			}

			ret = spi_nor_process_bfp(dev, php, bfp);

			if (ret != 0) {
				break;
			}
		}
		++php;
	}
#elif defined(CONFIG_FLASH_ANDES_QSPI_SFDP_DEVICETREE)
	/* For devicetree we need to synthesize a parameter header and
	 * process the stored BFP data as if we had read it.
	 */
	const struct flash_andes_qspi_config *config = dev->config;
	struct jesd216_param_header bfp_hdr = {
		.len_dw = config->bfp_len,
	};

	ret = spi_nor_process_bfp(dev, &bfp_hdr, config->bfp);
#else
#error Unhandled SFDP choice
#endif
	return ret;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static int setup_pages_layout(const struct device *dev)
{
	int ret = 0;

#if defined(CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME)

	struct flash_andes_qspi_data *dev_data = dev->data;
	const size_t flash_size = dev_flash_size(dev);
	const uint32_t layout_page_size =
		CONFIG_FLASH_ANDES_QSPI_LAYOUT_PAGE_SIZE;
	uint8_t exponent = 0;

	/* Find the smallest erase size. */
	for (size_t i = 0; i < ARRAY_SIZE(dev_data->erase_types); ++i) {
		const struct jesd216_erase_type *etp =
				&dev_data->erase_types[i];

		if ((etp->cmd != 0) &&
		   ((exponent == 0) || (etp->exp < exponent))) {
			exponent = etp->exp;
		}
	}

	if (exponent == 0) {
		return -ENOTSUP;
	}

	uint32_t erase_size = BIT(exponent);

	/* Error if layout page size is not a multiple of smallest
	 * erase size.
	 */
	if ((layout_page_size % erase_size) != 0) {
		LOG_ERR("layout page %u not compatible with erase size %u",
			layout_page_size, erase_size);
		return -EINVAL;
	}

	/* Warn but accept layout page sizes that leave inaccessible
	 * space.
	 */
	if ((flash_size % layout_page_size) != 0) {
		LOG_WRN("layout page %u wastes space with device size %zu",
			layout_page_size, flash_size);
	}

	dev_data->layout.pages_size = layout_page_size;
	dev_data->layout.pages_count = flash_size / layout_page_size;
	LOG_DBG("layout %zu x %zu By pages", dev_data->layout.pages_count,
		dev_data->layout.pages_size);

#elif defined(CONFIG_FLASH_ANDES_QSPI_SFDP_DEVICETREE)
	const struct flash_andes_qspi_config *config = dev->config;
	const struct flash_pages_layout *layout = &config->layout;
	const size_t flash_size = dev_flash_size(dev);
	size_t layout_size = layout->pages_size * layout->pages_count;

	if (!SPI_NOR_IS_SECTOR_ALIGNED(layout->pages_size)) {
		LOG_ERR("ANDES_QSPI_FLASH_LAYOUT_PAGE_SIZE must be "
			"multiple of 4096");
		return -EINVAL;
	}

	if (flash_size != layout_size) {
		LOG_ERR("device size %zu mismatch %zu * %zu By pages",
			flash_size, layout->pages_count, layout->pages_size);
		return -EINVAL;
	}
#else /* CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME */
#error Unhandled SFDP choice
#endif /* CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME */
	return ret;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int qspi_andes_configure(const struct device *dev)
{
	const struct flash_andes_qspi_config *config = dev->config;
	uint32_t base = config->base;

	/* Setting the divisor value to 0xff indicates the SCLK
	 * frequency should be the same as the spi_clock frequency.
	 */
	sys_set_bits(QSPI_TIMIN(base), TIMIN_SCLK_DIV_MSK);

	/* Set Master mode */
	sys_clear_bits(QSPI_TFMAT(base), TFMAT_SLVMODE_MSK);

	/* Disable data merge mode */
	sys_clear_bits(QSPI_TFMAT(base), TFMAT_DATA_MERGE_MSK);

	/* Set data length */
	sys_clear_bits(QSPI_TFMAT(base), TFMAT_DATA_LEN_MSK);
	sys_set_bits(QSPI_TFMAT(base), (7 << TFMAT_DATA_LEN_OFFSET));

	/* Set TX/RX FIFO threshold */
	sys_clear_bits(QSPI_CTRL(base), CTRL_TX_THRES_MSK);
	sys_clear_bits(QSPI_CTRL(base), CTRL_RX_THRES_MSK);

	sys_set_bits(QSPI_CTRL(base), TX_FIFO_THRESHOLD);
	sys_set_bits(QSPI_CTRL(base), RX_FIFO_THRESHOLD);

	return 0;
}

static void qspi_andes_irq_handler(const struct device *dev)
{
	struct flash_andes_qspi_data *data = dev->data;
	const struct flash_andes_qspi_config *config = dev->config;
	uint32_t base = config->base;

	uint32_t i, intr_status, spi_status;
	uint32_t rx_data, cur_tx_fifo_num, cur_rx_fifo_num;
	uint32_t tx_num = 0, tx_data = 0;

	intr_status = sys_read32(QSPI_INTST(base));

	if ((intr_status & INTST_TX_FIFO_INT_MSK) &&
	    !(intr_status & INTST_END_INT_MSK)) {

		spi_status = sys_read32(QSPI_STAT(base));
		cur_tx_fifo_num = GET_TX_NUM(base);

		tx_num = data->tx_fifo_size - cur_tx_fifo_num;

		if (tx_num > data->tx_len) {
			tx_num = data->tx_len;
		}

		for (i = tx_num; i > 0; i--) {
			tx_data = data->tx_buf[data->tx_ptr];
			sys_write32(tx_data, QSPI_DATA(base));
			data->tx_ptr++;
			if (data->tx_ptr == data->tx_len) {
				sys_clear_bits(QSPI_INTEN(base), IEN_TX_FIFO_MSK);
				break;
			}
		}
		sys_write32(INTST_TX_FIFO_INT_MSK, QSPI_INTST(base));
	}

	if (intr_status & INTST_RX_FIFO_INT_MSK) {
		cur_rx_fifo_num = GET_RX_NUM(base);

		for (i = cur_rx_fifo_num; i > 0; i--) {
			rx_data = sys_read32(QSPI_DATA(base));
			data->rx_buf[data->rx_ptr] = rx_data;
			data->rx_ptr++;
			if (data->rx_ptr == data->rx_len) {
				sys_clear_bits(QSPI_INTEN(base), IEN_RX_FIFO_MSK);
				break;
			}
		}
		sys_write32(INTST_RX_FIFO_INT_MSK, QSPI_INTST(base));
	}

	if (intr_status & INTST_END_INT_MSK) {

		/* Clear end interrupt */
		sys_write32(INTST_END_INT_MSK, QSPI_INTST(base));

		/* Disable all SPI interrupts */
		sys_write32(0, QSPI_INTEN(base));

		k_sem_give(&data->device_sync_sem);
	}
}

/**
 * @brief Initialize and configure the flash
 *
 * @param name The flash name
 * @return 0 on success, negative errno code otherwise
 */
static int flash_andes_qspi_init(const struct device *dev)
{
	const struct flash_andes_qspi_config *config = dev->config;
	struct flash_andes_qspi_data *dev_data = dev->data;
	uint32_t base = config->base;

	uint8_t ret, reg = (0x1UL << 6);
	uint8_t jedec_id[SPI_NOR_MAX_ID_LEN];

	/* we should not configure the device we are running on */
	if (config->xip) {
		return -EINVAL;
	}

	k_sem_init(&dev_data->sem, 1, 1);
	k_sem_init(&dev_data->device_sync_sem, 0, 1);

	/* Get the TX/RX FIFO size of this device */
	dev_data->tx_fifo_size = TX_FIFO_SIZE(base);
	dev_data->rx_fifo_size = RX_FIFO_SIZE(base);

	config->cfg_func();
	irq_enable(config->irq_num);

	qspi_andes_configure(dev);

	ret = flash_andes_qspi_read_jedec_id(dev, jedec_id);
	if (ret != 0) {
		LOG_ERR("JEDEC ID read failed: %d", ret);
		return -ENODEV;
	}

#ifndef CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME

	if (memcmp(jedec_id, config->jedec_id, sizeof(jedec_id)) != 0) {
		LOG_ERR("Device id %02x %02x %02x does not match config"
			"%02x %02x %02x", jedec_id[0], jedec_id[1], jedec_id[2],
			config->jedec_id[0], config->jedec_id[1], config->jedec_id[2]);
		return -EINVAL;
	}
#endif

	ret = spi_nor_process_sfdp(dev);
	if (ret != 0) {
		LOG_ERR("SFDP read failed: %d", ret);
		return -ENODEV;
	}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	ret = setup_pages_layout(dev);
	if (ret != 0) {
		LOG_ERR("layout setup failed: %d", ret);
		return -ENODEV;
	}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

	/* Set status register QE bit. */
	flash_andes_qspi_wrsr(dev, reg);

	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_andes_qspi_pages_layout(const struct device *dev,
				     const struct flash_pages_layout **layout,
				     size_t *layout_size)
{
#ifdef CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME
	const struct flash_andes_qspi_data *dev_data = dev->data;

	*layout = &dev_data->layout;
#else /* CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME */
	const struct flash_andes_qspi_config *config = dev->config;

	*layout = &config->layout;
#endif /* CONFIG_FLASH_ANDES_QSPI_SFDP_RUNTIME */
	*layout_size = 1;
}
#endif


static const struct flash_parameters *
flash_andes_qspi_get_parameters(const struct device *dev)
{
	const struct flash_andes_qspi_config *config = dev->config;

	return &config->parameters;
}

static DEVICE_API(flash, flash_andes_qspi_api) = {
	.read = flash_andes_qspi_read,
	.write = flash_andes_qspi_write,
	.erase = flash_andes_qspi_erase,
	.get_parameters = flash_andes_qspi_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_andes_qspi_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = flash_andes_qspi_sfdp_read,
	.read_jedec_id = flash_andes_qspi_read_jedec_id,
#endif
};

#if (CONFIG_XIP)
#define QSPI_ROM_CFG_XIP(node_id) DT_SAME_NODE(node_id, DT_CHOSEN(zephyr_flash))
#else
#define QSPI_ROM_CFG_XIP(node_id) false
#endif

#define LAYOUT_PAGES_PROP(n)						\
	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,				\
		(.layout = {						\
			.pages_count = ((DT_INST_PROP(n, size) / 8) /	\
				CONFIG_FLASH_ANDES_QSPI_LAYOUT_PAGE_SIZE), \
			.pages_size =					\
				CONFIG_FLASH_ANDES_QSPI_LAYOUT_PAGE_SIZE, \
		},							\
		))

#define ANDES_QSPI_SFDP_DEVICETREE_CONFIG(n)				\
	IF_ENABLED(CONFIG_FLASH_ANDES_QSPI_SFDP_DEVICETREE,		\
		(							\
		static const __aligned(4) uint8_t bfp_data_##n[] =	\
			DT_INST_PROP(n, sfdp_bfp);			\
		))

#define ANDES_QSPI_SFDP_DEVICETREE_PROP(n)				\
	IF_ENABLED(CONFIG_FLASH_ANDES_QSPI_SFDP_DEVICETREE,		\
		(.jedec_id = DT_INST_PROP(n, jedec_id),			\
		.flash_size = DT_INST_PROP(n, size) / 8,		\
		.bfp_len = sizeof(bfp_data_##n) / 4,			\
		.bfp = (const struct jesd216_bfp *)bfp_data_##n,	\
		LAYOUT_PAGES_PROP(n)					\
		))

#define FLASH_ANDES_QSPI_INIT(n)					\
	static struct flash_andes_qspi_data flash_andes_qspi_data_##n;	\
	ANDES_QSPI_SFDP_DEVICETREE_CONFIG(n)				\
									\
	static void flash_andes_qspi_configure_##n(void);		\
	static const struct flash_andes_qspi_config			\
		flash_andes_qspi_config_##n = {				\
			.cfg_func = flash_andes_qspi_configure_##n,	\
			.base = DT_REG_ADDR(DT_INST_BUS(n)),		\
			.irq_num = DT_IRQN(DT_INST_BUS(n)),		\
			.parameters = {					\
				.write_block_size = 1,			\
				.erase_value = 0xff			\
			},						\
			.xip = QSPI_ROM_CFG_XIP(DT_DRV_INST(n)),	\
			ANDES_QSPI_SFDP_DEVICETREE_PROP(n)		\
		};							\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			&flash_andes_qspi_init,				\
			NULL,						\
			&flash_andes_qspi_data_##n,			\
			&flash_andes_qspi_config_##n,			\
			POST_KERNEL,					\
			CONFIG_FLASH_ANDES_QSPI_INIT_PRIORITY,		\
			&flash_andes_qspi_api);				\
									\
	static void flash_andes_qspi_configure_##n(void)		\
	{								\
		IRQ_CONNECT(DT_IRQN(DT_INST_BUS(n)),			\
			DT_IRQ(DT_INST_BUS(n), priority),		\
			qspi_andes_irq_handler,				\
			DEVICE_DT_INST_GET(n),				\
			0);						\
	}								\

DT_INST_FOREACH_STATUS_OKAY(FLASH_ANDES_QSPI_INIT)
