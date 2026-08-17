/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_nfc_g1_flash

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/memc/mchp_smc_g1.h>
#include <zephyr/drivers/flash/mchp_nand_g1.h>
#include <zephyr/drivers/flash/mchp_pmecc_g1.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_mchp_nfc_g1, CONFIG_FLASH_LOG_LEVEL);

#define NFC_TIMEOUT_MS 1000

#define IRQ_ERROR_MASK	(HSMC_IER_NFCASE_Msk | \
			 HSMC_IER_AWB_Msk    | \
			 HSMC_IER_UNDEF_Msk  | \
			 HSMC_IER_DTOE_Msk)

#define IRQ_WAIT_MASK	(IRQ_ERROR_MASK     | \
			 HSMC_IER_CMDDONE_1 | \
			 HSMC_IER_XFRDONE_1 | \
			 HSMC_IER_RB_RISE_1)

struct nfc_config {
	smc_registers_t *regs;
	uint8_t *nfc_sram;
	uint32_t *nfc_io;
	const struct device *smc_dev;
	const struct device *ecc_dev;
#ifdef CONFIG_FLASH_MCHP_NFC_G1_DMA
	const struct device *dma_dev;
	uint32_t dma_chan;
#endif

	void (*irq_config_func)(const struct device *dev);
};

struct nfc_data {
	uint32_t wait;
	struct k_sem completion;
	struct k_mutex mutex;
#ifdef CONFIG_FLASH_MCHP_NFC_G1_DMA
	struct k_sem dma_sem;
	uint32_t dma_status;
#endif
};

union nfc_addr_cmd {
	uint32_t cmd;
	struct {
		uint32_t rsvd: 2;
		uint32_t cmd1: 8;
		uint32_t cmd2: 8;
		uint32_t vcmd2: 1;
		uint32_t acycle: 3;
		uint32_t csid: 3;
		uint32_t dataen: 1;
		uint32_t nfcwr: 1;
		uint32_t rsvd1: 5;
	} reg;
};

union nfc_data_addr {
	uint32_t addr;
	uint8_t cycle[4];
};

static inline uint32_t len(struct nand_info *nand, uint16_t flag)
{
	uint32_t len = 0;

	if (flag & F_PAGE) {
		len = nand->pagesize;

		if (flag & F_OOB) {
			len += nand->oobsize;
		}
	} else if (flag & F_OOB) {
		len = nand->oobsize;
	} else {
	}

	return len;
}

#ifdef CONFIG_FLASH_MCHP_NFC_G1_DMA
static void dma_callback(const struct device *dma_dev, void *user_data,
			 uint32_t channel, int status)
{
	struct nfc_data *data = user_data;

	data->dma_status = status;

	k_sem_give(&data->dma_sem);
}

static int dma_memcpy(const struct device *dev,
		      void *dest, const void *src, size_t n)
{
	const struct nfc_config *cfg = dev->config;
	struct nfc_data *data = dev->data;
	uint32_t dcache_line;
	size_t count;
	int ret;

	if (dest == NULL || src == NULL) {
		return -EINVAL;
	}

	dcache_line = sys_cache_data_line_size_get();
	if (dcache_line == 0U) {
		return -ENOTSUP;
	}

	if (n < dcache_line) {
		memcpy(dest, src, n);
		return 0;
	}

	if (!IS_ALIGNED((uintptr_t)(dest), dcache_line) ||
	    !IS_ALIGNED((uintptr_t)(src), dcache_line)) {
		return -EINVAL;
	}

	count = ROUND_DOWN(n, dcache_line);

	struct dma_block_config dma_block_cfg = {
		.block_size = count,
		.source_address = (uint32_t)src,
		.dest_address = (uint32_t)dest,
	};
	struct dma_config dma_cfg = {
		.channel_direction = MEMORY_TO_MEMORY,
		.complete_callback_en = true,
		.source_data_size = 1,
		.dest_data_size = 1,
		.source_burst_length = 16,
		.dest_burst_length = 16,
		.block_count = 1,
		.head_block = &dma_block_cfg,
		.user_data = (void *)data,
		.dma_callback = dma_callback,
	};

	sys_cache_data_invd_range(dest, count);
	sys_cache_data_flush_range((void *)(uintptr_t)src, count);

	ret = dma_config(cfg->dma_dev, cfg->dma_chan, &dma_cfg);
	if (ret != 0) {
		LOG_ERR("NFC: Failed to configure dma, ret=%d", ret);
		return -EIO;
	}

	ret = dma_start(cfg->dma_dev, cfg->dma_chan);
	if (ret != 0) {
		LOG_ERR("NFC: Failed to start dma, ret=%d", ret);
		return -EIO;
	}

	ret = k_sem_take(&data->dma_sem, K_MSEC(NFC_TIMEOUT_MS));
	if (ret) {
		LOG_ERR("NFC: Wait dma timeout!");
		return -EIO;
	}

	if (data->dma_status) {
		LOG_ERR("NFC: dma status 0x%x", data->dma_status);
		return -EIO;
	}

	sys_cache_data_invd_range((void *)dest, count);

	if (n > count) {
		memcpy((uint8_t *)dest + count,
		       (const uint8_t *)src + count, n - count);
	}

	return 0;
}
#endif

static void nfc_init(smc_registers_t *smc)
{
	smc->HSMC_CFG  = HSMC_CFG_DTOMUL_X1048576 |
			 HSMC_CFG_DTOCYC(0xf) |
			 HSMC_CFG_RBEDGE_1;
	smc->HSMC_CTRL = HSMC_CTRL_NFCEN_1;

	smc->HSMC_IDR = HSMC_IDR_Msk;
	(void)smc->HSMC_SR;
}

static void nfc_irq_en(smc_registers_t *smc)
{
	smc->HSMC_IER = IRQ_WAIT_MASK;
}

static void nfc_irq_dis(smc_registers_t *smc)
{
	smc->HSMC_IDR = IRQ_WAIT_MASK;
}

static int nfc_update_cfg(smc_registers_t *smc,
			  uint32_t pagesize, uint32_t sparesize, uint32_t spare)
{
	switch (pagesize) {
	case 512:
		smc->HSMC_CFG |= HSMC_CFG_PAGESIZE(HSMC_CFG_PAGESIZE_PS512_Val);
		break;
	case 1024:
		smc->HSMC_CFG |= HSMC_CFG_PAGESIZE(HSMC_CFG_PAGESIZE_PS1024_Val);
		break;
	case 2048:
		smc->HSMC_CFG |= HSMC_CFG_PAGESIZE(HSMC_CFG_PAGESIZE_PS2048_Val);
		break;
	case 4096:
		smc->HSMC_CFG |= HSMC_CFG_PAGESIZE(HSMC_CFG_PAGESIZE_PS4096_Val);
		break;
	case 8192:
		smc->HSMC_CFG |= HSMC_CFG_PAGESIZE(HSMC_CFG_PAGESIZE_PS8192_Val);
		break;
	default:
		LOG_ERR("NFC: Unsupported page size %d!\n", pagesize);
		return -EINVAL;
	}

	if ((sparesize & 0x3) || ((sparesize >> 2) > 0x7F)) {
		LOG_ERR("NFC: Unsupported spare size %d!\n", sparesize);
		return -EINVAL;
	}

	smc->HSMC_CFG |= HSMC_CFG_NFCSPARESIZE((sparesize >> 2) - 1);
	if (spare) {
		smc->HSMC_CFG |= HSMC_CFG_RSPARE_1 |
				 HSMC_CFG_WSPARE_1;
	} else {
		smc->HSMC_CFG &= ~(HSMC_CFG_RSPARE_1 |
				   HSMC_CFG_WSPARE_1);
	}

	return 0;
}

static int nfc_is_ready(smc_registers_t *smc)
{
	return !(smc->HSMC_SR & HSMC_SR_NFCBUSY_Msk);
}

static void nfc_data_in(void *addr, uint8_t *buf, uint32_t len)
{
	for (int i = 0; i < len; i++) {
		buf[i] = *(volatile uint8_t *)addr;
	}
}

static void nfc_data_out(void *addr, const uint8_t *buf, uint32_t len)
{
	for (int i = 0; i < len; i++) {
		*(volatile uint8_t *)addr = buf[i];
	}
}

static int nfc_isr(const struct device *dev)
{
	const struct nfc_config *cfg = dev->config;
	struct nfc_data *data = dev->data;
	smc_registers_t *smc = cfg->regs;
	uint32_t status;

	status = smc->HSMC_SR;
	LOG_DBG("NFC: %s S:%08x W:%08x", __func__, status, data->wait);

	if (data->wait) {
		data->wait ^= status & data->wait;

		if ((data->wait == 0) ||
		    (status & IRQ_ERROR_MASK)) {
			k_sem_give(&data->completion);
		}
	}

	return 0;
}

static int nfc_exec_op(const struct device *dev, struct nand_chip *chip,
		       const struct nand_op *op, uint8_t *addrs, uint8_t *buf)
{
	const struct nfc_config *cfg = dev->config;
	struct nfc_data *data = dev->data;
	smc_registers_t *smc = cfg->regs;
	struct nand_info *nand = &chip->nand;
	struct ecc_info *ecc = &chip->ecc;
	const struct bus_info *bus = &chip->bus;
	union nfc_addr_cmd n_cmd = {0};
	union nfc_data_addr n_addr = {0};
	uint8_t *oob_buf = (uint8_t *)cfg->nfc_sram + nand->pagesize;
	unsigned int lock_key;
	int use_ecc = 0;
	int ret, i;

	if (!nfc_is_ready(smc)) {
		LOG_ERR("NFC: NFC is not ready!");
		return -EBUSY;
	}

	n_cmd.reg.cmd1 = op->cmd;
	n_cmd.reg.csid = bus->cs;
	n_cmd.reg.acycle = op->acycle;
	if (op->flag & F_ADDR) {
		if (op->acycle == 0) {
			LOG_ERR("NFC: Address cycle is 0!\n");
			return -EINVAL;
		}

		if (op->acycle == 5) {
			smc->HSMC_ADDR = HSMC_ADDR_ADDR_CYCLE0(*addrs++);
		}

		for (i = 0; i < op->acycle; i++) {
			n_addr.cycle[i] = addrs[i];
		}
	} else {
		if (op->acycle) {
			n_addr.cycle[0] = op->addr;
		}
	}

	if (op->flag & F_CMD2) {
		n_cmd.reg.cmd2 = op->cmd2;
		n_cmd.reg.vcmd2 = 1;
	}

	if (op->flag & F_PAGE) {
		uint32_t spare_en = 0;

		n_cmd.reg.dataen = 1;

		if (op->flag & F_OUT) {
			n_cmd.reg.nfcwr = 1;

			if (!(op->flag & F_OOB)) {
				memset(oob_buf, 0xff, nand->oobsize);
			}

#ifdef CONFIG_FLASH_MCHP_NFC_G1_DMA
			/* if dam_memcpy() fails, fall back to memcpy() */
			if (dma_memcpy(dev, (void *)cfg->nfc_sram, buf, len(nand, op->flag))) {
				memcpy((void *)cfg->nfc_sram, buf, len(nand, op->flag));
			}
#else
			memcpy((void *)cfg->nfc_sram, buf, len(nand, op->flag));
#endif
		}

		if (op->flag & F_OUT) {
			if ((op->flag & F_OOB) && (op->flag & F_RAW)) {
				spare_en = 1;
			}
		} else {
			if ((op->flag & F_OOB) || !(op->flag & F_RAW)) {
				spare_en = 1;
			}
		}

		ret = nfc_update_cfg(smc, nand->pagesize, nand->oobsize, spare_en);
		if (ret) {
			return ret;
		}

		if (nand->eccbits && !(op->flag & F_RAW)) {
			use_ecc = 1;
			ecc_enable(cfg->ecc_dev, (op->flag & F_OUT));
		}
	}

	lock_key = irq_lock();

	data->wait = HSMC_SR_CMDDONE_Msk;
	if (op->flag & F_BUSY) {
		data->wait |= HSMC_SR_RB_RISE_Msk;
	}
	if (op->flag & F_PAGE) {
		data->wait |= HSMC_SR_XFRDONE_Msk;
	}

	(void)smc->HSMC_SR;
	nfc_irq_en(smc);

	irq_unlock(lock_key);

	LOG_DBG("NFC: %s send cmd %02x %02x, ADDR=0x%08x DATA=0x%08x CFG=0x%08x",
		__func__, op->cmd, op->cmd2, n_cmd.cmd, n_addr.addr, smc->HSMC_CFG);
	*(volatile uint32_t *)((uint32_t)cfg->nfc_io + n_cmd.cmd) = n_addr.addr;

	ret = k_sem_take(&data->completion, K_MSEC(NFC_TIMEOUT_MS));

	nfc_irq_dis(smc);

	if (ret || data->wait) {
		LOG_DBG("NFC: Wait nfc timeout, cmd %02x %02x wait=0x%08x!",
			op->cmd, op->cmd2, data->wait);

		return -ETIMEDOUT;
	}

	if (op->dcycle) {
		if (op->flag & F_OUT) {
			nfc_data_out(bus->addr, buf, op->dcycle);
		} else {
			nfc_data_in(bus->addr, buf, op->dcycle);
		}

		return 0;
	}

	if (op->flag & F_PAGE) {
		if (op->flag & F_OUT) {
			if (use_ecc && !(op->flag & F_RAW)) {
				ret = ecc_get_eccbytes(cfg->ecc_dev, &oob_buf[ecc->addr]);
				if (ret) {
					return ret;
				}
			}

			nfc_data_out(bus->addr, oob_buf, nand->oobsize);
		} else {
			if (use_ecc && !(op->flag & F_RAW)) {
				ret = ecc_process(cfg->ecc_dev, cfg->nfc_sram, oob_buf);
				if (ret < 0) {
					return ret;
				}
			}

#ifdef CONFIG_FLASH_MCHP_NFC_G1_DMA
			/* if dam_memcpy() fails, fall back to memcpy() */
			if (dma_memcpy(dev, buf, (void *)cfg->nfc_sram, len(nand, op->flag))) {
				memcpy(buf, (void *)cfg->nfc_sram, len(nand, op->flag));
			}
#else
			memcpy(buf, (void *)cfg->nfc_sram, len(nand, op->flag));
#endif

			return ret;
		}
	} else if (op->flag & F_OOB) {
		if (op->flag & F_OUT) {
			nfc_data_out(bus->addr, buf, nand->oobsize);
		} else {
			nfc_data_in(bus->addr, buf, nand->oobsize);
		}
	} else {
	}

	return 0;
}

static void nc_data_in(const struct device *dev, struct nand_chip *chip,
			uint8_t *buf, uint32_t len)
{
	struct nfc_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);

	nfc_data_in(chip->bus.addr, buf, len);

	k_mutex_unlock(&data->mutex);
}

static void nc_data_out(const struct device *dev, struct nand_chip *chip,
			 const uint8_t *buf, uint32_t len)
{
	struct nfc_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);

	nfc_data_out(chip->bus.addr, buf, len);

	k_mutex_unlock(&data->mutex);
}

static int nc_exec_op(const struct device *dev, struct nand_chip *chip,
		      const struct nand_op *op, uint8_t *addrs, uint8_t *buf)
{
	struct nfc_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = nfc_exec_op(dev, chip, op, addrs, buf);

	k_mutex_unlock(&data->mutex);

	return ret;
}

static int nc_ecc_init(const struct device *dev, struct nand_chip *chip)
{
	const struct nfc_config *cfg = dev->config;
	struct nfc_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = ecc_init_user(cfg->ecc_dev, chip);

	k_mutex_unlock(&data->mutex);

	return ret;
}

static struct nand_controller_ops nfc_nc_ops = {
	.ecc_init = nc_ecc_init,
	.exec_op  = nc_exec_op,
	.data_in  = nc_data_in,
	.data_out = nc_data_out,
};

static int flash_nfc_init(const struct device *dev)
{
	const struct nfc_config *cfg = dev->config;
	smc_registers_t *smc = cfg->regs;
	struct nfc_data *data = dev->data;

	if (!device_is_ready(cfg->smc_dev)) {
		LOG_ERR("NFC: SMC device is not ready");
		return -ENODEV;
	}

	if (!device_is_ready(cfg->ecc_dev)) {
		LOG_ERR("NFC: ECC device is not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->mutex);

	if (k_sem_init(&data->completion, 0, 1)) {
		return -EINVAL;
	}

#ifdef CONFIG_FLASH_MCHP_NFC_G1_DMA
	if (!device_is_ready(cfg->dma_dev)) {
		LOG_ERR("NFC: DMA device is not ready");
		return -ENODEV;
	}

	if (k_sem_init(&data->dma_sem, 0, 1)) {
		return -EINVAL;
	}
#endif

	nfc_init(smc);

	cfg->irq_config_func(dev);

	return 0;
}

#ifdef CONFIG_FLASH_MCHP_NFC_G1_DMA
#define NFC_DMA_INIT(inst)							\
	.dma_dev  = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(inst, dma)),	\
	.dma_chan = DT_INST_DMAS_CELL_BY_NAME(inst, dma, channel),
#else
#define NFC_DMA_INIT(inst)
#endif

#define FLASH_NFC_DEFINE(inst)								\
	static void nfc_irq_config_func_##inst(const struct device *dev);		\
											\
	static const struct nfc_config nfc_config_##inst = {				\
		.regs     = (smc_registers_t *)DT_INST_REG_ADDR(inst),			\
		.nfc_sram = (uint8_t *)DT_REG_ADDR(DT_INST_PHANDLE(inst, nfc_sram)),	\
		.nfc_io   = (uint32_t *)DT_REG_ADDR(DT_INST_PHANDLE(inst, nfc_io)),	\
		.smc_dev  = DEVICE_DT_GET(DT_INST_PARENT(inst)),			\
		.ecc_dev  = DEVICE_DT_GET(DT_INST_PHANDLE(inst, ecc_engine)),		\
		.irq_config_func = nfc_irq_config_func_##inst,				\
		NFC_DMA_INIT(inst)							\
	};										\
											\
	static struct nfc_data nfc_data_##inst = {0};					\
											\
	DEVICE_DT_INST_DEFINE(inst, flash_nfc_init, NULL,				\
			      &nfc_data_##inst, &nfc_config_##inst,			\
			      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,			\
			      &nfc_nc_ops);						\
											\
	static void nfc_irq_config_func_##inst(const struct device *dev)		\
	{										\
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(inst)),				\
			DT_IRQ(DT_INST_PARENT(inst), priority),				\
			nfc_isr, DEVICE_DT_INST_GET(inst), 0);				\
		irq_enable(DT_IRQN(DT_INST_PARENT(inst)));				\
	}

DT_INST_FOREACH_STATUS_OKAY(FLASH_NFC_DEFINE)
