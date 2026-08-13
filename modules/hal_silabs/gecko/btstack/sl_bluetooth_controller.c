/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sl_bluetooth_controller.h>

#include <assert.h>
#include <stdbool.h>
#include <em_common.h>
#include <sl_rail_util_pa_config.h>
#include <sl_btctrl_linklayer.h>
#include <sl_btctrl_hci.h>
#include <sl_bt_ll_config.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/util_macro.h>
#include <sl_power_manager.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gecko_btctrl, CONFIG_BT_HCI_DRIVER_LOG_LEVEL);

#if defined(CONFIG_BT_MAX_CONN)
#define SL_BT_CONFIG_MAX_CONNECTIONS CONFIG_BT_MAX_CONN
#elif defined(CONFIG_BT_PER_ADV_SYNC)
#define SL_BT_CONFIG_MAX_CONNECTIONS 1
#else
#define SL_BT_CONFIG_MAX_CONNECTIONS 0
#endif

sl_status_t sl_bt_ll_deinit(void);
sl_status_t ll_connPowerControlEnable(const sl_bt_ll_power_control_config_t *config);
sl_status_t ll_initDefaultPowerLevelRange(int16_t minPower, int16_t maxPower);

/* NOTE: This is an internal, undocumented variable. */
extern void *ll_connSchInit;

static void conn_sch_init_stub(void)
{
}

/* Compared to the original, this returns a status. */
static sl_status_t sl_bt_controller_init_zephyr(void)
{
	sl_status_t status;

	if (sl_btctrl_is_initialized() == true) {
		return SL_STATUS_OK;
	}

	/* WORKAROUND: sl_btctrl_init_basic calls ll_initAllocMem which calls ll_connAllocMem which
	 * calls the function pointer at ll_connSchInit. If we didn't call sl_btctrl_init_conn, it
	 * will be NULL though.
	 */
	ll_connSchInit = conn_sch_init_stub;

#ifndef CONFIG_BT_CTLR_PHY_2M
	sl_btctrl_disable_2m_phy();
#endif
#ifndef CONFIG_BT_CTLR_PHY_CODED
	sl_btctrl_disable_coded_phy();
#endif

	if (sl_btctrl_init_mem(CONFIG_BT_SILABS_EFR32_BUFFER_MEMORY) == 0) {
		return SL_STATUS_NO_MORE_RESOURCE;
	}
	sl_btctrl_configure_le_buffer_size(CONFIG_BT_BUF_ACL_TX_COUNT);

	sli_btctrl_events_init();

	status = sl_btctrl_init_ll();
	if (status != SL_STATUS_OK) {
		return status;
	}

#ifdef CONFIG_BT_PER_ADV_RSP
	struct sl_btctrl_pawr_advertiser_config _pawr_config = {
		.max_pawr_sets = CONFIG_BT_SILABS_EFR32_MAX_PAWR_ADVERTISERS,
		.max_advertised_data_length_hint =
			CONFIG_BT_SILABS_EFR32_MAX_PAWR_ADVERTISED_DATA_LENGTH_HINT,
		.subevent_data_request_count = CONFIG_BT_SILABS_EFR32_PAWR_PACKET_REQUEST_COUNT,
		.subevent_data_request_advance = CONFIG_BT_SILABS_EFR32_PAWR_PACKET_REQUEST_ADVANCE,
	};
	status = sl_btctrl_pawr_advertiser_configure(&_pawr_config);
	if (status != SL_STATUS_OK) {
		return status;
	}
#endif

#ifdef CONFIG_BT_PER_ADV_SYNC_RSP
	struct sl_btctrl_pawr_synchronizer_config _pawr_config = {
		.max_pawr_sets = CONFIG_BT_SILABS_EFR32_MAX_PAWR_SYNCHRONIZERS,
	};
	status = sl_btctrl_pawr_synchronizer_configure(&_pawr_config);
	if (status != SL_STATUS_OK) {
		return status;
	}
#endif

#ifdef CONFIG_BT_BROADCASTER
	sl_btctrl_init_adv();
#endif

#if defined(CONFIG_BT_OBSERVER) || defined(CONFIG_BT_EXT_ADV)
	sl_btctrl_init_scan();
#endif

#ifdef CONFIG_BT_CONN
	sl_btctrl_init_conn();
#endif

#ifdef CONFIG_BT_PHY_UPDATE
	sl_btctrl_init_phy();
#endif

#ifdef CONFIG_BT_EXT_ADV
	sl_btctrl_init_adv_ext();
#endif

#ifdef CONFIG_BT_TRANSMIT_POWER_CONTROL
	status = ll_initDefaultPowerLevelRange(SL_BT_USE_MIN_POWER_LEVEL_SUPPORTED_BY_RADIO,
					       SL_BT_USE_MAX_POWER_LEVEL_SUPPORTED_BY_RADIO);
	if (status) {
		return status;
	}

	const sl_bt_ll_power_control_config_t power_control_config = {
		.activate_power_control = CONFIG_BT_SILABS_EFR32_ACTIVATE_POWER_CONTROL,
		.golden_rssi_min_1m = CONFIG_BT_SILABS_EFR32_GOLDEN_RSSI_MIN_1M,
		.golden_rssi_max_1m = CONFIG_BT_SILABS_EFR32_GOLDEN_RSSI_MAX_1M,
		.golden_rssi_min_2m = CONFIG_BT_SILABS_EFR32_GOLDEN_RSSI_MIN_2M,
		.golden_rssi_max_2m = CONFIG_BT_SILABS_EFR32_GOLDEN_RSSI_MAX_2M,
		.golden_rssi_min_coded_s8 = CONFIG_BT_SILABS_EFR32_GOLDEN_RSSI_MIN_CODED_S8,
		.golden_rssi_max_coded_s8 = CONFIG_BT_SILABS_EFR32_GOLDEN_RSSI_MAX_CODED_S8,
		.golden_rssi_min_coded_s2 = CONFIG_BT_SILABS_EFR32_GOLDEN_RSSI_MIN_CODED_S2,
		.golden_rssi_max_coded_s2 = CONFIG_BT_SILABS_EFR32_GOLDEN_RSSI_MAX_CODED_S2,
	};

	status = ll_connPowerControlEnable(&power_control_config);
	if (status) {
		return status;
	}
#endif

#ifdef CONFIG_BT_PER_ADV_SYNC
	status = sl_btctrl_alloc_periodic_scan(CONFIG_BT_PER_ADV_SYNC_MAX);
	if (status != SL_STATUS_OK) {
		return status;
	}

	sl_btctrl_init_periodic_scan();
#endif

#ifdef CONFIG_BT_PER_ADV
	status = sl_btctrl_alloc_periodic_adv(CONFIG_BT_SILABS_EFR32_MAX_PERIODIC_ADVERTISERS);
	if (status != SL_STATUS_OK) {
		return status;
	}

	sl_btctrl_init_periodic_adv();
#endif

#ifdef CONFIG_SILABS_GECKO_RAIL_MULTIPROTOCOL
	sl_btctrl_init_multiprotocol();
#endif

#if defined(CONFIG_BT_PER_ADV) && defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)
	sl_btctrl_init_past_local_sync_transfer();
#endif

#if defined(CONFIG_BT_PER_ADV_SYNC) && defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)
	sl_btctrl_init_past_remote_sync_transfer();
#endif

#ifdef CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER
	sl_btctrl_init_past_receiver();
#endif

#ifdef CONFIG_BT_EXT_ADV
	sl_btctrl_init_scan_ext();
#endif

	status = sl_btctrl_init_basic(SL_BT_CONFIG_MAX_CONNECTIONS,
				      CONFIG_BT_SILABS_EFR32_USER_ADVERTISERS,
				      CONFIG_BT_SILABS_EFR32_ACCEPT_LIST_SIZE);
	if (status != SL_STATUS_OK) {
		return status;
	}

#if defined(CONFIG_BT_BROADCASTER) || defined(CONFIG_BT_EXT_ADV)
	struct sl_btctrl_adv_config adv_config = {0};

	status = sl_btctrl_config_adv(&adv_config);
	if (status != SL_STATUS_OK) {
		return status;
	}
#endif

#ifdef CONFIG_BT_CTLR_PRIVACY
	sl_btctrl_init_privacy();
	status = sl_btctrl_allocate_resolving_list_memory(CONFIG_BT_CTLR_RL_SIZE);
	if (status != SL_STATUS_OK) {
		return status;
	}
#endif

#ifdef CONFIG_BT_SILABS_EFR32_HIGH_POWER_AFH
	status = sl_btctrl_init_afh(SL_BTCTRL_CHANNELMAP_FLAG_ACTIVE_ADAPTIVITY);
	if (status != SL_STATUS_OK) {
		return status;
	}
#endif

#ifdef CONFIG_BT_SILABS_EFR32_HIGH_POWER
	sl_btctrl_init_highpower();
#endif

#ifdef CONFIG_BT_CONN
	sl_btctrl_configure_completed_packets_reporting(
		CONFIG_BT_SILABS_EFR32_COMPLETED_PACKETS_THRESHOLD,
		CONFIG_BT_SILABS_EFR32_COMPLETED_PACKETS_TIMEOUT);
#endif

#ifdef CONFIG_BT_OBSERVER
	sl_btctrl_configure_max_queued_adv_reports(CONFIG_BT_SILABS_EFR32_MAX_QUEUED_ADV_REPORTS);
#endif

	sl_bthci_init_upper();
#ifdef CONFIG_BT_SILABS_EFR32_HCI_VS
	sl_bthci_init_vs();
#endif
	sl_btctrl_hci_parser_init_default();
#ifdef CONFIG_BT_CONN
	sl_btctrl_hci_parser_init_conn();
#endif
#ifdef CONFIG_BT_BROADCASTER
	sl_btctrl_hci_parser_init_adv();
#endif
#ifdef CONFIG_BT_PHY_UPDATE
	sl_btctrl_hci_parser_init_phy();
#endif
#if (defined(CONFIG_BT_PER_ADV) && defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)) ||             \
	(defined(CONFIG_BT_PER_ADV_SYNC) && defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)) ||    \
	defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER)
	sl_btctrl_hci_parser_init_past();
#endif
#ifdef CONFIG_BT_CTLR_PRIVACY
	sl_btctrl_hci_parser_init_privacy();
#endif

	return SL_STATUS_OK;
}

void sl_bt_controller_deinit(void)
{
	if (sl_btctrl_is_initialized() == false) {
		return;
	}

#ifdef CONFIG_BT_CTLR_PRIVACY
	sl_btctrl_allocate_resolving_list_memory(0);
#endif

	sl_bt_ll_deinit();

#ifdef CONFIG_BT_PER_ADV_SYNC
	sl_btctrl_alloc_periodic_scan(0);
#endif

#ifdef CONFIG_BT_PER_ADV
	sl_btctrl_alloc_periodic_adv(0);
#endif

#ifdef CONFIG_BT_PER_ADV_RSP
	struct sl_btctrl_pawr_advertiser_config _pawr_config = {
		.max_pawr_sets = 0,
	};
	sl_btctrl_pawr_advertiser_configure(&_pawr_config);
#endif

#ifdef CONFIG_BT_PER_ADV_SYNC_RSP
	struct sl_btctrl_pawr_synchronizer_config _pawr_config = {
		.max_pawr_sets = 0,
	};
	sl_btctrl_pawr_synchronizer_configure(&_pawr_config);
#endif

	sli_btctrl_deinit_mem();
}

/* Compared to the original, this returns a status. */
sl_status_t sl_btctrl_init_zephyr(void)
{
	return sl_bt_controller_init_zephyr();
}

/* For compatibility with Zephyrs HCI driver. */
void sl_btctrl_deinit(void)
{
	sl_bt_controller_deinit();
}

/* libbtcontroller can call this internally. */
void sl_bt_controller_init(void)
{
	sl_status_t status = sl_bt_controller_init_zephyr();

	if (status != SL_STATUS_OK) {
		LOG_ERR("controller init failed: %d", status);
	}
}

sl_status_t __wrap_bg_random_init(uint32_t m_w, uint32_t m_z)
{
	/* These are used for memory profiling only. */
	ARG_UNUSED(m_w);
	ARG_UNUSED(m_z);

	return SL_STATUS_OK;
}

void __wrap_bg_random_deinit(void)
{
}

sl_status_t __wrap_bg_random_get_data(unsigned int len, uint8_t *data)
{
	int ret;

	ret = sys_csrand_get(data, len);
	if (ret != 0) {
		LOG_ERR("Failed to obtain entropy, err %d", ret);
		return SL_STATUS_BT_CRYPTO;
	}

	return SL_STATUS_OK;
}
