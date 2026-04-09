/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/math_extras.h>

#include "siwx91x_nwp.h"
#include "siwx91x_nwp_bus.h"
#include "siwx91x_nwp_api.h"

#include "device/silabs/si91x/wireless/ble/inc/rsi_bt_common.h"
#include "device/silabs/si91x/wireless/socket/inc/sl_si91x_socket_constants.h"
#include "service/network_manager/inc/sli_net_constants.h"
#include "sli_wifi/inc/sli_wifi_types.h"

LOG_MODULE_DECLARE(siwx91x_nwp, CONFIG_SIWX91X_NWP_LOG_LEVEL);

/* Contains low level helpers for NWP. Each function will run one (and only one) NWP command. This
 * is a first layer of abstraction for the driver.
 *
 * The function tries to expose only the relevant arguments to the upper layers. In some case,
 * several functions may be mapped to the same NWP API. It happens when API has several mode and
 * some argument are only meaningful in some mode (typically, siwx91x_nwp_fw_upgrade_write vs
 * siwx91x_nwp_fw_upgrade_start or siwx91x_nwp_flash_write vs siwx91x_nwp_flash_erase).
 *
 * We prefer to expose NWP agnostic parameters, however, for the complex API, rather than exposing
 * ridiculous number of parameters, we may expose the NWP structure to the caller.
 */


/* sli_si91x_driver_send_command */
static uint32_t siwx91x_nwp_send_cmd(const struct device *dev,
				     const void *data_buf, size_t data_len,
				     uint16_t command, int queue_id, uint8_t flags,
				     struct net_buf **reply_buf)
{
	struct siwx91x_frame_desc desc_buf;
	NET_BUF_DEFINE_STACK(data_container, data_buf, data_len, 0);
	NET_BUF_DEFINE_STACK(desc_container, &desc_buf, sizeof(desc_buf), sizeof(void *));
	struct net_buf *local_reply;
	uint32_t status;

	__ASSERT(!(flags & SIWX91X_FRAME_FLAG_ASYNC), "Invalid call");
	net_buf_frag_add(desc_container, net_buf_ref(data_container));
	net_buf_unref(data_container);

	local_reply = siwx91x_nwp_send_frame(dev, desc_container, command, queue_id, flags);
	__ASSERT(local_reply != NULL, "Corrupted state");
	status = *(uint16_t *)(&((struct siwx91x_frame_desc *)local_reply->data)->reserved[8]);
	if (reply_buf) {
		*reply_buf = net_buf_ref(local_reply);
	}
	net_buf_unref(local_reply);
	return status ? (0x10000 | status) : 0;
}

void siwx91x_nwp_get_firmware_version(const struct device *dev,
				      struct siwx91x_nwp_version *reply)
{
	struct net_buf *reply_buf;
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, NULL, 0, SLI_WLAN_REQ_FULL_FW_VERSION,
				      SLI_WLAN_MGMT_Q, 0, &reply_buf);
	__ASSERT(!status, "Corrupted NWP reply");

	net_buf_linearize(reply, sizeof(struct siwx91x_nwp_version),
			  reply_buf, sizeof(struct siwx91x_frame_desc), SIZE_MAX);
	net_buf_unref(reply_buf);
}

void siwx91x_nwp_opermode(const struct device *dev,
			  sl_wifi_system_boot_configuration_t *params)
{
	uint32_t status;

	__ASSERT(params->coex_mode != SL_SI91X_WLAN_MODE, "You mean SL_SI91X_WLAN_ONLY_MODE");
	__ASSERT(params->oper_mode != SL_SI91X_CONCURRENT_MODE, "Not supported");

	status = siwx91x_nwp_send_cmd(dev, params, sizeof(*params), SLI_WIFI_REQ_OPERMODE,
				      SLI_WLAN_MGMT_Q, SIWX91X_FRAME_FLAG_NO_LOCK, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_dynamic_pool(const struct device *dev, int tx, int rx, int global)
{
	sl_wifi_system_dynamic_pool_t params = {
		.tx_ratio_in_buffer_pool = tx,
		.rx_ratio_in_buffer_pool = rx,
		.global_ratio_in_buffer_pool = global,
	};
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WLAN_REQ_DYNAMIC_POOL,
				      SLI_WLAN_MGMT_Q, SIWX91X_FRAME_FLAG_NO_LOCK, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_feature(const struct device *dev, bool enable_pll)
{
	sli_si91x_feature_frame_request params = {
		.rf_type = RSI_INTERNAL_RF,
		.wireless_mode = SL_WIFI_HP_CHAIN,
		.enable_ppp = 0,
		.afe_type = 1,
		.feature_enables = SLI_FEAT_FRAME_PREAMBLE_DUTY_CYCLE |
				   SLI_FEAT_FRAME_LP_CHAIN |
				   SLI_FEAT_FRAME_IN_PACKET_DUTY_CYCLE,
		.pll_mode = enable_pll ? 1 : 0,
	};
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_COMMON_REQ_FEATURE_FRAME,
				      SLI_WLAN_MGMT_Q, SIWX91X_FRAME_FLAG_NO_LOCK, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_soft_reset(const struct device *dev)
{
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, NULL, 0, SLI_COMMON_REQ_SOFT_RESET,
				      SLI_WLAN_MGMT_Q, SIWX91X_FRAME_FLAG_NO_LOCK, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_force_assert(const struct device *dev)
{
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, NULL, 0, SLI_COMMON_REQ_ASSERT,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}
