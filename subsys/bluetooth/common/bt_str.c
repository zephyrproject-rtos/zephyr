/* Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/* Helper for printk parameters to convert from binary to hex.
 * We declare multiple buffers so the helper can be used multiple times
 * in a single printk call.
 */

#include <stddef.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

const char *bt_hex(const void *buf, size_t len)
{
	static const char hex[] = "0123456789abcdef";
	static char str[129];
	const uint8_t *b = buf;
	size_t i;

	len = MIN(len, (sizeof(str) - 1) / 2);

	for (i = 0; i < len; i++) {
		str[i * 2] = hex[b[i] >> 4];
		str[i * 2 + 1] = hex[b[i] & 0xf];
	}

	str[i * 2] = '\0';

	return str;
}

const char *bt_addr_str(const bt_addr_t *addr)
{
	static char str[BT_ADDR_STR_LEN];

	bt_addr_to_str(addr, str, sizeof(str));

	return str;
}

const char *bt_addr_le_str(const bt_addr_le_t *addr)
{
	static char str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, str, sizeof(str));

	return str;
}

const char *bt_uuid_str(const struct bt_uuid *uuid)
{
	static char str[BT_UUID_STR_LEN];

	bt_uuid_to_str(uuid, str, sizeof(str));

	return str;
}

#if defined(CONFIG_BT_HCI_ERR_TO_STR)
const char *bt_hci_err_to_str(uint8_t hci_err)
{
	#define HCI_ERR(err) [err] = #err

	const char * const mapping_table[] = {
		HCI_ERR(BT_HCI_ERR_SUCCESS),
		HCI_ERR(BT_HCI_ERR_UNKNOWN_CMD),
		HCI_ERR(BT_HCI_ERR_UNKNOWN_CONN_ID),
		HCI_ERR(BT_HCI_ERR_HW_FAILURE),
		HCI_ERR(BT_HCI_ERR_PAGE_TIMEOUT),
		HCI_ERR(BT_HCI_ERR_AUTH_FAIL),
		HCI_ERR(BT_HCI_ERR_PIN_OR_KEY_MISSING),
		HCI_ERR(BT_HCI_ERR_MEM_CAPACITY_EXCEEDED),
		HCI_ERR(BT_HCI_ERR_CONN_TIMEOUT),
		HCI_ERR(BT_HCI_ERR_CONN_LIMIT_EXCEEDED),
		HCI_ERR(BT_HCI_ERR_SYNC_CONN_LIMIT_EXCEEDED),
		HCI_ERR(BT_HCI_ERR_CONN_ALREADY_EXISTS),
		HCI_ERR(BT_HCI_ERR_CMD_DISALLOWED),
		HCI_ERR(BT_HCI_ERR_INSUFFICIENT_RESOURCES),
		HCI_ERR(BT_HCI_ERR_INSUFFICIENT_SECURITY),
		HCI_ERR(BT_HCI_ERR_BD_ADDR_UNACCEPTABLE),
		HCI_ERR(BT_HCI_ERR_CONN_ACCEPT_TIMEOUT),
		HCI_ERR(BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL),
		HCI_ERR(BT_HCI_ERR_INVALID_PARAM),
		HCI_ERR(BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		HCI_ERR(BT_HCI_ERR_REMOTE_LOW_RESOURCES),
		HCI_ERR(BT_HCI_ERR_REMOTE_POWER_OFF),
		HCI_ERR(BT_HCI_ERR_LOCALHOST_TERM_CONN),
		HCI_ERR(BT_HCI_ERR_REPEATED_ATTEMPTS),
		HCI_ERR(BT_HCI_ERR_PAIRING_NOT_ALLOWED),
		HCI_ERR(BT_HCI_ERR_UNKNOWN_LMP_PDU),
		HCI_ERR(BT_HCI_ERR_UNSUPP_REMOTE_FEATURE),
		HCI_ERR(BT_HCI_ERR_SCO_OFFSET_REJECTED),
		HCI_ERR(BT_HCI_ERR_SCO_INTERVAL_REJECTED),
		HCI_ERR(BT_HCI_ERR_SCO_AIR_MODE_REJECTED),
		HCI_ERR(BT_HCI_ERR_INVALID_LL_PARAM),
		HCI_ERR(BT_HCI_ERR_UNSPECIFIED),
		HCI_ERR(BT_HCI_ERR_UNSUPP_LL_PARAM_VAL),
		HCI_ERR(BT_HCI_ERR_ROLE_CHANGE_NOT_ALLOWED),
		HCI_ERR(BT_HCI_ERR_LL_RESP_TIMEOUT),
		HCI_ERR(BT_HCI_ERR_LL_PROC_COLLISION),
		HCI_ERR(BT_HCI_ERR_LMP_PDU_NOT_ALLOWED),
		HCI_ERR(BT_HCI_ERR_ENC_MODE_NOT_ACCEPTABLE),
		HCI_ERR(BT_HCI_ERR_LINK_KEY_CANNOT_BE_CHANGED),
		HCI_ERR(BT_HCI_ERR_REQUESTED_QOS_NOT_SUPPORTED),
		HCI_ERR(BT_HCI_ERR_INSTANT_PASSED),
		HCI_ERR(BT_HCI_ERR_PAIRING_NOT_SUPPORTED),
		HCI_ERR(BT_HCI_ERR_DIFF_TRANS_COLLISION),
		HCI_ERR(BT_HCI_ERR_QOS_UNACCEPTABLE_PARAM),
		HCI_ERR(BT_HCI_ERR_QOS_REJECTED),
		HCI_ERR(BT_HCI_ERR_CHAN_ASSESS_NOT_SUPPORTED),
		HCI_ERR(BT_HCI_ERR_INSUFF_SECURITY),
		HCI_ERR(BT_HCI_ERR_PARAM_OUT_OF_MANDATORY_RANGE),
		HCI_ERR(BT_HCI_ERR_ROLE_SWITCH_PENDING),
		HCI_ERR(BT_HCI_ERR_RESERVED_SLOT_VIOLATION),
		HCI_ERR(BT_HCI_ERR_ROLE_SWITCH_FAILED),
		HCI_ERR(BT_HCI_ERR_EXT_INQ_RESP_TOO_LARGE),
		HCI_ERR(BT_HCI_ERR_SIMPLE_PAIR_NOT_SUPP_BY_HOST),
		HCI_ERR(BT_HCI_ERR_HOST_BUSY_PAIRING),
		HCI_ERR(BT_HCI_ERR_CONN_REJECTED_DUE_TO_NO_CHAN),
		HCI_ERR(BT_HCI_ERR_CONTROLLER_BUSY),
		HCI_ERR(BT_HCI_ERR_UNACCEPT_CONN_PARAM),
		HCI_ERR(BT_HCI_ERR_ADV_TIMEOUT),
		HCI_ERR(BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL),
		HCI_ERR(BT_HCI_ERR_CONN_FAIL_TO_ESTAB),
		HCI_ERR(BT_HCI_ERR_MAC_CONN_FAILED),
		HCI_ERR(BT_HCI_ERR_CLOCK_ADJUST_REJECTED),
		HCI_ERR(BT_HCI_ERR_SUBMAP_NOT_DEFINED),
		HCI_ERR(BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER),
		HCI_ERR(BT_HCI_ERR_LIMIT_REACHED),
		HCI_ERR(BT_HCI_ERR_OP_CANCELLED_BY_HOST),
		HCI_ERR(BT_HCI_ERR_PACKET_TOO_LONG),
		HCI_ERR(BT_HCI_ERR_TOO_LATE),
		HCI_ERR(BT_HCI_ERR_TOO_EARLY),
	};

	if (hci_err < ARRAY_SIZE(mapping_table) && mapping_table[hci_err]) {
		return mapping_table[hci_err];
	} else {
		return "(unknown)";
	}

	#undef HCI_ERR
}
#endif /* CONFIG_BT_HCI_ERR_TO_STR */

#if defined(CONFIG_BT_HCI_OPCODE_TO_STR)
#define BT_OPCODE_STR(opcode)                                                         \
    (opcode == BT_HCI_OP_INQUIRY                       ? "INQUIRY"                    \
            : opcode == BT_HCI_OP_INQUIRY_CANCEL       ? "INQUIRY_CANCEL"             \
            : opcode == BT_HCI_OP_CONNECT              ? "CONNECT"                    \
            : opcode == BT_HCI_OP_DISCONNECT           ? "DISCONNECT"                 \
            : opcode == BT_HCI_OP_CONNECT_CANCEL       ? "CONNECT_CANCEL"             \
            : opcode == BT_HCI_OP_ACCEPT_CONN_REQ      ? "ACCEPT_CONN_REQ"            \
            : opcode == BT_HCI_OP_SETUP_SYNC_CONN      ? "SETUP_SYNC_CONN"            \
            : opcode == BT_HCI_OP_ACCEPT_SYNC_CONN_REQ ? "ACCEPT_SYNC_CONN_REQ"       \
            : opcode == BT_HCI_OP_REJECT_CONN_REQ      ? "REJECT_CONN_REQ"            \
            : opcode == BT_HCI_OP_LINK_KEY_REPLY       ? "LINK_KEY_REPLY"             \
            : opcode == BT_HCI_OP_LINK_KEY_NEG_REPLY   ? "LINK_KEY_NEG_REPLY"         \
            : opcode == BT_HCI_OP_PIN_CODE_REPLY       ? "PIN_CODE_REPLY"             \
            : opcode == BT_HCI_OP_PIN_CODE_NEG_REPLY   ? "PIN_CODE_NEG_REPLY"         \
            : opcode == BT_HCI_OP_AUTH_REQUESTED       ? "AUTH_REQUESTED"             \
            : opcode == BT_HCI_OP_SET_CONN_ENCRYPT     ? "SET_CONN_ENCRYPT"           \
            : opcode == BT_HCI_OP_REMOTE_NAME_REQUEST  ? "REMOTE_NAME_REQUEST"        \
            : opcode == BT_HCI_OP_REMOTE_NAME_CANCEL   ? "REMOTE_NAME_CANCEL"         \
            : opcode == BT_HCI_OP_READ_REMOTE_FEATURES ? "READ_REMOTE_FEATURES"       \
            : opcode == BT_HCI_OP_READ_REMOTE_EXT_FEATURES                            \
            ? "READ_REMOTE_EXT_FEATURES"                                              \
            : opcode == BT_HCI_OP_READ_REMOTE_VERSION_INFO                            \
            ? "READ_REMOTE_VERSION_INFO"                                              \
            : opcode == BT_HCI_OP_IO_CAPABILITY_REPLY     ? "IO_CAPABILITY_REPLY"     \
            : opcode == BT_HCI_OP_USER_CONFIRM_REPLY      ? "USER_CONFIRM_REPLY"      \
            : opcode == BT_HCI_OP_USER_CONFIRM_NEG_REPLY  ? "USER_CONFIRM_NEG_REPLY"  \
            : opcode == BT_HCI_OP_USER_PASSKEY_REPLY      ? "USER_PASSKEY_REPLY"      \
            : opcode == BT_HCI_OP_USER_PASSKEY_NEG_REPLY  ? "USER_PASSKEY_NEG_REPLY"  \
            : opcode == BT_HCI_OP_IO_CAPABILITY_NEG_REPLY ? "IO_CAPABILITY_NEG_REPLY" \
            : opcode == BT_HCI_OP_SNIFF_MODE              ? "SNIFF_MODE"              \
            : opcode == BT_HCI_OP_EXIT_SNIFF_MODE         ? "EXIT_SNIFF_MODE"         \
            : opcode == BT_HCI_OP_ROLE_DISCOVERY          ? "ROLE_DISCOVERY"          \
            : opcode == BT_HCI_OP_SWITCH_ROLE             ? "SWITCH_ROLE"             \
            : opcode == BT_HCI_OP_READ_LINK_POLICY_SETTINGS                           \
            ? "READ_LINK_POLICY_SETTINGS"                                             \
            : opcode == BT_HCI_OP_WRITE_LINK_POLICY_SETTINGS                          \
            ? "WRITE_LINK_POLICY_SETTINGS"                                            \
            : opcode == BT_HCI_OP_SET_EVENT_MASK         ? "SET_EVENT_MASK"           \
            : opcode == BT_HCI_OP_RESET                  ? "RESET"                    \
            : opcode == BT_HCI_OP_DELETE_STORED_LINK_KEY ? "DELETE_STORED_LINK_KEY"   \
            : opcode == BT_HCI_OP_WRITE_LOCAL_NAME       ? "WRITE_LOCAL_NAME"         \
            : opcode == BT_HCI_OP_READ_CONN_ACCEPT_TIMEOUT                            \
            ? "READ_CONN_ACCEPT_TIMEOUT"                                              \
            : opcode == BT_HCI_OP_WRITE_CONN_ACCEPT_TIMEOUT                           \
            ? "WRITE_CONN_ACCEPT_TIMEOUT"                                             \
            : opcode == BT_HCI_OP_WRITE_PAGE_TIMEOUT ? "WRITE_PAGE_TIMEOUT"           \
            : opcode == BT_HCI_OP_WRITE_SCAN_ENABLE  ? "WRITE_SCAN_ENABLE"            \
            : opcode == BT_HCI_OP_WRITE_PAGE_SCAN_ACTIVITY                            \
            ? "WRITE_PAGE_SCAN_ACTIVITY"                                              \
            : opcode == BT_HCI_OP_WRITE_INQUIRY_SCAN_ACTIVITY                         \
            ? "WRITE_INQUIRY_SCAN_ACTIVITY"                                           \
            : opcode == BT_HCI_OP_WRITE_CLASS_OF_DEVICE ? "WRITE_CLASS_OF_DEVICE"     \
            : opcode == BT_HCI_OP_READ_TX_POWER_LEVEL   ? "READ_TX_POWER_LEVEL"       \
            : opcode == BT_HCI_OP_READ_EXTENDED_INQUIRY_RESPONSE                      \
            ? "READ_EXTENDED_INQUIRY_RESPONSE"                                        \
            : opcode == BT_HCI_OP_WRITE_EXTENDED_INQUIRY_RESPONSE                     \
            ? "WRITE_EXTENDED_INQUIRY_RESPONSE"                                       \
            : opcode == BT_HCI_OP_LE_ENH_READ_TX_POWER_LEVEL                          \
            ? "LE_ENH_READ_TX_POWER_LEVEL"                                            \
            : opcode == BT_HCI_OP_LE_READ_REMOTE_TX_POWER_LEVEL                       \
            ? "LE_READ_REMOTE_TX_POWER_LEVEL"                                         \
            : opcode == BT_HCI_OP_LE_SET_TX_POWER_REPORT_ENABLE                       \
            ? "LE_SET_TX_POWER_REPORT_ENABLE"                                         \
            : opcode == BT_HCI_OP_LE_SET_PATH_LOSS_REPORTING_PARAMETERS               \
            ? "LE_SET_PATH_LOSS_REPORTING_PARAMETERS"                                 \
            : opcode == BT_HCI_OP_LE_SET_PATH_LOSS_REPORTING_ENABLE                   \
            ? "LE_SET_PATH_LOSS_REPORTING_ENABLE"                                     \
            : opcode == BT_HCI_OP_LE_SET_DEFAULT_SUBRATE ? "LE_SET_DEFAULT_SUBRATE"   \
            : opcode == BT_HCI_OP_LE_SUBRATE_REQUEST     ? "LE_SUBRATE_REQUEST"       \
            : opcode == BT_HCI_OP_SET_CTL_TO_HOST_FLOW   ? "SET_CTL_TO_HOST_FLOW"     \
            : opcode == BT_HCI_OP_HOST_BUFFER_SIZE       ? "HOST_BUFFER_SIZE"         \
            : opcode == BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS                          \
            ? "HOST_NUM_COMPLETED_PACKETS"                                            \
            : opcode == BT_HCI_OP_WRITE_LINK_SUPERVISION_TIMEOUT                      \
            ? "WRITE_LINK_SUPERVISION_TIMEOUT"                                        \
            : opcode == BT_HCI_OP_WRITE_INQUIRY_SCAN_TYPE ? "WRITE_INQUIRY_SCAN_TYPE" \
            : opcode == BT_HCI_OP_WRITE_INQUIRY_MODE      ? "WRITE_INQUIRY_MODE"      \
            : opcode == BT_HCI_OP_WRITE_PAGE_SCAN_TYPE    ? "WRITE_PAGE_SCAN_TYPE"    \
            : opcode == BT_HCI_OP_WRITE_SSP_MODE          ? "WRITE_SSP_MODE"          \
            : opcode == BT_HCI_OP_SET_EVENT_MASK_PAGE_2   ? "SET_EVENT_MASK_PAGE_2"   \
            : opcode == BT_HCI_OP_LE_WRITE_LE_HOST_SUPP   ? "LE_WRITE_LE_HOST_SUPP "  \
            : opcode == BT_HCI_OP_WRITE_SC_HOST_SUPP      ? "WRITE_SC_HOST_SUPP"      \
            : opcode == BT_HCI_OP_READ_AUTH_PAYLOAD_TIMEOUT                           \
            ? "READ_AUTH_PAYLOAD_TIMEOUT"                                             \
            : opcode == BT_HCI_OP_WRITE_AUTH_PAYLOAD_TIMEOUT                          \
            ? "WRITE_AUTH_PAYLOAD_TIMEOUT"                                            \
            : opcode == BT_HCI_OP_CONFIGURE_DATA_PATH     ? "CONFIGURE_DATA_PATH"     \
            : opcode == BT_HCI_OP_READ_LOCAL_VERSION_INFO ? "READ_LOCAL_VERSION_INFO" \
            : opcode == BT_HCI_OP_READ_SUPPORTED_COMMANDS ? "READ_SUPPORTED_COMMANDS" \
            : opcode == BT_HCI_OP_READ_LOCAL_EXT_FEATURES ? "READ_LOCAL_EXT_FEATURES" \
            : opcode == BT_HCI_OP_READ_LOCAL_FEATURES     ? "READ_LOCAL_FEATURES"     \
            : opcode == BT_HCI_OP_READ_BUFFER_SIZE        ? "READ_BUFFER_SIZE"        \
            : opcode == BT_HCI_OP_READ_BD_ADDR            ? "READ_BD_ADDR"            \
            : opcode == BT_HCI_OP_READ_CODECS             ? "READ_CODECS"             \
            : opcode == BT_HCI_OP_READ_CODECS_V2          ? "READ_CODECS_V2"          \
            : opcode == BT_HCI_OP_READ_CODEC_CAPABILITIES ? "READ_CODEC_CAPABILITIES" \
            : opcode == BT_HCI_OP_READ_CTLR_DELAY         ? "READ_CTLR_DELAY"         \
            : opcode == BT_HCI_OP_READ_RSSI               ? "READ_RSSI"               \
            : opcode == BT_HCI_OP_READ_ENCRYPTION_KEY_SIZE                            \
            ? "READ_ENCRYPTION_KEY_SIZE"                                              \
            : opcode == BT_HCI_OP_LE_SET_EVENT_MASK      ? "LE_SET_EVENT_MASK"        \
            : opcode == BT_HCI_OP_LE_READ_BUFFER_SIZE    ? "LE_READ_BUFFER_SIZE"      \
            : opcode == BT_HCI_OP_LE_READ_LOCAL_FEATURES ? "LE_READ_LOCAL_FEATURES"   \
            : opcode == BT_HCI_OP_LE_SET_RANDOM_ADDRESS  ? "LE_SET_RANDOM_ADDRESS"    \
            : opcode == BT_HCI_OP_LE_SET_ADV_PARAM       ? "LE_SET_ADV_PARAM "        \
            : opcode == BT_HCI_OP_LE_READ_ADV_CHAN_TX_POWER                           \
            ? "LE_READ_ADV_CHAN_TX_POWER"                                             \
            : opcode == BT_HCI_OP_LE_SET_ADV_DATA       ? "LE_SET_ADV_DATA"           \
            : opcode == BT_HCI_OP_LE_SET_SCAN_RSP_DATA  ? "LE_SET_SCAN_RSP_DATA"      \
            : opcode == BT_HCI_OP_LE_SET_ADV_ENABLE     ? "LE_SET_ADV_ENABLE"         \
            : opcode == BT_HCI_OP_LE_SET_SCAN_PARAM     ? "LE_SET_SCAN_PARAM"         \
            : opcode == BT_HCI_OP_LE_SET_SCAN_ENABLE    ? "LE_SET_SCAN_ENABLE"        \
            : opcode == BT_HCI_OP_LE_CREATE_CONN        ? "LE_CREATE_CONN"            \
            : opcode == BT_HCI_OP_LE_CREATE_CONN_CANCEL ? "LE_CREATE_CONN_CANCEL"     \
            : opcode == BT_HCI_OP_LE_READ_FAL_SIZE      ? "LE_READ_FAL_SIZE"          \
            : opcode == BT_HCI_OP_LE_CLEAR_FAL          ? "LE_CLEAR_FAL"              \
            : opcode == BT_HCI_OP_LE_ADD_DEV_TO_FAL     ? "LE_ADD_DEV_TO_FAL"         \
            : opcode == BT_HCI_OP_LE_REM_DEV_FROM_FAL   ? "LE_REM_DEV_FROM_FAL"       \
            : opcode == BT_HCI_OP_LE_CONN_UPDATE        ? "LE_CONN_UPDATE"            \
            : opcode == BT_HCI_OP_LE_SET_HOST_CHAN_CLASSIF                            \
            ? "LE_SET_HOST_CHAN_CLASSIF"                                              \
            : opcode == BT_HCI_OP_LE_READ_CHAN_MAP        ? "LE_READ_CHAN_MAP"        \
            : opcode == BT_HCI_OP_LE_READ_REMOTE_FEATURES ? "LE_READ_REMOTE_FEATURES" \
            : opcode == BT_HCI_OP_LE_ENCRYPT              ? "LE_ENCRYPTESET"          \
            : opcode == BT_HCI_OP_LE_RAND                 ? "LE_RAND"                 \
            : opcode == BT_HCI_OP_LE_START_ENCRYPTION     ? "LE_START_ENCRYPTION"     \
            : opcode == BT_HCI_OP_LE_LTK_REQ_REPLY        ? "LE_LTK_REQ_REPLY"        \
            : opcode == BT_HCI_OP_LE_LTK_REQ_NEG_REPLY    ? "LE_LTK_REQ_NEG_REPLY"    \
            : opcode == BT_HCI_OP_LE_READ_SUPP_STATES     ? "LE_READ_SUPP_STATES"     \
            : opcode == BT_HCI_OP_LE_RX_TEST              ? "LE_RX_TEST"              \
            : opcode == BT_HCI_OP_LE_TX_TEST              ? "LE_TX_TEST"              \
            : opcode == BT_HCI_OP_LE_TEST_END             ? "LE_TEST_END"             \
            : opcode == BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY ? "LE_CONN_PARAM_REQ_REPLY" \
            : opcode == BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY                         \
            ? "LE_CONN_PARAM_REQ_NEG_REPLY"                                           \
            : opcode == BT_HCI_OP_LE_SET_DATA_LEN ? "LE_SET_DATA_LEN"                 \
            : opcode == BT_HCI_OP_LE_READ_DEFAULT_DATA_LEN                            \
            ? "LE_READ_DEFAULT_DATA_LEN"                                              \
            : opcode == BT_HCI_OP_LE_WRITE_DEFAULT_DATA_LEN                           \
            ? "LE_WRITE_DEFAULT_DATA_LEN"                                             \
            : opcode == BT_HCI_OP_LE_P256_PUBLIC_KEY     ? "LE_P256_PUBLIC_KEY"       \
            : opcode == BT_HCI_OP_LE_GENERATE_DHKEY      ? "LE_GENERATE_DHKEY"        \
            : opcode == BT_HCI_OP_LE_GENERATE_DHKEY_V2   ? "LE_GENERATE_DHKEY_V2"     \
            : opcode == BT_HCI_OP_LE_ADD_DEV_TO_RL       ? "LE_ADD_DEV_TO_RL"         \
            : opcode == BT_HCI_OP_LE_REM_DEV_FROM_RL     ? "LE_REM_DEV_FROM_RL"       \
            : opcode == BT_HCI_OP_LE_CLEAR_RL            ? "LE_CLEAR_RL"              \
            : opcode == BT_HCI_OP_LE_READ_RL_SIZE        ? "LE_READ_RL_SIZE"          \
            : opcode == BT_HCI_OP_LE_READ_PEER_RPA       ? "LE_READ_PEER_RPA"         \
            : opcode == BT_HCI_OP_LE_READ_LOCAL_RPA      ? "LE_READ_LOCAL_RPA"        \
            : opcode == BT_HCI_OP_LE_SET_ADDR_RES_ENABLE ? "LE_SET_ADDR_RES_ENABLE"   \
            : opcode == BT_HCI_OP_LE_SET_RPA_TIMEOUT     ? "LE_SET_RPA_TIMEOUT"       \
            : opcode == BT_HCI_OP_LE_READ_MAX_DATA_LEN   ? "LE_READ_MAX_DATA_LEN"     \
            : opcode == BT_HCI_OP_LE_READ_PHY            ? "LE_READ_PHY"              \
            : opcode == BT_HCI_OP_LE_SET_DEFAULT_PHY     ? "LE_SET_DEFAULT_PHY"       \
            : opcode == BT_HCI_OP_LE_SET_PHY             ? "LE_SET_PHY"               \
            : opcode == BT_HCI_OP_LE_ENH_RX_TEST         ? "LE_ENH_RX_TEST"           \
            : opcode == BT_HCI_OP_LE_ENH_TX_TEST         ? "LE_ENH_TX_TEST"           \
            : opcode == BT_HCI_OP_LE_SET_ADV_SET_RANDOM_ADDR                          \
            ? "LE_SET_ADV_SET_RANDOM_ADDR"                                            \
            : opcode == BT_HCI_OP_LE_SET_EXT_ADV_PARAM ? "LE_SET_EXT_ADV_PARAM"       \
            : opcode == BT_HCI_OP_LE_SET_EXT_ADV_DATA  ? "LE_SET_EXT_ADV_DATA"        \
            : opcode == BT_HCI_OP_LE_SET_EXT_SCAN_RSP_DATA                            \
            ? "LE_SET_EXT_SCAN_RSP_DATA"                                              \
            : opcode == BT_HCI_OP_LE_SET_EXT_ADV_ENABLE ? "LE_SET_EXT_ADV_ENABLE"     \
            : opcode == BT_HCI_OP_LE_READ_MAX_ADV_DATA_LEN                            \
            ? "LE_READ_MAX_ADV_DATA_LEN"                                              \
            : opcode == BT_HCI_OP_LE_READ_NUM_ADV_SETS   ? "LE_READ_NUM_ADV_SETS"     \
            : opcode == BT_HCI_OP_LE_REMOVE_ADV_SET      ? "LE_REMOVE_ADV_SET"        \
            : opcode == BT_HCI_OP_CLEAR_ADV_SETS         ? "CLEAR_ADV_SETS"           \
            : opcode == BT_HCI_OP_LE_SET_PER_ADV_PARAM   ? "LE_SET_PER_ADV_PARAM"     \
            : opcode == BT_HCI_OP_LE_SET_PER_ADV_DATA    ? "LE_SET_PER_ADV_DATA"      \
            : opcode == BT_HCI_OP_LE_SET_PER_ADV_ENABLE  ? "LE_SET_PER_ADV_ENABLE"    \
            : opcode == BT_HCI_OP_LE_SET_EXT_SCAN_PARAM  ? "LE_SET_EXT_SCAN_PARAM"    \
            : opcode == BT_HCI_OP_LE_SET_EXT_SCAN_ENABLE ? "LE_SET_EXT_SCAN_ENABLE "  \
            : opcode == BT_HCI_OP_LE_EXT_CREATE_CONN     ? "LE_EXT_CREATE_CONN"       \
            : opcode == BT_HCI_OP_LE_EXT_CREATE_CONN_V2  ? "LE_EXT_CREATE_CONN_V2"    \
            : opcode == BT_HCI_OP_LE_SET_PER_ADV_SUBEVENT_DATA                        \
            ? "LE_SET_PER_ADV_SUBEVENT_DATA"                                          \
            : opcode == BT_HCI_OP_LE_SET_PER_ADV_RESPONSE_DATA                        \
            ? "LE_SET_PER_ADV_RESPONSE_DATA"                                          \
            : opcode == BT_HCI_OP_LE_SET_PER_ADV_SYNC_SUBEVENT                        \
            ? "LE_SET_PER_ADV_SYNC_SUBEVENT"                                          \
            : opcode == BT_HCI_OP_LE_SET_PER_ADV_PARAM_V2 ? "LE_SET_PER_ADV_PARAM_V2" \
            : opcode == BT_HCI_OP_LE_PER_ADV_CREATE_SYNC  ? "LE_PER_ADV_CREATE_SYNC"  \
            : opcode == BT_HCI_OP_LE_PER_ADV_CREATE_SYNC_CANCEL                       \
            ? "LE_PER_ADV_CREATE_SYNC_CANCEL"                                         \
            : opcode == BT_HCI_OP_LE_PER_ADV_TERMINATE_SYNC                           \
            ? "LE_PER_ADV_TERMINATE_SYNC"                                             \
            : opcode == BT_HCI_OP_LE_ADD_DEV_TO_PER_ADV_LIST                          \
            ? "LE_ADD_DEV_TO_PER_ADV_LIST"                                            \
            : opcode == BT_HCI_OP_LE_REM_DEV_FROM_PER_ADV_LIST                        \
            ? "LE_REM_DEV_FROM_PER_ADV_LIST"                                          \
            : opcode == BT_HCI_OP_LE_CLEAR_PER_ADV_LIST ? "LE_CLEAR_PER_ADV_LIST"     \
            : opcode == BT_HCI_OP_LE_READ_PER_ADV_LIST_SIZE                           \
            ? "LE_READ_PER_ADV_LIST_SIZE"                                             \
            : opcode == BT_HCI_OP_LE_READ_TX_POWER        ? "LE_READ_TX_POWER"        \
            : opcode == BT_HCI_OP_LE_READ_RF_PATH_COMP    ? "LE_READ_RF_PATH_COMP"    \
            : opcode == BT_HCI_OP_LE_WRITE_RF_PATH_COMP   ? "LE_WRITE_RF_PATH_COMP"   \
            : opcode == BT_HCI_OP_LE_SET_PRIVACY_MODE     ? "LE_SET_PRIVACY_MODE"     \
            : opcode == BT_HCI_OP_LE_RX_TEST_V3           ? "LE_RX_TEST_V3"           \
            : opcode == BT_HCI_OP_LE_SET_CL_CTE_TX_PARAMS ? "LE_SET_CL_CTE_TX_PARAMS" \
            : opcode == BT_HCI_OP_LE_SET_CL_CTE_TX_ENABLE ? "LE_SET_CL_CTE_TX_ENABLE" \
            : opcode == BT_HCI_OP_LE_SET_CL_CTE_SAMPLING_ENABLE                       \
            ? "LE_SET_CL_CTE_SAMPLING_ENABLE"                                         \
            : opcode == BT_HCI_OP_LE_SET_CONN_CTE_RX_PARAMS                           \
            ? "LE_SET_CONN_CTE_RX_PARAMS"                                             \
            : opcode == BT_HCI_OP_LE_SET_CONN_CTE_TX_PARAMS                           \
            ? "LE_SET_CONN_CTE_TX_PARAMS"                                             \
            : opcode == BT_HCI_OP_LE_CONN_CTE_REQ_ENABLE ? "LE_CONN_CTE_REQ_ENABLE"   \
            : opcode == BT_HCI_OP_LE_CONN_CTE_RSP_ENABLE ? "LE_CONN_CTE_RSP_ENABLE"   \
            : opcode == BT_HCI_OP_LE_READ_ANT_INFO       ? "LE_READ_ANT_INFO"         \
            : opcode == BT_HCI_OP_LE_SET_PER_ADV_RECV_ENABLE                          \
            ? "LE_SET_PER_ADV_RECV_ENABLE"                                            \
            : opcode == BT_HCI_OP_LE_PER_ADV_SYNC_TRANSFER                            \
            ? "LE_PER_ADV_SYNC_TRANSFER"                                              \
            : opcode == BT_HCI_OP_LE_PER_ADV_SET_INFO_TRANSFER                        \
            ? "LE_PER_ADV_SET_INFO_TRANSFER"                                          \
            : opcode == BT_HCI_OP_LE_PAST_PARAM          ? "LE_PAST_PARAM"            \
            : opcode == BT_HCI_OP_LE_DEFAULT_PAST_PARAM  ? "LE_DEFAULT_PAST_PARAM"    \
            : opcode == BT_HCI_OP_LE_READ_BUFFER_SIZE_V2 ? "LE_READ_BUFFER_SIZE_V2"   \
            : opcode == BT_HCI_OP_LE_READ_ISO_TX_SYNC    ? "LE_READ_ISO_TX_SYNC"      \
            : opcode == BT_HCI_OP_LE_SET_CIG_PARAMS      ? "LE_SET_CIG_PARAMS"        \
            : opcode == BT_HCI_OP_LE_SET_CIG_PARAMS_TEST ? "LE_SET_CIG_PARAMS_TEST"   \
            : opcode == BT_HCI_OP_LE_CREATE_CIS          ? "LE_CREATE_CIS"            \
            : opcode == BT_HCI_OP_LE_REMOVE_CIG          ? "LE_REMOVE_CIG"            \
            : opcode == BT_HCI_OP_LE_ACCEPT_CIS          ? "LE_ACCEPT_CIS"            \
            : opcode == BT_HCI_OP_LE_REJECT_CIS          ? "LE_REJECT_CIS"            \
            : opcode == BT_HCI_OP_LE_CREATE_BIG          ? "LE_CREATE_BIG"            \
            : opcode == BT_HCI_OP_LE_CREATE_BIG_TEST     ? "LE_CREATE_BIG_TEST"       \
            : opcode == BT_HCI_OP_LE_TERMINATE_BIG       ? "LE_TERMINATE_BIG"         \
            : opcode == BT_HCI_OP_LE_BIG_CREATE_SYNC     ? "LE_BIG_CREATE_SYNC"       \
            : opcode == BT_HCI_OP_LE_BIG_TERMINATE_SYNC  ? "LE_BIG_TERMINATE_SYNC"    \
            : opcode == BT_HCI_OP_LE_REQ_PEER_SC         ? "LE_REQ_PEER_SC"           \
            : opcode == BT_HCI_OP_LE_SETUP_ISO_PATH      ? "LE_SETUP_ISO_PATH"        \
            : opcode == BT_HCI_OP_LE_REMOVE_ISO_PATH     ? "LE_REMOVE_ISO_PATH"       \
            : opcode == BT_HCI_OP_LE_ISO_TRANSMIT_TEST   ? "LE_ISO_TRANSMIT_TEST"     \
            : opcode == BT_HCI_OP_LE_ISO_RECEIVE_TEST    ? "LE_ISO_RECEIVE_TEST"      \
            : opcode == BT_HCI_OP_LE_ISO_READ_TEST_COUNTERS                           \
            ? "LE_ISO_READ_TEST_COUNTERS"                                             \
            : opcode == BT_HCI_OP_LE_ISO_TEST_END     ? "LE_ISO_TEST_END"             \
            : opcode == BT_HCI_OP_LE_SET_HOST_FEATURE ? "LE_SET_HOST_FEATURE"         \
            : opcode == BT_HCI_OP_LE_READ_ISO_LINK_QUALITY                            \
            ? "LE_READ_ISO_LINK_QUALITY"                                              \
            : opcode == BT_HCI_OP_LE_TX_TEST_V4 ? "LE_TX_TEST_V4"                     \
            : opcode == BT_HCI_OP_LE_CS_READ_LOCAL_SUPPORTED_CAPABILITIES             \
            ? "LE_CS_READ_LOCAL_SUPPORTED_CAPABILITIES"                               \
            : opcode == BT_HCI_OP_LE_CS_READ_REMOTE_SUPPORTED_CAPABILITIES            \
            ? "LE_CS_READ_REMOTE_SUPPORTED_CAPABILITIES"                              \
            : opcode == BT_HCI_OP_LE_CS_WRITE_CACHED_REMOTE_SUPPORTED_CAPABILITIES    \
            ? "LE_CS_WRITE_CACHED_REMOTE_SUPPORTED_CAPABILITIES"                      \
            : opcode == BT_HCI_OP_LE_CS_SECURITY_ENABLE ? "LE_CS_SECURITY_ENABLE"     \
            : opcode == BT_HCI_OP_LE_CS_SET_DEFAULT_SETTINGS                          \
            ? "LE_CS_SET_DEFAULT_SETTINGS"                                            \
            : opcode == BT_HCI_OP_LE_CS_READ_REMOTE_FAE_TABLE                         \
            ? "LE_CS_READ_REMOTE_FAE_TABLE"                                           \
            : opcode == BT_HCI_OP_LE_CS_WRITE_CACHED_REMOTE_FAE_TABLE                 \
            ? "LE_CS_WRITE_CACHED_REMOTE_FAE_TABLE"                                   \
            : opcode == BT_HCI_OP_LE_CS_SET_CHANNEL_CLASSIFICATION                    \
            ? "LE_CS_SET_CHANNEL_CLASSIFICATION"                                      \
            : opcode == BT_HCI_OP_LE_CS_SET_PROCEDURE_PARAMETERS                      \
            ? "LE_CS_SET_PROCEDURE_PARAMETERS"                                        \
            : opcode == BT_HCI_OP_LE_CS_PROCEDURE_ENABLE ? "LE_CS_PROCEDURE_ENABLE"   \
            : opcode == BT_HCI_OP_LE_CS_TEST             ? "LE_CS_TEST"               \
            : opcode == BT_HCI_OP_LE_CS_CREATE_CONFIG    ? "LE_CS_CREATE_CONFIG"      \
            : opcode == BT_HCI_OP_LE_CS_REMOVE_CONFIG    ? "LE_CS_REMOVE_CONFIG"      \
            : opcode == BT_HCI_OP_LE_CS_TEST_END         ? "LE_CS_TEST_END"           \
                                                         : "(Unknown Command)")

const char *bt_hci_opcode_to_str(uint16_t opcode)
{
    return BT_OPCODE_STR(opcode);
}
#endif //CONFIG_BT_HCI_OPCODE_TO_STR
