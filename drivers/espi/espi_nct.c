/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct_espi

#include <assert.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/espi_nct.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/espi/nct_espi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <soc.h>
#include "espi_utils.h"
#include "soc_host.h"
#include "soc_miwu.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(espi, CONFIG_ESPI_LOG_LEVEL);

struct espi_nct_config {
	uintptr_t base;
	/* clock configuration */
	uint32_t clk_cfg;
	/* mapping table between eSPI reset signal and wake-up input */
	struct nct_wui espi_rst_wui;
#if 0
	/* pinmux configuration */
	const struct pinctrl_dev_config *pcfg;
#endif
};

struct espi_nct_data {
	sys_slist_t callbacks;
	uint8_t plt_rst_asserted;
	uint8_t espi_rst_deasserted;
	uint8_t sx_state;
// #if defined(CONFIG_ESPI_OOB_CHANNEL)
// #if !defined(CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC)
// 	struct k_sem oob_rx_lock;
// #endif
// #endif
#if defined(CONFIG_ESPI_FLASH_CHANNEL)
	struct k_sem flash_rx_lock;
	struct k_sem tafs_tx_lock;
#endif
};

/* Driver convenience defines */
#define DRV_CONFIG(dev) ((const struct espi_nct_config *)(dev)->config)
#define DRV_DATA(dev) ((struct espi_nct_data *)(dev)->data)
#define HAL_INSTANCE(dev) ((struct espi_reg *)(DRV_CONFIG(dev)->base))

/* eSPI channels */
#define NCT_ESPI_CH_PC              0
#define NCT_ESPI_CH_VW              1
#define NCT_ESPI_CH_OOB             2
#define NCT_ESPI_CH_FLASH           3
#define NCT_ESPI_CH_COUNT           4
#define NCT_ESPI_HOST_CH_EN(ch)     (ch + 4)

/* eSPI max supported frequency */
#define NCT_ESPI_MAXFREQ_20         0
#define NCT_ESPI_MAXFREQ_25         1
#define NCT_ESPI_MAXFREQ_33         2
#define NCT_ESPI_MAXFREQ_50         3
#define NCT_ESPI_MAXFREQ_66         4

/* Minimum delay before acknowledging a virtual wire */
#define NCT_ESPI_VWIRE_ACK_DELAY    10ul /* 10 us */

/* OOB channel maximum payload size */
#define NCT_ESPI_OOB_MAX_PAYLOAD    64
#define NCT_OOB_RX_PACKAGE_LEN(hdr) (((hdr & 0xff000000) >> 24) | \
							((hdr & 0xf0000) >> 8))

/* Flash channel maximum payload size */
#define NCT_ESPI_FLASH_MAX_RX_PAYLOAD DT_INST_PROP(0, rx_plsize)
#define NCT_ESPI_FLASH_MAX_TX_PAYLOAD DT_INST_PROP(0, tx_plsize)

/* eSPI cycle type field for OOB and FLASH channels */
#define ESPI_FLASH_READ_CYCLE_TYPE                 0x00
#define ESPI_FLASH_WRITE_CYCLE_TYPE                0x01
#define ESPI_FLASH_ERASE_CYCLE_TYPE                0x02
#define ESPI_FLASH_SUCCESS_WITH_DATA_CYCLE_TYPE    0x0f
#define ESPI_FLASH_SUCCESS_WITHOUT_DATA_CYCLE_TYPE 0x06
#define ESPI_FLASH_HEADER_PCKT_SIZE                0x07
#define ESPI_FLASH_MAX_TIMEOUT                     1000ul /* 1000 ms */
#define ESPI_OOB_GET_CYCLE_TYPE                    0x21
#define ESPI_OOB_TAG                               0x00
#define ESPI_OOB_MAX_TIMEOUT                       500ul /* 500 ms */

/* eSPI bus interrupt configuration structure and macro function */
struct espi_bus_isr {
	uint8_t status_bit; /* bit order in ESPISTS register */
	uint8_t int_en_bit; /* bit order in ESPIIE register */
	uint8_t wake_en_bit; /* bit order in ESPIWE register */
	void (*bus_isr)(const struct device *dev); /* eSPI bus ISR */
};

#define NCT_ESPI_BUS_INT_ITEM(event, isr) {     \
	.status_bit = NCT_ESPISTS_##event,      \
	.int_en_bit = NCT_ESPIIE_##event##IE,   \
	.wake_en_bit = NCT_ESPIWE_##event##WE,  \
	.bus_isr = isr }

/* eSPI Virtual Wire Input (Master-to-Slave) signals configuration structure */
struct nct_vw_in_config {
	enum espi_vwire_signal sig; /* Virtual Wire signal */
	uint8_t  reg_idx; /* register index for VW signal */
	uint8_t  bitmask; /* VW signal bits-mask */
	struct nct_wui vw_wui; /* WUI mapping in MIWU modules for VW signal */
};

/* eSPI Virtual Wire Output (Slave-to-Master) signals configuration structure */
struct nct_vw_out_config {
	enum espi_vwire_signal sig; /* Virtual Wire signal */
	uint8_t  reg_idx; /* register index for VW signal */
	uint8_t  bitmask; /* VW signal bits-mask */
};

/*
 * eSPI VW input/Output signal configuration tables. Please refer
 * nct-espi-vws-map.dtsi device tree file for more detail.
 */
static const struct nct_vw_in_config vw_in_tbl[] = {
	/* index 02h (In)  */
	NCT_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SLP_S3, vw_slp_s3),
	NCT_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SLP_S4, vw_slp_s4),
	NCT_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SLP_S5, vw_slp_s5),
	/* index 03h (In)  */
	NCT_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SUS_STAT, vw_sus_stat),
	NCT_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_PLTRST, vw_plt_rst),
	NCT_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_OOB_RST_WARN, vw_oob_rst_warn),
	/* index 07h (In)  */
	NCT_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_HOST_RST_WARN, vw_host_rst_warn),
	/* index 41h (In)  */
	NCT_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SUS_WARN, vw_sus_warn),
	NCT_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK, vw_sus_pwrdn_ack),
	NCT_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SLP_A, vw_slp_a),
	/* index 42h (In)  */
	NCT_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SLP_LAN, vw_slp_lan),
	NCT_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_SLP_WLAN, vw_slp_wlan),
	/* index 4Ah (In)  */
	/* NCT_DT_VW_IN_CONF(ESPI_VWIRE_SIGNAL_DNX_WARN, vw_dnx_warn), */
};

static const struct nct_vw_out_config vw_out_tbl[] = {
	/* index 04h (Out) */
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_OOB_RST_ACK, vw_oob_rst_ack),
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_WAKE, vw_wake),
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_PME, vw_pme),
	/* index 05h (Out) */
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, vw_slv_boot_done),
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_ERR_FATAL, vw_err_fatal),
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_ERR_NON_FATAL, vw_err_non_fatal),
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS,
						vw_slv_boot_sts_with_done),
	/* index 06h (Out) */
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_SCI, vw_sci),
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_SMI, vw_smi),
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_HOST_RST_ACK, vw_host_rst_ack),
	/* index 40h (Out) */
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_SUS_ACK, vw_sus_ack),
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_DNX_ACK, vw_dnx_ack),
};

/*  Virtual wire GPIOs for platform level usage (High at Reset state) */
static const struct nct_vw_out_config vw_out_gpio_tbl1[] = {
#if defined(CONFIG_ESPI_NCT_SUPP_VW_GPIO)
	/* index 50h (Out) */
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_TARGET_GPIO_0, vw_slv_gpio_0),
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_TARGET_GPIO_1, vw_slv_gpio_1),
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_TARGET_GPIO_2, vw_slv_gpio_2),
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_TARGET_GPIO_3, vw_slv_gpio_3),
	/* index 51h (Out) */
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_TARGET_GPIO_4, vw_slv_gpio_4),
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_TARGET_GPIO_5, vw_slv_gpio_5),
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_TARGET_GPIO_6, vw_slv_gpio_6),
	NCT_DT_VW_OUT_CONF(ESPI_VWIRE_SIGNAL_TARGET_GPIO_7, vw_slv_gpio_7),
#endif
};

/* Callbacks for eSPI bus reset and Virtual Wire signals. */
static struct miwu_callback espi_rst_callback;
static struct miwu_callback vw_in_callback[ARRAY_SIZE(vw_in_tbl)];

/* eSPI VW service function forward declarations */
static int espi_nct_receive_vwire(const struct device *dev,
				enum espi_vwire_signal signal, uint8_t *level);
static int espi_nct_send_vwire(const struct device *dev,
			enum espi_vwire_signal signal, uint8_t level);
static void espi_vw_send_bootload_done(const struct device *dev);

/* eSPI local initialization functions */
static void espi_init_wui_callback(const struct device *dev,
		struct miwu_callback *callback, const struct nct_wui *wui,
		miwu_dev_callback_handler_t handler)
{
	/* VW signal which has no wake-up input source */
	if (wui->table == NCT_MIWU_TABLE_NONE)
		return;

	/* Install callback function */
	nct_miwu_init_dev_callback(callback, wui, handler, dev);
	nct_miwu_manage_callback(callback, 1);

	/* Configure MIWU setting and enable its interrupt */
	nct_miwu_interrupt_configure(wui, NCT_MIWU_MODE_EDGE,
							NCT_MIWU_TRIG_BOTH);

	nct_miwu_irq_enable(wui);
}

/* eSPI local bus interrupt service functions */
static void espi_bus_err_isr(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint32_t err = inst->ESPIERR;

	LOG_ERR("eSPI Bus Error %08X", err);
	/* Clear error status bits */
	inst->ESPIERR = err;

#if defined(CONFIG_ESPI_FLASH_CHANNEL)
	struct espi_nct_data *const data = DRV_DATA(dev);
	k_sem_give(&data->tafs_tx_lock);
#endif
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

#if defined(CONFIG_ESPI_FLASH_CHANNEL)
	struct espi_reg *const inst = HAL_INSTANCE(dev);
#if defined(CONFIG_ESPI_SAF)
	/* Support TAFS and CAFS */
	SET_FIELD(inst->FLASHCFG, NCT_FLASHCFG_FLASHCAPA,
			NCT_FLASHCFG_SUPP_TAFS_CAFS);
#if defined(CONFIG_ESPI_NCT_TAF_AUTO_MODE)
	inst->FLASHCTL |= BIT(NCT_FLASHCTL_SAF_AUTO_READ);
#else
	inst->FLASHCTL &= ~BIT(NCT_FLASHCTL_SAF_AUTO_READ);
#endif
#else
	/* Support CAFS */
	SET_FIELD(inst->FLASHCFG, NCT_FLASHCFG_FLASHCAPA,
			NCT_FLASHCFG_SUPP_CAFS);
#endif
#endif
}

static void espi_bus_cfg_update_isr(const struct device *dev)
{
	int chan;
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_nct_data *const data = DRV_DATA(dev);
	struct espi_event evt = { .evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				  .evt_details = 0,
				  .evt_data = 0 };
	/* If host enable bits are not sync with ready bits on slave side. */
	uint8_t chg_mask = GET_FIELD(inst->ESPICFG, NCT_ESPICFG_HCHANS_FIELD)
			 ^ GET_FIELD(inst->ESPICFG, NCT_ESPICFG_CHANS_FIELD);
	chg_mask &= (ESPI_CHANNEL_VWIRE | ESPI_CHANNEL_OOB |
							ESPI_CHANNEL_FLASH);

	LOG_DBG("ESPI CFG Change Updated! 0x%02X", chg_mask);
	/*
	 * If host enable/disable channel for VW/OOB/FLASH, NCT should follow
	 * except Peripheral channel. It is handled after receiving PLTRST
	 * event separately.
	 */
	for (chan = NCT_ESPI_CH_VW; chan < NCT_ESPI_CH_COUNT; chan++) {
		/* Channel ready bit isn't sync with enabled bit on host side */
		if (chg_mask & BIT(chan)) {
			evt.evt_data = IS_BIT_SET(inst->ESPICFG,
						NCT_ESPI_HOST_CH_EN(chan));
			evt.evt_details = BIT(chan);

			if (evt.evt_data) {
				inst->ESPICFG |= BIT(chan);
			} else {
				inst->ESPICFG &= ~BIT(chan);
			}

			espi_send_callbacks(&data->callbacks, dev, evt);
		}
	}
	LOG_DBG("ESPI CFG Updated! 0x%02X", GET_FIELD(inst->ESPICFG,
						NCT_ESPICFG_CHANS_FIELD));

	/* If VW channel is enabled and ready, send bootload done VW signal */
	if ((chg_mask & BIT(NCT_ESPI_CH_VW)) && IS_BIT_SET(inst->ESPICFG,
				NCT_ESPI_HOST_CH_EN(NCT_ESPI_CH_VW))) {
		espi_vw_send_bootload_done(dev);
	}

#if defined(CONFIG_ESPI_FLASH_CHANNEL)
	if ((chg_mask & BIT(NCT_ESPI_CH_FLASH)) && IS_BIT_SET(inst->ESPICFG,
				NCT_ESPI_HOST_CH_EN(NCT_ESPI_CH_FLASH))) {
#if defined(CONFIG_ESPI_SAF)
		/* Support TAFS and CAFS */
		SET_FIELD(inst->FLASHCFG, NCT_FLASHCFG_FLASHCAPA,
				NCT_FLASHCFG_SUPP_TAFS_CAFS);
#if defined(CONFIG_ESPI_NCT_TAF_AUTO_MODE)
		inst->FLASHCTL |= BIT(NCT_FLASHCTL_SAF_AUTO_READ);
#else
		inst->FLASHCTL &= ~BIT(NCT_FLASHCTL_SAF_AUTO_READ);
#endif

		if (0 == k_sem_count_get(&data->tafs_tx_lock)) {
			k_sem_give(&data->tafs_tx_lock);
		}
#else
		/* Support CAFS */
		SET_FIELD(inst->FLASHCFG, NCT_FLASHCFG_FLASHCAPA,
				NCT_FLASHCFG_SUPP_CAFS);
#endif
	}
#endif
}

static void espi_bus_vw_update_isr(const struct device *dev)
{
	LOG_DBG("ESPI VW Updated!");

	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_nct_data *const data = DRV_DATA(dev);
	uint8_t i;
	struct espi_event evt = { ESPI_BUS_EVENT_VWIRE_RECEIVED, 0, 0 };

	for (i = 0; i < ARRAY_SIZE(inst->VWEVMS); i++) {
		if (IS_BIT_SET(inst->VWEVMS[i], NCT_VWEVMS_MODIFIED)) {
			inst->VWEVMS[i] |= BIT(NCT_VWEVMS_MODIFIED);
			evt.evt_details = (NCT_VWEV_M_TO_S << NCT_VWEVMS_DIRECTION_POS) | i;
			evt.evt_data = (uint8_t)(inst->VWEVMS[i] & 0xFF);
			espi_send_callbacks(&data->callbacks, dev, evt);
		}
	}

	for (i = 0; i < ARRAY_SIZE(inst->VWGPMS); i++) {
		if (IS_BIT_SET(inst->VWGPMS[i], NCT_VWGPMS_MODIFIED)) {
			inst->VWGPMS[i] |= BIT(NCT_VWGPMS_MODIFIED);
			evt.evt_details = (NCT_VWGP_M_TO_S << NCT_VWGPMS_DIRECTION_POS) | i;
			evt.evt_data = (uint8_t)(inst->VWGPMS[i] & 0xFF);
			espi_send_callbacks(&data->callbacks, dev, evt);
		}
	}
}

#if defined(CONFIG_ESPI_OOB_CHANNEL)
static void espi_bus_oob_rx_isr(const struct device *dev)
{
	struct espi_nct_data *const data = DRV_DATA(dev);
// #if defined(CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC)
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_event evt = {
			.evt_type = ESPI_BUS_EVENT_OOB_RECEIVED,
			.evt_details = 0,
			.evt_data = 0,
		};

	/* Get received package length and set to additional detail of event */
	evt.evt_details = NCT_OOB_RX_PACKAGE_LEN(inst->OOBRXBUF[0]);
	espi_send_callbacks(&data->callbacks, dev, evt);
// #else
// 	LOG_DBG("%s", __func__);
// 	k_sem_give(&data->oob_rx_lock);
// #endif
}
#endif

#if defined(CONFIG_ESPI_FLASH_CHANNEL)
#if defined(CONFIG_ESPI_SAF)
static struct espi_taf_pckt taf_pckt;

static uint32_t espi_taf_parse(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct nct_taf_head taf_head;
	uint32_t taf_addr;
	uint8_t i, roundsize;

	/* Get type, length and tag from RX buffer */
	memcpy(&taf_head, (void *)&inst->FLASHRXBUF[0], sizeof(taf_head));
	taf_pckt.type = taf_head.type;
	taf_pckt.len = (((uint16_t)taf_head.tag_hlen & 0xF) << 8) | taf_head.llen;
	taf_pckt.tag = taf_head.tag_hlen >> 4;

	if ((taf_pckt.len == 0) && ((taf_pckt.type & 0xF) == NCT_ESPI_TAF_REQ_READ)) {
		taf_pckt.len = KB(4);
	}

	/* Get address from RX buffer */
	taf_addr = inst->FLASHRXBUF[1];
	taf_pckt.addr = sys_cpu_to_be32(taf_addr);

	/* Get written data if eSPI TAF write */
	if ((taf_pckt.type & 0xF) == NCT_ESPI_TAF_REQ_WRITE) {
		roundsize = DIV_ROUND_UP(taf_pckt.len, sizeof(uint32_t));
		for (i = 0; i < roundsize; i++) {
			taf_pckt.src[i] = inst->FLASHRXBUF[2 + i];
		}
	}

	return (uint32_t)&taf_pckt;
}
#endif /* CONFIG_ESPI_SAF */

static void espi_bus_flash_rx_isr(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_nct_data *const data = DRV_DATA(dev);

	/* Controller Attached Flash Access */
	if ((inst->ESPICFG & BIT(NCT_ESPICFG_FLASHCHANMODE)) == 0) {
		k_sem_give(&data->flash_rx_lock);
	} else { /* Target Attached Flash Access */
#if defined(CONFIG_ESPI_SAF)
		struct espi_event evt = {
			.evt_type = ESPI_BUS_SAF_NOTIFICATION,
			.evt_details = ESPI_CHANNEL_FLASH,
			.evt_data = espi_taf_parse(dev),
		};
		espi_send_callbacks(&data->callbacks, dev, evt);
#else
		LOG_WRN("ESPI TAF not supported");
#endif
	}
}

static void espi_bus_sflashrd_update_isr(const struct device *dev)
{
	struct espi_nct_data *const data = DRV_DATA(dev);
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint32_t reg;

	k_sem_give(&data->tafs_tx_lock);
	reg = inst->FLASHCTL;
	reg &= ~BIT(NCT_FLASHCTL_FLASH_TX_AVAIL);
	reg |= BIT(NCT_FLASHCTL_FLASH_NP_FREE);
	inst->FLASHCTL = reg;
}
#endif

const struct espi_bus_isr espi_bus_isr_tbl[] = {
	NCT_ESPI_BUS_INT_ITEM(BERR, espi_bus_err_isr),
	NCT_ESPI_BUS_INT_ITEM(IBRST, espi_bus_inband_rst_isr),
	NCT_ESPI_BUS_INT_ITEM(ESPIRST, espi_bus_reset_isr),
	NCT_ESPI_BUS_INT_ITEM(CFGUPD, espi_bus_cfg_update_isr),
	NCT_ESPI_BUS_INT_ITEM(VWUPD, espi_bus_vw_update_isr),
#if defined(CONFIG_ESPI_OOB_CHANNEL)
	NCT_ESPI_BUS_INT_ITEM(OOBRX, espi_bus_oob_rx_isr),
#endif
#if defined(CONFIG_ESPI_FLASH_CHANNEL)
	NCT_ESPI_BUS_INT_ITEM(FLASHRX, espi_bus_flash_rx_isr),
	NCT_ESPI_BUS_INT_ITEM(SFLASHRD, espi_bus_sflashrd_update_isr),
#endif
};

static void espi_bus_generic_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	int i;
	uint32_t mask, status;

	/*
	 * We need to set the same bit in mask in case bit 27
	 * in ESPISTS of nct is not cleared in ISR.
	 */
	mask = inst->ESPIIE | (1 << NCT_ESPISTS_VWUPDW);
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
				const struct nct_vw_in_config *config_in)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	int idx = config_in->reg_idx;

	/* IE & WE bits are already set? */
	if (IS_BIT_SET(inst->VWEVMS[idx], NCT_VWEVMS_IE) &&
		IS_BIT_SET(inst->VWEVMS[idx], NCT_VWEVMS_WE))
		return;

	/* Set IE & WE bits in VWEVMS */
	inst->VWEVMS[idx] |= BIT(NCT_VWEVMS_IE) | BIT(NCT_VWEVMS_WE);
	LOG_DBG("VWEVMS%d 0x%08X", idx, inst->VWEVMS[idx]);
}

static void espi_vw_config_output(const struct device *dev,
				const struct nct_vw_out_config *config_out)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	int idx = config_out->reg_idx;
	uint8_t valid = GET_FIELD(inst->VWEVSM[idx], NCT_VWEVSM_VALID);

	/* Set valid bits for vw signal which we have declared in table. */
	valid |= config_out->bitmask;
	SET_FIELD(inst->VWEVSM[idx], NCT_VWEVSM_VALID, valid);

	/*
	 * Turn off hardware-wire feature which generates VW events that
	 * connected to hardware signals. We will set it manually by software.
	 */
	SET_FIELD(inst->VWEVSM[idx], NCT_VWEVSM_HW_WIRE, 0);

	LOG_DBG("VWEVSM%d 0x%08X", idx, inst->VWEVSM[idx]);
}

#if 0
static void espi_vw_gpio_config_output(const struct device *dev,
				const struct nct_vw_out_config *config_out,
				uint8_t init_level)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	int idx = config_out->reg_idx;
	uint8_t valid = GET_FIELD(inst->VWGPSM[idx], NCT_VWEVSM_VALID);
	uint8_t val = GET_FIELD(inst->VWGPSM[idx], NCT_VWEVSM_WIRE);

	/* Set valid bits for vw signal which we have declared in table. */
	valid |= config_out->bitmask;
	SET_FIELD(inst->VWGPSM[idx], NCT_VWEVSM_VALID, valid);

	inst->VWGPSM[idx] |= BIT(NCT_VWGPSM_INDEX_EN);

	if (init_level) {
		val |= config_out->bitmask;
	} else {
		val &= ~config_out->bitmask;
	}

	SET_FIELD(inst->VWGPSM[idx], NCT_VWEVSM_WIRE, val);

	LOG_DBG("VWEVSM%d 0x%08X", idx, inst->VWGPSM[idx]);
}
#endif

static void espi_vw_notify_system_state(const struct device *dev,
				enum espi_vwire_signal signal)
{
	struct espi_nct_data *const data = DRV_DATA(dev);
	struct espi_event evt = { ESPI_BUS_EVENT_VWIRE_RECEIVED, 0, 0 };
	uint8_t wire = 0;

	espi_nct_receive_vwire(dev, signal, &wire);
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
	uint8_t wire = 0;

	espi_nct_receive_vwire(dev, signal, &wire);

	k_busy_wait(NCT_ESPI_VWIRE_ACK_DELAY);
	switch (signal) {
	case ESPI_VWIRE_SIGNAL_OOB_RST_WARN:
		espi_nct_send_vwire(dev, ESPI_VWIRE_SIGNAL_OOB_RST_ACK,
				wire);
		break;
	case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
		espi_nct_send_vwire(dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK,
				wire);
		break;
	case ESPI_VWIRE_SIGNAL_SUS_WARN:
		espi_nct_send_vwire(dev, ESPI_VWIRE_SIGNAL_SUS_ACK,
				wire);
		break;
	/*
	 * case ESPI_VWIRE_SIGNAL_DNX_WARN:
	 * 	espi_nct_send_vwire(dev, ESPI_VWIRE_SIGNAL_DNX_ACK,
	 * 			wire);
	 * 	break;
	 */
	default:
		break;
	}
}

static void espi_vw_notify_plt_rst(const struct device *dev)
{
	struct espi_nct_data *const data = DRV_DATA(dev);
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_event evt = { ESPI_BUS_EVENT_VWIRE_RECEIVED,
		ESPI_VWIRE_SIGNAL_PLTRST, 0
	};
	uint8_t wire = 0;

	espi_nct_receive_vwire(dev, ESPI_VWIRE_SIGNAL_PLTRST, &wire);
	LOG_DBG("VW_PLT_RST is %d!", wire);
	if (wire) {
		/* Set Peripheral Channel ready when PLTRST is de-asserted */
		inst->ESPICFG |= BIT(NCT_ESPICFG_PCHANEN);
		/* Configure all host sub-modules in host domain */
		nct_host_init_subs_host_domain();
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
	uint8_t boot_done = 0;

	ret = espi_nct_receive_vwire(dev,
			ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, &boot_done);
	LOG_DBG("%s: %d", __func__, boot_done);
	if (!ret && !boot_done) {
		/* Send slave boot status bit with done bit at the same time. */
		espi_nct_send_vwire(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS, 1);
	}
}

static void espi_vw_generic_isr(const struct device *dev, struct nct_wui *wui)
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
	} else if (signal == ESPI_VWIRE_SIGNAL_OOB_RST_WARN
		|| signal == ESPI_VWIRE_SIGNAL_HOST_RST_WARN
		|| signal == ESPI_VWIRE_SIGNAL_SUS_WARN
		/* || signal == ESPI_VWIRE_SIGNAL_DNX_WARN */) {
		espi_vw_notify_host_warning(dev, signal);
	} else if (signal == ESPI_VWIRE_SIGNAL_PLTRST) {
		espi_vw_notify_plt_rst(dev);
	}
}

static void espi_vw_espi_rst_isr(const struct device *dev, struct nct_wui *wui)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_nct_data *const data = DRV_DATA(dev);
	struct espi_event evt = { ESPI_BUS_RESET, 0, 0 };

	data->espi_rst_deasserted = IS_BIT_SET(inst->ESPISTS,
						NCT_ESPISTS_ESPIRST_DEASSERT);
	LOG_DBG("eSPI RST is %d!", data->espi_rst_deasserted);

	evt.evt_data = data->espi_rst_deasserted;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

/* eSPI api functions */
static int espi_nct_configure(const struct device *dev, struct espi_cfg *cfg)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);

	uint8_t max_freq = 0;
	uint8_t cur_io_mode, io_mode = 0;

	/* Configure eSPI frequency */
	switch (cfg->max_freq) {
	case 20:
		max_freq = NCT_ESPI_MAXFREQ_20;
		break;
	case 25:
		max_freq = NCT_ESPI_MAXFREQ_25;
		break;
	case 33:
		max_freq = NCT_ESPI_MAXFREQ_33;
		break;
	case 50:
		max_freq = NCT_ESPI_MAXFREQ_50;
		break;
	case 66:
		max_freq = NCT_ESPI_MAXFREQ_66;
		break;
	default:
		return -EINVAL;
	}
	SET_FIELD(inst->ESPICFG, NCT_ESPICFG_MAXFREQ_FIELD, max_freq);

	/* Configure eSPI IO mode */
	io_mode = (cfg->io_caps >> 1);
	if (io_mode > 3) {
		return -EINVAL;
	}

	cur_io_mode = GET_FIELD(inst->ESPICFG, NCT_ESPICFG_IOMODE_FIELD);
	if (io_mode != cur_io_mode) {
		SET_FIELD(inst->ESPICFG, NCT_ESPICFG_IOMODE_FIELD, io_mode);
	}

	/* Configure eSPI supported channels */
	if (cfg->channel_caps & ESPI_CHANNEL_PERIPHERAL) {
		inst->ESPICFG |= BIT(NCT_ESPICFG_PCCHN_SUPP);
	}

	if (cfg->channel_caps & ESPI_CHANNEL_VWIRE) {
		inst->ESPICFG |= BIT(NCT_ESPICFG_VWMS_VALID_EN);
		inst->ESPICFG |= BIT(NCT_ESPICFG_VWSM_VALID_EN);
		inst->ESPICFG |= BIT(NCT_ESPICFG_VWCHN_SUPP);
	}

	if (cfg->channel_caps & ESPI_CHANNEL_OOB) {
		inst->ESPICFG |= BIT(NCT_ESPICFG_OOBCHN_SUPP);
	}

	if (cfg->channel_caps & ESPI_CHANNEL_FLASH) {
		inst->ESPICFG |= BIT(NCT_ESPICFG_FLASHCHN_SUPP);
#if defined(CONFIG_ESPI_SAF)
		/* Support TAFS and CAFS */
		SET_FIELD(inst->FLASHCFG, NCT_FLASHCFG_FLASHCAPA,
				NCT_FLASHCFG_SUPP_TAFS_CAFS);
#if defined(CONFIG_ESPI_NCT_TAF_AUTO_MODE)
		inst->FLASHCTL |= BIT(NCT_FLASHCTL_SAF_AUTO_READ);
#else
		inst->FLASHCTL &= ~BIT(NCT_FLASHCTL_SAF_AUTO_READ);
#endif
#else
		/* Support CAFS */
		SET_FIELD(inst->FLASHCFG, NCT_FLASHCFG_FLASHCAPA,
				NCT_FLASHCFG_SUPP_CAFS);
#endif
	}

	LOG_DBG("%s: %d %d ESPICFG: 0x%08X", __func__,
				max_freq, io_mode, inst->ESPICFG);

	return 0;
}

static bool espi_nct_channel_ready(const struct device *dev,
					enum espi_channel ch)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	bool sts;

	switch (ch) {
	case ESPI_CHANNEL_PERIPHERAL:
		sts = IS_BIT_SET(inst->ESPICFG, NCT_ESPICFG_PCHANEN);
		break;
	case ESPI_CHANNEL_VWIRE:
		sts = IS_BIT_SET(inst->ESPICFG, NCT_ESPICFG_VWCHANEN);
		break;
	case ESPI_CHANNEL_OOB:
		sts = IS_BIT_SET(inst->ESPICFG, NCT_ESPICFG_OOBCHANEN);
		break;
	case ESPI_CHANNEL_FLASH:
		sts = IS_BIT_SET(inst->ESPICFG, NCT_ESPICFG_FLASHCHANEN);
		break;
	default:
		sts = false;
		break;
	}

	return sts;
}

static int espi_nct_send_vwire(const struct device *dev,
			enum espi_vwire_signal signal, uint8_t level)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint8_t reg_idx, bitmask, sig_idx, val = 0, vw_tbl_size;
	const struct nct_vw_out_config *vw_tbl;
	uint32_t reg_val;
	char *reg_name;

	if (signal >= ESPI_VWIRE_SIGNAL_COUNT) {
		LOG_ERR("Invalid VW: %d", signal);
		return -EINVAL;
	}

	if (signal >= ESPI_VWIRE_SIGNAL_TARGET_GPIO_0) {
		vw_tbl = vw_out_gpio_tbl1;
		vw_tbl_size = ARRAY_SIZE(vw_out_gpio_tbl1);
		reg_name = "VWGPSM";
	} else {
		vw_tbl = vw_out_tbl;
		vw_tbl_size = ARRAY_SIZE(vw_out_tbl);
		reg_name = "VWEVSM";
	}

	/* Find signal in VW output table */
	for (sig_idx = 0; sig_idx < vw_tbl_size; sig_idx++)
		if (vw_tbl[sig_idx].sig == signal)
			break;

	if (sig_idx == vw_tbl_size) {
		LOG_ERR("%s signal %d is invalid", __func__, signal);
		return -EIO;
	}

	reg_idx = vw_tbl[sig_idx].reg_idx;
	bitmask = vw_tbl[sig_idx].bitmask;

	/* Get wire field and set/clear wire bit */
	if (signal >= ESPI_VWIRE_SIGNAL_TARGET_GPIO_0) {
		val = GET_FIELD(inst->VWGPSM[reg_idx], NCT_VWEVSM_WIRE);
	} else {
		val = GET_FIELD(inst->VWEVSM[reg_idx], NCT_VWEVSM_WIRE);
	}

	if (level) {
		val |= bitmask;
	} else {
		val &= ~bitmask;
	}

	if (signal >= ESPI_VWIRE_SIGNAL_TARGET_GPIO_0) {
		SET_FIELD(inst->VWGPSM[reg_idx], NCT_VWEVSM_WIRE, val);
		reg_val = inst->VWGPSM[reg_idx];
	} else {
		SET_FIELD(inst->VWEVSM[reg_idx], NCT_VWEVSM_WIRE, val);
		reg_val = inst->VWEVSM[reg_idx];
	}

	LOG_DBG("Send VW: %s%d 0x%08X", reg_name, reg_idx, reg_val);

	return 0;
}

static int espi_nct_receive_vwire(const struct device *dev,
				  enum espi_vwire_signal signal, uint8_t *level)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint8_t reg_idx, bitmask, sig_idx, val;
	uint8_t ignore_valid_bit = !!(*level & ESPI_VW_READ_IGNORE_VALID_BIT);

	/* Find signal in VW input table */
	for (sig_idx = 0; sig_idx < ARRAY_SIZE(vw_in_tbl); sig_idx++)
		if (vw_in_tbl[sig_idx].sig == signal) {
			reg_idx = vw_in_tbl[sig_idx].reg_idx;
			bitmask = vw_in_tbl[sig_idx].bitmask;

			val = GET_FIELD(inst->VWEVMS[reg_idx],
							NCT_VWEVMS_WIRE);
			if (!ignore_valid_bit) {
				val &= GET_FIELD(inst->VWEVMS[reg_idx],
								NCT_VWEVMS_VALID);
			}

			*level = !!(val & bitmask);
			return 0;
		}

	/* Find signal in VW output table */
	for (sig_idx = 0; sig_idx < ARRAY_SIZE(vw_out_tbl); sig_idx++)
		if (vw_out_tbl[sig_idx].sig == signal) {
			reg_idx = vw_out_tbl[sig_idx].reg_idx;
			bitmask = vw_out_tbl[sig_idx].bitmask;

			val = GET_FIELD(inst->VWEVSM[reg_idx],
							NCT_VWEVSM_WIRE);
			if (!ignore_valid_bit) {
				val &= GET_FIELD(inst->VWEVSM[reg_idx],
								NCT_VWEVSM_VALID);
			}

			*level = !!(val & bitmask);
			return 0;
		}

	LOG_ERR("%s Out of index %d", __func__, signal);
	return -EIO;
}

static int espi_nct_manage_callback(const struct device *dev,
				    struct espi_callback *callback, bool set)
{
	struct espi_nct_data *const data = DRV_DATA(dev);

	return espi_manage_callback(&data->callbacks, callback, set);
}

static int espi_nct_read_lpc_request(const struct device *dev,
				     enum lpc_peripheral_opcode op,
				     uint32_t  *data)
{
	ARG_UNUSED(dev);

	return nct_host_periph_read_request(op, data);
}

static int espi_nct_write_lpc_request(const struct device *dev,
				      enum lpc_peripheral_opcode op,
				      uint32_t *data)
{
	ARG_UNUSED(dev);

	return nct_host_periph_write_request(op, data);
}

#if defined(CONFIG_ESPI_OOB_CHANNEL)
static int espi_nct_send_oob(const struct device *dev,
				struct espi_oob_packet *pckt)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint8_t *oob_buf = pckt->buf;
	int sz_oob_tx = pckt->len;
	int idx_tx_buf;
	uint32_t oob_data;

	/* Check out of OOB transmitted buffer size */
	if (sz_oob_tx > NCT_ESPI_OOB_MAX_PAYLOAD) {
		LOG_ERR("Out of OOB transmitted buffer: %d", sz_oob_tx);
		return -EINVAL;
	}

	/* Check OOB Transmit Queue is empty? */
	if (IS_BIT_SET(inst->OOBCTL, NCT_OOBCTL_OOB_AVAIL)) {
		LOG_ERR("OOB channel is busy");
		return -EBUSY;
	}

	/*
	 * GET_OOB header (first 4 bytes) in nct 32-bits tx buffer
	 *
	 * [24:31] - LEN[0:7]     Data length of GET_OOB request package
	 * [20:23] - TAG          Tag of GET_OOB
	 * [16:19] - LEN[8:11]    Ignore it since max payload is 64 bytes
	 * [8:15]  - CYCLE_TYPE   Cycle type of GET_OOB
	 * [0:7]   - SZ_PACK      Package size plus 3 bytes header. (NCT only)
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
	oob_data = inst->OOBCTL & ~(BIT(NCT_OOBCTL_OOB_FREE));
	oob_data |= BIT(NCT_OOBCTL_OOB_AVAIL);
	inst->OOBCTL = oob_data;

	while (IS_BIT_SET(inst->OOBCTL, NCT_OOBCTL_OOB_AVAIL))
		;

	LOG_DBG("%s issued!!", __func__);
	return 0;
}

static int espi_nct_receive_oob(const struct device *dev,
				struct espi_oob_packet *pckt)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	uint8_t *oob_buf = pckt->buf;
	uint32_t oob_data;
	int idx_rx_buf, sz_oob_rx;

	/* Check eSPI bus status first */
	if (IS_BIT_SET(inst->ESPISTS, NCT_ESPISTS_BERR)) {
		LOG_ERR("%s: eSPI Bus Error: 0x%08X", __func__, inst->ESPIERR);
		return -EIO;
	}

// #if !defined(CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC)
// 	struct espi_nct_data *const data = DRV_DATA(dev);
// 	int ret;

// 	/* Wait until get oob package or timeout */
// 	ret = k_sem_take(&data->oob_rx_lock, K_MSEC(ESPI_OOB_MAX_TIMEOUT));
// 	if (ret == -EAGAIN) {
// 		LOG_ERR("%s: Timeout", __func__);
// 		return -ETIMEDOUT;
// 	}
// #endif

	/*
	 * PUT_OOB header (first 4 bytes) in nct 32-bits rx buffer
	 *
	 * [24:31] - LEN[0:7]     Data length of PUT_OOB request package
	 * [20:23] - TAG          Tag of PUT_OOB
	 * [16:19] - LEN[8:11]    Data length of PUT_OOB request package
	 * [8:15]  - CYCLE_TYPE   Cycle type of PUT_OOB
	 * [0:7]   - SZ_PACK      Reserved. (NCT only)
	 */
	oob_data = inst->OOBRXBUF[0];
	/* Get received package length first */
	sz_oob_rx = NCT_OOB_RX_PACKAGE_LEN(oob_data);

	/* Check OOB received buffer size */
	if (sz_oob_rx > NCT_ESPI_OOB_MAX_PAYLOAD) {
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

	/* Notify host that OOB received buffer is free now. */
	inst->OOBCTL |= BIT(NCT_OOBCTL_OOB_FREE);

	return 0;
}
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
static void espi_nct_flash_prepare_tx_header(const struct device *dev,
	int cyc_type,  int flash_addr, int flash_len, int tx_payload)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);

	/*
	 * First 3 bytes of flash cycle command header in tx buffer
	 *
	 * [24:31] - LEN[0:7]   = n  Data length of flash cycle request
	 * [16:23] - LEN[8:15]  = 0  Ignore it since max buffer size is 64 bytes
	 * [12:15] - TAG        = 0  Tag of flash cycle command is always 0 here
	 * [8:11]  - CYCLE_TYPE = 0  Cycle type of flash command
	 * [0:7]   - SZ_PACK    = 7  Overall tx package size. (Used internally.)
	 */
	inst->FLASHTXBUF[0] = (flash_len << 24) |
			      (cyc_type << 8) |
			      (tx_payload + ESPI_FLASH_HEADER_PCKT_SIZE);

	/*
	 * Following 4 bytes of tager flash address in tx buffer
	 *
	 * [24:31] - ADDR[0:7]   Start address of flash cycle command request
	 * [16:23] - ADDR[15:8]
	 * [8:15]  - ADDR[23:16]
	 * [0:7]   - ADDR[31:24]
	 */
	inst->FLASHTXBUF[1] = sys_cpu_to_be32(flash_addr);
}

static int espi_nct_flash_parse_completion(const struct device *dev)
{
	int cycle_type;
	struct espi_reg *const inst = HAL_INSTANCE(dev);

	/*
	 * First 3 bytes of flash cycle completion header in rx buffer
	 *
	 * [24:31] - LEN[0:7]   Data length of flash cycle completion package
	 * [16:23] - LEN[8:15]  Ignore it since rx bufer size is 64 bytes
	 * [12:15] - TAG        Tag of flash cycle completion package
	 * [8:11]  - CYCLE_TYPE Cycle type of flash completion
	 * [0:7]   - Reserved
	 */
	cycle_type = (inst->FLASHRXBUF[0] & 0xff00) >> 8;
	if (cycle_type == ESPI_FLASH_SUCCESS_WITHOUT_DATA_CYCLE_TYPE) {
		return 0;
	}

	return -EIO;
}

static int espi_nct_flash_parse_completion_with_data(const struct device *dev,
						struct espi_flash_packet *pckt)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	int cycle_type, sz_rx_payload;

	/*
	 * First 3 bytes of flash cycle completion header in rx buffer
	 *
	 * [24:31] - LEN[0:7]   Data length of flash cycle completion package
	 * [16:23] - LEN[8:15]  Ignore it since rx bufer size is 64 bytes
	 * [12:15] - TAG        Tag of flash cycle completion package
	 * [8:11]  - CYCLE_TYPE Cycle type of flash completion
	 * [0:7]   - Reserved
	 *
	 * The following is flash data/
	 */
	cycle_type = (inst->FLASHRXBUF[0] & 0xff00) >> 8;
	sz_rx_payload  = inst->FLASHRXBUF[0] >> 24;
	if (cycle_type == ESPI_FLASH_SUCCESS_WITH_DATA_CYCLE_TYPE) {
		volatile uint32_t *rx_buf = &inst->FLASHRXBUF[1];
		uint8_t *buf = pckt->buf;
		uint32_t data;

		/* Get data from flash RX buffer */
		for (int i = 0; i < sz_rx_payload / 4; i++, rx_buf++) {
			data = *rx_buf;
			for (int j = 0; j < 4; j++, buf++) {
				*buf = data & 0xff;
				data = data >> 8;
			}
		}

		/* Get remaining bytes */
		if (sz_rx_payload % 4) {
			data = *rx_buf;
			for (int j = 0; j < sz_rx_payload % 4; j++, buf++) {
				*buf = data & 0xff;
				data = data >> 8;
			}
		}

		return 0;
	}

	return -EIO;
}

static int espi_nct_flash_read(const struct device *dev,
			       struct espi_flash_packet *pckt)
{
	int ret;
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_nct_data *const data = DRV_DATA(dev);

	/* Check out of FLASH received buffer size */
	if (pckt->len > NCT_ESPI_FLASH_MAX_RX_PAYLOAD) {
		LOG_ERR("Out of FLASH transmitted buffer: %d", pckt->len);
		return -EINVAL;
	}

	/* Check Flash Transmit Queue is empty? */
	if (IS_BIT_SET(inst->FLASHCTL, NCT_FLASHCTL_FLASH_TX_AVAIL)) {
		LOG_ERR("flash channel is busy");
		return -EBUSY;
	}

	/* Prepare FLASH_READ header in tx buffer */
	espi_nct_flash_prepare_tx_header(dev,
					  ESPI_FLASH_READ_CYCLE_TYPE,
					  pckt->flash_addr,
					  pckt->len,
					  0);

	/* Set the FLASHCTL.FLASH_TX_AVAIL bit to 1 to enqueue the packet */
	inst->FLASHCTL |= BIT(NCT_FLASHCTL_FLASH_TX_AVAIL);

	/* Wait until get flash package or timeout */
	ret = k_sem_take(&data->flash_rx_lock, K_MSEC(ESPI_FLASH_MAX_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		return -ETIMEDOUT;
	}

	return espi_nct_flash_parse_completion_with_data(dev, pckt);
}

static int espi_nct_flash_write(const struct device *dev,
					struct espi_flash_packet *pckt)
{
	int ret;
	uint32_t tx_data;
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_nct_data *const data = DRV_DATA(dev);
	volatile uint32_t *tx_buf = &inst->FLASHTXBUF[2];
	uint8_t *buf = pckt->buf;

	/* Check out of FLASH transmitted buffer size */
	if (pckt->len > NCT_ESPI_FLASH_MAX_TX_PAYLOAD) {
		LOG_ERR("Out of FLASH transmitted buffer: %d", pckt->len);
		return -EINVAL;
	}

	/* Check Flash Transmit Queue is empty? */
	if (IS_BIT_SET(inst->FLASHCTL, NCT_FLASHCTL_FLASH_TX_AVAIL)) {
		LOG_ERR("flash channel is busy");
		return -EBUSY;
	}

	/* Prepare FLASH_WRITE header in tx buffer */
	espi_nct_flash_prepare_tx_header(dev,
					  ESPI_FLASH_WRITE_CYCLE_TYPE,
					  pckt->flash_addr,
					  pckt->len,
					  pckt->len);

	/* Put package data to flash TX buffer */
	for (int i = 0; i < pckt->len / 4; i++, tx_buf++) {
		tx_data = 0;
		for (int j = 0; j < 4; j++, buf++) {
			tx_data |= (*buf << (j * 8));
		}
		*tx_buf = tx_data;
	}

	/* Put remaining bytes to flash TX buffer */
	if (pckt->len % 4) {
		tx_data = 0;
		for (int j = 0; j < pckt->len % 4; j++, buf++) {
			tx_data |= (*buf << (j * 8));
		}
		*tx_buf = tx_data;
	}

	/* Set the FLASHCTL.FLASH_TX_AVAIL bit to 1 to enqueue the packet */
	inst->FLASHCTL |= BIT(NCT_FLASHCTL_FLASH_TX_AVAIL);

	/* Wait until get flash package or timeout */
	ret = k_sem_take(&data->flash_rx_lock, K_MSEC(ESPI_FLASH_MAX_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		return -ETIMEDOUT;
	}

	/* Parse completion package in rx buffer */
	return espi_nct_flash_parse_completion(dev);
}

static int espi_nct_flash_erase(const struct device *dev,
					struct espi_flash_packet *pckt)
{
	int ret;
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	struct espi_nct_data *const data = DRV_DATA(dev);

	/* Check Flash Transmit Queue is empty? */
	if (IS_BIT_SET(inst->FLASHCTL, NCT_FLASHCTL_FLASH_TX_AVAIL)) {
		LOG_ERR("flash channel is busy");
		return -EBUSY;
	}

	/* Prepare FLASH_ERASE header in tx buffer */
	espi_nct_flash_prepare_tx_header(dev,
					  ESPI_FLASH_ERASE_CYCLE_TYPE,
					  pckt->flash_addr,
					  pckt->len,
					  0);

	/* Set the FLASHCTL.FLASH_TX_AVAIL bit to 1 to enqueue the packet */
	inst->FLASHCTL |= BIT(NCT_FLASHCTL_FLASH_TX_AVAIL);

	/* Wait until get flash package or timeout */
	ret = k_sem_take(&data->flash_rx_lock, K_MSEC(ESPI_FLASH_MAX_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		return -ETIMEDOUT;
	}

	/* Parse completion package in rx buffer */
	return espi_nct_flash_parse_completion(dev);
}
#endif

/* Platform specific espi module functions */
void nct_espi_enable_interrupts(const struct device *dev)
{
	const struct espi_nct_config *const config = DRV_CONFIG(dev);

	/* Enable eSPI bus interrupt */
	irq_enable(DT_INST_IRQN(0));

	/* Turn on all VW inputs' MIWU interrupts */
	for (int idx = 0; idx < ARRAY_SIZE(vw_in_tbl); idx++) {
		nct_miwu_irq_enable(&(vw_in_tbl[idx].vw_wui));
	}

	nct_miwu_irq_enable(&config->espi_rst_wui);
}

void nct_espi_disable_interrupts(const struct device *dev)
{
	const struct espi_nct_config *const config = DRV_CONFIG(dev);

	/* Disable eSPI bus interrupt */
	irq_disable(DT_INST_IRQN(0));

	/* Turn off all VW inputs' MIWU interrupts */
	for (int idx = 0; idx < ARRAY_SIZE(vw_in_tbl); idx++) {
		nct_miwu_irq_disable(&(vw_in_tbl[idx].vw_wui));
	}

	nct_miwu_irq_disable(&config->espi_rst_wui);
}

/* eSPI driver registration */
static int espi_nct_init(const struct device *dev);

static const struct espi_driver_api espi_nct_driver_api = {
	.config = espi_nct_configure,
	.get_channel_status = espi_nct_channel_ready,
	.send_vwire = espi_nct_send_vwire,
	.receive_vwire = espi_nct_receive_vwire,
	.manage_callback = espi_nct_manage_callback,
	.read_lpc_request = espi_nct_read_lpc_request,
	.write_lpc_request = espi_nct_write_lpc_request,
#if defined(CONFIG_ESPI_OOB_CHANNEL)
	.send_oob = espi_nct_send_oob,
	.receive_oob = espi_nct_receive_oob,
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	.flash_read = espi_nct_flash_read,
	.flash_write = espi_nct_flash_write,
	.flash_erase = espi_nct_flash_erase,
#endif
};

static struct espi_nct_data espi_nct_data;

#if 0
PINCTRL_DT_INST_DEFINE(0);
#endif

static const struct espi_nct_config espi_nct_config = {
	.base = DT_INST_REG_ADDR(0),
	.espi_rst_wui = NCT_DT_WUI_ITEM_BY_NAME(0, espi_rst_wui),
	.clk_cfg = DT_INST_PHA(0, clocks, clk_cfg),
#if 0
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
#endif
};

DEVICE_DT_INST_DEFINE(0, &espi_nct_init, NULL,
		    &espi_nct_data, &espi_nct_config,
		    PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY,
		    &espi_nct_driver_api);

static int espi_nct_init(const struct device *dev)
{
	const struct espi_nct_config *const config = DRV_CONFIG(dev);
	struct espi_nct_data *const data = DRV_DATA(dev);
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));
	int i, ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

#define ESPI_FREQ_20MHZ       20u
#define ESPI_FREQ_25MHZ       25u
#define ESPI_FREQ_33MHZ       33u
#define ESPI_FREQ_50MHZ       50u
#define ESPI_FREQ_66MHZ       66u

	/* Indicate to eSPI master simplest configuration: Single line,
	 * 20MHz frequency and only logical channel 0 and 1 are supported
	 */
	struct espi_cfg cfg = {
		.io_caps = ESPI_IO_MODE_SINGLE_LINE | ESPI_IO_MODE_DUAL_LINES
			   | ESPI_IO_MODE_QUAD_LINES,
		.channel_caps = ESPI_CHANNEL_VWIRE | ESPI_CHANNEL_PERIPHERAL,
		.max_freq = ESPI_FREQ_20MHZ,
	};

	/* If eSPI driver supports additional capabilities use them */
#ifdef CONFIG_ESPI_OOB_CHANNEL
	cfg.channel_caps |= ESPI_CHANNEL_OOB;
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	cfg.channel_caps |= ESPI_CHANNEL_FLASH;
#endif

	inst->ESPICFG &= ~BIT(NCT_ESPICFG_VWCHANEN);

	/* Turn on eSPI device clock first */
	ret = clock_control_on(clk_dev,
					(clock_control_subsys_t)config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on eSPI clock fail %d", ret);
		return ret;
	}

	/* Enable events which share the same espi bus interrupt */
	for (i = 0; i < ARRAY_SIZE(espi_bus_isr_tbl); i++) {
		inst->ESPIIE |= BIT(espi_bus_isr_tbl[i].int_en_bit);
		inst->ESPIWE |= BIT(espi_bus_isr_tbl[i].wake_en_bit);
	}

// #if defined(CONFIG_ESPI_OOB_CHANNEL)
// #if !defined(CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC)
// 	k_sem_init(&data->oob_rx_lock, 0, 1);
// #endif
// #endif

#if defined(CONFIG_ESPI_FLASH_CHANNEL)
	k_sem_init(&data->flash_rx_lock, 0, 1);
	k_sem_init(&data->tafs_tx_lock, 1, 1);
#endif

	/* Configure Virtual Wire input signals */
	for (i = 0; i < ARRAY_SIZE(vw_in_tbl); i++)
		espi_vw_config_input(dev, &vw_in_tbl[i]);

	/* Configure Virtual Wire output signals */
	for (i = 0; i < ARRAY_SIZE(vw_out_tbl); i++)
		espi_vw_config_output(dev, &vw_out_tbl[i]);

#if 0
	/* Configure Virtual Wire GPIOs that are output high at reset state */
	for (i = 0; i < ARRAY_SIZE(vw_out_gpio_tbl1); i++) {
		espi_vw_gpio_config_output(dev, &vw_out_gpio_tbl1[i], 1);
	}
#endif

	/* Configure wake-up input and callback for eSPI VW input signal */
	for (i = 0; i < ARRAY_SIZE(vw_in_tbl); i++)
		espi_init_wui_callback(dev, &vw_in_callback[i],
				&vw_in_tbl[i].vw_wui, espi_vw_generic_isr);

	/* Configure wake-up input and callback for ESPI_RST signal */
	espi_init_wui_callback(dev, &espi_rst_callback,
				&config->espi_rst_wui, espi_vw_espi_rst_isr);

	for (i = 0; i < ARRAY_SIZE(inst->VWGPMS); i++) {
		inst->VWGPMS[i] |= (BIT(NCT_VWGPMS_INDEX_EN) | BIT(NCT_VWGPMS_IE));
		inst->VWGPMS[i] &= ~BIT(NCT_VWGPMS_ENESPIRST);
	}

	for (i = 0; i < ARRAY_SIZE(inst->VWGPSM); i++) {
		inst->VWGPSM[i] |= (BIT(NCT_VWGPSM_INDEX_EN) | BIT(NCT_VWGPSM_IE));
	}

#if 0
	/* Configure pin-mux for eSPI bus device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("eSPI pinctrl setup failed (%d)", ret);
		return ret;
	}
#endif

	espi_nct_configure(dev, &cfg);

	/* Configure host sub-modules which HW blocks belong to core domain */
	nct_host_init_subs_core_domain(dev, &data->callbacks);

	/* eSPI Bus interrupt installation */
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    espi_bus_generic_isr,
		    DEVICE_DT_INST_GET(0), 0);

	/* Enable eSPI bus interrupt */
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

void nct_espi_vmevsm_rcin_sw(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	inst->VWEVSMTYPE &= ~BIT(NCT_VWEVSMTYPE_RCINTYPE);
}

void nct_espi_vmevsm_smi_sw(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	inst->VWEVSMTYPE &= ~BIT(NCT_VWEVSMTYPE_SMITYPE);
}

void nct_espi_vmevsm_sci_sw(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	inst->VWEVSMTYPE &= ~BIT(NCT_VWEVSMTYPE_SCITYPE);
}

void nct_espi_vmevsm_pme_sw(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	inst->VWEVSMTYPE &= ~BIT(NCT_VWEVSMTYPE_PMETYPE);
}

void nct_espi_vmevsm_wake_sw(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	inst->VWEVSMTYPE &= ~BIT(NCT_VWEVSMTYPE_WAKETYPE);
}

void nct_espi_vmevsm_rcin_hw(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	inst->VWEVSMTYPE |= BIT(NCT_VWEVSMTYPE_RCINTYPE);
}

void nct_espi_vmevsm_smi_hw(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	inst->VWEVSMTYPE |= BIT(NCT_VWEVSMTYPE_SMITYPE);
}

void nct_espi_vmevsm_sci_hw(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	inst->VWEVSMTYPE |= BIT(NCT_VWEVSMTYPE_SCITYPE);
}

void nct_espi_vmevsm_pme_hw(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	inst->VWEVSMTYPE |= BIT(NCT_VWEVSMTYPE_PMETYPE);
}

void nct_espi_vmevsm_wake_hw(const struct device *dev)
{
	struct espi_reg *const inst = HAL_INSTANCE(dev);
	inst->VWEVSMTYPE |= BIT(NCT_VWEVSMTYPE_WAKETYPE);
}
