/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BT_MESH_RPR_CLI_H__
#define ZEPHYR_INCLUDE_BT_MESH_RPR_CLI_H__

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/mesh/access.h>
#include <zephyr/bluetooth/mesh/rpr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_mesh_rpr_cli Remote Provisioning Client model
 * @ingroup bt_mesh
 * @{
 */

/** Special value for the @c max_devs parameter of @ref bt_mesh_rpr_scan_start.
 *
 *  Tells the Remote Provisioning Server not to put restrictions on the max
 *  number of devices reported to the Client.
 */
#define BT_MESH_RPR_SCAN_MAX_DEVS_ANY 0

struct bt_mesh_rpr_cli;

/**
 *
 * @brief Remote Provisioning Client model composition data entry.
 *
 * @param _cli Pointer to a @ref bt_mesh_rpr_cli instance.
 */
#define BT_MESH_MODEL_RPR_CLI(_cli)                                            \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_REMOTE_PROV_CLI,                     \
			 _bt_mesh_rpr_cli_op, NULL, _cli, &_bt_mesh_rpr_cli_cb)

/** Scan status response */
struct bt_mesh_rpr_scan_status {
	/** Current scan status */
	enum bt_mesh_rpr_status status;
	/** Current scan state */
	enum bt_mesh_rpr_scan scan;
	/** Max number of devices to report in current scan. */
	uint8_t max_devs;
	/** Seconds remaining of the scan. */
	uint8_t timeout;
};

/** Remote Provisioning Server scanning capabilities */
struct bt_mesh_rpr_caps {
	/** Max number of scannable devices */
	uint8_t max_devs;
	/** Supports active scan */
	bool active_scan;
};

/** Remote Provisioning Client model instance. */
struct bt_mesh_rpr_cli {
	/** @brief Scan report callback.
	 *
	 *  @param cli      Remote Provisioning Client.
	 *  @param srv      Remote Provisioning Server.
	 *  @param unprov   Unprovisioned device.
	 *  @param adv_data Advertisement data for the unprovisioned device, or
	 *                  NULL if extended scanning hasn't been enabled. An
	 *                  empty buffer indicates that the extended scanning
	 *                  finished without collecting additional information.
	 */
	void (*scan_report)(struct bt_mesh_rpr_cli *cli,
			    const struct bt_mesh_rpr_node *srv,
			    struct bt_mesh_rpr_unprov *unprov,
			    struct net_buf_simple *adv_data);

	/* Internal parameters */

	struct bt_mesh_msg_ack_ctx scan_ack_ctx;
	struct bt_mesh_msg_ack_ctx prov_ack_ctx;

	struct {
		struct k_work_delayable timeout;
		struct bt_mesh_rpr_node srv;
		uint8_t time;
		uint8_t tx_pdu;
		uint8_t rx_pdu;
		enum bt_mesh_rpr_link_state state;
	} link;

	const struct bt_mesh_model *mod;
};

/** @brief Get scanning capabilities of Remote Provisioning Server.
 *
 *  @param cli  Remote Provisioning Client.
 *  @param srv  Remote Provisioning Server.
 *  @param caps Capabilities response buffer.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_rpr_scan_caps_get(struct bt_mesh_rpr_cli *cli,
			      const struct bt_mesh_rpr_node *srv,
			      struct bt_mesh_rpr_caps *caps);

/** @brief Get current scanning state of Remote Provisioning Server.
 *
 *  @param cli    Remote Provisioning Client.
 *  @param srv    Remote Provisioning Server.
 *  @param status Scan status response buffer.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_rpr_scan_get(struct bt_mesh_rpr_cli *cli,
			 const struct bt_mesh_rpr_node *srv,
			 struct bt_mesh_rpr_scan_status *status);

/** @brief Start scanning for unprovisioned devices.
 *
 *  Tells the Remote Provisioning Server to start scanning for unprovisioned
 *  devices. The Server will report back the results through the @ref
 *  bt_mesh_rpr_cli::scan_report callback.
 *
 *  Use the @c uuid parameter to scan for a specific device, or leave it as NULL
 *  to report all unprovisioned devices.
 *
 *  The Server will ignore duplicates, and report up to @c max_devs number of
 *  devices. Requesting a @c max_devs number that's higher than the Server's
 *  capability will result in an error.
 *
 *  @param cli      Remote Provisioning Client.
 *  @param srv      Remote Provisioning Server.
 *  @param uuid     Device UUID to scan for, or NULL to report all devices.
 *  @param timeout  Scan timeout in seconds. Must be at least 1 second.
 *  @param max_devs Max number of devices to report, or 0 to report as many as
 *                  possible.
 *  @param status   Scan status response buffer.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_rpr_scan_start(struct bt_mesh_rpr_cli *cli,
			   const struct bt_mesh_rpr_node *srv,
			   const uint8_t uuid[16], uint8_t timeout,
			   uint8_t max_devs,
			   struct bt_mesh_rpr_scan_status *status);

/** @brief Start extended scanning for unprovisioned devices.
 *
 *  Extended scanning supplements regular unprovisioned scanning, by allowing
 *  the Server to report additional data for a specific device. The Remote
 *  Provisioning Server will use active scanning to request a scan
 *  response from the unprovisioned device, if supported. If no UUID is
 *  provided, the Server will report a scan on its own OOB information and
 *  advertising data.
 *
 *  Use the ad_types array to specify which AD types to include in the scan
 *  report. Some AD types invoke special behavior:
 *  - @ref BT_DATA_NAME_COMPLETE Will report both the complete and the
 *    shortened name.
 *  - @ref BT_DATA_URI If the unprovisioned beacon contains a URI hash, the
 *    Server will extend the scanning to include packets other than
 *    the scan response, to look for URIs matching the URI hash. Only matching
 *    URIs will be reported.
 *
 *  The following AD types should not be used:
 *  - @ref BT_DATA_NAME_SHORTENED
 *  - @ref BT_DATA_UUID16_SOME
 *  - @ref BT_DATA_UUID32_SOME
 *  - @ref BT_DATA_UUID128_SOME
 *
 *  Additionally, each AD type should only occur once.
 *
 *  @param cli      Remote Provisioning Client.
 *  @param srv      Remote Provisioning Server.
 *  @param uuid     Device UUID to start extended scanning for, or NULL to scan
 *                  the remote server.
 *  @param timeout  Scan timeout in seconds. Valid values from @ref BT_MESH_RPR_EXT_SCAN_TIME_MIN
 *                  to @ref BT_MESH_RPR_EXT_SCAN_TIME_MAX. Ignored if UUID is NULL.
 *  @param ad_types List of AD types to include in the scan report. Must contain 1 to
 *                  CONFIG_BT_MESH_RPR_AD_TYPES_MAX entries.
 *  @param ad_count Number of AD types in @c ad_types.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_rpr_scan_start_ext(struct bt_mesh_rpr_cli *cli,
			       const struct bt_mesh_rpr_node *srv,
			       const uint8_t uuid[16], uint8_t timeout,
			       const uint8_t *ad_types, size_t ad_count);

/** @brief Stop any ongoing scanning on the Remote Provisioning Server.
 *
 *  @param cli    Remote Provisioning Client.
 *  @param srv    Remote Provisioning Server.
 *  @param status Scan status response buffer.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_rpr_scan_stop(struct bt_mesh_rpr_cli *cli,
			  const struct bt_mesh_rpr_node *srv,
			  struct bt_mesh_rpr_scan_status *status);

/** @brief Get the current link status of the Remote Provisioning Server.
 *
 *  @param cli Remote Provisioning Client.
 *  @param srv Remote Provisioning Server.
 *  @param rsp Link status response buffer.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_rpr_link_get(struct bt_mesh_rpr_cli *cli,
			 const struct bt_mesh_rpr_node *srv,
			 struct bt_mesh_rpr_link *rsp);

/** @brief Close any open link on the Remote Provisioning Server.
 *
 *  @param cli Remote Provisioning Client.
 *  @param srv Remote Provisioning Server.
 *  @param rsp Link status response buffer.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_rpr_link_close(struct bt_mesh_rpr_cli *cli,
			   const struct bt_mesh_rpr_node *srv,
			   struct bt_mesh_rpr_link *rsp);

/** @brief Get the current transmission timeout value.
 *
 *  @return The configured transmission timeout in milliseconds.
 */
int32_t bt_mesh_rpr_cli_timeout_get(void);

/** @brief Set the transmission timeout value.
 *
 *  The transmission timeout controls the amount of time the Remote Provisioning
 *  Client models will wait for a response from the Server.
 *
 *  @param timeout The new transmission timeout.
 */
void bt_mesh_rpr_cli_timeout_set(int32_t timeout);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_rpr_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_rpr_cli_cb;
/** @endcond */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BT_MESH_RPR_CLI_H__ */
