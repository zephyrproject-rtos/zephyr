/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5912_espi

#include <zephyr/kernel.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(espi, CONFIG_ESPI_LOG_LEVEL);

#include "espi_utils.h"
#include "reg/reg_espi.h"
#include "reg/reg_port80.h"

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "support only one espi compatible node");

struct espi_rts5912_config {
	volatile struct espi_reg *const espi_reg;
	uint32_t espislv_clk_grp;
	uint32_t espislv_clk_idx;
#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	volatile struct port80_reg *const port80_reg;
	uint32_t port80_clk_grp;
	uint32_t port80_clk_idx;
#endif
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pcfg;
};

struct espi_rts5912_data {
	sys_slist_t callbacks;
	uint32_t config_data;
#ifdef CONFIG_ESPI_OOB_CHANNEL
	struct k_sem oob_rx_lock;
	struct k_sem oob_tx_lock;
	uint8_t *oob_tx_ptr;
	uint8_t *oob_rx_ptr;
	bool oob_tx_busy;
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	struct k_sem flash_lock;
	uint8_t *maf_ptr;
#endif
};

/*
 * =========================================================================
 * ESPI Peripheral Debug Port 80
 * =========================================================================
 */

#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80

static void espi_port80_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	struct espi_event evt = {ESPI_BUS_PERIPHERAL_NOTIFICATION,
				 ESPI_PERIPHERAL_INDEX_0 << 16 | ESPI_PERIPHERAL_DEBUG_PORT80,
				 ESPI_PERIPHERAL_NODATA};
	volatile struct port80_reg *const port80_reg = espi_config->port80_reg;

	evt.evt_data = port80_reg->DATA;
	espi_send_callbacks(&espi_data->callbacks, dev, evt);
}

static int espi_peri_ch_port80_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct rts5912_sccon_subsys sccon;
	volatile struct port80_reg *const port80_reg = espi_config->port80_reg;
	int rc;

	if (!device_is_ready(espi_config->clk_dev)) {
		return -ENODEV;
	}

	sccon.clk_grp = espi_config->port80_clk_grp;
	sccon.clk_idx = espi_config->port80_clk_idx;

	rc = clock_control_on(espi_config->clk_dev, (clock_control_subsys_t)&sccon);
	if (rc != 0) {
		return rc;
	}

	port80_reg->ADDR = 0x80UL;
	port80_reg->CFG = PORT80_CFG_CLRFLG | PORT80_CFG_THREEN;
	port80_reg->INTEN = PORT80_INTEN_THREINTEN;

	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), port80, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), port80, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), port80, priority), espi_port80_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), port80, irq));

	return 0;
}

#endif /* CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80 */

/*
 * =========================================================================
 * ESPI VWIRE channel
 * =========================================================================
 */

#ifdef CONFIG_ESPI_VWIRE_CHANNEL

#define VW_CH_IDX2  0x02UL
#define VW_CH_IDX3  0x03UL
#define VW_CH_IDX4  0x04UL
#define VW_CH_IDX5  0x05UL
#define VW_CH_IDX6  0x06UL
#define VW_CH_IDX7  0x07UL
#define VW_CH_IDX40 0x40UL
#define VW_CH_IDX41 0x41UL
#define VW_CH_IDX42 0x42UL
#define VW_CH_IDX43 0x43UL
#define VW_CH_IDX44 0x44UL
#define VW_CH_IDX47 0x47UL
#define VW_CH_IDX4A 0x4AUL
#define VW_CH_IDX51 0x51UL
#define VW_CH_IDX61 0x61UL

struct espi_vw_channel {
	uint8_t vw_index;
	uint8_t level_mask;
	uint8_t valid_mask;
};

struct espi_vw_signal_t {
	enum espi_vwire_signal signal;
	void (*vw_signal_callback)(const struct device *dev);
};

#define VW_CH(signal, index, level, valid)                                                         \
	[signal] = {.vw_index = index, .level_mask = level, .valid_mask = valid}

static const struct espi_vw_channel vw_channel_list[] = {
	VW_CH(ESPI_VWIRE_SIGNAL_SLP_S3, VW_CH_IDX2, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_SLP_S4, VW_CH_IDX2, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_SLP_S5, VW_CH_IDX2, BIT(2), BIT(6)),
	VW_CH(ESPI_VWIRE_SIGNAL_OOB_RST_WARN, VW_CH_IDX3, BIT(2), BIT(6)),
	VW_CH(ESPI_VWIRE_SIGNAL_PLTRST, VW_CH_IDX3, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_SUS_STAT, VW_CH_IDX3, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_NMIOUT, VW_CH_IDX7, BIT(2), BIT(6)),
	VW_CH(ESPI_VWIRE_SIGNAL_SMIOUT, VW_CH_IDX7, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_HOST_RST_WARN, VW_CH_IDX7, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_SLP_A, VW_CH_IDX41, BIT(3), BIT(7)),
	VW_CH(ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK, VW_CH_IDX41, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_SUS_WARN, VW_CH_IDX41, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_SLP_WLAN, VW_CH_IDX42, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_SLP_LAN, VW_CH_IDX42, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_HOST_C10, VW_CH_IDX47, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_DNX_WARN, VW_CH_IDX4A, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_PME, VW_CH_IDX4, BIT(3), BIT(7)),
	VW_CH(ESPI_VWIRE_SIGNAL_WAKE, VW_CH_IDX4, BIT(2), BIT(6)),
	VW_CH(ESPI_VWIRE_SIGNAL_OOB_RST_ACK, VW_CH_IDX4, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS, VW_CH_IDX5, BIT(3), BIT(7)),
	VW_CH(ESPI_VWIRE_SIGNAL_ERR_NON_FATAL, VW_CH_IDX5, BIT(2), BIT(6)),
	VW_CH(ESPI_VWIRE_SIGNAL_ERR_FATAL, VW_CH_IDX5, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, VW_CH_IDX5, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_HOST_RST_ACK, VW_CH_IDX6, BIT(3), BIT(7)),
	VW_CH(ESPI_VWIRE_SIGNAL_RST_CPU_INIT, VW_CH_IDX6, BIT(2), BIT(6)),
	VW_CH(ESPI_VWIRE_SIGNAL_SMI, VW_CH_IDX6, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_SCI, VW_CH_IDX6, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_DNX_ACK, VW_CH_IDX40, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_SUS_ACK, VW_CH_IDX40, BIT(0), BIT(4)),
};

struct espi_vw_ch_cached {
	uint8_t idx2;
	uint8_t idx3;
	uint8_t idx7;
	uint8_t idx41;
	uint8_t idx42;
	uint8_t idx43;
	uint8_t idx44;
	uint8_t idx47;
	uint8_t idx4a;
	uint8_t idx51;
	uint8_t idx61;
};

struct espi_vw_tx_cached {
	uint8_t idx4;
	uint8_t idx5;
	uint8_t idx6;
	uint8_t idx40;
};

static struct espi_vw_ch_cached espi_vw_ch_cached_data;
static struct espi_vw_tx_cached espi_vw_tx_cached_data;

static int espi_rts5912_send_vwire(const struct device *dev, enum espi_vwire_signal signal,
				   uint8_t level);
static int espi_rts5912_receive_vwire(const struct device *dev, enum espi_vwire_signal signal,
				      uint8_t *level);
static int vw_signal_set_valid(const struct device *dev, enum espi_vwire_signal signal,
			       uint8_t valid);
static void notify_system_state(const struct device *dev, enum espi_vwire_signal signal);
static void notify_host_warning(const struct device *dev, enum espi_vwire_signal signal);
static void vw_slp3_handler(const struct device *dev);
static void vw_slp4_handler(const struct device *dev);
static void vw_slp5_handler(const struct device *dev);
static void vw_sus_stat_handler(const struct device *dev);
static void vw_pltrst_handler(const struct device *dev);
static void vw_oob_rst_warn_handler(const struct device *dev);
static void vw_host_rst_warn_handler(const struct device *dev);
static void vw_smiout_handler(const struct device *dev);
static void vw_nmiout_handler(const struct device *dev);
static void vw_sus_warn_handler(const struct device *dev);
static void vw_sus_pwrdn_ack_handler(const struct device *dev);
static void vw_sus_slp_a_handler(const struct device *dev);
static void vw_slp_lan_handler(const struct device *dev);
static void vw_slp_wlan_handler(const struct device *dev);
static void vw_host_c10_handler(const struct device *dev);
static void espi_vw_ch_setup(const struct device *dev);
static void espi_vw_ch_isr(const struct device *dev);
#ifdef CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE
static void send_target_bootdone(const struct device *dev);
#endif

static void espi_vw_ch_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *data = dev->data;

	struct espi_event evt = {.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				 .evt_details = ESPI_CHANNEL_VWIRE,
				 .evt_data = 0};

	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	uint32_t config = espi_reg->EVCFG;

	evt.evt_data = (config & ESPI_EVCFG_CHEN) ? 1 : 0;
	espi_send_callbacks(&data->callbacks, dev, evt);

	if (config & ESPI_EVCFG_CHEN) {
		vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS, 1);
		vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, 1);

#ifdef CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE
		send_target_bootdone(dev);
#endif
	}
	espi_reg->EVSTS = ESPI_EVSTS_RXIDXCHG;
}

static const struct espi_vw_signal_t vw_idx2_signals[] = {
	{ESPI_VWIRE_SIGNAL_SLP_S3, vw_slp3_handler},
	{ESPI_VWIRE_SIGNAL_SLP_S4, vw_slp4_handler},
	{ESPI_VWIRE_SIGNAL_SLP_S5, vw_slp5_handler},
};

static void espi_vw_idx2_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint8_t cur_idx_data = espi_reg->EVIDX2;
	uint8_t updated_bit = cur_idx_data ^ espi_vw_ch_cached_data.idx2;

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX2CHG) {
		espi_vw_ch_cached_data.idx2 = cur_idx_data;

		for (int i = 0; i < ARRAY_SIZE(vw_idx2_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx2_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask &&
			    vw_idx2_signals[i].vw_signal_callback != NULL) {
				vw_idx2_signals[i].vw_signal_callback(dev);
			}
		}
		if (espi_vw_ch_cached_data.idx2 == espi_reg->EVIDX2) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX2CHG;
		}
	}
}

static const struct espi_vw_signal_t vw_idx3_signals[] = {
	{ESPI_VWIRE_SIGNAL_SUS_STAT, vw_sus_stat_handler},
	{ESPI_VWIRE_SIGNAL_PLTRST, vw_pltrst_handler},
	{ESPI_VWIRE_SIGNAL_OOB_RST_WARN, vw_oob_rst_warn_handler},
};

static void espi_vw_idx3_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint8_t cur_idx_data = espi_reg->EVIDX3;
	uint8_t updated_bit = cur_idx_data ^ espi_vw_ch_cached_data.idx3;

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX3CHG) {
		espi_vw_ch_cached_data.idx3 = cur_idx_data;

		for (int i = 0; i < ARRAY_SIZE(vw_idx3_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx3_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask &&
			    vw_idx3_signals[i].vw_signal_callback != NULL) {
				vw_idx3_signals[i].vw_signal_callback(dev);
			}
		}
		if (espi_vw_ch_cached_data.idx3 == espi_reg->EVIDX3) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX3CHG;
		}
	}
}

static const struct espi_vw_signal_t vw_idx7_signals[] = {
	{ESPI_VWIRE_SIGNAL_HOST_RST_WARN, vw_host_rst_warn_handler},
	{ESPI_VWIRE_SIGNAL_SMIOUT, vw_smiout_handler},
	{ESPI_VWIRE_SIGNAL_NMIOUT, vw_nmiout_handler},
};

static void espi_vw_idx7_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint8_t cur_idx_data = espi_reg->EVIDX7;
	uint8_t updated_bit = cur_idx_data ^ espi_vw_ch_cached_data.idx7;

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX7CHG) {
		espi_vw_ch_cached_data.idx7 = cur_idx_data;

		for (int i = 0; i < ARRAY_SIZE(vw_idx7_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx7_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask &&
			    vw_idx7_signals[i].vw_signal_callback != NULL) {
				vw_idx7_signals[i].vw_signal_callback(dev);
			}
		}
		if (espi_vw_ch_cached_data.idx7 == espi_reg->EVIDX7) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX7CHG;
		}
	}
}

static const struct espi_vw_signal_t vw_idx41_signals[] = {
	{ESPI_VWIRE_SIGNAL_SUS_WARN, vw_sus_warn_handler},
	{ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK, vw_sus_pwrdn_ack_handler},
	{ESPI_VWIRE_SIGNAL_SLP_A, vw_sus_slp_a_handler},
};

static void espi_vw_idx41_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint8_t cur_idx_data = espi_reg->EVIDX41;
	uint8_t updated_bit = cur_idx_data ^ espi_vw_ch_cached_data.idx41;

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX41CHG) {
		espi_vw_ch_cached_data.idx41 = cur_idx_data;
		for (int i = 0; i < ARRAY_SIZE(vw_idx41_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx41_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask &&
			    vw_idx41_signals[i].vw_signal_callback != NULL) {
				vw_idx41_signals[i].vw_signal_callback(dev);
			}
		}
		if (espi_vw_ch_cached_data.idx41 == espi_reg->EVIDX41) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX41CHG;
		}
	}
}

static const struct espi_vw_signal_t vw_idx42_signals[] = {
	{ESPI_VWIRE_SIGNAL_SLP_LAN, vw_slp_lan_handler},
	{ESPI_VWIRE_SIGNAL_SLP_WLAN, vw_slp_wlan_handler},
};

static void espi_vw_idx42_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint8_t cur_idx_data = espi_reg->EVIDX42;
	uint8_t updated_bit = cur_idx_data ^ espi_vw_ch_cached_data.idx42;

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX42CHG) {
		espi_vw_ch_cached_data.idx42 = cur_idx_data;

		for (int i = 0; i < ARRAY_SIZE(vw_idx42_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx42_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask &&
			    vw_idx42_signals[i].vw_signal_callback != NULL) {
				vw_idx42_signals[i].vw_signal_callback(dev);
			}
		}
		if (espi_vw_ch_cached_data.idx42 == espi_reg->EVIDX42) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX42CHG;
		}
	}
}

static const struct espi_vw_signal_t vw_idx43_signals[] = {};

static void espi_vw_idx43_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint8_t cur_idx_data = espi_reg->EVIDX43;
	uint8_t updated_bit = cur_idx_data ^ espi_vw_ch_cached_data.idx43;

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX43CHG) {
		espi_vw_ch_cached_data.idx43 = cur_idx_data;

		for (int i = 0; i < ARRAY_SIZE(vw_idx43_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx43_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask &&
			    vw_idx43_signals[i].vw_signal_callback != NULL) {
				vw_idx43_signals[i].vw_signal_callback(dev);
			}
		}
		if (espi_vw_ch_cached_data.idx43 == espi_reg->EVIDX43) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX43CHG;
		}
	}
}

static const struct espi_vw_signal_t vw_idx44_signals[] = {};

static void espi_vw_idx44_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint8_t cur_idx_data = espi_reg->EVIDX44;
	uint8_t updated_bit = cur_idx_data ^ espi_vw_ch_cached_data.idx44;

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX44CHG) {
		espi_vw_ch_cached_data.idx44 = cur_idx_data;

		for (int i = 0; i < ARRAY_SIZE(vw_idx44_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx44_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask &&
			    vw_idx44_signals[i].vw_signal_callback != NULL) {
				vw_idx44_signals[i].vw_signal_callback(dev);
			}
		}
		if (espi_vw_ch_cached_data.idx44 == espi_reg->EVIDX44) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX44CHG;
		}
	}
}

static const struct espi_vw_signal_t vw_idx47_signals[] = {
	{ESPI_VWIRE_SIGNAL_HOST_C10, vw_host_c10_handler},
};

static void espi_vw_idx47_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint8_t cur_idx_data = espi_reg->EVIDX47;
	uint8_t updated_bit = cur_idx_data ^ espi_vw_ch_cached_data.idx47;

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX47CHG) {
		espi_vw_ch_cached_data.idx47 = cur_idx_data;

		for (int i = 0; i < ARRAY_SIZE(vw_idx47_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx47_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask &&
			    vw_idx47_signals[i].vw_signal_callback != NULL) {
				vw_idx47_signals[i].vw_signal_callback(dev);
			}
		}
		if (espi_vw_ch_cached_data.idx47 == espi_reg->EVIDX47) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX47CHG;
		}
	}
}

static void espi_vw_idx4a_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void espi_vw_idx51_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void espi_vw_idx61_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int vw_signal_set_valid(const struct device *dev, enum espi_vwire_signal signal,
			       uint8_t valid)
{
	uint8_t vw_index = vw_channel_list[signal].vw_index;
	uint8_t valid_mask = vw_channel_list[signal].valid_mask;

	if (signal > ARRAY_SIZE(vw_channel_list)) {
		return -EIO;
	}

	switch (vw_index) {
	case VW_CH_IDX4:
		if (valid) {
			espi_vw_tx_cached_data.idx4 |= valid_mask;
		} else {
			espi_vw_tx_cached_data.idx4 &= ~valid_mask;
		}
		break;
	case VW_CH_IDX5:
		if (valid) {
			espi_vw_tx_cached_data.idx5 |= valid_mask;
		} else {
			espi_vw_tx_cached_data.idx5 &= ~valid_mask;
		}
		break;
	case VW_CH_IDX6:
		if (valid) {
			espi_vw_tx_cached_data.idx6 |= valid_mask;
		} else {
			espi_vw_tx_cached_data.idx6 &= ~valid_mask;
		}
		break;
	case VW_CH_IDX40:
		if (valid) {
			espi_vw_tx_cached_data.idx40 |= valid_mask;
		} else {
			espi_vw_tx_cached_data.idx40 &= ~valid_mask;
		}
		break;
	default:
		return -EIO;
	}

	return 0;
}

static void vw_ch_isr_wa_cb(struct k_work *work)
{
	espi_vw_ch_isr(DEVICE_DT_GET(DT_DRV_INST(0)));
}
static K_WORK_DELAYABLE_DEFINE(vw_ch_isr_wa, vw_ch_isr_wa_cb);

#ifdef CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE
static void send_target_bootdone(const struct device *dev)
{
	int ret;
	uint8_t boot_done;

	ret = espi_rts5912_receive_vwire(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, &boot_done);
	if (!ret && !boot_done) {
		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS, 1);
		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, 1);
		k_work_cancel_delayable(&vw_ch_isr_wa);
	}
}
#endif

static void notify_system_state(const struct device *dev, enum espi_vwire_signal signal)
{
	struct espi_rts5912_data *data = dev->data;
	struct espi_event evt = {ESPI_BUS_EVENT_VWIRE_RECEIVED, 0, 0};

	uint8_t status = 0;

	espi_rts5912_receive_vwire(dev, signal, &status);

	evt.evt_details = signal;
	evt.evt_data = status;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void notify_host_warning(const struct device *dev, enum espi_vwire_signal signal)
{
	uint8_t status = 0;

	espi_rts5912_receive_vwire(dev, signal, &status);
	k_busy_wait(200);

	switch (signal) {
	case ESPI_VWIRE_SIGNAL_SUS_WARN:
		vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_SUS_ACK, 1);
		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_SUS_ACK, status);
		break;
	case ESPI_VWIRE_SIGNAL_OOB_RST_WARN:
		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_OOB_RST_ACK, status);
		break;
	case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, status);
		break;
	default:
		break;
	}
}

static void vw_slp3_handler(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SLP_S3);
}

static void vw_slp4_handler(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SLP_S4);
}

static void vw_slp5_handler(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SLP_S5);
}

static void vw_sus_stat_handler(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SUS_STAT);
}

static void vw_pltrst_handler(const struct device *dev)
{
	struct espi_rts5912_data *data = dev->data;

	struct espi_event evt = {ESPI_BUS_EVENT_VWIRE_RECEIVED, ESPI_VWIRE_SIGNAL_PLTRST, 0};
	uint8_t status = 0;

	espi_rts5912_receive_vwire(dev, ESPI_VWIRE_SIGNAL_PLTRST, &status);

	if (status) {
		vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_SMI, 1);
		vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_SCI, 1);
		vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, 1);
		vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_RST_CPU_INIT, 1);

		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_SMI, 1);
		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_SCI, 1);
		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, 1);
		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_RST_CPU_INIT, 1);
	}

	evt.evt_data = status;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void vw_oob_rst_warn_handler(const struct device *dev)
{
	if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
		notify_system_state(dev, ESPI_VWIRE_SIGNAL_OOB_RST_WARN);
	} else {
		notify_host_warning(dev, ESPI_VWIRE_SIGNAL_OOB_RST_WARN);
	}
}

static void vw_host_rst_warn_handler(const struct device *dev)
{
	if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
		notify_system_state(dev, ESPI_VWIRE_SIGNAL_HOST_RST_WARN);
	} else {
		notify_host_warning(dev, ESPI_VWIRE_SIGNAL_HOST_RST_WARN);
	}
}

static void vw_smiout_handler(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SMIOUT);
}

static void vw_nmiout_handler(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_NMIOUT);
}

static void vw_sus_warn_handler(const struct device *dev)
{
	if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
		notify_system_state(dev, ESPI_VWIRE_SIGNAL_SUS_WARN);
	} else {
		notify_host_warning(dev, ESPI_VWIRE_SIGNAL_SUS_WARN);
	}
}

static void vw_sus_pwrdn_ack_handler(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK);
}

static void vw_sus_slp_a_handler(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SLP_A);
}

static void vw_slp_lan_handler(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SLP_LAN);
}

static void vw_slp_wlan_handler(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_SLP_WLAN);
}

static void vw_host_c10_handler(const struct device *dev)
{
	notify_system_state(dev, ESPI_VWIRE_SIGNAL_HOST_C10);
}

#define VW_TIMEOUT_US 1000

static int espi_rts5912_send_vwire(const struct device *dev, enum espi_vwire_signal signal,
				   uint8_t level)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	uint8_t vw_idx = vw_channel_list[signal].vw_index;
	uint8_t lev_msk = vw_channel_list[signal].level_mask;
	uint32_t tx_data = 0;

	if (signal > ARRAY_SIZE(vw_channel_list)) {
		return -EIO;
	}

	switch (vw_idx) {
	case VW_CH_IDX4:
		tx_data = espi_vw_tx_cached_data.idx4;
		break;
	case VW_CH_IDX5:
		tx_data = espi_vw_tx_cached_data.idx5;
		break;
	case VW_CH_IDX6:
		tx_data = espi_vw_tx_cached_data.idx6;
		break;
	case VW_CH_IDX40:
		tx_data = espi_vw_tx_cached_data.idx40;
		break;
	default:
		return -EIO;
	}

	tx_data |= (vw_idx << 8);

	if (level) {
		tx_data |= lev_msk;
	} else {
		tx_data &= ~lev_msk;
	}

	if (espi_reg->EVSTS & ESPI_EVSTS_TXFULL) {
		return -EIO;
	}

	espi_reg->EVTXDAT = tx_data;

	WAIT_FOR(!(espi_reg->EVSTS & ESPI_EVSTS_TXFULL), VW_TIMEOUT_US, k_busy_wait(10));

	switch (vw_idx) {
	case VW_CH_IDX4:
		espi_vw_tx_cached_data.idx4 = tx_data;
		break;
	case VW_CH_IDX5:
		espi_vw_tx_cached_data.idx5 = tx_data;
		break;
	case VW_CH_IDX6:
		espi_vw_tx_cached_data.idx6 = tx_data;
		break;
	case VW_CH_IDX40:
		espi_vw_tx_cached_data.idx40 = tx_data;
		break;
	default:
		return -EIO;
	}

	return 0;
}

static int espi_rts5912_receive_vwire(const struct device *dev, enum espi_vwire_signal signal,
				      uint8_t *level)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint8_t vw_idx, lev_msk, valid_msk;
	uint8_t vw_data;

	vw_idx = vw_channel_list[signal].vw_index;
	lev_msk = vw_channel_list[signal].level_mask;
	valid_msk = vw_channel_list[signal].valid_mask;

	if (signal > ARRAY_SIZE(vw_channel_list)) {
		return -EIO;
	}

	switch (vw_idx) {
	case VW_CH_IDX2:
		vw_data = espi_vw_ch_cached_data.idx2;
		break;
	case VW_CH_IDX3:
		if (espi_vw_ch_cached_data.idx3 != espi_reg->EVIDX3) {
			espi_vw_ch_cached_data.idx3 = espi_reg->EVIDX3;
		}
		vw_data = espi_vw_ch_cached_data.idx3;
		break;
	case VW_CH_IDX4:
		vw_data = espi_vw_tx_cached_data.idx4;
		break;
	case VW_CH_IDX5:
		vw_data = espi_vw_tx_cached_data.idx5;
		break;
	case VW_CH_IDX6:
		vw_data = espi_vw_tx_cached_data.idx6;
		break;
	case VW_CH_IDX7:
		if (espi_vw_ch_cached_data.idx7 != espi_reg->EVIDX7) {
			espi_vw_ch_cached_data.idx7 = espi_reg->EVIDX7;
		}
		vw_data = espi_vw_ch_cached_data.idx7;
		break;
	case VW_CH_IDX40:
		vw_data = espi_vw_tx_cached_data.idx40;
		break;
	case VW_CH_IDX41:
		if (espi_vw_ch_cached_data.idx41 != espi_reg->EVIDX41) {
			espi_vw_ch_cached_data.idx41 = espi_reg->EVIDX41;
		}
		vw_data = espi_vw_ch_cached_data.idx41;
		break;
	case VW_CH_IDX42:
		if (espi_vw_ch_cached_data.idx42 != espi_reg->EVIDX42) {
			espi_vw_ch_cached_data.idx42 = espi_reg->EVIDX42;
		}
		vw_data = espi_vw_ch_cached_data.idx42;
		break;
	case VW_CH_IDX43:
		if (espi_vw_ch_cached_data.idx43 != espi_reg->EVIDX43) {
			espi_vw_ch_cached_data.idx43 = espi_reg->EVIDX43;
		}
		vw_data = espi_vw_ch_cached_data.idx43;
		break;
	case VW_CH_IDX44:
		if (espi_vw_ch_cached_data.idx44 != espi_reg->EVIDX44) {
			espi_vw_ch_cached_data.idx44 = espi_reg->EVIDX44;
		}
		vw_data = espi_vw_ch_cached_data.idx44;
		break;
	case VW_CH_IDX47:
		if (espi_vw_ch_cached_data.idx47 != espi_reg->EVIDX47) {
			espi_vw_ch_cached_data.idx47 = espi_reg->EVIDX47;
		}
		vw_data = espi_vw_ch_cached_data.idx47;
		break;
	case VW_CH_IDX4A:
		if (espi_vw_ch_cached_data.idx4a != espi_reg->EVIDX4A) {
			espi_vw_ch_cached_data.idx4a = espi_reg->EVIDX4A;
		}
		vw_data = espi_vw_ch_cached_data.idx4a;
		break;
	case VW_CH_IDX51:
		if (espi_vw_ch_cached_data.idx51 != espi_reg->EVIDX51) {
			espi_vw_ch_cached_data.idx51 = espi_reg->EVIDX51;
		}
		vw_data = espi_vw_ch_cached_data.idx51;
		break;
	case VW_CH_IDX61:
		if (espi_vw_ch_cached_data.idx61 != espi_reg->EVIDX61) {
			espi_vw_ch_cached_data.idx61 = espi_reg->EVIDX61;
		}
		vw_data = espi_vw_ch_cached_data.idx61;
		break;
	default:
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_ESPI_VWIRE_VALID_BIT_CHECK)) {
		if (vw_data & valid_msk) {
			*level = !!(vw_data & lev_msk);
		} else {
			/* Not valid */
			*level = 0;
		}
	} else {
		*level = !!(vw_data & lev_msk);
	}

	return 0;
}

static void espi_vw_ch_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	espi_reg->EVSTS |= ESPI_EVSTS_RXIDXCLR;

	espi_vw_ch_cached_data.idx2 = espi_reg->EVIDX2;
	espi_vw_ch_cached_data.idx3 = espi_reg->EVIDX3;
	espi_vw_ch_cached_data.idx7 = espi_reg->EVIDX7;
	espi_vw_ch_cached_data.idx41 = espi_reg->EVIDX41;
	espi_vw_ch_cached_data.idx42 = espi_reg->EVIDX42;
	espi_vw_ch_cached_data.idx43 = espi_reg->EVIDX43;
	espi_vw_ch_cached_data.idx44 = espi_reg->EVIDX44;
	espi_vw_ch_cached_data.idx47 = espi_reg->EVIDX47;
	espi_vw_ch_cached_data.idx4a = espi_reg->EVIDX4A;
	espi_vw_ch_cached_data.idx51 = espi_reg->EVIDX51;
	espi_vw_ch_cached_data.idx61 = espi_reg->EVIDX61;

	espi_vw_tx_cached_data.idx4 = 0;
	espi_vw_tx_cached_data.idx5 = 0;
	espi_vw_tx_cached_data.idx6 = 0;
	espi_vw_tx_cached_data.idx40 = 0;

	espi_reg->EVRXINTEN = (ESPI_EVRXINTEN_CFGCHGEN | ESPI_EVRXINTEN_RXCHGEN);

	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_ch, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx2, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx3, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx7, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx41, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx42, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx43, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx44, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx47, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx4a, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx51, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx61, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_ch, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_ch, priority), espi_vw_ch_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_ch, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx2, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx2, priority), espi_vw_idx2_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx2, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx3, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx3, priority), espi_vw_idx3_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx3, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx7, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx7, priority), espi_vw_idx7_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx7, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx41, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx41, priority), espi_vw_idx41_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx41, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx42, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx42, priority), espi_vw_idx42_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx42, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx43, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx43, priority), espi_vw_idx43_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx43, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx44, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx44, priority), espi_vw_idx44_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx44, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx47, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx47, priority), espi_vw_idx47_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx47, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx4a, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx4a, priority), espi_vw_idx4a_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx4a, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx51, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx51, priority), espi_vw_idx51_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx51, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx61, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx61, priority), espi_vw_idx61_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), vw_idx61, irq));
}

#endif /* CONFIG_ESPI_VWIRE_CHANNEL */

/*
 * =========================================================================
 * ESPI OOB channel
 * =========================================================================
 */

#ifdef CONFIG_ESPI_OOB_CHANNEL

#define MAX_OOB_TIMEOUT 200UL /* ms */
#define OOB_BUFFER_SIZE 256UL

static int espi_rts5912_send_oob(const struct device *dev, struct espi_oob_packet *pckt)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	int ret;

	if (!(espi_reg->EOCFG & ESPI_EOCFG_CHRDY)) {
		LOG_ERR("%s: OOB channel isn't ready", __func__);
		return -EIO;
	}

	if (espi_data->oob_tx_busy) {
		LOG_ERR("%s: OOB channel is busy", __func__);
		return -EIO;
	}

	if (pckt->len > OOB_BUFFER_SIZE) {
		LOG_ERR("%s: OOB Tx have no insufficient space", __func__);
		return -EINVAL;
	}

	for (int i = 0; i < pckt->len; i++) {
		espi_data->oob_tx_ptr[i] = pckt->buf[i];
	}

	espi_reg->EOTXLEN = pckt->len - 1;
	espi_reg->EOTXCTRL = ESPI_EOTXCTRL_TXSTR;

	espi_data->oob_tx_busy = true;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&espi_data->oob_tx_lock, K_MSEC(MAX_OOB_TIMEOUT));
	if (ret == -EAGAIN) {
		return -ETIMEDOUT;
	}

	espi_data->oob_tx_busy = false;

	return 0;
}

static int espi_rts5912_receive_oob(const struct device *dev, struct espi_oob_packet *pckt)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint32_t rx_len;

	if (!(espi_reg->EOCFG & ESPI_EOCFG_CHRDY)) {
		LOG_ERR("%s: OOB channel isn't ready", __func__);
		return -EIO;
	}

	if (espi_reg->EOSTS & ESPI_EOSTS_RXPND) {
		LOG_ERR("OOB Receive Pending");
		return -EIO;
	}

#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	/* Wait until ISR or timeout */
	int ret = k_sem_take(&espi_data->oob_rx_lock, K_MSEC(MAX_OOB_TIMEOUT));

	if (ret == -EAGAIN) {
		LOG_ERR("OOB Rx Timeout");
		return -ETIMEDOUT;
	}
#endif

	/* Check if buffer passed to driver can fit the received buffer */
	rx_len = espi_reg->EORXLEN;

	if (rx_len > pckt->len) {
		LOG_ERR("space rcvd %d vs %d", rx_len, pckt->len);
		return -EIO;
	}

	pckt->len = rx_len;

	for (int i = 0; i < rx_len; i++) {
		pckt->buf[i] = espi_data->oob_rx_ptr[i];
	}

	return 0;
}

static void espi_oob_tx_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint32_t status = espi_reg->EOSTS;

	if (status & ESPI_EOSTS_TXDONE) {
		k_sem_give(&espi_data->oob_tx_lock);
		espi_reg->EOSTS = ESPI_EOSTS_TXDONE;
	}
}

static void espi_oob_rx_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint32_t status = espi_reg->EOSTS;

#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	struct espi_event evt = {
		.evt_type = ESPI_BUS_EVENT_OOB_RECEIVED, .evt_details = 0, .evt_data = 0};
#endif

	if (status & ESPI_EOSTS_RXDONE) {
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
		k_sem_give(&espi_data->oob_rx_lock);
#else
		k_busy_wait(250);
		evt.evt_details = espi_reg->EORXLEN;
		espi_send_callbacks(&espi_data->callbacks, dev, evt);
#endif
		espi_reg->EOSTS = ESPI_EOSTS_RXDONE;
	}
}

static void espi_oob_chg_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;

	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	struct espi_event evt = {.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				 .evt_details = ESPI_CHANNEL_OOB,
				 .evt_data = 0};

	uint32_t status = espi_reg->EOSTS;
	uint32_t config = espi_reg->EOCFG;

	if (status & ESPI_EOSTS_CFGENCHG) {
		evt.evt_data = config & ESPI_EVCFG_CHEN ? 1 : 0;
		espi_send_callbacks(&espi_data->callbacks, dev, evt);

		if (config & ESPI_EVCFG_CHEN) {
			vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_OOB_RST_ACK, 1);
		}

		espi_reg->EOSTS = ESPI_EOSTS_CFGENCHG;
	}
}

static uint8_t oob_tx_buffer[OOB_BUFFER_SIZE] __aligned(4);
static uint8_t oob_rx_buffer[OOB_BUFFER_SIZE] __aligned(4);

static int espi_oob_ch_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	espi_data->oob_tx_busy = false;

	espi_data->oob_tx_ptr = oob_tx_buffer;
	if (espi_data->oob_tx_ptr == NULL) {
		LOG_ERR("Failed to allocate OOB Tx buffer");
		return -ENOMEM;
	}

	espi_data->oob_rx_ptr = oob_rx_buffer;
	if (espi_data->oob_tx_ptr == NULL) {
		LOG_ERR("Failed to allocate OOB Rx buffer");
		return -ENOMEM;
	}

	espi_reg->EOTXBUF = (uint32_t)espi_data->oob_tx_ptr;
	espi_reg->EORXBUF = (uint32_t)espi_data->oob_rx_ptr;

	espi_reg->EOTXINTEN = ESPI_EOTXINTEN_TXEN;
	espi_reg->EORXINTEN = (ESPI_EORXINTEN_RXEN | ESPI_EORXINTEN_CHENCHG);

	k_sem_init(&espi_data->oob_tx_lock, 0, 1);
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	k_sem_init(&espi_data->oob_rx_lock, 0, 1);
#endif

	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_tx, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_rx, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_chg, irq));

	/* Tx */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_tx, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_tx, priority), espi_oob_tx_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_tx, irq));

	/* Rx */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_rx, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_rx, priority), espi_oob_rx_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_rx, irq));

	/* Chg */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_chg, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_chg, priority), espi_oob_chg_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_chg, irq));

	return 0;
}

#endif /* CONFIG_ESPI_OOB_CHANNEL */

/*
 * =========================================================================
 * ESPI flash channel
 * =========================================================================
 */

#ifdef CONFIG_ESPI_FLASH_CHANNEL

#define MAX_FLASH_TIMEOUT 1000UL
#define MAF_BUFFER_SIZE   512UL

enum {
	MAF_TR_READ = 0,
	MAF_TR_WRITE = 1,
	MAF_TR_ERASE = 2,
};

static int espi_rts5912_flash_read(const struct device *dev, struct espi_flash_packet *pckt)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	int ret;
	uint32_t ctrl;

	if (!(espi_reg->EFCONF & ESPI_EFCONF_CHEN)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if (pckt->len > MAF_BUFFER_SIZE) {
		LOG_ERR("Invalid size request");
		return -EINVAL;
	}

	if (espi_reg->EMCTRL & ESPI_EMCTRL_START) {
		LOG_ERR("Channel still busy");
		return -EBUSY;
	}

	ctrl = (MAF_TR_READ << ESPI_EMCTRL_MDSEL_Pos) | ESPI_EMCTRL_START;

	espi_reg->EMADR = pckt->flash_addr;
	espi_reg->EMTRLEN = pckt->len;
	espi_reg->EMCTRL = ctrl;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&espi_data->flash_lock, K_MSEC(MAX_FLASH_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	for (int i = 0; i < pckt->len; i++) {
		pckt->buf[i] = espi_data->maf_ptr[i];
	}

	return 0;
}

static int espi_rts5912_flash_write(const struct device *dev, struct espi_flash_packet *pckt)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	int ret;
	uint32_t ctrl;

	if (!(espi_reg->EFCONF & ESPI_EFCONF_CHEN)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if (pckt->len > MAF_BUFFER_SIZE) {
		LOG_ERR("Packet length is too big");
		return -EINVAL;
	}

	if (espi_reg->EMCTRL & ESPI_EMCTRL_START) {
		LOG_ERR("Channel still busy");
		return -EBUSY;
	}

	for (int i = 0; i < pckt->len; i++) {
		espi_data->maf_ptr[i] = pckt->buf[i];
	}

	ctrl = (MAF_TR_WRITE << ESPI_EMCTRL_MDSEL_Pos) | ESPI_EMCTRL_START;

	espi_reg->EMADR = pckt->flash_addr;
	espi_reg->EMTRLEN = pckt->len;
	espi_reg->EMCTRL = ctrl;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&espi_data->flash_lock, K_MSEC(MAX_FLASH_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int espi_rts5912_flash_erase(const struct device *dev, struct espi_flash_packet *pckt)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	int ret;
	uint32_t ctrl;

	if (!(espi_reg->EFCONF & ESPI_EFCONF_CHEN)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if (espi_reg->EMCTRL & ESPI_EMCTRL_START) {
		LOG_ERR("Channel still busy");
		return -EBUSY;
	}

	ctrl = (MAF_TR_ERASE << ESPI_EMCTRL_MDSEL_Pos) | ESPI_EMCTRL_START;

	espi_reg->EMADR = pckt->flash_addr;
	espi_reg->EMTRLEN = pckt->len;
	espi_reg->EMCTRL = ctrl;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&espi_data->flash_lock, K_MSEC(MAX_FLASH_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static void espi_maf_tr_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint32_t status = espi_reg->EFSTS;

	if (status & ESPI_EFSTS_MAFTXDN) {
		k_sem_give(&espi_data->flash_lock);
		espi_reg->EFSTS = ESPI_EFSTS_MAFTXDN;
	}
}

static void espi_flash_chg_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	struct espi_event evt = {.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				 .evt_details = ESPI_CHANNEL_FLASH,
				 .evt_data = 0};

	uint32_t status = espi_reg->EFSTS;
	uint32_t config = espi_reg->EFCONF;

	if (status & ESPI_EFSTS_CHENCHG) {
		evt.evt_data = (config & ESPI_EFCONF_CHEN) ? 1 : 0;
		espi_send_callbacks(&espi_data->callbacks, dev, evt);

		espi_reg->EFSTS = ESPI_EFSTS_CHENCHG;
	}
}

static uint8_t flash_channel_buffer[MAF_BUFFER_SIZE] __aligned(4);

static int espi_flash_ch_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	espi_data->maf_ptr = flash_channel_buffer;
	if (espi_data->maf_ptr == NULL) {
		LOG_ERR("Failed to allocate MAF buffer");
		return -ENOMEM;
	}

	espi_reg->EMBUF = (uint32_t)espi_data->maf_ptr;
	espi_reg->EMINTEN = ESPI_EMINTEN_CHENCHG | ESPI_EMINTEN_TRDONEEN;

	k_sem_init(&espi_data->flash_lock, 0, 1);

	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), maf_tr, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), flash_chg, irq));

	/* MAF Tr */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), maf_tr, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), maf_tr, priority), espi_maf_tr_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), maf_tr, irq));

	/* Chg */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), flash_chg, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), flash_chg, priority), espi_flash_chg_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), flash_chg, irq));

	return 0;
}

#endif /* CONFIG_ESPI_FLASH_CHANNEL */

/*
 * =========================================================================
 * ESPI common function and API
 * =========================================================================
 */

#define RTS5912_ESPI_MAX_FREQ_20 20
#define RTS5912_ESPI_MAX_FREQ_25 25
#define RTS5912_ESPI_MAX_FREQ_33 33
#define RTS5912_ESPI_MAX_FREQ_50 50
#define RTS5912_ESPI_MAX_FREQ_66 66

static int espi_rts5912_configure(const struct device *dev, struct espi_cfg *cfg)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	uint32_t gen_conf = 0;
	uint8_t io_mode = 0;

	/* Maximum Frequency Supported */
	switch (cfg->max_freq) {
	case RTS5912_ESPI_MAX_FREQ_20:
		gen_conf |= 0UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case RTS5912_ESPI_MAX_FREQ_25:
		gen_conf |= 1UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case RTS5912_ESPI_MAX_FREQ_33:
		gen_conf |= 2UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case RTS5912_ESPI_MAX_FREQ_50:
		gen_conf |= 3UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case RTS5912_ESPI_MAX_FREQ_66:
		gen_conf |= 4UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	default:
		return -EINVAL;
	}

	/* I/O Mode Supported */
	io_mode = cfg->io_caps >> 1;

	if (io_mode > 3) {
		return -EINVAL;
	}
	gen_conf |= io_mode << ESPI_ESPICFG_IOSUP_Pos;

	/* Channel Supported */
	if (cfg->channel_caps & ESPI_CHANNEL_PERIPHERAL) {
		gen_conf |= BIT(0) << ESPI_ESPICFG_CHSUP_Pos;
	}

	if (cfg->channel_caps & ESPI_CHANNEL_VWIRE) {
		gen_conf |= BIT(1) << ESPI_ESPICFG_CHSUP_Pos;
	}

	if (cfg->channel_caps & ESPI_CHANNEL_OOB) {
		gen_conf |= BIT(2) << ESPI_ESPICFG_CHSUP_Pos;
	}

	if (cfg->channel_caps & ESPI_CHANNEL_FLASH) {
		gen_conf |= BIT(3) << ESPI_ESPICFG_CHSUP_Pos;
	}

	espi_reg->ESPICFG = gen_conf;
	data->config_data = espi_reg->ESPICFG;

	return 0;
}

static bool espi_rts5912_channel_ready(const struct device *dev, enum espi_channel ch)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	switch (ch) {
	case ESPI_CHANNEL_PERIPHERAL:
		return espi_reg->EPCFG & ESPI_EPCFG_CHEN ? true : false;
	case ESPI_CHANNEL_VWIRE:
		return espi_reg->EVCFG & ESPI_EVCFG_CHEN ? true : false;
	case ESPI_CHANNEL_OOB:
		return espi_reg->EOCFG & ESPI_EOCFG_CHEN ? true : false;
	case ESPI_CHANNEL_FLASH:
		return espi_reg->EFCONF & ESPI_EFCONF_CHEN ? true : false;
	default:
		return false;
	}
}

static int espi_rts5912_manage_callback(const struct device *dev, struct espi_callback *callback,
					bool set)
{
	struct espi_rts5912_data *data = dev->data;

	return espi_manage_callback(&data->callbacks, callback, set);
}

static DEVICE_API(espi, espi_rts5912_driver_api) = {
	.config = espi_rts5912_configure,
	.get_channel_status = espi_rts5912_channel_ready,
	.manage_callback = espi_rts5912_manage_callback,
#ifdef CONFIG_ESPI_VWIRE_CHANNEL
	.send_vwire = espi_rts5912_send_vwire,
	.receive_vwire = espi_rts5912_receive_vwire,
#endif
#ifdef CONFIG_ESPI_OOB_CHANNEL
	.send_oob = espi_rts5912_send_oob,
	.receive_oob = espi_rts5912_receive_oob,
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	.flash_read = espi_rts5912_flash_read,
	.flash_write = espi_rts5912_flash_write,
	.flash_erase = espi_rts5912_flash_erase,
#endif
};

static void espi_vw_ch_setup(const struct device *dev);

#define VW_RESET_DELAY 150UL

static void espi_rst_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *data = dev->data;

	struct espi_event evt = {.evt_type = ESPI_BUS_RESET, .evt_details = 0, .evt_data = 0};

	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint32_t status = espi_reg->ERSTCFG;

	espi_reg->ERSTCFG |= ESPI_ERSTCFG_RSTSTS;
	espi_reg->ERSTCFG ^= ESPI_ERSTCFG_RSTPOL;

	if (status & ESPI_ERSTCFG_RSTSTS) {
		if (status & ESPI_ERSTCFG_RSTPOL) {
			/* rst pin high go low trigger interrupt */
			evt.evt_data = 0;
		} else {
			/* rst pin low go high trigger interrupt */
			evt.evt_data = 1;
#ifdef CONFIG_ESPI_VWIRE_CHANNEL
			espi_vw_ch_setup(dev);
			espi_reg->ESPICFG = data->config_data;
			if (espi_reg->EVCFG & ESPI_EVCFG_CHEN) {
				k_timeout_t delay = K_MSEC(VW_RESET_DELAY);

				k_work_schedule(&vw_ch_isr_wa, delay);
			}
#endif
		}
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}

static void espi_bus_reset_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	espi_reg->ERSTCFG = ESPI_ERSTCFG_RSTINTEN;
	espi_reg->ERSTCFG = ESPI_ERSTCFG_RSTMONEN;

	if (espi_reg->ERSTCFG & ESPI_ERSTCFG_RSTSTS) {
		/* high to low */
		espi_reg->ERSTCFG =
			ESPI_ERSTCFG_RSTMONEN | ESPI_ERSTCFG_RSTPOL | ESPI_ERSTCFG_RSTINTEN;
	} else {
		/* low to high */
		espi_reg->ERSTCFG = ESPI_ERSTCFG_RSTMONEN | ESPI_ERSTCFG_RSTINTEN;
	}

	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), bus_rst, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), bus_rst, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), bus_rst, priority), espi_rst_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), bus_rst, irq));
}

static int espi_rts5912_init(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct rts5912_sccon_subsys sccon;

	int rc;

	/* Setup eSPI pins */
	rc = pinctrl_apply_state(espi_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		LOG_ERR("eSPI pinctrl setup failed (%d)", rc);
		return rc;
	}

	if (!device_is_ready(espi_config->clk_dev)) {
		LOG_ERR("eSPI clock not ready");
		return -ENODEV;
	}

	/* Enable eSPI clock */
	sccon.clk_grp = espi_config->espislv_clk_grp;
	sccon.clk_idx = espi_config->espislv_clk_idx;
	rc = clock_control_on(espi_config->clk_dev, (clock_control_subsys_t)&sccon);
	if (rc != 0) {
		LOG_ERR("eSPI clock control on failed");
		goto exit;
	}

	/* Setup eSPI bus reset */
	espi_bus_reset_setup(dev);

#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	/* Setup Port80 */
	rc = espi_peri_ch_port80_setup(dev);
	if (rc != 0) {
		LOG_ERR("eSPI Port80 setup failed");
		goto exit;
	}
#endif

#ifdef CONFIG_ESPI_VWIRE_CHANNEL
	/* Setup eSPI virtual-wire channel */
	espi_vw_ch_setup(dev);
#endif

#ifdef CONFIG_ESPI_OOB_CHANNEL
	/* Setup eSPI OOB channel */
	rc = espi_oob_ch_setup(dev);
	if (rc != 0) {
		LOG_ERR("eSPI OOB channel setup failed");
		goto exit;
	}
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	/* Setup eSPI flash channel */
	rc = espi_flash_ch_setup(dev);
	if (rc != 0) {
		LOG_ERR("eSPI flash channel setup failed");
		goto exit;
	}
#endif

exit:
	return rc;
}

PINCTRL_DT_INST_DEFINE(0);

static struct espi_rts5912_data espi_rts5912_data_0;

static const struct espi_rts5912_config espi_rts5912_config = {
	.espi_reg = (volatile struct espi_reg *const)DT_INST_REG_ADDR_BY_NAME(0, espi_target),
	.espislv_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(0), espi_target, clk_grp),
	.espislv_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(0), espi_target, clk_idx),
#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	.port80_reg = (volatile struct port80_reg *const)DT_INST_REG_ADDR_BY_NAME(0, port80),
	.port80_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(0), port80, clk_grp),
	.port80_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(0), port80, clk_idx),
#endif
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

DEVICE_DT_INST_DEFINE(0, &espi_rts5912_init, NULL, &espi_rts5912_data_0, &espi_rts5912_config,
		      PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY, &espi_rts5912_driver_api);
