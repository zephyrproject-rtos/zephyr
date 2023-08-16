/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_espi

#include <zephyr/kernel.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include "espi_utils.h"

/* Minimum delay before acknowledging a virtual wire */
#define ESPI_XEC_VWIRE_ACK_DELAY    10ul

/* Maximum timeout to transmit a virtual wire packet.
 * 10 ms expressed in multiples of 100us
 */
#define ESPI_XEC_VWIRE_SEND_TIMEOUT 100ul

#define VW_MAX_GIRQS                2ul

/* 200ms */
#define MAX_OOB_TIMEOUT             200ul
/* 1s */
#define MAX_FLASH_TIMEOUT           1000ul

/* While issuing flash erase command, it should be ensured that the transfer
 * length specified is non-zero.
 */
#define ESPI_FLASH_ERASE_DUMMY	    0x01ul

/* OOB maximum address configuration */
#define ESPI_XEC_OOB_ADDR_MSW       0x1FFFul
#define ESPI_XEC_OOB_ADDR_LSW       0xFFFFul

/* OOB Rx length */
#define ESPI_XEC_OOB_RX_LEN         0x7F00ul

/* BARs as defined in LPC spec chapter 11 */
#define ESPI_XEC_KBC_BAR_ADDRESS    0x00600000
#define ESPI_XEC_UART0_BAR_ADDRESS  0x03F80000
#define ESPI_XEC_MBOX_BAR_ADDRESS   0x03600000
#define ESPI_XEC_PORT80_BAR_ADDRESS 0x00800000
#define ESPI_XEC_PORT81_BAR_ADDRESS 0x00810000

/* Espi peripheral has 3 uart ports */
#define ESPI_PERIPHERAL_UART_PORT0  0
#define ESPI_PERIPHERAL_UART_PORT1  1
#define ESPI_PERIPHERAL_UART_PORT2  2

#define UART_DEFAULT_IRQ_POS	    2u
#define UART_DEFAULT_IRQ	    BIT(UART_DEFAULT_IRQ_POS)

/* VM index 0x50 for OCB */
#define ESPI_OCB_VW_INDEX		0x50u

LOG_MODULE_REGISTER(espi, CONFIG_ESPI_LOG_LEVEL);

struct espi_isr {
	uint32_t girq_bit;
	void (*the_isr)(const struct device *dev);
};

struct espi_xec_config {
	uint32_t base_addr;
	uint8_t bus_girq_id;
	uint8_t vw_girq_ids[VW_MAX_GIRQS];
	uint8_t pc_girq_id;
	const struct pinctrl_dev_config *pcfg;
};

struct espi_xec_data {
	sys_slist_t callbacks;
	struct k_sem tx_lock;
	struct k_sem rx_lock;
	struct k_sem flash_lock;
};

struct xec_signal {
	uint8_t xec_reg_idx;
	uint8_t bit;
	uint8_t dir;
};

enum mchp_msvw_regs {
	MCHP_MSVW00,
	MCHP_MSVW01,
	MCHP_MSVW02,
	MCHP_MSVW03,
	MCHP_MSVW04,
	MCHP_MSVW05,
	MCHP_MSVW06,
	MCHP_MSVW07,
	MCHP_MSVW08,
};

enum mchp_smvw_regs {
	MCHP_SMVW00,
	MCHP_SMVW01,
	MCHP_SMVW02,
	MCHP_SMVW03,
	MCHP_SMVW04,
	MCHP_SMVW05,
	MCHP_SMVW06,
	MCHP_SMVW07,
	MCHP_SMVW08,
};

/* Microchip canonical virtual wire mapping
 * ------------------------------------------------------------------------|
 * VW Idx | VW reg | SRC_ID3      | SRC_ID2      | SRC_ID1   | SRC_ID0     |
 * ------------------------------------------------------------------------|
 * System Event Virtual Wires
 * ------------------------------------------------------------------------|
 *  2h    | MSVW00 | res          | SLP_S5#      | SLP_S4#   | SLP_S3#     |
 *  3h    | MSVW01 | res          | OOB_RST_WARN | PLTRST#   | SUS_STAT#   |
 *  4h    | SMVW00 | PME#         | WAKE#        | res       | OOB_RST_ACK |
 *  5h    | SMVW01 | SLV_BOOT_STS | ERR_NONFATAL | ERR_FATAL | SLV_BT_DONE |
 *  6h    | SMVW02 | HOST_RST_ACK | RCIN#        | SMI#      | SCI#        |
 *  7h    | MSVW02 | res          | res          | res       | HOS_RST_WARN|
 * ------------------------------------------------------------------------|
 * Platform specific virtual wires
 * ------------------------------------------------------------------------|
 *  40h   | SMVW03 | res          | res          | DNX_ACK   | SUS_ACK#    |
 *  41h   | MSVW03 | SLP_A#       | res          | SUS_PDNACK| SUS_WARN#   |
 *  42h   | MSVW04 | res          | res          | SLP_WLAN# | SLP_LAN#    |
 *  43h   | MSVW05 | generic      | generic      | generic   | generic     |
 *  44h   | MSVW06 | generic      | generic      | generic   | generic     |
 *  45h   | SMVW04 | generic      | generic      | generic   | generic     |
 *  46h   | SMVW05 | generic      | generic      | generic   | generic     |
 *  47h   | MSVW07 | res          | res          | res       | HOST_C10    |
 *  4Ah   | MSVW08 | res          | res          | DNX_WARN  | res         |
 *  50h   | SMVW06 | ESPI_OCB_3   | ESPI_OCB_2   | ESPI_OCB_1| ESPI_OCB_0  |
 */

static const struct xec_signal vw_tbl[] = {
	/* MSVW00 */
	[ESPI_VWIRE_SIGNAL_SLP_S3]        = {MCHP_MSVW00, ESPI_VWIRE_SRC_ID0,
					     ESPI_MASTER_TO_SLAVE},
	[ESPI_VWIRE_SIGNAL_SLP_S4]        = {MCHP_MSVW00, ESPI_VWIRE_SRC_ID1,
					     ESPI_MASTER_TO_SLAVE},
	[ESPI_VWIRE_SIGNAL_SLP_S5]        = {MCHP_MSVW00, ESPI_VWIRE_SRC_ID2,
					     ESPI_MASTER_TO_SLAVE},
	/* MSVW01 */
	[ESPI_VWIRE_SIGNAL_SUS_STAT]      = {MCHP_MSVW01, ESPI_VWIRE_SRC_ID0,
					     ESPI_MASTER_TO_SLAVE},
	[ESPI_VWIRE_SIGNAL_PLTRST]        = {MCHP_MSVW01, ESPI_VWIRE_SRC_ID1,
					     ESPI_MASTER_TO_SLAVE},
	[ESPI_VWIRE_SIGNAL_OOB_RST_WARN]  = {MCHP_MSVW01, ESPI_VWIRE_SRC_ID2,
					     ESPI_MASTER_TO_SLAVE},
	/* SMVW00 */
	[ESPI_VWIRE_SIGNAL_OOB_RST_ACK]   = {MCHP_SMVW00, ESPI_VWIRE_SRC_ID0,
					     ESPI_SLAVE_TO_MASTER},
	[ESPI_VWIRE_SIGNAL_WAKE]          = {MCHP_SMVW00, ESPI_VWIRE_SRC_ID2,
					     ESPI_SLAVE_TO_MASTER},
	[ESPI_VWIRE_SIGNAL_PME]           = {MCHP_SMVW00, ESPI_VWIRE_SRC_ID3,
					     ESPI_SLAVE_TO_MASTER},
	/* SMVW01 */
	[ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE] = {MCHP_SMVW01, ESPI_VWIRE_SRC_ID0,
					     ESPI_SLAVE_TO_MASTER},
	[ESPI_VWIRE_SIGNAL_ERR_FATAL]     = {MCHP_SMVW01, ESPI_VWIRE_SRC_ID1,
					     ESPI_SLAVE_TO_MASTER},
	[ESPI_VWIRE_SIGNAL_ERR_NON_FATAL] = {MCHP_SMVW01, ESPI_VWIRE_SRC_ID2,
					     ESPI_SLAVE_TO_MASTER},
	[ESPI_VWIRE_SIGNAL_SLV_BOOT_STS]  = {MCHP_SMVW01, ESPI_VWIRE_SRC_ID3,
					     ESPI_SLAVE_TO_MASTER},
	/* SMVW02 */
	[ESPI_VWIRE_SIGNAL_SCI]           = {MCHP_SMVW02, ESPI_VWIRE_SRC_ID0,
					     ESPI_SLAVE_TO_MASTER},
	[ESPI_VWIRE_SIGNAL_SMI]           = {MCHP_SMVW02, ESPI_VWIRE_SRC_ID1,
					     ESPI_SLAVE_TO_MASTER},
	[ESPI_VWIRE_SIGNAL_RST_CPU_INIT]  = {MCHP_SMVW02, ESPI_VWIRE_SRC_ID2,
					     ESPI_SLAVE_TO_MASTER},
	[ESPI_VWIRE_SIGNAL_HOST_RST_ACK]  = {MCHP_SMVW02, ESPI_VWIRE_SRC_ID3,
					     ESPI_SLAVE_TO_MASTER},
	/* MSVW02 */
	[ESPI_VWIRE_SIGNAL_HOST_RST_WARN] = {MCHP_MSVW02, ESPI_VWIRE_SRC_ID0,
					     ESPI_MASTER_TO_SLAVE},
	/* SMVW03 */
	[ESPI_VWIRE_SIGNAL_SUS_ACK]       = {MCHP_SMVW03, ESPI_VWIRE_SRC_ID0,
					     ESPI_SLAVE_TO_MASTER},
	[ESPI_VWIRE_SIGNAL_DNX_ACK]       = {MCHP_SMVW03, ESPI_VWIRE_SRC_ID1,
					     ESPI_SLAVE_TO_MASTER},
	/* MSVW03 */
	[ESPI_VWIRE_SIGNAL_SUS_WARN]      = {MCHP_MSVW03, ESPI_VWIRE_SRC_ID0,
					     ESPI_MASTER_TO_SLAVE},
	[ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK] = {MCHP_MSVW03, ESPI_VWIRE_SRC_ID1,
					     ESPI_MASTER_TO_SLAVE},
	[ESPI_VWIRE_SIGNAL_SLP_A]         = {MCHP_MSVW03, ESPI_VWIRE_SRC_ID3,
					     ESPI_MASTER_TO_SLAVE},
	/* MSVW04 */
	[ESPI_VWIRE_SIGNAL_SLP_LAN]       = {MCHP_MSVW04, ESPI_VWIRE_SRC_ID0,
					     ESPI_MASTER_TO_SLAVE},
	[ESPI_VWIRE_SIGNAL_SLP_WLAN]      = {MCHP_MSVW04, ESPI_VWIRE_SRC_ID1,
					     ESPI_MASTER_TO_SLAVE},
	/* MSVW07 */
	[ESPI_VWIRE_SIGNAL_HOST_C10]      = {MCHP_MSVW07, ESPI_VWIRE_SRC_ID0,
					     ESPI_MASTER_TO_SLAVE},
	/* MSVW08 */
	[ESPI_VWIRE_SIGNAL_DNX_WARN]      = {MCHP_MSVW08, ESPI_VWIRE_SRC_ID1,
					     ESPI_MASTER_TO_SLAVE},
	/* SMVW06 */
	[ESPI_VWIRE_SIGNAL_OCB_0]       = {MCHP_SMVW06, ESPI_VWIRE_SRC_ID0,
					     ESPI_SLAVE_TO_MASTER},
	[ESPI_VWIRE_SIGNAL_OCB_1]       = {MCHP_SMVW06, ESPI_VWIRE_SRC_ID1,
					     ESPI_SLAVE_TO_MASTER},
	[ESPI_VWIRE_SIGNAL_OCB_2]       = {MCHP_SMVW06, ESPI_VWIRE_SRC_ID2,
					     ESPI_SLAVE_TO_MASTER},
	[ESPI_VWIRE_SIGNAL_OCB_3]       = {MCHP_SMVW06, ESPI_VWIRE_SRC_ID3,
					     ESPI_SLAVE_TO_MASTER},
};

/* Buffer size are expressed in bytes */
#ifdef CONFIG_ESPI_OOB_CHANNEL
static uint32_t target_rx_mem[CONFIG_ESPI_OOB_BUFFER_SIZE >> 2];
static uint32_t target_tx_mem[CONFIG_ESPI_OOB_BUFFER_SIZE >> 2];
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
static uint32_t target_mem[CONFIG_ESPI_FLASH_BUFFER_SIZE >> 2];
#endif

static int espi_xec_configure(const struct device *dev, struct espi_cfg *cfg)
{
	uint8_t iomode = 0;
	uint8_t cap0 = ESPI_CAP_REGS->GLB_CAP0;
	uint8_t cap1 = ESPI_CAP_REGS->GLB_CAP1;
	uint8_t cur_iomode = (cap1 & MCHP_ESPI_GBL_CAP1_IO_MODE_MASK) >>
			   MCHP_ESPI_GBL_CAP1_IO_MODE_POS;

	/* Set frequency */
	cap1 &= ~MCHP_ESPI_GBL_CAP1_MAX_FREQ_MASK;

	switch (cfg->max_freq) {
	case 20:
		cap1 |= MCHP_ESPI_GBL_CAP1_MAX_FREQ_20M;
		break;
	case 25:
		cap1 |= MCHP_ESPI_GBL_CAP1_MAX_FREQ_25M;
		break;
	case 33:
		cap1 |= MCHP_ESPI_GBL_CAP1_MAX_FREQ_33M;
		break;
	case 50:
		cap1 |= MCHP_ESPI_GBL_CAP1_MAX_FREQ_50M;
		break;
	case 66:
		cap1 |= MCHP_ESPI_GBL_CAP1_MAX_FREQ_66M;
		break;
	default:
		return -EINVAL;
	}

	/* Set IO mode */
	iomode = (cfg->io_caps >> 1);
	if (iomode > 3) {
		return -EINVAL;
	}

	if (iomode != cur_iomode) {
		cap1 &= ~(MCHP_ESPI_GBL_CAP1_IO_MODE_MASK0 <<
			MCHP_ESPI_GBL_CAP1_IO_MODE_POS);
		cap1 |= (iomode << MCHP_ESPI_GBL_CAP1_IO_MODE_POS);
	}

	/* Validate and translate eSPI API channels to MEC capabilities */
	cap0 &= ~MCHP_ESPI_GBL_CAP0_MASK;
	if (cfg->channel_caps & ESPI_CHANNEL_PERIPHERAL) {
		if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_CHANNEL)) {
			cap0 |= MCHP_ESPI_GBL_CAP0_PC_SUPP;
		} else {
			return -EINVAL;
		}
	}

	if (cfg->channel_caps & ESPI_CHANNEL_VWIRE) {
		if (IS_ENABLED(CONFIG_ESPI_VWIRE_CHANNEL)) {
			cap0 |= MCHP_ESPI_GBL_CAP0_VW_SUPP;
		} else {
			return -EINVAL;
		}
	}

	if (cfg->channel_caps & ESPI_CHANNEL_OOB) {
		if (IS_ENABLED(CONFIG_ESPI_OOB_CHANNEL)) {
			cap0 |= MCHP_ESPI_GBL_CAP0_OOB_SUPP;
		} else {
			return -EINVAL;
		}
	}

	if (cfg->channel_caps & ESPI_CHANNEL_FLASH) {
		if (IS_ENABLED(CONFIG_ESPI_FLASH_CHANNEL)) {
			cap0 |= MCHP_ESPI_GBL_CAP0_FC_SUPP;
		} else {
			LOG_ERR("Flash channel not supported");
			return -EINVAL;
		}
	}

	ESPI_CAP_REGS->GLB_CAP0 = cap0;
	ESPI_CAP_REGS->GLB_CAP1 = cap1;

	/* Activate the eSPI block *.
	 * Need to guarantee that this register is configured before RSMRST#
	 * de-assertion and after pinmux
	 */
	ESPI_EIO_BAR_REGS->IO_ACTV = 1;
	LOG_DBG("eSPI block activated successfully");

	return 0;
}

static bool espi_xec_channel_ready(const struct device *dev,
				   enum espi_channel ch)
{
	bool sts;

	switch (ch) {
	case ESPI_CHANNEL_PERIPHERAL:
		sts = ESPI_CAP_REGS->PC_RDY & MCHP_ESPI_PC_READY;
		break;
	case ESPI_CHANNEL_VWIRE:
		sts = ESPI_CAP_REGS->VW_RDY & MCHP_ESPI_VW_READY;
		break;
	case ESPI_CHANNEL_OOB:
		sts = ESPI_CAP_REGS->OOB_RDY & MCHP_ESPI_OOB_READY;
		break;
	case ESPI_CHANNEL_FLASH:
		sts = ESPI_CAP_REGS->FC_RDY & MCHP_ESPI_FC_READY;
		break;
	default:
		sts = false;
		break;
	}

	return sts;
}

static int espi_xec_read_lpc_request(const struct device *dev,
				     enum lpc_peripheral_opcode op,
				     uint32_t  *data)
{
	ARG_UNUSED(dev);

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		/* Make sure kbc 8042 is on */
		if (!(KBC_REGS->KBC_CTRL & MCHP_KBC_CTRL_OBFEN)) {
			return -ENOTSUP;
		}

		switch (op) {
		case E8042_OBF_HAS_CHAR:
			/* EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			*data = KBC_REGS->EC_KBC_STS & MCHP_KBC_STS_OBF ? 1 : 0;
			break;
		case E8042_IBF_HAS_CHAR:
			*data = KBC_REGS->EC_KBC_STS & MCHP_KBC_STS_IBF ? 1 : 0;
			break;
		case E8042_READ_KB_STS:
			*data = KBC_REGS->EC_KBC_STS;
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int espi_xec_write_lpc_request(const struct device *dev,
				      enum lpc_peripheral_opcode op,
				      uint32_t *data)
{
	struct espi_xec_config *config =
		(struct espi_xec_config *) (dev->config);

	volatile uint32_t __attribute__((unused)) dummy;

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		/* Make sure kbc 8042 is on */
		if (!(KBC_REGS->KBC_CTRL & MCHP_KBC_CTRL_OBFEN)) {
			return -ENOTSUP;
		}

		switch (op) {
		case E8042_WRITE_KB_CHAR:
			KBC_REGS->EC_DATA = *data & 0xff;
			break;
		case E8042_WRITE_MB_CHAR:
			KBC_REGS->EC_AUX_DATA = *data & 0xff;
			break;
		case E8042_RESUME_IRQ:
			MCHP_GIRQ_SRC(config->pc_girq_id) = MCHP_KBC_IBF_GIRQ;
			MCHP_GIRQ_ENSET(config->pc_girq_id) = MCHP_KBC_IBF_GIRQ;
			break;
		case E8042_PAUSE_IRQ:
			MCHP_GIRQ_ENCLR(config->pc_girq_id) = MCHP_KBC_IBF_GIRQ;
			break;
		case E8042_CLEAR_OBF:
			dummy = KBC_REGS->HOST_AUX_DATA;
			break;
		case E8042_SET_FLAG:
			/* FW shouldn't modify these flags directly */
			*data &= ~(MCHP_KBC_STS_OBF | MCHP_KBC_STS_IBF |
				   MCHP_KBC_STS_AUXOBF);
			KBC_REGS->EC_KBC_STS |= *data;
			break;
		case E8042_CLEAR_FLAG:
			/* FW shouldn't modify these flags directly */
			*data |= (MCHP_KBC_STS_OBF | MCHP_KBC_STS_IBF |
				  MCHP_KBC_STS_AUXOBF);
			KBC_REGS->EC_KBC_STS &= ~(*data);
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -ENOTSUP;

	}

	return 0;
}

static int espi_xec_send_vwire(const struct device *dev,
			       enum espi_vwire_signal signal, uint8_t level)
{
	struct xec_signal signal_info = vw_tbl[signal];
	uint8_t xec_id = signal_info.xec_reg_idx;
	uint8_t src_id = signal_info.bit;

	if ((src_id >= ESPI_VWIRE_SRC_ID_MAX) ||
	    (xec_id >= ESPI_MSVW_IDX_MAX)) {
		return -EINVAL;
	}

	if (signal_info.dir == ESPI_MASTER_TO_SLAVE) {
		ESPI_MSVW_REG *reg = &(ESPI_M2S_VW_REGS->MSVW00) + xec_id;
		uint8_t *p8 = (uint8_t *)&reg->SRC;

		*(p8 + (uintptr_t) src_id) = level;
	}

	if (signal_info.dir == ESPI_SLAVE_TO_MASTER) {
		ESPI_SMVW_REG *reg = &(ESPI_S2M_VW_REGS->SMVW00) + xec_id;
		uint8_t *p8 = (uint8_t *)&reg->SRC;

		*(p8 + (uintptr_t) src_id) = level;

		/* Ensure eSPI virtual wire packet is transmitted
		 * There is no interrupt, so need to poll register
		 */
		uint8_t rd_cnt = ESPI_XEC_VWIRE_SEND_TIMEOUT;

		while (reg->SRC_CHG && rd_cnt--) {
			k_busy_wait(100);
		}
	}

	return 0;
}

static int espi_xec_receive_vwire(const struct device *dev,
				  enum espi_vwire_signal signal, uint8_t *level)
{
	struct xec_signal signal_info = vw_tbl[signal];
	uint8_t xec_id = signal_info.xec_reg_idx;
	uint8_t src_id = signal_info.bit;

	if ((src_id >= ESPI_VWIRE_SRC_ID_MAX) ||
	    (xec_id >= ESPI_SMVW_IDX_MAX) || (level == NULL)) {
		return -EINVAL;
	}

	if (signal_info.dir == ESPI_MASTER_TO_SLAVE) {
		ESPI_MSVW_REG *reg = &(ESPI_M2S_VW_REGS->MSVW00) + xec_id;
		*level = ((reg->SRC >> (src_id << 3)) & 0x01ul);
	}

	if (signal_info.dir == ESPI_SLAVE_TO_MASTER) {
		ESPI_SMVW_REG *reg = &(ESPI_S2M_VW_REGS->SMVW00) + xec_id;
		*level = ((reg->SRC >> (src_id << 3)) & 0x01ul);
	}

	return 0;
}

#ifdef CONFIG_ESPI_OOB_CHANNEL
static int espi_xec_send_oob(const struct device *dev,
			     struct espi_oob_packet *pckt)
{
	int ret;
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);
	uint8_t err_mask = MCHP_ESPI_OOB_TX_STS_IBERR |
			MCHP_ESPI_OOB_TX_STS_OVRUN |
			MCHP_ESPI_OOB_TX_STS_BADREQ;

	LOG_DBG("%s", __func__);

	if (!(ESPI_OOB_REGS->TX_STS & MCHP_ESPI_OOB_TX_STS_CHEN)) {
		LOG_ERR("OOB channel is disabled");
		return -EIO;
	}

	if (ESPI_OOB_REGS->TX_STS & MCHP_ESPI_OOB_TX_STS_BUSY) {
		LOG_ERR("OOB channel is busy");
		return -EBUSY;
	}

	if (pckt->len > CONFIG_ESPI_OOB_BUFFER_SIZE) {
		LOG_ERR("insufficient space");
		return -EINVAL;
	}

	memcpy(target_tx_mem, pckt->buf, pckt->len);

	ESPI_OOB_REGS->TX_LEN = pckt->len;
	ESPI_OOB_REGS->TX_CTRL = MCHP_ESPI_OOB_TX_CTRL_START;
	LOG_DBG("%s %d", __func__, ESPI_OOB_REGS->TX_LEN);

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->tx_lock, K_MSEC(MAX_OOB_TIMEOUT));
	if (ret == -EAGAIN) {
		return -ETIMEDOUT;
	}

	if (ESPI_OOB_REGS->TX_STS & err_mask) {
		LOG_ERR("Tx failed %x", ESPI_OOB_REGS->TX_STS);
		ESPI_OOB_REGS->TX_STS = err_mask;
		return -EIO;
	}

	return 0;
}

static int espi_xec_receive_oob(const struct device *dev,
				struct espi_oob_packet *pckt)
{
	uint8_t err_mask = MCHP_ESPI_OOB_RX_STS_IBERR |
			MCHP_ESPI_OOB_RX_STS_OVRUN;

	if (ESPI_OOB_REGS->TX_STS & err_mask) {
		return -EIO;
	}

#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	int ret;
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->rx_lock, K_MSEC(MAX_OOB_TIMEOUT));
	if (ret == -EAGAIN) {
		return -ETIMEDOUT;
	}
#endif
	/* Check if buffer passed to driver can fit the received buffer */
	uint32_t rcvd_len = ESPI_OOB_REGS->RX_LEN & MCHP_ESPI_OOB_RX_LEN_MASK;

	if (rcvd_len > pckt->len) {
		LOG_ERR("space rcvd %d vs %d", rcvd_len, pckt->len);
		return -EIO;
	}

	pckt->len = rcvd_len;
	memcpy(pckt->buf, target_rx_mem, pckt->len);
	memset(target_rx_mem, 0, pckt->len);

	/* Only after data has been copied from SRAM, indicate channel
	 * is available for next packet
	 */
	ESPI_OOB_REGS->RX_CTRL |= MCHP_ESPI_OOB_RX_CTRL_AVAIL;

	return 0;
}
#endif /* CONFIG_ESPI_OOB_CHANNEL */

#ifdef CONFIG_ESPI_FLASH_CHANNEL
static int espi_xec_flash_read(const struct device *dev,
			       struct espi_flash_packet *pckt)
{
	int ret;
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);
	uint32_t err_mask = MCHP_ESPI_FC_STS_IBERR |
			MCHP_ESPI_FC_STS_FAIL |
			MCHP_ESPI_FC_STS_OVFL |
			MCHP_ESPI_FC_STS_BADREQ;

	LOG_DBG("%s", __func__);

	if (!(ESPI_FC_REGS->STS & MCHP_ESPI_FC_STS_CHAN_EN)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if (pckt->len > CONFIG_ESPI_FLASH_BUFFER_SIZE) {
		LOG_ERR("Invalid size request");
		return -EINVAL;
	}

	ESPI_FC_REGS->FL_ADDR_MSW = 0;
	ESPI_FC_REGS->FL_ADDR_LSW = pckt->flash_addr;
	ESPI_FC_REGS->MEM_ADDR_MSW = 0;
	ESPI_FC_REGS->MEM_ADDR_LSW = (uint32_t)&target_mem[0];
	ESPI_FC_REGS->XFR_LEN = pckt->len;
	ESPI_FC_REGS->CTRL = MCHP_ESPI_FC_CTRL_FUNC(MCHP_ESPI_FC_CTRL_RD0);
	ESPI_FC_REGS->CTRL |= MCHP_ESPI_FC_CTRL_START;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->flash_lock, K_MSEC(MAX_FLASH_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	if (ESPI_FC_REGS->STS & err_mask) {
		LOG_ERR("%s error %x", __func__, err_mask);
		ESPI_FC_REGS->STS = err_mask;
		return -EIO;
	}

	memcpy(pckt->buf, target_mem, pckt->len);

	return 0;
}

static int espi_xec_flash_write(const struct device *dev,
				struct espi_flash_packet *pckt)
{
	int ret;
	uint32_t err_mask = MCHP_ESPI_FC_STS_IBERR |
			MCHP_ESPI_FC_STS_OVRUN |
			MCHP_ESPI_FC_STS_FAIL |
			MCHP_ESPI_FC_STS_BADREQ;

	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);

	LOG_DBG("%s", __func__);

	if (sizeof(target_mem) < pckt->len) {
		LOG_ERR("Packet length is too big");
		return -ENOMEM;
	}

	if (!(ESPI_FC_REGS->STS & MCHP_ESPI_FC_STS_CHAN_EN)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if ((ESPI_FC_REGS->CFG & MCHP_ESPI_FC_CFG_BUSY)) {
		LOG_ERR("Flash channel is busy");
		return -EBUSY;
	}

	memcpy(target_mem, pckt->buf, pckt->len);

	ESPI_FC_REGS->FL_ADDR_MSW = 0;
	ESPI_FC_REGS->FL_ADDR_LSW = pckt->flash_addr;
	ESPI_FC_REGS->MEM_ADDR_MSW = 0;
	ESPI_FC_REGS->MEM_ADDR_LSW = (uint32_t)&target_mem[0];
	ESPI_FC_REGS->XFR_LEN = pckt->len;
	ESPI_FC_REGS->CTRL = MCHP_ESPI_FC_CTRL_FUNC(MCHP_ESPI_FC_CTRL_WR0);
	ESPI_FC_REGS->CTRL |= MCHP_ESPI_FC_CTRL_START;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->flash_lock, K_MSEC(MAX_FLASH_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	if (ESPI_FC_REGS->STS & err_mask) {
		LOG_ERR("%s err: %x", __func__, err_mask);
		ESPI_FC_REGS->STS = err_mask;
		return -EIO;
	}

	return 0;
}

static int espi_xec_flash_erase(const struct device *dev,
				struct espi_flash_packet *pckt)
{
	int ret;
	uint32_t status;
	uint32_t err_mask = MCHP_ESPI_FC_STS_IBERR |
			MCHP_ESPI_FC_STS_OVRUN |
			MCHP_ESPI_FC_STS_FAIL |
			MCHP_ESPI_FC_STS_BADREQ;

	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);

	LOG_DBG("%s", __func__);

	if (!(ESPI_FC_REGS->STS & MCHP_ESPI_FC_STS_CHAN_EN)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if ((ESPI_FC_REGS->CFG & MCHP_ESPI_FC_CFG_BUSY)) {
		LOG_ERR("Flash channel is busy");
		return -EBUSY;
	}

	/* Clear status register */
	status = ESPI_FC_REGS->STS;
	ESPI_FC_REGS->STS = status;

	ESPI_FC_REGS->FL_ADDR_MSW = 0;
	ESPI_FC_REGS->FL_ADDR_LSW = pckt->flash_addr;
	ESPI_FC_REGS->XFR_LEN = ESPI_FLASH_ERASE_DUMMY;
	ESPI_FC_REGS->CTRL = MCHP_ESPI_FC_CTRL_FUNC(MCHP_ESPI_FC_CTRL_ERS0);
	ESPI_FC_REGS->CTRL |= MCHP_ESPI_FC_CTRL_START;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->flash_lock, K_MSEC(MAX_FLASH_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	if (ESPI_FC_REGS->STS & err_mask) {
		LOG_ERR("%s err: %x", __func__, err_mask);
		ESPI_FC_REGS->STS = err_mask;
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_ESPI_FLASH_CHANNEL */

static int espi_xec_manage_callback(const struct device *dev,
				    struct espi_callback *callback, bool set)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);

	return espi_manage_callback(&data->callbacks, callback, set);
}

#ifdef CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE
static void send_slave_bootdone(const struct device *dev)
{
	int ret;
	uint8_t boot_done;

	ret = espi_xec_receive_vwire(dev, ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE,
				     &boot_done);
	if (!ret && !boot_done) {
		/* SLAVE_BOOT_DONE & SLAVE_LOAD_STS have to be sent together */
		espi_xec_send_vwire(dev, ESPI_VWIRE_SIGNAL_SLV_BOOT_STS, 1);
		espi_xec_send_vwire(dev, ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE, 1);
	}
}
#endif

#ifdef CONFIG_ESPI_OOB_CHANNEL
static void espi_init_oob(const struct device *dev)
{
	struct espi_xec_config *config =
		(struct espi_xec_config *) (dev->config);

	/* Enable OOB Tx/Rx interrupts */
	MCHP_GIRQ_ENSET(config->bus_girq_id) = (MCHP_ESPI_OOB_UP_GIRQ_VAL |
			MCHP_ESPI_OOB_DN_GIRQ_VAL);

	ESPI_OOB_REGS->TX_ADDR_MSW = 0;
	ESPI_OOB_REGS->RX_ADDR_MSW = 0;
	ESPI_OOB_REGS->TX_ADDR_LSW = (uint32_t)&target_tx_mem[0];
	ESPI_OOB_REGS->RX_ADDR_LSW = (uint32_t)&target_rx_mem[0];
	ESPI_OOB_REGS->RX_LEN = 0x00FF0000;

	/* Enable OOB Tx channel enable change status interrupt */
	ESPI_OOB_REGS->TX_IEN |= MCHP_ESPI_OOB_TX_IEN_CHG_EN |
				MCHP_ESPI_OOB_TX_IEN_DONE;

	/* Enable Rx channel to receive data any time
	 * there are case where OOB is not initiated by a previous OOB Tx
	 */
	ESPI_OOB_REGS->RX_IEN |= MCHP_ESPI_OOB_RX_IEN;
	ESPI_OOB_REGS->RX_CTRL |= MCHP_ESPI_OOB_RX_CTRL_AVAIL;
}
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
static void espi_init_flash(const struct device *dev)
{
	struct espi_xec_config *config =
	    (struct espi_xec_config *)(dev->config);

	LOG_DBG("%s", __func__);

	/* Need to clear status done when ROM boots in MAF */
	LOG_DBG("%s ESPI_FC_REGS->CFG %X", __func__, ESPI_FC_REGS->CFG);
	ESPI_FC_REGS->STS = MCHP_ESPI_FC_STS_DONE;

	/* Enable interrupts */
	MCHP_GIRQ_ENSET(config->bus_girq_id) = BIT(MCHP_ESPI_FC_GIRQ_POS);
	ESPI_FC_REGS->IEN |= MCHP_ESPI_FC_IEN_CHG_EN;
	ESPI_FC_REGS->IEN |= MCHP_ESPI_FC_IEN_DONE;
}
#endif

static void espi_bus_init(const struct device *dev)
{
	const struct espi_xec_config *config = dev->config;

	/* Enable bus interrupts */
	MCHP_GIRQ_ENSET(config->bus_girq_id) = MCHP_ESPI_ESPI_RST_GIRQ_VAL |
		MCHP_ESPI_VW_EN_GIRQ_VAL | MCHP_ESPI_PC_GIRQ_VAL;
}

void espi_config_vw_ocb(void)
{
	ESPI_SMVW_REG *reg = &(ESPI_S2M_VW_REGS->SMVW06);

	/* Keep index bits [7:0] in initial 0h value (disabled state) */
	mec_espi_smvw_index_set(reg, 0);
	/* Set 01b (eSPI_RESET# domain) into bits [9:8] which frees the
	 * register from all except chip level resets and set initial state
	 * of VW wires as 1111b in bits [15:12].
	 */
	mec_espi_msvw_stom_set(reg, VW_RST_SRC_ESPI_RESET, 0x1);
	/* Set 4 SMVW SRC bits in bit positions [0], [8], [16] and [24] to
	 * initial value '1'.
	 */
	mec_espi_smvw_set_all_bitmap(reg, 0xF);
	/* Set 00b (eSPI_RESET# domain) into bits [9:8] while preserving
	 * the values in bits [15:12].
	 */
	mec_espi_msvw_stom_set(reg, VW_RST_SRC_ESPI_RESET, 0x0);
	/* Set INDEX field with OCB VW index */
	mec_espi_smvw_index_set(reg, ESPI_OCB_VW_INDEX);
}

static void espi_rst_isr(const struct device *dev)
{
	uint8_t rst_sts;
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);
	struct espi_event evt = { ESPI_BUS_RESET, 0, 0 };

	rst_sts = ESPI_CAP_REGS->ERST_STS;

	/* eSPI reset status register is clear on write register */
	ESPI_CAP_REGS->ERST_STS = MCHP_ESPI_RST_ISTS;

	if (rst_sts & MCHP_ESPI_RST_ISTS) {
		if (rst_sts & MCHP_ESPI_RST_ISTS_PIN_RO_HI) {
			evt.evt_data = 1;
		} else {
			evt.evt_data = 0;
		}

		espi_send_callbacks(&data->callbacks, dev, evt);
#ifdef CONFIG_ESPI_OOB_CHANNEL
		espi_init_oob(dev);
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
		espi_init_flash(dev);
#endif
		espi_bus_init(dev);
	}
}

/* Configure sub devices BAR address if not using default I/O based address
 * then make its BAR valid.
 * Refer to microchip eSPI I/O base addresses for default values
 */
static void config_sub_devices(const struct device *dev)
{
#ifdef CONFIG_ESPI_PERIPHERAL_UART
	/* eSPI logical UART is tied to corresponding physical UART
	 * Not all boards use same UART port for debug, hence needs to set
	 * eSPI host logical UART0 bar address based on configuration.
	 */
	switch (CONFIG_ESPI_PERIPHERAL_UART_SOC_MAPPING) {
	case 0:
		ESPI_EIO_BAR_REGS->EC_BAR_UART_0 = ESPI_XEC_UART0_BAR_ADDRESS |
		MCHP_ESPI_IO_BAR_HOST_VALID;
		break;
	case 1:
		ESPI_EIO_BAR_REGS->EC_BAR_UART_1 = ESPI_XEC_UART0_BAR_ADDRESS |
		MCHP_ESPI_IO_BAR_HOST_VALID;
		break;
	case 2:
		ESPI_EIO_BAR_REGS->EC_BAR_UART_2 = ESPI_XEC_UART0_BAR_ADDRESS |
		MCHP_ESPI_IO_BAR_HOST_VALID;
		break;
	}
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	KBC_REGS->KBC_CTRL |= MCHP_KBC_CTRL_AUXH;
	KBC_REGS->KBC_CTRL |= MCHP_KBC_CTRL_OBFEN;
	/* This is the activate register, but the HAL has a funny name */
	KBC_REGS->KBC_PORT92_EN = MCHP_KBC_PORT92_EN;
	ESPI_EIO_BAR_REGS->EC_BAR_KBC = ESPI_XEC_KBC_BAR_ADDRESS |
		MCHP_ESPI_IO_BAR_HOST_VALID;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
	ESPI_EIO_BAR_REGS->EC_BAR_ACPI_EC_0 |= MCHP_ESPI_IO_BAR_HOST_VALID;
	ESPI_EIO_BAR_REGS->EC_BAR_MBOX = ESPI_XEC_MBOX_BAR_ADDRESS |
		MCHP_ESPI_IO_BAR_HOST_VALID;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT
	ESPI_EIO_BAR_REGS->EC_BAR_ACPI_EC_1 =
	       CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT_PORT_NUM |
		MCHP_ESPI_IO_BAR_HOST_VALID;
	ESPI_EIO_BAR_REGS->EC_BAR_MBOX = ESPI_XEC_MBOX_BAR_ADDRESS |
		MCHP_ESPI_IO_BAR_HOST_VALID;
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	ESPI_EIO_BAR_REGS->EC_BAR_P80CAP_0 = ESPI_XEC_PORT80_BAR_ADDRESS |
		MCHP_ESPI_IO_BAR_HOST_VALID;
	PORT80_CAP0_REGS->ACTV = 1;
	ESPI_EIO_BAR_REGS->EC_BAR_P80CAP_1 = ESPI_XEC_PORT81_BAR_ADDRESS |
		MCHP_ESPI_IO_BAR_HOST_VALID;
	PORT80_CAP1_REGS->ACTV = 1;
#endif
}

static void configure_sirq(void)
{
#ifdef CONFIG_ESPI_PERIPHERAL_UART
	switch (CONFIG_ESPI_PERIPHERAL_UART_SOC_MAPPING) {
	case ESPI_PERIPHERAL_UART_PORT0:
		ESPI_SIRQ_REGS->UART_0_SIRQ = UART_DEFAULT_IRQ;
		break;
	case ESPI_PERIPHERAL_UART_PORT1:
		ESPI_SIRQ_REGS->UART_1_SIRQ = UART_DEFAULT_IRQ;
		break;
	case ESPI_PERIPHERAL_UART_PORT2:
		ESPI_SIRQ_REGS->UART_2_SIRQ = UART_DEFAULT_IRQ;
		break;
	}
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	ESPI_SIRQ_REGS->KBC_SIRQ_0 = 0x01;
	ESPI_SIRQ_REGS->KBC_SIRQ_1 = 0x0C;
#endif
}

static void setup_espi_io_config(const struct device *dev,
				 uint16_t host_address)
{
	ESPI_EIO_BAR_REGS->EC_BAR_IOC = (host_address << 16) |
		MCHP_ESPI_IO_BAR_HOST_VALID;

	config_sub_devices(dev);
	configure_sirq();

	ESPI_PC_REGS->PC_STATUS = (MCHP_ESPI_PC_STS_EN_CHG |
				    MCHP_ESPI_PC_STS_BM_EN_CHG_POS);
	ESPI_PC_REGS->PC_IEN |= MCHP_ESPI_PC_IEN_EN_CHG;
	ESPI_CAP_REGS->PC_RDY = 1;
}

static void espi_pc_isr(const struct device *dev)
{
	uint32_t status = ESPI_PC_REGS->PC_STATUS;

	if (status & MCHP_ESPI_PC_STS_EN_CHG) {
		if (status & MCHP_ESPI_PC_STS_EN) {
			setup_espi_io_config(dev, MCHP_ESPI_IOBAR_INIT_DFLT);
		}

		ESPI_PC_REGS->PC_STATUS = MCHP_ESPI_PC_STS_EN_CHG;
	}
}

static void espi_vwire_chanel_isr(const struct device *dev)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);
	const struct espi_xec_config *config = dev->config;
	struct espi_event evt = { .evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				  .evt_details = ESPI_CHANNEL_VWIRE,
				  .evt_data = 0 };
	uint32_t status;

	status = ESPI_IO_VW_REGS->VW_EN_STS;

	if (status & MCHP_ESPI_VW_EN_STS_RO) {
		ESPI_IO_VW_REGS->VW_RDY = 1;
		evt.evt_data = 1;
		/* VW channel interrupt can disabled at this point */
		MCHP_GIRQ_ENCLR(config->bus_girq_id) = MCHP_ESPI_VW_EN_GIRQ_VAL;
#ifdef CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE
		send_slave_bootdone(dev);
#endif
	}

	espi_send_callbacks(&data->callbacks, dev, evt);
}

#ifdef CONFIG_ESPI_OOB_CHANNEL
static void espi_oob_down_isr(const struct device *dev)
{
	uint32_t status;
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	struct espi_event evt = { .evt_type = ESPI_BUS_EVENT_OOB_RECEIVED,
				  .evt_details = 0,
				  .evt_data = 0 };
#endif

	status = ESPI_OOB_REGS->RX_STS;

	LOG_DBG("%s %x", __func__, status);
	if (status & MCHP_ESPI_OOB_RX_STS_DONE) {
		/* Register is write-on-clear, ensure only 1 bit is affected */
		ESPI_OOB_REGS->RX_STS = MCHP_ESPI_OOB_RX_STS_DONE;

#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
		k_sem_give(&data->rx_lock);
#else
		evt.evt_details = ESPI_OOB_REGS->RX_LEN & MCHP_ESPI_OOB_RX_LEN_MASK;
		espi_send_callbacks(&data->callbacks, dev, evt);
#endif
	}
}

static void espi_oob_up_isr(const struct device *dev)
{
	uint32_t status;
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);
	struct espi_event evt = { .evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				  .evt_details = ESPI_CHANNEL_OOB,
				  .evt_data = 0
				};

	status = ESPI_OOB_REGS->TX_STS;
	LOG_DBG("%s sts:%x", __func__, status);

	if (status & MCHP_ESPI_OOB_TX_STS_DONE) {
		/* Register is write-on-clear, ensure only 1 bit is affected */
		ESPI_OOB_REGS->TX_STS = MCHP_ESPI_OOB_TX_STS_DONE;
		k_sem_give(&data->tx_lock);
	}

	if (status & MCHP_ESPI_OOB_TX_STS_CHG_EN) {
		if (status & MCHP_ESPI_OOB_TX_STS_CHEN) {
			espi_init_oob(dev);
			/* Indicate OOB channel is ready to eSPI host */
			ESPI_CAP_REGS->OOB_RDY = 1;
			evt.evt_data = 1;
		}

		ESPI_OOB_REGS->TX_STS = MCHP_ESPI_OOB_TX_STS_CHG_EN;
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
static void espi_flash_isr(const struct device *dev)
{
	uint32_t status;
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);
	struct espi_event evt = { .evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				  .evt_details = ESPI_CHANNEL_FLASH,
				  .evt_data = 0,
				};

	status = ESPI_FC_REGS->STS;
	LOG_DBG("%s %x", __func__, status);

	if (status & MCHP_ESPI_FC_STS_DONE) {
		/* Ensure to clear only relevant bit */
		ESPI_FC_REGS->STS = MCHP_ESPI_FC_STS_DONE;

		k_sem_give(&data->flash_lock);
	}

	if (status & MCHP_ESPI_FC_STS_CHAN_EN_CHG) {
		/* Ensure to clear only relevant bit */
		ESPI_FC_REGS->STS = MCHP_ESPI_FC_STS_CHAN_EN_CHG;

		if (status & MCHP_ESPI_FC_STS_CHAN_EN) {
			espi_init_flash(dev);
			/* Indicate flash channel is ready to eSPI master */
			ESPI_CAP_REGS->FC_RDY = MCHP_ESPI_FC_READY;
			evt.evt_data = 1;
		}

		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}
#endif

static void vw_pltrst_isr(const struct device *dev)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);
	struct espi_event evt = { ESPI_BUS_EVENT_VWIRE_RECEIVED,
		ESPI_VWIRE_SIGNAL_PLTRST, 0
	};
	uint8_t status = 0;

	espi_xec_receive_vwire(dev, ESPI_VWIRE_SIGNAL_PLTRST, &status);
	if (status) {
		setup_espi_io_config(dev, MCHP_ESPI_IOBAR_INIT_DFLT);
	}

	evt.evt_data = status;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

/* Send callbacks if enabled and track eSPI host system state */
static void notify_system_state(const struct device *dev,
				enum espi_vwire_signal signal)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);
	struct espi_event evt = { ESPI_BUS_EVENT_VWIRE_RECEIVED, 0, 0 };
	uint8_t status = 0;

	espi_xec_receive_vwire(dev, signal, &status);
	evt.evt_details = signal;
	evt.evt_data = status;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void notify_host_warning(const struct device *dev,
				enum espi_vwire_signal signal)
{
	uint8_t status;

	espi_xec_receive_vwire(dev, signal, &status);

	if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
		struct espi_xec_data *data =
			(struct espi_xec_data *)(dev->data);
		struct espi_event evt = {ESPI_BUS_EVENT_VWIRE_RECEIVED, 0, 0 };

		evt.evt_details = signal;
		evt.evt_data = status;
		espi_send_callbacks(&data->callbacks, dev, evt);
	} else {
		k_busy_wait(ESPI_XEC_VWIRE_ACK_DELAY);
		/* Some flows are dependent on awareness of client's driver
		 * about these warnings in such cases these automatic response
		 * should not be enabled.
		 */
		switch (signal) {
		case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
			espi_xec_send_vwire(dev,
					    ESPI_VWIRE_SIGNAL_HOST_RST_ACK,
					    status);
			break;
		case ESPI_VWIRE_SIGNAL_SUS_WARN:
			espi_xec_send_vwire(dev, ESPI_VWIRE_SIGNAL_SUS_ACK,
					    status);
			break;
		case ESPI_VWIRE_SIGNAL_OOB_RST_WARN:
			espi_xec_send_vwire(dev, ESPI_VWIRE_SIGNAL_OOB_RST_ACK,
					    status);
			break;
		case ESPI_VWIRE_SIGNAL_DNX_WARN:
			espi_xec_send_vwire(dev, ESPI_VWIRE_SIGNAL_DNX_ACK,
					    status);
			break;
		default:
			break;
		}
	}
}

static void vw_slp3_isr(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SLP_S3);
}

static void vw_slp4_isr(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SLP_S4);
}

static void vw_slp5_isr(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SLP_S5);
}

static void vw_host_rst_warn_isr(const struct device *dev)
{
	notify_host_warning(dev, ESPI_VWIRE_SIGNAL_HOST_RST_WARN);
}

static void vw_sus_warn_isr(const struct device *dev)
{
	notify_host_warning(dev, ESPI_VWIRE_SIGNAL_SUS_WARN);
	/* Configure spare VW register SMVW06 to VW index 50h. As per
	 * per microchip recommendation, spare VW register should be
	 * configured between SLAVE_BOOT_LOAD_DONE = 1 VW event and
	 * point where SUS_ACK=1 VW is sent to SOC.
	 */
	espi_config_vw_ocb();
}

static void vw_oob_rst_isr(const struct device *dev)
{
	notify_host_warning(dev, ESPI_VWIRE_SIGNAL_OOB_RST_WARN);
}

static void vw_sus_pwrdn_ack_isr(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK);
}

static void vw_sus_slp_a_isr(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SLP_A);
}

static void ibf_isr(const struct device *dev)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);
	struct espi_event evt = { ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_HOST_IO, ESPI_PERIPHERAL_NODATA
	};

	espi_send_callbacks(&data->callbacks, dev, evt);
}

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT
static void ibf_pvt_isr(const struct device *dev)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_HOST_IO_PVT,
		.evt_data = ESPI_PERIPHERAL_NODATA
	};

	espi_send_callbacks(&data->callbacks, dev, evt);
}
#endif

static void ibf_kbc_isr(const struct device *dev)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);

	/* The high byte contains information from the host,
	 * and the lower byte specifies if the host sent
	 * a command or data. 1 = Command.
	 */
	uint32_t isr_data = ((KBC_REGS->EC_DATA & 0xFF) << E8042_ISR_DATA_POS) |
				((KBC_REGS->EC_KBC_STS & MCHP_KBC_STS_CD) <<
				 E8042_ISR_CMD_DATA_POS);

	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_8042_KBC,
		.evt_data = isr_data
	};

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void port80_isr(const struct device *dev)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);
	struct espi_event evt = { ESPI_BUS_PERIPHERAL_NOTIFICATION,
		(ESPI_PERIPHERAL_INDEX_0 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,
		ESPI_PERIPHERAL_NODATA
	};

	evt.evt_data = PORT80_CAP0_REGS->EC_DATA;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void port81_isr(const struct device *dev)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);
	struct espi_event evt = { ESPI_BUS_PERIPHERAL_NOTIFICATION,
		(ESPI_PERIPHERAL_INDEX_1 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,
		ESPI_PERIPHERAL_NODATA
	};

	evt.evt_data = PORT80_CAP1_REGS->EC_DATA;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

const struct espi_isr espi_bus_isr[] = {
	{MCHP_ESPI_PC_GIRQ_VAL, espi_pc_isr},
#ifdef CONFIG_ESPI_OOB_CHANNEL
	{MCHP_ESPI_OOB_UP_GIRQ_VAL, espi_oob_up_isr},
	{MCHP_ESPI_OOB_DN_GIRQ_VAL, espi_oob_down_isr},
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	{MCHP_ESPI_FC_GIRQ_VAL, espi_flash_isr},
#endif
	{MCHP_ESPI_ESPI_RST_GIRQ_VAL, espi_rst_isr},
	{MCHP_ESPI_VW_EN_GIRQ_VAL, espi_vwire_chanel_isr},
};

uint8_t vw_wires_int_en[] = {
	ESPI_VWIRE_SIGNAL_SLP_S3,
	ESPI_VWIRE_SIGNAL_SLP_S4,
	ESPI_VWIRE_SIGNAL_SLP_S5,
	ESPI_VWIRE_SIGNAL_PLTRST,
	ESPI_VWIRE_SIGNAL_OOB_RST_WARN,
	ESPI_VWIRE_SIGNAL_HOST_RST_WARN,
	ESPI_VWIRE_SIGNAL_SUS_WARN,
	ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK,
	ESPI_VWIRE_SIGNAL_DNX_WARN,
};

const struct espi_isr m2s_vwires_isr[] = {
	{MEC_ESPI_MSVW00_SRC0_VAL, vw_slp3_isr},
	{MEC_ESPI_MSVW00_SRC1_VAL, vw_slp4_isr},
	{MEC_ESPI_MSVW00_SRC2_VAL, vw_slp5_isr},
	{MEC_ESPI_MSVW01_SRC1_VAL, vw_pltrst_isr},
	{MEC_ESPI_MSVW01_SRC2_VAL, vw_oob_rst_isr},
	{MEC_ESPI_MSVW02_SRC0_VAL, vw_host_rst_warn_isr},
	{MEC_ESPI_MSVW03_SRC0_VAL, vw_sus_warn_isr},
	{MEC_ESPI_MSVW03_SRC1_VAL, vw_sus_pwrdn_ack_isr},
	{MEC_ESPI_MSVW03_SRC3_VAL, vw_sus_slp_a_isr},
};

const struct espi_isr peripherals_isr[] = {
	{MCHP_ACPI_EC_0_IBF_GIRQ, ibf_isr},
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT
	{MCHP_ACPI_EC_1_IBF_GIRQ, ibf_pvt_isr},
#endif
	{MCHP_KBC_IBF_GIRQ, ibf_kbc_isr},
	{MCHP_PORT80_DEBUG0_GIRQ_VAL, port80_isr},
	{MCHP_PORT80_DEBUG1_GIRQ_VAL, port81_isr},
};

static uint8_t bus_isr_cnt = sizeof(espi_bus_isr) / sizeof(struct espi_isr);
static uint8_t m2s_vwires_isr_cnt =
	sizeof(m2s_vwires_isr) / sizeof(struct espi_isr);
static uint8_t periph_isr_cnt = sizeof(peripherals_isr) / sizeof(struct espi_isr);

static void espi_xec_bus_isr(const struct device *dev)
{
	const struct espi_xec_config *config = dev->config;
	uint32_t girq_result;

	girq_result = MCHP_GIRQ_RESULT(config->bus_girq_id);

	for (int i = 0; i < bus_isr_cnt; i++) {
		struct espi_isr entry = espi_bus_isr[i];

		if (girq_result & entry.girq_bit) {
			if (entry.the_isr != NULL) {
				entry.the_isr(dev);
			}
		}
	}

	REG32(MCHP_GIRQ_SRC_ADDR(config->bus_girq_id)) = girq_result;
}

static void espi_xec_vw_isr(const struct device *dev)
{
	const struct espi_xec_config *config = dev->config;
	uint32_t girq_result;

	girq_result = MCHP_GIRQ_RESULT(config->vw_girq_ids[0]);

	for (int i = 0; i < m2s_vwires_isr_cnt; i++) {
		struct espi_isr entry = m2s_vwires_isr[i];

		if (girq_result & entry.girq_bit) {
			if (entry.the_isr != NULL) {
				entry.the_isr(dev);
			}
		}
	}

	REG32(MCHP_GIRQ_SRC_ADDR(config->vw_girq_ids[0])) = girq_result;
}

#if DT_INST_PROP_HAS_IDX(0, vw_girqs, 1)
static void vw_sus_dnx_warn_isr(const struct device *dev)
{
	notify_host_warning(dev, ESPI_VWIRE_SIGNAL_DNX_WARN);
}

const struct espi_isr m2s_vwires_ext_isr[] = {
	{MEC_ESPI_MSVW08_SRC1_VAL, vw_sus_dnx_warn_isr}
};

static void espi_xec_vw_ext_isr(const struct device *dev)
{
	const struct espi_xec_config *config = dev->config;
	uint32_t girq_result;

	girq_result = MCHP_GIRQ_RESULT(config->vw_girq_ids[1]);
	MCHP_GIRQ_SRC(config->vw_girq_ids[1]) = girq_result;

	for (int i = 0; i < ARRAY_SIZE(m2s_vwires_ext_isr); i++) {
		struct espi_isr entry = m2s_vwires_ext_isr[i];

		if (girq_result & entry.girq_bit) {
			if (entry.the_isr != NULL) {
				entry.the_isr(dev);
			}
		}
	}
}
#endif

static void espi_xec_periph_isr(const struct device *dev)
{
	const struct espi_xec_config *config = dev->config;
	uint32_t girq_result;

	girq_result = MCHP_GIRQ_RESULT(config->pc_girq_id);

	for (int i = 0; i < periph_isr_cnt; i++) {
		struct espi_isr entry = peripherals_isr[i];

		if (girq_result & entry.girq_bit) {
			if (entry.the_isr != NULL) {
				entry.the_isr(dev);
			}
		}
	}

	REG32(MCHP_GIRQ_SRC_ADDR(config->pc_girq_id)) = girq_result;
}

static int espi_xec_init(const struct device *dev);

static const struct espi_driver_api espi_xec_driver_api = {
	.config = espi_xec_configure,
	.get_channel_status = espi_xec_channel_ready,
	.send_vwire = espi_xec_send_vwire,
	.receive_vwire = espi_xec_receive_vwire,
#ifdef CONFIG_ESPI_OOB_CHANNEL
	.send_oob = espi_xec_send_oob,
	.receive_oob = espi_xec_receive_oob,
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	.flash_read = espi_xec_flash_read,
	.flash_write = espi_xec_flash_write,
	.flash_erase = espi_xec_flash_erase,
#endif
	.manage_callback = espi_xec_manage_callback,
	.read_lpc_request = espi_xec_read_lpc_request,
	.write_lpc_request = espi_xec_write_lpc_request,
};

static struct espi_xec_data espi_xec_data;

/* pin control structure(s) */
PINCTRL_DT_INST_DEFINE(0);

static const struct espi_xec_config espi_xec_config = {
	.base_addr = DT_INST_REG_ADDR(0),
	.bus_girq_id = DT_INST_PROP(0, io_girq),
	.vw_girq_ids[0] = DT_INST_PROP_BY_IDX(0, vw_girqs, 0),
	.vw_girq_ids[1] = DT_INST_PROP_BY_IDX(0, vw_girqs, 1),
	.pc_girq_id = DT_INST_PROP(0, pc_girq),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

DEVICE_DT_INST_DEFINE(0, &espi_xec_init, NULL,
		    &espi_xec_data, &espi_xec_config,
		    PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY,
		    &espi_xec_driver_api);

static int espi_xec_init(const struct device *dev)
{
	const struct espi_xec_config *config = dev->config;
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->data);
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("XEC eSPI pinctrl setup failed (%d)", ret);
		return ret;
	}

	/* Configure eSPI_PLTRST# to cause nSIO_RESET reset */
	PCR_REGS->PWR_RST_CTRL = MCHP_PCR_PR_CTRL_USE_ESPI_PLTRST;
	ESPI_CAP_REGS->PLTRST_SRC = MCHP_ESPI_PLTRST_SRC_IS_VW;

	/* Configure the channels and its capabilities based on build config */
	ESPI_CAP_REGS->GLB_CAP0 |= MCHP_ESPI_GBL_CAP0_VW_SUPP;
	ESPI_CAP_REGS->GLB_CAP0 |= MCHP_ESPI_GBL_CAP0_PC_SUPP;

	/* Max VW count is 12 pairs */
	ESPI_CAP_REGS->VW_CAP = ESPI_NUM_SMVW;
	ESPI_CAP_REGS->PC_CAP |= MCHP_ESPI_PC_CAP_MAX_PLD_SZ_64;

#ifdef CONFIG_ESPI_OOB_CHANNEL
	ESPI_CAP_REGS->GLB_CAP0 |= MCHP_ESPI_GBL_CAP0_OOB_SUPP;
	ESPI_CAP_REGS->OOB_CAP |= MCHP_ESPI_OOB_CAP_MAX_PLD_SZ_73;

	k_sem_init(&data->tx_lock, 0, 1);
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	k_sem_init(&data->rx_lock, 0, 1);
#endif /* CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC */
#else
	ESPI_CAP_REGS->GLB_CAP0 &= ~MCHP_ESPI_GBL_CAP0_OOB_SUPP;
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	ESPI_CAP_REGS->GLB_CAP0 |= MCHP_ESPI_GBL_CAP0_FC_SUPP;
	ESPI_CAP_REGS->GLB_CAP0 |= MCHP_ESPI_FC_CAP_MAX_PLD_SZ_64;
	ESPI_CAP_REGS->FC_CAP |= MCHP_ESPI_FC_CAP_SHARE_MAF_SAF;
	ESPI_CAP_REGS->FC_CAP |= MCHP_ESPI_FC_CAP_MAX_RD_SZ_64;

	k_sem_init(&data->flash_lock, 0, 1);
#else
	ESPI_CAP_REGS->GLB_CAP0 &= ~MCHP_ESPI_GBL_CAP0_FC_SUPP;
#endif

	/* Clear reset interrupt status and enable interrupts */
	ESPI_CAP_REGS->ERST_STS = MCHP_ESPI_RST_ISTS;
	ESPI_CAP_REGS->ERST_IEN |= MCHP_ESPI_RST_IEN;
	ESPI_PC_REGS->PC_STATUS = MCHP_ESPI_PC_STS_EN_CHG;
	ESPI_PC_REGS->PC_IEN |= MCHP_ESPI_PC_IEN_EN_CHG;

	/* Enable VWires interrupts */
	for (int i = 0; i < sizeof(vw_wires_int_en); i++) {
		uint8_t signal = vw_wires_int_en[i];
		struct xec_signal signal_info = vw_tbl[signal];
		uint8_t xec_id = signal_info.xec_reg_idx;
		ESPI_MSVW_REG *reg = &(ESPI_M2S_VW_REGS->MSVW00) + xec_id;

		mec_espi_msvw_irq_sel_set(reg, signal_info.bit,
					  MSVW_IRQ_SEL_EDGE_BOTH);
	}

	/* Enable interrupts for each logical channel enable assertion */
	MCHP_GIRQ_ENSET(config->bus_girq_id) = MCHP_ESPI_ESPI_RST_GIRQ_VAL |
		MCHP_ESPI_VW_EN_GIRQ_VAL | MCHP_ESPI_PC_GIRQ_VAL;

#ifdef CONFIG_ESPI_OOB_CHANNEL
	espi_init_oob(dev);
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	espi_init_flash(dev);
#endif
	/* Enable aggregated block interrupts for VWires */
	MCHP_GIRQ_ENSET(config->vw_girq_ids[0]) = MEC_ESPI_MSVW00_SRC0_VAL |
		MEC_ESPI_MSVW00_SRC1_VAL | MEC_ESPI_MSVW00_SRC2_VAL |
		MEC_ESPI_MSVW01_SRC1_VAL | MEC_ESPI_MSVW01_SRC2_VAL |
		MEC_ESPI_MSVW02_SRC0_VAL | MEC_ESPI_MSVW03_SRC0_VAL;

	/* Enable aggregated block interrupts for peripherals supported */
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	MCHP_GIRQ_ENSET(config->pc_girq_id) = MCHP_KBC_IBF_GIRQ;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
	MCHP_GIRQ_ENSET(config->pc_girq_id) = MCHP_ACPI_EC_0_IBF_GIRQ;
	MCHP_GIRQ_ENSET(config->pc_girq_id) = MCHP_ACPI_EC_2_IBF_GIRQ;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT
	MCHP_GIRQ_ENSET(config->pc_girq_id) = MCHP_ACPI_EC_1_IBF_GIRQ;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	MCHP_GIRQ_ENSET(config->pc_girq_id) = MCHP_PORT80_DEBUG0_GIRQ_VAL |
		MCHP_PORT80_DEBUG1_GIRQ_VAL;
#endif
	/* Enable aggregated interrupt block for eSPI bus events */
	MCHP_GIRQ_BLK_SETEN(config->bus_girq_id);
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    espi_xec_bus_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	/* Enable aggregated interrupt block for eSPI VWire events */
	MCHP_GIRQ_BLK_SETEN(config->vw_girq_ids[0]);
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 1, irq),
		    DT_INST_IRQ_BY_IDX(0, 1, priority),
		    espi_xec_vw_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(0, 1, irq));

	/* Enable aggregated interrupt block for eSPI peripheral channel */
	MCHP_GIRQ_BLK_SETEN(config->pc_girq_id);
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 2, irq),
		    DT_INST_IRQ_BY_IDX(0, 2, priority),
		    espi_xec_periph_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(0, 2, irq));

#if DT_INST_PROP_HAS_IDX(0, vw_girqs, 1)
	MCHP_GIRQ_ENSET(config->vw_girq_ids[1]) = MEC_ESPI_MSVW08_SRC1_VAL;
	MCHP_GIRQ_BLK_SETEN(config->vw_girq_ids[1]);
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 3, irq),
		    DT_INST_IRQ_BY_IDX(0, 3, priority),
		    espi_xec_vw_ext_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(0, 3, irq));
#endif

	return 0;
}
