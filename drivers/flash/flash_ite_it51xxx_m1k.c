/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_manual_flash_1k

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_ite_it51xxx, CONFIG_FLASH_LOG_LEVEL);

#define SOC_NV_FLASH_NODE  DT_INST(0, soc_nv_flash)
#define FLASH_SIZE         DT_REG_SIZE(SOC_NV_FLASH_NODE)
#define FLASH_READ_MAX_SZ  KB(1)
#define FLASH_WRITE_MAX_SZ KB(1)
#define FLASH_WRITE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define FLASH_ERASE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

/* it51xxx SMFI registers definition */
#define IT51XXX_M1K_REGS_BASE  DT_INST_REG_ADDR_BY_IDX(0, 0)
#define IT51XXX_SMFI_REGS_BASE DT_INST_REG_ADDR_BY_IDX(0, 1)

/* 0x63: Flash Control Register 3 */
#define SMFI_FLHCTRL3R   (IT51XXX_SMFI_REGS_BASE + 0x63)
#define SIFE             BIT(3)
#define FFSPITRI         BIT(0)
/* 0x64: Flash Control Register 4 */
#define SMFI_FLHCTRL4R   (IT51XXX_SMFI_REGS_BASE + 0x64)
#define EN2FLH           BIT(7)
/* 0xa6: Manual Flash 1K Command Control 1 */
#define SMFI_M1KFLHCTRL1 (IT51XXX_M1K_REGS_BASE + 0x00)
#define W1S_M1K_PE       BIT(1)
#define W1S_READ         BIT(0)
/* 0xa8: Manual Flash 1K Command Control 3 */
#define SMFI_M1KFLHCTRL3 (IT51XXX_M1K_REGS_BASE + 0x02)
#define M1KPHY           BIT(5)
/* 0xa9: Manual Flash 1K Command Control 4 */
#define SMFI_M1KFLHCTRL4 (IT51XXX_M1K_REGS_BASE + 0x03)
/* 0xaa: Manual Flash 1K Command Control 5 */
#define SMFI_M1KFLHCTRL5 (IT51XXX_M1K_REGS_BASE + 0x04)
#define M1K_READ_CMD(n)  FIELD_PREP(GENMASK(7, 6), n)
#define M1K_READ         0x01
#define M1K_FETCH        0x02
#define M1K_SFDP         0x03
#define M1K_READ_BCNT(n) FIELD_PREP(GENMASK(1, 0), n)
/* 0xab: Manual Flash 1K Command Control 6 */
#define SMFI_M1KFLHCTRL6 (IT51XXX_M1K_REGS_BASE + 0x05)
/* 0xb9: Manual Flash 1K Command Control 7 */
#define SMFI_M1KFLHCTRL7 (IT51XXX_M1K_REGS_BASE + 0x13)
#define M1K_PE_CMD(n)    FIELD_PREP(GENMASK(7, 6), n)
#define M1K_PROG         0x01
#define M1K_ERASE        0x02
#define M1K_PROG_BCNT(n) FIELD_PREP(GENMASK(1, 0), n)
/* 0xac: M1K DLM BASE Address Byte 0 */
#define SMFI_M1K_DLM_BA0 (IT51XXX_M1K_REGS_BASE + 0x06)
/* 0xad: M1K DLM BASE Address Byte 1 */
#define SMFI_M1K_DLM_BA1 (IT51XXX_M1K_REGS_BASE + 0x07)
/* 0xae: M1K DLM BASE Address Byte 2 */
#define SMFI_M1K_DLM_BA2 (IT51XXX_M1K_REGS_BASE + 0x08)
/* 0xaf: M1K Status Register 1 */
#define SMFI_M1KSTS1     (IT51XXX_M1K_REGS_BASE + 0x09)
/* 0xbc: M1K Status Register 2 */
#define SMFI_M1KSTS2     (IT51XXX_M1K_REGS_BASE + 0x16)
enum m1ksts2 {
	DMA_FETCH_CYC = 3,
	M1K_READ_DUTY,
	M1K_RESERVED,
	M1K_PE_CYC,
};
/* 0xd0: M1K-PROG/M1K-ERASE Lower Bound Address Byte 0 */
#define SMFI_M1K_PE_LBA0    (IT51XXX_M1K_REGS_BASE + 0x2a)
/* 0xd1: M1K-PROG/M1K-ERASE Lower Bound Address Byte 1 */
#define SMFI_M1K_PE_LBA1    (IT51XXX_M1K_REGS_BASE + 0x2b)
/* 0xd2: M1K-PROG/M1K-ERASE Lower Bound Address Byte 2 */
#define SMFI_M1K_PE_LBA2    (IT51XXX_M1K_REGS_BASE + 0x2c)
/* 0xd3: M1K-PROG/M1K-ERASE Lower Bound Address Byte 3 */
#define SMFI_M1K_PE_LBA3    (IT51XXX_M1K_REGS_BASE + 0x2d)
#define M1K_PE_SEL_FSPI     BIT(7)
#define M1K_PE_SEL_FSCE1    BIT(6)
/* 0xd5: M1K-ERASE Upper Bound Address Byte 1 */
#define SMFI_M1K_ERASE_UBA1 (IT51XXX_M1K_REGS_BASE + 0x2f)
#define M1K_ERASE_UBA(n)    FIELD_PREP(GENMASK(7, 2), n)
/* 0xd6: M1K-ERASE Lower Bound Address Byte 2 */
#define SMFI_M1K_ERASE_UBA2 (IT51XXX_M1K_REGS_BASE + 0x30)
/* 0xd7: M1K-ERASE Lower Bound Address Byte 3 */
#define SMFI_M1K_ERASE_UBA3 (IT51XXX_M1K_REGS_BASE + 0x31)
/* 0xd8: M1K-READ Lower Bound Address Byte 0 */
#define SMFI_M1K_READ_LBA0  (IT51XXX_M1K_REGS_BASE + 0x32)
/* 0xd9: M1K-READ Lower Bound Address Byte 1 */
#define SMFI_M1K_READ_LBA1  (IT51XXX_M1K_REGS_BASE + 0x33)
/* 0xda: M1K-READ Lower Bound Address Byte 2 */
#define SMFI_M1K_READ_LBA2  (IT51XXX_M1K_REGS_BASE + 0x34)
/* 0xdb: M1K-READ Lower Bound Address Byte 3 */
#define SMFI_M1K_READ_LBA3  (IT51XXX_M1K_REGS_BASE + 0x35)
#define M1K_READ_SEL_FSPI   BIT(7)
#define M1K_READ_SEL_FSCE1  BIT(6)

#define M1K_READ_BCNT_MASK GENMASK(9, 0)
#define M1K_PROG_BCNT_MASK GENMASK(9, 0)

enum flash_select {
	INTERNAL,
	EXTERNAL_FSPI_CS0,
	EXTERNAL_FSPI_CS1,
};

struct flash_it51xxx_dev_data {
	struct k_sem sem;
};

struct flash_it51xxx_config {
	const struct pinctrl_dev_config *pcfg;
	enum flash_select m1k_sel_access_flash;
};

static bool is_valid_range(off_t offset, uint32_t len)
{
	return (offset >= 0) && ((offset + len) <= FLASH_SIZE);
}

static void flash_set_m1k_read_lba(const struct device *dev, uint32_t lb_addr)
{
	uint8_t m1k_pe_lba3_value;

	m1k_pe_lba3_value = sys_read8(SMFI_M1K_READ_LBA3);
	/* Start address of M1K-READ[27:24] */
	sys_write8(m1k_pe_lba3_value | FIELD_GET(GENMASK(27, 24), lb_addr), SMFI_M1K_READ_LBA3);
	/* Start address of M1K-READ[23:16] */
	sys_write8(FIELD_GET(GENMASK(23, 16), lb_addr), SMFI_M1K_READ_LBA2);
	/* Start address of M1K-READ[15:8] */
	sys_write8(FIELD_GET(GENMASK(15, 8), lb_addr), SMFI_M1K_READ_LBA1);
	/* Start address of M1K-READ[7:0] */
	sys_write8(FIELD_GET(GENMASK(7, 0), lb_addr), SMFI_M1K_READ_LBA0);
}

static void flash_set_m1k_pe_lba(const struct device *dev, uint32_t lb_addr)
{
	uint8_t m1k_pe_lba3_value;

	m1k_pe_lba3_value = sys_read8(SMFI_M1K_PE_LBA3);
	/* Start address of M1K-PROG / Lower bound address of M1K-ERASE[27:24] */
	sys_write8(m1k_pe_lba3_value | FIELD_GET(GENMASK(27, 24), lb_addr), SMFI_M1K_PE_LBA3);
	/* Start address of M1K-PROG / Lower bound address of M1K-ERASE[23:16] */
	sys_write8(FIELD_GET(GENMASK(23, 16), lb_addr), SMFI_M1K_PE_LBA2);
	/* Start address of M1K-PROG / Lower bound address of M1K-ERASE[15:8] */
	sys_write8(FIELD_GET(GENMASK(15, 8), lb_addr), SMFI_M1K_PE_LBA1);
	/* Start address of M1K-PROG / Lower bound address of M1K-ERASE[7:0] */
	sys_write8(FIELD_GET(GENMASK(7, 0), lb_addr), SMFI_M1K_PE_LBA0);
}

static void flash_set_m1k_erase_uba(const struct device *dev, uint32_t ub_addr)
{
	/* Upper bound address of M1K-ERASE[27:24] */
	sys_write8(FIELD_GET(GENMASK(27, 24), ub_addr), SMFI_M1K_ERASE_UBA3);
	/* Upper bound address of M1K-ERASE[23:16] */
	sys_write8(FIELD_GET(GENMASK(23, 16), ub_addr), SMFI_M1K_ERASE_UBA2);
	/* Upper bound address of M1K-ERASE[15:10] */
	sys_write8(M1K_ERASE_UBA(FIELD_GET(GENMASK(15, 10), ub_addr)), SMFI_M1K_ERASE_UBA1);
}

static void flash_set_m1k_dlm_ba(const struct device *dev, uint32_t dlm_addr)
{
	/* M1K DLM base address[17:16] */
	sys_write8(FIELD_GET(GENMASK(17, 16), dlm_addr), SMFI_M1K_DLM_BA2);
	/* M1K DLM base address[15:8] */
	sys_write8(FIELD_GET(GENMASK(15, 8), dlm_addr), SMFI_M1K_DLM_BA1);
	/* M1K DLM base address[7:0] */
	sys_write8(FIELD_GET(GENMASK(7, 0), dlm_addr), SMFI_M1K_DLM_BA0);
}

static int flash_wait_status(const struct device *dev, enum m1ksts2 state)
{
	/* Waiting for M1K-READ to complete */
	if (WAIT_FOR(!IS_BIT_SET(sys_read8(SMFI_M1KSTS2), state), INT_MAX, NULL) == false) {
		LOG_ERR("%s: Timeout waiting for status", __func__);

		return -ETIMEDOUT;
	}

	return 0;
}

static int m1k_flash_read(const struct device *dev, off_t offset, const void *dst_data, size_t len)
{
	int ret;
	uint16_t count;
	uint8_t m1kflhctrl5_cmd;

	/* It's the start address of M1K-READ */
	flash_set_m1k_read_lba(dev, offset);

	/* M1K DLM base address */
	flash_set_m1k_dlm_ba(dev, (uint32_t)dst_data);

	/* M1k-READ size (Maximum 1024 bytes) */
	count = (len - 1) & M1K_READ_BCNT_MASK;

	m1kflhctrl5_cmd = sys_read8(SMFI_M1KFLHCTRL5) & ~GENMASK(1, 0);
	/* M1K-READ byte count[9:8] */
	sys_write8(m1kflhctrl5_cmd | M1K_READ_BCNT(FIELD_GET(GENMASK(9, 8), count)),
		   SMFI_M1KFLHCTRL5);
	/* M1K-READ byte count[7:0] */
	sys_write8(FIELD_GET(GENMASK(7, 0), count), SMFI_M1KFLHCTRL4);

	m1kflhctrl5_cmd = sys_read8(SMFI_M1KFLHCTRL5) & ~GENMASK(7, 6);
	/* Read data from the flash to DLM */
	sys_write8(m1kflhctrl5_cmd | M1K_READ_CMD(M1K_READ), SMFI_M1KFLHCTRL5);

	/* Write-1-Start M1K-READ */
	sys_write8(W1S_READ, SMFI_M1KFLHCTRL1);
	ret = flash_wait_status(dev, M1K_READ_DUTY);

	/* Reset the M1K setting and counter to 0 */
	sys_write8(0, SMFI_M1KFLHCTRL4);
	sys_write8(0, SMFI_M1KFLHCTRL5);

	return ret;
}

static int m1k_flash_write(const struct device *dev, off_t offset, const void *src_data, size_t len)
{
	int ret;
	uint16_t count;
	uint8_t m1kflhctrl7_cmd;

	/* It's the start address of M1K-PROG */
	flash_set_m1k_pe_lba(dev, offset);

	/* M1K DLM base address */
	flash_set_m1k_dlm_ba(dev, (uint32_t)src_data);

	/* M1k-PROG size (Maximum 1024 bytes) */
	count = (len - 1) & M1K_PROG_BCNT_MASK;

	m1kflhctrl7_cmd = sys_read8(SMFI_M1KFLHCTRL7) & ~GENMASK(1, 0);
	/* M1K-PROG byte count[9:8] */
	sys_write8(m1kflhctrl7_cmd | M1K_PROG_BCNT(FIELD_GET(GENMASK(9, 8), count)),
		   SMFI_M1KFLHCTRL7);
	/* M1K-PROG byte count[7:0] */
	sys_write8(FIELD_GET(GENMASK(7, 0), count), SMFI_M1KFLHCTRL6);

	m1kflhctrl7_cmd = sys_read8(SMFI_M1KFLHCTRL7) & ~GENMASK(7, 6);
	/* Copy byte count data from the DLM to flash */
	sys_write8(m1kflhctrl7_cmd | M1K_PE_CMD(M1K_PROG), SMFI_M1KFLHCTRL7);

	/* Write-1-Start M1K-PROG/M1K-ERASE */
	sys_write8(W1S_M1K_PE, SMFI_M1KFLHCTRL1);
	ret = flash_wait_status(dev, M1K_PE_CYC);

	/* Reset counter to 0 */
	sys_write8(0, SMFI_M1KFLHCTRL6);
	sys_write8(0, SMFI_M1KFLHCTRL7);

	return ret;
}

static int m1k_flash_erase(const struct device *dev, off_t offset, size_t len)
{
	int ret;
	uint8_t m1kflhctrl7_cmd;

	/* It's the lower bound address of M1K-ERASE */
	flash_set_m1k_pe_lba(dev, offset);

	/* It's the upper bound address of M1K-ERASE */
	flash_set_m1k_erase_uba(dev, offset + FLASH_ERASE_BLK_SZ);

	m1kflhctrl7_cmd = sys_read8(SMFI_M1KFLHCTRL7) & ~GENMASK(7, 6);
	/* Erase the flash within an address range */
	sys_write8(m1kflhctrl7_cmd | M1K_PE_CMD(M1K_ERASE), SMFI_M1KFLHCTRL7);

	/* Write-1-Start M1K-ERASE */
	sys_write8(W1S_M1K_PE, SMFI_M1KFLHCTRL1);
	ret = flash_wait_status(dev, M1K_PE_CYC);

	return ret;
}

/* Read data from flash */
static int flash_it51xxx_read(const struct device *dev, off_t offset, void *dst_data, size_t len)
{
	struct flash_it51xxx_dev_data *data = dev->data;
	int ret;
	uint8_t *dst = (uint8_t *)dst_data;

	LOG_DBG("%s: offset=%lx, data addr=%p, len=%u", __func__, offset, (uint8_t *)dst_data, len);

	if (len == 0) {
		return 0;
	}

	if (!is_valid_range(offset, len)) {
		LOG_ERR("Out of boundaries: FLASH_SIZE=%#x, offset=%#lx, len=%u", FLASH_SIZE,
			offset, len);
		return -EINVAL;
	}

	k_sem_take(&data->sem, K_FOREVER);

	/* For M1K_READ, setting 1 will not plus EC image location */
	sys_write8(sys_read8(SMFI_M1KFLHCTRL3) | M1KPHY, SMFI_M1KFLHCTRL3);

	while (len > 0) {
		size_t read_len = MIN(len, FLASH_READ_MAX_SZ);

		ret = m1k_flash_read(dev, offset, dst, read_len);
		if (ret != 0) {
			LOG_ERR("%s: failed at offset=%#lx", __func__, offset);
			break;
		}

		offset += read_len;
		dst += read_len;
		len -= read_len;
	}

	/* Reset the M1K setting and counter to 0 */
	sys_write8(0, SMFI_M1KFLHCTRL3);
	sys_write8(0, SMFI_M1KFLHCTRL4);
	sys_write8(0, SMFI_M1KFLHCTRL5);

	k_sem_give(&data->sem);

	return ret;
}

/* Write data to the flash, page by page */
static int flash_it51xxx_write(const struct device *dev, off_t offset, const void *src_data,
			       size_t len)
{
	struct flash_it51xxx_dev_data *data = dev->data;
	int ret;
	const uint8_t *src = (const uint8_t *)src_data;

	LOG_DBG("%s: offset=%lx, data addr=%p, len=%u", __func__, offset, (const uint8_t *)src_data,
		len);

	if (len == 0) {
		return 0;
	}

	if (!is_valid_range(offset, len)) {
		LOG_ERR("Out of boundaries: FLASH_SIZE=%#x, offset=%#lx, len=%u", FLASH_SIZE,
			offset, len);
		return -EINVAL;
	}

	k_sem_take(&data->sem, K_FOREVER);

	while (len > 0) {
		size_t write_len = MIN(len, FLASH_WRITE_MAX_SZ);

		ret = m1k_flash_write(dev, offset, src, write_len);
		if (ret != 0) {
			LOG_ERR("%s: failed at offset=%#lx", __func__, offset);
			break;
		}

		offset += write_len;
		src += write_len;
		len -= write_len;
	}

	/* Reset counter to 0 */
	sys_write8(0, SMFI_M1KFLHCTRL6);
	sys_write8(0, SMFI_M1KFLHCTRL7);

	k_sem_give(&data->sem);

	return ret;
}

/* Erase multiple blocks */
static int flash_it51xxx_erase(const struct device *dev, off_t offset, size_t len)
{
	struct flash_it51xxx_dev_data *data = dev->data;
	int ret;

	LOG_DBG("%s: offset=%lx, len=%u", __func__, offset, len);

	if (len == 0) {
		return 0;
	}

	if (!is_valid_range(offset, len)) {
		LOG_ERR("Out of boundaries: FLASH_SIZE=%#x, offset=%#lx, len=%u", FLASH_SIZE,
			offset, len);
		return -EINVAL;
	}

	/* Check that the offset and length are multiples of the erase block size */
	if ((offset % FLASH_ERASE_BLK_SZ) || (len % FLASH_ERASE_BLK_SZ)) {
		LOG_ERR("Erase range is not a multiple of the block size. offset=%#lx, len=%u",
			offset, len);
		return -EINVAL;
	}

	k_sem_take(&data->sem, K_FOREVER);

	while (len > 0) {
		ret = m1k_flash_erase(dev, offset, len);
		if (ret != 0) {
			LOG_ERR("%s: failed at offset=%#lx", __func__, offset);
			break;
		}

		offset += FLASH_ERASE_BLK_SZ;
		len -= FLASH_ERASE_BLK_SZ;
	}

	k_sem_give(&data->sem);

	return ret;
}

static const struct flash_parameters flash_it51xxx_parameters = {
	.write_block_size = FLASH_WRITE_BLK_SZ,
	.erase_value = 0xff,
};

static const struct flash_parameters *flash_it51xxx_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_it51xxx_parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout dev_layout = {
	.pages_count = FLASH_SIZE / FLASH_ERASE_BLK_SZ,
	.pages_size = FLASH_ERASE_BLK_SZ,
};

static void flash_it51xxx_pages_layout(const struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, flash_it51xxx_api) = {
	.read = flash_it51xxx_read,
	.write = flash_it51xxx_write,
	.erase = flash_it51xxx_erase,
	.get_parameters = flash_it51xxx_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_it51xxx_pages_layout,
#endif
};

static int flash_it51xxx_init(const struct device *dev)
{
	const struct flash_it51xxx_config *cfg = dev->config;
	struct flash_it51xxx_dev_data *data = dev->data;
	int ret;
	uint8_t flhctrl3r_val;

	LOG_INF("%s: M1K select access flash=%d", __func__, cfg->m1k_sel_access_flash);

	flhctrl3r_val = sys_read8(SMFI_FLHCTRL3R);
	if (cfg->m1k_sel_access_flash) {
		/* Enable SPI flash and SPI pins are normal operation */
		sys_write8((flhctrl3r_val | SIFE) & ~FFSPITRI, SMFI_FLHCTRL3R);

		/* M1K-READ will access the SPI flash (FSPI) */
		sys_write8(M1K_READ_SEL_FSPI, SMFI_M1K_READ_LBA3);
		/* M1K-PROG/M1K-ERASE will access the SPI flash (FSPI) */
		sys_write8(M1K_PE_SEL_FSPI, SMFI_M1K_PE_LBA3);

		/* Set the pin to FSPI alternate function. */
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("%s: Failed to configure FSPI pins", dev->name);
			return ret;
		}

		if (cfg->m1k_sel_access_flash == EXTERNAL_FSPI_CS1) {
			/* M1K-READ will access the SPI flash on FSCE1# */
			sys_write8(sys_read8(SMFI_M1K_READ_LBA3) | M1K_READ_SEL_FSCE1,
				   SMFI_M1K_READ_LBA3);
			/* M1K-PROG/M1K-ERASE will access the SPI flash on FSCE1# */
			sys_write8(sys_read8(SMFI_M1K_PE_LBA3) | M1K_PE_SEL_FSCE1,
				   SMFI_M1K_PE_LBA3);
			/* Enable two-flash */
			sys_write8(sys_read8(SMFI_FLHCTRL4R) | EN2FLH, SMFI_FLHCTRL4R);
		}
	} else {
		/* Use internal flash, the SPI pins should be set to tri-state */
		sys_write8((flhctrl3r_val & ~SIFE) | FFSPITRI, SMFI_FLHCTRL3R);
	}

	/* Initialize mutex for flash controller */
	k_sem_init(&data->sem, 1, 1);

	return 0;
}

static struct flash_it51xxx_dev_data flash_it51xxx_data;

PINCTRL_DT_INST_DEFINE(0);

static const struct flash_it51xxx_config flash_it51xxx_cfg = {
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.m1k_sel_access_flash = DT_INST_ENUM_IDX(0, m1k_sel_access_flash),
};

BUILD_ASSERT(!((DT_INST_ENUM_IDX(0, m1k_sel_access_flash) >= EXTERNAL_FSPI_CS0) &&
	       !DT_INST_NODE_HAS_PROP(0, pinctrl_0)),
	     "Access external-fspi-cs0/cs1, pinctrl must be configured.");

DEVICE_DT_INST_DEFINE(0, flash_it51xxx_init, NULL, &flash_it51xxx_data, &flash_it51xxx_cfg,
		      PRE_KERNEL_2, CONFIG_FLASH_INIT_PRIORITY, &flash_it51xxx_api);
