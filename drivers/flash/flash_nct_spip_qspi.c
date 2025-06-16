/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct_spip_qspi

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash/nct_flash_api_ex.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/dt-bindings/flash_controller/nct_qspi.h>
#include <soc.h>

#include "spi_nor.h"
#include "flash_nct_qspi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nct_spip_qspi, LOG_LEVEL_ERR);

#define NCT_SPIP_FIFO_DEPTH			8
#define NCT_SPIP_BURST_LENGTH(len)		(((len>>2) > NCT_SPIP_FIFO_DEPTH)?(NCT_SPIP_FIFO_DEPTH):(len>>2))

/* System configuration */
#define HAL_SFCG_INST() (struct scfg_reg *)(DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), scfg))

/* Driver convenience defines */
#define HAL_INSTANCE(dev) \
	((struct spip_reg *)((const struct nct_qspi_spip_config *)(dev)->config)->base)

/* Device config */
struct nct_qspi_spip_config {
	/* Flash controller base address */
	uintptr_t base;
	/* Clock configuration */
	uint32_t clk_cfg;
};

/* NCT SPI Normal functions */
static inline void qspi_nct_normal_cs_level(const struct device *dev, uint8_t sw_cs, bool level)
{
	struct spip_reg *const inst = HAL_INSTANCE(dev);

	/* Set chip select to high/low level */
	if (level) {
		inst->SPIP_SSCTL &= ~BIT(NCT_SSCTL_SS); /* CS high */
	} else {
		inst->SPIP_SSCTL |= BIT(NCT_SSCTL_SS); /* CS low */
	}
}

static inline void qspi_nct_io_mode(const struct device *dev, uint8_t dual_io_wr, uint8_t io_mode)
{
	struct spip_reg *const inst = HAL_INSTANCE(dev);
	uint32_t ctl_io_mode;

	if (io_mode == 4) {
		ctl_io_mode = BIT(NCT_CTL_QUADIOEN) | ((dual_io_wr)?(BIT(NCT_CTL_QDIODIR)):(0));
	}
	else if(io_mode == 2) {
		ctl_io_mode = BIT(NCT_CTL_DUALIOEN) | ((dual_io_wr)?(BIT(NCT_CTL_QDIODIR)):(0));
	}
	else {
		ctl_io_mode = 0;
	}

	/* Clear I/O mode setting */
    inst->SPIP_CTL &= ~(BIT(NCT_CTL_SPIEN) | BIT(NCT_CTL_DUALIOEN) | BIT(NCT_CTL_QDIODIR));

	/* Wait spip disable */
	while(inst->SPIP_STATUS & BIT(NCT_STATUS_SPIENSTS));

	/* Update new I/O mode setting */
	inst->SPIP_CTL |= (BIT(NCT_CTL_SPIEN) | ctl_io_mode);
}

static inline void qspi_nct_normal_write_bytes(const struct device *dev, uint8_t *data, uint32_t len)
{
	struct spip_reg *const inst = HAL_INSTANCE(dev);
	int i;

	if(len >= 4) {
		/* change to 32-bit width when transfer length > 4 byte */
		SET_FIELD(inst->SPIP_CTL, NCT_CTL_DWIDTH, 0);
	}

	while(len) {
		if(len < 4) {
			/* write data */
			while(len) {
				inst->SPIP_TX = *data++;
				len--;
			}

			/* wait data transfer finish */
			while(inst->SPIP_STATUS & BIT(NCT_STATUS_BUSY));
		}
		else {
			/* Burst mode using 32-bit and maximum fifo depth */
			for(i = 0; i < NCT_SPIP_BURST_LENGTH(len); i++) {
				/* write data with reverse endianness */
				inst->SPIP_TX = __REV(*((uint32_t*)data));
				data += 4;
			}

			len -= (NCT_SPIP_BURST_LENGTH(len) << 2);

			/* wait data ready */
			while(inst->SPIP_STATUS & BIT(NCT_STATUS_BUSY));

			/* change to 8-bit width */
			if(len < 4) {
				SET_FIELD(inst->SPIP_CTL, NCT_CTL_DWIDTH, 8);
			}
		}
	}
}

static inline void qspi_nct_normal_read_bytes(const struct device *dev, uint8_t *data, uint32_t len)
{
	struct spip_reg *const inst = HAL_INSTANCE(dev);
	int i;

	if(len >= 4) {
		/* change to 32-bit width when transfer length > 4 byte */
		SET_FIELD(inst->SPIP_CTL, NCT_CTL_DWIDTH, 0);
	}

	while(len) {
		if(len < 4) {
			/* push data */
			for(i = 0; i < len; i++) {
				inst->SPIP_TX = 0xFF;
			}

			/* wait data ready */
			while(inst->SPIP_STATUS & BIT(NCT_STATUS_BUSY));

			while(len) {
				*data++ = inst->SPIP_RX;
				len--;
			}
		}
		else {
			/* Burst mode using 32-bit and maximum fifo depth */
			for(i = 0; i < NCT_SPIP_BURST_LENGTH(len); i++) {
				/* push data */
				inst->SPIP_TX = 0xFFFFFFFF;
			}

			/* wait data ready */
			while(inst->SPIP_STATUS & BIT(NCT_STATUS_BUSY));

			for(i = 0; i < NCT_SPIP_BURST_LENGTH(len); i++) {
				/* read data with reverse endianness */
				*((uint32_t*)data) = __REV(inst->SPIP_RX);
				data += 4;
			}

			len -= (NCT_SPIP_BURST_LENGTH(len) << 2);

			/* change to 8-bit width */
			if(len < 4) {
				SET_FIELD(inst->SPIP_CTL, NCT_CTL_DWIDTH, 8);
			}
		}
	}
}

static inline void qspi_nct_spip_set_operation(const struct device *dev, uint32_t operation)
{
	if ((operation & NCT_EX_OP_EXT_FLASH_SPIP_WP) != 0) {
		nct_pinctrl_flash_write_protect_set(NCT_SPIP_FLASH_WP);
	}
}

/* NCT specific QSPI-SPIP controller functions */
static int qspi_nct_spip_normal_transceive(const struct device *dev, struct nct_transceive_cfg *cfg,
				     uint32_t flags)
{
	struct spip_reg *const inst = HAL_INSTANCE(dev);
	struct nct_qspi_data *const data = dev->data;
	uint32_t ctrl_value = 0;
	uint8_t adr_mode = 1, data_mode = 1;
	uint32_t dummy_len = 0;
	uint8_t dummy_dat[3] = {0xFF, 0xFF, 0xFF};
	uint32_t spip_ctl;

	/* Transaction is permitted? */
	if ((data->operation & NCT_EX_OP_LOCK_TRANSCEIVE) != 0) {
		return -EPERM;
	}

	/* Keep CTL setting */
	ctrl_value = inst->SPIP_CTL;

	/*
	 * Set CPOL and CPHA.
	 * The following is how to map nct spip control register to CPOL and CPHA
	 *   CPOL    CPHA  |  CLKPOL  TXNEG   RXNEG
	 *   --------------------------------------
	 *    0       0    |    0       1       0
	 *    0       1    |    0       0       1
	 *    1       0    |    1       0       1
	 *    1       1    |    1       1       0
	 */

	/* PSPI enable / 8-bit width / CPOL = 0 CPHA = 1 (default) / 3 clock suspend (default) */
	spip_ctl = 0;
	SET_FIELD(spip_ctl, NCT_CTL_DWIDTH, 8);
	SET_FIELD(spip_ctl, NCT_CTL_SUSPITV, 3);
	inst->SPIP_CTL = spip_ctl | BIT(NCT_CTL_TXNEG) | BIT(NCT_CTL_SPIEN);

	/* check address and data bit mode */
	if(cfg->opcode == SPI_NOR_CMD_4READ) {
		adr_mode = 4;
		data_mode = 4;
		dummy_len = 3;
	}
	else if (cfg->opcode == SPI_NOR_CMD_2READ) {
		adr_mode = 2;
		data_mode = 2;
		dummy_len = 1;
	}
	else if (cfg->opcode == SPI_NOR_CMD_DREAD) {
		adr_mode = 2;
		data_mode = 1;
		dummy_len = 1;
	}
	else {
		adr_mode = 1;
		data_mode = 1;
	}

	/* Deassert CS */
	qspi_nct_normal_cs_level(dev, data->sw_cs, false);

	/* op code */
	qspi_nct_io_mode(dev, 0, 1);
	qspi_nct_normal_write_bytes(dev, &cfg->opcode, 1);

	/* address */
	if ((flags & NCT_TRANSCEIVE_ACCESS_ADDR) != 0) {
		qspi_nct_io_mode(dev, 1, adr_mode);

		/* 3-byte or 4-byte address? */
		qspi_nct_normal_write_bytes(dev, &cfg->addr.u8[(data->cur_cfg->enter_4ba != 0) ? 0 : 1],
				(data->cur_cfg->enter_4ba != 0) ? 4 : 3);

		/* process dummy */
		if(dummy_len > 0) {
			qspi_nct_normal_write_bytes(dev, dummy_dat, (uint32_t)dummy_len);
		}
	}

	/* write data */
	if ((flags & NCT_TRANSCEIVE_ACCESS_WRITE) != 0) {
		qspi_nct_io_mode(dev, 0, 1);

		if (cfg->tx_buf == NULL) {
			return -EINVAL;
		}

		qspi_nct_normal_write_bytes(dev, cfg->tx_buf, cfg->tx_count);
	}

	/* Clear fifo */
	inst->SPIP_FIFOCTL |= (BIT(NCT_FIFOCTL_RXRST) | BIT(NCT_FIFOCTL_TXRST));
    while(inst->SPIP_STATUS & BIT(NCT_STATUS_TXRXRST)) {};

    /* read data */
	if ((flags & NCT_TRANSCEIVE_ACCESS_READ) != 0) {
		qspi_nct_io_mode(dev, 0, data_mode);

		if (cfg->rx_buf == NULL) {
			return -EINVAL;
		}

		qspi_nct_normal_read_bytes(dev, cfg->rx_buf, cfg->rx_count);
	}

	/* Deassert CS */
	qspi_nct_normal_cs_level(dev, data->sw_cs, true);

	/* Clear fifo */
	inst->SPIP_FIFOCTL |= (BIT(NCT_FIFOCTL_TXRST) | BIT(NCT_FIFOCTL_RXRST));
	while(inst->SPIP_STATUS & BIT(NCT_STATUS_TXRXRST)) {};

	/* Resume CTL setting */
	inst->SPIP_CTL = ctrl_value;

	return 0;
}

static void qspi_nct_spip_mutex_lock_configure(const struct device *dev,
					const struct nct_qspi_cfg *cfg,
					const uint32_t operation)
{
	struct nct_qspi_data *const data = dev->data;
	struct scfg_reg *inst_scfg = HAL_SFCG_INST();

	k_sem_take(&data->lock_sem, K_FOREVER);

	/* If the current device is different from previous one, configure it */
	if (data->cur_cfg != cfg) {
		data->cur_cfg = cfg;

		/* Apply pin-muxing and tri-state */
		pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	}

	/* force function pin select */
	inst_scfg->DEVALT0[0xc] &= ~BIT(4);

	/* Set QSPI bus operation */
	if (data->operation != operation) {
		qspi_nct_spip_set_operation(dev, operation);
		data->operation = operation;
	}
}

static void qspi_nct_spip_mutex_unlock(const struct device *dev)
{
	struct nct_qspi_data *const data = dev->data;

	k_sem_give(&data->lock_sem);
}

struct nct_qspi_ops nct_qspi_spip_ops = {
	.lock_configure = qspi_nct_spip_mutex_lock_configure,
        .unlock = qspi_nct_spip_mutex_unlock,
        .transceive = qspi_nct_spip_normal_transceive,
};

static int qspi_nct_spip_init(const struct device *dev)
{
	const struct nct_qspi_spip_config *const config = (void *)dev->config;
	struct nct_qspi_data *const data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("%s device not ready", clk_dev->name);
		return -ENODEV;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev,
			       (clock_control_subsys_t)config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on SPIP clock fail %d", ret);
		return ret;
	}

	/* initialize mutex for qspi controller */
	k_sem_init(&data->lock_sem, 1, 1);

	return 0;
}

#define NCT_SPI_SPIP_INIT(n)							\
static const struct nct_qspi_spip_config nct_qspi_spip_config_##n = {		\
	.base = DT_INST_REG_ADDR(n),						\
	.clk_cfg = DT_INST_PHA(n, clocks, clk_cfg),					\
};										\
static struct nct_qspi_data nct_qspi_data_##n = {				\
	.qspi_ops = &nct_qspi_spip_ops						\
};										\
DEVICE_DT_INST_DEFINE(n, qspi_nct_spip_init, NULL,				\
		      &nct_qspi_data_##n, &nct_qspi_spip_config_##n,	\
		      PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NCT_SPI_SPIP_INIT)
