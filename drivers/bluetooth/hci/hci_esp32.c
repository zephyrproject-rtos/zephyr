/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>

#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/version.h>

#include <zephyr/drivers/bluetooth.h>

#include <esp_bt.h>
#include <esp_mac.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_driver_esp32);

#if defined(CONFIG_SOC_SERIES_ESP32) || defined(CONFIG_SOC_SERIES_ESP32S3) ||                      \
	defined(CONFIG_SOC_SERIES_ESP32C3)
extern const char *btdm_controller_get_compile_version(void);
#define esp32_get_controller_version() btdm_controller_get_compile_version()
#else
extern char *ble_controller_get_compile_version(void);
#define esp32_get_controller_version() ble_controller_get_compile_version()
#endif

#if defined(CONFIG_SOC_SERIES_ESP32)
#define ESP32_HAS_ENHANCED_TX_POWER_API 0
#else
#define ESP32_HAS_ENHANCED_TX_POWER_API 1
#endif


static esp_power_level_t dbm_to_esp_power_level(int8_t dbm)
{
#if ESP32_HAS_ENHANCED_TX_POWER_API
	if (dbm >= 20) {
		return ESP_PWR_LVL_P20;
	} else if (dbm >= 18) {
		return ESP_PWR_LVL_P18;
	} else if (dbm >= 15) {
		return ESP_PWR_LVL_P15;
	} else if (dbm >= 12) {
		return ESP_PWR_LVL_P12;
	} else if (dbm >= 9) {
		return ESP_PWR_LVL_P9;
	} else if (dbm >= 6) {
		return ESP_PWR_LVL_P6;
	} else if (dbm >= 3) {
		return ESP_PWR_LVL_P3;
	} else if (dbm >= 0) {
		return ESP_PWR_LVL_N0;
	} else if (dbm >= -3) {
		return ESP_PWR_LVL_N3;
	} else if (dbm >= -6) {
		return ESP_PWR_LVL_N6;
	} else if (dbm >= -9) {
		return ESP_PWR_LVL_N9;
	} else if (dbm >= -12) {
		return ESP_PWR_LVL_N12;
	} else {
		return ESP_PWR_LVL_N15;
	}
#else
	if (dbm >= 9) {
		return ESP_PWR_LVL_P9;
	} else if (dbm >= 6) {
		return ESP_PWR_LVL_P6;
	} else if (dbm >= 3) {
		return ESP_PWR_LVL_P3;
	} else if (dbm >= 0) {
		return ESP_PWR_LVL_N0;
	} else if (dbm >= -3) {
		return ESP_PWR_LVL_N3;
	} else if (dbm >= -6) {
		return ESP_PWR_LVL_N6;
	} else if (dbm >= -9) {
		return ESP_PWR_LVL_N9;
	} else {
		return ESP_PWR_LVL_N12;
	}
#endif
}

static int8_t esp_power_level_to_dbm(esp_power_level_t level)
{
	switch (level) {
#if ESP32_HAS_ENHANCED_TX_POWER_API
	case ESP_PWR_LVL_N15:
		return -15;
#endif
	case ESP_PWR_LVL_N12:
		return -12;
	case ESP_PWR_LVL_N9:
		return -9;
	case ESP_PWR_LVL_N6:
		return -6;
	case ESP_PWR_LVL_N3:
		return -3;
	case ESP_PWR_LVL_N0:
		return 0;
	case ESP_PWR_LVL_P3:
		return 3;
	case ESP_PWR_LVL_P6:
		return 6;
	case ESP_PWR_LVL_P9:
		return 9;
#if ESP32_HAS_ENHANCED_TX_POWER_API
	case ESP_PWR_LVL_P12:
		return 12;
	case ESP_PWR_LVL_P15:
		return 15;
	case ESP_PWR_LVL_P18:
		return 18;
	case ESP_PWR_LVL_P20:
		return 20;
#endif
	default:
		return 0;
	}
}

#if ESP32_HAS_ENHANCED_TX_POWER_API
static esp_ble_enhanced_power_type_t handle_type_to_esp_enhanced_type(uint8_t handle_type)
{
	switch (handle_type) {
	case BT_HCI_VS_LL_HANDLE_TYPE_ADV:
		return ESP_BLE_ENHANCED_PWR_TYPE_ADV;
	case BT_HCI_VS_LL_HANDLE_TYPE_SCAN:
		return ESP_BLE_ENHANCED_PWR_TYPE_SCAN;
	case BT_HCI_VS_LL_HANDLE_TYPE_CONN:
		return ESP_BLE_ENHANCED_PWR_TYPE_CONN;
	default:
		return ESP_BLE_ENHANCED_PWR_TYPE_DEFAULT;
	}
}
#else
static esp_ble_power_type_t handle_to_esp_power_type(uint8_t handle_type, uint16_t handle)
{
	switch (handle_type) {
	case BT_HCI_VS_LL_HANDLE_TYPE_ADV:
		return ESP_BLE_PWR_TYPE_ADV;
	case BT_HCI_VS_LL_HANDLE_TYPE_SCAN:
		return ESP_BLE_PWR_TYPE_SCAN;
	case BT_HCI_VS_LL_HANDLE_TYPE_CONN:
		if (handle <= 8) {
			return (esp_ble_power_type_t)(ESP_BLE_PWR_TYPE_CONN_HDL0 + handle);
		}
		return ESP_BLE_PWR_TYPE_DEFAULT;
	default:
		return ESP_BLE_PWR_TYPE_DEFAULT;
	}
}
#endif

#define DT_DRV_COMPAT espressif_esp32_bt_hci

#define HCI_BT_ESP32_TIMEOUT K_MSEC(2000)

struct bt_esp32_data {
	bt_hci_recv_t recv;
};

static K_SEM_DEFINE(hci_send_sem, 0, 1);

static int bt_esp32_vs_send_cmd_complete(const struct device *dev, uint16_t opcode, const void *rsp,
					 size_t rsp_len)
{
	struct bt_esp32_data *hci = dev->data;
	struct net_buf *buf;
	struct bt_hci_evt_hdr *evt_hdr;
	struct bt_hci_evt_cmd_complete *cmd_complete;

	buf = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE, false, K_NO_WAIT);
	if (!buf) {
		LOG_ERR("No available event buffers for VS cmd complete");
		return -ENOMEM;
	}

	evt_hdr = net_buf_add(buf, sizeof(*evt_hdr));
	evt_hdr->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt_hdr->len = sizeof(*cmd_complete) + rsp_len;

	cmd_complete = net_buf_add(buf, sizeof(*cmd_complete));
	cmd_complete->ncmd = 1;
	cmd_complete->opcode = sys_cpu_to_le16(opcode);

	if (rsp && rsp_len > 0) {
		net_buf_add_mem(buf, rsp, rsp_len);
	}

	LOG_DBG("VS cmd complete: opcode 0x%04x, rsp_len %zu", opcode, rsp_len);

	hci->recv(dev, buf);

	return 0;
}

static int bt_esp32_vs_write_tx_power(const struct device *dev, struct net_buf *buf)
{
	struct bt_hci_cp_vs_write_tx_power_level *cp;
	struct bt_hci_rp_vs_write_tx_power_level rp;
	esp_power_level_t esp_level;
	esp_err_t err;
	int8_t requested_dbm;
	uint16_t handle;

	if (buf->len < sizeof(*cp)) {
		LOG_ERR("VS Write TX Power: invalid param length");
		rp.status = BT_HCI_ERR_INVALID_PARAM;
		rp.handle_type = 0;
		rp.handle = 0;
		rp.selected_tx_power = 0;
		return bt_esp32_vs_send_cmd_complete(dev, BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL, &rp,
						     sizeof(rp));
	}

	cp = (struct bt_hci_cp_vs_write_tx_power_level *)buf->data;
	requested_dbm = cp->tx_power_level;
	handle = sys_le16_to_cpu(cp->handle);

	LOG_DBG("VS Write TX Power: type=%u handle=%u power=%d dBm", cp->handle_type, handle,
		requested_dbm);

	esp_level = dbm_to_esp_power_level(requested_dbm);

#if ESP32_HAS_ENHANCED_TX_POWER_API
	esp_ble_enhanced_power_type_t esp_type;

	esp_type = handle_type_to_esp_enhanced_type(cp->handle_type);
	err = esp_ble_tx_power_set_enhanced(esp_type, handle, esp_level);
	if (err != ESP_OK) {
		LOG_WRN("esp_ble_tx_power_set_enhanced failed: %d", err);
		rp.status = BT_HCI_ERR_INVALID_PARAM;
	} else {
		rp.status = BT_HCI_ERR_SUCCESS;
	}
#else
	esp_ble_power_type_t esp_type;

	esp_type = handle_to_esp_power_type(cp->handle_type, handle);
	err = esp_ble_tx_power_set(esp_type, esp_level);
	if (err != ESP_OK) {
		LOG_WRN("esp_ble_tx_power_set failed: %d", err);
		rp.status = BT_HCI_ERR_INVALID_PARAM;
	} else {
		rp.status = BT_HCI_ERR_SUCCESS;
	}
#endif

	rp.handle_type = cp->handle_type;
	rp.handle = cp->handle;
	rp.selected_tx_power = esp_power_level_to_dbm(esp_level);

	LOG_DBG("VS Write TX Power response: status=%u selected=%d dBm", rp.status,
		rp.selected_tx_power);

	return bt_esp32_vs_send_cmd_complete(dev, BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL, &rp,
					     sizeof(rp));
}

static int bt_esp32_vs_read_tx_power(const struct device *dev, struct net_buf *buf)
{
	struct bt_hci_cp_vs_read_tx_power_level *cp;
	struct bt_hci_rp_vs_read_tx_power_level rp;
	esp_power_level_t esp_level;
	uint16_t handle;

	if (buf->len < sizeof(*cp)) {
		LOG_ERR("VS Read TX Power: invalid param length");
		rp.status = BT_HCI_ERR_INVALID_PARAM;
		rp.handle_type = 0;
		rp.handle = 0;
		rp.tx_power_level = 0;
		return bt_esp32_vs_send_cmd_complete(dev, BT_HCI_OP_VS_READ_TX_POWER_LEVEL, &rp,
						     sizeof(rp));
	}

	cp = (struct bt_hci_cp_vs_read_tx_power_level *)buf->data;
	handle = sys_le16_to_cpu(cp->handle);

	LOG_DBG("VS Read TX Power: type=%u handle=%u", cp->handle_type, handle);

#if ESP32_HAS_ENHANCED_TX_POWER_API
	esp_ble_enhanced_power_type_t esp_type;

	esp_type = handle_type_to_esp_enhanced_type(cp->handle_type);
	esp_level = esp_ble_tx_power_get_enhanced(esp_type, handle);

	if (esp_level == ESP_PWR_LVL_INVALID) {
		LOG_WRN("esp_ble_tx_power_get_enhanced returned invalid");
		rp.status = BT_HCI_ERR_INVALID_PARAM;
		rp.tx_power_level = 0;
	} else {
		rp.status = BT_HCI_ERR_SUCCESS;
		rp.tx_power_level = esp_power_level_to_dbm(esp_level);
	}
#else
	esp_ble_power_type_t esp_type;

	esp_type = handle_to_esp_power_type(cp->handle_type, handle);
	esp_level = esp_ble_tx_power_get(esp_type);

	rp.status = BT_HCI_ERR_SUCCESS;
	rp.tx_power_level = esp_power_level_to_dbm(esp_level);
#endif

	rp.handle_type = cp->handle_type;
	rp.handle = cp->handle;

	LOG_DBG("VS Read TX Power response: status=%u power=%d dBm", rp.status, rp.tx_power_level);

	return bt_esp32_vs_send_cmd_complete(dev, BT_HCI_OP_VS_READ_TX_POWER_LEVEL, &rp,
					     sizeof(rp));
}

static int bt_esp32_vs_read_static_addrs(const struct device *dev)
{
	struct {
		struct bt_hci_rp_vs_read_static_addrs hdr;
		struct bt_hci_vs_static_addr addr;
	} __packed rp;
	uint8_t mac[6];
	esp_err_t err;

	memset(&rp, 0, sizeof(rp));

	err = esp_read_mac(mac, ESP_MAC_BT);
	if (err != ESP_OK) {
		LOG_DBG("Failed to read BT MAC from eFuse");
		rp.hdr.status = BT_HCI_ERR_SUCCESS;
		rp.hdr.num_addrs = 0;
		return bt_esp32_vs_send_cmd_complete(dev, BT_HCI_OP_VS_READ_STATIC_ADDRS, &rp.hdr,
						     sizeof(rp.hdr));
	}

	/* Copy MAC and set static random address bits [47:46] = 0b11 */
	memcpy(rp.addr.bdaddr.val, mac, sizeof(rp.addr.bdaddr.val));
	BT_ADDR_SET_STATIC(&rp.addr.bdaddr);

	rp.hdr.status = BT_HCI_ERR_SUCCESS;
	rp.hdr.num_addrs = 1;

	LOG_DBG("VS Read Static Addrs: %02x:%02x:%02x:%02x:%02x:%02x", rp.addr.bdaddr.val[5],
		rp.addr.bdaddr.val[4], rp.addr.bdaddr.val[3], rp.addr.bdaddr.val[2],
		rp.addr.bdaddr.val[1], rp.addr.bdaddr.val[0]);

	return bt_esp32_vs_send_cmd_complete(dev, BT_HCI_OP_VS_READ_STATIC_ADDRS, &rp, sizeof(rp));
}

static int bt_esp32_vs_read_build_info(const struct device *dev)
{
	const char *version;
	size_t version_len;
	struct net_buf *buf;
	struct bt_hci_evt_hdr *evt_hdr;
	struct bt_hci_evt_cmd_complete *cmd_complete;
	struct bt_hci_rp_vs_read_build_info *rp;
	struct bt_esp32_data *hci = dev->data;

	version = esp32_get_controller_version();
	if (version == NULL) {
		version = "unknown";
	}
	version_len = strlen(version) + 1;

	buf = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE, false, K_NO_WAIT);
	if (!buf) {
		LOG_ERR("No available event buffers for VS read build info");
		return -ENOMEM;
	}

	evt_hdr = net_buf_add(buf, sizeof(*evt_hdr));
	evt_hdr->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt_hdr->len = sizeof(*cmd_complete) + sizeof(*rp) + version_len;

	cmd_complete = net_buf_add(buf, sizeof(*cmd_complete));
	cmd_complete->ncmd = 1;
	cmd_complete->opcode = sys_cpu_to_le16(BT_HCI_OP_VS_READ_BUILD_INFO);

	rp = net_buf_add(buf, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;

	net_buf_add_mem(buf, version, version_len);

	LOG_DBG("VS Read Build Info: %s", version);

	hci->recv(dev, buf);

	return 0;
}

static uint16_t bt_esp32_get_hw_variant(void)
{
#if defined(CONFIG_SOC_SERIES_ESP32)
	return BT_HCI_VS_HW_VAR_ESP32;
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
	return BT_HCI_VS_HW_VAR_ESP32S3;
#elif defined(CONFIG_SOC_SERIES_ESP32C2)
	return BT_HCI_VS_HW_VAR_ESP32C2;
#elif defined(CONFIG_SOC_SERIES_ESP32C3)
	return BT_HCI_VS_HW_VAR_ESP32C3;
#elif defined(CONFIG_SOC_SERIES_ESP32C6)
	return BT_HCI_VS_HW_VAR_ESP32C6;
#elif defined(CONFIG_SOC_SERIES_ESP32H2)
	return BT_HCI_VS_HW_VAR_ESP32H2;
#else
	return 0x0000;
#endif
}

static int bt_esp32_vs_read_version_info(const struct device *dev)
{
	struct bt_hci_rp_vs_read_version_info rp;

	memset(&rp, 0, sizeof(rp));

	rp.status = BT_HCI_ERR_SUCCESS;
	rp.hw_platform = sys_cpu_to_le16(BT_HCI_VS_HW_PLAT_ESPRESSIF);
	rp.hw_variant = sys_cpu_to_le16(bt_esp32_get_hw_variant());
	rp.fw_variant = 0;
	rp.fw_version = KERNEL_VERSION_MAJOR & 0xff;
	rp.fw_revision = sys_cpu_to_le16(KERNEL_VERSION_MINOR);
	rp.fw_build = sys_cpu_to_le32(KERNEL_PATCHLEVEL & 0xffff);

	LOG_DBG("VS Read Version Info: plat=0x%04x var=0x%04x fw=%u.%u.%u",
		BT_HCI_VS_HW_PLAT_ESPRESSIF, bt_esp32_get_hw_variant(), KERNEL_VERSION_MAJOR,
		KERNEL_VERSION_MINOR, KERNEL_PATCHLEVEL);

	return bt_esp32_vs_send_cmd_complete(dev, BT_HCI_OP_VS_READ_VERSION_INFO, &rp, sizeof(rp));
}

static int bt_esp32_vs_read_supported_commands(const struct device *dev)
{
	struct bt_hci_rp_vs_read_supported_commands rp;

	memset(&rp, 0, sizeof(rp));
	rp.status = BT_HCI_ERR_SUCCESS;

	rp.commands[0] |= BIT(0);
	rp.commands[0] |= BIT(1);
	rp.commands[0] |= BIT(2);
	rp.commands[0] |= BIT(7);
	rp.commands[1] |= BIT(0);
	rp.commands[1] |= BIT(5);
	rp.commands[1] |= BIT(6);

	LOG_DBG("VS Read Supported Commands");

	return bt_esp32_vs_send_cmd_complete(dev, BT_HCI_OP_VS_READ_SUPPORTED_COMMANDS, &rp,
					     sizeof(rp));
}

static int bt_esp32_vs_read_supported_features(const struct device *dev)
{
	struct bt_hci_rp_vs_read_supported_features rp;

	memset(&rp, 0, sizeof(rp));
	rp.status = BT_HCI_ERR_SUCCESS;

	rp.features[0] |= BIT(0);

	LOG_DBG("VS Read Supported Features");

	return bt_esp32_vs_send_cmd_complete(dev, BT_HCI_OP_VS_READ_SUPPORTED_FEATURES, &rp,
					     sizeof(rp));
}

static uint16_t bt_esp32_get_vs_opcode(struct net_buf *buf)
{
	struct bt_hci_cmd_hdr *hdr;

	if (buf->len < sizeof(*hdr)) {
		return 0;
	}

	if (buf->data[0] != BT_HCI_H4_CMD) {
		return 0;
	}

	hdr = (struct bt_hci_cmd_hdr *)(buf->data + 1);
	return sys_le16_to_cpu(hdr->opcode);
}

static int bt_esp32_handle_vs_cmd(const struct device *dev, struct net_buf *buf)
{
	uint16_t opcode;
	struct net_buf cmd_buf;

	opcode = bt_esp32_get_vs_opcode(buf);

	cmd_buf.data = buf->data + 1 + sizeof(struct bt_hci_cmd_hdr);
	cmd_buf.len = buf->len - 1 - sizeof(struct bt_hci_cmd_hdr);

	switch (opcode) {
	case BT_HCI_OP_VS_READ_VERSION_INFO:
		return bt_esp32_vs_read_version_info(dev);
	case BT_HCI_OP_VS_READ_SUPPORTED_COMMANDS:
		return bt_esp32_vs_read_supported_commands(dev);
	case BT_HCI_OP_VS_READ_SUPPORTED_FEATURES:
		return bt_esp32_vs_read_supported_features(dev);
	case BT_HCI_OP_VS_READ_BUILD_INFO:
		return bt_esp32_vs_read_build_info(dev);
	case BT_HCI_OP_VS_READ_STATIC_ADDRS:
		return bt_esp32_vs_read_static_addrs(dev);
	case BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL:
		return bt_esp32_vs_write_tx_power(dev, &cmd_buf);
	case BT_HCI_OP_VS_READ_TX_POWER_LEVEL:
		return bt_esp32_vs_read_tx_power(dev, &cmd_buf);
	default:
		return -ENOTSUP;
	}
}

static bool is_hci_event_discardable(uint8_t evt_code, const uint8_t *payload, size_t plen)
{
	switch (evt_code) {
#if defined(CONFIG_BT_CLASSIC)
	case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
	case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
		return true;
#endif
	case BT_HCI_EVT_LE_META_EVENT: {
		/* Need at least 1 byte to read LE subevent safely */
		if (plen < 1U) {
			return false;
		}
		uint8_t subevt_type = payload[0];

		switch (subevt_type) {
		case BT_HCI_EVT_LE_ADVERTISING_REPORT:
		case BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT:
			return true;
		default:
			return false;
		}
	}
	default:
		return false;
	}
}

static struct net_buf *bt_esp_evt_recv(uint8_t *data, size_t remaining)
{
	bool discardable = false;
	struct bt_hci_evt_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data for event header");
		return NULL;
	}

	memcpy((void *)&hdr, data, sizeof(hdr));
	data += sizeof(hdr);
	remaining -= sizeof(hdr);

	if (remaining != hdr.len) {
		LOG_ERR("Event payload length is not correct");
		return NULL;
	}
	LOG_DBG("len %u", hdr.len);

	discardable = is_hci_event_discardable(hdr.evt, data, remaining);

	buf = bt_buf_get_evt(hdr.evt, discardable, K_NO_WAIT);
	if (!buf) {
		if (discardable) {
			LOG_DBG("Discardable buffer pool full, ignoring event");
		} else {
			LOG_ERR("No available event buffers!");
		}
		return buf;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));

	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < remaining) {
		LOG_ERR("Not enough space in buffer %zu/%zu", remaining, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, data, remaining);

	return buf;
}

static struct net_buf *bt_esp_acl_recv(uint8_t *data, size_t remaining)
{
	struct bt_hci_acl_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data for ACL header");
		return NULL;
	}

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	if (buf) {
		memcpy((void *)&hdr, data, sizeof(hdr));
		data += sizeof(hdr);
		remaining -= sizeof(hdr);

		net_buf_add_mem(buf, &hdr, sizeof(hdr));
	} else {
		LOG_ERR("No available ACL buffers!");
		return NULL;
	}

	if (remaining != sys_le16_to_cpu(hdr.len)) {
		LOG_ERR("ACL payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}

	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < remaining) {
		LOG_ERR("Not enough space in buffer %zu/%zu", remaining, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	LOG_DBG("len %u", remaining);
	net_buf_add_mem(buf, data, remaining);

	return buf;
}

static struct net_buf *bt_esp_iso_recv(uint8_t *data, size_t remaining)
{
	struct bt_hci_iso_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data for ISO header");
		return NULL;
	}

	buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
	if (buf) {
		memcpy((void *)&hdr, data, sizeof(hdr));
		data += sizeof(hdr);
		remaining -= sizeof(hdr);

		net_buf_add_mem(buf, &hdr, sizeof(hdr));
	} else {
		LOG_ERR("No available ISO buffers!");
		return NULL;
	}

	if (remaining != bt_iso_hdr_len(sys_le16_to_cpu(hdr.len))) {
		LOG_ERR("ISO payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}

	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < remaining) {
		LOG_ERR("Not enough space in buffer %zu/%zu", remaining, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	LOG_DBG("len %zu", remaining);
	net_buf_add_mem(buf, data, remaining);

	return buf;
}

static int hci_esp_host_rcv_pkt(uint8_t *data, uint16_t len)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_esp32_data *hci = dev->data;
	uint8_t pkt_indicator;
	struct net_buf *buf = NULL;
	size_t remaining = len;

	LOG_HEXDUMP_DBG(data, len, "host packet data:");

	pkt_indicator = *data++;
	remaining -= sizeof(pkt_indicator);

	switch (pkt_indicator) {
	case BT_HCI_H4_EVT:
		buf = bt_esp_evt_recv(data, remaining);
		break;

	case BT_HCI_H4_ACL:
		buf = bt_esp_acl_recv(data, remaining);
		break;

	case BT_HCI_H4_SCO:
		buf = bt_esp_iso_recv(data, remaining);
		break;

	default:
		LOG_ERR("Unknown HCI type %u", pkt_indicator);
		return -1;
	}

	if (buf) {
		LOG_DBG("Calling bt_recv(%p)", buf);

		hci->recv(dev, buf);
	}

	return 0;
}

static void hci_esp_controller_rcv_pkt_ready(void)
{
	k_sem_give(&hci_send_sem);
}

static esp_vhci_host_callback_t vhci_host_cb = {
	hci_esp_controller_rcv_pkt_ready,
	hci_esp_host_rcv_pkt,
};

static int bt_esp32_send(const struct device *dev, struct net_buf *buf)
{
	int err = 0;

	LOG_DBG("buf %p type %u len %u", buf, buf->data[0], buf->len);

	LOG_HEXDUMP_DBG(buf->data, buf->len, "Final HCI buffer:");

	err = bt_esp32_handle_vs_cmd(dev, buf);
	if (err == 0) {
		net_buf_unref(buf);
		return 0;
	} else if (err != -ENOTSUP) {
		return err;
	}

	err = 0;

	if (k_sem_take(&hci_send_sem, HCI_BT_ESP32_TIMEOUT) != 0) {
		LOG_ERR("Send packet timeout error");
		err = -ETIMEDOUT;
	} else {
		if (!esp_vhci_host_check_send_available()) {
			LOG_WRN("VHCI not available, sending anyway");
		}
		esp_vhci_host_send_packet(buf->data, buf->len);
	}

	if (!err) {
		net_buf_unref(buf);
	}

	return err;
}

static int bt_esp32_ble_init(void)
{
	int ret;
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

#if defined(CONFIG_BT_CLASSIC) && defined(CONFIG_SOC_SERIES_ESP32)
	esp_bt_mode_t mode = ESP_BT_MODE_BTDM;
#else
	esp_bt_mode_t mode = ESP_BT_MODE_BLE;
#endif

	ret = esp_bt_controller_init(&bt_cfg);
	if (ret == ESP_ERR_NO_MEM) {
		LOG_ERR("Not enough memory to initialize Bluetooth.");
		LOG_ERR("Consider increasing CONFIG_HEAP_MEM_POOL_SIZE value.");
		return -ENOMEM;
	} else if (ret != ESP_OK) {
		LOG_ERR("Unable to initialize the Bluetooth: %d", ret);
		return -EIO;
	}

	ret = esp_bt_controller_enable(mode);
	if (ret) {
		LOG_ERR("Bluetooth controller enable failed: %d", ret);
		return -EIO;
	}

	esp_vhci_host_register_callback(&vhci_host_cb);

	if (esp_vhci_host_check_send_available()) {
		k_sem_give(&hci_send_sem);
	}

	return 0;
}

static int bt_esp32_ble_deinit(void)
{
	int ret;

	ret = esp_bt_controller_disable();
	if (ret) {
		LOG_ERR("Bluetooth controller disable failed %d", ret);
		return ret;
	}

	ret = esp_bt_controller_deinit();
	if (ret) {
		LOG_ERR("Bluetooth controller deinit failed %d", ret);
		return ret;
	}

	return 0;
}

static int bt_esp32_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct bt_esp32_data *hci = dev->data;
	int err;

	k_sem_reset(&hci_send_sem);

	err = bt_esp32_ble_init();
	if (err) {
		return err;
	}

	hci->recv = recv;

	LOG_DBG("ESP32 BT started");

	return 0;
}

static int bt_esp32_close(const struct device *dev)
{
	struct bt_esp32_data *hci = dev->data;
	int err;

	err = bt_esp32_ble_deinit();
	if (err) {
		return err;
	}

	hci->recv = NULL;

	LOG_DBG("ESP32 BT stopped");

	return 0;
}

static DEVICE_API(bt_hci, drv) = {
	.open = bt_esp32_open,
	.send = bt_esp32_send,
	.close = bt_esp32_close,
};

#define BT_ESP32_DEVICE_INIT(inst)                                                                 \
	static struct bt_esp32_data bt_esp32_data_##inst = {};                                     \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &bt_esp32_data_##inst, NULL, POST_KERNEL,          \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv)

BT_ESP32_DEVICE_INIT(0)
