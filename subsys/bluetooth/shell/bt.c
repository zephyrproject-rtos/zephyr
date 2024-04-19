/** @file
 * @brief Bluetooth shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/ead.h>

#include <zephyr/shell/shell.h>

#include "bt.h"
#include "ll.h"
#include "hci.h"
#include "../audio/shell/audio.h"

static bool no_settings_load;

uint8_t selected_id = BT_ID_DEFAULT;
const struct shell *ctx_shell;

#if defined(CONFIG_BT_CONN)
struct bt_conn *default_conn;

/* Connection context for BR/EDR legacy pairing in sec mode 3 */
static struct bt_conn *pairing_conn;

static struct bt_le_oob oob_local;
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
static struct bt_le_oob oob_remote;
#endif /* CONFIG_BT_SMP || CONFIG_BT_CLASSIC) */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_SMP)
static struct bt_conn_auth_info_cb auth_info_cb;
#endif /* CONFIG_BT_SMP */

#define NAME_LEN 30

#define KEY_STR_LEN 33

#define ADV_DATA_DELIMITER ", "

#define AD_SIZE 9

/*
 * Based on the maximum number of parameters for HCI_LE_Generate_DHKey
 * See BT Core Spec V5.2 Vol. 4, Part E, section 7.8.37
 */
#define HCI_CMD_MAX_PARAM 65

#if defined(CONFIG_BT_BROADCASTER)
enum {
	SHELL_ADV_OPT_CONNECTABLE,
	SHELL_ADV_OPT_DISCOVERABLE,
	SHELL_ADV_OPT_EXT_ADV,
	SHELL_ADV_OPT_APPEARANCE,
	SHELL_ADV_OPT_KEEP_RPA,

	SHELL_ADV_OPT_NUM,
};

static ATOMIC_DEFINE(adv_opt, SHELL_ADV_OPT_NUM);
#if defined(CONFIG_BT_EXT_ADV)
uint8_t selected_adv;
struct bt_le_ext_adv *adv_sets[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
static ATOMIC_DEFINE(adv_set_opt, SHELL_ADV_OPT_NUM)[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
#endif /* CONFIG_BT_EXT_ADV */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER) || defined(CONFIG_BT_USER_PHY_UPDATE)
static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case 0: return "No packets";
	case BT_GAP_LE_PHY_1M:
		return "LE 1M";
	case BT_GAP_LE_PHY_2M:
		return "LE 2M";
	case BT_GAP_LE_PHY_CODED:
		return "LE Coded";
	default:
		return "Unknown";
	}
}
#endif

#if defined(CONFIG_BT_CONN) || (defined(CONFIG_BT_BROADCASTER) && defined(CONFIG_BT_EXT_ADV))
static void print_le_addr(const char *desc, const bt_addr_le_t *addr)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	const char *addr_desc = bt_addr_le_is_identity(addr) ? "identity" :
				bt_addr_le_is_rpa(addr) ? "resolvable" :
				"non-resolvable";

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	shell_print(ctx_shell, "%s address: %s (%s)", desc, addr_str,
		    addr_desc);
}
#endif /* CONFIG_BT_CONN || (CONFIG_BT_BROADCASTER && CONFIG_BT_EXT_ADV) */

#if defined(CONFIG_BT_TRANSMIT_POWER_CONTROL)
static const char *tx_power_flag2str(int8_t flag)
{
	switch (flag) {
	case 0:
		return "Neither Max nor Min Tx Power";
	case 1:
		return "Tx Power Level is at minimum";
	case 2:
		return "Tx Power Level is at maximum";
	/* Current Tx Power Level is the only available one*/
	case 3:
		return "Tx Power Level is at minimum & maximum.";
	default:
		return "Unknown";
	}
}

static const char *tx_power_report_reason2str(uint8_t reason)
{
	switch (reason) {
	case BT_HCI_LE_TX_POWER_REPORT_REASON_LOCAL_CHANGED:
		return "Local Tx Power changed";
	case BT_HCI_LE_TX_POWER_REPORT_REASON_REMOTE_CHANGED:
		return "Remote Tx Power changed";
	case BT_HCI_LE_TX_POWER_REPORT_REASON_READ_REMOTE_COMPLETED:
		return "Completed to read remote Tx Power";
	default:
		return "Unknown";
	}
}

static const char *tx_pwr_ctrl_phy2str(enum bt_conn_le_tx_power_phy phy)
{
	switch (phy) {
	case BT_CONN_LE_TX_POWER_PHY_NONE:
		return "None";
	case BT_CONN_LE_TX_POWER_PHY_1M:
		return "LE 1M";
	case BT_CONN_LE_TX_POWER_PHY_2M:
		return "LE 2M";
	case BT_CONN_LE_TX_POWER_PHY_CODED_S8:
		return "LE Coded S8";
	case BT_CONN_LE_TX_POWER_PHY_CODED_S2:
		return "LE Coded S2";
	default:
		return "Unknown";
	}
}

static const char *enabled2str(bool enabled)
{
	if (enabled) {
		return "Enabled";
	} else {
		return "Disabled";
	}
}

#endif /* CONFIG_BT_TRANSMIT_POWER_CONTROL */

#if defined(CONFIG_BT_CENTRAL)
static int cmd_scan_off(const struct shell *sh);
static int cmd_connect_le(const struct shell *sh, size_t argc, char *argv[]);
static int cmd_scan_filter_clear_name(const struct shell *sh, size_t argc,
				      char *argv[]);

static struct bt_auto_connect {
	bt_addr_le_t addr;
	bool addr_set;
	bool connect_name;
} auto_connect;
#endif

#if defined(CONFIG_BT_OBSERVER)
static struct bt_scan_filter {
	char name[NAME_LEN];
	bool name_set;
	char addr[BT_ADDR_STR_LEN];
	bool addr_set;
	int8_t rssi;
	bool rssi_set;
	uint16_t pa_interval;
	bool pa_interval_set;
} scan_filter;

static const char scan_response_label[] = "[DEVICE]: ";
static bool scan_verbose_output;

#if defined(CONFIG_BT_EAD)
static uint8_t bt_shell_ead_session_key[BT_EAD_KEY_SIZE] = {0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5,
							    0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB,
							    0xCC, 0xCD, 0xCE, 0xCF};
static uint8_t bt_shell_ead_iv[BT_EAD_IV_SIZE] = {0xFB, 0x56, 0xE1, 0xDA, 0xDC, 0x7E, 0xAD, 0xF5};

/* this is the number of ad struct allowed */
#define BT_SHELL_EAD_MAX_AD 10
static size_t bt_shell_ead_ad_len;

#if defined(CONFIG_BT_CTLR_ADV_DATA_LEN_MAX)
/* this is the maximum total size of the ad data */
#define BT_SHELL_EAD_DATA_MAX_SIZE CONFIG_BT_CTLR_ADV_DATA_LEN_MAX
#else
#define BT_SHELL_EAD_DATA_MAX_SIZE 31
#endif
static size_t bt_shell_ead_data_size;
static uint8_t bt_shell_ead_data[BT_SHELL_EAD_DATA_MAX_SIZE] = {0};

int ead_update_ad(void);
#endif

static bool bt_shell_ead_decrypt_scan;

/**
 * @brief Compares two strings without case sensitivy
 *
 * @param substr The substring
 * @param str The string to find the substring in
 *
 * @return true if @substr is a substring of @p, else false
 */
static bool is_substring(const char *substr, const char *str)
{
	const size_t str_len = strlen(str);
	const size_t sub_str_len = strlen(substr);

	if (sub_str_len > str_len) {
		return false;
	}

	for (size_t pos = 0; pos < str_len; pos++) {
		if (pos + sub_str_len > str_len) {
			return false;
		}

		if (strncasecmp(substr, &str[pos], sub_str_len) == 0) {
			return true;
		}
	}

	return false;
}

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
	case BT_DATA_BROADCAST_NAME:
		memcpy(name, data->data, MIN(data->data_len, NAME_LEN - 1));
		return false;
	default:
		return true;
	}
}

static void print_data_hex(const uint8_t *data, uint8_t len, enum shell_vt100_color color)
{
	if (len == 0)
		return;

	shell_fprintf(ctx_shell, color, "0x");
	/* Reverse the byte order when printing as advertising data is LE
	 * and the MSB should be first in the printed output.
	 */
	for (int16_t i = len - 1; i >= 0; i--) {
		shell_fprintf(ctx_shell, color, "%02x", data[i]);
	}
}

static void print_data_set(uint8_t set_value_len,
			   const uint8_t *scan_data, uint8_t scan_data_len)
{
	uint8_t idx = 0;

	if (scan_data_len == 0 || set_value_len > scan_data_len) {
		return;
	}

	do {
		if (idx > 0) {
			shell_fprintf(ctx_shell, SHELL_INFO, ADV_DATA_DELIMITER);
		}

		print_data_hex(&scan_data[idx], set_value_len, SHELL_INFO);
		idx += set_value_len;
	} while (idx + set_value_len <= scan_data_len);

	if (idx < scan_data_len) {
		shell_fprintf(ctx_shell, SHELL_WARNING, " Excess data: ");
		print_data_hex(&scan_data[idx], scan_data_len - idx, SHELL_WARNING);
	}
}

static bool data_verbose_cb(struct bt_data *data, void *user_data)
{
	shell_fprintf(ctx_shell, SHELL_INFO, "%*sType 0x%02x: ",
		      strlen(scan_response_label), "", data->type);

	switch (data->type) {
	case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
	case BT_DATA_SOLICIT16:
		print_data_set(BT_UUID_SIZE_16, data->data, data->data_len);
		break;
	case BT_DATA_SVC_DATA16:
		/* Data starts with a UUID16 (2 bytes),
		 * the rest is unknown and printed as single bytes
		 */
		if (data->data_len < BT_UUID_SIZE_16) {
			shell_fprintf(ctx_shell, SHELL_WARNING,
				      "BT_DATA_SVC_DATA16 data length too short (%u)",
				      data->data_len);
			break;
		}
		print_data_set(BT_UUID_SIZE_16, data->data, BT_UUID_SIZE_16);
		if (data->data_len > BT_UUID_SIZE_16) {
			shell_fprintf(ctx_shell, SHELL_INFO, ADV_DATA_DELIMITER);
			print_data_set(1, data->data + BT_UUID_SIZE_16,
				       data->data_len - BT_UUID_SIZE_16);
		}
		break;
	case BT_DATA_UUID32_SOME:
	case BT_DATA_UUID32_ALL:
		print_data_set(BT_UUID_SIZE_32, data->data, data->data_len);
		break;
	case BT_DATA_SVC_DATA32:
		/* Data starts with a UUID32 (4 bytes),
		 * the rest is unknown and printed as single bytes
		 */
		if (data->data_len < BT_UUID_SIZE_32) {
			shell_fprintf(ctx_shell, SHELL_WARNING,
				      "BT_DATA_SVC_DATA32 data length too short (%u)",
				      data->data_len);
			break;
		}
		print_data_set(BT_UUID_SIZE_32, data->data, BT_UUID_SIZE_32);
		if (data->data_len > BT_UUID_SIZE_32) {
			shell_fprintf(ctx_shell, SHELL_INFO, ADV_DATA_DELIMITER);
			print_data_set(1, data->data + BT_UUID_SIZE_32,
				       data->data_len - BT_UUID_SIZE_32);
		}
		break;
	case BT_DATA_UUID128_SOME:
	case BT_DATA_UUID128_ALL:
	case BT_DATA_SOLICIT128:
		print_data_set(BT_UUID_SIZE_128, data->data, data->data_len);
		break;
	case BT_DATA_SVC_DATA128:
		/* Data starts with a UUID128 (16 bytes),
		 * the rest is unknown and printed as single bytes
		 */
		if (data->data_len < BT_UUID_SIZE_128) {
			shell_fprintf(ctx_shell, SHELL_WARNING,
				      "BT_DATA_SVC_DATA128 data length too short (%u)",
				      data->data_len);
			break;
		}
		print_data_set(BT_UUID_SIZE_128, data->data, BT_UUID_SIZE_128);
		if (data->data_len > BT_UUID_SIZE_128) {
			shell_fprintf(ctx_shell, SHELL_INFO, ADV_DATA_DELIMITER);
			print_data_set(1, data->data + BT_UUID_SIZE_128,
				       data->data_len - BT_UUID_SIZE_128);
		}
		break;
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
	case BT_DATA_BROADCAST_NAME:
		shell_fprintf(ctx_shell, SHELL_INFO, "%.*s", data->data_len, data->data);
		break;
	case BT_DATA_PUB_TARGET_ADDR:
	case BT_DATA_RAND_TARGET_ADDR:
	case BT_DATA_LE_BT_DEVICE_ADDRESS:
		print_data_set(BT_ADDR_SIZE, data->data, data->data_len);
		break;
	case BT_DATA_CSIS_RSI:
		print_data_set(3, data->data, data->data_len);
		break;
	case BT_DATA_ENCRYPTED_AD_DATA:
		shell_fprintf(ctx_shell, SHELL_INFO, "Encrypted Advertising Data: ");
		print_data_set(1, data->data, data->data_len);

		if (bt_shell_ead_decrypt_scan) {
#if defined(CONFIG_BT_EAD)
			shell_fprintf(ctx_shell, SHELL_INFO, "\n%*s[START DECRYPTED DATA]\n",
				      strlen(scan_response_label), "");

			int ead_err;
			struct net_buf_simple decrypted_buf;
			size_t decrypted_data_size = BT_EAD_DECRYPTED_PAYLOAD_SIZE(data->data_len);
			uint8_t decrypted_data[decrypted_data_size];

			ead_err = bt_ead_decrypt(bt_shell_ead_session_key, bt_shell_ead_iv,
						 data->data, data->data_len, decrypted_data);
			if (ead_err) {
				shell_error(ctx_shell, "Error during decryption (err %d)", ead_err);
			}

			net_buf_simple_init_with_data(&decrypted_buf, &decrypted_data[0],
						      decrypted_data_size);

			bt_data_parse(&decrypted_buf, &data_verbose_cb, user_data);

			shell_fprintf(ctx_shell, SHELL_INFO, "%*s[END DECRYPTED DATA]",
				      strlen(scan_response_label), "");
#endif
		}
		break;
	default:
		print_data_set(1, data->data, data->data_len);
	}

	shell_fprintf(ctx_shell, SHELL_INFO, "\n");

	return true;
}

static const char *scan_response_type_txt(uint8_t type)
{
	switch (type) {
	case BT_GAP_ADV_TYPE_ADV_IND:
		return "ADV_IND";
	case BT_GAP_ADV_TYPE_ADV_DIRECT_IND:
		return "ADV_DIRECT_IND";
	case BT_GAP_ADV_TYPE_ADV_SCAN_IND:
		return "ADV_SCAN_IND";
	case BT_GAP_ADV_TYPE_ADV_NONCONN_IND:
		return "ADV_NONCONN_IND";
	case BT_GAP_ADV_TYPE_SCAN_RSP:
		return "SCAN_RSP";
	case BT_GAP_ADV_TYPE_EXT_ADV:
		return "EXT_ADV";
	default:
		return "UNKNOWN";
	}
}

bool passes_scan_filter(const struct bt_le_scan_recv_info *info, const struct net_buf_simple *buf)
{

	if (scan_filter.rssi_set && (scan_filter.rssi > info->rssi)) {
		return false;
	}

	if (scan_filter.pa_interval_set &&
	    (scan_filter.pa_interval > BT_CONN_INTERVAL_TO_MS(info->interval))) {
		return false;
	}

	if (scan_filter.addr_set) {
		char le_addr[BT_ADDR_LE_STR_LEN] = {0};

		bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

		if (!is_substring(scan_filter.addr, le_addr)) {
			return false;
		}
	}

	if (scan_filter.name_set) {
		struct net_buf_simple buf_copy;
		char name[NAME_LEN] = {0};

		/* call to bt_data_parse consumes netbufs so shallow clone for verbose output */
		net_buf_simple_clone(buf, &buf_copy);
		bt_data_parse(&buf_copy, data_cb, name);

		if (!is_substring(scan_filter.name, name)) {
			return false;
		}
	}

	return true;
}

static void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];
	struct net_buf_simple buf_copy;

	if (!passes_scan_filter(info, buf)) {
		return;
	}

	if (scan_verbose_output) {
		/* call to bt_data_parse consumes netbufs so shallow clone for verbose output */
		net_buf_simple_clone(buf, &buf_copy);
	}

	(void)memset(name, 0, sizeof(name));

	bt_data_parse(buf, data_cb, name);
	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	shell_print(ctx_shell, "%s%s, AD evt type %u, RSSI %i %s "
		    "C:%u S:%u D:%d SR:%u E:%u Prim: %s, Secn: %s, "
		    "Interval: 0x%04x (%u us), SID: 0x%x",
		    scan_response_label,
		    le_addr, info->adv_type, info->rssi, name,
		    (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
		    (info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
		    (info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
		    (info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
		    (info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0,
		    phy2str(info->primary_phy), phy2str(info->secondary_phy),
		    info->interval, BT_CONN_INTERVAL_TO_US(info->interval),
		    info->sid);

	if (scan_verbose_output) {
		shell_info(ctx_shell,
			   "%*s[SCAN DATA START - %s]",
			   strlen(scan_response_label), "",
			   scan_response_type_txt(info->adv_type));
		bt_data_parse(&buf_copy, data_verbose_cb, NULL);
		shell_info(ctx_shell, "%*s[SCAN DATA END]", strlen(scan_response_label), "");
	}

#if defined(CONFIG_BT_CENTRAL)
	if ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0U) {
		struct bt_conn *conn = bt_conn_lookup_addr_le(selected_id, info->addr);

		/* Only store auto-connect address for devices we are not already connected to */
		if (conn == NULL) {
			/* Store address for later use */
			auto_connect.addr_set = true;
			bt_addr_le_copy(&auto_connect.addr, info->addr);

			/* Use the above auto_connect.addr address to automatically connect */
			if (auto_connect.connect_name) {
				auto_connect.connect_name = false;

				cmd_scan_off(ctx_shell);

				/* "name" is what would be in argv[0] normally */
				cmd_scan_filter_clear_name(ctx_shell, 1, (char *[]){"name"});

				/* "connect" is what would be in argv[0] normally */
				cmd_connect_le(ctx_shell, 1, (char *[]){"connect"});
			}
		} else {
			bt_conn_unref(conn);
		}
	}
#endif /* CONFIG_BT_CENTRAL */
}

static void scan_timeout(void)
{
	shell_print(ctx_shell, "Scan timeout");

#if defined(CONFIG_BT_CENTRAL)
	if (auto_connect.connect_name) {
		auto_connect.connect_name = false;
		/* "name" is what would be in argv[0] normally */
		cmd_scan_filter_clear_name(ctx_shell, 1, (char *[]){ "name" });
	}
#endif /* CONFIG_BT_CENTRAL */
}
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_EXT_ADV)
#if defined(CONFIG_BT_BROADCASTER)
static void adv_sent(struct bt_le_ext_adv *adv,
		     struct bt_le_ext_adv_sent_info *info)
{
	shell_print(ctx_shell, "Advertiser[%d] %p sent %d",
		    bt_le_ext_adv_get_index(adv), adv, info->num_sent);
}

static void adv_scanned(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_scanned_info *info)
{
	char str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, str, sizeof(str));

	shell_print(ctx_shell, "Advertiser[%d] %p scanned by %s",
		    bt_le_ext_adv_get_index(adv), adv, str);
}
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_PERIPHERAL)
static void adv_connected(struct bt_le_ext_adv *adv,
			  struct bt_le_ext_adv_connected_info *info)
{
	char str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(info->conn), str, sizeof(str));

	shell_print(ctx_shell, "Advertiser[%d] %p connected by %s",
		    bt_le_ext_adv_get_index(adv), adv, str);
}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_PRIVACY)
static bool adv_rpa_expired(struct bt_le_ext_adv *adv)
{
	uint8_t adv_index = bt_le_ext_adv_get_index(adv);

	bool keep_rpa = atomic_test_bit(adv_set_opt[adv_index],
					  SHELL_ADV_OPT_KEEP_RPA);
	shell_print(ctx_shell, "Advertiser[%d] %p RPA %s",
		    adv_index, adv,
		    keep_rpa ? "not expired" : "expired");

#if defined(CONFIG_BT_EAD)
	/* EAD must be updated each time the RPA is updated */
	if (!keep_rpa) {
		ead_update_ad();
	}
#endif

	return keep_rpa;
}
#endif /* defined(CONFIG_BT_PRIVACY) */

#endif /* CONFIG_BT_EXT_ADV */

#if !defined(CONFIG_BT_CONN)
#if 0 /* FIXME: Add support for changing prompt */
static const char *current_prompt(void)
{
	return NULL;
}
#endif
#endif /* !CONFIG_BT_CONN */

#if defined(CONFIG_BT_CONN)
#if 0 /* FIXME: Add support for changing prompt */
static const char *current_prompt(void)
{
	static char str[BT_ADDR_LE_STR_LEN + 2];
	static struct bt_conn_info info;

	if (!default_conn) {
		return NULL;
	}

	if (bt_conn_get_info(default_conn, &info) < 0) {
		return NULL;
	}

	if (info.type != BT_CONN_TYPE_LE) {
		return NULL;
	}

	bt_addr_le_to_str(info.le.dst, str, sizeof(str) - 2);
	strcat(str, "> ");
	return str;
}
#endif

void conn_addr_str(struct bt_conn *conn, char *addr, size_t len)
{
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info) < 0) {
		addr[0] = '\0';
		return;
	}

	switch (info.type) {
#if defined(CONFIG_BT_CLASSIC)
	case BT_CONN_TYPE_BR:
		bt_addr_to_str(info.br.dst, addr, len);
		break;
#endif
	case BT_CONN_TYPE_LE:
		bt_addr_le_to_str(info.le.dst, addr, len);
		break;
	default:
		break;
	}
}

static void print_le_oob(const struct shell *sh, struct bt_le_oob *oob)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char c[KEY_STR_LEN];
	char r[KEY_STR_LEN];

	bt_addr_le_to_str(&oob->addr, addr, sizeof(addr));

	bin2hex(oob->le_sc_data.c, sizeof(oob->le_sc_data.c), c, sizeof(c));
	bin2hex(oob->le_sc_data.r, sizeof(oob->le_sc_data.r), r, sizeof(r));

	shell_print(sh, "OOB data:");
	shell_print(sh, "%-29s %-32s %-32s", "addr", "random", "confirm");
	shell_print(sh, "%29s %32s %32s", addr, r, c);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_info info;
	int info_err;

	conn_addr_str(conn, addr, sizeof(addr));

	if (err) {
		shell_error(ctx_shell, "Failed to connect to %s (0x%02x)", addr,
			     err);
		goto done;
	}

	shell_print(ctx_shell, "Connected: %s", addr);

	info_err = bt_conn_get_info(conn, &info);
	if (info_err != 0) {
		shell_error(ctx_shell, "Failed to connection information: %d", info_err);
		goto done;
	}

	if (info.role == BT_CONN_ROLE_CENTRAL) {
		if (default_conn != NULL) {
			bt_conn_unref(default_conn);
		}

		default_conn = bt_conn_ref(conn);
	} else if (info.role == BT_CONN_ROLE_PERIPHERAL) {
		if (default_conn == NULL) {
			default_conn = bt_conn_ref(conn);
		}
	}

done:
	/* clear connection reference for sec mode 3 pairing */
	if (pairing_conn) {
		bt_conn_unref(pairing_conn);
		pairing_conn = NULL;
	}
}

static void disconnected_set_new_default_conn_cb(struct bt_conn *conn, void *user_data)
{
	struct bt_conn_info info;

	if (default_conn != NULL) {
		/* nop */
		return;
	}

	if (bt_conn_get_info(conn, &info) != 0) {
		shell_error(ctx_shell, "Unable to get info: conn %p", conn);
		return;
	}

	if (info.state == BT_CONN_STATE_CONNECTED) {
		char addr_str[BT_ADDR_LE_STR_LEN];

		default_conn = bt_conn_ref(conn);

		bt_addr_le_to_str(info.le.dst, addr_str, sizeof(addr_str));
		shell_print(ctx_shell, "Selected conn is now: %s", addr_str);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));
	shell_print(ctx_shell, "Disconnected: %s (reason 0x%02x)", addr, reason);

	if (default_conn == conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;

		/* If we are connected to other devices, set one of them as default */
		bt_conn_foreach(BT_CONN_TYPE_LE, disconnected_set_new_default_conn_cb, NULL);
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	shell_print(ctx_shell, "LE conn  param req: int (0x%04x, 0x%04x) lat %d"
		    " to %d", param->interval_min, param->interval_max,
		    param->latency, param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	shell_print(ctx_shell, "LE conn param updated: int 0x%04x lat %d "
		     "to %d", interval, latency, timeout);
}

#if defined(CONFIG_BT_SMP)
static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	shell_print(ctx_shell, "Identity resolved %s -> %s", addr_rpa,
	      addr_identity);
}
#endif

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
static const char *security_err_str(enum bt_security_err err)
{
	switch (err) {
	case BT_SECURITY_ERR_SUCCESS:
		return "Success";
	case BT_SECURITY_ERR_AUTH_FAIL:
		return "Authentication failure";
	case BT_SECURITY_ERR_PIN_OR_KEY_MISSING:
		return "PIN or key missing";
	case BT_SECURITY_ERR_OOB_NOT_AVAILABLE:
		return "OOB not available";
	case BT_SECURITY_ERR_AUTH_REQUIREMENT:
		return "Authentication requirements";
	case BT_SECURITY_ERR_PAIR_NOT_SUPPORTED:
		return "Pairing not supported";
	case BT_SECURITY_ERR_PAIR_NOT_ALLOWED:
		return "Pairing not allowed";
	case BT_SECURITY_ERR_INVALID_PARAM:
		return "Invalid parameters";
	case BT_SECURITY_ERR_UNSPECIFIED:
		return "Unspecified";
	default:
		return "Unknown";
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));

	if (!err) {
		shell_print(ctx_shell, "Security changed: %s level %u", addr,
			    level);
	} else {
		shell_print(ctx_shell, "Security failed: %s level %u "
			    "reason: %s (%d)",
			    addr, level, security_err_str(err), err);
	}
}
#endif

#if defined(CONFIG_BT_REMOTE_INFO)
static const char *ver_str(uint8_t ver)
{
	const char * const str[] = {
		"1.0b", "1.1", "1.2", "2.0", "2.1", "3.0", "4.0", "4.1", "4.2",
		"5.0", "5.1", "5.2", "5.3", "5.4"
	};

	if (ver < ARRAY_SIZE(str)) {
		return str[ver];
	}

	return "unknown";
}

static void remote_info_available(struct bt_conn *conn,
				  struct bt_conn_remote_info *remote_info)
{
	struct bt_conn_info info;

	bt_conn_get_info(conn, &info);

	if (IS_ENABLED(CONFIG_BT_REMOTE_VERSION)) {
		shell_print(ctx_shell,
			    "Remote LMP version %s (0x%02x) subversion 0x%04x "
			    "manufacturer 0x%04x", ver_str(remote_info->version),
			    remote_info->version, remote_info->subversion,
			    remote_info->manufacturer);
	}

	if (info.type == BT_CONN_TYPE_LE) {
		uint8_t features[8];
		char features_str[2 * sizeof(features) +  1];

		sys_memcpy_swap(features, remote_info->le.features,
				sizeof(features));
		bin2hex(features, sizeof(features),
			features_str, sizeof(features_str));
		shell_print(ctx_shell, "LE Features: 0x%s ", features_str);
	}
}
#endif /* defined(CONFIG_BT_REMOTE_INFO) */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
void le_data_len_updated(struct bt_conn *conn,
			 struct bt_conn_le_data_len_info *info)
{
	shell_print(ctx_shell,
		    "LE data len updated: TX (len: %d time: %d)"
		    " RX (len: %d time: %d)", info->tx_max_len,
		    info->tx_max_time, info->rx_max_len, info->rx_max_time);
}
#endif

#if defined(CONFIG_BT_USER_PHY_UPDATE)
void le_phy_updated(struct bt_conn *conn,
		    struct bt_conn_le_phy_info *info)
{
	shell_print(ctx_shell, "LE PHY updated: TX PHY %s, RX PHY %s",
		    phy2str(info->tx_phy), phy2str(info->rx_phy));
}
#endif

#if defined(CONFIG_BT_TRANSMIT_POWER_CONTROL)
void tx_power_report(struct bt_conn *conn,
		    const struct bt_conn_le_tx_power_report *report)
{
	shell_print(ctx_shell, "Tx Power Report: Reason: %s, PHY: %s, Tx Power Level: %d",
		    tx_power_report_reason2str(report->reason), tx_pwr_ctrl_phy2str(report->phy),
		    report->tx_power_level);
	shell_print(ctx_shell, "Tx Power Level Flag Info: %s, Delta: %d",
		    tx_power_flag2str(report->tx_power_level_flag), report->delta);
}
#endif


static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
#if defined(CONFIG_BT_SMP)
	.identity_resolved = identity_resolved,
#endif
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
	.security_changed = security_changed,
#endif
#if defined(CONFIG_BT_REMOTE_INFO)
	.remote_info_available = remote_info_available,
#endif
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	.le_data_len_updated = le_data_len_updated,
#endif
#if defined(CONFIG_BT_USER_PHY_UPDATE)
	.le_phy_updated = le_phy_updated,
#endif
#if defined(CONFIG_BT_TRANSMIT_POWER_CONTROL)
	.tx_power_report = tx_power_report,
#endif
};
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_OBSERVER)
static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
	.timeout = scan_timeout,
};
#endif /* defined(CONFIG_BT_OBSERVER) */

#if defined(CONFIG_BT_EXT_ADV)
#if defined(CONFIG_BT_BROADCASTER)
static struct bt_le_ext_adv_cb adv_callbacks = {
	.sent = adv_sent,
	.scanned = adv_scanned,
#if defined(CONFIG_BT_PERIPHERAL)
	.connected = adv_connected,
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_PRIVACY)
	.rpa_expired = adv_rpa_expired,
#endif /* defined(CONFIG_BT_PRIVACY) */

};
#endif /* CONFIG_BT_BROADCASTER */
#endif /* CONFIG_BT_EXT_ADV */


#if defined(CONFIG_BT_PER_ADV_SYNC)
struct bt_le_per_adv_sync *per_adv_syncs[CONFIG_BT_PER_ADV_SYNC_MAX];
size_t selected_per_adv_sync;

static void per_adv_sync_sync_cb(struct bt_le_per_adv_sync *sync,
				 struct bt_le_per_adv_sync_synced_info *info)
{
	const bool is_past_peer = info->conn != NULL;
	char le_addr[BT_ADDR_LE_STR_LEN];
	char past_peer[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	if (is_past_peer) {
		conn_addr_str(info->conn, past_peer, sizeof(past_peer));
	}

	shell_print(ctx_shell, "PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
		    "Interval 0x%04x (%u us), PHY %s, SD 0x%04X, PAST peer %s",
		    bt_le_per_adv_sync_get_index(sync), le_addr,
		    info->interval, BT_CONN_INTERVAL_TO_US(info->interval),
		    phy2str(info->phy), info->service_data,
		    is_past_peer ? past_peer : "not present");

	if (info->conn) { /* if from PAST */
		for (int i = 0; i < ARRAY_SIZE(per_adv_syncs); i++) {
			if (!per_adv_syncs[i]) {
				per_adv_syncs[i] = sync;
				break;
			}
		}
	}
}

static void per_adv_sync_terminated_cb(
	struct bt_le_per_adv_sync *sync,
	const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	for (int i = 0; i < ARRAY_SIZE(per_adv_syncs); i++) {
		if (per_adv_syncs[i] == sync) {
			per_adv_syncs[i] = NULL;
			break;
		}
	}

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	shell_print(ctx_shell, "PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated",
		    bt_le_per_adv_sync_get_index(sync), le_addr);
}

static void per_adv_sync_recv_cb(
	struct bt_le_per_adv_sync *sync,
	const struct bt_le_per_adv_sync_recv_info *info,
	struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	shell_print(ctx_shell, "PER_ADV_SYNC[%u]: [DEVICE]: %s, tx_power %i, "
		    "RSSI %i, CTE %u, data length %u",
		    bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power,
		    info->rssi, info->cte_type, buf->len);
}

static void per_adv_sync_biginfo_cb(struct bt_le_per_adv_sync *sync,
				    const struct bt_iso_biginfo *biginfo)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(biginfo->addr, le_addr, sizeof(le_addr));
	shell_print(ctx_shell, "BIG_INFO PER_ADV_SYNC[%u]: [DEVICE]: %s, sid 0x%02x, num_bis %u, "
		    "nse 0x%02x, interval 0x%04x (%u us), bn 0x%02x, pto 0x%02x, irc 0x%02x, "
		    "max_pdu 0x%04x, sdu_interval 0x%04x, max_sdu 0x%04x, phy %s, framing 0x%02x, "
		    "%sencrypted",
		    bt_le_per_adv_sync_get_index(sync), le_addr, biginfo->sid, biginfo->num_bis,
		    biginfo->sub_evt_count, biginfo->iso_interval,
		    BT_CONN_INTERVAL_TO_US(biginfo->iso_interval), biginfo->burst_number,
		    biginfo->offset, biginfo->rep_count, biginfo->max_pdu, biginfo->sdu_interval,
		    biginfo->max_sdu, phy2str(biginfo->phy), biginfo->framing,
		    biginfo->encryption ? "" : "not ");
}

static struct bt_le_per_adv_sync_cb per_adv_sync_cb = {
	.synced = per_adv_sync_sync_cb,
	.term = per_adv_sync_terminated_cb,
	.recv = per_adv_sync_recv_cb,
	.biginfo = per_adv_sync_biginfo_cb,
};
#endif /* CONFIG_BT_PER_ADV_SYNC */

static void bt_ready(int err)
{
	if (err) {
		shell_error(ctx_shell, "Bluetooth init failed (err %d)", err);
		return;
	}

	shell_print(ctx_shell, "Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS) && !no_settings_load) {
		settings_load();
		shell_print(ctx_shell, "Settings Loaded");
	}

	if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		bt_le_oob_set_legacy_flag(true);
	}

#if defined(CONFIG_BT_OBSERVER)
	bt_le_scan_cb_register(&scan_callbacks);
#endif

#if defined(CONFIG_BT_CONN)
	default_conn = NULL;

	/* Unregister to avoid register repeatedly */
	bt_conn_cb_unregister(&conn_callbacks);
	bt_conn_cb_register(&conn_callbacks);
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_PER_ADV_SYNC)
	bt_le_per_adv_sync_cb_register(&per_adv_sync_cb);
#endif /* CONFIG_BT_PER_ADV_SYNC */

#if defined(CONFIG_BT_SMP)
	bt_conn_auth_info_cb_register(&auth_info_cb);
#endif /* CONFIG_BT_SMP */
}

static int cmd_init(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	bool sync = false;

	ctx_shell = sh;

	for (size_t argn = 1; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "no-settings-load")) {
			no_settings_load = true;
		} else if (!strcmp(arg, "sync")) {
			sync = true;
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	if (sync) {
		err = bt_enable(NULL);
		bt_ready(err);
	} else {
		err = bt_enable(bt_ready);
		if (err) {
			shell_error(sh, "Bluetooth init failed (err %d)",
				    err);
		}
	}

	return err;
}

static int cmd_disable(const struct shell *sh, size_t argc, char *argv[])
{
	return bt_disable();
}

#ifdef CONFIG_SETTINGS
static int cmd_settings_load(const struct shell *sh, size_t argc,
			     char *argv[])
{
	int err;

	err = settings_load();
	if (err) {
		shell_error(sh, "Settings load failed (err %d)", err);
		return err;
	}

	shell_print(sh, "Settings loaded");
	return 0;
}
#endif

#if defined(CONFIG_BT_HCI)
static int cmd_hci_cmd(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t ogf;
	uint16_t ocf;
	struct net_buf *buf = NULL, *rsp;
	int err;
	static uint8_t hex_data[HCI_CMD_MAX_PARAM];
	int hex_data_len;

	hex_data_len = 0;
	ogf = strtoul(argv[1], NULL, 16);
	ocf = strtoul(argv[2], NULL, 16);

	if (argc > 3) {
		size_t len;

		if (strlen(argv[3]) > 2 * HCI_CMD_MAX_PARAM) {
			shell_error(sh, "Data field too large\n");
			return -ENOEXEC;
		}

		len = hex2bin(argv[3], strlen(argv[3]), &hex_data[hex_data_len],
			      sizeof(hex_data) - hex_data_len);
		if (!len) {
			shell_error(sh, "HCI command illegal data field\n");
			return -ENOEXEC;
		}

		buf = bt_hci_cmd_create(BT_OP(ogf, ocf), len);
		net_buf_add_mem(buf, hex_data, len);
	}

	err = bt_hci_cmd_send_sync(BT_OP(ogf, ocf), buf, &rsp);
	if (err) {
		shell_error(sh, "HCI command failed (err %d)", err);
		return err;
	} else {
		shell_hexdump(sh, rsp->data, rsp->len);
		net_buf_unref(rsp);
	}

	return 0;
}
#endif /* CONFIG_BT_HCI */

static int cmd_name(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (argc < 2) {
		shell_print(sh, "Bluetooth Local Name: %s", bt_get_name());
		return 0;
	}

	err = bt_set_name(argv[1]);
	if (err) {
		shell_error(sh, "Unable to set name %s (err %d)", argv[1],
			    err);
		return err;
	}

	return 0;
}

static int cmd_appearance(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc == 1) {
		shell_print(sh, "Bluetooth Appearance: 0x%04x", bt_get_appearance());
		return 0;
	}

#if defined(CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC)
	uint16_t app;
	int err = 0;
	const char *val;

	val = argv[1];

	if (strlen(val) != 6 || strncmp(val, "0x", 2)) {
		shell_error(sh, "Argument must be 0x followed by exactly 4 hex digits.");
		return -EINVAL;
	}

	app = shell_strtoul(val, 16, &err);
	if (err) {
		shell_error(sh, "Argument must be 0x followed by exactly 4 hex digits.");
		return -EINVAL;
	}

	err = bt_set_appearance(app);
	if (err) {
		shell_error(sh, "bt_set_appearance(0x%04x) failed with err %d", app, err);
		return err;
	}
#endif /* defined(CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC) */

	return 0;
}

static int cmd_id_create(const struct shell *sh, size_t argc, char *argv[])
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr;
	int err;

	if (argc > 1) {
		err = bt_addr_le_from_str(argv[1], "random", &addr);
		if (err) {
			shell_error(sh, "Invalid address");
		}
	} else {
		bt_addr_le_copy(&addr, BT_ADDR_LE_ANY);
	}

	err = bt_id_create(&addr, NULL);
	if (err < 0) {
		shell_error(sh, "Creating new ID failed (err %d)", err);
		return err;
	}

	bt_addr_le_to_str(&addr, addr_str, sizeof(addr_str));
	shell_print(sh, "New identity (%d) created: %s", err, addr_str);

	return 0;
}

static int cmd_id_reset(const struct shell *sh, size_t argc, char *argv[])
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr;
	uint8_t id;
	int err;

	if (argc < 2) {
		shell_error(sh, "Identity identifier not specified");
		return -ENOEXEC;
	}

	id = strtol(argv[1], NULL, 10);

	if (argc > 2) {
		err = bt_addr_le_from_str(argv[2], "random", &addr);
		if (err) {
			shell_print(sh, "Invalid address");
			return err;
		}
	} else {
		bt_addr_le_copy(&addr, BT_ADDR_LE_ANY);
	}

	err = bt_id_reset(id, &addr, NULL);
	if (err < 0) {
		shell_print(sh, "Resetting ID %u failed (err %d)", id, err);
		return err;
	}

	bt_addr_le_to_str(&addr, addr_str, sizeof(addr_str));
	shell_print(sh, "Identity %u reset: %s", id, addr_str);

	return 0;
}

static int cmd_id_delete(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t id;
	int err;

	if (argc < 2) {
		shell_error(sh, "Identity identifier not specified");
		return -ENOEXEC;
	}

	id = strtol(argv[1], NULL, 10);

	err = bt_id_delete(id);
	if (err < 0) {
		shell_error(sh, "Deleting ID %u failed (err %d)", id, err);
		return err;
	}

	shell_print(sh, "Identity %u deleted", id);

	return 0;
}

static int cmd_id_show(const struct shell *sh, size_t argc, char *argv[])
{
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t i, count = CONFIG_BT_ID_MAX;

	bt_id_get(addrs, &count);

	for (i = 0; i < count; i++) {
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(&addrs[i], addr_str, sizeof(addr_str));
		shell_print(sh, "%s%zu: %s", i == selected_id ? "*" : " ", i,
		      addr_str);
	}

	return 0;
}

static int cmd_id_select(const struct shell *sh, size_t argc, char *argv[])
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t count = CONFIG_BT_ID_MAX;
	uint8_t id;

	id = strtol(argv[1], NULL, 10);

	bt_id_get(addrs, &count);
	if (count <= id) {
		shell_error(sh, "Invalid identity");
		return -ENOEXEC;
	}

	bt_addr_le_to_str(&addrs[id], addr_str, sizeof(addr_str));
	shell_print(sh, "Selected identity: %s", addr_str);
	selected_id = id;

	return 0;
}

#if defined(CONFIG_BT_OBSERVER)
static int cmd_active_scan_on(const struct shell *sh, uint32_t options,
			      uint16_t timeout)
{
	int err;
	struct bt_le_scan_param param = {
			.type       = BT_LE_SCAN_TYPE_ACTIVE,
			.options    = BT_LE_SCAN_OPT_NONE,
			.interval   = BT_GAP_SCAN_FAST_INTERVAL,
			.window     = BT_GAP_SCAN_FAST_WINDOW,
			.timeout    = timeout, };

	param.options |= options;

	err = bt_le_scan_start(&param, NULL);
	if (err) {
		shell_error(sh, "Bluetooth set active scan failed "
		      "(err %d)", err);
		return err;
	} else {
		shell_print(sh, "Bluetooth active scan enabled");
	}

	return 0;
}

static int cmd_passive_scan_on(const struct shell *sh, uint32_t options,
			       uint16_t timeout)
{
	struct bt_le_scan_param param = {
			.type       = BT_LE_SCAN_TYPE_PASSIVE,
			.options    = BT_LE_SCAN_OPT_NONE,
			.interval   = 0x10,
			.window     = 0x10,
			.timeout    = timeout, };
	int err;

	param.options |= options;

	err = bt_le_scan_start(&param, NULL);
	if (err) {
		shell_error(sh, "Bluetooth set passive scan failed "
			    "(err %d)", err);
		return err;
	} else {
		shell_print(sh, "Bluetooth passive scan enabled");
	}

	return 0;
}

static int cmd_scan_off(const struct shell *sh)
{
	int err;

	err = bt_le_scan_stop();
	if (err) {
		shell_error(sh, "Stopping scanning failed (err %d)", err);
		return err;
	} else {
		shell_print(sh, "Scan successfully stopped");
	}

	return 0;
}

static int cmd_scan(const struct shell *sh, size_t argc, char *argv[])
{
	const char *action;
	uint32_t options = 0;
	uint16_t timeout = 0;

	/* Parse duplicate filtering data */
	for (size_t argn = 2; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "dups")) {
			options |= BT_LE_SCAN_OPT_FILTER_DUPLICATE;
		} else if (!strcmp(arg, "nodups")) {
			options &= ~BT_LE_SCAN_OPT_FILTER_DUPLICATE;
		} else if (!strcmp(arg, "fal")) {
			options |= BT_LE_SCAN_OPT_FILTER_ACCEPT_LIST;
		} else if (!strcmp(arg, "coded")) {
			options |= BT_LE_SCAN_OPT_CODED;
		} else if (!strcmp(arg, "no-1m")) {
			options |= BT_LE_SCAN_OPT_NO_1M;
		} else if (!strcmp(arg, "timeout")) {
			if (++argn == argc) {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			timeout = strtoul(argv[argn], NULL, 16);
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	action = argv[1];
	if (!strcmp(action, "on")) {
		return cmd_active_scan_on(sh, options, timeout);
	} else if (!strcmp(action, "off")) {
		return cmd_scan_off(sh);
	} else if (!strcmp(action, "passive")) {
		return cmd_passive_scan_on(sh, options, timeout);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	return 0;
}

static int cmd_scan_verbose_output(const struct shell *sh, size_t argc, char *argv[])
{
	const char *verbose_state;

	verbose_state = argv[1];
	if (!strcmp(verbose_state, "on")) {
		scan_verbose_output = true;
	} else if (!strcmp(verbose_state, "off")) {
		scan_verbose_output = false;
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	return 0;
}

static int cmd_scan_filter_set_name(const struct shell *sh, size_t argc,
				    char *argv[])
{
	const char *name_arg = argv[1];

	if (strlen(name_arg) >= sizeof(scan_filter.name)) {
		shell_error(ctx_shell, "Name is too long (max %zu): %s\n",
			    sizeof(scan_filter.name), name_arg);
		return -ENOEXEC;
	}

	strcpy(scan_filter.name, name_arg);
	scan_filter.name_set = true;

	return 0;
}

static int cmd_scan_filter_set_addr(const struct shell *sh, size_t argc,
				    char *argv[])
{
	const size_t max_cpy_len = sizeof(scan_filter.addr) - 1;
	const char *addr_arg = argv[1];

	/* Validate length including null terminator. */
	if (strlen(addr_arg) > max_cpy_len) {
		shell_error(ctx_shell, "Invalid address string: %s\n",
			    addr_arg);
		return -ENOEXEC;
	}

	/* Validate input to check if valid (subset of) BT address */
	for (size_t i = 0; i < strlen(addr_arg); i++) {
		const char c = addr_arg[i];
		uint8_t tmp;

		if (c != ':' && char2hex(c, &tmp) < 0) {
			shell_error(ctx_shell,
					"Invalid address string: %s\n",
					addr_arg);
			return -ENOEXEC;
		}
	}

	strncpy(scan_filter.addr, addr_arg, max_cpy_len);
	scan_filter.addr[max_cpy_len] = '\0'; /* ensure NULL termination */
	scan_filter.addr_set = true;

	return 0;
}

static int cmd_scan_filter_set_rssi(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	long rssi;

	rssi = shell_strtol(argv[1], 10, &err);

	if (!err) {
		if (IN_RANGE(rssi, INT8_MIN, INT8_MAX)) {
			scan_filter.rssi = (int8_t)rssi;
			scan_filter.rssi_set = true;
			shell_print(sh, "RSSI cutoff set at %d dB", scan_filter.rssi);

			return 0;
		}

		shell_print(sh, "value out of bounds (%d to %d)", INT8_MIN, INT8_MAX);
		err = -ERANGE;
	}

	shell_print(sh, "error %d", err);
	shell_help(sh);

	return SHELL_CMD_HELP_PRINTED;
}

static int cmd_scan_filter_set_pa_interval(const struct shell *sh, size_t argc,
					   char *argv[])
{
	unsigned long pa_interval;
	int err = 0;

	pa_interval = shell_strtoul(argv[1], 10, &err);

	if (!err) {
		if (IN_RANGE(pa_interval,
			     BT_GAP_PER_ADV_MIN_INTERVAL,
			     BT_GAP_PER_ADV_MAX_INTERVAL)) {
			scan_filter.pa_interval = (uint16_t)pa_interval;
			scan_filter.pa_interval_set = true;
			shell_print(sh, "PA interval cutoff set at %u",
				    scan_filter.pa_interval);

			return 0;
		}

		shell_print(sh, "value out of bounds (%d to %d)",
			    BT_GAP_PER_ADV_MIN_INTERVAL,
			    BT_GAP_PER_ADV_MAX_INTERVAL);

		err = -ERANGE;
	}

	shell_print(sh, "error %d", err);
	shell_help(sh);

	return SHELL_CMD_HELP_PRINTED;
}

static int cmd_scan_filter_clear_all(const struct shell *sh, size_t argc,
				     char *argv[])
{
	(void)memset(&scan_filter, 0, sizeof(scan_filter));

	return 0;
}

static int cmd_scan_filter_clear_name(const struct shell *sh, size_t argc,
				      char *argv[])
{
	(void)memset(scan_filter.name, 0, sizeof(scan_filter.name));
	scan_filter.name_set = false;

	return 0;
}

static int cmd_scan_filter_clear_addr(const struct shell *sh, size_t argc,
				      char *argv[])
{
	(void)memset(scan_filter.addr, 0, sizeof(scan_filter.addr));
	scan_filter.addr_set = false;

	return 0;
}

#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_BROADCASTER)
static ssize_t ad_init(struct bt_data *data_array, const size_t data_array_size,
		       const atomic_t *adv_options)
{
	const bool discoverable = atomic_test_bit(adv_options, SHELL_ADV_OPT_DISCOVERABLE);
	const bool appearance = atomic_test_bit(adv_options, SHELL_ADV_OPT_APPEARANCE);
	const bool adv_ext = atomic_test_bit(adv_options, SHELL_ADV_OPT_EXT_ADV);
	static uint8_t ad_flags;
	size_t ad_len = 0;

	/* Set BR/EDR Not Supported if LE-only device */
	ad_flags = IS_ENABLED(CONFIG_BT_CLASSIC) ? 0 : BT_LE_AD_NO_BREDR;

	if (discoverable) {
		/* A privacy-enabled Set Member should advertise RSI values only when in
		 * the GAP Limited Discoverable mode.
		 */
		if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
		    IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER) &&
		    svc_inst != NULL) {
			ad_flags |= BT_LE_AD_LIMITED;
		} else {
			ad_flags |= BT_LE_AD_GENERAL;
		}
	}

	if (ad_flags != 0) {
		__ASSERT(data_array_size > ad_len, "No space for AD_FLAGS");
		data_array[ad_len].type = BT_DATA_FLAGS;
		data_array[ad_len].data_len = sizeof(ad_flags);
		data_array[ad_len].data = &ad_flags;
		ad_len++;
	}

	if (appearance) {
		const uint16_t appearance2 = bt_get_appearance();
		static uint8_t appearance_data[sizeof(appearance2)];

		__ASSERT(data_array_size > ad_len, "No space for appearance");
		sys_put_le16(appearance2, appearance_data);
		data_array[ad_len].type = BT_DATA_GAP_APPEARANCE;
		data_array[ad_len].data_len = sizeof(appearance_data);
		data_array[ad_len].data = appearance_data;
		ad_len++;
	}

	if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER)) {
		ssize_t csis_ad_len;

		csis_ad_len = csis_ad_data_add(&data_array[ad_len],
					       data_array_size - ad_len, discoverable);
		if (csis_ad_len < 0) {
			shell_error(ctx_shell, "Failed to add CSIS data (err %d)", csis_ad_len);
			return ad_len;
		}

		ad_len += csis_ad_len;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO) && IS_ENABLED(CONFIG_BT_EXT_ADV) && adv_ext) {
		const bool connectable = atomic_test_bit(adv_options, SHELL_ADV_OPT_CONNECTABLE);
		ssize_t audio_ad_len;

		audio_ad_len = audio_ad_data_add(&data_array[ad_len], data_array_size - ad_len,
						 discoverable, connectable);
		if (audio_ad_len < 0) {
			return audio_ad_len;
		}

		ad_len += audio_ad_len;
	}

	return ad_len;
}

void set_ad_name_complete(struct bt_data *ad, const char *name)
{
	ad->type = BT_DATA_NAME_COMPLETE;
	ad->data_len = strlen(name);
	ad->data = name;
}

void set_ad_device_name_complete(struct bt_data *ad)
{
	const char *name = bt_get_name();

	set_ad_name_complete(ad, name);
}

static int cmd_advertise(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_le_adv_param param = {};
	struct bt_data ad[4];
	struct bt_data sd[4];
	bool discoverable = true;
	bool appearance = false;
	ssize_t ad_len = 0;
	ssize_t sd_len = 0;
	int err;
	bool with_name = true;
	bool name_ad = false;
	bool name_sd = true;

	if (!strcmp(argv[1], "off")) {
		if (bt_le_adv_stop() < 0) {
			shell_error(sh, "Failed to stop advertising");
			return -ENOEXEC;
		} else {
			shell_print(sh, "Advertising stopped");
		}

		return 0;
	}

	param.id = selected_id;
	param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;

	if (!strcmp(argv[1], "on")) {
		param.options = BT_LE_ADV_OPT_CONNECTABLE;
	} else if (!strcmp(argv[1], "nconn")) {
		param.options = 0U;
	} else {
		goto fail;
	}

	for (size_t argn = 2; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "discov")) {
			discoverable = true;
		} else if (!strcmp(arg, "non_discov")) {
			discoverable = false;
		} else if (!strcmp(arg, "appearance")) {
			appearance = true;
		} else if (!strcmp(arg, "fal")) {
			param.options |= BT_LE_ADV_OPT_FILTER_SCAN_REQ;
			param.options |= BT_LE_ADV_OPT_FILTER_CONN;
		} else if (!strcmp(arg, "fal-scan")) {
			param.options |= BT_LE_ADV_OPT_FILTER_SCAN_REQ;
		} else if (!strcmp(arg, "fal-conn")) {
			param.options |= BT_LE_ADV_OPT_FILTER_CONN;
		} else if (!strcmp(arg, "identity")) {
			param.options |= BT_LE_ADV_OPT_USE_IDENTITY;
		} else if (!strcmp(arg, "no-name")) {
			with_name = false;
		} else if (!strcmp(arg, "name-ad")) {
			name_ad = true;
			name_sd = false;
		} else if (!strcmp(arg, "one-time")) {
			param.options |= BT_LE_ADV_OPT_ONE_TIME;
		} else if (!strcmp(arg, "disable-37")) {
			param.options |= BT_LE_ADV_OPT_DISABLE_CHAN_37;
		} else if (!strcmp(arg, "disable-38")) {
			param.options |= BT_LE_ADV_OPT_DISABLE_CHAN_38;
		} else if (!strcmp(arg, "disable-39")) {
			param.options |= BT_LE_ADV_OPT_DISABLE_CHAN_39;
		} else {
			goto fail;
		}
	}

	if (name_ad && with_name) {
		set_ad_device_name_complete(&ad[0]);
		ad_len++;
	}

	if (name_sd && with_name) {
		set_ad_device_name_complete(&sd[0]);
		sd_len++;
	}

	atomic_clear(adv_opt);
	atomic_set_bit_to(adv_opt, SHELL_ADV_OPT_CONNECTABLE,
			  (param.options & BT_LE_ADV_OPT_CONNECTABLE) > 0);
	atomic_set_bit_to(adv_opt, SHELL_ADV_OPT_DISCOVERABLE, discoverable);
	atomic_set_bit_to(adv_opt, SHELL_ADV_OPT_APPEARANCE, appearance);

	err = ad_init(&ad[ad_len], ARRAY_SIZE(ad) - ad_len, adv_opt);
	if (ad_len < 0) {
		return -ENOEXEC;
	}
	ad_len += err;

	err = bt_le_adv_start(&param, ad_len > 0 ? ad : NULL, ad_len, sd_len > 0 ? sd : NULL,
			      sd_len);
	if (err < 0) {
		shell_error(sh, "Failed to start advertising (err %d)",
			    err);
		return err;
	} else {
		shell_print(sh, "Advertising started");
	}

	return 0;

fail:
	shell_help(sh);
	return -ENOEXEC;
}

#if defined(CONFIG_BT_PERIPHERAL)
static int cmd_directed_adv(const struct shell *sh,
			     size_t argc, char *argv[])
{
	int err;
	bt_addr_le_t addr;
	struct bt_le_adv_param param;

	err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	param = *BT_LE_ADV_CONN_DIR(&addr);
	if (err) {
		shell_error(sh, "Invalid peer address (err %d)", err);
		return err;
	}

	for (size_t argn = 3; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "low")) {
			param.options |= BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY;
			param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
			param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
		} else if (!strcmp(arg, "identity")) {
			param.options |= BT_LE_ADV_OPT_USE_IDENTITY;
		} else if (!strcmp(arg, "dir-rpa")) {
			param.options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;
		} else if (!strcmp(arg, "disable-37")) {
			param.options |= BT_LE_ADV_OPT_DISABLE_CHAN_37;
		} else if (!strcmp(arg, "disable-38")) {
			param.options |= BT_LE_ADV_OPT_DISABLE_CHAN_38;
		} else if (!strcmp(arg, "disable-39")) {
			param.options |= BT_LE_ADV_OPT_DISABLE_CHAN_39;
		} else {
			shell_help(sh);
			return -ENOEXEC;
		}
	}

	err = bt_le_adv_start(&param, NULL, 0, NULL, 0);
	if (err) {
		shell_error(sh, "Failed to start directed advertising (%d)",
			    err);
		return -ENOEXEC;
	} else {
		shell_print(sh, "Started directed advertising");
	}

	return 0;
}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_EXT_ADV)
static bool adv_param_parse(size_t argc, char *argv[],
			   struct bt_le_adv_param *param)
{
	memset(param, 0, sizeof(struct bt_le_adv_param));

	if (!strcmp(argv[1], "conn-scan")) {
		param->options |= BT_LE_ADV_OPT_CONNECTABLE;
		param->options |= BT_LE_ADV_OPT_SCANNABLE;
	} else if (!strcmp(argv[1], "conn-nscan")) {
		param->options |= BT_LE_ADV_OPT_CONNECTABLE;
	} else if (!strcmp(argv[1], "nconn-scan")) {
		param->options |= BT_LE_ADV_OPT_SCANNABLE;
	} else if (!strcmp(argv[1], "nconn-nscan")) {
		/* Acceptable option, nothing to do */
	} else {
		return false;
	}

	for (size_t argn = 2; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "ext-adv")) {
			param->options |= BT_LE_ADV_OPT_EXT_ADV;
		} else if (!strcmp(arg, "coded")) {
			param->options |= BT_LE_ADV_OPT_CODED;
		} else if (!strcmp(arg, "no-2m")) {
			param->options |= BT_LE_ADV_OPT_NO_2M;
		} else if (!strcmp(arg, "anon")) {
			param->options |= BT_LE_ADV_OPT_ANONYMOUS;
		} else if (!strcmp(arg, "tx-power")) {
			param->options |= BT_LE_ADV_OPT_USE_TX_POWER;
		} else if (!strcmp(arg, "scan-reports")) {
			param->options |= BT_LE_ADV_OPT_NOTIFY_SCAN_REQ;
		} else if (!strcmp(arg, "fal")) {
			param->options |= BT_LE_ADV_OPT_FILTER_SCAN_REQ;
			param->options |= BT_LE_ADV_OPT_FILTER_CONN;
		} else if (!strcmp(arg, "fal-scan")) {
			param->options |= BT_LE_ADV_OPT_FILTER_SCAN_REQ;
		} else if (!strcmp(arg, "fal-conn")) {
			param->options |= BT_LE_ADV_OPT_FILTER_CONN;
		} else if (!strcmp(arg, "identity")) {
			param->options |= BT_LE_ADV_OPT_USE_IDENTITY;
		} else if (!strcmp(arg, "low")) {
			param->options |= BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY;
		} else if (!strcmp(arg, "dir-rpa")) {
			param->options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;
		} else if (!strcmp(arg, "disable-37")) {
			param->options |= BT_LE_ADV_OPT_DISABLE_CHAN_37;
		} else if (!strcmp(arg, "disable-38")) {
			param->options |= BT_LE_ADV_OPT_DISABLE_CHAN_38;
		} else if (!strcmp(arg, "disable-39")) {
			param->options |= BT_LE_ADV_OPT_DISABLE_CHAN_39;
		} else if (!strcmp(arg, "directed")) {
			static bt_addr_le_t addr;

			if ((argn + 2) >= argc) {
				return false;
			}

			if (bt_addr_le_from_str(argv[argn + 1], argv[argn + 2],
						&addr)) {
				return false;
			}

			param->peer = &addr;
			argn += 2;
		} else {
			return false;
		}
	}

	param->id = selected_id;
	param->sid = 0;
	if (param->peer &&
	    !(param->options & BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY)) {
		param->interval_min = 0;
		param->interval_max = 0;
	} else {
		param->interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
		param->interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
	}

	return true;
}

static int cmd_adv_create(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_le_adv_param param;
	struct bt_le_ext_adv *adv;
	uint8_t adv_index;
	int err;

	if (!adv_param_parse(argc, argv, &param)) {
		shell_help(sh);
		return -ENOEXEC;
	}

	err = bt_le_ext_adv_create(&param, &adv_callbacks, &adv);
	if (err) {
		shell_error(sh, "Failed to create advertiser set (%d)", err);
		return -ENOEXEC;
	}

	adv_index = bt_le_ext_adv_get_index(adv);
	adv_sets[adv_index] = adv;

	atomic_clear(adv_set_opt[adv_index]);
	atomic_set_bit_to(adv_set_opt[adv_index], SHELL_ADV_OPT_CONNECTABLE,
			  (param.options & BT_LE_ADV_OPT_CONNECTABLE) > 0);
	atomic_set_bit_to(adv_set_opt[adv_index], SHELL_ADV_OPT_EXT_ADV,
			  (param.options & BT_LE_ADV_OPT_EXT_ADV) > 0);

	shell_print(sh, "Created adv id: %d, adv: %p", adv_index, adv);

	return 0;
}

static int cmd_adv_param(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	struct bt_le_adv_param param;
	int err;

	if (!adv_param_parse(argc, argv, &param)) {
		shell_help(sh);
		return -ENOEXEC;
	}

	err = bt_le_ext_adv_update_param(adv, &param);
	if (err) {
		shell_error(sh, "Failed to update advertiser set (%d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_adv_data(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	static uint8_t hex_data[1650];
	bool appearance = false;
	struct bt_data *data;
	struct bt_data ad[AD_SIZE];
	struct bt_data sd[AD_SIZE];
	size_t hex_data_len;
	size_t ad_len = 0;
	size_t sd_len = 0;
	ssize_t len = 0;
	bool discoverable = false;
	size_t *data_len;
	int err;
	bool name = false;
	bool dev_name = false;
	const char *name_value = NULL;

	if (!adv) {
		return -EINVAL;
	}

	hex_data_len = 0;
	data = ad;
	data_len = &ad_len;

	for (size_t argn = 1; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (name && !dev_name && name_value == NULL) {
			if (*data_len == ARRAY_SIZE(ad)) {
				/* Maximum entries limit reached. */
				shell_print(sh, "Failed to set advertising data: "
						"Maximum entries limit reached");

				return -ENOEXEC;
			}

			len = strlen(arg);
			memcpy(&hex_data[hex_data_len], arg, len);
			name_value = &hex_data[hex_data_len];

			set_ad_name_complete(&data[*data_len], name_value);

			(*data_len)++;
			hex_data_len += len;

			continue;
		}

		if (strcmp(arg, "scan-response") && *data_len == ARRAY_SIZE(ad)) {
			/* Maximum entries limit reached. */
			shell_print(sh, "Failed to set advertising data: "
					"Maximum entries limit reached");

			return -ENOEXEC;
		}

		if (!strcmp(arg, "discov")) {
			discoverable = true;
		} else if (!strcmp(arg, "non_discov")) {
			discoverable = false;
		} else if (!strcmp(arg, "appearance")) {
			appearance = true;
		} else if (!strcmp(arg, "scan-response")) {
			if (data == sd) {
				shell_print(sh, "Failed to set advertising data: "
						"duplicate scan-response option");
				return -ENOEXEC;
			}

			data = sd;
			data_len = &sd_len;
		} else if (!strcmp(arg, "name") && !name) {
			name = true;
		} else if (!strcmp(arg, "dev-name")) {
			name = true;
			dev_name = true;
		} else {
			len = hex2bin(arg, strlen(arg), &hex_data[hex_data_len],
				      sizeof(hex_data) - hex_data_len);

			if (!len || (len - 1) != (hex_data[hex_data_len])) {
				shell_print(sh, "Failed to set advertising data: "
						"malformed hex data");
				return -ENOEXEC;
			}

			data[*data_len].type = hex_data[hex_data_len + 1];
			data[*data_len].data_len = len - 2;
			data[*data_len].data = &hex_data[hex_data_len + 2];
			(*data_len)++;
			hex_data_len += len;
		}
	}

	if (name && !dev_name && name_value == NULL) {
		shell_error(sh, "Failed to set advertising data: Expected a value for 'name'");
		return -ENOEXEC;
	}

	if (name && dev_name && name_value == NULL) {
		if (*data_len == ARRAY_SIZE(ad)) {
			/* Maximum entries limit reached. */
			shell_print(sh, "Failed to set advertising data: "
					"Maximum entries limit reached");

			return -ENOEXEC;
		}

		set_ad_device_name_complete(&data[*data_len]);

		(*data_len)++;
	}

	atomic_set_bit_to(adv_set_opt[selected_adv], SHELL_ADV_OPT_DISCOVERABLE, discoverable);
	atomic_set_bit_to(adv_set_opt[selected_adv], SHELL_ADV_OPT_APPEARANCE,
			  appearance);

	len = ad_init(&data[*data_len], AD_SIZE - *data_len, adv_set_opt[selected_adv]);
	if (len < 0) {
		shell_error(sh, "Failed to initialize stack advertising data");

		return -ENOEXEC;
	}

	if (data == ad) {
		ad_len += len;
	} else {
		sd_len += len;
	}

	err = bt_le_ext_adv_set_data(adv, ad_len > 0 ? ad : NULL, ad_len,
					  sd_len > 0 ? sd : NULL, sd_len);
	if (err) {
		shell_print(sh, "Failed to set advertising set data (%d)",
			    err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_adv_start(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	struct bt_le_ext_adv_start_param param;
	uint8_t num_events = 0;
	int32_t timeout = 0;
	int err;

	if (!adv) {
		shell_print(sh, "Advertiser[%d] not created", selected_adv);
		return -EINVAL;
	}

	for (size_t argn = 1; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "timeout")) {
			if (++argn == argc) {
				goto fail_show_help;
			}

			timeout = strtoul(argv[argn], NULL, 16);
		}

		if (!strcmp(arg, "num-events")) {
			if (++argn == argc) {
				goto fail_show_help;
			}

			num_events = strtoul(argv[argn], NULL, 16);
		}
	}

	param.timeout = timeout;
	param.num_events = num_events;

	err = bt_le_ext_adv_start(adv, &param);
	if (err) {
		shell_print(sh, "Failed to start advertising set (%d)", err);
		return -ENOEXEC;
	}

	shell_print(sh, "Advertiser[%d] %p set started", selected_adv, adv);
	return 0;

fail_show_help:
	shell_help(sh);
	return -ENOEXEC;
}

static int cmd_adv_stop(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	int err;

	if (!adv) {
		shell_print(sh, "Advertiser[%d] not created", selected_adv);
		return -EINVAL;
	}

	err = bt_le_ext_adv_stop(adv);
	if (err) {
		shell_print(sh, "Failed to stop advertising set (%d)", err);
		return -ENOEXEC;
	}

	shell_print(sh, "Advertiser set stopped");
	return 0;
}

static int cmd_adv_delete(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	int err;

	if (!adv) {
		shell_print(sh, "Advertiser[%d] not created", selected_adv);
		return -EINVAL;
	}

	err = bt_le_ext_adv_delete(adv);
	if (err) {
		shell_error(ctx_shell, "Failed to delete advertiser set");
		return err;
	}

	adv_sets[selected_adv] = NULL;
	return 0;
}

static int cmd_adv_select(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc == 2) {
		uint8_t id = strtol(argv[1], NULL, 10);

		if (!(id < ARRAY_SIZE(adv_sets))) {
			return -EINVAL;
		}

		selected_adv = id;
		return 0;
	}

	for (int i = 0; i < ARRAY_SIZE(adv_sets); i++) {
		if (adv_sets[i]) {
			shell_print(sh, "Advertiser[%d] %p", i, adv_sets[i]);
		}
	}

	return -ENOEXEC;
}

static int cmd_adv_info(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	struct bt_le_ext_adv_info info;
	int err;

	if (!adv) {
		return -EINVAL;
	}

	err = bt_le_ext_adv_get_info(adv, &info);
	if (err) {
		shell_error(sh, "OOB data failed");
		return err;
	}

	shell_print(sh, "Advertiser[%d] %p", selected_adv, adv);
	shell_print(sh, "Id: %d, TX power: %d dBm", info.id, info.tx_power);
	print_le_addr("Address", info.addr);

	return 0;
}

#if defined(CONFIG_BT_PERIPHERAL)
static int cmd_adv_oob(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	int err;

	if (!adv) {
		return -EINVAL;
	}

	err = bt_le_ext_adv_oob_get_local(adv, &oob_local);
	if (err) {
		shell_error(sh, "OOB data failed");
		return err;
	}

	print_le_oob(sh, &oob_local);

	return 0;
}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_PRIVACY)
static int cmd_adv_rpa_expire(const struct shell *sh, size_t argc, char *argv[])
{
	if (!strcmp(argv[1], "on")) {
		atomic_clear_bit(adv_set_opt[selected_adv], SHELL_ADV_OPT_KEEP_RPA);
		shell_print(sh, "RPA will expire on next timeout");
	} else if (!strcmp(argv[1], "off")) {
		atomic_set_bit(adv_set_opt[selected_adv], SHELL_ADV_OPT_KEEP_RPA);
		shell_print(sh, "RPA will not expire on RPA timeout");
	} else {
		shell_error(sh, "Invalid argument: %s", argv[1]);
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_BT_PRIVACY */

#if defined(CONFIG_BT_PER_ADV)
static int cmd_per_adv(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];

	if (!adv) {
		shell_error(sh, "No extended advertisement set selected");
		return -EINVAL;
	}

	if (!strcmp(argv[1], "off")) {
		if (bt_le_per_adv_stop(adv) < 0) {
			shell_error(sh,
				    "Failed to stop periodic advertising");
		} else {
			shell_print(sh, "Periodic advertising stopped");
		}
	} else if (!strcmp(argv[1], "on")) {
		if (bt_le_per_adv_start(adv) < 0) {
			shell_error(sh,
				    "Failed to start periodic advertising");
		} else {
			shell_print(sh, "Periodic advertising started");
		}
	} else {
		shell_error(sh, "Invalid argument: %s", argv[1]);
		return -EINVAL;
	}

	return 0;
}

static int cmd_per_adv_param(const struct shell *sh, size_t argc,
			     char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	struct bt_le_per_adv_param param;
	int err;

	if (!adv) {
		shell_error(sh, "No extended advertisement set selected");
		return -EINVAL;
	}

	if (argc > 1) {
		param.interval_min = strtol(argv[1], NULL, 16);
	} else {
		param.interval_min = BT_GAP_ADV_SLOW_INT_MIN;
	}

	if (argc > 2) {
		param.interval_max = strtol(argv[2], NULL, 16);
	} else {
		param.interval_max = param.interval_min * 1.2;

	}

	if (param.interval_min > param.interval_max) {
		shell_error(sh,
			    "Min interval shall be less than max interval");
		return -EINVAL;
	}

	if (argc > 3 && !strcmp(argv[3], "tx-power")) {
		param.options = BT_LE_ADV_OPT_USE_TX_POWER;
	} else {
		param.options = 0;
	}

	err = bt_le_per_adv_set_param(adv, &param);
	if (err) {
		shell_error(sh, "Failed to set periodic advertising "
			    "parameters (%d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static ssize_t pa_ad_init(struct bt_data *data_array,
			  const size_t data_array_size)
{
	size_t ad_len = 0;

	if (IS_ENABLED(CONFIG_BT_AUDIO)) {
		ssize_t audio_pa_ad_len;

		audio_pa_ad_len = audio_pa_data_add(&data_array[ad_len],
						    data_array_size - ad_len);
		if (audio_pa_ad_len < 0U) {
			return audio_pa_ad_len;
		}

		ad_len += audio_pa_ad_len;
	}

	return ad_len;
}

static int cmd_per_adv_data(const struct shell *sh, size_t argc,
			    char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	static uint8_t hex_data[256];
	static struct bt_data ad[2U];
	ssize_t stack_ad_len;
	uint8_t ad_len = 0;
	int err;

	if (!adv) {
		shell_error(sh, "No extended advertisement set selected");
		return -EINVAL;
	}

	if (argc > 1) {
		size_t hex_len = 0U;

		(void)memset(hex_data, 0, sizeof(hex_data));
		hex_len = hex2bin(argv[1U], strlen(argv[1U]), hex_data,
				  sizeof(hex_data));

		if (hex_len == 0U) {
			shell_error(sh, "Could not parse adv data");

			return -ENOEXEC;
		}

		ad[ad_len].data_len = hex_data[0U];
		ad[ad_len].type = hex_data[1U];
		ad[ad_len].data = &hex_data[2U];
		ad_len++;
	}

	stack_ad_len = pa_ad_init(&ad[ad_len], ARRAY_SIZE(ad) - ad_len);
	if (stack_ad_len < 0) {
		shell_error(sh, "Failed to get stack PA data");

		return -ENOEXEC;
	}
	ad_len += stack_ad_len;

	err = bt_le_per_adv_set_data(adv, ad, ad_len);
	if (err) {
		shell_error(sh,
			    "Failed to set periodic advertising data (%d)",
			    err);
		return -ENOEXEC;
	}

	return 0;
}
#endif /* CONFIG_BT_PER_ADV */
#endif /* CONFIG_BT_EXT_ADV */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_PER_ADV_SYNC)

static int cmd_per_adv_sync_create(const struct shell *sh, size_t argc,
				   char *argv[])
{
	struct bt_le_per_adv_sync *per_adv_sync = per_adv_syncs[selected_per_adv_sync];
	int err;
	struct bt_le_per_adv_sync_param create_params = { 0 };
	uint32_t options = 0;

	if (per_adv_sync != NULL) {
		shell_error(sh, "Selected per-adv-sync is not NULL");
		return -ENOEXEC;
	}

	err = bt_addr_le_from_str(argv[1], argv[2], &create_params.addr);
	if (err) {
		shell_error(sh, "Invalid peer address (err %d)", err);
		return -ENOEXEC;
	}

	/* Default values */
	create_params.timeout = 1000; /* 10 seconds */
	create_params.skip = 10;

	create_params.sid = strtol(argv[3], NULL, 16);

	for (int j = 4; j < argc; j++) {
		if (!strcmp(argv[j], "aoa")) {
			options |= BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOA;
		} else if (!strcmp(argv[j], "aod_1us")) {
			options |= BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOD_1US;
		} else if (!strcmp(argv[j], "aod_2us")) {
			options |= BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOD_2US;
		} else if (!strcmp(argv[j], "only_cte")) {
			options |=
				BT_LE_PER_ADV_SYNC_OPT_SYNC_ONLY_CONST_TONE_EXT;
		} else if (!strcmp(argv[j], "timeout")) {
			if (++j == argc) {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			create_params.timeout = strtoul(argv[j], NULL, 16);
		} else if (!strcmp(argv[j], "skip")) {
			if (++j == argc) {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			create_params.skip = strtoul(argv[j], NULL, 16);
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}

		/* TODO: add support to parse using the per adv list */
	}

	create_params.options = options;

	err = bt_le_per_adv_sync_create(&create_params, &per_adv_syncs[selected_per_adv_sync]);
	if (err) {
		shell_error(sh, "Per adv sync failed (%d)", err);
	} else {
		shell_print(sh, "Per adv sync pending");
	}

	return 0;
}

static int cmd_per_adv_sync_delete(const struct shell *sh, size_t argc,
				   char *argv[])
{
	struct bt_le_per_adv_sync *per_adv_sync = per_adv_syncs[selected_per_adv_sync];
	int err;

	if (!per_adv_sync) {
		shell_error(sh, "Selected per-adv-sync is NULL");
		return -EINVAL;
	}

	err = bt_le_per_adv_sync_delete(per_adv_sync);

	if (err) {
		shell_error(sh, "Per adv sync delete failed (%d)", err);
	} else {
		shell_print(sh, "Per adv sync deleted");
		per_adv_syncs[selected_per_adv_sync] = NULL;
	}

	return 0;
}

static int cmd_per_adv_sync_select(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc == 2) {
		unsigned long id;
		int err = 0;

		id = shell_strtoul(argv[1], 0, &err);
		if (err != 0) {
			shell_error(sh, "Could not parse id: %d", err);
			return -ENOEXEC;
		}

		if (id > ARRAY_SIZE(adv_sets)) {
			shell_error(sh, "Invalid id: %lu", id);
			return -EINVAL;
		}

		selected_per_adv_sync = id;
		return 0;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(adv_sets); i++) {
		if (adv_sets[i]) {
			shell_print(sh, "PER_ADV_SYNC[%zu] %p", i, adv_sets[i]);
		}
	}

	return -ENOEXEC;
}

#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER)
static int cmd_past_subscribe(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct bt_le_per_adv_sync_transfer_param param;
	int err;
	int i = 0;
	bool global = true;

	if (i == ARRAY_SIZE(per_adv_syncs)) {
		shell_error(sh, "Cannot create more per adv syncs");
		return -ENOEXEC;
	}

	/* Default values */
	param.options = 0;
	param.timeout = 1000; /* 10 seconds */
	param.skip = 10;

	for (int j = 1; j < argc; j++) {
		if (!strcmp(argv[j], "aoa")) {
			param.options |=
				BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOA;
		} else if (!strcmp(argv[j], "aod_1us")) {
			param.options |=
				BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOD_1US;
		} else if (!strcmp(argv[j], "aod_2us")) {
			param.options |=
				BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOD_2US;
		} else if (!strcmp(argv[j], "only_cte")) {
			param.options |=
				BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_ONLY_CTE;
		} else if (!strcmp(argv[j], "timeout")) {
			if (++j == argc) {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			param.timeout = strtoul(argv[j], NULL, 16);
		} else if (!strcmp(argv[j], "skip")) {
			if (++j == argc) {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			param.skip = strtoul(argv[j], NULL, 16);
		} else if (!strcmp(argv[j], "conn")) {
			if (!default_conn) {
				shell_print(sh, "Not connected");
				return -EINVAL;
			}
			global = false;
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	bt_le_per_adv_sync_cb_register(&per_adv_sync_cb);

	err = bt_le_per_adv_sync_transfer_subscribe(
		global ? NULL : default_conn, &param);

	if (err) {
		shell_error(sh, "PAST subscribe failed (%d)", err);
	} else {
		shell_print(sh, "Subscribed to PAST");
	}

	return 0;
}

static int cmd_past_unsubscribe(const struct shell *sh, size_t argc,
				char *argv[])
{
	int err;

	if (argc > 1) {
		if (!strcmp(argv[1], "conn")) {
			if (default_conn) {
				err =
					bt_le_per_adv_sync_transfer_unsubscribe(
						default_conn);
			} else {
				shell_print(sh, "Not connected");
				return -EINVAL;
			}
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	} else {
		err = bt_le_per_adv_sync_transfer_unsubscribe(NULL);
	}

	if (err) {
		shell_error(sh, "PAST unsubscribe failed (%d)", err);
	}

	return err;
}
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER */

#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)
static int cmd_per_adv_sync_transfer(const struct shell *sh, size_t argc,
				     char *argv[])
{
	int err;
	int index;
	struct bt_le_per_adv_sync *per_adv_sync;

	if (argc > 1) {
		index = strtol(argv[1], NULL, 10);
	} else {
		index = 0;
	}

	if (index >= ARRAY_SIZE(per_adv_syncs)) {
		shell_error(sh, "Maximum index is %zu but %d was requested",
			    ARRAY_SIZE(per_adv_syncs) - 1, index);
	}

	per_adv_sync = per_adv_syncs[index];
	if (!per_adv_sync) {
		return -EINVAL;
	}

	err = bt_le_per_adv_sync_transfer(per_adv_sync, default_conn, 0);
	if (err) {
		shell_error(sh, "Periodic advertising sync transfer failed (%d)", err);
	}

	return err;
}
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER */
#endif /* CONFIG_BT_PER_ADV_SYNC */

#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER) && defined(CONFIG_BT_PER_ADV)
static int cmd_per_adv_set_info_transfer(const struct shell *sh, size_t argc,
					 char *argv[])
{
	const struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "%s: at least, one connection is required",
			    sh->ctx->active_cmd.syntax);
		return -ENOEXEC;
	}

	err = bt_le_per_adv_set_info_transfer(adv, default_conn, 0U);
	if (err) {
		shell_error(sh, "Periodic advertising sync transfer failed (%d)", err);
	}

	return err;
}
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER && CONFIG_BT_PER_ADV */

#if defined(CONFIG_BT_TRANSMIT_POWER_CONTROL)
static int cmd_read_remote_tx_power(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc < 3) {
		int err = 0;
		enum bt_conn_le_tx_power_phy phy = strtoul(argv[1], NULL, 16);

		err = bt_conn_le_get_remote_tx_power_level(default_conn, phy);

		if (!err) {
			shell_print(sh, "Read Remote TX Power for PHY %s",
				    tx_pwr_ctrl_phy2str(phy));
		} else {
			shell_print(sh, "error %d", err);
		}
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}
	return 0;
}

static int cmd_read_local_tx_power(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (argc < 3) {
		struct bt_conn_le_tx_power tx_power_level;

		tx_power_level.phy = strtoul(argv[1], NULL, 16);

		int8_t unachievable_current_level = -100;
		/* Arbitrary, these are output parameters.*/
		tx_power_level.current_level = unachievable_current_level;
		tx_power_level.max_level = 6;

		if (default_conn == NULL) {
			shell_error(sh, "Conn handle error, at least one connection is required.");
			return -ENOEXEC;
		}
		err = bt_conn_le_get_tx_power_level(default_conn, &tx_power_level);
		if (err) {
			shell_print(sh, "Commad returned error error %d", err);
			return err;
		}
		if (tx_power_level.current_level == unachievable_current_level) {
			shell_print(sh, "We received no current tx power level.");
			return -EIO;
		}
		shell_print(sh, "Read local TX Power: current level: %d, PHY: %s, Max Level: %d",
			    tx_power_level.current_level,
			    tx_pwr_ctrl_phy2str((enum bt_conn_le_tx_power_phy)tx_power_level.phy),
			    tx_power_level.max_level);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	return err;
}

static int cmd_set_power_report_enable(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc < 4) {
		int err = 0;
		bool local_enable = 0;
		bool remote_enable = 0;

		if (*argv[1] == '1') {
			local_enable = 1;
		}
		if (*argv[2] == '1') {
			remote_enable = 1;
		}
		if (default_conn == NULL) {
			shell_error(sh, "Conn handle error, at least one connection is required.");
			return -ENOEXEC;
		}
		err = bt_conn_le_set_tx_power_report_enable(default_conn, local_enable,
							     remote_enable);
		if (!err) {
			shell_print(sh, "Tx Power Report: local: %s, remote: %s",
				    enabled2str(local_enable), enabled2str(remote_enable));
		} else {
			shell_print(sh, "error %d", err);
		}
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}
	return 0;
}

#endif


#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_CENTRAL)
static int cmd_connect_le(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	bt_addr_le_t addr;
	struct bt_conn *conn;
	uint32_t options = 0;

	/* When no arguments are specified, connect to the last scanned device. */
	if (argc == 1) {
		if (auto_connect.addr_set) {
			bt_addr_le_copy(&addr, &auto_connect.addr);
		} else {
			shell_error(sh, "No connectable adv stored, please trigger a scan first.");
			shell_help(sh);

			return SHELL_CMD_HELP_PRINTED;
		}
	} else {
		err = bt_addr_le_from_str(argv[1], argv[2], &addr);
		if (err) {
			shell_error(sh, "Invalid peer address (err %d)", err);
			return err;
		}
	}

#if defined(CONFIG_BT_EXT_ADV)
	for (size_t argn = 3; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "coded")) {
			options |= BT_CONN_LE_OPT_CODED;
		} else if (!strcmp(arg, "no-1m")) {
			options |= BT_CONN_LE_OPT_NO_1M;
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}
#endif /* defined(CONFIG_BT_EXT_ADV) */

	struct bt_conn_le_create_param *create_params =
		BT_CONN_LE_CREATE_PARAM(options,
					BT_GAP_SCAN_FAST_INTERVAL,
					BT_GAP_SCAN_FAST_INTERVAL);

	err = bt_conn_le_create(&addr, create_params, BT_LE_CONN_PARAM_DEFAULT,
				&conn);
	if (err) {
		shell_error(sh, "Connection failed (%d)", err);
		return -ENOEXEC;
	} else {

		shell_print(sh, "Connection pending");

		/* unref connection obj in advance as app user */
		bt_conn_unref(conn);
	}

	return 0;
}

#if !defined(CONFIG_BT_FILTER_ACCEPT_LIST)
static int cmd_auto_conn(const struct shell *sh, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	int err;

	err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	if (err) {
		shell_error(sh, "Invalid peer address (err %d)", err);
		return err;
	}

	if (argc < 4) {
		return bt_le_set_auto_conn(&addr, BT_LE_CONN_PARAM_DEFAULT);
	} else if (!strcmp(argv[3], "on")) {
		return bt_le_set_auto_conn(&addr, BT_LE_CONN_PARAM_DEFAULT);
	} else if (!strcmp(argv[3], "off")) {
		return bt_le_set_auto_conn(&addr, NULL);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	return 0;
}
#endif /* !defined(CONFIG_BT_FILTER_ACCEPT_LIST) */

static int cmd_connect_le_name(const struct shell *sh, size_t argc, char *argv[])
{
	const uint16_t timeout_seconds = 10;
	const struct bt_le_scan_param param = {
		.type       = BT_LE_SCAN_TYPE_ACTIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
		.timeout    = timeout_seconds * 100, /* 10ms units */
	};
	int err;

	/* Set the name filter which we will use in the scan callback to
	 * automatically connect to the first device that passes the filter
	 */
	err = cmd_scan_filter_set_name(sh, argc, argv);
	if (err) {
		shell_error(sh,
			    "Bluetooth set scan filter name to %s failed (err %d)",
			    argv[1], err);
		return err;
	}

	err = bt_le_scan_start(&param, NULL);
	if (err) {
		shell_error(sh, "Bluetooth scan failed (err %d)", err);
		return err;
	}

	shell_print(sh, "Bluetooth active scan enabled");

	/* Set boolean to tell the scan callback to connect to this name */
	auto_connect.connect_name = true;

	return 0;
}
#endif /* CONFIG_BT_CENTRAL */

static int cmd_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_conn *conn;
	int err;

	if (default_conn && argc < 3) {
		conn = bt_conn_ref(default_conn);
	} else {
		bt_addr_le_t addr;

		if (argc < 3) {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}

		err = bt_addr_le_from_str(argv[1], argv[2], &addr);
		if (err) {
			shell_error(sh, "Invalid peer address (err %d)",
				    err);
			return err;
		}

		conn = bt_conn_lookup_addr_le(selected_id, &addr);
	}

	if (!conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		shell_error(sh, "Disconnection failed (err %d)", err);
		return err;
	}

	bt_conn_unref(conn);

	return 0;
}

static int cmd_select(const struct shell *sh, size_t argc, char *argv[])
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	struct bt_conn *conn;
	bt_addr_le_t addr;
	int err;

	err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	if (err) {
		shell_error(sh, "Invalid peer address (err %d)", err);
		return err;
	}

	conn = bt_conn_lookup_addr_le(selected_id, &addr);
	if (!conn) {
		shell_error(sh, "No matching connection found");
		return -ENOEXEC;
	}

	if (default_conn) {
		bt_conn_unref(default_conn);
	}

	default_conn = conn;

	bt_addr_le_to_str(&addr, addr_str, sizeof(addr_str));
	shell_print(sh, "Selected conn is now: %s", addr_str);

	return 0;
}

static const char *get_conn_type_str(uint8_t type)
{
	switch (type) {
	case BT_CONN_TYPE_LE: return "LE";
	case BT_CONN_TYPE_BR: return "BR/EDR";
	case BT_CONN_TYPE_SCO: return "SCO";
	default: return "Invalid";
	}
}

static const char *get_conn_role_str(uint8_t role)
{
	switch (role) {
	case BT_CONN_ROLE_CENTRAL: return "central";
	case BT_CONN_ROLE_PERIPHERAL: return "peripheral";
	default: return "Invalid";
	}
}

static int cmd_info(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_conn *conn = NULL;
	struct bt_conn_info info;
	bt_addr_le_t addr;
	int err;

	switch (argc) {
	case 1:
		if (default_conn) {
			conn = bt_conn_ref(default_conn);
		}
		break;
	case 2:
		addr.type = BT_ADDR_LE_PUBLIC;
		err = bt_addr_from_str(argv[1], &addr.a);
		if (err) {
			shell_error(sh, "Invalid peer address (err %d)",
				    err);
			return err;
		}
		conn = bt_conn_lookup_addr_le(selected_id, &addr);
		break;
	case 3:
		err = bt_addr_le_from_str(argv[1], argv[2], &addr);

		if (err) {
			shell_error(sh, "Invalid peer address (err %d)",
				    err);
			return err;
		}
		conn = bt_conn_lookup_addr_le(selected_id, &addr);
		break;
	}

	if (!conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	err = bt_conn_get_info(conn, &info);
	if (err) {
		shell_print(ctx_shell, "Failed to get info");
		goto done;
	}

	shell_print(ctx_shell, "Type: %s, Role: %s, Id: %u",
		    get_conn_type_str(info.type),
		    get_conn_role_str(info.role),
		    info.id);

	if (info.type == BT_CONN_TYPE_LE) {
		print_le_addr("Remote", info.le.dst);
		print_le_addr("Local", info.le.src);
		print_le_addr("Remote on-air", info.le.remote);
		print_le_addr("Local on-air", info.le.local);

		shell_print(ctx_shell, "Interval: 0x%04x (%u us)",
			    info.le.interval,
			    BT_CONN_INTERVAL_TO_US(info.le.interval));
		shell_print(ctx_shell, "Latency: 0x%04x",
			    info.le.latency);
		shell_print(ctx_shell, "Supervision timeout: 0x%04x (%d ms)",
			    info.le.timeout, info.le.timeout * 10);
#if defined(CONFIG_BT_USER_PHY_UPDATE)
		shell_print(ctx_shell, "LE PHY: TX PHY %s, RX PHY %s",
			    phy2str(info.le.phy->tx_phy),
			    phy2str(info.le.phy->rx_phy));
#endif
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
		shell_print(ctx_shell, "LE data len: TX (len: %d time: %d)"
			    " RX (len: %d time: %d)",
			    info.le.data_len->tx_max_len,
			    info.le.data_len->tx_max_time,
			    info.le.data_len->rx_max_len,
			    info.le.data_len->rx_max_time);
#endif
	}

#if defined(CONFIG_BT_CLASSIC)
	if (info.type == BT_CONN_TYPE_BR) {
		char addr_str[BT_ADDR_STR_LEN];

		bt_addr_to_str(info.br.dst, addr_str, sizeof(addr_str));
		shell_print(ctx_shell, "Peer address %s", addr_str);
	}
#endif /* defined(CONFIG_BT_CLASSIC) */

done:
	bt_conn_unref(conn);

	return err;
}

static int cmd_conn_update(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_le_conn_param param;
	int err;

	if (default_conn == NULL) {
		shell_error(sh,
				"%s: at least, one connection is required",
				sh->ctx->active_cmd.syntax);
		return -ENOEXEC;
	}

	param.interval_min = strtoul(argv[1], NULL, 16);
	param.interval_max = strtoul(argv[2], NULL, 16);
	param.latency = strtoul(argv[3], NULL, 16);
	param.timeout = strtoul(argv[4], NULL, 16);

	err = bt_conn_le_param_update(default_conn, &param);
	if (err) {
		shell_error(sh, "conn update failed (err %d).", err);
	} else {
		shell_print(sh, "conn update initiated.");
	}

	return err;
}

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
static uint16_t tx_time_calc(uint8_t phy, uint16_t max_len)
{
	/* Access address + header + payload + MIC + CRC */
	uint16_t total_len = 4 + 2 + max_len + 4 + 3;

	switch (phy) {
	case BT_GAP_LE_PHY_1M:
		/* 1 byte preamble, 8 us per byte */
		return 8 * (1 + total_len);
	case BT_GAP_LE_PHY_2M:
		/* 2 byte preamble, 4 us per byte */
		return 4 * (2 + total_len);
	case BT_GAP_LE_PHY_CODED:
		/* S8: Preamble + CI + TERM1 + 64 us per byte + TERM2 */
		return 80 + 16 + 24 + 64 * (total_len) + 24;
	default:
		return 0;
	}
}

static int cmd_conn_data_len_update(const struct shell *sh, size_t argc,
				    char *argv[])
{
	struct bt_conn_le_data_len_param param;
	int err;

	if (default_conn == NULL) {
		shell_error(sh,
				"%s: at least, one connection is required",
				sh->ctx->active_cmd.syntax);
		return -ENOEXEC;
	}

	param.tx_max_len = strtoul(argv[1], NULL, 10);

	if (argc > 2) {
		param.tx_max_time = strtoul(argv[2], NULL, 10);
	} else {
		/* Assume 1M if not able to retrieve PHY */
		uint8_t phy = BT_GAP_LE_PHY_1M;

#if defined(CONFIG_BT_USER_PHY_UPDATE)
		struct bt_conn_info info;

		err = bt_conn_get_info(default_conn, &info);
		if (!err) {
			phy = info.le.phy->tx_phy;
		}
#endif
		param.tx_max_time = tx_time_calc(phy, param.tx_max_len);
		shell_print(sh, "Calculated tx time: %d", param.tx_max_time);
	}



	err = bt_conn_le_data_len_update(default_conn, &param);
	if (err) {
		shell_error(sh, "data len update failed (err %d).", err);
	} else {
		shell_print(sh, "data len update initiated.");
	}

	return err;
}
#endif

#if defined(CONFIG_BT_USER_PHY_UPDATE)
static int cmd_conn_phy_update(const struct shell *sh, size_t argc,
			       char *argv[])
{
	struct bt_conn_le_phy_param param;
	int err;

	if (default_conn == NULL) {
		shell_error(sh,
				"%s: at least, one connection is required",
				sh->ctx->active_cmd.syntax);
		return -ENOEXEC;
	}

	param.pref_tx_phy = strtoul(argv[1], NULL, 16);
	param.pref_rx_phy = param.pref_tx_phy;
	param.options = BT_CONN_LE_PHY_OPT_NONE;

	for (size_t argn = 2; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "s2")) {
			param.options |= BT_CONN_LE_PHY_OPT_CODED_S2;
		} else if (!strcmp(arg, "s8")) {
			param.options |= BT_CONN_LE_PHY_OPT_CODED_S8;
		} else {
			param.pref_rx_phy = strtoul(arg, NULL, 16);
		}
	}

	err = bt_conn_le_phy_update(default_conn, &param);
	if (err) {
		shell_error(sh, "PHY update failed (err %d).", err);
	} else {
		shell_print(sh, "PHY update initiated.");
	}

	return err;
}
#endif

#if defined(CONFIG_BT_CENTRAL) || defined(CONFIG_BT_BROADCASTER)
static int cmd_chan_map(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t chan_map[5] = {};
	int err;

	if (hex2bin(argv[1], strlen(argv[1]), chan_map, 5) == 0) {
		shell_error(sh, "Invalid channel map");
		return -ENOEXEC;
	}
	sys_mem_swap(chan_map, 5);

	err = bt_le_set_chan_map(chan_map);
	if (err) {
		shell_error(sh, "Failed to set channel map (err %d)", err);
	} else {
		shell_print(sh, "Channel map set");
	}

	return err;
}
#endif /* CONFIG_BT_CENTRAL */

static int cmd_oob(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_le_oob_get_local(selected_id, &oob_local);
	if (err) {
		shell_error(sh, "OOB data failed");
		return err;
	}

	print_le_oob(sh, &oob_local);

	return 0;
}

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
static int cmd_oob_remote(const struct shell *sh, size_t argc,
			     char *argv[])
{
	int err;
	bt_addr_le_t addr;

	err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	if (err) {
		shell_error(sh, "Invalid peer address (err %d)", err);
		return err;
	}

	bt_addr_le_copy(&oob_remote.addr, &addr);

	if (argc == 5) {
		hex2bin(argv[3], strlen(argv[3]), oob_remote.le_sc_data.r,
			sizeof(oob_remote.le_sc_data.r));
		hex2bin(argv[4], strlen(argv[4]), oob_remote.le_sc_data.c,
			sizeof(oob_remote.le_sc_data.c));
		bt_le_oob_set_sc_flag(true);
	} else {
		shell_help(sh);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_oob_clear(const struct shell *sh, size_t argc, char *argv[])
{
	memset(&oob_remote, 0, sizeof(oob_remote));
	bt_le_oob_set_sc_flag(false);

	return 0;
}
#endif /* CONFIG_BT_SMP || CONFIG_BT_CLASSIC) */

static int cmd_clear(const struct shell *sh, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	int err;

	if (strcmp(argv[1], "all") == 0) {
		err = bt_unpair(selected_id, NULL);
		if (err) {
			shell_error(sh, "Failed to clear pairings (err %d)",
			      err);
			return err;
		} else {
			shell_print(sh, "Pairings successfully cleared");
		}

		return 0;
	}

	if (argc < 3) {
#if defined(CONFIG_BT_CLASSIC)
		addr.type = BT_ADDR_LE_PUBLIC;
		err = bt_addr_from_str(argv[1], &addr.a);
#else
		shell_print(sh, "Both address and address type needed");
		return -ENOEXEC;
#endif
	} else {
		err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	}

	if (err) {
		shell_print(sh, "Invalid address");
		return err;
	}

	err = bt_unpair(selected_id, &addr);
	if (err) {
		shell_error(sh, "Failed to clear pairing (err %d)", err);
	} else {
		shell_print(sh, "Pairing successfully cleared");
	}

	return err;
}
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
static int cmd_security(const struct shell *sh, size_t argc, char *argv[])
{
	int err, sec;
	struct bt_conn_info info;

	if (!default_conn || (bt_conn_get_info(default_conn, &info) < 0)) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (argc < 2) {
		shell_print(sh, "BT_SECURITY_L%d", bt_conn_get_security(default_conn));

		return 0;
	}

	sec = *argv[1] - '0';

	if ((info.type == BT_CONN_TYPE_BR &&
	    (sec < BT_SECURITY_L0 || sec > BT_SECURITY_L3))) {
		shell_error(sh, "Invalid BR/EDR security level (%d)", sec);
		return -ENOEXEC;
	}

	if ((info.type == BT_CONN_TYPE_LE &&
	    (sec < BT_SECURITY_L1 || sec > BT_SECURITY_L4))) {
		shell_error(sh, "Invalid LE security level (%d)", sec);
		return -ENOEXEC;
	}

	if (argc > 2) {
		if (!strcmp(argv[2], "force-pair")) {
			sec |= BT_SECURITY_FORCE_PAIR;
		} else {
			shell_help(sh);
			return -ENOEXEC;
		}
	}

	err = bt_conn_set_security(default_conn, sec);
	if (err) {
		shell_error(sh, "Setting security failed (err %d)", err);
	}

	return err;
}

static int cmd_bondable(const struct shell *sh, size_t argc, char *argv[])
{
	const char *bondable;

	bondable = argv[1];
	if (!strcmp(bondable, "on")) {
		bt_set_bondable(true);
	} else if (!strcmp(bondable, "off")) {
		bt_set_bondable(false);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	return 0;
}

static void bond_info(const struct bt_bond_info *info, void *user_data)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int *bond_count = user_data;

	bt_addr_le_to_str(&info->addr, addr, sizeof(addr));
	shell_print(ctx_shell, "Remote Identity: %s", addr);
	(*bond_count)++;
}

static int cmd_bonds(const struct shell *sh, size_t argc, char *argv[])
{
	int bond_count = 0;

	shell_print(sh, "Bonded devices:");
	bt_foreach_bond(selected_id, bond_info, &bond_count);
	shell_print(sh, "Total %d", bond_count);

	return 0;
}

static const char *role_str(uint8_t role)
{
	switch (role) {
	case BT_CONN_ROLE_CENTRAL:
		return "Central";
	case BT_CONN_ROLE_PERIPHERAL:
		return "Peripheral";
	}

	return "Unknown";
}

static void connection_info(struct bt_conn *conn, void *user_data)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int *conn_count = user_data;
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info) < 0) {
		shell_error(ctx_shell, "Unable to get info: conn %p", conn);
		return;
	}

	switch (info.type) {
#if defined(CONFIG_BT_CLASSIC)
	case BT_CONN_TYPE_BR:
		bt_addr_to_str(info.br.dst, addr, sizeof(addr));
		shell_print(ctx_shell, " #%u [BR][%s] %s", info.id, role_str(info.role), addr);
		break;
#endif
	case BT_CONN_TYPE_LE:
		bt_addr_le_to_str(info.le.dst, addr, sizeof(addr));
		shell_print(ctx_shell, "%s#%u [LE][%s] %s: Interval %u latency %u timeout %u",
			    conn == default_conn ? "*" : " ", info.id, role_str(info.role), addr,
			    info.le.interval, info.le.latency, info.le.timeout);
		break;
#if defined(CONFIG_BT_ISO)
	case BT_CONN_TYPE_ISO:
		bt_addr_le_to_str(info.le.dst, addr, sizeof(addr));
		shell_print(ctx_shell, " #%u [ISO][%s] %s", info.id, role_str(info.role), addr);
		break;
#endif
	default:
		break;
	}

	(*conn_count)++;
}

static int cmd_connections(const struct shell *sh, size_t argc, char *argv[])
{
	int conn_count = 0;

	shell_print(sh, "Connected devices:");
	bt_conn_foreach(BT_CONN_TYPE_ALL, connection_info, &conn_count);
	shell_print(sh, "Total %d", conn_count);

	return 0;
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char passkey_str[7];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	snprintk(passkey_str, 7, "%06u", passkey);

	shell_print(ctx_shell, "Passkey for %s: %s", addr, passkey_str);
}

#if defined(CONFIG_BT_PASSKEY_KEYPRESS)
static void auth_passkey_display_keypress(struct bt_conn *conn,
					  enum bt_conn_auth_keypress type)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	shell_print(ctx_shell, "Passkey keypress notification from %s: type %d",
		    addr, type);
}
#endif

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char passkey_str[7];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	snprintk(passkey_str, 7, "%06u", passkey);

	shell_print(ctx_shell, "Confirm passkey for %s: %s", addr, passkey_str);
}

static void auth_passkey_entry(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	shell_print(ctx_shell, "Enter passkey for %s", addr);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));

	shell_print(ctx_shell, "Pairing cancelled: %s", addr);

	/* clear connection reference for sec mode 3 pairing */
	if (pairing_conn) {
		bt_conn_unref(pairing_conn);
		pairing_conn = NULL;
	}
}

static void auth_pairing_confirm(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	shell_print(ctx_shell, "Confirm pairing for %s", addr);
}

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
static const char *oob_config_str(int oob_config)
{
	switch (oob_config) {
	case BT_CONN_OOB_LOCAL_ONLY:
		return "Local";
	case BT_CONN_OOB_REMOTE_ONLY:
		return "Remote";
	case BT_CONN_OOB_BOTH_PEERS:
		return "Local and Remote";
	case BT_CONN_OOB_NO_DATA:
	default:
		return "no";
	}
}
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */

static void auth_pairing_oob_data_request(struct bt_conn *conn,
					  struct bt_conn_oob_info *oob_info)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err) {
		return;
	}

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
	if (oob_info->type == BT_CONN_OOB_LE_SC) {
		struct bt_le_oob_sc_data *oobd_local =
			oob_info->lesc.oob_config != BT_CONN_OOB_REMOTE_ONLY
						  ? &oob_local.le_sc_data
						  : NULL;
		struct bt_le_oob_sc_data *oobd_remote =
			oob_info->lesc.oob_config != BT_CONN_OOB_LOCAL_ONLY
						  ? &oob_remote.le_sc_data
						  : NULL;

		if (oobd_remote &&
		    !bt_addr_le_eq(info.le.remote, &oob_remote.addr)) {
			bt_addr_le_to_str(info.le.remote, addr, sizeof(addr));
			shell_print(ctx_shell,
				    "No OOB data available for remote %s",
				    addr);
			bt_conn_auth_cancel(conn);
			return;
		}

		if (oobd_local &&
		    !bt_addr_le_eq(info.le.local, &oob_local.addr)) {
			bt_addr_le_to_str(info.le.local, addr, sizeof(addr));
			shell_print(ctx_shell,
				    "No OOB data available for local %s",
				    addr);
			bt_conn_auth_cancel(conn);
			return;
		}

		bt_le_oob_set_sc_data(conn, oobd_local, oobd_remote);

		bt_addr_le_to_str(info.le.dst, addr, sizeof(addr));
		shell_print(ctx_shell, "Set %s OOB SC data for %s, ",
			    oob_config_str(oob_info->lesc.oob_config), addr);
		return;
	}
#endif /* CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY */

	bt_addr_le_to_str(info.le.dst, addr, sizeof(addr));
	shell_print(ctx_shell, "Legacy OOB TK requested from remote %s", addr);
}

static void auth_pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	shell_print(ctx_shell, "%s with %s", bonded ? "Bonded" : "Paired",
		    addr);
}

static void auth_pairing_failed(struct bt_conn *conn, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	shell_print(ctx_shell, "Pairing failed with %s reason: %s (%d)", addr,
		    security_err_str(err), err);
}

#if defined(CONFIG_BT_CLASSIC)
static void auth_pincode_entry(struct bt_conn *conn, bool highsec)
{
	char addr[BT_ADDR_STR_LEN];
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info) < 0) {
		return;
	}

	if (info.type != BT_CONN_TYPE_BR) {
		return;
	}

	bt_addr_to_str(info.br.dst, addr, sizeof(addr));

	if (highsec) {
		shell_print(ctx_shell, "Enter 16 digits wide PIN code for %s",
			    addr);
	} else {
		shell_print(ctx_shell, "Enter PIN code for %s", addr);
	}

	/*
	 * Save connection info since in security mode 3 (link level enforced
	 * security) PIN request callback is called before connected callback
	 */
	if (!default_conn && !pairing_conn) {
		pairing_conn = bt_conn_ref(conn);
	}
}
#endif

#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
enum bt_security_err pairing_accept(
	struct bt_conn *conn, const struct bt_conn_pairing_feat *const feat)
{
	shell_print(ctx_shell, "Remote pairing features: "
			       "IO: 0x%02x, OOB: %d, AUTH: 0x%02x, Key: %d, "
			       "Init Kdist: 0x%02x, Resp Kdist: 0x%02x",
			       feat->io_capability, feat->oob_data_flag,
			       feat->auth_req, feat->max_enc_key_size,
			       feat->init_key_dist, feat->resp_key_dist);

	return BT_SECURITY_ERR_SUCCESS;
}
#endif /* CONFIG_BT_SMP_APP_PAIRING_ACCEPT */

void bond_deleted(uint8_t id, const bt_addr_le_t *peer)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(peer, addr, sizeof(addr));
	shell_print(ctx_shell, "Bond deleted for %s, id %u", addr, id);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
#if defined(CONFIG_BT_PASSKEY_KEYPRESS)
	.passkey_display_keypress = auth_passkey_display_keypress,
#endif
	.passkey_entry = NULL,
	.passkey_confirm = NULL,
#if defined(CONFIG_BT_CLASSIC)
	.pincode_entry = auth_pincode_entry,
#endif
	.oob_data_request = NULL,
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
	.pairing_accept = pairing_accept,
#endif
};

static struct bt_conn_auth_cb auth_cb_display_yes_no = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.passkey_confirm = auth_passkey_confirm,
#if defined(CONFIG_BT_CLASSIC)
	.pincode_entry = auth_pincode_entry,
#endif
	.oob_data_request = NULL,
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
	.pairing_accept = pairing_accept,
#endif
};

static struct bt_conn_auth_cb auth_cb_input = {
	.passkey_display = NULL,
	.passkey_entry = auth_passkey_entry,
	.passkey_confirm = NULL,
#if defined(CONFIG_BT_CLASSIC)
	.pincode_entry = auth_pincode_entry,
#endif
	.oob_data_request = NULL,
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
	.pairing_accept = pairing_accept,
#endif
};

static struct bt_conn_auth_cb auth_cb_confirm = {
#if defined(CONFIG_BT_CLASSIC)
	.pincode_entry = auth_pincode_entry,
#endif
	.oob_data_request = NULL,
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
	.pairing_accept = pairing_accept,
#endif
};

static struct bt_conn_auth_cb auth_cb_all = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = auth_passkey_entry,
	.passkey_confirm = auth_passkey_confirm,
#if defined(CONFIG_BT_CLASSIC)
	.pincode_entry = auth_pincode_entry,
#endif
	.oob_data_request = auth_pairing_oob_data_request,
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
	.pairing_accept = pairing_accept,
#endif
};

static struct bt_conn_auth_cb auth_cb_oob = {
	.passkey_display = NULL,
	.passkey_entry = NULL,
	.passkey_confirm = NULL,
#if defined(CONFIG_BT_CLASSIC)
	.pincode_entry = NULL,
#endif
	.oob_data_request = auth_pairing_oob_data_request,
	.cancel = auth_cancel,
	.pairing_confirm = NULL,
#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
	.pairing_accept = pairing_accept,
#endif
};

static struct bt_conn_auth_cb auth_cb_status = {
#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
	.pairing_accept = pairing_accept,
#endif
};

static struct bt_conn_auth_info_cb auth_info_cb = {
	.pairing_failed = auth_pairing_failed,
	.pairing_complete = auth_pairing_complete,
	.bond_deleted = bond_deleted,
};

static int cmd_auth(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!strcmp(argv[1], "all")) {
		err = bt_conn_auth_cb_register(&auth_cb_all);
	} else if (!strcmp(argv[1], "input")) {
		err = bt_conn_auth_cb_register(&auth_cb_input);
	} else if (!strcmp(argv[1], "display")) {
		err = bt_conn_auth_cb_register(&auth_cb_display);
	} else if (!strcmp(argv[1], "yesno")) {
		err = bt_conn_auth_cb_register(&auth_cb_display_yes_no);
	} else if (!strcmp(argv[1], "confirm")) {
		err = bt_conn_auth_cb_register(&auth_cb_confirm);
	} else if (!strcmp(argv[1], "oob")) {
		err = bt_conn_auth_cb_register(&auth_cb_oob);
	} else if (!strcmp(argv[1], "status")) {
		err = bt_conn_auth_cb_register(&auth_cb_status);
	} else if (!strcmp(argv[1], "none")) {
		err = bt_conn_auth_cb_register(NULL);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (err) {
		shell_error(sh, "Failed to set auth handlers (%d)", err);
	}

	return err;
}

static int cmd_auth_cancel(const struct shell *sh,
			   size_t argc, char *argv[])
{
	struct bt_conn *conn;

	if (default_conn) {
		conn = default_conn;
	} else if (pairing_conn) {
		conn = pairing_conn;
	} else {
		conn = NULL;
	}

	if (!conn) {
		shell_print(sh, "Not connected");
		return -ENOEXEC;
	}

	bt_conn_auth_cancel(conn);

	return 0;
}

static int cmd_auth_passkey_confirm(const struct shell *sh,
				    size_t argc, char *argv[])
{
	if (!default_conn) {
		shell_print(sh, "Not connected");
		return -ENOEXEC;
	}

	bt_conn_auth_passkey_confirm(default_conn);
	return 0;
}

static int cmd_auth_pairing_confirm(const struct shell *sh,
				    size_t argc, char *argv[])
{
	if (!default_conn) {
		shell_print(sh, "Not connected");
		return -ENOEXEC;
	}

	bt_conn_auth_pairing_confirm(default_conn);
	return 0;
}

#if defined(CONFIG_BT_FILTER_ACCEPT_LIST)
static int cmd_fal_add(const struct shell *sh, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	int err;

	err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	if (err) {
		shell_error(sh, "Invalid peer address (err %d)", err);
		return err;
	}

	err = bt_le_filter_accept_list_add(&addr);
	if (err) {
		shell_error(sh, "Add to fa list failed (err %d)", err);
		return err;
	}

	return 0;
}

static int cmd_fal_rem(const struct shell *sh, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	int err;

	err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	if (err) {
		shell_error(sh, "Invalid peer address (err %d)", err);
		return err;
	}

	err = bt_le_filter_accept_list_remove(&addr);
	if (err) {
		shell_error(sh, "Remove from fa list failed (err %d)",
			    err);
		return err;
	}
	return 0;
}

static int cmd_fal_clear(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_le_filter_accept_list_clear();
	if (err) {
		shell_error(sh, "Clearing fa list failed (err %d)", err);
		return err;
	}

	return 0;
}

#if defined(CONFIG_BT_CENTRAL)
static int cmd_fal_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	const char *action = argv[1];
	uint32_t options = 0;

#if defined(CONFIG_BT_EXT_ADV)
	for (size_t argn = 2; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "coded")) {
			options |= BT_CONN_LE_OPT_CODED;
		} else if (!strcmp(arg, "no-1m")) {
			options |= BT_CONN_LE_OPT_NO_1M;
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}
#endif /* defined(CONFIG_BT_EXT_ADV) */
	struct bt_conn_le_create_param *create_params =
		BT_CONN_LE_CREATE_PARAM(options,
					BT_GAP_SCAN_FAST_INTERVAL,
					BT_GAP_SCAN_FAST_WINDOW);

	if (!strcmp(action, "on")) {
		err = bt_conn_le_create_auto(create_params,
					     BT_LE_CONN_PARAM_DEFAULT);
		if (err) {
			shell_error(sh, "Auto connect failed (err %d)", err);
			return err;
		}
	} else if (!strcmp(action, "off")) {
		err = bt_conn_create_auto_stop();
		if (err) {
			shell_error(sh, "Auto connect stop failed (err %d)",
				    err);
		}
		return err;
	}

	return 0;
}
#endif /* CONFIG_BT_CENTRAL */
#endif /* defined(CONFIG_BT_FILTER_ACCEPT_LIST) */

#if defined(CONFIG_BT_FIXED_PASSKEY)
static int cmd_fixed_passkey(const struct shell *sh,
			     size_t argc, char *argv[])
{
	unsigned int passkey;
	int err;

	if (argc < 2) {
		bt_passkey_set(BT_PASSKEY_INVALID);
		shell_print(sh, "Fixed passkey cleared");
		return 0;
	}

	passkey = atoi(argv[1]);
	if (passkey > 999999) {
		shell_print(sh, "Passkey should be between 0-999999");
		return -ENOEXEC;
	}

	err = bt_passkey_set(passkey);
	if (err) {
		shell_print(sh, "Setting fixed passkey failed (err %d)",
			    err);
	}

	return err;
}
#endif

static int cmd_auth_passkey(const struct shell *sh,
			    size_t argc, char *argv[])
{
	unsigned int passkey;
	int err;

	if (!default_conn) {
		shell_print(sh, "Not connected");
		return -ENOEXEC;
	}

	passkey = atoi(argv[1]);
	if (passkey > 999999) {
		shell_print(sh, "Passkey should be between 0-999999");
		return -EINVAL;
	}

	err = bt_conn_auth_passkey_entry(default_conn, passkey);
	if (err) {
		shell_error(sh, "Failed to set passkey (%d)", err);
		return err;
	}

	return 0;
}

#if defined(CONFIG_BT_PASSKEY_KEYPRESS)
static int cmd_auth_passkey_notify(const struct shell *sh,
				   size_t argc, char *argv[])
{
	unsigned long type;
	int err;

	if (!default_conn) {
		shell_print(sh, "Not connected");
		return -ENOEXEC;
	}

	err = 0;
	type = shell_strtoul(argv[1], 0, &err);
	if (err ||
	    (type != BT_CONN_AUTH_KEYPRESS_ENTRY_STARTED &&
	     type != BT_CONN_AUTH_KEYPRESS_DIGIT_ENTERED &&
	     type != BT_CONN_AUTH_KEYPRESS_DIGIT_ERASED && type != BT_CONN_AUTH_KEYPRESS_CLEARED &&
	     type != BT_CONN_AUTH_KEYPRESS_ENTRY_COMPLETED)) {
		shell_error(sh, "<type> must be a value of enum bt_conn_auth_keypress");
		return -EINVAL;
	}

	err = bt_conn_auth_keypress_notify(default_conn, type);
	if (err) {
		shell_error(sh, "bt_conn_auth_keypress_notify errno %d", err);
		return err;
	}

	return 0;
}
#endif /* CONFIG_BT_PASSKEY_KEYPRESS */

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
static int cmd_auth_oob_tk(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t tk[16];
	size_t len;
	int err;

	len = hex2bin(argv[1], strlen(argv[1]), tk, sizeof(tk));
	if (len != sizeof(tk)) {
		shell_error(sh, "TK should be 16 bytes");
		return -EINVAL;
	}

	err = bt_le_oob_set_legacy_tk(default_conn, tk);
	if (err) {
		shell_error(sh, "Failed to set TK (%d)", err);
		return err;
	}

	return 0;
}
#endif /* !defined(CONFIG_BT_SMP_SC_PAIR_ONLY) */
#endif /* CONFIG_BT_SMP) || CONFIG_BT_CLASSIC */

#if defined(CONFIG_BT_EAD)
static int cmd_encrypted_ad_set_keys(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;

	const char *session_key = argv[1];
	const char *iv = argv[2];

	len = hex2bin(session_key, strlen(session_key), bt_shell_ead_session_key, BT_EAD_KEY_SIZE);
	if (len != BT_EAD_KEY_SIZE) {
		shell_error(sh, "Failed to set session key");
		return -ENOEXEC;
	}

	len = hex2bin(iv, strlen(iv), bt_shell_ead_iv, BT_EAD_IV_SIZE);
	if (len != BT_EAD_IV_SIZE) {
		shell_error(sh, "Failed to set initialisation vector");
		return -ENOEXEC;
	}

	shell_info(sh, "session key set to:");
	shell_hexdump(sh, bt_shell_ead_session_key, BT_EAD_KEY_SIZE);
	shell_info(sh, "initialisation vector set to:");
	shell_hexdump(sh, bt_shell_ead_iv, BT_EAD_IV_SIZE);

	return 0;
}

int encrypted_ad_store_ad(const struct shell *sh, uint8_t type, const uint8_t *data,
			  uint8_t data_len)
{
	/* data_len is the size of the data, add two bytes for the size of the type
	 * and the length that will be stored with the data
	 */
	uint8_t new_data_size = data_len + 2;

	if (bt_shell_ead_data_size + new_data_size > BT_SHELL_EAD_DATA_MAX_SIZE) {
		shell_error(sh, "Failed to add data (trying to add %d but %d already used on %d)",
			    new_data_size, bt_shell_ead_data_size, BT_SHELL_EAD_DATA_MAX_SIZE);
		return -ENOEXEC;
	}

	/* the length is the size of the data + the size of the type */
	bt_shell_ead_data[bt_shell_ead_data_size] = data_len + 1;
	bt_shell_ead_data[bt_shell_ead_data_size + 1] = type;
	memcpy(&bt_shell_ead_data[bt_shell_ead_data_size + 2], data, data_len);

	bt_shell_ead_data_size += new_data_size;
	bt_shell_ead_ad_len += 1;

	return 0;
}

bool is_payload_valid_ad(uint8_t *payload, size_t payload_size)
{
	size_t idx = 0;
	bool is_valid = true;

	uint8_t ad_len;

	while (idx < payload_size) {
		ad_len = payload[idx];

		if (payload_size <= ad_len) {
			is_valid = false;
			break;
		}

		idx += ad_len + 1;
	}

	if (idx != payload_size) {
		is_valid = false;
	}

	return is_valid;
}

static int cmd_encrypted_ad_add_ead(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;

	char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	uint8_t payload[BT_SHELL_EAD_DATA_MAX_SIZE] = {0};
	uint8_t payload_size = hex_payload_size / 2 + hex_payload_size % 2;

	uint8_t ead_size = BT_EAD_ENCRYPTED_PAYLOAD_SIZE(payload_size);

	if (ead_size > BT_SHELL_EAD_DATA_MAX_SIZE) {
		shell_error(sh,
			    "Failed to add data. Maximum AD size is %d, passed data size after "
			    "encryption is %d",
			    BT_SHELL_EAD_DATA_MAX_SIZE, ead_size);
		return -ENOEXEC;
	}

	len = hex2bin(hex_payload, hex_payload_size, payload, BT_SHELL_EAD_DATA_MAX_SIZE);
	if (len != payload_size) {
		shell_error(sh, "Failed to add data");
		return -ENOEXEC;
	}

	/* check that the given advertising data structures are valid before encrypting them */
	if (!is_payload_valid_ad(payload, payload_size)) {
		shell_error(sh, "Failed to add data. Advertising structure are malformed.");
		return -ENOEXEC;
	}

	/* store not-yet encrypted AD but claim the expected size of encrypted AD */
	return encrypted_ad_store_ad(sh, BT_DATA_ENCRYPTED_AD_DATA, payload, ead_size);
}

static int cmd_encrypted_ad_add_ad(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	uint8_t ad_len;
	uint8_t ad_type;

	char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	uint8_t payload[BT_SHELL_EAD_DATA_MAX_SIZE] = {0};
	uint8_t payload_size = hex_payload_size / 2 + hex_payload_size % 2;

	len = hex2bin(hex_payload, hex_payload_size, payload, BT_SHELL_EAD_DATA_MAX_SIZE);
	if (len != payload_size) {
		shell_error(sh, "Failed to add data");
		return -ENOEXEC;
	}

	/* the length received is the length of ad data + the length of the data
	 * type but `bt_data` struct `data_len` field is only the size of the
	 * data
	 */
	ad_len = payload[0] - 1;
	ad_type = payload[1];

	/* if the ad data is malformed or there is more than 1 ad data passed we
	 * fail
	 */
	if (len != ad_len + 2) {
		shell_error(sh,
			    "Failed to add data. Data need to be formated as specified in the "
			    "Core Spec. Only one non-encrypted AD payload can be added at a time.");
		return -ENOEXEC;
	}

	return encrypted_ad_store_ad(sh, ad_type, payload, payload_size);
}

static int cmd_encrypted_ad_clear_ad(const struct shell *sh, size_t argc, char *argv[])
{
	memset(bt_shell_ead_data, 0, BT_SHELL_EAD_DATA_MAX_SIZE);

	bt_shell_ead_ad_len = 0;
	bt_shell_ead_data_size = 0;

	shell_info(sh, "Advertising data has been cleared.");

	return 0;
}

int ead_encrypt_ad(const uint8_t *payload, uint8_t payload_size, uint8_t *encrypted_payload)
{
	int err;

	err = bt_ead_encrypt(bt_shell_ead_session_key, bt_shell_ead_iv, payload, payload_size,
			     encrypted_payload);
	if (err != 0) {
		shell_error(ctx_shell, "Failed to encrypt AD.");
		return -1;
	}

	return 0;
}

int ead_update_ad(void)
{
	int err;
	size_t idx = 0;
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];

	struct bt_data *ad;
	size_t ad_structs_idx = 0;
	struct bt_data ad_structs[BT_SHELL_EAD_MAX_AD] = {0};

	size_t encrypted_data_buf_len = 0;
	uint8_t encrypted_data_buf[BT_SHELL_EAD_DATA_MAX_SIZE] = {0};

	while (idx < bt_shell_ead_data_size && ad_structs_idx < BT_SHELL_EAD_MAX_AD) {
		ad = &ad_structs[ad_structs_idx];

		/* the data_len from bt_data struct doesn't include the size of the type */
		ad->data_len = bt_shell_ead_data[idx] - 1;

		if (ad->data_len < 0) {
			/* if the len is less than 0 that mean there is not even a type field */
			shell_error(ctx_shell, "Failed to update AD due to malformed AD.");
			return -ENOEXEC;
		}

		ad->type = bt_shell_ead_data[idx + 1];

		if (ad->data_len > 0) {
			if (ad->type == BT_DATA_ENCRYPTED_AD_DATA) {
				/* for EAD the size used to store the non-encrypted data
				 * is the size of those data when encrypted
				 */
				ead_encrypt_ad(&bt_shell_ead_data[idx + 2],
					       BT_EAD_DECRYPTED_PAYLOAD_SIZE(ad->data_len),
					       &encrypted_data_buf[encrypted_data_buf_len]);

				ad->data = &encrypted_data_buf[encrypted_data_buf_len];
				encrypted_data_buf_len += ad->data_len;
			} else {
				ad->data = &bt_shell_ead_data[idx + 2];
			}
		}

		ad_structs_idx += 1;
		idx += ad->data_len + 2;
	}

	err = bt_le_ext_adv_set_data(adv, ad_structs, bt_shell_ead_ad_len, NULL, 0);
	if (err != 0) {
		shell_error(ctx_shell, "Failed to set advertising data (err %d)", err);
		return -ENOEXEC;
	}

	shell_info(ctx_shell, "Advertising data for Advertiser[%d] %p updated.", selected_adv, adv);

	return 0;
}

static int cmd_encrypted_ad_commit_ad(const struct shell *sh, size_t argc, char *argv[])
{
	return ead_update_ad();
}

static int cmd_encrypted_ad_decrypt_scan(const struct shell *sh, size_t argc, char *argv[])
{
	const char *action = argv[1];

	if (strcmp(action, "on") == 0) {
		bt_shell_ead_decrypt_scan = true;
		shell_info(sh, "Received encrypted advertising data will now be decrypted using "
			       "provided key materials.");
	} else if (strcmp(action, "off") == 0) {
		bt_shell_ead_decrypt_scan = false;
		shell_info(sh, "Received encrypted advertising data will now longer be decrypted.");
	} else {
		shell_error(sh, "Invalid option.");
		return -ENOEXEC;
	}

	return 0;
}
#endif

static int cmd_default_handler(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

#define HELP_NONE "[none]"
#define HELP_ONOFF "<on, off>"
#define HELP_ADDR "<address: XX:XX:XX:XX:XX:XX>"
#define HELP_ADDR_LE "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"

#if defined(CONFIG_BT_EXT_ADV)
#define EXT_ADV_SCAN_OPT " [coded] [no-1m]"
#define EXT_ADV_PARAM                                                                              \
	"<type: conn-scan conn-nscan, nconn-scan nconn-nscan> "                                    \
	"[ext-adv] [no-2m] [coded] [anon] [tx-power] [scan-reports] "                              \
	"[filter-accept-list: fal, fal-scan, fal-conn] [identity] "                                \
	"[directed " HELP_ADDR_LE "] [mode: low] [dir-rpa] "                                       \
	"[disable-37] [disable-38] [disable-39]"
#else
#define EXT_ADV_SCAN_OPT ""
#endif /* defined(CONFIG_BT_EXT_ADV) */

#if defined(CONFIG_BT_OBSERVER)
SHELL_STATIC_SUBCMD_SET_CREATE(bt_scan_filter_set_cmds,
	SHELL_CMD_ARG(name, NULL, "<name>", cmd_scan_filter_set_name, 2, 0),
	SHELL_CMD_ARG(addr, NULL, HELP_ADDR, cmd_scan_filter_set_addr, 2, 0),
	SHELL_CMD_ARG(rssi, NULL, "<rssi>", cmd_scan_filter_set_rssi, 2, 0),
	SHELL_CMD_ARG(pa_interval, NULL, "<pa_interval>",
		      cmd_scan_filter_set_pa_interval, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(bt_scan_filter_clear_cmds,
	SHELL_CMD_ARG(all, NULL, "", cmd_scan_filter_clear_all, 1, 0),
	SHELL_CMD_ARG(name, NULL, "", cmd_scan_filter_clear_name, 1, 0),
	SHELL_CMD_ARG(addr, NULL, "", cmd_scan_filter_clear_addr, 1, 0),
	SHELL_SUBCMD_SET_END
);
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_EAD)
SHELL_STATIC_SUBCMD_SET_CREATE(
	bt_encrypted_ad_cmds,
	SHELL_CMD_ARG(set-keys, NULL, "<session key> <init vector>", cmd_encrypted_ad_set_keys, 3,
		      0),
	SHELL_CMD_ARG(add-ead, NULL, "<advertising data>", cmd_encrypted_ad_add_ead, 2, 0),
	SHELL_CMD_ARG(add-ad, NULL, "<advertising data>", cmd_encrypted_ad_add_ad, 2, 0),
	SHELL_CMD(clear-ad, NULL, HELP_NONE, cmd_encrypted_ad_clear_ad),
	SHELL_CMD(commit-ad, NULL, HELP_NONE, cmd_encrypted_ad_commit_ad),
	SHELL_CMD_ARG(decrypt-scan, NULL, HELP_ONOFF, cmd_encrypted_ad_decrypt_scan, 2, 0),
	SHELL_SUBCMD_SET_END);
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(bt_cmds,
	SHELL_CMD_ARG(init, NULL, "[no-settings-load], [sync]",
		      cmd_init, 1, 2),
	SHELL_CMD_ARG(disable, NULL, HELP_NONE, cmd_disable, 1, 0),
#if defined(CONFIG_SETTINGS)
	SHELL_CMD_ARG(settings-load, NULL, HELP_NONE, cmd_settings_load, 1, 0),
#endif
#if defined(CONFIG_BT_HCI)
	SHELL_CMD_ARG(hci-cmd, NULL, "<ogf> <ocf> [data]", cmd_hci_cmd, 3, 1),
#endif
	SHELL_CMD_ARG(id-create, NULL, HELP_ADDR, cmd_id_create, 1, 1),
	SHELL_CMD_ARG(id-reset, NULL, "<id> "HELP_ADDR, cmd_id_reset, 2, 1),
	SHELL_CMD_ARG(id-delete, NULL, "<id>", cmd_id_delete, 2, 0),
	SHELL_CMD_ARG(id-show, NULL, HELP_NONE, cmd_id_show, 1, 0),
	SHELL_CMD_ARG(id-select, NULL, "<id>", cmd_id_select, 2, 0),
	SHELL_CMD_ARG(name, NULL, "[name]", cmd_name, 1, 1),
#if defined(CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC)
	SHELL_CMD_ARG(appearance, NULL, "[new appearance value]", cmd_appearance, 1, 1),
#else
	SHELL_CMD_ARG(appearance, NULL, HELP_NONE, cmd_appearance, 1, 0),
#endif /* CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC */
#if defined(CONFIG_BT_OBSERVER)
	SHELL_CMD_ARG(scan, NULL,
		      "<value: on, passive, off> [filter: dups, nodups] [fal]"
		      EXT_ADV_SCAN_OPT,
		      cmd_scan, 2, 4),
	SHELL_CMD(scan-filter-set, &bt_scan_filter_set_cmds,
		      "Scan filter set commands",
		      cmd_default_handler),
	SHELL_CMD(scan-filter-clear, &bt_scan_filter_clear_cmds,
		      "Scan filter clear commands",
		      cmd_default_handler),
	SHELL_CMD_ARG(scan-verbose-output, NULL, "<value: on, off>", cmd_scan_verbose_output, 2, 0),
#endif /* CONFIG_BT_OBSERVER */
#if defined(CONFIG_BT_TRANSMIT_POWER_CONTROL)
	SHELL_CMD_ARG(read-remote-tx-power, NULL, HELP_NONE, cmd_read_remote_tx_power, 2, 0),
	SHELL_CMD_ARG(read-local-tx-power, NULL, HELP_NONE, cmd_read_local_tx_power, 2, 0),
	SHELL_CMD_ARG(set-power-report-enable, NULL, HELP_NONE, cmd_set_power_report_enable, 3, 0),
#endif
#if defined(CONFIG_BT_BROADCASTER)
	SHELL_CMD_ARG(advertise, NULL,
		      "<type: off, on, nconn> [mode: discov, non_discov] "
		      "[filter-accept-list: fal, fal-scan, fal-conn] [identity] [no-name] "
		      "[one-time] [name-ad] [appearance] "
		      "[disable-37] [disable-38] [disable-39]",
		      cmd_advertise, 2, 8),
#if defined(CONFIG_BT_PERIPHERAL)
	SHELL_CMD_ARG(directed-adv, NULL, HELP_ADDR_LE " [mode: low] "
		      "[identity] [dir-rpa]",
		      cmd_directed_adv, 3, 6),
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_EXT_ADV)
	SHELL_CMD_ARG(adv-create, NULL, EXT_ADV_PARAM, cmd_adv_create, 2, 11),
	SHELL_CMD_ARG(adv-param, NULL, EXT_ADV_PARAM, cmd_adv_param, 2, 11),
	SHELL_CMD_ARG(adv-data, NULL, "<data> [scan-response <data>] "
				      "<type: discov, hex> [appearance] "
				      "[name <str>] [dev-name]",
		      cmd_adv_data, 1, 16),
	SHELL_CMD_ARG(adv-start, NULL,
		"[timeout <timeout>] [num-events <num events>]",
		cmd_adv_start, 1, 4),
	SHELL_CMD_ARG(adv-stop, NULL, HELP_NONE, cmd_adv_stop, 1, 0),
	SHELL_CMD_ARG(adv-delete, NULL, HELP_NONE, cmd_adv_delete, 1, 0),
	SHELL_CMD_ARG(adv-select, NULL, "[adv]", cmd_adv_select, 1, 1),
	SHELL_CMD_ARG(adv-info, NULL, HELP_NONE, cmd_adv_info, 1, 0),
#if defined(CONFIG_BT_PERIPHERAL)
	SHELL_CMD_ARG(adv-oob, NULL, HELP_NONE, cmd_adv_oob, 1, 0),
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_PRIVACY)
	SHELL_CMD_ARG(adv-rpa-expire, NULL, HELP_ONOFF, cmd_adv_rpa_expire, 2, 0),
#endif
#if defined(CONFIG_BT_PER_ADV)
	SHELL_CMD_ARG(per-adv, NULL, HELP_ONOFF, cmd_per_adv, 2, 0),
	SHELL_CMD_ARG(per-adv-param, NULL,
		      "[<interval-min> [<interval-max> [tx_power]]]",
		      cmd_per_adv_param, 1, 3),
	SHELL_CMD_ARG(per-adv-data, NULL, "[data]", cmd_per_adv_data, 1, 1),
#endif /* CONFIG_BT_PER_ADV */
#endif /* CONFIG_BT_EXT_ADV */
#endif /* CONFIG_BT_BROADCASTER */
#if defined(CONFIG_BT_PER_ADV_SYNC)
	SHELL_CMD_ARG(per-adv-sync-create, NULL,
		      HELP_ADDR_LE " <sid> [skip <count>] [timeout <ms>] [aoa] "
		      "[aod_1us] [aod_2us] [cte_only]",
		      cmd_per_adv_sync_create, 4, 6),
	SHELL_CMD_ARG(per-adv-sync-delete, NULL, "[<index>]",
		      cmd_per_adv_sync_delete, 1, 1),
	SHELL_CMD_ARG(per-adv-sync-select, NULL, "[adv]", cmd_per_adv_sync_select, 1, 1),
#endif /* defined(CONFIG_BT_PER_ADV_SYNC) */
#if defined(CONFIG_BT_EAD)
	SHELL_CMD(encrypted-ad, &bt_encrypted_ad_cmds, "Manage advertiser with encrypted data",
		  cmd_default_handler),
#endif /* CONFIG_BT_EAD */
#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER)
	SHELL_CMD_ARG(past-subscribe, NULL, "[conn] [skip <count>] "
		      "[timeout <ms>] [aoa] [aod_1us] [aod_2us] [cte_only]",
		      cmd_past_subscribe, 1, 7),
	SHELL_CMD_ARG(past-unsubscribe, NULL, "[conn]",
		      cmd_past_unsubscribe, 1, 1),
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER */
#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)
#if defined(CONFIG_BT_PER_ADV_SYNC)
	SHELL_CMD_ARG(per-adv-sync-transfer, NULL, "[<index>]",
		      cmd_per_adv_sync_transfer, 1, 1),
#endif /* CONFIG_BT_PER_ADV_SYNC */
#if defined(CONFIG_BT_PER_ADV)
	SHELL_CMD_ARG(per-adv-set-info-transfer, NULL, "",
		      cmd_per_adv_set_info_transfer, 1, 0),
#endif /* CONFIG_BT_PER_ADV */
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER */
#if defined(CONFIG_BT_CENTRAL)
	SHELL_CMD_ARG(connect, NULL, HELP_ADDR_LE EXT_ADV_SCAN_OPT,
		      cmd_connect_le, 1, 3),
#if !defined(CONFIG_BT_FILTER_ACCEPT_LIST)
	SHELL_CMD_ARG(auto-conn, NULL, HELP_ADDR_LE, cmd_auto_conn, 3, 0),
#endif /* !defined(CONFIG_BT_FILTER_ACCEPT_LIST) */
	SHELL_CMD_ARG(connect-name, NULL, "<name filter>",
		      cmd_connect_le_name, 2, 0),
#endif /* CONFIG_BT_CENTRAL */
	SHELL_CMD_ARG(disconnect, NULL, HELP_ADDR_LE, cmd_disconnect, 1, 2),
	SHELL_CMD_ARG(select, NULL, HELP_ADDR_LE, cmd_select, 3, 0),
	SHELL_CMD_ARG(info, NULL, HELP_ADDR_LE, cmd_info, 1, 2),
	SHELL_CMD_ARG(conn-update, NULL, "<min> <max> <latency> <timeout>",
		      cmd_conn_update, 5, 0),
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	SHELL_CMD_ARG(data-len-update, NULL, "<tx_max_len> [tx_max_time]",
		      cmd_conn_data_len_update, 2, 1),
#endif
#if defined(CONFIG_BT_USER_PHY_UPDATE)
	SHELL_CMD_ARG(phy-update, NULL, "<tx_phy> [rx_phy] [s2] [s8]",
		      cmd_conn_phy_update, 2, 3),
#endif
#if defined(CONFIG_BT_CENTRAL) || defined(CONFIG_BT_BROADCASTER)
	SHELL_CMD_ARG(channel-map, NULL, "<channel-map: XXXXXXXXXX> (36-0)",
		      cmd_chan_map, 2, 1),
#endif /* CONFIG_BT_CENTRAL */
	SHELL_CMD_ARG(oob, NULL, HELP_NONE, cmd_oob, 1, 0),
	SHELL_CMD_ARG(clear, NULL, "[all] ["HELP_ADDR_LE"]", cmd_clear, 2, 1),
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
	SHELL_CMD_ARG(security, NULL, "<security level BR/EDR: 0 - 3, "
				      "LE: 1 - 4> [force-pair]",
		      cmd_security, 1, 2),
	SHELL_CMD_ARG(bondable, NULL, HELP_ONOFF, cmd_bondable,
		      2, 0),
	SHELL_CMD_ARG(bonds, NULL, HELP_NONE, cmd_bonds, 1, 0),
	SHELL_CMD_ARG(connections, NULL, HELP_NONE, cmd_connections, 1, 0),
	SHELL_CMD_ARG(auth, NULL,
		      "<method: all, input, display, yesno, confirm, "
		      "oob, status, none>",
		      cmd_auth, 2, 0),
	SHELL_CMD_ARG(auth-cancel, NULL, HELP_NONE, cmd_auth_cancel, 1, 0),
	SHELL_CMD_ARG(auth-passkey, NULL, "<passkey>", cmd_auth_passkey, 2, 0),
#if defined(CONFIG_BT_PASSKEY_KEYPRESS)
	SHELL_CMD_ARG(auth-passkey-notify, NULL, "<type>",
		      cmd_auth_passkey_notify, 2, 0),
#endif /* CONFIG_BT_PASSKEY_KEYPRESS */
	SHELL_CMD_ARG(auth-passkey-confirm, NULL, HELP_NONE,
		      cmd_auth_passkey_confirm, 1, 0),
	SHELL_CMD_ARG(auth-pairing-confirm, NULL, HELP_NONE,
		      cmd_auth_pairing_confirm, 1, 0),
#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	SHELL_CMD_ARG(auth-oob-tk, NULL, "<tk>", cmd_auth_oob_tk, 2, 0),
#endif /* !defined(CONFIG_BT_SMP_SC_PAIR_ONLY) */
	SHELL_CMD_ARG(oob-remote, NULL,
		      HELP_ADDR_LE" <oob rand> <oob confirm>",
		      cmd_oob_remote, 3, 2),
	SHELL_CMD_ARG(oob-clear, NULL, HELP_NONE, cmd_oob_clear, 1, 0),
#if defined(CONFIG_BT_FILTER_ACCEPT_LIST)
	SHELL_CMD_ARG(fal-add, NULL, HELP_ADDR_LE, cmd_fal_add, 3, 0),
	SHELL_CMD_ARG(fal-rem, NULL, HELP_ADDR_LE, cmd_fal_rem, 3, 0),
	SHELL_CMD_ARG(fal-clear, NULL, HELP_NONE, cmd_fal_clear, 1, 0),

#if defined(CONFIG_BT_CENTRAL)
	SHELL_CMD_ARG(fal-connect, NULL, HELP_ONOFF EXT_ADV_SCAN_OPT,
		      cmd_fal_connect, 2, 3),
#endif /* CONFIG_BT_CENTRAL */
#endif /* defined(CONFIG_BT_FILTER_ACCEPT_LIST) */
#if defined(CONFIG_BT_FIXED_PASSKEY)
	SHELL_CMD_ARG(fixed-passkey, NULL, "[passkey]", cmd_fixed_passkey,
		      1, 1),
#endif
#endif /* CONFIG_BT_SMP || CONFIG_BT_CLASSIC) */
#endif /* CONFIG_BT_CONN */
#if defined(CONFIG_BT_HCI_MESH_EXT)
	SHELL_CMD(mesh_adv, NULL, HELP_ONOFF, cmd_mesh_adv),
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_LL_SW_SPLIT)
	SHELL_CMD(ll-addr, NULL, "<random|public>", cmd_ll_addr_read),
#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_BROADCASTER)
	SHELL_CMD_ARG(advx, NULL,
		      "<on hdcd ldcd off> [coded] [anon] [txp] [ad]",
		      cmd_advx, 2, 4),
#endif /* CONFIG_BT_BROADCASTER */
#if defined(CONFIG_BT_OBSERVER)
	SHELL_CMD_ARG(scanx, NULL, "<on passive off> [coded]", cmd_scanx,
		      2, 1),
#endif /* CONFIG_BT_OBSERVER */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#if defined(CONFIG_BT_CTLR_DTM)
	SHELL_CMD_ARG(test_tx, NULL, "<chan> <len> <type> <phy>", cmd_test_tx,
		      5, 0),
	SHELL_CMD_ARG(test_rx, NULL, "<chan> <phy> <mod_idx>", cmd_test_rx,
		      4, 0),
	SHELL_CMD_ARG(test_end, NULL, HELP_NONE, cmd_test_end, 1, 0),
#endif /* CONFIG_BT_CTLR_DTM */
#endif /* CONFIG_BT_LL_SW_SPLIT */

	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(bt, &bt_cmds, "Bluetooth shell commands", cmd_default_handler);
