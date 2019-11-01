/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <soc.h>
#include <errno.h>
#include <drivers/espi.h>
#include <logging/log.h>
#include "espi_utils.h"

/* Minimum delay before acknowledging a virtual wire */
#define ESPI_XEC_VWIRE_ACK_DELAY    10ul

/* Maximum timeout to transmit a virtual wire packet.
 * 10 ms expresed in multiples of 100us
 */
#define ESPI_XEC_VWIRE_SEND_TIMEOUT 100

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

#define LOG_LEVEL CONFIG_ESPI_LOG_LEVEL
LOG_MODULE_REGISTER(espi);

struct espi_isr {
	u32_t girq_bit;
	void (*the_isr)(struct device *dev);
};

struct espi_xec_config {
	u32_t base_addr;
	u8_t bus_girq_id;
	u8_t vw_girq_id;
	u8_t pc_girq_id;
};

struct espi_xec_data {
	sys_slist_t callbacks;
	u8_t plt_rst_asserted;
	u8_t espi_rst_asserted;
	u8_t sx_state;
};

struct xec_signal {
	u8_t xec_reg_idx;
	u8_t bit;
	u8_t dir;
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

/* Microchip cannonical virtual wire mapping
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
};

static int espi_xec_configure(struct device *dev, struct espi_cfg *cfg)
{
	u8_t iomode = 0;
	u8_t cap1 = ESPI_CAP_REGS->GLB_CAP1;
	u8_t cur_iomode = (cap1 & MCHP_ESPI_GBL_CAP1_IO_MODE_MASK) >>
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
		cap1 &= ~MCHP_ESPI_GBL_CAP1_IO_MODE_MASK0 <<
			MCHP_ESPI_GBL_CAP1_IO_MODE_POS;
		cap1 |= (iomode << MCHP_ESPI_GBL_CAP1_IO_MODE_POS);
	}

	ESPI_CAP_REGS->GLB_CAP1 = cap1;

	/* Activate the eSPI block *.
	 * Need to guarantee that this register is configured before RSMRST#
	 * de-assertion and after pinmux
	 */
	ESPI_EIO_BAR_REGS->IO_ACTV = 1;
	LOG_DBG("eSPI block activated successfully\n");

	return 0;
}

static bool espi_xec_channel_ready(struct device *dev, enum espi_channel ch)
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
		sts = ESPI_CAP_REGS->FC_RDY & MCHP_ESPI_PC_READY;
		break;
	default:
		sts = false;
		break;
	}

	return sts;
}

static int espi_xec_send_vwire(struct device *dev,
			       enum espi_vwire_signal signal, u8_t level)
{
	struct xec_signal signal_info = vw_tbl[signal];
	u8_t xec_id = signal_info.xec_reg_idx;
	u8_t src_id = signal_info.bit;

	if ((src_id >= ESPI_VWIRE_SRC_ID_MAX) ||
	    (xec_id >= ESPI_MSVW_IDX_MAX)) {
		return -EINVAL;
	}

	if (signal_info.dir == ESPI_MASTER_TO_SLAVE) {
		ESPI_MSVW_REG *reg = &(ESPI_M2S_VW_REGS->MSVW00) + xec_id;
		u8_t *p8 = (u8_t *)&reg->SRC;

		*(p8 + (uintptr_t) src_id) = level;
	}

	if (signal_info.dir == ESPI_SLAVE_TO_MASTER) {
		ESPI_SMVW_REG *reg = &(ESPI_S2M_VW_REGS->SMVW00) + xec_id;
		u8_t *p8 = (u8_t *)&reg->SRC;

		*(p8 + (uintptr_t) src_id) = level;

		/* Ensure eSPI virtual wire packet is transmitted
		 * There is no interrupt, so need to poll register
		 */
		u8_t rd_cnt = ESPI_XEC_VWIRE_SEND_TIMEOUT;

		while (reg->SRC_CHG && rd_cnt--) {
			k_busy_wait(100);
		}
	}

	return 0;
}

static int espi_xec_receive_vwire(struct device *dev,
				  enum espi_vwire_signal signal, u8_t *level)
{
	struct xec_signal signal_info = vw_tbl[signal];
	u8_t xec_id = signal_info.xec_reg_idx;
	u8_t src_id = signal_info.bit;

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

static int espi_xec_manage_callback(struct device *dev,
				    struct espi_callback *callback, bool set)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->driver_data);

	return espi_manage_callback(&data->callbacks, callback, set);
}

static void send_slave_bootdone(struct device *dev)
{
	int ret;
	u8_t boot_done;

	ret = espi_xec_receive_vwire(dev, ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE,
				     &boot_done);
	if (!ret && !boot_done) {
		/* SLAVE_BOOT_DONE & SLAVE_LOAD_STS have to be sent together */
		ESPI_S2M_VW_REGS->SMVW01.SRC = 0x01000001;
	}
}

#ifdef CONFIG_ESPI_OOB_CHANNEL
static void espi_init_oob(struct device *dev)
{
	struct espi_xec_config *config =
		(struct espi_xec_config *) (dev->config->config_info);

	MCHP_GIRQ_ENSET(config->bus_girq_id) =
		BIT(MCHP_ESPI_OOB_UP_GIRQ_POS) | BIT(MCHP_ESPI_OOB_DN_GIRQ_POS);
}
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
static void espi_init_flash(struct device *dev)
{
	struct espi_xec_config *config =
	    (struct espi_xec_config *)(dev->config->config_info);

	MCHP_GIRQ_ENSET(config->bus_girq_id) = BIT(MCHP_ESPI_FC_GIRQ_POS);
}
#endif

static void espi_rst_isr(struct device *dev)
{
	u8_t rst_sts;
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->driver_data);
	struct espi_event evt = { ESPI_BUS_RESET, 0, 0 };

	rst_sts = ESPI_CAP_REGS->ERST_STS;

	/* eSPI reset status register is clear on write register */
	ESPI_CAP_REGS->ERST_STS |= MCHP_ESPI_RST_ISTS;

	if (rst_sts & MCHP_ESPI_RST_ISTS) {
		if (rst_sts & ~MCHP_ESPI_RST_ISTS_PIN_RO_HI) {
			data->espi_rst_asserted = 1;
		} else {
			data->espi_rst_asserted = 0;
		}

		evt.evt_data = data->espi_rst_asserted;
		espi_send_callbacks(&data->callbacks, dev, evt);
#ifdef CONFIG_ESPI_OOB_CHANNEL
		espi_init_oob(dev);
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
		espi_init_flash(dev);
#endif
	}
}

/* Configure sub devices BAR address if not using default I/O based address
 * then make its BAR valid.
 * Refer to microchip eSPI I/O base addresses for default values
 */
static void config_sub_devices(struct device *dev)
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
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KEYBOARD
	KBC_REGS->KBC_CTRL |= MCHP_KBC_CTRL_AUXH;
	KBC_REGS->KBC_PORT92_EN = MCHP_KBC_PORT92_EN;
	ESPI_EIO_BAR_REGS->EC_BAR_KBC = ESPI_XEC_KBC_BAR_ADDRESS |
		MCHP_ESPI_IO_BAR_HOST_VALID;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
	ESPI_EIO_BAR_REGS->EC_BAR_ACPI_EC_0 |= MCHP_ESPI_IO_BAR_HOST_VALID;
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
	ESPI_SIRQ_REGS->UART_1_SIRQ = 0x04;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KEYBOARD
	ESPI_SIRQ_REGS->KBC_SIRQ_0 = 0x01;
	ESPI_SIRQ_REGS->KBC_SIRQ_1 = 0x0C;
#endif
}

static void setup_espi_io_config(struct device *dev, u16_t host_address)
{
	ESPI_EIO_BAR_REGS->EC_BAR_IOC = (host_address << 16) |
		MCHP_ESPI_IO_BAR_HOST_VALID;

	config_sub_devices(dev);
	configure_sirq();

	ESPI_PC_REGS->PC_STATUS |= (MCHP_ESPI_PC_STS_EN_CHG |
				    MCHP_ESPI_PC_STS_BM_EN_CHG_POS);
	ESPI_PC_REGS->PC_IEN |= MCHP_ESPI_PC_IEN_EN_CHG;
	ESPI_CAP_REGS->PC_RDY = 1;
}

static void espi_pc_isr(struct device *dev)
{
	u32_t status = ESPI_PC_REGS->PC_STATUS;

	if (status & MCHP_ESPI_PC_STS_EN_CHG) {
		if (status & MCHP_ESPI_PC_STS_EN) {
			setup_espi_io_config(dev, MCHP_ESPI_IOBAR_INIT_DFLT);
		}

		ESPI_PC_REGS->PC_STATUS = MCHP_ESPI_PC_STS_EN_CHG;
	}

	GIRQ19_REGS->SRC = MCHP_ESPI_PC_GIRQ_VAL;
}

static void espi_vwire_chanel_isr(struct device *dev)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->driver_data);
	const struct espi_xec_config *config = dev->config->config_info;
	struct espi_event evt = { ESPI_BUS_EVENT_CHANNEL_READY, 0, 0 };
	u32_t status;

	status = ESPI_IO_VW_REGS->VW_EN_STS;
	GIRQ19_REGS->SRC = MCHP_ESPI_VW_EN_GIRQ_VAL;

	if (status & MCHP_ESPI_VW_EN_STS_RO) {
		ESPI_IO_VW_REGS->VW_RDY = 1;

		/* VW channel interrupt can disabled at this point */
		MCHP_GIRQ_ENCLR(config->bus_girq_id) = MCHP_ESPI_VW_EN_GIRQ_VAL;
		send_slave_bootdone(dev);
	}

	evt.evt_details = ESPI_CHANNEL_VWIRE;
	evt.evt_data = status;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void espi_oob_down_isr(struct device *dev)
{
	u32_t status;

	status = ESPI_OOB_REGS->RX_STS;
	if (status & MCHP_ESPI_OOB_RX_STS_DONE) {
		ESPI_OOB_REGS->RX_IEN = ~MCHP_ESPI_OOB_RX_IEN;
	}

	GIRQ19_REGS->SRC = MCHP_ESPI_OOB_DN_GIRQ_VAL;
}

static void espi_oob_up_isr(struct device *dev)
{
	u32_t status;

	status = ESPI_OOB_REGS->TX_STS;
	if (status & MCHP_ESPI_OOB_TX_STS_DONE) {
		ESPI_OOB_REGS->TX_IEN = ~MCHP_ESPI_OOB_TX_IEN_DONE;
	}

	GIRQ19_REGS->SRC = MCHP_ESPI_OOB_UP_GIRQ_VAL;
}

static void espi_flash_isr(struct device *dev)
{
	u32_t status;

	status = ESPI_FC_REGS->STS;
	if (status & MCHP_ESPI_FC_STS_DONE) {
		ESPI_FC_REGS->IEN = ~BIT(0);
	}

	GIRQ19_REGS->SRC = MCHP_ESPI_FC_GIRQ_VAL;
}

static void vw_pltrst_isr(struct device *dev)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->driver_data);
	struct espi_event evt = { ESPI_BUS_EVENT_VWIRE_RECEIVED,
		ESPI_VWIRE_SIGNAL_PLTRST, 0
	};
	u8_t status = 0;

	espi_xec_receive_vwire(dev, ESPI_VWIRE_SIGNAL_PLTRST, &status);
	if (status) {
		setup_espi_io_config(dev, MCHP_ESPI_IOBAR_INIT_DFLT);
	}

	/* PLT_RST will be received several times */
	if (status != data->plt_rst_asserted) {
		data->plt_rst_asserted = status;
		evt.evt_data = status;
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}

/* Send callbacks if enabled and track eSPI host system state */
static void notify_system_state(struct device *dev,
				enum espi_vwire_signal signal)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->driver_data);
	struct espi_event evt = { ESPI_BUS_EVENT_VWIRE_RECEIVED, 0, 0 };
	u8_t status = 0;

	espi_xec_receive_vwire(dev, signal, &status);
	if (!status) {
		data->sx_state = signal;
	}

	evt.evt_details = signal;
	evt.evt_data = status;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void vw_slp3_isr(struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SLP_S3);
}

static void vw_slp4_isr(struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SLP_S4);
}

static void vw_slp5_isr(struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SLP_S5);
}

static void vw_host_rst_warn_isr(struct device *dev)
{
	u8_t status;

	espi_xec_receive_vwire(dev, ESPI_VWIRE_SIGNAL_HOST_RST_WARN, &status);
	k_busy_wait(ESPI_XEC_VWIRE_ACK_DELAY);
	espi_xec_send_vwire(dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, status);
}

static void vw_sus_warn_isr(struct device *dev)
{
	u8_t status;

	espi_xec_receive_vwire(dev, ESPI_VWIRE_SIGNAL_SUS_WARN, &status);
	k_busy_wait(ESPI_XEC_VWIRE_ACK_DELAY);
	espi_xec_send_vwire(dev, ESPI_VWIRE_SIGNAL_SUS_ACK, status);
}

static void ibf_isr(struct device *dev)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->driver_data);
	struct espi_event evt = { ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_HOST_IO, ESPI_PERIPHERAL_NODATA
	};

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void port80_isr(struct device *dev)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->driver_data);
	struct espi_event evt = { ESPI_BUS_PERIPHERAL_NOTIFICATION,
		(ESPI_PERIPHERAL_INDEX_0 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,
		ESPI_PERIPHERAL_NODATA
	};

	evt.evt_data = PORT80_CAP0_REGS->EC_DATA;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void port81_isr(struct device *dev)
{
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->driver_data);
	struct espi_event evt = { ESPI_BUS_PERIPHERAL_NOTIFICATION,
		(ESPI_PERIPHERAL_INDEX_1 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,
		ESPI_PERIPHERAL_NODATA
	};

	evt.evt_data = PORT80_CAP1_REGS->EC_DATA;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

const struct espi_isr espi_bus_isr[] = {
	{MCHP_ESPI_PC_GIRQ_VAL, espi_pc_isr},
	{MCHP_ESPI_OOB_UP_GIRQ_VAL, espi_oob_up_isr},
	{MCHP_ESPI_OOB_DN_GIRQ_VAL, espi_oob_down_isr},
	{MCHP_ESPI_FC_GIRQ_VAL, espi_flash_isr},
	{MCHP_ESPI_ESPI_RST_GIRQ_VAL, espi_rst_isr},
	{MCHP_ESPI_VW_EN_GIRQ_VAL, espi_vwire_chanel_isr},
};

const struct espi_isr m2s_vwires_isr[] = {
	{MEC_ESPI_MSVW00_SRC0_VAL, vw_slp3_isr},
	{MEC_ESPI_MSVW00_SRC1_VAL, vw_slp4_isr},
	{MEC_ESPI_MSVW00_SRC2_VAL, vw_slp5_isr},
	{MEC_ESPI_MSVW01_SRC1_VAL, vw_pltrst_isr},
	{MEC_ESPI_MSVW02_SRC0_VAL, vw_host_rst_warn_isr},
	{MEC_ESPI_MSVW03_SRC0_VAL, vw_sus_warn_isr},
};

const struct espi_isr peripherals_isr[] = {
	{MCHP_ACPI_EC_0_IBF_GIRQ, ibf_isr},
	{MCHP_PORT80_DEBUG0_GIRQ_VAL, port80_isr},
	{MCHP_PORT80_DEBUG1_GIRQ_VAL, port81_isr},
};

static u8_t bus_isr_cnt = sizeof(espi_bus_isr) / sizeof(struct espi_isr);
static u8_t m2s_vwires_isr_cnt =
	sizeof(m2s_vwires_isr) / sizeof(struct espi_isr);
static u8_t periph_isr_cnt = sizeof(peripherals_isr) / sizeof(struct espi_isr);

static void espi_xec_bus_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct espi_xec_config *config = dev->config->config_info;
	u32_t girq_result;

	girq_result = MCHP_GIRQ_RESULT(config->bus_girq_id);
	REG32(MCHP_GIRQ_SRC_ADDR(config->bus_girq_id)) = girq_result;

	for (int i = 0; i < bus_isr_cnt; i++) {
		struct espi_isr entry = espi_bus_isr[i];

		if (girq_result & entry.girq_bit) {
			if (entry.the_isr != NULL) {
				entry.the_isr(dev);
			}
		}
	}
}

static void espi_xec_vw_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct espi_xec_config *config = dev->config->config_info;
	u32_t girq_result;

	girq_result = MCHP_GIRQ_RESULT(config->vw_girq_id);
	REG32(MCHP_GIRQ_SRC_ADDR(config->vw_girq_id)) = girq_result;

	for (int i = 0; i < m2s_vwires_isr_cnt; i++) {
		struct espi_isr entry = m2s_vwires_isr[i];

		if (girq_result & entry.girq_bit) {
			if (entry.the_isr != NULL) {
				entry.the_isr(dev);
			}
		}
	}
}

static void espi_xec_periph_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct espi_xec_config *config = dev->config->config_info;
	u32_t girq_result;

	girq_result = MCHP_GIRQ_RESULT(config->pc_girq_id);
	REG32(MCHP_GIRQ_SRC_ADDR(config->pc_girq_id)) = girq_result;

	for (int i = 0; i < periph_isr_cnt; i++) {
		struct espi_isr entry = peripherals_isr[i];

		if (girq_result & entry.girq_bit) {
			if (entry.the_isr != NULL) {
				entry.the_isr(dev);
			}
		}
	}
}

static int espi_xec_init(struct device *dev);

static const struct espi_driver_api espi_xec_driver_api = {
	.config = espi_xec_configure,
	.get_channel_status = espi_xec_channel_ready,
	.send_vwire = espi_xec_send_vwire,
	.receive_vwire = espi_xec_receive_vwire,
	.manage_callback = espi_xec_manage_callback,
};

static struct espi_xec_data espi_xec_data;

static const struct espi_xec_config espi_xec_config = {
	.base_addr = DT_INST_0_MICROCHIP_XEC_ESPI_BASE_ADDRESS,
	.bus_girq_id = DT_INST_0_MICROCHIP_XEC_ESPI_IO_GIRQ,
	.vw_girq_id = DT_INST_0_MICROCHIP_XEC_ESPI_VW_GIRQ,
	.pc_girq_id = DT_INST_0_MICROCHIP_XEC_ESPI_PC_GIRQ,
};

DEVICE_AND_API_INIT(espi_xec_0, DT_INST_0_MICROCHIP_XEC_ESPI_LABEL,
		    &espi_xec_init, &espi_xec_data, &espi_xec_config,
		    PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY,
		    &espi_xec_driver_api);

static int espi_xec_init(struct device *dev)
{
	const struct espi_xec_config *config = dev->config->config_info;
	struct espi_xec_data *data = (struct espi_xec_data *)(dev->driver_data);

	data->plt_rst_asserted = 0;

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
	ESPI_OOB_REGS->TX_ADDR_MSW = ESPI_XEC_OOB_ADDR_MSW;
	ESPI_OOB_REGS->RX_ADDR_MSW = ESPI_XEC_OOB_ADDR_MSW;
	ESPI_OOB_REGS->TX_ADDR_LSW = ESPI_XEC_OOB_ADDR_LSW;
	ESPI_OOB_REGS->RX_ADDR_LSW = ESPI_XEC_OOB_ADDR_LSW;
	ESPI_OOB_REGS->RX_LEN = (ESPI_XEC_OOB_RX_LEN &
				 ESPI_XEC_OOB_RX_LEN_MASK);
#else
	ESPI_CAP_REGS->GLB_CAP0 &= ~MCHP_ESPI_GBL_CAP0_OOB_SUPP;
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	ESPI_CAP_REGS->GLB_CAP0 |= MCHP_ESPI_GBL_CAP0_FC_SUPP;
	ESPI_CAP_REGS->GLB_CAP0 |= MCHP_ESPI_FC_CAP_MAX_PLD_SZ_64;
	ESPI_CAP_REGS->FC_CAP |= MCHP_ESPI_FC_CAP_SHARE_MAF_SAF;
	ESPI_CAP_REGS->FC_CAP |= MCHP_ESPI_FC_CAP_MAX_RD_SZ_64;
#else
	ESPI_CAP_REGS->GLB_CAP0 &= ~MCHP_ESPI_GBL_CAP0_FC_SUPP;
#endif

	/* Clear reset interrupt status and enable interrupts */
	ESPI_CAP_REGS->ERST_STS |= MCHP_ESPI_RST_ISTS;
	ESPI_CAP_REGS->ERST_IEN |= MCHP_ESPI_RST_IEN;
	ESPI_PC_REGS->PC_STATUS |= MCHP_ESPI_PC_STS_EN_CHG;
	ESPI_PC_REGS->PC_IEN |= MCHP_ESPI_PC_IEN_EN_CHG;

	/* Enable VWires interrupts */
	mec_espi_msvw_irq_sel_set(&ESPI_M2S_VW_REGS->MSVW00,
				  ESPI_VWIRE_SRC_ID0, MSVW_IRQ_SEL_EDGE_BOTH);
	mec_espi_msvw_irq_sel_set(&ESPI_M2S_VW_REGS->MSVW00,
				  ESPI_VWIRE_SRC_ID1, MSVW_IRQ_SEL_EDGE_BOTH);
	mec_espi_msvw_irq_sel_set(&ESPI_M2S_VW_REGS->MSVW00,
				  ESPI_VWIRE_SRC_ID2, MSVW_IRQ_SEL_EDGE_BOTH);
	mec_espi_msvw_irq_sel_set(&ESPI_M2S_VW_REGS->MSVW01,
				  ESPI_VWIRE_SRC_ID0, MSVW_IRQ_SEL_EDGE_BOTH);
	mec_espi_msvw_irq_sel_set(&ESPI_M2S_VW_REGS->MSVW01,
				  ESPI_VWIRE_SRC_ID1, MSVW_IRQ_SEL_EDGE_BOTH);
	mec_espi_msvw_irq_sel_set(&ESPI_M2S_VW_REGS->MSVW01,
				  ESPI_VWIRE_SRC_ID2, MSVW_IRQ_SEL_EDGE_BOTH);
	mec_espi_msvw_irq_sel_set(&ESPI_M2S_VW_REGS->MSVW02,
				  ESPI_VWIRE_SRC_ID0, MSVW_IRQ_SEL_EDGE_BOTH);
	mec_espi_msvw_irq_sel_set(&ESPI_M2S_VW_REGS->MSVW03,
				  ESPI_VWIRE_SRC_ID0, MSVW_IRQ_SEL_EDGE_BOTH);
	mec_espi_msvw_irq_sel_set(&ESPI_M2S_VW_REGS->MSVW03,
				  ESPI_VWIRE_SRC_ID1, MSVW_IRQ_SEL_EDGE_BOTH);
	mec_espi_msvw_irq_sel_set(&ESPI_M2S_VW_REGS->MSVW03,
				  ESPI_VWIRE_SRC_ID3, MSVW_IRQ_SEL_EDGE_BOTH);

	/* Enable interrupts for each logical channel enable assertion */
	MCHP_GIRQ_ENSET(config->bus_girq_id) = MCHP_ESPI_ESPI_RST_GIRQ_VAL |
		MCHP_ESPI_VW_EN_GIRQ_VAL | MCHP_ESPI_PC_GIRQ_VAL;

	/* Enable aggregated block interrupts for VWires */
	MCHP_GIRQ_ENSET(config->vw_girq_id) = MEC_ESPI_MSVW00_SRC0_VAL |
		MEC_ESPI_MSVW00_SRC1_VAL | MEC_ESPI_MSVW00_SRC2_VAL |
		MEC_ESPI_MSVW01_SRC1_VAL | MEC_ESPI_MSVW02_SRC0_VAL |
		MEC_ESPI_MSVW03_SRC0_VAL;

	/* Enable aggregated block interrupts for peripherals supported */
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KEYBOARD
	MCHP_GIRQ_ENSET(config->pc_girq_id) = MCHP_KBC_IBF_GIRQ;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
	MCHP_GIRQ_ENSET(config->pc_girq_id) = MCHP_ACPI_EC_0_IBF_GIRQ;
	MCHP_GIRQ_ENSET(config->pc_girq_id) = MCHP_ACPI_EC_1_IBF_GIRQ;
	MCHP_GIRQ_ENSET(config->pc_girq_id) = MCHP_ACPI_EC_2_IBF_GIRQ;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	MCHP_GIRQ_ENSET(config->pc_girq_id) = MCHP_PORT80_DEBUG0_GIRQ_VAL |
		MCHP_PORT80_DEBUG1_GIRQ_VAL;
#endif
	/* Enable aggregated interrupt block for eSPI bus events */
	MCHP_GIRQ_BLK_SETEN(config->bus_girq_id);
	IRQ_CONNECT(DT_INST_0_MICROCHIP_XEC_ESPI_IRQ_0,
		    CONFIG_ESPI_INIT_PRIORITY, espi_xec_bus_isr,
		    DEVICE_GET(espi_xec_0), 0);
	irq_enable(DT_INST_0_MICROCHIP_XEC_ESPI_IRQ_0);

	/* Enable aggregated interrupt block for eSPI VWire events */
	MCHP_GIRQ_BLK_SETEN(config->vw_girq_id);
	IRQ_CONNECT(DT_INST_0_MICROCHIP_XEC_ESPI_IRQ_1,
		    CONFIG_ESPI_INIT_PRIORITY, espi_xec_vw_isr,
		    DEVICE_GET(espi_xec_0), 0);
	irq_enable(DT_INST_0_MICROCHIP_XEC_ESPI_IRQ_1);

	/* Enable aggregated interrupt block for eSPI peripheral channel */
	MCHP_GIRQ_BLK_SETEN(config->pc_girq_id);
	IRQ_CONNECT(DT_INST_0_MICROCHIP_XEC_ESPI_IRQ_2,
		    CONFIG_ESPI_INIT_PRIORITY, espi_xec_periph_isr,
		    DEVICE_GET(espi_xec_0), 0);
	irq_enable(DT_INST_0_MICROCHIP_XEC_ESPI_IRQ_2);
	return 0;
}
