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

#include "espi_utils.h"
#include "reg/reg_acpi.h"
#include "reg/reg_emi.h"
#include "reg/reg_espi.h"
#include "reg/reg_gpio.h"
#include "reg/reg_kbc.h"
#include "reg/reg_mbx.h"
#include "reg/reg_port80.h"

LOG_MODULE_REGISTER(espi, CONFIG_ESPI_LOG_LEVEL);

#define RTS5912_ESPI_RST_IRQ DT_INST_IRQ_BY_IDX(0, 0, irq)

struct espi_rts5912_config {
	uint32_t espislv_base;
	uint32_t espislv_clk_grp;
	uint32_t espislv_clk_idx;
	uint32_t port80_base;
	uint32_t port80_clk_grp;
	uint32_t port80_clk_idx;
	uint32_t acpi_base;
	uint32_t acpi_clk_grp;
	uint32_t acpi_clk_idx;
	uint32_t promt0_base;
	uint32_t promt0_clk_grp;
	uint32_t promt0_clk_idx;
	uint32_t promt1_base;
	uint32_t promt1_clk_grp;
	uint32_t promt1_clk_idx;
	uint32_t promt2_base;
	uint32_t promt2_clk_grp;
	uint32_t promt2_clk_idx;
	uint32_t promt3_base;
	uint32_t promt3_clk_grp;
	uint32_t promt3_clk_idx;
	uint32_t emi0_base;
	uint32_t emi0_clk_grp;
	uint32_t emi0_clk_idx;
	uint32_t emi1_base;
	uint32_t emi1_clk_grp;
	uint32_t emi1_clk_idx;
	uint32_t emi2_base;
	uint32_t emi2_clk_grp;
	uint32_t emi2_clk_idx;
	uint32_t emi3_base;
	uint32_t emi3_clk_grp;
	uint32_t emi3_clk_idx;
	uint32_t emi4_base;
	uint32_t emi4_clk_grp;
	uint32_t emi4_clk_idx;
	uint32_t emi5_base;
	uint32_t emi5_clk_grp;
	uint32_t emi5_clk_idx;
	uint32_t emi6_base;
	uint32_t emi6_clk_grp;
	uint32_t emi6_clk_idx;
	uint32_t emi7_base;
	uint32_t emi7_clk_grp;
	uint32_t emi7_clk_idx;
	uint32_t kbc_base;
	uint32_t kbc_clk_grp;
	uint32_t kbc_clk_idx;
	uint32_t mbx_base;
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

struct espi_vw_channel {
	uint8_t vw_index;
	uint8_t level_mask;
	uint8_t valid_mask;
};

struct espi_vw_signal_t {
	enum espi_vwire_signal signal;
	void (*vw_signal_callback)(const struct device *dev);
};

#define ESPI_RTK_PERIPHERAL_HOST_CMD_PARAM_SIZE 256
#define ESPI_RTK_PERIPHERAL_ACPI_SHD_MEM_SIZE   256
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
static uint8_t ec_host_cmd_sram[ESPI_RTK_PERIPHERAL_HOST_CMD_PARAM_SIZE] __aligned(256);
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
static uint8_t acpi_shd_mem_sram[ESPI_RTK_PERIPHERAL_ACPI_SHD_MEM_SIZE] __aligned(256);
#endif

/* eSPI api functions */
#define VW_CH(signal, index, level, valid)                                                         \
	[signal] = {.vw_index = index, .level_mask = level, .valid_mask = valid}

/* VW signals used in eSPI */
static const struct espi_vw_channel vw_channel_list[] = {
	VW_CH(ESPI_VWIRE_SIGNAL_SLP_S3, 0x02, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_SLP_S4, 0x02, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_SLP_S5, 0x02, BIT(2), BIT(6)),
	VW_CH(ESPI_VWIRE_SIGNAL_OOB_RST_WARN, 0x03, BIT(2), BIT(6)),
	VW_CH(ESPI_VWIRE_SIGNAL_PLTRST, 0x03, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_SUS_STAT, 0x03, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_NMIOUT, 0x07, BIT(2), BIT(6)),
	VW_CH(ESPI_VWIRE_SIGNAL_SMIOUT, 0x07, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_HOST_RST_WARN, 0x07, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_SLP_A, 0x41, BIT(3), BIT(7)),
	VW_CH(ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK, 0x41, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_SUS_WARN, 0x41, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_SLP_WLAN, 0x42, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_SLP_LAN, 0x42, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_HOST_C10, 0x47, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_DNX_WARN, 0x4a, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_PME, 0x04, BIT(3), BIT(7)),
	VW_CH(ESPI_VWIRE_SIGNAL_WAKE, 0x04, BIT(2), BIT(6)),
	VW_CH(ESPI_VWIRE_SIGNAL_OOB_RST_ACK, 0x04, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS, 0x05, BIT(3), BIT(7)),
	VW_CH(ESPI_VWIRE_SIGNAL_ERR_NON_FATAL, 0x05, BIT(2), BIT(6)),
	VW_CH(ESPI_VWIRE_SIGNAL_ERR_FATAL, 0x05, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, 0x05, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_HOST_RST_ACK, 0x06, BIT(3), BIT(7)),
	VW_CH(ESPI_VWIRE_SIGNAL_RST_CPU_INIT, 0x06, BIT(2), BIT(6)),
	VW_CH(ESPI_VWIRE_SIGNAL_SMI, 0x06, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_SCI, 0x06, BIT(0), BIT(4)),
	VW_CH(ESPI_VWIRE_SIGNAL_DNX_ACK, 0x40, BIT(1), BIT(5)),
	VW_CH(ESPI_VWIRE_SIGNAL_SUS_ACK, 0x40, BIT(0), BIT(4)),
};

static struct espi_vw_ch_cached {
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
} espi_vw_ch_cached_data;

static struct espi_vw_tx_cached {
	uint8_t idx4;
	uint8_t idx5;
	uint8_t idx6;
	uint8_t idx40;
} espi_vw_tx_cached_data;

static int espi_rts5912_send_vwire(const struct device *dev, enum espi_vwire_signal signal,
				   uint8_t level);

static int espi_rts5912_receive_vwire(const struct device *dev, enum espi_vwire_signal signal,
				      uint8_t *level);

static int vw_signal_set_valid(const struct device *dev, enum espi_vwire_signal signal,
			       uint8_t valid);

#ifdef CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE
static void send_slave_bootdone(const struct device *dev);
#endif

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

void vw_ch_isr_manual(struct k_work *work)
{
	espi_vw_ch_isr(DEVICE_DT_GET(DT_NODELABEL(espi0)));
}
static K_WORK_DELAYABLE_DEFINE(vw_ch_isr_manual_trigger, vw_ch_isr_manual);

static void espi_rst_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *data = dev->data;

	struct espi_event evt = {.evt_type = ESPI_BUS_RESET, .evt_details = 0, .evt_data = 0};

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;
	uint32_t status = espi_reg->ERSTCFG;

	espi_reg->ERSTCFG |= ESPI_ERSTCFG_RSTSTS_Msk;
	espi_reg->ERSTCFG ^= ESPI_ERSTCFG_RSTPOL_Msk;

	if (status & ESPI_ERSTCFG_RSTSTS_Msk) {
		if (status & ESPI_ERSTCFG_RSTPOL_Msk) {
			evt.evt_data = 0;
		} else {
			evt.evt_data = 1;
			espi_vw_ch_setup(dev);
			espi_reg->ESPICFG = data->config_data;

			if (espi_reg->EVCFG & ESPI_EPCFG_CHEN_Msk) {
				k_work_schedule(&vw_ch_isr_manual_trigger, K_MSEC(150));
			}
		}
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}

static void espi_periph_ch_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *data = dev->data;

	struct espi_event evt = {.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				 .evt_details = ESPI_CHANNEL_PERIPHERAL,
				 .evt_data = 0};

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	uint32_t status = espi_reg->EPSTS;
	uint32_t config = espi_reg->EPCFG;

	if (status & ESPI_EPSTS_CLRSTS_Msk) {
		evt.evt_data = (config & ESPI_EPCFG_CHEN_Msk) ? 1 : 0;
		espi_send_callbacks(&data->callbacks, dev, evt);
		espi_reg->EPSTS = ESPI_EPSTS_CLRSTS_Msk;
	}
}

static void espi_vw_ch_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *data = dev->data;
	struct espi_event evt = {.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				 .evt_details = ESPI_CHANNEL_VWIRE,
				 .evt_data = 0};
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;
	uint32_t config = espi_reg->EVCFG;

	evt.evt_data = (config & ESPI_EVCFG_CHEN_Msk) ? 1 : 0;
	espi_send_callbacks(&data->callbacks, dev, evt);

	if (config & ESPI_EVCFG_CHEN_Msk) {
		vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS, 1);
		vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, 1);
#ifdef CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE
		send_slave_bootdone(dev);
#endif
	}
	espi_reg->EVSTS = ESPI_EVSTS_RXIDXCHG_Msk;
}

static const struct espi_vw_signal_t vw_idx2_signals[] = {
	{ESPI_VWIRE_SIGNAL_SLP_S3, vw_slp3_handler},
	{ESPI_VWIRE_SIGNAL_SLP_S4, vw_slp4_handler},
	{ESPI_VWIRE_SIGNAL_SLP_S5, vw_slp5_handler},
};

static void espi_vw_idx2_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;
	uint8_t cur_idx_data = espi_reg->EVIDX2;
	uint8_t updated_bit = cur_idx_data ^ espi_vw_ch_cached_data.idx2;

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX2CHG_Msk) {
		espi_vw_ch_cached_data.idx2 = cur_idx_data;

		for (int i = 0; i < ARRAY_SIZE(vw_idx2_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx2_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask) {
				if (vw_idx2_signals[i].vw_signal_callback != NULL) {
					vw_idx2_signals[i].vw_signal_callback(dev);
				}
			}
		}
		if (espi_vw_ch_cached_data.idx2 == espi_reg->EVIDX2) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX2CHG_Msk;
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
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;
	uint8_t cur_idx_data = espi_reg->EVIDX3;
	uint8_t updated_bit = cur_idx_data ^ espi_vw_ch_cached_data.idx3;

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX3CHG_Msk) {
		espi_vw_ch_cached_data.idx3 = cur_idx_data;

		for (int i = 0; i < ARRAY_SIZE(vw_idx3_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx3_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask) {
				if (vw_idx3_signals[i].vw_signal_callback != NULL) {
					vw_idx3_signals[i].vw_signal_callback(dev);
				}
			}
		}
		if (espi_vw_ch_cached_data.idx3 == espi_reg->EVIDX3) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX3CHG_Msk;
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
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;
	uint8_t cur_idx_data = espi_reg->EVIDX7;
	uint8_t updated_bit = (cur_idx_data ^ espi_vw_ch_cached_data.idx7);

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX7CHG_Msk) {
		espi_vw_ch_cached_data.idx7 = cur_idx_data;

		for (int i = 0; i < ARRAY_SIZE(vw_idx7_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx7_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask) {
				if (vw_idx7_signals[i].vw_signal_callback != NULL) {
					vw_idx7_signals[i].vw_signal_callback(dev);
				}
			}
		}
		if (espi_vw_ch_cached_data.idx7 == espi_reg->EVIDX7) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX7CHG_Msk;
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
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;
	uint8_t cur_idx_data = espi_reg->EVIDX41;
	uint8_t updated_bit = (cur_idx_data ^ espi_vw_ch_cached_data.idx41);

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX41CHG_Msk) {
		espi_vw_ch_cached_data.idx41 = cur_idx_data;

		for (int i = 0; i < ARRAY_SIZE(vw_idx41_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx41_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask) {
				if (vw_idx41_signals[i].vw_signal_callback != NULL) {
					vw_idx41_signals[i].vw_signal_callback(dev);
				}
			}
		}
		if (espi_vw_ch_cached_data.idx41 == espi_reg->EVIDX41) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX41CHG_Msk;
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
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;
	uint8_t cur_idx_data = espi_reg->EVIDX42;
	uint8_t updated_bit = (cur_idx_data ^ espi_vw_ch_cached_data.idx42);

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX42CHG_Msk) {
		espi_vw_ch_cached_data.idx42 = cur_idx_data;

		for (int i = 0; i < ARRAY_SIZE(vw_idx42_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx42_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask) {
				if (vw_idx42_signals[i].vw_signal_callback != NULL) {
					vw_idx42_signals[i].vw_signal_callback(dev);
				}
			}
		}
		if (espi_vw_ch_cached_data.idx42 == espi_reg->EVIDX42) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX42CHG_Msk;
		}
	}
}

static const struct espi_vw_signal_t vw_idx43_signals[] = {};

static void espi_vw_idx43_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;
	uint8_t cur_idx_data = espi_reg->EVIDX43;
	uint8_t updated_bit = (cur_idx_data ^ espi_vw_ch_cached_data.idx43);

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX43CHG_Msk) {
		espi_vw_ch_cached_data.idx43 = cur_idx_data;

		for (int i = 0; i < ARRAY_SIZE(vw_idx43_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx43_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask) {
				if (vw_idx43_signals[i].vw_signal_callback != NULL) {
					vw_idx43_signals[i].vw_signal_callback(dev);
				}
			}
		}
		if (espi_vw_ch_cached_data.idx43 == espi_reg->EVIDX43) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX43CHG_Msk;
		}
	}
}

static const struct espi_vw_signal_t vw_idx44_signals[] = {};

static void espi_vw_idx44_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;
	uint8_t cur_idx_data = espi_reg->EVIDX44;
	uint8_t updated_bit = (cur_idx_data ^ espi_vw_ch_cached_data.idx44);

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX44CHG_Msk) {
		espi_vw_ch_cached_data.idx44 = cur_idx_data;

		for (int i = 0; i < ARRAY_SIZE(vw_idx44_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx44_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask) {
				if (vw_idx44_signals[i].vw_signal_callback != NULL) {
					vw_idx44_signals[i].vw_signal_callback(dev);
				}
			}
		}
		if (espi_vw_ch_cached_data.idx44 == espi_reg->EVIDX44) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX44CHG_Msk;
		}
	}
}

static const struct espi_vw_signal_t vw_idx47_signals[] = {
	{ESPI_VWIRE_SIGNAL_HOST_C10, vw_host_c10_handler},
};

static void espi_vw_idx47_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;
	uint8_t cur_idx_data = espi_reg->EVIDX47;
	uint8_t updated_bit = (cur_idx_data ^ espi_vw_ch_cached_data.idx47);

	if (espi_reg->EVSTS & ESPI_EVSTS_IDX47CHG_Msk) {
		espi_vw_ch_cached_data.idx47 = cur_idx_data;
		for (int i = 0; i < ARRAY_SIZE(vw_idx47_signals); i++) {
			enum espi_vwire_signal vw_signal = vw_idx47_signals[i].signal;

			if (updated_bit & vw_channel_list[vw_signal].level_mask) {
				if (vw_idx47_signals[i].vw_signal_callback != NULL) {
					vw_idx47_signals[i].vw_signal_callback(dev);
				}
			}
		}
		if (espi_vw_ch_cached_data.idx47 == espi_reg->EVIDX47) {
			espi_reg->EVSTS = ESPI_EVSTS_IDX47CHG_Msk;
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

#ifdef CONFIG_ESPI_OOB_CHANNEL
static void espi_oob_tx_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;
	uint32_t status = espi_reg->EOSTS;

	if (status & ESPI_EOSTS_TXDONE_Msk) {
		k_sem_give(&espi_data->oob_tx_lock);
		espi_reg->EOSTS = ESPI_EOSTS_TXDONE_Msk;
	}
}

static void espi_oob_rx_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	uint32_t status = espi_reg->EOSTS;

#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	struct espi_event evt = {
		.evt_type = ESPI_BUS_EVENT_OOB_RECEIVED, .evt_details = 0, .evt_data = 0};
#endif

	if (status & ESPI_EOSTS_RXDONE_Msk) {
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
		k_sem_give(&espi_data->oob_rx_lock);
#else
		k_busy_wait(250);
		evt.evt_details = espi_reg->EORXLEN;
		espi_send_callbacks(&espi_data->callbacks, dev, evt);
#endif
		espi_reg->EOSTS = ESPI_EOSTS_RXDONE_Msk;
	}
}

static void espi_oob_chg_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	struct espi_event evt = {.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				 .evt_details = ESPI_CHANNEL_OOB,
				 .evt_data = 0};

	uint32_t status = espi_reg->EOSTS;
	uint32_t config = espi_reg->EOCFG;

	if (status & ESPI_EOSTS_CFGENCHG_Msk) {
		evt.evt_data = (config & ESPI_EVCFG_CHEN_Msk) ? 1 : 0;
		espi_send_callbacks(&espi_data->callbacks, dev, evt);

		if (config & ESPI_EVCFG_CHEN_Msk) {
			vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_OOB_RST_ACK, 1);
		}

		espi_reg->EOSTS = ESPI_EOSTS_CFGENCHG_Msk;
	}
}
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
static void espi_maf_tr_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	uint32_t status = espi_reg->EFSTS;

	if (status & ESPI_EFSTS_MAFTXDN_Msk) {
		k_sem_give(&espi_data->flash_lock);
		espi_reg->EFSTS = ESPI_EFSTS_MAFTXDN_Msk;
	}
}

static void espi_flash_chg_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	struct espi_event evt = {.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				 .evt_details = ESPI_CHANNEL_FLASH,
				 .evt_data = 0};

	uint32_t status = espi_reg->EFSTS;
	uint32_t config = espi_reg->EFCONF;

	if (status & ESPI_EFSTS_CHENCHG_Msk) {
		evt.evt_data = (config & ESPI_EFCONF_CHEN_Msk) ? 1 : 0;
		espi_send_callbacks(&espi_data->callbacks, dev, evt);

		espi_reg->EFSTS = ESPI_EFSTS_CHENCHG_Msk;
	}
}
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
static void espi_port80_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;

	struct espi_event evt = {ESPI_BUS_PERIPHERAL_NOTIFICATION,
				 (ESPI_PERIPHERAL_INDEX_0 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,
				 ESPI_PERIPHERAL_NODATA};

	PORT80_Type *port80_reg = (PORT80_Type *)espi_config->port80_base;

	evt.evt_data = port80_reg->DATA;
	espi_send_callbacks(&espi_data->callbacks, dev, evt);
}
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
static void acpi_ibf_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	struct espi_event evt = {ESPI_BUS_PERIPHERAL_NOTIFICATION, ESPI_PERIPHERAL_HOST_IO,
				 ESPI_PERIPHERAL_NODATA};

	/* Updates to fit Chrome shim layer design */
	struct espi_evt_data_acpi *acpi_evt = (struct espi_evt_data_acpi *)&evt.evt_data;
	ACPI_Type *acpi_reg = (ACPI_Type *)espi_config->acpi_base;
	/* Host put data on input buffer of ACPI EC0 channel */
	if (acpi_reg->STS & ACPI_STS_IBF_Msk) {
		/*
		 * Indicates if the host sent a command or data.
		 * 0 = data
		 * 1 = Command.
		 */
		acpi_evt->type = acpi_reg->STS & ACPI_STS_CMDSEL_Msk ? 1 : 0;
		acpi_evt->data = (uint8_t)acpi_reg->IB;
	}
	espi_send_callbacks(&espi_data->callbacks, dev, evt);
}

static void acpi_obe_isr(const struct device *dev)
{
}
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
static void kbc_ibf_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_8042_KBC,
		ESPI_PERIPHERAL_NODATA,
	};

	KBC_Type *kbc_reg = (KBC_Type *)espi_config->kbc_base;
	struct espi_evt_data_kbc *kbc_evt = (struct espi_evt_data_kbc *)&evt.evt_data;
	/*
	 * Indicates if the host sent a command or data.
	 * 0 = data
	 * 1 = Command.
	 */
	kbc_evt->type = kbc_reg->STS & KBC_STS_CMDSEL_Msk ? 1 : 0;

	/* The data in KBC Input Buffer */
	kbc_evt->data = kbc_reg->IB;

	/* KBC Input Buffer Full event */
	kbc_evt->evt = HOST_KBC_EVT_IBF;
	espi_send_callbacks(&espi_data->callbacks, dev, evt);
}

static void kbc_obe_isr(const struct device *dev)
{
}
#endif

static int vw_signal_set_valid(const struct device *dev, enum espi_vwire_signal signal,
			       uint8_t valid)
{
	uint8_t vw_index = vw_channel_list[signal].vw_index;
	uint8_t valid_mask = vw_channel_list[signal].valid_mask;

	if (signal > ARRAY_SIZE(vw_channel_list)) {
		return -EIO;
	}

	switch (vw_index) {
	case 0x04:
		if (valid) {
			espi_vw_tx_cached_data.idx4 |= valid_mask;
		} else {
			espi_vw_tx_cached_data.idx4 &= ~valid_mask;
		}
		break;
	case 0x05:
		if (valid) {
			espi_vw_tx_cached_data.idx5 |= valid_mask;
		} else {
			espi_vw_tx_cached_data.idx5 &= ~valid_mask;
		}
		break;
	case 0x06:
		if (valid) {
			espi_vw_tx_cached_data.idx6 |= valid_mask;
		} else {
			espi_vw_tx_cached_data.idx6 &= ~valid_mask;
		}
		break;
	case 0x40:
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

#ifdef CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE
static void send_slave_bootdone(const struct device *dev)
{
	int ret;
	uint8_t boot_done;

	ret = espi_rts5912_receive_vwire(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, &boot_done);
	if (!ret && !boot_done) {
		/* SLAVE_BOOT_DONE & SLAVE_LOAD_STS have to be sent together */
		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS, 1);
		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, 1);
		k_work_cancel_delayable(&vw_ch_isr_manual_trigger);
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
		vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, 1);
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
		/* reference from IT8*/
		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, 0);
		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_RST_CPU_INIT, 1);
		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_SMI, 1);
		espi_rts5912_send_vwire(dev, ESPI_VWIRE_SIGNAL_SCI, 1);
		/* reference from IT8*/

		vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, 0);
		vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_RST_CPU_INIT, 0);
		vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_SMI, 0);
		vw_signal_set_valid(dev, ESPI_VWIRE_SIGNAL_SCI, 1);
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

static int espi_rts5912_configure(const struct device *dev, struct espi_cfg *cfg)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *data = dev->data;
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	uint32_t gen_conf = 0;
	uint8_t io_mode = 0;

	/* Maximum Frequency Supported */
	switch (cfg->max_freq) {
	case 20:
		gen_conf |= 0ul << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case 25:
		gen_conf |= 1ul << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case 33:
		gen_conf |= 2ul << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case 50:
		gen_conf |= 3ul << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case 66:
		gen_conf |= 4ul << ESPI_ESPICFG_MXFREQSUP_Pos;
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
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	bool sts = false;

	switch (ch) {
	case ESPI_CHANNEL_PERIPHERAL:
		sts = espi_reg->EPCFG & ESPI_EPCFG_CHEN_Msk ? true : false;
		break;
	case ESPI_CHANNEL_VWIRE:
		sts = espi_reg->EVCFG & ESPI_EVCFG_CHEN_Msk ? true : false;
		break;
	case ESPI_CHANNEL_OOB:
		sts = (espi_reg->EOCFG & ESPI_EOCFG_CHEN_Msk) ? true : false;
		break;
	case ESPI_CHANNEL_FLASH:
		sts = espi_reg->EFCONF & ESPI_EFCONF_CHEN_Msk ? true : false;
		break;
	}

	return sts;
}

static int espi_rts5912_send_vwire(const struct device *dev, enum espi_vwire_signal signal,
				   uint8_t level)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	uint8_t vw_idx = vw_channel_list[signal].vw_index;
	uint8_t lev_msk = vw_channel_list[signal].level_mask;
	uint8_t rd_cnt = 100;
	uint32_t tx_data = 0;

	if (signal > ARRAY_SIZE(vw_channel_list)) {
		return -EIO;
	}

	switch (vw_idx) {
	case 0x04:
		tx_data = espi_vw_tx_cached_data.idx4;
		break;
	case 0x05:
		tx_data = espi_vw_tx_cached_data.idx5;
		break;
	case 0x06:
		tx_data = espi_vw_tx_cached_data.idx6;
		break;
	case 0x40:
		tx_data = espi_vw_tx_cached_data.idx40;
		break;
	default:
		return -EIO;
	}

	tx_data |= vw_idx << 8;

	if (level) {
		tx_data |= lev_msk;
	} else {
		tx_data &= ~lev_msk;
	}

	if (espi_reg->EVSTS & ESPI_EVSTS_TXFULL_Msk) {
		return -EIO;
	}

	espi_reg->EVTXDAT = tx_data;
	while ((espi_reg->EVSTS & ESPI_EVSTS_TXFULL_Msk) && rd_cnt--) {
		k_busy_wait(100);
	}

	switch (vw_idx) {
	case 0x04:
		espi_vw_tx_cached_data.idx4 = tx_data;
		break;
	case 0x05:
		espi_vw_tx_cached_data.idx5 = tx_data;
		break;
	case 0x06:
		espi_vw_tx_cached_data.idx6 = tx_data;
		break;
	case 0x40:
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
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	uint8_t vw_idx, lev_msk, valid_msk;
	uint8_t vw_data;

	vw_idx = vw_channel_list[signal].vw_index;
	lev_msk = vw_channel_list[signal].level_mask;
	valid_msk = vw_channel_list[signal].valid_mask;

	if (signal > ARRAY_SIZE(vw_channel_list)) {
		return -EIO;
	}

	switch (vw_idx) {
	case 0x02:
		if (espi_vw_ch_cached_data.idx2 != espi_reg->EVIDX2) {
			espi_vw_ch_cached_data.idx2 = espi_reg->EVIDX2;
		}
		vw_data = espi_vw_ch_cached_data.idx2;
		break;
	case 0x03:
		if (espi_vw_ch_cached_data.idx3 != espi_reg->EVIDX3) {
			espi_vw_ch_cached_data.idx3 = espi_reg->EVIDX3;
		}
		vw_data = espi_vw_ch_cached_data.idx3;
		break;
	case 0x04:
		vw_data = espi_vw_tx_cached_data.idx4;
		break;
	case 0x05:
		vw_data = espi_vw_tx_cached_data.idx5;
		break;
	case 0x06:
		vw_data = espi_vw_tx_cached_data.idx6;
		break;
	case 0x07:
		if (espi_vw_ch_cached_data.idx7 != espi_reg->EVIDX7) {
			espi_vw_ch_cached_data.idx7 = espi_reg->EVIDX7;
		}
		vw_data = espi_vw_ch_cached_data.idx7;
		break;
	case 0x40:
		vw_data = espi_vw_tx_cached_data.idx40;
		break;
	case 0x41:
		if (espi_vw_ch_cached_data.idx41 != espi_reg->EVIDX41) {
			espi_vw_ch_cached_data.idx41 = espi_reg->EVIDX41;
		}
		vw_data = espi_vw_ch_cached_data.idx41;
		break;
	case 0x42:
		if (espi_vw_ch_cached_data.idx42 != espi_reg->EVIDX42) {
			espi_vw_ch_cached_data.idx42 = espi_reg->EVIDX42;
		}
		vw_data = espi_vw_ch_cached_data.idx42;
		break;
	case 0x43:
		if (espi_vw_ch_cached_data.idx43 != espi_reg->EVIDX43) {
			espi_vw_ch_cached_data.idx43 = espi_reg->EVIDX43;
		}
		vw_data = espi_vw_ch_cached_data.idx43;
		break;
	case 0x44:
		if (espi_vw_ch_cached_data.idx44 != espi_reg->EVIDX44) {
			espi_vw_ch_cached_data.idx44 = espi_reg->EVIDX44;
		}
		vw_data = espi_vw_ch_cached_data.idx44;
		break;
	case 0x47:
		if (espi_vw_ch_cached_data.idx47 != espi_reg->EVIDX47) {
			espi_vw_ch_cached_data.idx47 = espi_reg->EVIDX47;
		}
		vw_data = espi_vw_ch_cached_data.idx47;
		break;
	case 0x4A:
		if (espi_vw_ch_cached_data.idx4a != espi_reg->EVIDX4A) {
			espi_vw_ch_cached_data.idx4a = espi_reg->EVIDX4A;
		}
		vw_data = espi_vw_ch_cached_data.idx4a;
		break;
	case 0x51:
		if (espi_vw_ch_cached_data.idx51 != espi_reg->EVIDX51) {
			espi_vw_ch_cached_data.idx51 = espi_reg->EVIDX51;
		}
		vw_data = espi_vw_ch_cached_data.idx51;
		break;
	case 0x61:
		if (espi_vw_ch_cached_data.idx61 != espi_reg->EVIDX61) {
			espi_vw_ch_cached_data.idx61 = espi_reg->EVIDX61;
		}
		vw_data = espi_vw_ch_cached_data.idx61;
		break;
	default:
		return -EIO;
	}

	if (vw_data & valid_msk) {
		*level = !!(vw_data & lev_msk);
	} else {
		/* Not valid */
		*level = 0;
	}

	return 0;
}

static int espi_rts5912_manage_callback(const struct device *dev, struct espi_callback *callback,
					bool set)
{
	struct espi_rts5912_data *data = dev->data;

	return espi_manage_callback(&data->callbacks, callback, set);
}

static void promt0_ibf_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	ACPI_Type *promt0_reg = (ACPI_Type *)espi_config->promt0_base;
	struct espi_rts5912_data *data = dev->data;
	struct espi_event evt = {.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
				 .evt_details = ESPI_PERIPHERAL_EC_HOST_CMD,
				 .evt_data = ESPI_PERIPHERAL_NODATA};

	promt0_reg->STS |= ACPI_STS_STS0_Msk;
	evt.evt_data = (uint8_t)promt0_reg->IB;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void promt0_obe_isr(const struct device *dev)
{
}

static int espi_promt0_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct rts5912_sccon_subsys sccon;
	ACPI_Type *promt0_reg = (ACPI_Type *)espi_config->promt0_base;
	int rc;

	if (!device_is_ready(espi_config->clk_dev)) {
		return -ENODEV;
	}

	sccon.clk_grp = espi_config->promt0_clk_grp;
	sccon.clk_idx = espi_config->promt0_clk_idx;
	rc = clock_control_on(espi_config->clk_dev, (clock_control_subsys_t)&sccon);
	if (rc != 0) {
		return rc;
	}

	promt0_reg->STS = 0;

	if (promt0_reg->STS & ACPI_STS_IBF_Msk) {
		rc = promt0_reg->IB;
	}

	if (promt0_reg->STS & ACPI_STS_IBF_Msk) {
		promt0_reg->IB |= BIT(ACPI_IB_IBCLR_Pos);
	}

	promt0_reg->PTADDR =
		CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM | (0x04 << ACPI_PTADDR_OFFSET_Pos);
	promt0_reg->VWCTRL1 = (ACPI_VWCTRL1_ACTEN_Msk);
	promt0_reg->INTEN = (ACPI_INTEN_IBFINTEN_Msk);

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), promt0_ibf, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), promt0_ibf, priority), promt0_ibf_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), promt0_ibf, irq));
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), promt0_obe, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), promt0_obe, priority), promt0_obe_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), promt0_obe, irq));
	return 0;
}

static int espi_rts5912_read_lpc_request(const struct device *dev, enum lpc_peripheral_opcode op,
					 uint32_t *data)
{
	const struct espi_rts5912_config *const espi_config = dev->config;

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		KBC_Type *kbc_reg = (KBC_Type *)espi_config->kbc_base;

		switch (op) {
		case E8042_OBF_HAS_CHAR:
			*data = kbc_reg->STS & KBC_STS_OBF_Msk ? 1 : 0;
			break;
		case E8042_IBF_HAS_CHAR:
			*data = kbc_reg->STS & KBC_STS_IBF_Msk ? 1 : 0;
			break;
		case E8042_READ_KB_STS:
			*data = kbc_reg->STS;
			break;
		default:
			return -EINVAL;
		}
	} else if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {
		ACPI_Type *acpi_reg = (ACPI_Type *)espi_config->acpi_base;

		switch (op) {
		case EACPI_OBF_HAS_CHAR:
			*data = acpi_reg->STS & ACPI_STS_OBF_Msk ? 1 : 0;
			break;
		case EACPI_IBF_HAS_CHAR:
			*data = acpi_reg->STS & ACPI_STS_IBF_Msk ? 1 : 0;
			break;
		case EACPI_READ_STS:
			*data = acpi_reg->STS;
			break;

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) && defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
		case EACPI_GET_SHARED_MEMORY:
			*data = (uint32_t)acpi_shd_mem_sram;
			break;
#endif
		default:
			return -EINVAL;
		}
	}
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	else if (op >= ECUSTOM_START_OPCODE && op <= ECUSTOM_MAX_OPCODE) {
		switch (op) {
		case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY:
			*data = (uint32_t)ec_host_cmd_sram;
			break;
		case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY_SIZE:
			*data = ESPI_RTK_PERIPHERAL_HOST_CMD_PARAM_SIZE;
			break;
		default:
			return -EINVAL;
		}
	}
#endif
	else {
		return -ENOTSUP;
	}

	return 0;
}

static void vw_set_irq1(uint8_t level, const struct espi_rts5912_config *const espi_config)
{
	uint16_t retry_counter = 1024;
	uint32_t timeout_cnt = 0;

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	if ((espi_reg->EVCFG & ESPI_EVCFG_CHEN_Msk) == 0 ||
	    (espi_reg->EVCFG & ESPI_EVCFG_CHRDY_Msk) == 0) {
		return;
	}

	while (espi_reg->EVSTS & ESPI_EVSTS_TXFULL_Msk) {
		if (++timeout_cnt >= 10000) {
			return;
		}
	}

	espi_reg->EVTXDAT = level ? 0x81 : 0x01;
	while (espi_reg->EVSTS & ESPI_EVSTS_TXFULL_Msk && --retry_counter) {
	}

	espi_reg->EVSTS |= BIT(16);
}

static int prev_irq1;

static void kbdata_with_irq1(uint32_t data, const struct espi_rts5912_config *const espi_config)
{
	uint32_t timeout_cnt = 0;

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;
	KBC_Type *kbc_reg = (KBC_Type *)espi_config->kbc_base;

	if ((espi_reg->EVCFG & ESPI_EVCFG_CHEN_Msk) == 0 ||
	    (espi_reg->EVCFG & ESPI_EVCFG_CHRDY_Msk) == 0) {
		return;
	}

	while (espi_reg->EVSTS & ESPI_EVSTS_TXFULL_Msk) {
		timeout_cnt++;
		if (timeout_cnt >= 10000) {
			return;
		}
	}

	__disable_irq();
	kbc_reg->OB = data;
	espi_reg->EVTXDAT = 0x81;

	timeout_cnt = 0;
	while (espi_reg->EVSTS & ESPI_EVSTS_TXFULL_Msk) {
		timeout_cnt++;
		if (timeout_cnt >= 10000) {
			prev_irq1 = 1;
			__enable_irq();
			return;
		}
	}

	prev_irq1 = 1;
	espi_reg->EVSTS |= BIT(16);
	__enable_irq();
}

static uint8_t kbc_write(uint8_t data, uint32_t kbc_int_en,
			 const struct espi_rts5912_config *const espi_config)
{
	KBC_Type *kbc_reg = (KBC_Type *)espi_config->kbc_base;
	uint32_t exData = (uint32_t)data;

	if (prev_irq1 == 1) {
		vw_set_irq1(0, espi_config);
	}

	if (kbc_int_en) {
		prev_irq1 = 1;
		kbdata_with_irq1(exData, espi_config);
	} else {
		kbc_reg->OB = exData;
	}

	return 0;
}

static int espi_rts5912_write_lpc_request(const struct device *dev, enum lpc_peripheral_opcode op,
					  uint32_t *data)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	static int kbc_int_en = 1;

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		KBC_Type *kbc_reg = (KBC_Type *)espi_config->kbc_base;

		switch (op) {
		case E8042_WRITE_KB_CHAR:
			kbc_write(*data & 0xff, kbc_int_en, espi_config);
			break;
		case E8042_WRITE_MB_CHAR:
			kbc_write(*data & 0xff, kbc_int_en, espi_config);
			break;
		case E8042_RESUME_IRQ:
			kbc_int_en = 1;
			break;
		case E8042_PAUSE_IRQ:
			kbc_int_en = 0;
			break;
		case E8042_CLEAR_OBF:
			kbc_reg->OB |= KBC_OB_OBCLR_Msk;
			break;
		case E8042_SET_FLAG:
			/* FW shouldn't modify these flags directly */
			*data &= ~(KBC_STS_OBF_Msk | KBC_STS_IBF_Msk | KBC_STS_STS2_Pos);
			kbc_reg->STS |= *data & 0xff;
			break;
		case E8042_CLEAR_FLAG:
			/* FW shouldn't modify these flags directly */
			*data |= KBC_STS_OBF_Msk | KBC_STS_IBF_Msk | KBC_STS_STS2_Pos;
			kbc_reg->STS &= ~(*data & 0xff);
			break;
		default:
			return -EINVAL;
		}
	} else if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {
		ACPI_Type *acpi_reg = (ACPI_Type *)espi_config->acpi_base;

		switch (op) {
		case EACPI_WRITE_CHAR:
			acpi_reg->OB = *data & 0xff;
			break;
		case EACPI_WRITE_STS:
			acpi_reg->STS = *data & 0xff;
			break;
		default:
			return -EINVAL;
		}
	}
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	else if (op >= ECUSTOM_START_OPCODE && op <= ECUSTOM_MAX_OPCODE) {
		ACPI_Type *promt0_reg = (ACPI_Type *)espi_config->promt0_base;
		ACPI_Type *acpi_reg = (ACPI_Type *)espi_config->acpi_base;
		KBC_Type *kbc_reg = (KBC_Type *)espi_config->kbc_base;
		PORT80_Type *port80_reg = (PORT80_Type *)espi_config->port80_base;

		switch (op) {
		case ECUSTOM_HOST_SUBS_INTERRUPT_EN:
			if (*data == 0) {
				promt0_reg->INTEN &= ~ACPI_INTEN_IBFINTEN_Msk;
				acpi_reg->INTEN &=
					~(ACPI_INTEN_IBFINTEN_Msk | ACPI_INTEN_OBFINTEN_Msk);
				port80_reg->INTEN &= ~PORT80_INTEN_THREINTEN_Msk;
				kbc_reg->INTEN &= ~KBC_INTEN_IBFINTEN_Msk;
			} else {
				promt0_reg->INTEN |= ACPI_INTEN_IBFINTEN_Msk;
				acpi_reg->INTEN |=
					(ACPI_INTEN_IBFINTEN_Msk | ACPI_INTEN_OBFINTEN_Msk);
				port80_reg->INTEN |= PORT80_INTEN_THREINTEN_Msk;
				kbc_reg->INTEN |= KBC_INTEN_IBFINTEN_Msk;
			}
			break;
		case ECUSTOM_HOST_CMD_SEND_RESULT:
			promt0_reg->STS &= ~ACPI_STS_STS0_Msk;
			promt0_reg->OB = (*data & 0xff);
			break;
		default:
			return -EINVAL;
		}
	}
#endif
	else {
		return -ENOTSUP;
	}

	return 0;
}

#if defined(CONFIG_ESPI_OOB_CHANNEL)

#define MAX_OOB_TIMEOUT 200
#define OOB_BUFFER_SIZE 256

static int espi_rts5912_send_oob(const struct device *dev, struct espi_oob_packet *pckt)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	int ret;

	if (!(espi_reg->EOCFG & ESPI_EOCFG_CHRDY_Msk)) {
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
		*(espi_data->oob_tx_ptr + i) = pckt->buf[i];
	}

	espi_reg->EOTXLEN = pckt->len - 1;
	espi_reg->EOTXCTRL = ESPI_EOTXCTRL_TXSTR_Msk;

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

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	int ret;
#endif
	uint32_t rx_len;

	if (!(espi_reg->EOCFG & ESPI_EOCFG_CHRDY_Msk)) {
		LOG_ERR("%s: OOB channel isn't ready", __func__);
		return -EIO;
	}

	if (espi_reg->EOSTS & ESPI_EOSTS_RXPND_Msk) {
		LOG_ERR("OOB Receive Pending");
		return -EIO;
	}

#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	/* Wait until ISR or timeout */
	ret = k_sem_take(&espi_data->oob_rx_lock, K_MSEC(MAX_OOB_TIMEOUT));
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
		pckt->buf[i] = *(espi_data->oob_rx_ptr + i);
	}

	return 0;
}
#endif

#if defined(CONFIG_ESPI_FLASH_CHANNEL)

#define MAX_FLASH_TIMEOUT 1000ul

enum {
	MAF_TR_READ = 0ul,
	MAF_TR_WRITE = 1ul,
	MAF_TR_ERASE = 2ul,
};

static int espi_rts5912_flash_read(const struct device *dev, struct espi_flash_packet *pckt)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	int ret;
	uint32_t ctrl;

	if (!(espi_reg->EFCONF & ESPI_EFCONF_CHEN_Msk)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if (pckt->len > CONFIG_MAF_BUFFER_SIZE) {
		LOG_ERR("Invalid size request");
		return -EINVAL;
	}

	if (espi_reg->EMCTRL & ESPI_EMCTRL_START_Msk) {
		LOG_ERR("Channel still busy");
		return -EBUSY;
	}

	ctrl = ((MAF_TR_READ << ESPI_EMCTRL_MDSEL_Pos) | ESPI_EMCTRL_START_Msk);

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
		pckt->buf[i] = *(espi_data->maf_ptr + i);
	}

	return 0;
}

static int espi_rts5912_flash_write(const struct device *dev, struct espi_flash_packet *pckt)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	int ret;
	uint32_t ctrl;

	if (!(espi_reg->EFCONF & ESPI_EFCONF_CHEN_Msk)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if (pckt->len > CONFIG_MAF_BUFFER_SIZE) {
		LOG_ERR("Packet length is too big");
		return -EINVAL;
	}

	if (espi_reg->EMCTRL & ESPI_EMCTRL_START_Msk) {
		LOG_ERR("Channel still busy");
		return -EBUSY;
	}

	for (int i = 0; i < pckt->len; i++) {
		*(espi_data->maf_ptr + i) = pckt->buf[i];
	}

	ctrl = ((MAF_TR_WRITE << ESPI_EMCTRL_MDSEL_Pos) | ESPI_EMCTRL_START_Msk);

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

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	int ret;
	uint32_t ctrl;

	if (!(espi_reg->EFCONF & ESPI_EFCONF_CHEN_Msk)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if (espi_reg->EMCTRL & ESPI_EMCTRL_START_Msk) {
		LOG_ERR("Channel still busy");
		return -EBUSY;
	}

	ctrl = (MAF_TR_ERASE << ESPI_EMCTRL_MDSEL_Pos) | ESPI_EMCTRL_START_Msk;

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

#endif

static const struct espi_driver_api espi_rts5912_driver_api = {
	.config = espi_rts5912_configure,
	.get_channel_status = espi_rts5912_channel_ready,
	.send_vwire = espi_rts5912_send_vwire,
	.receive_vwire = espi_rts5912_receive_vwire,
	.manage_callback = espi_rts5912_manage_callback,
	.read_lpc_request = espi_rts5912_read_lpc_request,
	.write_lpc_request = espi_rts5912_write_lpc_request,
#if defined(CONFIG_ESPI_OOB_CHANNEL)
	.send_oob = espi_rts5912_send_oob,
	.receive_oob = espi_rts5912_receive_oob,
#endif
#if defined(CONFIG_ESPI_FLASH_CHANNEL)
	.flash_read = espi_rts5912_flash_read,
	.flash_write = espi_rts5912_flash_write,
	.flash_erase = espi_rts5912_flash_erase,
#endif
};

static void espi_bus_reset_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	espi_reg->ERSTCFG = (ESPI_ERSTCFG_RSTINTEN_Msk);
	espi_reg->ERSTCFG = (ESPI_ERSTCFG_RSTMONEN_Msk);

	if (espi_reg->ERSTCFG & ESPI_ERSTCFG_RSTSTS_Msk) {
		/* high to low */
		espi_reg->ERSTCFG = (ESPI_ERSTCFG_RSTMONEN_Msk | ESPI_ERSTCFG_RSTPOL_Msk |
				     ESPI_ERSTCFG_RSTINTEN_Msk);
	} else {
		/* low to high */
		espi_reg->ERSTCFG = (ESPI_ERSTCFG_RSTMONEN_Msk | ESPI_ERSTCFG_RSTINTEN_Msk);
	}

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), bus_rst, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), bus_rst, priority), espi_rst_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), bus_rst, irq));
}

static void espi_periph_ch_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	espi_reg->EPINTEN =
		(ESPI_EPINTEN_CFGCHGEN_Msk | ESPI_EPINTEN_MEMWREN_Msk | ESPI_EPINTEN_MEMRDEN_Msk);

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), periph_ch, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), periph_ch, priority), espi_periph_ch_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), periph_ch, irq));
}

static void espi_vw_ch_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	espi_reg->EVSTS |= ESPI_EVSTS_RXIDXCLR_Msk;

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

	espi_reg->EVRXINTEN = (ESPI_EVRXINTEN_CFGCHGEN_Msk | ESPI_EVRXINTEN_RXCHGEN_Msk);

	/* virtual-wire */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_ch, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_ch, priority), espi_vw_ch_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_ch, irq));

	/* Index2 */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx2, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx2, priority), espi_vw_idx2_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx2, irq));

	/* Index3 */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx3, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx3, priority), espi_vw_idx3_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx3, irq));

	/* Index7 */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx7, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx7, priority), espi_vw_idx7_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx7, irq));

	/* Index41 */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx41, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx41, priority), espi_vw_idx41_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx41, irq));

	/* Index42 */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx42, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx42, priority), espi_vw_idx42_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx42, irq));

	/* Index43 */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx43, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx43, priority), espi_vw_idx43_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx43, irq));

	/* Index44 */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx44, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx44, priority), espi_vw_idx44_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx44, irq));

	/* Index47 */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx47, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx47, priority), espi_vw_idx47_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx47, irq));

	/* Index4a */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx4a, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx4a, priority), espi_vw_idx4a_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx4a, irq));

	/* Index51 */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx51, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx51, priority), espi_vw_idx51_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx51, irq));

	/* Index61 */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx61, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx61, priority), espi_vw_idx61_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), vw_idx61, irq));
}

#if defined(CONFIG_ESPI_OOB_CHANNEL)
static int espi_oob_ch_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *const espi_data = dev->data;

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	espi_data->oob_tx_busy = false;

	espi_data->oob_tx_ptr = k_aligned_alloc(4, OOB_BUFFER_SIZE);
	if (espi_data->oob_tx_ptr == NULL) {
		return -ENOMEM;
	}

	espi_data->oob_rx_ptr = k_aligned_alloc(4, OOB_BUFFER_SIZE);
	if (espi_data->oob_tx_ptr == NULL) {
		return -ENOMEM;
	}

	espi_reg->EOTXBUF = (uint32_t)espi_data->oob_tx_ptr;
	espi_reg->EORXBUF = (uint32_t)espi_data->oob_rx_ptr;

	espi_reg->EOTXINTEN = ESPI_EOTXINTEN_TXEN_Msk;
	espi_reg->EORXINTEN = (ESPI_EORXINTEN_RXEN_Msk | ESPI_EORXINTEN_CHENCHG_Msk);

	k_sem_init(&espi_data->oob_tx_lock, 0, 1);
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	k_sem_init(&espi_data->oob_rx_lock, 0, 1);
#endif

	/* Tx */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), oob_tx, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), oob_tx, priority), espi_oob_tx_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), oob_tx, irq));

	/* Rx */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), oob_rx, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), oob_rx, priority), espi_oob_rx_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), oob_rx, irq));

	/* Chg */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), oob_chg, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), oob_chg, priority), espi_oob_chg_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), oob_chg, irq));

	return 0;
}
#endif

#if defined(CONFIG_ESPI_FLASH_CHANNEL)
static int espi_flash_ch_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;

	ESPI_Type *espi_reg = (ESPI_Type *)espi_config->espislv_base;

	espi_data->maf_ptr = k_aligned_alloc(4, CONFIG_MAF_BUFFER_SIZE);
	if (espi_data->maf_ptr == NULL) {
		return -ENOMEM;
	}

	espi_reg->EMBUF = (uint32_t)espi_data->maf_ptr;
	espi_reg->EMINTEN = (ESPI_EMINTEN_CHENCHG_Msk | ESPI_EMINTEN_TRDONEEN_Msk);

	k_sem_init(&espi_data->flash_lock, 0, 1);

	/* MAF Tr */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), maf_tr, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), maf_tr, priority), espi_maf_tr_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), maf_tr, irq));

	/* Chg */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), flash_chg, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), flash_chg, priority), espi_flash_chg_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), flash_chg, irq));

	return 0;
}
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
static int espi_peri_ch_port80_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct rts5912_sccon_subsys sccon;

	PORT80_Type *port80_reg = (PORT80_Type *)espi_config->port80_base;

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

	port80_reg->ADDR = 0x80ul;
	port80_reg->CFG = (PORT80_CFG_CLRFLG_Msk | PORT80_CFG_THREEN_Msk);
	port80_reg->INTEN = PORT80_INTEN_THREINTEN_Msk;

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), port80, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), port80, priority), espi_port80_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), port80, irq));

	return 0;
}
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
static int espi_acpi_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct rts5912_sccon_subsys sccon;

	ACPI_Type *acpi_reg = (ACPI_Type *)espi_config->acpi_base;

	int rc;

	if (!device_is_ready(espi_config->clk_dev)) {
		return -ENODEV;
	}

	sccon.clk_grp = espi_config->acpi_clk_grp;
	sccon.clk_idx = espi_config->acpi_clk_idx;
	rc = clock_control_on(espi_config->clk_dev, (clock_control_subsys_t)&sccon);
	if (rc != 0) {
		return rc;
	}

	acpi_reg->VWCTRL0 = ACPI_VWCTRL0_IRQEN_Msk;
	acpi_reg->VWCTRL1 = ((0x00ul << ACPI_VWCTRL1_IRQNUM_Pos) | ACPI_VWCTRL1_ACTEN_Msk);
	acpi_reg->INTEN = (ACPI_INTEN_IBFINTEN_Msk | ACPI_INTEN_OBFINTEN_Msk);

	/* IBF */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), acpi_ibf, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), acpi_ibf, priority), acpi_ibf_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), acpi_ibf, irq));

	/* OBE */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), acpi_obe, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), acpi_obe, priority), acpi_obe_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), acpi_obe, irq));

	return 0;
}
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
static int espi_kbc_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct rts5912_sccon_subsys sccon;

	KBC_Type *kbc_reg = (KBC_Type *)espi_config->kbc_base;

	if (!device_is_ready(espi_config->clk_dev)) {
		return -ENODEV;
	}

	sccon.clk_grp = espi_config->kbc_clk_grp;
	sccon.clk_idx = espi_config->kbc_clk_idx;
	int rc = clock_control_on(espi_config->clk_dev, (clock_control_subsys_t)&sccon);

	if (rc != 0) {
		return rc;
	}

	kbc_reg->VWCTRL0 = KBC_VWCTRL0_IRQEN_Msk;
	kbc_reg->VWCTRL1 = (0x01ul << KBC_VWCTRL1_IRQNUM_Pos) | KBC_VWCTRL1_ACTEN_Msk;
	kbc_reg->INTEN = KBC_INTEN_IBFINTEN_Msk;

	/* IBF */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), kbc_ibf, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), kbc_ibf, priority), kbc_ibf_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), kbc_ibf, irq));

	/* OBE */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), kbc_obe, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), kbc_obe, priority), kbc_obe_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), kbc_obe, irq));

	return 0;
}
#endif

static void espi_emi0_isr(const struct device *dev)
{
}

static void espi_emi1_isr(const struct device *dev)
{
}

volatile uint32_t rts_offset;

static void espi_mbx_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	MBX_Type *mbx_reg = (MBX_Type *)espi_config->mbx_base;
	uint32_t sts = (mbx_reg->STS & 0xFF);
	int start;

	if (sts == 0) {
		return;
	}

	if (sts & 0x80) {
		rts_offset = mbx_reg->DATA[0x02];
		start = rts_offset;

		for (int i = 4; i < 32; i++) {
			ec_host_cmd_sram[rts_offset++] = mbx_reg->DATA[i];
		}

		for (int j = 0; j < 32; j++) {
			LOG_INF("mbx_out[%d] = 0x%02x ", j, mbx_reg->DATA[j]);
			LOG_INF("sram[%d] = 0x%02x ", j, ec_host_cmd_sram[start + j]);
		}

		mbx_reg->STS = 0;
		mbx_reg->DATA[0x01] |= 0x01;
	} else {
		rts_offset = mbx_reg->DATA[0x02];
		start = rts_offset;
		for (int i = 4; i < 32; i++) {
			mbx_reg->DATA[i] = ec_host_cmd_sram[rts_offset++];
		}
		for (int j = 0; j < 32; j++) {
			LOG_INF("mbx_in[%d] = 0x%02x ", j, mbx_reg->DATA[j]);
			LOG_INF("sram[%d] = 0x%02x ", j, ec_host_cmd_sram[start + j]);
		}
		mbx_reg->STS = 0;
		mbx_reg->DATA[0x00] |= 0x01;
	}
	mbx_reg->INTSTS |= BIT(MBX_INTSTS_CLR_Pos);
}

static int espi_rts5912_init(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct rts5912_sccon_subsys sccon;

	MBX_Type *mbx_reg = (MBX_Type *)espi_config->mbx_base;
	EMI_Type *emi0_reg = (EMI_Type *)espi_config->emi0_base;
	EMI_Type *emi1_reg = (EMI_Type *)espi_config->emi1_base;

	int rc;

	/* Setup eSPI pins */
	rc = pinctrl_apply_state(espi_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		LOG_ERR("eSPI pinctrl setup failed (%d)", rc);
		return rc;
	}

	emi0_reg->SAR = (uint32_t)&ec_host_cmd_sram[0];
	emi0_reg->INTCTRL = 0x4;

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), emi0, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), emi0, priority), espi_emi0_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), emi0, irq));

	emi1_reg->SAR = (uint32_t)&acpi_shd_mem_sram[0];

	acpi_shd_mem_sram[0x20] = 'E';
	acpi_shd_mem_sram[0x21] = 'C';
	acpi_shd_mem_sram[0x22] = 1;
	acpi_shd_mem_sram[0x26] = 1;
	acpi_shd_mem_sram[0x27] = 0x02;

	emi1_reg->INTCTRL = 0x4;
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), emi1, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), emi1, priority), espi_emi1_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), emi1, irq));

	if (!device_is_ready(espi_config->clk_dev)) {
		return -ENODEV;
	}

	/* Enable eSPI clock */
	sccon.clk_grp = espi_config->espislv_clk_grp;
	sccon.clk_idx = espi_config->espislv_clk_idx;
	rc = clock_control_on(espi_config->clk_dev, (clock_control_subsys_t)&sccon);
	if (rc != 0) {
		goto exit;
	}

	/* Setup eSPI bus reset */
	espi_bus_reset_setup(dev);

	/* Setup eSPI peripheral channel */
	espi_periph_ch_setup(dev);

	/* Setup eSPI virtual-wire channel */
	espi_vw_ch_setup(dev);

	mbx_reg->INTCTRL = 0x4;

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), mbx, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(espi0), mbx, priority), espi_mbx_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(espi0), mbx, irq));

#if defined(CONFIG_ESPI_OOB_CHANNEL)
	rc = espi_oob_ch_setup(dev);
	if (rc != 0) {
		goto exit;
	}
#endif

#if defined(CONFIG_ESPI_FLASH_CHANNEL)
	rc = espi_flash_ch_setup(dev);
	if (rc != 0) {
		goto exit;
	}
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	rc = espi_peri_ch_port80_setup(dev);
	if (rc != 0) {
		goto exit;
	}
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
	/* Setup ACPI */
	rc = espi_acpi_setup(dev);
	rc = espi_promt0_setup(dev);
	if (rc != 0) {
		goto exit;
	}
	if (rc != 0) {
		goto exit;
	}
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	rc = espi_kbc_setup(dev);
	if (rc != 0) {
		goto exit;
	}
#endif

exit:
	return rc;
}

PINCTRL_DT_INST_DEFINE(0);

static struct espi_rts5912_data espi_rts5912_data;

static const struct espi_rts5912_config espi_rts5912_config = {
	.espislv_base = DT_INST_REG_ADDR_BY_IDX(0, 0),
	.espislv_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), espi_slave, clk_grp),
	.espislv_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), espi_slave, clk_idx),

	.port80_base = DT_INST_REG_ADDR_BY_IDX(0, 1),
	.port80_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), port80, clk_grp),
	.port80_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), port80, clk_idx),

	.acpi_base = DT_INST_REG_ADDR_BY_IDX(0, 2),
	.acpi_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), acpi, clk_grp),
	.acpi_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), acpi, clk_idx),

	.promt0_base = DT_INST_REG_ADDR_BY_IDX(0, 3),
	.promt0_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), promt0, clk_grp),
	.promt0_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), promt0, clk_idx),
	.promt1_base = DT_INST_REG_ADDR_BY_IDX(0, 4),
	.promt1_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), promt1, clk_grp),
	.promt1_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), promt1, clk_idx),
	.promt2_base = DT_INST_REG_ADDR_BY_IDX(0, 5),
	.promt2_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), promt2, clk_grp),
	.promt2_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), promt2, clk_idx),
	.promt3_base = DT_INST_REG_ADDR_BY_IDX(0, 6),
	.promt3_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), promt3, clk_grp),
	.promt3_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), promt3, clk_idx),

	.emi0_base = DT_INST_REG_ADDR_BY_IDX(0, 7),
	.emi0_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi0, clk_grp),
	.emi0_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi0, clk_idx),
	.emi1_base = DT_INST_REG_ADDR_BY_IDX(0, 8),
	.emi1_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi1, clk_grp),
	.emi1_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi1, clk_idx),
	.emi2_base = DT_INST_REG_ADDR_BY_IDX(0, 9),
	.emi2_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi2, clk_grp),
	.emi2_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi2, clk_idx),
	.emi3_base = DT_INST_REG_ADDR_BY_IDX(0, 10),
	.emi3_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi3, clk_grp),
	.emi3_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi3, clk_idx),
	.emi4_base = DT_INST_REG_ADDR_BY_IDX(0, 11),
	.emi4_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi4, clk_grp),
	.emi4_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi4, clk_idx),
	.emi5_base = DT_INST_REG_ADDR_BY_IDX(0, 12),
	.emi5_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi5, clk_grp),
	.emi5_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi5, clk_idx),
	.emi6_base = DT_INST_REG_ADDR_BY_IDX(0, 13),
	.emi6_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi6, clk_grp),
	.emi6_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi6, clk_idx),
	.emi7_base = DT_INST_REG_ADDR_BY_IDX(0, 14),
	.emi7_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi7, clk_grp),
	.emi7_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), emi7, clk_idx),

	.kbc_base = DT_INST_REG_ADDR_BY_IDX(0, 15),
	.kbc_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), kbc, clk_grp),
	.kbc_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(espi0), kbc, clk_idx),

	.mbx_base = DT_INST_REG_ADDR_BY_IDX(0, 16),
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

DEVICE_DT_INST_DEFINE(0, &espi_rts5912_init, NULL, &espi_rts5912_data, &espi_rts5912_config,
		      PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY, &espi_rts5912_driver_api);
