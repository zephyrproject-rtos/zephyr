/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_espi

#include <assert.h>
#include <drivers/espi.h>
#include <drivers/gpio.h>
#include <drivers/clock_control.h>
#include <dt-bindings/espi/npcx_espi.h>
#include <kernel.h>
#include <soc.h>
#include "espi_utils.h"
#include "soc_host.h"
#include "soc_miwu.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(espi, CONFIG_ESPI_LOG_LEVEL);

struct espi_npcx_config {
	uintptr_t base;
	/* clock configuration */
	struct npcx_clk_cfg clk_cfg;
	/* mapping table between eSPI reset signal and wake-up input */
	struct npcx_wui espi_rst_wui;
	/* pinmux configuration */
	const uint8_t alts_size;
	const struct npcx_alt *alts_list;
};

struct espi_npcx_data {
	sys_slist_t callbacks;
	uint8_t plt_rst_asserted;
	uint8_t espi_rst_asserted;
	uint8_t sx_state;
#if defined(CONFIG_ESPI_OOB_CHANNEL)
	struct k_sem oob_rx_lock;
#endif
};

/* Driver convenience defines */
#define DRV_CONFIG(dev) ((const struct espi_npcx_config *)(dev)->config)

#define DRV_DATA(dev) ((struct espi_npcx_data *)(dev)->data)

#define HAL_INSTANCE(dev) (struct espi_reg *)(DRV_CONFIG(dev)->base)

/* eSPI channels */
#define NPCX_ESPI_CH_PC              0
#define NPCX_ESPI_CH_VW              1
#define NPCX_ESPI_CH_OOB             2
#define NPCX_ESPI_CH_FLASH           3
#define NPCX_ESPI_CH_COUNT           4
#define NPCX_ESPI_HOST_CH_EN(ch)     (ch + 4)

/* eSPI max supported frequency */
#define NPCX_ESPI_MAXFREQ_20         0
#define NPCX_ESPI_MAXFREQ_25         1
#define NPCX_ESPI_MAXFREQ_33         2
#define NPCX_ESPI_MAXFREQ_50         3

/* Minimum delay before acknowledging a virtual wire */
#define NPCX_ESPI_VWIRE_ACK_DELAY    10ul /* 10 us */

/* OOB channel maximum payload size */
#define NPCX_ESPI_OOB_MAX_PAYLOAD    64
#define NPCX_OOB_RX_PACKAGE_LEN(hdr) (((hdr & 0xff000000) >> 24) | \
							((hdr & 0xf0000) >> 8))

/* eSPI cycle type field for OOB and FLASH channels */
#define ESPI_FLASH_READ_CYCLE_TYPE   0x00
#define ESPI_FLASH_WRITE_CYCLE_TYPE  0x01
#define ESPI_FLASH_ERASE_CYCLE_TYPE  0x02
#define ESPI_OOB_GET_CYCLE_TYPE      0x21
#define ESPI_OOB_TAG                 0x00
#define ESPI_OOB_MAX_TIMEOUT         500ul /* 500 ms */

/* eSPI bus interrupt configuration structure and macro function */
struct espi_bus_isr {
	uint8_t status_bit; /* bit order in ESPISTS register */
	uint8_t int_en_bit; /* bit order in ESPIIE register */
	uint8_t wake_en_bit; /* bit order in ESPIWE register */
	void (*bus_isr)(const struct device *dev); /* eSPI bus ISR */
};

#define NPCX_ESPI_BUS_INT_ITEM(event, isr) {     \
	.status_bit = NPCX_ESPISTS_##event,      \
	.int_en_bit = NPCX_ESPIIE_##event##IE,   \
	.wake_en_bit = NPCX_ESPIWE_##event##WE,  \
	.bus_isr = isr }

/* eSPI Virtual Wire Input (Master-to-Slave) signals configuration structure */
struct npcx_vw_in_config {
	enum espi_vwire_signal sig; /* Virtual Wire signal */
	uint8_t  reg_idx; /* register index for VW signal */
	uint8_t  bitmask; /* VW signal bits-mask */
	struct npcx_wui vw_wui; /* WUI mapping in MIWU modules for VW signal */
};

/* eSPI Virtual Wire Output (Slave-to-Master) signals configuration structure */
struct npcx_vw_out_config {
	enum espi_vwire_signal sig; /* Virtual Wire signal */
	uint8_t  reg_idx; /* register index for VW signal */
	uint8_t  bitmask; /* VW signal bits-mask */
};

/*
 * eSPI VW input/Output signal configuration tables. Please refer
 * npcxn-espi-vws-map.dtsi device tree file for more detail.
 */
static const struct npcx_vw_in_config vw_in_tbl[] = {
	/* index 02h (In)  */
	NPCX_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SLP_S3, vw_slp_s3),
	NPCX_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SLP_S4, vw_slp_s4),
	NPCX_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SLP_S5, vw_slp_s5),
	/* index 03h (In)  */
	NPCX_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SUS_STAT, vw_sus_stat),
	NPCX_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_PLTRST, vw_plt_rst),
	NPCX_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_OOB_RST_WARN, vw_oob_rst_warn),
	/* index 07h (In)  */
	NPCX_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_HOST_RST_WARN, vw_host_rst_warn),
	/* index 41h (In)  */
	NPCX_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SUS_WARN, vw_sus_warn),
	NPCX_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK, vw_sus_pwrdn_ack),
	NPCX_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SLP_A, vw_slp_a),
	/* index 42h (In)  */
	NPCX_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SLP_LAN, vw_slp_lan),
	NPCX_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SLP_WLAN, vw_slp_wlan),
};

static const struct npcx_vw_out_config vw_out_tbl[] = {
	/* index 04h (Out) */
	NPCX_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_OOB_RST_ACK, vw_oob_rst_ack),
	NPCX_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_WAKE, vw_wake),
	NPCX_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_PME, vw_pme),
	/* index 05h (Out) */
	NPCX_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE, vw_slv_boot_done),
	NPCX_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_ERR_FATAL, vw_err_fatal),
	NPCX_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_ERR_NON_FATAL, vw_err_non_fatal),
	NPCX_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_SLV_BOOT_STS,
						vw_slv_boot_sts_with_done),
	/* index 06h (Out) */
	NPCX_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_SCI, vw_sci),
	NPCX_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_SMI, vw_smi),
	NPCX_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_HOST_RST_ACK, vw_host_rst_ack),
	/* index 40h (Out) */
	NPCX_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_SUS_ACK, vw_sus_ack),
};

/* Callbacks for eSPI bus reset and Virtual Wire signals. */
static struct miwu_dev_callback espi_rst_callback;
static struct miwu_dev_callback vw_in_callback[ARRAY_SIZE(vw_in_tbl)];

/* eSPI VW service function forward declarations */
static int espi_npcx_receive_vwire(const struct device *dev,
				enum espi_vwire_signal signal, uint8_t *level);
static int espi_npcx_send_vwire(const struct device *dev,
			enum espi_vwire_signal signal, uint8_t level);
static void espi_vw_send_bootload_done(const struct device *dev);

/* eSPI local initialization functions */
static void espi_init_wui_callback(const struct device *dev,
		struct miwu_dev_callback *callback, const struct npcx_wui *wui,
		miwu_dev_callback_handler_t handler)
{
	/* VW signal which has no wake-up input source */
	if (wui->table == NPCX_MIWU_TABLE_NONE)
		return;

	/* Install callback function */
	npcx_miwu_init_dev_callback(callback, wui, handler, dev);
	npcx_miwu_manage_dev_callback(callback, 1);

	/* Congiure MIWU setting and enable its interrupt */
	npcx_miwu_interrupt_configure(wui, NPCX_MIWU_MODE_EDGE,
							NPCX_MIWU_TRIG_BOTH);
	npcx_miwu_irq_enable(wui);
}

/* eSPI local bus interrupt service functions */
static void espi_bus_err_isr(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint32_t err = inst->ESPIERR;

	LOG_ERR("eSPI Bus Error %08X", err);
	/* Clear error status bits */
	inst->ESPIERR = err;
}

static void espi_bus_inband_rst_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	LOG_DBG("%s issued", __func__);
}

static void espi_bus_reset_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	LOG_DBG("%s issued", __func__);
	/* Do nothing! This signal is handled in ESPI_RST VW signal ISR */
}

static void espi_bus_cfg_update_isr(const struct device *dev)
{
	int chan;
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_npcx_data *const data = DRV_DATA(dev);
	struct espi_event evt = { .evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				  .evt_details = 0,
				  .evt_data = 0 };
	/* If host enable bits are not sync with ready bits on slave side. */
	uint8_t chg_mask = GET_FIELD(inst->ESPICFG, NPCX_ESPICFG_HCHANS_FIELD)
			 ^ GET_FIELD(inst->ESPICFG, NPCX_ESPICFG_CHANS_FIELD);
	chg_mask &= (ESPI_CHANNEL_VWIRE | ESPI_CHANNEL_OOB |
							ESPI_CHANNEL_FLASH);

	LOG_DBG("ESPI CFG Change Updated! 0x%02X", chg_mask);
	/*
	 * If host enable/disable channel for VW/OOB/FLASH, EC should follow
	 * except Peripheral channel. It is handled after receiving PLTRST
	 * event separately.
	 */
	for (chan = NPCX_ESPI_CH_VW; chan < NPCX_ESPI_CH_COUNT; chan++) {
		/* Channel ready bit isn't sync with enabled bit on host side */
		if (chg_mask & BIT(chan)) {
			evt.evt_data = IS_BIT_SET(inst->ESPICFG,
						NPCX_ESPI_HOST_CH_EN(chan));
			evt.evt_details = BIT(chan);

			if (evt.evt_data)
				inst->ESPICFG |= BIT(chan);
			else
				inst->ESPICFG &= ~BIT(chan);

			espi_send_callbacks(&data->callbacks, dev, evt);
		}
	}
	LOG_DBG("ESPI CFG EC Updated! 0x%02X", GET_FIELD(inst->ESPICFG,
						NPCX_ESPICFG_CHANS_FIELD));

	/* If VW channel is enabled and ready, send bootload done VW signal */
	if ((chg_mask & BIT(NPCX_ESPI_CH_VW)) && IS_BIT_SET(inst->ESPICFG,
				NPCX_ESPI_HOST_CH_EN(NPCX_ESPI_CH_VW))) {
		espi_vw_send_bootload_done(dev);
	}
}

#if defined(CONFIG_ESPI_OOB_CHANNEL)
static void espi_bus_oob_rx_isr(const struct device *dev)
{
	struct espi_npcx_data *const data = DRV_DATA(dev);

	LOG_DBG("%s", __func__);
	k_sem_give(&data->oob_rx_lock);
}
#endif

const struct espi_bus_isr espi_bus_isr_tbl[] = {
	NPCX_ESPI_BUS_INT_ITEM(BERR, espi_bus_err_isr),
	NPCX_ESPI_BUS_INT_ITEM(IBRST, espi_bus_inband_rst_isr),
	NPCX_ESPI_BUS_INT_ITEM(ESPIRST, espi_bus_reset_isr),
	NPCX_ESPI_BUS_INT_ITEM(CFGUPD, espi_bus_cfg_update_isr),
#if defined(CONFIG_ESPI_OOB_CHANNEL)
	NPCX_ESPI_BUS_INT_ITEM(OOBRX, espi_bus_oob_rx_isr),
#endif
};

static void espi_bus_generic_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	int i;
	uint32_t mask, status;

	/*
	 * Bit 17 of ESPIIE is reserved. We need to set the same bit in mask
	 * in case bit 17 in ESPISTS of npcx7 is not cleared in ISR.
	 */
	mask = inst->ESPIIE | (1 << NPCX_ESPISTS_VWUPDW);
	status = inst->ESPISTS & mask;

	/* Clear pending bits of status register first */
	inst->ESPISTS = status;

	LOG_DBG("%s: 0x%08X", __func__, status);
	for (i = 0; i < ARRAY_SIZE(espi_bus_isr_tbl); i++) {
		struct espi_bus_isr entry = espi_bus_isr_tbl[i];

		if (status & BIT(entry.status_bit)) {
			if (entry.bus_isr != NULL) {
				entry.bus_isr(dev);
			}
		}
	}
}

/* eSPI local virtual-wire service functions */
static void espi_vw_config_input(const struct device *dev,
				const struct npcx_vw_in_config *config_in)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	int idx = config_in->reg_idx;

	/* IE & WE bits are already set? */
	if (IS_BIT_SET(inst->VWEVMS[idx], NPCX_VWEVMS_IE) &&
		IS_BIT_SET(inst->VWEVMS[idx], NPCX_VWEVMS_WE))
		return;

	/* Set IE & WE bits in VWEVMS */
	inst->VWEVMS[idx] |= BIT(NPCX_VWEVMS_IE) | BIT(NPCX_VWEVMS_WE);
	LOG_DBG("VWEVMS%d 0x%08X", idx, inst->VWEVMS[idx]);
}

static void espi_vw_config_output(const struct device *dev,
				const struct npcx_vw_out_config *config_out)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	int idx = config_out->reg_idx;
	uint8_t valid = GET_FIELD(inst->VWEVSM[idx], NPCX_VWEVSM_VALID);

	/* Set valid bits for vw signal which we have declared in table. */
	valid |= config_out->bitmask;
	SET_FIELD(inst->VWEVSM[idx], NPCX_VWEVSM_VALID, valid);

	/*
	 * Turn off hardware-wire feature which generates VW events that
	 * connected to hardware signals. We will set it manually by software.
	 */
	SET_FIELD(inst->VWEVSM[idx], NPCX_VWEVSM_HW_WIRE, 0);

	LOG_DBG("VWEVSM%d 0x%08X", idx, inst->VWEVSM[idx]);
}

static void espi_vw_notify_system_state(const struct device *dev,
				enum espi_vwire_signal signal)
{
	struct espi_npcx_data *const data = DRV_DATA(dev);
	struct espi_event evt = { ESPI_BUS_EVENT_VWIRE_RECEIVED, 0, 0 };
	uint8_t wire = 0;

	espi_npcx_receive_vwire(dev, signal, &wire);
	if (!wire) {
		data->sx_state = signal;
	}

	evt.evt_details = signal;
	evt.evt_data = wire;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void espi_vw_notify_host_warning(const struct device *dev,
				enum espi_vwire_signal signal)
{
	uint8_t wire;

	espi_npcx_receive_vwire(dev, signal, &wire);

	k_busy_wait(NPCX_ESPI_VWIRE_ACK_DELAY);
	switch (signal) {
	case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
		espi_npcx_send_vwire(dev,
				ESPI_VWIRE_SIGNAL_HOST_RST_ACK,
				wire);
		break;
	case ESPI_VWIRE_SIGNAL_SUS_WARN:
		espi_npcx_send_vwire(dev, ESPI_VWIRE_SIGNAL_SUS_ACK,
				wire);
		break;
	case ESPI_VWIRE_SIGNAL_OOB_RST_WARN:
		espi_npcx_send_vwire(dev, ESPI_VWIRE_SIGNAL_OOB_RST_ACK,
				wire);
		break;
	default:
		break;
	}
}

static void espi_vw_notify_plt_rst(const struct device *dev)
{
	struct espi_npcx_data *const data = DRV_DATA(dev);
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_event evt = { ESPI_BUS_EVENT_VWIRE_RECEIVED,
		ESPI_VWIRE_SIGNAL_PLTRST, 0
	};
	uint8_t wire = 0;

	espi_npcx_receive_vwire(dev, ESPI_VWIRE_SIGNAL_PLTRST, &wire);
	LOG_DBG("VW_PLT_RST is %d!", wire);
	if (wire) {
		/* Set Peripheral Channel ready when PLTRST is de-asserted */
		inst->ESPICFG |= BIT(NPCX_ESPICFG_PCHANEN);
		/* Configure all host sub-modules in host doamin */
		npcx_host_init_subs_host_domain();
	}

	/* PLT_RST will be received several times */
	if (wire != data->plt_rst_asserted) {
		data->plt_rst_asserted = wire;
		evt.evt_data = wire;
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}

static void espi_vw_send_bootload_done(const struct device *dev)
{
	int ret;
	uint8_t boot_done;

	ret = espi_npcx_receive_vwire(dev,
			ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE, &boot_done);
	LOG_DBG("%s: %d", __func__, boot_done);
	if (!ret && !boot_done) {
		/* Send slave boot status bit with done bit at the same time. */
		espi_npcx_send_vwire(dev, ESPI_VWIRE_SIGNAL_SLV_BOOT_STS, 1);
	}
}

static void espi_vw_generic_isr(const struct device *dev, struct npcx_wui *wui)
{
	int idx;
	enum espi_vwire_signal signal;

	LOG_DBG("%s: WUI %d %d %d", __func__, wui->table, wui->group, wui->bit);
	for (idx = 0; idx < ARRAY_SIZE(vw_in_tbl); idx++) {
		if (wui->table == vw_in_tbl[idx].vw_wui.table &&
			wui->group == vw_in_tbl[idx].vw_wui.group &&
			wui->bit == vw_in_tbl[idx].vw_wui.bit) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(vw_in_tbl)) {
		LOG_ERR("Unknown VW event! %d %d %d", wui->table,
				wui->group, wui->bit);
		return;
	}

	signal = vw_in_tbl[idx].sig;
	if (signal == ESPI_VWIRE_SIGNAL_SLP_S3
		|| signal == ESPI_VWIRE_SIGNAL_SLP_S4
		|| signal == ESPI_VWIRE_SIGNAL_SLP_S5
		|| signal == ESPI_VWIRE_SIGNAL_SLP_A) {
		espi_vw_notify_system_state(dev, signal);
	} else if (signal == ESPI_VWIRE_SIGNAL_HOST_RST_WARN
		|| signal == ESPI_VWIRE_SIGNAL_SUS_WARN
		|| signal == ESPI_VWIRE_SIGNAL_OOB_RST_WARN) {
		espi_vw_notify_host_warning(dev, signal);
	} else if (signal == ESPI_VWIRE_SIGNAL_PLTRST) {
		espi_vw_notify_plt_rst(dev);
	}
}

static void espi_vw_espi_rst_isr(const struct device *dev, struct npcx_wui *wui)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_npcx_data *const data = DRV_DATA(dev);
	struct espi_event evt = { ESPI_BUS_RESET, 0, 0 };

	data->espi_rst_asserted = !IS_BIT_SET(inst->ESPISTS,
						NPCX_ESPISTS_ESPIRST_LVL);
	LOG_DBG("eSPI RST asserted is %d!", data->espi_rst_asserted);

	evt.evt_data = data->espi_rst_asserted;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

/* eSPI api functions */
static int espi_npcx_configure(const struct device *dev, struct espi_cfg *cfg)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);

	uint8_t max_freq = 0;
	uint8_t cur_io_mode, io_mode = 0;

	/* Configure eSPI frequency */
	switch (cfg->max_freq) {
	case 20:
		max_freq = NPCX_ESPI_MAXFREQ_20;
		break;
	case 25:
		max_freq = NPCX_ESPI_MAXFREQ_25;
		break;
	case 33:
		max_freq = NPCX_ESPI_MAXFREQ_33;
		break;
	case 50:
		max_freq = NPCX_ESPI_MAXFREQ_50;
		break;
	default:
		return -EINVAL;
	}
	SET_FIELD(inst->ESPICFG, NPCX_ESPICFG_MAXFREQ_FIELD, max_freq);

	/* Configure eSPI IO mode */
	io_mode = (cfg->io_caps >> 1);
	if (io_mode > 3) {
		return -EINVAL;
	}

	cur_io_mode = GET_FIELD(inst->ESPICFG, NPCX_ESPICFG_IOMODE_FIELD);
	if (io_mode != cur_io_mode) {
		SET_FIELD(inst->ESPICFG, NPCX_ESPICFG_IOMODE_FIELD, io_mode);
	}

	/* Configure eSPI supported channels */
	if (cfg->channel_caps & ESPI_CHANNEL_PERIPHERAL)
		inst->ESPICFG |= BIT(NPCX_ESPICFG_PCCHN_SUPP);

	if (cfg->channel_caps & ESPI_CHANNEL_VWIRE)
		inst->ESPICFG |= BIT(NPCX_ESPICFG_VWCHN_SUPP);

	if (cfg->channel_caps & ESPI_CHANNEL_OOB)
		inst->ESPICFG |= BIT(NPCX_ESPICFG_OOBCHN_SUPP);

	if (cfg->channel_caps & ESPI_CHANNEL_FLASH)
		inst->ESPICFG |= BIT(NPCX_ESPICFG_FLASHCHN_SUPP);

	LOG_DBG("%s: %d %d ESPICFG: 0x%08X", __func__,
				max_freq, io_mode, inst->ESPICFG);

	return 0;
}

static bool espi_npcx_channel_ready(const struct device *dev,
					enum espi_channel ch)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	bool sts;

	switch (ch) {
	case ESPI_CHANNEL_PERIPHERAL:
		sts = IS_BIT_SET(inst->ESPICFG, NPCX_ESPICFG_PCHANEN);
		break;
	case ESPI_CHANNEL_VWIRE:
		sts = IS_BIT_SET(inst->ESPICFG, NPCX_ESPICFG_VWCHANEN);
		break;
	case ESPI_CHANNEL_OOB:
		sts = IS_BIT_SET(inst->ESPICFG, NPCX_ESPICFG_OOBCHANEN);
		break;
	case ESPI_CHANNEL_FLASH:
		sts = IS_BIT_SET(inst->ESPICFG, NPCX_ESPICFG_FLASHCHANEN);
		break;
	default:
		sts = false;
		break;
	}

	return sts;
}

static int espi_npcx_send_vwire(const struct device *dev,
			enum espi_vwire_signal signal, uint8_t level)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint8_t reg_idx, bitmask, sig_idx, val = 0;

	/* Find signal in VW output table */
	for (sig_idx = 0; sig_idx < ARRAY_SIZE(vw_out_tbl); sig_idx++)
		if (vw_out_tbl[sig_idx].sig == signal)
			break;

	if (sig_idx == ARRAY_SIZE(vw_out_tbl)) {
		LOG_ERR("%s signal %d is invalid", __func__, signal);
		return -EIO;
	}

	reg_idx = vw_out_tbl[sig_idx].reg_idx;
	bitmask = vw_out_tbl[sig_idx].bitmask;

	/* Get wire field and set/clear wire bit */
	val = GET_FIELD(inst->VWEVSM[reg_idx], NPCX_VWEVSM_WIRE);
	if (level)
		val |= bitmask;
	else
		val &= ~bitmask;

	SET_FIELD(inst->VWEVSM[reg_idx], NPCX_VWEVSM_WIRE, val);
	LOG_DBG("Send VW: VWEVSM%d 0x%08X", reg_idx, inst->VWEVSM[reg_idx]);

	return 0;
}

static int espi_npcx_receive_vwire(const struct device *dev,
				  enum espi_vwire_signal signal, uint8_t *level)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint8_t reg_idx, bitmask, sig_idx, val;

	/* Find signal in VW input table */
	for (sig_idx = 0; sig_idx < ARRAY_SIZE(vw_in_tbl); sig_idx++)
		if (vw_in_tbl[sig_idx].sig == signal) {
			reg_idx = vw_in_tbl[sig_idx].reg_idx;
			bitmask = vw_in_tbl[sig_idx].bitmask;

			val = GET_FIELD(inst->VWEVMS[reg_idx],
							NPCX_VWEVMS_WIRE);
			val &= GET_FIELD(inst->VWEVMS[reg_idx],
							NPCX_VWEVMS_VALID);

			*level = !!(val & bitmask);
			return 0;
		}

	/* Find signal in VW output table */
	for (sig_idx = 0; sig_idx < ARRAY_SIZE(vw_out_tbl); sig_idx++)
		if (vw_out_tbl[sig_idx].sig == signal) {
			reg_idx = vw_out_tbl[sig_idx].reg_idx;
			bitmask = vw_out_tbl[sig_idx].bitmask;

			val = GET_FIELD(inst->VWEVSM[reg_idx],
							NPCX_VWEVSM_WIRE);
			val &= GET_FIELD(inst->VWEVSM[reg_idx],
							NPCX_VWEVSM_VALID);
			*level = !!(val & bitmask);
			return 0;
		}

	LOG_ERR("%s Out of index %d", __func__, signal);
	return -EIO;
}

static int espi_npcx_manage_callback(const struct device *dev,
				    struct espi_callback *callback, bool set)
{
	struct espi_npcx_data *const data = DRV_DATA(dev);

	return espi_manage_callback(&data->callbacks, callback, set);
}

static int espi_npcx_read_lpc_request(const struct device *dev,
				     enum lpc_peripheral_opcode op,
				     uint32_t  *data)
{
	ARG_UNUSED(dev);

	return npcx_host_periph_read_request(op, data);
}

static int espi_npcx_write_lpc_request(const struct device *dev,
				      enum lpc_peripheral_opcode op,
				      uint32_t *data)
{
	ARG_UNUSED(dev);

	return npcx_host_periph_write_request(op, data);
}

#if defined(CONFIG_ESPI_OOB_CHANNEL)
static int espi_npcx_send_oob(const struct device *dev,
				struct espi_oob_packet *pckt)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint8_t *oob_buf = pckt->buf;
	int sz_oob_tx = pckt->len;
	int idx_tx_buf;
	uint32_t oob_data;

	/* Check out of OOB transmitted buffer size */
	if (sz_oob_tx > NPCX_ESPI_OOB_MAX_PAYLOAD) {
		LOG_ERR("Out of OOB transmitted buffer: %d", sz_oob_tx);
		return -EINVAL;
	}

	/* Check OOB Transmit Queue is empty? */
	if (IS_BIT_SET(inst->OOBCTL, NPCX_OOBCTL_OOB_AVAIL)) {
		LOG_ERR("OOB channel is busy");
		return -EBUSY;
	}

	/*
	 * GET_OOB header (first 4 bytes) in npcx 32-bits tx buffer
	 *
	 * [24:31] - LEN[0:7]     Data length of GET_OOB request package
	 * [20:23] - TAG          Tag of GET_OOB
	 * [16:19] - LEN[8:11]    Ignore it since max payload is 64 bytes
	 * [8:15]  - CYCLE_TYPE   Cycle type of GET_OOB
	 * [0:7]   - SZ_PACK      Package size plus 3 bytes header. (Npcx only)
	 */
	inst->OOBTXBUF[0] = (sz_oob_tx + 3)
			  | (ESPI_OOB_GET_CYCLE_TYPE << 8)
			  | (ESPI_OOB_TAG << 16)
			  | (sz_oob_tx << 24);

	/* Write GET_OOB data into 32-bits tx buffer in little endian */
	for (idx_tx_buf = 0; idx_tx_buf < sz_oob_tx/4; idx_tx_buf++,
								oob_buf += 4)
		inst->OOBTXBUF[idx_tx_buf + 1] = oob_buf[0]
					  | (oob_buf[1] << 8)
					  | (oob_buf[2] << 16)
					  | (oob_buf[3] << 24);

	/* Write remaining bytes of package */
	if (sz_oob_tx % 4) {
		int i;

		oob_data = 0;
		for (i = 0; i < sz_oob_tx % 4; i++)
			oob_data |= (oob_buf[i] << (8 * i));
		inst->OOBTXBUF[idx_tx_buf + 1] = oob_data;
	}

	/*
	 * Notify host a new OOB packet is ready. Please don't write OOB_FREE
	 * to 1 at the same tiem in case clear it unexpectedly.
	 */
	oob_data = inst->OOBCTL & ~(BIT(NPCX_OOBCTL_OOB_FREE));
	oob_data |= BIT(NPCX_OOBCTL_OOB_AVAIL);
	inst->OOBCTL = oob_data;

	while (IS_BIT_SET(inst->OOBCTL, NPCX_OOBCTL_OOB_AVAIL))
		;

	LOG_DBG("%s issued!!", __func__);
	return 0;
}

static int espi_npcx_receive_oob(const struct device *dev,
				struct espi_oob_packet *pckt)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_npcx_data *const data = DRV_DATA(dev);
	uint8_t *oob_buf = pckt->buf;
	uint32_t oob_data;
	int idx_rx_buf, sz_oob_rx, ret;

	/* Check eSPI bus status first */
	if (IS_BIT_SET(inst->ESPISTS, NPCX_ESPISTS_BERR)) {
		LOG_ERR("%s: eSPI Bus Error: 0x%08X", __func__, inst->ESPIERR);
		return -EIO;
	}

	/* Notify host that OOB received buffer is free now. */
	inst->OOBCTL |= BIT(NPCX_OOBCTL_OOB_FREE);

	/* Wait until get oob package or timeout */
	ret = k_sem_take(&data->oob_rx_lock, K_MSEC(ESPI_OOB_MAX_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		return -ETIMEDOUT;
	}

	/*
	 * PUT_OOB header (first 4 bytes) in npcx 32-bits rx buffer
	 *
	 * [24:31] - LEN[0:7]     Data length of PUT_OOB request package
	 * [20:23] - TAG          Tag of PUT_OOB
	 * [16:19] - LEN[8:11]    Data length of PUT_OOB request package
	 * [8:15]  - CYCLE_TYPE   Cycle type of PUT_OOB
	 * [0:7]   - SZ_PACK      Reserved. (Npcx only)
	 */
	oob_data = inst->OOBRXBUF[0];
	/* Get received package length first */
	sz_oob_rx = NPCX_OOB_RX_PACKAGE_LEN(oob_data);

	/* Check OOB received buffer size */
	if (sz_oob_rx > NPCX_ESPI_OOB_MAX_PAYLOAD) {
		LOG_ERR("Out of OOB received buffer: %d", sz_oob_rx);
		return -EINVAL;
	}

	/* Set received size to package structure */
	pckt->len = sz_oob_rx;

	/* Read PUT_OOB data into 32-bits rx buffer in little endian */
	for (idx_rx_buf = 0; idx_rx_buf < sz_oob_rx/4; idx_rx_buf++) {
		oob_data = inst->OOBRXBUF[idx_rx_buf + 1];

		*(oob_buf++) = oob_data & 0xFF;
		*(oob_buf++) = (oob_data >> 8) & 0xFF;
		*(oob_buf++) = (oob_data >> 16) & 0xFF;
		*(oob_buf++) = (oob_data >> 24) & 0xFF;
	}

	/* Read remaining bytes of package */
	if (sz_oob_rx % 4) {
		int i;

		oob_data = inst->OOBRXBUF[idx_rx_buf + 1];
		for (i = 0; i < sz_oob_rx % 4; i++)
			*(oob_buf++) = (oob_data >> (8 * i)) & 0xFF;
	}
	return 0;
}
#endif

/* Platform specific espi module functions */
void npcx_espi_enable_interrupts(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Enable eSPI bus interrupt */
	irq_enable(DT_INST_IRQN(0));

	/* Turn on all VW inputs' MIWU interrupts */
	for (int idx = 0; idx < ARRAY_SIZE(vw_in_tbl); idx++) {
		npcx_miwu_irq_enable(&(vw_in_tbl[idx].vw_wui));
	}
}

void npcx_espi_disable_interrupts(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Disable eSPI bus interrupt */
	irq_disable(DT_INST_IRQN(0));

	/* Turn off all VW inputs' MIWU interrupts */
	for (int idx = 0; idx < ARRAY_SIZE(vw_in_tbl); idx++) {
		npcx_miwu_irq_disable(&(vw_in_tbl[idx].vw_wui));
	}
}

/* eSPI driver registration */
static int espi_npcx_init(const struct device *dev);

static const struct espi_driver_api espi_npcx_driver_api = {
	.config = espi_npcx_configure,
	.get_channel_status = espi_npcx_channel_ready,
	.send_vwire = espi_npcx_send_vwire,
	.receive_vwire = espi_npcx_receive_vwire,
	.manage_callback = espi_npcx_manage_callback,
	.read_lpc_request = espi_npcx_read_lpc_request,
	.write_lpc_request = espi_npcx_write_lpc_request,
#if defined(CONFIG_ESPI_OOB_CHANNEL)
	.send_oob = espi_npcx_send_oob,
	.receive_oob = espi_npcx_receive_oob,
#endif
};

static struct espi_npcx_data espi_npcx_data;

static const struct npcx_alt espi_alts[] = NPCX_DT_ALT_ITEMS_LIST(0);

static const struct espi_npcx_config espi_npcx_config = {
	.base = DT_INST_REG_ADDR(0),
	.espi_rst_wui = NPCX_DT_WUI_ITEM_BY_NAME(0, espi_rst_wui),
	.clk_cfg = NPCX_DT_CLK_CFG_ITEM(0),
	.alts_size = ARRAY_SIZE(espi_alts),
	.alts_list = espi_alts,
};

DEVICE_DT_INST_DEFINE(0, &espi_npcx_init, NULL,
		    &espi_npcx_data, &espi_npcx_config,
		    PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY,
		    &espi_npcx_driver_api);

static int espi_npcx_init(const struct device *dev)
{
	const struct espi_npcx_config *const config = DRV_CONFIG(dev);
	struct espi_npcx_data *const data = DRV_DATA(dev);
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	const struct device *const clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	int i, ret;

	/* Turn on eSPI device clock first */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t *)
							&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on eSPI clock fail %d", ret);
		return ret;
	}

	/* Enable events which share the same espi bus interrupt */
	for (i = 0; i < ARRAY_SIZE(espi_bus_isr_tbl); i++) {
		inst->ESPIIE |= BIT(espi_bus_isr_tbl[i].int_en_bit);
		inst->ESPIWE |= BIT(espi_bus_isr_tbl[i].wake_en_bit);
	}

#if defined(CONFIG_ESPI_OOB_CHANNEL)
	k_sem_init(&data->oob_rx_lock, 0, 1);
#endif

	/* Configure Virtual Wire input signals */
	for (i = 0; i < ARRAY_SIZE(vw_in_tbl); i++)
		espi_vw_config_input(dev, &vw_in_tbl[i]);

	/* Configure Virtual Wire output signals */
	for (i = 0; i < ARRAY_SIZE(vw_out_tbl); i++)
		espi_vw_config_output(dev, &vw_out_tbl[i]);

	/* Configure wake-up input and callback for eSPI VW input signal */
	for (i = 0; i < ARRAY_SIZE(vw_in_tbl); i++)
		espi_init_wui_callback(dev, &vw_in_callback[i],
				&vw_in_tbl[i].vw_wui, espi_vw_generic_isr);

	/* Configure wake-up input and callback for ESPI_RST signal */
	espi_init_wui_callback(dev, &espi_rst_callback,
				&config->espi_rst_wui, espi_vw_espi_rst_isr);

	/* Configure pin-mux for eSPI bus device */
	npcx_pinctrl_mux_configure(config->alts_list, config->alts_size, 1);

	/* Configure host sub-modules which HW blocks belong to core domain */
	npcx_host_init_subs_core_domain(dev, &data->callbacks);

	/* eSPI Bus interrupt installation */
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    espi_bus_generic_isr,
		    DEVICE_DT_INST_GET(0), 0);

	/* Enable eSPI bus interrupt */
	irq_enable(DT_INST_IRQN(0));

	return 0;
}
