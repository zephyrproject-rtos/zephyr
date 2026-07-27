/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_nand_g1_flash

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/flash/mchp_nand_g1.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_mchp_nand_g1, CONFIG_FLASH_LOG_LEVEL);

#define MAX_PAGE_ACYCLE  5
#define MAX_BLOCK_ACYCLE 3

struct nand_config {
	const struct nand_info *ids;
	const uint32_t num_ids;
	const struct nand_op *ops;
	const struct device *nc_dev;
	struct gpio_dt_spec cs_gpio;
};

struct nand_data {
	struct nand_chip chip;
	struct k_mutex mutex;
};

const struct nand_info nand_ids[] = {
	/* Micron MT29F4G08ABAEA 512MB */
	{ .id = { 0x2C, 0xDC }, 0x1000, 0xE0, 0x40, 0x800, 8, 0x200 },
	/* Winbond W29N01HV 128MB */
	{ .id = { 0xEF, 0xF1 }, 0x800, 0x40, 0x40, 0x400, 1, 0x200 },
};

const struct nand_op nand_ops[OP_MAX] = {
	[OP_RESET]    = { CMD(CMD_RESET)   , FLAG(F_BUSY) },
	[OP_READID]   = { CMD(CMD_READID)  , ACYCLE(1), DCYCLE(5), ADDR(0) },
	[OP_ONFI]     = { CMD(CMD_READID)  , ACYCLE(1), DCYCLE(4), ADDR(0x20) },
	[OP_PARAM]    = { CMD(CMD_PARAM)   , ACYCLE(1), DCYCLE(256), ADDR(0), FLAG(F_BUSY) },
	[OP_GET_FEAT] = { CMD(CMD_GET_FEAT), ACYCLE(1), DCYCLE(4), FLAG(F_ADDR | F_BUSY) },
	[OP_SET_FEAT] = { CMD(CMD_SET_FEAT), ACYCLE(1), DCYCLE(4), FLAG(F_OUT | F_ADDR) },
	[OP_STATUS]   = { CMD(CMD_STATUS)  , DCYCLE(1) },
	[OP_READ]     = { CMD(CMD_READ0)   , CMD2(CMD_READ2), FLAG(F_READ) },
	[OP_WRITE]    = { CMD(CMD_SEQIN)   , FLAG(F_WRITE) },
	[OP_PROGRAM]  = { CMD(CMD_PAGEPROG), FLAG(F_BUSY) },
	[OP_ERASE]    = { CMD(CMD_ERASE1)  , CMD2(CMD_ERASE2), FLAG(F_ERASE) },
};

static uint16_t onfi_crc16(uint8_t const *p)
{
	uint16_t crc = ONFI_CRC_BASE;
	size_t len = ONFI_CRC_SIZE;

	while (len--) {
		crc ^= *p++ << 8;
		for (int i = 0; i < 8; i++) {
			crc = (crc << 1) ^ ((crc & 0x8000) ? 0x8005 : 0);
		}
	}

	return crc;
}

#define PAGE_ADDRS(nand, page, addrs)   _to_addrs((nand), (page), (addrs), 0, 0)
#define OOB_ADDRS(nand, page, addrs)    _to_addrs((nand), (page), (addrs), 1, 0)
#define BLOCK_ADDRS(nand, block, addrs) _to_addrs((nand), (block), (addrs), 0, 1)
static void _to_addrs(struct nand_info *nand, uint32_t addr, uint8_t *addrs,
		      uint8_t is_oob, uint8_t is_block)
{
	int i;

	if (!is_block) {
		for (i = 0; i < (nand->addrcycle >> 4); i++) {
			if (is_oob) {
				*addrs++ = (nand->pagesize >> (i * 8)) & 0xFF;
			} else {
				*addrs++ = 0;
			}
		}
	}

	for (i = 0; i < (nand->addrcycle & 0xF); i++) {
		if (!is_block) {
			*addrs++ = (addr >> (i * 8)) & 0xFF;
		} else {
			*addrs++ = ((addr * nand->blockpages) >> (i * 8)) & 0xFF;
		}
	}
}

void update_acycle(struct nand_info *nand)
{
	uint32_t column = nand->pagesize;
	uint32_t row = nand->blockpages * nand->blocknum;

	nand->addrcycle = ROUND_UP((find_msb_set(row) - 1), 8) >> 3;
	nand->addrcycle |= (ROUND_UP(find_msb_set(column), 8) >> 3) << 4;
}

static int exec_op(const struct device *dev, struct nand_chip *chip,
		   const struct nand_op *op, uint8_t *addrs, uint8_t *buf)
{
	const struct nand_config *cfg = dev->config;
	const struct device *nc_dev = cfg->nc_dev;
	const struct nand_controller_ops *nc_ops =
		(const struct nand_controller_ops *)nc_dev->api;
	int ret;

	if (cfg->cs_gpio.port) {
		gpio_pin_set_dt(&cfg->cs_gpio, 1);
	}

	ret = nc_ops->exec_op(nc_dev, chip, op, addrs, buf);

	if (cfg->cs_gpio.port) {
		gpio_pin_set_dt(&cfg->cs_gpio, 0);
	}

	return ret;
}

static void data_in(const struct device *dev, struct nand_chip *chip,
		    uint8_t *buf, uint32_t len)
{
	const struct nand_config *cfg = dev->config;
	const struct device *nc_dev = cfg->nc_dev;
	const struct nand_controller_ops *nc_ops =
		(const struct nand_controller_ops *)nc_dev->api;

	if (cfg->cs_gpio.port) {
		gpio_pin_set_dt(&cfg->cs_gpio, 1);
	}

	nc_ops->data_in(nc_dev, chip, buf, len);

	if (cfg->cs_gpio.port) {
		gpio_pin_set_dt(&cfg->cs_gpio, 0);
	}
}

static void __maybe_unused data_out(const struct device *dev, struct nand_chip *chip,
				    const uint8_t *buf, uint32_t len)
{
	const struct nand_config *cfg = dev->config;
	const struct device *nc_dev = cfg->nc_dev;
	const struct nand_controller_ops *nc_ops =
		(const struct nand_controller_ops *)nc_dev->api;

	if (cfg->cs_gpio.port) {
		gpio_pin_set_dt(&cfg->cs_gpio, 1);
	}

	nc_ops->data_out(nc_dev, chip, buf, len);

	if (cfg->cs_gpio.port) {
		gpio_pin_set_dt(&cfg->cs_gpio, 0);
	}
}

static int onfi_detect(const struct device *dev)
{
	const struct nand_config *cfg = dev->config;
	const struct nand_op *ops = cfg->ops;
	struct nand_data *data = dev->data;
	struct nand_chip *chip = &data->chip;
	struct nand_info *nand = &chip->nand;
	uint8_t buf[ONFI_PARAM_SIZE];
	int i, ret;

	ret = exec_op(dev, chip, &ops[OP_ONFI], NULL, buf);
	if (ret) {
		return ret;
	}

	if ((buf[0] != 'O') ||
	    (buf[1] != 'N') ||
	    (buf[2] != 'F') ||
	    (buf[3] != 'I')) {
		LOG_INF("NAND: ONFI not supported");
		return -EINVAL;
	}
	LOG_INF("NAND: ONFI NAND Flash detected");

	for (i = 0; i < ONFI_PARAM_PAGES; i++) {
		if (i == 0) {
			ret = exec_op(dev, chip, &ops[OP_PARAM], NULL, buf);
			if (ret) {
				return ret;
			}
		} else {
			data_in(dev, chip, buf, ONFI_PARAM_SIZE);
		}

		if (onfi_crc16(buf) == sys_get_le16(&buf[PARAMS_OFFSET_CRC])) {
			break;
		}
	}

	if (i == ONFI_PARAM_PAGES) {
		LOG_ERR("NAND: ONFI CRC error!");
		return -EINVAL;
	}

	nand->pagesize   = sys_get_le32(&buf[PARAMS_OFFSET_PAGESIZE]);
	nand->oobsize    = sys_get_le16(&buf[PARAMS_OFFSET_SPARESIZE]);
	nand->blockpages = sys_get_le32(&buf[PARAMS_OFFSET_BLOCKSIZE]);
	nand->blocknum   = sys_get_le32(&buf[PARAMS_OFFSET_UNITSIZE]) *
			   buf[PARAMS_OFFSET_UNIT_NUM];
	nand->eccbits    = buf[PARAMS_OFFSET_ECC_BITS];
	nand->eccsize    = ONFI_DEF_ECC_SIZE;
	nand->features   = sys_get_le16(&buf[PARAMS_OFFSET_FEATURES]);
	nand->opt_cmd    = sys_get_le16(&buf[PARAMS_OFFSET_OPT_CMD]);
	nand->timingmode = sys_get_le16(&buf[PARAMS_OFFSET_TIMING]);
	nand->addrcycle  = buf[PARAMS_OFFSET_ADDRCYCLE];
	nand->cellbits   = buf[PARAMS_OFFSET_CELLBITS];

	if (nand->eccbits == 0xFF) {
		if (nand->features & PARAMS_FEATURE_EXTENDED_PARAM) {
			LOG_ERR("NAND: ONFI support for extended parameter page is TBD!");
			return -ENOTSUP;
		} else  {
			LOG_ERR("NAND: ONFI incorrect ECC bits 0xFF!");
			return -EINVAL;
		}
	}

	buf[PARAMS_OFFSET_MOD_NAME - 1] = '\0';
	buf[PARAMS_OFFSET_MFR_ID - 1]   = '\0';
	LOG_DBG("NAND: Manufacturer ID: 0x%02x", buf[PARAMS_OFFSET_MFR_ID]);
	LOG_DBG("      %s", &buf[PARAMS_OFFSET_MFR_NAME]);
	LOG_DBG("      %s", &buf[PARAMS_OFFSET_MOD_NAME]);
	LOG_DBG("      %cLC NAND Flash",
		nand->cellbits == 1 ? 'S' :
		nand->cellbits == 2 ? 'M' :
		nand->cellbits == 3 ? 'T' :
		nand->cellbits == 4 ? 'Q' :
		nand->cellbits == 5 ? 'P' : 'x');
	LOG_DBG("      features    : 0x%04x", nand->features);
	LOG_DBG("      opt_cmd     : 0x%04x", nand->opt_cmd);
	LOG_DBG("      timing_mode : 0x%04x", nand->timingmode);
	LOG_DBG("      addrcycle   : 0x%02x", nand->addrcycle);

	return 0;
}

static int nand_init(const struct device *dev)
{
	const struct nand_config *cfg = dev->config;
	const struct nand_op *ops = cfg->ops;
	struct nand_data *data = dev->data;
	struct nand_chip *chip = &data->chip;
	struct nand_info *nand = &chip->nand;
	int i, ret = 0;

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = exec_op(dev, chip, &ops[OP_RESET], NULL, NULL);
	if (ret) {
		goto OUT;
	}

	ret = exec_op(dev, chip, &ops[OP_READID], NULL, nand->id);
	if (ret) {
		goto OUT;
	}

	if (onfi_detect(dev)) {
		for (i = 0; i < cfg->num_ids; i++) {
			if ((nand->mfr_id == cfg->ids[i].mfr_id) &&
			    (nand->dev_id == cfg->ids[i].dev_id)) {
				LOG_INF("NAND: NAND Flash IDs table matched");
				*nand = cfg->ids[i];
				break;
			}
		}

		if (i == cfg->num_ids) {
			LOG_INF("NAND: NAND Flash ID: 0x%02x 0x%02x not found in IDs table",
				nand->mfr_id, nand->dev_id);
			ret = -EINVAL;
			goto OUT;
		}

		if (!nand->addrcycle) {
			update_acycle(nand);
		}
	}

	LOG_INF("NAND: NAND Flash found, Manufacturer ID: 0x%02x, Chip ID: 0x%02x",
		nand->mfr_id, nand->dev_id);
	LOG_INF("      %d MiB, erase size: %d KiB, page size: %d, OOB size: %d",
		(nand->pagesize * nand->blockpages * nand->blocknum) >> 20,
		(nand->pagesize * nand->blockpages) >> 10,
		nand->pagesize, nand->oobsize);
	LOG_INF("      ECC bits: %d, sector size: %d",
		nand->eccbits, nand->eccsize);

OUT:
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int nand_ecc_init(const struct device *dev)
{
	const struct nand_config *cfg = dev->config;
	struct nand_data *data = dev->data;
	const struct device *nc_dev = cfg->nc_dev;
	struct nand_chip *chip = &data->chip;
	const struct nand_controller_ops *nc_ops =
		(const struct nand_controller_ops *)nc_dev->api;
	int ret;

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = nc_ops->ecc_init(nc_dev, chip);

	k_mutex_unlock(&data->mutex);

	return ret;
}

static inline uint8_t page_acycle(struct nand_info *nand)
{
	return (nand->addrcycle & 0xF) + (nand->addrcycle >> 4);
}

static inline uint8_t block_acycle(struct nand_info *nand)
{
	return nand->addrcycle & 0xF;
}

static int _nand_read(const struct device *dev, unsigned int page, void *buf,
		      int oob_req, int is_raw, int is_oob)
{
	const struct nand_config *cfg = dev->config;
	struct nand_data *data = dev->data;
	struct nand_chip *chip = &data->chip;
	struct nand_info *nand = &chip->nand;
	struct nand_op op = cfg->ops[OP_READ];
	uint8_t addrs[MAX_PAGE_ACYCLE] = {0};
	int ret;

	if ((buf == NULL) ||
	    (page >= nand->blockpages * nand->blocknum)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	op.acycle = page_acycle(nand);
	if (is_oob) {
		OOB_ADDRS(nand, page, addrs);
		op.flag |= F_OOB;
	} else {
		PAGE_ADDRS(nand, page, addrs);
		op.flag |= F_PAGE;

		if (oob_req) {
			op.flag |= F_OOB;
		}
		if (is_raw) {
			op.flag |= F_RAW;
		}
	}

	ret = exec_op(dev, chip, &op, addrs, buf);

	k_mutex_unlock(&data->mutex);

	return ret;
}

static int _nand_write(const struct device *dev, unsigned int page, const void *buf,
		       int oob_req, int is_raw, int is_oob)
{
	const struct nand_config *cfg = dev->config;
	struct nand_data *data = dev->data;
	const struct nand_op *ops = cfg->ops;
	struct nand_chip *chip = &data->chip;
	struct nand_info *nand = &chip->nand;
	struct nand_op op = ops[OP_WRITE];
	uint8_t addrs[MAX_PAGE_ACYCLE] = {0};
	uint8_t status;
	int ret = 0;

	if ((buf == NULL) ||
	    (page >= nand->blockpages * nand->blocknum)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	op.acycle = page_acycle(nand);
	if (is_oob) {
		OOB_ADDRS(nand, page, addrs);
		op.flag |= F_OOB;
	} else {
		PAGE_ADDRS(nand, page, addrs);
		op.flag |= F_PAGE;

		if (oob_req) {
			op.flag |= F_OOB;
		}
		if (is_raw) {
			op.flag |= F_RAW;
		}
	}

	ret = exec_op(dev, chip, &op, addrs, (void *)buf);
	if (ret) {
		goto OUT;
	}

	ret = exec_op(dev, chip, &ops[OP_PROGRAM], NULL, NULL);
	if (ret) {
		goto OUT;
	}

	ret = exec_op(dev, chip, &ops[OP_STATUS], NULL, &status);
	if (ret) {
		goto OUT;
	}

	if (status & STATUS_FAIL) {
		ret = -EIO;
	}

OUT:
	k_mutex_unlock(&data->mutex);

	return ret;
}

int z_impl_nand_read_page(const struct device *dev,
			  unsigned int page, void *buf, int oob_req)
{
	return _nand_read(dev, page, buf, oob_req, 0, 0);
}

int z_impl_nand_write_page(const struct device *dev,
			   unsigned int page, const void *buf, int oob_req)
{
	return _nand_write(dev, page, buf, oob_req, 0, 0);
}

int z_impl_nand_read_page_raw(const struct device *dev,
			      unsigned int page, void *buf, int oob_req)
{
	return _nand_read(dev, page, buf, oob_req, 1, 0);
}

int z_impl_nand_write_page_raw(const struct device *dev,
			       unsigned int page, const void *buf, int oob_req)
{
	return _nand_write(dev, page, buf, oob_req, 1, 0);
}

int z_impl_nand_read_oob(const struct device *dev, unsigned int page, void *buf)
{
	return _nand_read(dev, page, buf, 0, 0, 1);
}

int z_impl_nand_write_oob(const struct device *dev, unsigned int page, const void *buf)
{
	return _nand_write(dev, page, buf, 0, 0, 1);
}

int z_impl_nand_erase(const struct device *dev, unsigned int block)
{
	const struct nand_config *cfg = dev->config;
	const struct nand_op *ops = cfg->ops;
	struct nand_data *data = dev->data;
	struct nand_chip *chip = &data->chip;
	struct nand_info *nand = &chip->nand;
	struct nand_op op = ops[OP_ERASE];
	uint8_t addrs[MAX_BLOCK_ACYCLE] = {0};
	uint8_t status;
	int ret = 0;

	if (block >= nand->blocknum) {
		return -EINVAL;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	op.acycle = block_acycle(nand);
	BLOCK_ADDRS(nand, block, addrs);

	ret = exec_op(dev, chip, &op, addrs, NULL);
	if (ret) {
		goto OUT;
	}

	ret = exec_op(dev, chip, &ops[OP_STATUS], NULL, &status);
	if (ret) {
		goto OUT;
	}

	if (status & STATUS_FAIL) {
		ret = -EIO;
	}

OUT:
	k_mutex_unlock(&data->mutex);

	return ret;
}

int z_impl_nand_info(const struct device *dev, struct nand_info *info)
{
	struct nand_data *data = dev->data;

	if (info == NULL) {
		return -EINVAL;
	}

	*info = data->chip.nand;

	return 0;
}

static int nand_get_size(const struct device *dev, uint64_t *size)
{
	struct nand_data *data = dev->data;
	struct nand_info *nand = &data->chip.nand;

	*size = (uint64_t)(nand->pagesize * nand->blockpages * nand->blocknum);

	return 0;
}

static DEVICE_API(flash, flash_nand_api) = {
	.get_size = nand_get_size,
};

static int flash_nand_init(const struct device *dev)
{
	const struct nand_config *cfg = dev->config;
	struct nand_data *data = dev->data;
	struct nand_chip *chip = &data->chip;
	const struct device *nc_dev = cfg->nc_dev;
	int ret;

	if (cfg->cs_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->cs_gpio)) {
			LOG_ERR("NAND: CS GPIO controller is not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->cs_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_ERR("NAND: Failed to configure CS GPIO ret=%d", ret);
			return ret;
		}
	}

	if (!device_is_ready(nc_dev)) {
		LOG_ERR("NAND: NAND Controller is not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->mutex);

	if (nand_init(dev)) {
		LOG_ERR("NAND: No NAND Flash found");
		return -1;
	}

	if (chip->nand.eccbits &&
	    nand_ecc_init(dev)) {
		return -1;
	}

	return 0;
}

#define NAND_DT_INST_BUS_CFG(inst)					\
	{								\
		.cs   = DT_PROP_BY_IDX(DT_INST_PARENT(inst), reg, 0),	\
		.addr = (void *)DT_REG_ADDR(DT_INST_PARENT(inst)),	\
	}

#define NAND_DT_INST_NC_INIT(inst) \
	DEVICE_DT_GET(DT_INST_PHANDLE(inst, nand_controller))

#define FLASH_NAND_DEFINE(inst)							\
	static const struct nand_config nand_config_##inst = {			\
		.ids     = nand_ids,						\
		.num_ids = ARRAY_SIZE(nand_ids),				\
		.ops     = nand_ops,						\
		.nc_dev  = NAND_DT_INST_NC_INIT(inst),				\
		.cs_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, cs_gpios, {0}),	\
	};									\
										\
	static struct nand_data nand_data_##inst = {				\
		.chip.bus = NAND_DT_INST_BUS_CFG(inst),				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, flash_nand_init, NULL,			\
			      &nand_data_##inst, &nand_config_##inst,		\
			      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,		\
			      &flash_nand_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_NAND_DEFINE)
