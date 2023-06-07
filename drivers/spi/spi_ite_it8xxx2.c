/*
 * Copyright (c) 2022 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_spi

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <soc_common.h>
#include <soc_dt.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/dt-bindings/spi/it8xxx2-spi.h>

LOG_MODULE_REGISTER(spi_ite_it8xxx2, CONFIG_SPI_LOG_LEVEL);

/* SPICTRL1 bit 1: Interrupt Enable           */
#define INTREN		BIT(1)

/* SPICTRL3 bit 5: Auto or One-Shot mode      */
#define AUTOMODE	BIT(5)

/* SPICTRL5 bit 0: CMDQ Mode Enable           */
#define CMDQMODE	BIT(0)

/* SPICTRL5 bit [5:4]:                        */
/*          bit5: Channel 1 CMDQ mode         */
/*          bit4: Channel 0 CMDQ mode         */
/*          bit1: SPI CLK: clk_sspi           */
#define CH0SELCMDQ	BIT(4)
#define SCKFREQDIV1	BIT(1)

/* INTSTS bit 4:                              */
/*          0: Mask Disabled                  */
/*          1: Mask Enabled                   */
#define SPICMDQENDMASK	BIT(4)

/* INTSTS bit 0:                              */
/*          0: Write-one-cleared              */
/*          1: CMDQ transmission ends         */
#define SPICMDQEND	BIT(0)

/* The Command Queue Application              */
#define CMDQ_WR_DATA_LEN 16

/* The CMD1 field in the Command Queue Header */
#define CMDQ_DTR_MODE		BIT(7)
#define CMDQ_DUAL_MODE		BIT(6)
#define CMDQ_CS_ACTIVE		BIT(3)
#define CMDQ_AUTO_CHECK		BIT(2)
#define CMDQ_R_W		BIT(1)
#define CMDQ_CMD_END		BIT(0)

/* EC Doze/Idle Mode is not allowed during the Command Queue Transactions */

struct spi_cmdq_header {
	uint8_t spi_write_cmd_length;  /* [7:0] of the Write Command Length */
	uint8_t command1;              /* [7:0] of the Command1             */
	uint8_t data_length1;          /* [7:0] of the Data Length          */
	uint8_t data_length2;          /* [15:8] of the Data Length         */
	uint8_t data_addr1;            /* [7:0] of the Data Address         */
	uint8_t data_addr2;            /* [15:8] of the Data Address        */
	uint8_t check_bit_mask;
	uint8_t check_bit_value;
	uint8_t cmdq_wr_data[CMDQ_WR_DATA_LEN];
};

/* Device constant configuration parameters */
struct spi_it8xxx2_config {
	uintptr_t base_addr;
	uint8_t irq_no;
	const struct pinctrl_dev_config *pcfg;
	int spi_freq_setting;
	uint8_t cpol;
	uint8_t cpha;
	uint8_t inst_no;
};

/* Device run time data */
struct spi_it8xxx2_data {
	uint32_t spi_cmdq_header_addr;
	uint32_t spi_cmdq_read_buff_addr;
	struct spi_cmdq_header it8xxx2_spi_cmdq_header;

	struct k_mutex it8xxx2_mutex;
	struct k_sem it8xxx2_sem;
};

static void it8xxx2_spi_configure_sckfreq(const struct device *dev)
{
	const struct spi_it8xxx2_config *cfg = dev->config;
	struct spi_it8xxx2_regs *const spi_regs =
		(struct spi_it8xxx2_regs *)cfg->base_addr;
	uint8_t spi_freq_setting = (uint8_t)cfg->spi_freq_setting;

	spi_regs->SPICTRL1 &= 0xE3; /* Clear Bit 4, 3, 2 */
	spi_regs->SPICTRL1 |= (spi_freq_setting << 2);
}

static void it8xxx2_spi_configure_spimode(const struct device *dev)
{
	const struct spi_it8xxx2_config *cfg = dev->config;
	struct spi_it8xxx2_regs *const spi_regs =
		(struct spi_it8xxx2_regs *)cfg->base_addr;
	uint8_t spi_mode =
		(cfg->cpol << 6) |  (cfg->cpha << 5);

	spi_regs->SPICTRL1 &= 0x9F; /* Clear Bit 6 and 5 */
	spi_regs->SPICTRL1 |= spi_mode;
}

static void it8xxx2_spi_int_init(const struct device *dev)
{
	const struct spi_it8xxx2_config *cfg = dev->config;
	struct spi_it8xxx2_regs *const spi_regs =
		(struct spi_it8xxx2_regs *)cfg->base_addr;

	ite_intc_isr_clear(cfg->irq_no);
	irq_enable(cfg->irq_no);
	spi_regs->SPICTRL1 |= INTREN;
}

static void it8xxx2_spi_cmdq_init(const struct device *dev)
{
	const struct spi_it8xxx2_config *cfg = dev->config;
	struct spi_it8xxx2_regs *const spi_regs =
		(struct spi_it8xxx2_regs *)cfg->base_addr;

	spi_regs->SPICTRL3 &= ~AUTOMODE;
	spi_regs->INTSTS &= ~SPICMDQENDMASK;
	spi_regs->INTSTS |= SPICMDQEND;
}

static void spi_it8xxx2_cmdq_header_cleanup(const struct device *dev)
{
	struct spi_it8xxx2_data *data = dev->data;
	struct spi_cmdq_header *header = &(data->it8xxx2_spi_cmdq_header);
	int cnt;

	header->spi_write_cmd_length = 0;
	header->command1 = 0;

	header->data_length1 = 0;
	header->data_length2 = 0;

	header->data_addr1 = 0;
	header->data_addr2 = 0;

	header->check_bit_mask = 0;
	header->check_bit_value = 0;

	for (cnt = 0 ; cnt < CMDQ_WR_DATA_LEN ; cnt++) {
		header->cmdq_wr_data[cnt] = 0;
	}

}

static void spi_it8xxx2_cmdq_header_setup(const struct device *dev,
	uint8_t wr_cmd_length,
	uint8_t command1,
	uint16_t data_length,
	uint32_t data_buf_addr,
	uint8_t check_bit_mask,
	uint8_t check_bit_value,
	uint8_t *cmdq_txbuf)
{
	struct spi_it8xxx2_data *data = dev->data;
	struct spi_cmdq_header *header = &(data->it8xxx2_spi_cmdq_header);

	int cnt;

	header->spi_write_cmd_length = wr_cmd_length;
	header->command1 = command1;

	header->data_length1 = data_length & 0x000000FF;
	header->data_length2 = (data_length & 0x0000FF00) >> 8;

	header->data_addr1 = (uint8_t)(data_buf_addr & 0x000000FF);
	header->data_addr2 = (uint8_t)((data_buf_addr & 0x0000FF00) >> 8);

	header->check_bit_mask = check_bit_mask;
	header->check_bit_value = check_bit_value;

	if (cmdq_txbuf != NULL) {
		for (cnt = 0 ; cnt < wr_cmd_length ; cnt++) {
			header->cmdq_wr_data[cnt] = cmdq_txbuf[cnt];
		}
	} else {
		LOG_WRN("[kern][header] cmdq_txbuf is NULL, addr=0x%X\r\n",
			(uint32_t)cmdq_txbuf);
	}

}

static void spi_it8xxx2_cmdq_buffer_mapping(const struct device *dev,
							uint16_t cmdq_header_addr,
							uint16_t cmdq_read_buf_addr)
{
	const struct spi_it8xxx2_config *cfg = dev->config;
	struct spi_it8xxx2_regs *const spi_regs =
		(struct spi_it8xxx2_regs *)cfg->base_addr;

	/* Set CMDQ and Rec(Read Data Ram) Address */
	cmdq_read_buf_addr  &= 0xFFFF;
	cmdq_header_addr &= 0xFFFF;

	/* Set CMDQ and Rec(Read Data Ram) Address to I2C Reg. */
	spi_regs->CH0CMDADDRLB =
		(uint8_t)((cmdq_header_addr & 0x000000FF));
	spi_regs->CH0CMDADDRHB =
		(uint8_t)((cmdq_header_addr & 0x0000FF00) >> 8);
	spi_regs->CH0WRMEMADDRLB =
		(uint8_t)((cmdq_read_buf_addr & 0x000000FF));
	spi_regs->CH0WRMEMADDRHB =
		(uint8_t)((cmdq_read_buf_addr & 0x0000FF00) >> 8);
}

static int spi_it8xxx2_trans(const struct device *dev,
			    const struct spi_config *config,
			    const struct spi_buf *ptx,
			    const struct spi_buf *prx)
{
	const struct spi_it8xxx2_config *cfg = dev->config;
	struct spi_it8xxx2_data *data = dev->data;
	struct spi_it8xxx2_regs *const spi_regs =
		(struct spi_it8xxx2_regs *)cfg->base_addr;

	uint8_t *txbfptr = ptx->buf;
	size_t txbflen = ptx->len;

	uint8_t *rxbfptr = prx->buf;
	size_t rxbflen = prx->len;

	int err = 0;

	/* Variabels for the CMDQ Header */
	uint8_t wr_cmd_length = 0;
	uint8_t command1 = CMDQ_CMD_END;
	uint16_t data_length = 0;
	uint32_t data_buf_addr = 0;
	uint8_t check_bit_mask = 0;
	uint8_t check_bit_value = 0;

	uint8_t *cmdq_header_txbuf = txbfptr;

	it8xxx2_spi_int_init(dev);
	spi_it8xxx2_cmdq_header_cleanup(dev);

	if ((!txbfptr) && (!rxbfptr) && (!txbflen) && (!rxbflen)) {
		LOG_WRN("[kern][trans] All Buffer Pointers Are NULL.\r\n");
		return -EIO;
	}

	data->spi_cmdq_header_addr = (uint32_t)(&(data->it8xxx2_spi_cmdq_header));
	data->spi_cmdq_read_buff_addr = (uint32_t)prx->buf;

	/* When the RX is not required, then just consider the TX requirements */
	if ((rxbfptr != NULL) && (rxbflen != 0)) {
		wr_cmd_length = txbflen;
		command1 = CMDQ_R_W | CMDQ_CMD_END;
		data_length = rxbflen;
		data_buf_addr = 0;
		check_bit_mask = 0;
		check_bit_value = 0;
		cmdq_header_txbuf = txbfptr;
	} else {
	/* It means that the RX doesn't exist, so we just consider the tx stuffs*/
		if (txbflen <= CMDQ_WR_DATA_LEN) {
		/* To compose the header with the appending mode with write direction*/
			wr_cmd_length = txbflen;
			command1 = CMDQ_CMD_END;
			data_length = 0;
			data_buf_addr = 0;
			check_bit_mask = 0;
			check_bit_value = 0;
			cmdq_header_txbuf = txbfptr;
		} else {
		/* To compose the header with the addressing mode with write direction*/
			wr_cmd_length = CMDQ_WR_DATA_LEN;
			command1 = CMDQ_CMD_END;
			data_length = txbflen-CMDQ_WR_DATA_LEN;
			data_buf_addr = (uint32_t)(&(txbfptr[CMDQ_WR_DATA_LEN]));
			check_bit_mask = 0;
			check_bit_value = 0;
			cmdq_header_txbuf = txbfptr;
		}

	}

	/* Assign Addresses for Header and the Read Buffer */
	spi_it8xxx2_cmdq_header_setup(dev,
		wr_cmd_length,
		command1,
		data_length,
		(uint32_t)data_buf_addr,
		check_bit_mask,
		check_bit_value,
		cmdq_header_txbuf);

	/* Assign Addresses for Header and the Read Buffer */
	spi_it8xxx2_cmdq_buffer_mapping(dev,
					data->spi_cmdq_header_addr,
					data->spi_cmdq_read_buff_addr);

	chip_block_idle();
	spi_regs->SPICTRL5 |= CH0SELCMDQ;
	spi_regs->SPICTRL5 |= CMDQMODE;

	int ret = k_sem_take(&data->it8xxx2_sem, K_FOREVER);

	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		err = -ETIMEDOUT;
	}

	spi_regs->INTSTS |= SPICMDQEND;
	return err;

}

static int it8xxx2_transceive(const struct device *dev,
			    const struct spi_config *config,
			    const struct spi_buf_set *tx_bufs,
			    const struct spi_buf_set *rx_bufs)
{
	struct spi_it8xxx2_data *data = dev->data;

	size_t nbrx = 0;
	size_t nbtx = 0;

	const struct spi_buf *ptx = tx_bufs->buffers;
	const struct spi_buf *prx = rx_bufs->buffers;

	int err = 0;
	int ret = 0;

	ret = k_mutex_lock(&data->it8xxx2_mutex, K_FOREVER);

	if (tx_bufs != NULL) {
		nbtx = tx_bufs->count;
	}

	while (nbtx--) {
		err = spi_it8xxx2_trans(dev, config, ptx, prx);
		if (err != 0) {
			LOG_ERR("[kern][spi_it8xxx2_trans] Returned value is not 0\r\n");
		}
	}

	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		err = ret;
	}

	if (rx_bufs != NULL) {
		nbrx = rx_bufs->count;
	}

	while (nbrx--) {
		err = spi_it8xxx2_trans(dev, config, ptx, prx);
		if (err != 0) {
			LOG_ERR("[kern][spi_it8xxx2_trans] Returned value is not 0\r\n");
		}
	}

	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		err = ret;
	}

	k_mutex_unlock(&data->it8xxx2_mutex);

	return err;

}

static int it8xxx2_transceive_sync(const struct device *dev,
				 const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	return it8xxx2_transceive(dev, config, tx_bufs, rx_bufs);
}

#ifdef CONFIG_SPI_ASYNC
static int it8xxx2_transceive_async(const struct device *dev,
				  const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs,
				  struct k_poll_signal *async)
{
	return it8xxx2_transceive(dev, config, tx_bufs, rx_bufs);
}
#endif

static int it8xxx2_release(const struct device *dev,
			 const struct spi_config *config)
{
	struct spi_it8xxx2_data *data = dev->data;

	k_sem_give(&data->it8xxx2_sem);
	return 0;
}

static void spi_it8xxx2_isr(const struct device *dev)
{
	const struct spi_it8xxx2_config *cfg = dev->config;
	struct spi_it8xxx2_data *data = dev->data;
	struct spi_it8xxx2_regs *const spi_regs =
		(struct spi_it8xxx2_regs *)cfg->base_addr;

	irq_disable(cfg->irq_no);

	spi_regs->INTSTS |= SPICMDQEND;
	ite_intc_isr_clear(cfg->irq_no);
	spi_regs->SPICTRL5 &= ~(CH0SELCMDQ);
	k_sem_give(&data->it8xxx2_sem);
	chip_permit_idle();
}

static int spi_it8xxx2_init(const struct device *dev)
{
	const struct spi_it8xxx2_config *cfg = dev->config;
	struct spi_it8xxx2_data *data = dev->data;

	int status;

	status = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	if (status < 0) {
		LOG_ERR("Failed to configure SPI pins\r\n");
		return status;
	}

	/* Initialize Mutex */
	k_mutex_init(&data->it8xxx2_mutex);
	/* Initialize Semaphore */
	k_sem_init(&data->it8xxx2_sem, 0, 1);
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
	spi_it8xxx2_isr, DEVICE_DT_INST_GET(0), 0);

	it8xxx2_spi_cmdq_init(dev);
	it8xxx2_spi_configure_sckfreq(dev);
	it8xxx2_spi_configure_spimode(dev);
	it8xxx2_spi_int_init(dev);

	return 0;
}

static const struct spi_driver_api spi_it8xxx2_driver_api = {
	.transceive = it8xxx2_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = it8xxx2_transceive_async,
#endif
	.release = it8xxx2_release,
};

#define SPI_ITE_IT8XXX2_INIT(inst)					\
	PINCTRL_DT_INST_DEFINE(inst);					\
	static const struct spi_it8xxx2_config spi_it8xxx2_cfg##inst = {\
		.base_addr = DT_INST_REG_ADDR(inst),			\
		.irq_no = DT_INST_IRQN(inst),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),		\
		.spi_freq_setting = DT_INST_PROP(inst, sckfreq),	\
		.cpol = DT_INST_PROP(inst, spi_cpol),			\
		.cpha = DT_INST_PROP(inst, spi_cpha),			\
		.inst_no = inst,					\
	};								\
									\
	static struct spi_it8xxx2_data spi_it8xxx2_dat##inst;		\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
		    &spi_it8xxx2_init,					\
		    NULL,						\
		    &spi_it8xxx2_dat##inst,				\
		    &spi_it8xxx2_cfg##inst,				\
		    POST_KERNEL,					\
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
		    &spi_it8xxx2_driver_api);				\

DT_INST_FOREACH_STATUS_OKAY(SPI_ITE_IT8XXX2_INIT)
