/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include "r_layer3_switch.h"
#include "eth_renesas_ra_eswm.h"
#include "eth.h"


LOG_MODULE_DECLARE(eth_renesas_ra, CONFIG_ETHERNET_LOG_LEVEL);

struct eswm_renesas_ra_data {
	layer3_switch_instance_ctrl_t *fsp_ctrl;
	ether_switch_cfg_t *fsp_cfg;
	uint32_t bridge_port_mask;
	struct net_if *offload_br;
};

struct eswm_renesas_ra_config {
	const struct device *gwcaclk_dev;
	const struct device *pclk_dev;
	const struct device *eswclk_dev;
	const struct device *eswphyclk_dev;
	const struct device *ethphyclk_dev;
	struct clock_control_ra_subsys_cfg pclk_subsys;
	struct clock_control_ra_subsys_cfg ethphyclk_subsys;
	const bool ethphyclk_enable;
	const struct pinctrl_dev_config *pin_cfg;
	void (*en_irq)(void);
};

#define ETHPHYCLK_25MHZ MHZ(25)
#define ETHPHYCLK_50MHZ MHZ(50)

static void eth_switch_cb(ether_switch_callback_args_t *args)
{
	ARG_UNUSED(args);
}

static int renesas_ra_eswm_init(const struct device *dev)
{
	struct eswm_renesas_ra_data *data = dev->data;
	const struct eswm_renesas_ra_config *config = dev->config;
	uint32_t gwcaclk, pclk, eswclk, eswphyclk, ethphyclk, phy_ref_rate;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int ret = 0;

	clock_control_get_rate(config->gwcaclk_dev, NULL, &gwcaclk);
	clock_control_get_rate(config->pclk_dev, NULL, &pclk);
	clock_control_get_rate(config->eswclk_dev, NULL, &eswclk);
	clock_control_get_rate(config->eswphyclk_dev, NULL, &eswphyclk);
	clock_control_get_rate(config->ethphyclk_dev, NULL, &ethphyclk);

	if (config->ethphyclk_enable) {
		clock_control_get_rate(config->ethphyclk_dev, NULL, &phy_ref_rate);
		if (phy_ref_rate != ETHPHYCLK_25MHZ && phy_ref_rate != ETHPHYCLK_50MHZ) {
			LOG_DBG("Internal PHY clock %d differ from 25/50 MHz", phy_ref_rate);
		}
	}

	if ((gwcaclk * 1.5 < eswclk) || (eswclk <= pclk) || (gwcaclk <= pclk)) {
		LOG_ERR("ESWM clock invalid");
		return -EIO;
	}

	fsp_err = R_LAYER3_SWITCH_Open(data->fsp_ctrl, data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("ESWM open failed, err=%d", fsp_err);
		return -EIO;
	}

	if (config->ethphyclk_enable) {
		ret = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
		if (ret) {
			return ret;
		}

		ret = clock_control_on(config->ethphyclk_dev,
				       (clock_control_subsys_t)&config->ethphyclk_subsys);
		if (ret) {
			LOG_DBG("failed to start eth phy clock, err=%d", ret);
		}
	} else {
		(void)clock_control_off(config->ethphyclk_dev,
					(clock_control_subsys_t)&config->ethphyclk_subsys);
	}

	config->en_irq();

	return 0;
}

#define DT_DRV_COMPAT renesas_ra_eswm

#define ETH_USE_INTERNAL_PHY_CLK(id)                                                               \
	UTIL_AND(DT_NODE_HAS_COMPAT(id, renesas_ra_ethernet_rmac),                                 \
		 DT_ENUM_HAS_VALUE(id, phy_clock_type, internal))

PINCTRL_DT_INST_DEFINE(0);

extern void layer3_switch_gwdi_isr(void);

static void renesas_ra_eswm_init_irq(void)
{
	R_ICU->IELSR_b[DT_INST_IRQ_BY_NAME(0, gwdi, irq)].IELS =
		BSP_PRV_IELS_ENUM(EVENT_ETHER_GWDI0);
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, gwdi, irq), DT_INST_IRQ_BY_NAME(0, gwdi, priority),
		    layer3_switch_gwdi_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, gwdi, irq));
}

static layer3_switch_extended_cfg_t eswm_ext_cfg = {
	.p_ether_phy_instances = {NULL, NULL},
	.fowarding_target_port_masks = {LAYER3_SWITCH_PORT_BITMASK_PORT2,
					LAYER3_SWITCH_PORT_BITMASK_PORT2},
};

static layer3_switch_instance_ctrl_t eswm_ctrl = {0};

static ether_switch_cfg_t eswm_cfg = {
	.channel = 0,
	.p_extend = &eswm_ext_cfg,
	.p_callback = &eth_switch_cb,
	.p_context = (void *)DEVICE_DT_INST_GET(0),
	.irq = DT_INST_IRQ_BY_NAME(0, gwdi, irq),
	.ipl = DT_INST_IRQ_BY_NAME(0, gwdi, priority),
};

ether_switch_instance_t eswm_inst = {
	.p_ctrl = (ether_switch_ctrl_t *)&eswm_ctrl,
	.p_cfg = (ether_switch_cfg_t *)&eswm_cfg,
	.p_api = (ether_switch_api_t *)&g_ether_switch_on_layer3_switch,
};

static struct eswm_renesas_ra_data eswm_data = {
	.fsp_ctrl = &eswm_ctrl,
	.fsp_cfg = &eswm_cfg,
};

static struct eswm_renesas_ra_config eswm_config = {
	.gwcaclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, gwcaclk)),
	.pclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, pclk)),
	.eswclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, eswclk)),
	.eswphyclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, eswphyclk)),
	.ethphyclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, ethphyclk)),
	.pclk_subsys.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_NAME(0, pclk, mstp),
	.pclk_subsys.stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(0, pclk, stop_bit),
	.ethphyclk_subsys.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_NAME(0, ethphyclk, mstp),
	.ethphyclk_subsys.stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(0, ethphyclk, stop_bit),
	.ethphyclk_enable =
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(0, ETH_USE_INTERNAL_PHY_CLK, (||)),
	.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.en_irq = &renesas_ra_eswm_init_irq,
};

DEVICE_DT_INST_DEFINE(0, renesas_ra_eswm_init, NULL, &eswm_data, &eswm_config, POST_KERNEL,
		      CONFIG_ETH_INIT_PRIORITY, NULL);

#if defined(CONFIG_ETH_RENESAS_RA_HW_BRIDGE)

/* FWPBFC[ch] controls the flood destination for frames whose destination MAC
 * is not yet in the hardware table.  Stride between registers is 0x10.
 */
#define RMAC_BRIDGE_FWPBFC_OFFSET 0x10U

static void rmac_bridge_set_flood_mask(uint8_t ch, uint32_t port_mask)
{
	volatile uint32_t *fwpbfc = (volatile uint32_t *)((uintptr_t)&R_MFWD->FWPBFC0 +
							  (ch * RMAC_BRIDGE_FWPBFC_OFFSET));

	*fwpbfc = R_MFWD_FWPBFC0_PBDV_Msk & port_mask;
}

/* Full hardware MAC table capacity — private to HAL source, mirrored here. */
#define RMAC_BRIDGE_MAC_ENTRY_MAX 0x7FFU

static int rmac_bridge_configure_learning(struct eswm_renesas_ra_data *eswm)
{
	layer3_switch_table_cfg_t tbl_cfg = {
		.p_table                    = NULL,
		.unsecure_entry_maximum_num = RMAC_BRIDGE_MAC_ENTRY_MAX,
		.mac_entry_aging_enable     = true,
		.mac_entry_aging_time_sec   = CONFIG_ETH_RENESAS_RA_HW_BRIDGE_MAC_AGING_TIME,
		.vlan_mode                  = LAYER3_SWITCH_VLAN_MODE_NO_VLAN,
	};
	uint32_t mask = eswm->bridge_port_mask;

	for (int p = 0; p <= BSP_FEATURE_ETHER_NUM_CHANNELS; p++) {
		bool bridged = !!(mask & BIT(p));

		tbl_cfg.port_cfg_list[p] = (layer3_switch_forwarding_port_cfg_t){
			.mac_table_enable             = bridged,
			.mac_hardware_learning_enable = bridged,
			.mac_reject_unknown           = false,
			.vlan_table_enable            = false,
		};
	}

	return (R_LAYER3_SWITCH_ConfigureTable(eswm->fsp_ctrl, &tbl_cfg) == FSP_SUCCESS) ? 0
											     : -EIO;
}

int renesas_ra_eswm_bridge_setif(const struct device *eswm_dev, uint8_t ch,
				  struct net_if *br, enum net_bridge_if_action action)
{
	struct eswm_renesas_ra_data *eswm = eswm_dev->data;

	switch (action) {
	case NET_BRIDGE_IF_ADD:
		if (eswm->offload_br != NULL && eswm->offload_br != br) {
			LOG_ERR("ESWM already bound to a different bridge");
			return -EBUSY;
		}
		eswm->offload_br = br;
		eswm->bridge_port_mask |= BIT(ch);
		return 0;
	case NET_BRIDGE_IF_DEL:
		eswm->bridge_port_mask &= ~BIT(ch);
		rmac_bridge_set_flood_mask(ch, LAYER3_SWITCH_PORT_BITMASK_PORT2);
		if (eswm->bridge_port_mask == 0) {
			eswm->offload_br = NULL;
		}
		return rmac_bridge_configure_learning(eswm);
	default:
		return -EINVAL;
	}
}

int renesas_ra_eswm_bridge_setfwd(const struct device *eswm_dev, uint8_t ch,
				   struct net_if *br, enum net_bridge_fwd_action action)
{
	struct eswm_renesas_ra_data *eswm = eswm_dev->data;
	int ret;

	ARG_UNUSED(br);

	switch (action) {
	case NET_BRIDGE_FWD_START:
		/* Enable hardware MAC learning for all bridged ports.
		 * ConfigureTable resets the MAC table — call it here, after all
		 * ports are recorded in bridge_port_mask, not in bridge_setif.
		 */
		ret = rmac_bridge_configure_learning(eswm);
		if (ret < 0) {
			return ret;
		}
		/* Flood unknown-destination frames to all other bridge ports + CPU.
		 * This bootstraps hardware MAC learning: the partner port sees the
		 * frame, learns the source MAC, and subsequent known-unicast traffic
		 * is forwarded in hardware without CPU involvement.
		 */
		rmac_bridge_set_flood_mask(
			ch,
			(eswm->bridge_port_mask | LAYER3_SWITCH_PORT_BITMASK_PORT2) & ~BIT(ch));
		return 0;
	case NET_BRIDGE_FWD_STOP:
		rmac_bridge_set_flood_mask(ch, LAYER3_SWITCH_PORT_BITMASK_PORT2);
		eswm->bridge_port_mask &= ~BIT(ch);
		if (eswm->bridge_port_mask == 0) {
			eswm->offload_br = NULL;
		}
		return rmac_bridge_configure_learning(eswm);
	default:
		return -EINVAL;
	}
}

/* Static allocation — GetTable writes through the MAC pointers,
 * so they must be pre-set to valid buffers before the call.
 * GetTable also ignores mac_list_length as input; it iterates all
 * 2047 hardware slots and increments the counter only when an entry
 * is found, so the array only needs to hold the expected entry count.
 */
#define FDB_DUMP_MAX_ENTRIES 32
static layer3_switch_table_entry_t fdb_entries[FDB_DUMP_MAX_ENTRIES];
static uint8_t fdb_dst_mac[FDB_DUMP_MAX_ENTRIES][NET_ETH_ADDR_LEN];
static uint8_t fdb_src_mac[FDB_DUMP_MAX_ENTRIES][NET_ETH_ADDR_LEN];

int renesas_ra_eswm_bridge_fdb_dump(const struct device *eswm_dev,
				    void (*cb)(const uint8_t *mac, uint32_t port_mask,
					      bool dynamic, void *user_data),
				    void *user_data)
{
	struct eswm_renesas_ra_data *eswm = eswm_dev->data;
	layer3_switch_table_t tbl;

	for (int i = 0; i < FDB_DUMP_MAX_ENTRIES; i++) {
		fdb_entries[i].target_frame.p_destination_mac_address = fdb_dst_mac[i];
		fdb_entries[i].target_frame.p_source_mac_address      = fdb_src_mac[i];
	}

	tbl.p_mac_entry_list  = fdb_entries;
	tbl.mac_list_length   = 0;
	tbl.p_vlan_entry_list = NULL;
	tbl.vlan_list_length  = 0;
	tbl.p_l3_entry_list   = NULL;
	tbl.l3_list_length    = 0;

	if (R_LAYER3_SWITCH_GetTable(eswm->fsp_ctrl, &tbl) != FSP_SUCCESS) {
		return -EIO;
	}

	for (uint32_t i = 0; i < tbl.mac_list_length; i++) {
		layer3_switch_table_entry_t *e = &fdb_entries[i];
		/* Hardware learning creates source-match entries (MACSSLVR).
		 * Static software entries use destination-match (MACDSLVR).
		 */
		const uint8_t *mac = e->target_frame.p_source_mac_address[0]
				     ? e->target_frame.p_source_mac_address
				     : e->target_frame.p_destination_mac_address;

		cb(mac, e->entry_cfg.destination_ports,
		   e->entry_cfg.mac.dinamic_entry, user_data);
	}

	return 0;
}

#endif /* CONFIG_ETH_RENESAS_RA_HW_BRIDGE */
