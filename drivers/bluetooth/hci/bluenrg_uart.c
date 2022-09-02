/*
 * Copyright (c) 2021 Marek Matej.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_bluenrg2_uart

#include <zephyr.h>
#include <string.h>
#include <init.h>
#include <drivers/uart.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <sys/ring_buffer.h>

#include <logging/log.h>

#include <devicetree.h>

#include <sys/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <drivers/bluetooth/hci_driver.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bluenrg2
#include "common/log.h"

/* uart support features */
#include "../util.h"

#define HCI_NONE		0x00
#define HCI_CMD			0x01
#define HCI_ACL			0x02
#define HCI_SCO			0x03
#define HCI_EVT			0x04
#define HCI_ISO			0x05
#define HCI_VENDOR		0xff

#define RELIABLE_PACKET(_t)  (_t == HCI_CMD || \
								_t == HCI_ACL || \
								_t = HCI_EVT || \
								_t == HCI_ISO || \
								_t == HCI_VENDOR)

#if defined(CONFIG_BT_BLUENRG_ACI)

/* From ST bluenrg1_gatt_aci.c */
#define BLUENRG_ACI_GATT_INIT						BT_OP(BT_OGF_VS, 0x101)
#define BLUENRG_ACI_GATT_ADD_SERVICE				BT_OP(BT_OGF_VS, 0x102)
#define BLUENRG_ACI_GATT_INCLUDE_SERVICE			BT_OP(BT_OGF_VS, 0x103)
#define BLUENRG_ACI_GATT_ADD_CHAR					BT_OP(BT_OGF_VS, 0x104)
#define BLUENRG_ACI_GATT_ADD_CHAR_DESC				BT_OP(BT_OGF_VS, 0x105)
#define BLUENRG_ACI_GATT_UPDATE_CHAR_VALUE			BT_OP(BT_OGF_VS, 0x106)
#define BLUENRG_ACI_GATT_DEL_CHAR					BT_OP(BT_OGF_VS, 0x107)
#define BLUENRG_ACI_GATT_DEL_SERVICE				BT_OP(BT_OGF_VS, 0x108)
#define BLUENRG_ACI_GATT_DEL_INCLUDE_SERVICE		BT_OP(BT_OGF_VS, 0x109)
#define BLUENRG_ACI_GATT_SET_EVENT_MASK				BT_OP(BT_OGF_VS, 0x10a)
#define BLUENRG_ACI_GATT_EXCHANGE_CONFIG			BT_OP(BT_OGF_VS, 0x10b)
#define BLUENRG_ACI_ATT_FIND_INFO_REQ				BT_OP(BT_OGF_VS, 0x10c)
#define BLUENRG_ACI_ATT_FIND_BY_TYPE_VALUE_REQ		BT_OP(BT_OGF_VS, 0x10d)
#define BLUENRG_ACI_ATT_READ_BY_TYPE_VALUE_REQ		BT_OP(BT_OGF_VS, 0x10e)
#define BLUENRG_ACI_ATT_READ_BY_GROUP_TYPE_REQ		BT_OP(BT_OGF_VS, 0x10f)
#define BLUENRG_ACI_ATT_PREPARE_WRITE_REQ			BT_OP(BT_OGF_VS, 0x110)
#define BLUENRG_ACI_ATT_EXECUTE_WRITE_REQ			BT_OP(BT_OGF_VS, 0x111)
#define BLUENRG_ACI_GATT_DISC_ALL_PRIM_SERVICES		BT_OP(BT_OGF_VS, 0x112)
#define BLUENRG_ACI_GATT_DISC_PRIM_SERVICE_BY_UUID	BT_OP(BT_OGF_VS, 0x113)
#define BLUENRG_ACI_GATT_FIND_INCLUDED_SERVICES		BT_OP(BT_OGF_VS, 0x114)
#define BLUENRG_ACI_GATT_DISC_ALL_CHAR_OF_SERVICE	BT_OP(BT_OGF_VS, 0x115)
#define BLUENRG_ACI_GATT_DISC_CHAR_BY_UUID			BT_OP(BT_OGF_VS, 0x116)
#define BLUENRG_ACI_GATT_DISC_ALL_CHAR_DESC			BT_OP(BT_OGF_VS, 0x117)
#define BLUENRG_ACI_GATT_READ_CHAR_VALUE			BT_OP(BT_OGF_VS, 0x118)
#define BLUENRG_ACI_GATT_READ_BY_CHAR_UUID			BT_OP(BT_OGF_VS, 0x119)
#define BLUENRG_ACI_GATT_READ_LONG_CHAR_VALUE		BT_OP(BT_OGF_VS, 0x11a)
#define BLUENRG_ACI_GATT_READ_MULTIPLE_CHAR_VALUE	BT_OP(BT_OGF_VS, 0x11b)
#define BLUENRG_ACI_GATT_WRITE_CHAR_VALUE			BT_OP(BT_OGF_VS, 0x11c)
#define BLUENRG_ACI_GATT_WRITE_LONG_CHAR_VALUE		BT_OP(BT_OGF_VS, 0x11d)
#define BLUENRG_ACI_GATT_WRITE_CHAR_RELIABLE		BT_OP(BT_OGF_VS, 0x11e)
#define BLUENRG_ACI_GATT_WRITE_LONG_CHAR_DESC		BT_OP(BT_OGF_VS, 0x11f)
#define BLUENRG_ACI_GATT_READ_LONG_CHAR_DESC		BT_OP(BT_OGF_VS, 0x120)
#define BLUENRG_ACI_GATT_WRITE_CHAR_DESC			BT_OP(BT_OGF_VS, 0x121)
#define BLUENRG_ACI_GATT_READ_CHAR_DESC				BT_OP(BT_OGF_VS, 0x122)
#define BLUENRG_ACI_GATT_WRITE_WITHOUT_RESP			BT_OP(BT_OGF_VS, 0x123)
#define BLUENRG_ACI_GATT_SIGNED_WRITE_WITHOUT_RESP	BT_OP(BT_OGF_VS, 0x124)
#define BLUENRG_ACI_GATT_CONFIRM_INDICATION			BT_OP(BT_OGF_VS, 0x125)
#define BLUENRG_ACI_GATT_WRITE_RESP					BT_OP(BT_OGF_VS, 0x126)
#define BLUENRG_ACI_GATT_ALLOW_READ					BT_OP(BT_OGF_VS, 0x127)
#define BLUENRG_ACI_GATT_SET_SECURITY_PERMISSION	BT_OP(BT_OGF_VS, 0x128)
#define BLUENRG_ACI_GATT_SET_DESC_VALUE				BT_OP(BT_OGF_VS, 0x129)
#define BLUENRG_ACI_GATT_READ_HANDLE_VALUE			BT_OP(BT_OGF_VS, 0x12a)
#define BLUENRG_ACI_GATT_UPDATE_CHAR_VALUE_EXT		BT_OP(BT_OGF_VS, 0x12c)
#define BLUENRG_ACI_GATT_DENY_READ					BT_OP(BT_OGF_VS, 0x12d)
#define BLUENRG_ACI_GATT_SET_ACCESS_PERMISSION		BT_OP(BT_OGF_VS, 0x12e)

/* From ST bluenrg1_gap_aci.c */
#define BLUENRG_ACI_GAP_SET_NON_DISCOVERABLE		BT_OP(BT_OGF_VS, 0x081)
#define BLUENRG_ACI_GAP_SET_LIMITED_DISCOVERABLE	BT_OP(BT_OGF_VS, 0x082)
#define BLUENRG_ACI_GAP_SET_DISCOVERABLE			BT_OP(BT_OGF_VS, 0x083)
#define BLUENRG_ACI_GAP_SET_DIRECT_CONNECTABLE		BT_OP(BT_OGF_VS, 0x084)
#define BLUENRG_ACI_GAP_SET_IO_CAPABILITY			BT_OP(BT_OGF_VS, 0x085)
#define BLUENRG_ACI_GAP_SET_AUTHEN_REQUIREMENT		BT_OP(BT_OGF_VS, 0x086)
#define BLUENRG_ACI_GAP_SET_AUTHOR_REQUIREMENT		BT_OP(BT_OGF_VS, 0x087)
#define BLUENRG_ACI_GAP_PASSKEY_RESP				BT_OP(BT_OGF_VS, 0x088)
#define BLUENRG_ACI_GAP_AUTHORIZATION_RESP			BT_OP(BT_OGF_VS, 0x089)
#define BLUENRG_ACI_GAP_INIT						BT_OP(BT_OGF_VS, 0x08a)
#define BLUENRG_ACI_GAP_SET_NON_CONNECTABLE			BT_OP(BT_OGF_VS, 0x08b)
#define BLUENRG_ACI_GAP_SET_UNDIRECT_CONNECTABLE	BT_OP(BT_OGF_VS, 0x08c)
#define BLUENRG_ACI_GAP_SLAVE_SECURITY_REQ			BT_OP(BT_OGF_VS, 0x08d)
#define BLUENRG_ACI_GAP_UPDATE_ADV_DATA				BT_OP(BT_OGF_VS, 0x08e)
#define BLUENRG_ACI_GAP_DELETE_AD_TYPE				BT_OP(BT_OGF_VS, 0x08f)
#define BLUENRG_ACI_GAP_GET_SECURITY_LEVEL			BT_OP(BT_OGF_VS, 0x090)
#define BLUENRG_ACI_GAP_SET_EVENT_MASK				BT_OP(BT_OGF_VS, 0x091)
#define BLUENRG_ACI_GAP_CONFIG_WHITELIST			BT_OP(BT_OGF_VS, 0x092)
#define BLUENRG_ACI_GAP_TERMINATE					BT_OP(BT_OGF_VS, 0x093)
#define BLUENRG_ACI_GAP_CLEAR_SECURITY_DB			BT_OP(BT_OGF_VS, 0x094)
#define BLUENRG_ACI_GAP_ALLOW_REBOND				BT_OP(BT_OGF_VS, 0x095)
#define BLUENRG_ACI_GAP_START_LIMITED_DISC_PROC		BT_OP(BT_OGF_VS, 0x096)
#define BLUENRG_ACI_GAP_START_GENERAL_DISC_PROC		BT_OP(BT_OGF_VS, 0x097)
#define BLUENRG_ACI_GAP_START_NAME_DISC_PROC		BT_OP(BT_OGF_VS, 0x098)
#define BLUENRG_ACI_GAP_START_AUTO_CONN_ESTAB_PROC	BT_OP(BT_OGF_VS, 0x099)
#define BLUENRG_ACI_GAP_START_GENERAL_CONN_ESTAB_PROC	BT_OP(BT_OGF_VS, 0x09a)
#define BLUENRG_ACI_GAP_START_SELECTIVE_CONN_ESTAB_PROC	BT_OP(BT_OGF_VS, 0x09b)
#define BLUENRG_ACI_GAP_CREATE_CONN					BT_OP(BT_OGF_VS, 0x09c)
#define BLUENRG_ACI_GAP_TERMINATE_GAP_PROC			BT_OP(BT_OGF_VS, 0x09d)
#define BLUENRG_ACI_GAP_START_CONN_UPDATE			BT_OP(BT_OGF_VS, 0x09e)
#define BLUENRG_ACI_GAP_SEND_PAIR_REQ				BT_OP(BT_OGF_VS, 0x09f)
#define BLUENRG_ACI_GAP_RESOLVE_PRIV_ADDR			BT_OP(BT_OGF_VS, 0x0a0)
#define BLUENRG_ACI_GAP_SET_BROADCAST_MODE			BT_OP(BT_OGF_VS, 0x0a1)
#define BLUENRG_ACI_GAP_START_OBSERVATION_PROC		BT_OP(BT_OGF_VS, 0x0a2)
#define BLUENRG_ACI_GAP_GET_BONDED_DEVICES			BT_OP(BT_OGF_VS, 0x0a3)
#define BLUENRG_ACI_GAP_IS_DEVICE_BONDED			BT_OP(BT_OGF_VS, 0x0a4)
#define BLUENRG_ACI_GAP_NUM_COMP_VAL_CONFIRM_YESNO	BT_OP(BT_OGF_VS, 0x0a5)
#define BLUENRG_ACI_GAP_GET_PASSKEY_INPUT			BT_OP(BT_OGF_VS, 0x0a6)
#define BLUENRG_ACI_GAP_GET_OOB_DATA				BT_OP(BT_OGF_VS, 0x0a7)
#define BLUENRG_ACI_GAP_SET_OOB_DATA				BT_OP(BT_OGF_VS, 0x0a8)
#define BLUENRG_ACI_GAP_ADD_DEVS_TO_RESOLVING_LIST	BT_OP(BT_OGF_VS, 0x0a9)
#define BLUENRG_ACI_GAP_REMOVE_BONDED_DEVICE		BT_OP(BT_OGF_VS, 0x0aa)

#define GAP_PERIPHERAL_ROLE						(0x01)
#define GAP_BROADCASTER_ROLE					(0x02)
#define GAP_CENTRAL_ROLE						(0x04)
#define GAP_OBSERVER_ROLE						(0x08)

/* From ST bluenrg1_hal_aci.c */
#define BLUENRG_ACI_HAL_GET_FW_BUILD_NUM		BT_OP(BT_OGF_VS, 0x000)
#define BLUENRG_ACI_HAL_GET_FW_DETAILS			BT_OP(BT_OGF_VS, 0x001)
#define BLUENRG_ACI_HAL_WRITE_CONFIG_DATA		BT_OP(BT_OGF_VS, 0x00c)
#define BLUENRG_ACI_HAL_READ_CONFIG_DATA		BT_OP(BT_OGF_VS, 0x00d)
#define BLUENRG_ACI_HAL_SET_TX_POWER			BT_OP(BT_OGF_VS, 0x00f)
#define BLUENRG_ACI_HAL_LE_TX_TEST_PKT_NUM		BT_OP(BT_OGF_VS, 0x014)
#define BLUENRG_ACI_HAL_TONE_START				BT_OP(BT_OGF_VS, 0x015)
#define BLUENRG_ACI_HAL_TONE_STOP				BT_OP(BT_OGF_VS, 0x016)
#define BLUENRG_ACI_HAL_GET_LINK_STATUS			BT_OP(BT_OGF_VS, 0x017)
#define BLUENRG_ACI_HAL_SET_RADIO_ACT_MASK		BT_OP(BT_OGF_VS, 0x018)
#define BLUENRG_ACI_HAL_GET_ANCHOR_PERIOD		BT_OP(BT_OGF_VS, 0x019)
#define BLUENRG_ACI_HAL_SET_EVENT_MASK			BT_OP(BT_OGF_VS, 0x01a)
#define BLUENRG_ACI_HAL_UPDATER_START			BT_OP(BT_OGF_VS, 0x020)
#define BLUENRG_ACI_HAL_UPDATER_REBOOT			BT_OP(BT_OGF_VS, 0x021)
#define BLUENRG_ACI_HAL_UPDATER_VERSION			BT_OP(BT_OGF_VS, 0x022)
#define BLUENRG_ACI_HAL_UPDATER_BUFSIZE			BT_OP(BT_OGF_VS, 0x023)
#define BLUENRG_ACI_HAL_UPDATER_ERASE_BLUE_FLAG	BT_OP(BT_OGF_VS, 0x024)
#define BLUENRG_ACI_HAL_UPDATER_RESET_BLUE_FLAG	BT_OP(BT_OGF_VS, 0x025)
#define BLUENRG_ACI_HAL_UPDATER_ERASE_SECTOR	BT_OP(BT_OGF_VS, 0x026)
#define BLUENRG_ACI_HAL_UPDATER_PROG_DATA_BLK	BT_OP(BT_OGF_VS, 0x027)
#define BLUENRG_ACI_HAL_UPDATER_READ_DATA_BLK	BT_OP(BT_OGF_VS, 0x028)
#define BLUENRG_ACI_HAL_UPDATER_CALC_CRC		BT_OP(BT_OGF_VS, 0x029)
#define BLUENRG_ACI_HAL_UPDATER_HW_VER			BT_OP(BT_OGF_VS, 0x02a)
#define BLUENRG_ACI_HAL_TRANSMITT_TEST_PACKETS	BT_OP(BT_OGF_VS, 0x02b)

#define BLUENRG_ACI_CONFIG_PUBADDR		0x00
#define BLUENRG_ACI_CONFIG_DIV			0x06
#define BLUENRG_ACI_CONFIG_ER			0x08
#define BLUENRG_ACI_CONFIG_IR			0x18
#define BLUENRG_ACI_CONFIG_LL_MODE		0x2C

/* Predefined values */
#define BLUENRG_ACI_LL_MODE_ON          0x01
#define BLUENRG_ACI_ROLE_1_CONN_6K      0x01
#define BLUENRG_ACI_ROLE_1_CONN_12K     0x02
#define BLUENRG_ACI_ROLE_8_CONN_12K     0x03


struct bluenrg_aci_cmd_ll_param {
    uint8_t cmd;
    uint8_t length;
    uint8_t value;
} __packed;

struct bluenrg_aci_cmd_read_config {
    uint8_t offset;
} __packed;

struct bluenrg_aci_cmd_config {
    uint8_t offset;
    uint8_t length;
    uint8_t value[0];
} __packed;

struct bluenrg_aci_cmd_txpwr {
    uint8_t en_hipwr;
    uint8_t pa_level;
} __packed;

struct bluenrg_aci_gap_init_par {
	uint8_t role;
	uint8_t enable_privacy;
	uint8_t device_name_char_len;
} __packed;

struct bluenrg_aci_gap_init_resp {
	uint8_t status;
	uint16_t svc_handle;
	uint16_t dev_name_char_handle;
	uint16_t appearance_char_handle;
} __packed;

struct bluenrg_aci_gatt_update_char_val_par {
	uint16_t serv_handle;
	uint16_t char_handle;
	uint8_t val_offset;
	uint8_t char_val_len;
	uint8_t char_val[0];
} __packed;

/* static int bluenrg_aci_config(uint8_t offset, uint8_t plen, uint8_t *param); */
static int bluenrg_aci_config_ll_mode(uint8_t mode);
static int bluenrg_aci_txpower(uint8_t en_hip, uint8_t pa_lvl);

static int bluenrg_aci_config_ll_mode(uint8_t mode)
{
	struct bluenrg_aci_cmd_ll_param *param;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BLUENRG_ACI_HAL_WRITE_CONFIG_DATA, sizeof(*param));
	if (!buf) {
		return -ENOBUFS;
	}

	param = net_buf_add(buf, sizeof(*param));
	param->cmd = BLUENRG_ACI_CONFIG_LL_MODE;
	param->length = 0x1;
	/* Force BlueNRG-MS roles to Link Layer only mode */
	param->value = mode;

	BT_DBG("LL_MODE: %s", bt_hex(buf->data, buf->len));

	bt_hci_cmd_send(BLUENRG_ACI_HAL_WRITE_CONFIG_DATA, buf);

	return 0;
}

static int bluenrg_aci_txpower(uint8_t hipwr, uint8_t palvl)
{
	struct bluenrg_aci_cmd_txpwr *pwr;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BLUENRG_ACI_HAL_SET_TX_POWER, 1+1);
	if (!buf) {
		BT_DBG("No buffers");
		return -ENOBUFS;
	}

	pwr = net_buf_add(buf, 1+1);
	pwr->en_hipwr = hipwr;
	pwr->pa_level = palvl;

	BT_DBG("Vendor: %s", bt_hex(buf->data, buf->len));

	bt_hci_cmd_send(BLUENRG_ACI_HAL_SET_TX_POWER, buf);

	return 0;
}
#endif /* CONFIG_BT_BLUENRG_ACI */

/* Vendor specific event */
#define EVT_BLUE_INITIALIZED	0x01

/* Message offsets */
#define PACKET_TYPE			0
#define EVT_HEADER_TYPE		0
#define EVT_HEADER_EVENT	1
#define EVT_HEADER_SIZE		2
#define EVT_VENDOR_CODE_LSB	3
#define EVT_VENDOR_CODE_MSB	4

#define CMD_OGF				1
#define CMD_OCF				2

#define GPIO_RESET_PIN		DT_INST_GPIO_PIN(0, reset_gpios)
#define GPIO_RESET_FLAGS	DT_INST_GPIO_FLAGS(0, reset_gpios)

/* Check DTS properties */
#define HAS_RESET_GPIO		DT_NODE_HAS_PROP(DT_INST(0, DT_DRV_COMPAT), reset_gpios)
#define HAS_WAKE_GPIO		DT_NODE_HAS_PROP(DT_INST(0, DT_DRV_COMPAT), wake_gpios)

/* RST pin */
static const struct gpio_dt_spec blerst =
					GPIO_DT_SPEC_GET(DT_INST(0, DT_DRV_COMPAT), reset_gpios);
/* WAKE pin */
static const struct gpio_dt_spec blewake =
					GPIO_DT_SPEC_GET(DT_INST(0, DT_DRV_COMPAT), wake_gpios);
/* UART device */
static const struct device *uart_dev;

static K_SEM_DEFINE(sem_initialised, 0, 1);
static K_SEM_DEFINE(sem_request, 0, 1);
static K_SEM_DEFINE(sem_busy, 1, 1);

static K_KERNEL_STACK_DEFINE(rx_stack, 3072);
static struct k_thread rx_thread_data;

/* Define and allocate ring buffer 2^7 words */
RING_BUF_ITEM_DECLARE_POW2(bluenrg_uart_rb, 8);
#define UART_RING_BUF_SIZE (256*sizeof(uint32_t))

/* Bluenrg HCI private data */
static struct bluenrg_uart {
	struct net_buf	*buf;
	struct k_fifo	fifo;

	uint16_t    remaining;
	uint16_t    discard;

	bool     have_hdr;
	bool     discardable;

	uint8_t     hdr_len;
	uint8_t     type;

	union {
		struct bt_hci_evt_hdr evt;
		struct bt_hci_acl_hdr acl;
		struct bt_hci_iso_hdr iso;
		uint8_t hdr[4];
	};

} rx = {
	.fifo = Z_FIFO_INITIALIZER(rx.fifo),
};


static inline uint16_t get_cmd(uint8_t *txmsg)
{
	return (txmsg[CMD_OCF] << 8) | txmsg[CMD_OGF];
}

static inline uint16_t get_evt(uint8_t *rxmsg)
{
	return (rxmsg[EVT_VENDOR_CODE_LSB] << 8) | rxmsg[EVT_VENDOR_CODE_MSB];
}

#if defined(CONFIG_BT_BLUENRG_PA)
#define BLUENRG_PWR_PA	CONFIG_BT_BLUENRG_PA
#else
#define BLUENRG_PWR_PA	0
#endif /* CONFIG_BT_BLUENRG_PA */
#if defined(CONFIG_BT_BLUENRG_PWR_LVL)
#define BLUENRG_PWM_LVL CONFIG_BT_BLUENRG_PWR_LVL
#else
#define BLUENRG_PWR_LVL	7
#endif /* CONFIG_BT_BLUENRG_PWR_LVL */

static void bluenrg_handle_vendor_evt(uint8_t *rxmsg)
{
	BT_DBG("VS event:%x", get_evt(rxmsg));
	/*printk("%s : %x\n", __func__, get_evt(rxmsg));*/

	switch (get_evt(rxmsg)) {
	case EVT_BLUE_INITIALIZED:
		BT_DBG("Blue initialized");
		/* Signal reset is over and we
		 * can communicate with the chip */
		k_sem_give(&sem_initialised);

#if defined(CONFIG_BT_BLUENRG_ACI)
		/* force BlueNRG to be on controller mode */
		bluenrg_aci_config_ll_mode(BLUENRG_ACI_LL_MODE_ON);
		/* Additional VS commands can be issued at this time
		 * in order to customize the DTS image on the controller */
		/*
		uint8_t pubadr[] = {0x58, 0x65, 0x00, 0xe1, 0x80, 0x03};
		bluenrg_aci_config(0, sizeof(pubadr), pubadr);
		*/
		bluenrg_aci_txpower(BLUENRG_PWR_PA, BLUENRG_PWR_LVL);
#endif /* CONFIG_BT_BLUENRG_ACI */

	default:
		break;
	}
}

static size_t data_discard(const struct device *uart, size_t len)
{
	uint8_t buf[33];
	return ring_buf_get(&bluenrg_uart_rb, buf, MIN(len, sizeof(buf)));
}

static inline void get_type(void)
{
	/* Get packet type */
	int ret = ring_buf_get(&bluenrg_uart_rb, &rx.type, 1);

	if (ret != 1) {
		rx.type = HCI_NONE;
		return;
	}

	switch (rx.type) {
	case HCI_EVT:
		rx.remaining = sizeof(rx.evt);
		rx.hdr_len = rx.remaining;
		break;
	case HCI_ACL:
		rx.remaining = sizeof(rx.acl);
		rx.hdr_len = rx.remaining;
		break;
	case HCI_ISO:
		if (IS_ENABLED(CONFIG_BT_ISO)) {
			rx.remaining = sizeof(rx.iso);
			rx.hdr_len = rx.remaining;
			break;
		}
		__fallthrough;
	/* TODO: add vendor type support */
	default:
		if (rx.type) {
			BT_ERR("Unknown HCI type 0x%02x", rx.type);
		}
		rx.type = HCI_NONE;
	}
}

static inline void get_acl_hdr(void)
{
	struct bt_hci_acl_hdr *hdr = &rx.acl;
	int to_read = sizeof(*hdr) - rx.remaining;

	rx.remaining -= ring_buf_get(&bluenrg_uart_rb, (uint8_t *)hdr + to_read,
						rx.remaining);
	if (!rx.remaining) {
		rx.remaining = sys_le16_to_cpu(hdr->len);
		rx.have_hdr = true;
	}
}

static inline void get_iso_hdr(void)
{
	struct bt_hci_iso_hdr *hdr = &rx.iso;
	unsigned int to_read = sizeof(*hdr) - rx.remaining;

	rx.remaining -= ring_buf_get(&bluenrg_uart_rb, (uint8_t *)hdr + to_read,
				       rx.remaining);
	if (!rx.remaining) {
		rx.remaining = sys_le16_to_cpu(hdr->len);
		rx.have_hdr = true;
	}
}

static inline void get_evt_hdr(void)
{
	struct bt_hci_evt_hdr *hdr = &rx.evt;
	int to_read = rx.hdr_len - rx.remaining;

	rx.remaining -= ring_buf_get(&bluenrg_uart_rb,
								(uint8_t *) hdr + to_read,
								rx.remaining);
	if (rx.hdr_len == sizeof(*hdr) && rx.remaining < sizeof(*hdr)) {
		switch (rx.evt.evt) {
		case BT_HCI_EVT_LE_META_EVENT:
			rx.remaining++;
			rx.hdr_len++;
			break;
#if defined(CONFIG_BT_BREDR)
		case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
		case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
			rx.discardable = true;
			break;
#endif
		}
	}

	if (!rx.remaining) {
		if (rx.evt.evt == BT_HCI_EVT_LE_META_EVENT &&
		    (rx.hdr[sizeof(*hdr)] == BT_HCI_EVT_LE_ADVERTISING_REPORT ||
		     rx.hdr[sizeof(*hdr)] == BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT)) {
			BT_DBG("LE_META_EVENT discardable");
			rx.discardable = true;
		}

		rx.remaining = hdr->len - (rx.hdr_len - sizeof(*hdr));
		rx.have_hdr = true;
	}
}

static inline void copy_hdr(struct net_buf *buf)
{
	net_buf_add_mem(buf, rx.hdr, rx.hdr_len);
}

static void reset_rx(void)
{
	rx.type = HCI_NONE;
	rx.remaining = 0U;
	rx.have_hdr = false;
	rx.hdr_len = 0U;
	rx.discardable = false;
}

static struct net_buf *get_rx_buf(k_timeout_t timeout)
{
	switch (rx.type) {
	case HCI_EVT:
		return bt_buf_get_evt(rx.evt.evt, rx.discardable, timeout);
	case HCI_ACL:
		return bt_buf_get_rx(BT_BUF_ACL_IN, timeout);
	case HCI_ISO:
		if (IS_ENABLED(CONFIG_BT_ISO)) {
			return bt_buf_get_rx(BT_BUF_ISO_IN, timeout);
		}
	}

	return NULL;
}

static inline void read_payload(void)
{
	struct net_buf *buf;
	uint8_t evt_flags;
	int read;

	if (!rx.buf) {
		rx.buf = get_rx_buf(K_NO_WAIT);
		if (!rx.buf) {
			if (rx.discardable) {
				BT_DBG("Discarding event 0x%02x", rx.evt.evt);
				BT_WARN("Discarding event 0x%02x", rx.evt.evt);
				rx.discard = rx.remaining;
				reset_rx();
				return;
			}

			BT_WARN("Failed to allocate, deferring to rx_thread");
			return;
		}

		if (rx.remaining > net_buf_tailroom(rx.buf)) {
			BT_ERR("Not enough space in buffer");
			rx.discard = rx.remaining;
			reset_rx();
			return;
		}

		net_buf_add_mem(rx.buf, rx.hdr, rx.hdr_len);
	}

	read = ring_buf_get(&bluenrg_uart_rb, net_buf_tail(rx.buf), rx.remaining);
	net_buf_add(rx.buf, read);
	rx.remaining -= read;

	if (rx.remaining) {
		return;
	}

	buf = rx.buf;
	rx.buf = NULL;

	if (rx.type == HCI_EVT) {
		/* Process vendor events at the driver level */
		if (rx.evt.evt == BT_HCI_EVT_VENDOR) {
			bluenrg_handle_vendor_evt(buf->data);
			reset_rx();
			return;
		}

		if (rx.evt.evt == BT_HCI_EVT_LE_META_EVENT &&
		    (rx.hdr[2] == BT_HCI_EVT_LE_ADVERTISING_REPORT ||
		     rx.hdr[2] == BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT)) {
			rx.discardable = true;
		}

		evt_flags = bt_hci_evt_get_flags(rx.evt.evt);
		bt_buf_set_type(buf, BT_BUF_EVT);

	} else {
		evt_flags = BT_HCI_EVT_FLAG_RECV;
		bt_buf_set_type(buf, BT_BUF_ACL_IN);
	}

	reset_rx();

	if (IS_ENABLED(CONFIG_BT_RECV_BLOCKING) &&
		(evt_flags & BT_HCI_EVT_FLAG_RECV_PRIO)) {
		bt_recv_prio(buf);
	}

	if (evt_flags & BT_HCI_EVT_FLAG_RECV) {
		net_buf_put(&rx.fifo, buf);
	}
}

static inline void read_header(void)
{
	switch (rx.type) {
	case HCI_NONE:
		get_type();
		return;
	case HCI_EVT:
		get_evt_hdr();
		break;
	case HCI_ACL:
		get_acl_hdr();
		break;
	case HCI_ISO:
		if (IS_ENABLED(CONFIG_BT_ISO)) {
			get_iso_hdr();
			break;
		}
		__fallthrough;
	default:
		CODE_UNREACHABLE;
		return;
	}

	if (rx.have_hdr && rx.buf) {
		if (rx.remaining > net_buf_tailroom(rx.buf)) {
			LOG_ERR("||| Not enough space in buffer");
			rx.discard = rx.remaining;
			reset_rx();
		} else {
			LOG_DBG("||||| have_hdr %i", rx.have_hdr);
			net_buf_add_mem(rx.buf, rx.hdr, rx.hdr_len);
		}
	}
}

static inline void process_rx(void)
{
#if 0
	BT_DBG("remaining %u discard %u have_hdr %u rx.buf %p len %u",
	       rx.remaining, rx.discard, rx.have_hdr, rx.buf,
	       rx.buf ? rx.buf->len : 0);
#endif

	if (rx.discard) {
		BT_DBG("RX.DISCARD !");
		rx.discard -= data_discard(uart_dev, rx.discard);
		return;
	}

	if (rx.have_hdr) {
		read_payload();
	} else {
		read_header();
	}
}

static void bt_uart_isr(const struct device *unused, void *user_data)
{
	ARG_UNUSED(unused);
	ARG_UNUSED(user_data);

	if (uart_irq_update(uart_dev) &&
		uart_irq_tx_ready(uart_dev)) {
		LOG_ERR("Spurious HCI interrupt!");
		return;
	}

	/* get all of the data off UART as fast as we can */
	while (uart_irq_update(uart_dev) && uart_irq_rx_ready(uart_dev)) {

		uint32_t size;
		uint8_t *data;
		int len, err;

		size = ring_buf_put_claim(&bluenrg_uart_rb, &data, UART_RING_BUF_SIZE);
		len = uart_fifo_read(uart_dev, data, size);
		err = ring_buf_put_finish(&bluenrg_uart_rb, len);
		if (err) {
			LOG_ERR("exceeds amount of valid data");
		}
	}
}

static void uart_rx_thread(void)
{
	struct net_buf *buf;

	while (1) {
		uart_irq_rx_enable(uart_dev);
		/* TODO: fifo reading needs some performance improvements */
		do {
			process_rx();
			buf = net_buf_get(&rx.fifo, K_NO_WAIT);
			k_msleep(1);
		} while (!buf);

		do {
			bt_recv(buf);

			/* Give other threads a chance to run if the ISR
			 * is receiving data so fast that rx.fifo never
			 * or very rarely goes empty.
			 */
			k_yield();

			buf = net_buf_get(&rx.fifo, K_NO_WAIT);
		} while (buf);
	}
}

static int bluenrg_uart_send(struct net_buf *buf)
{
	int ret = 0;
	/* BT_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len); */

	k_sem_take(&sem_busy, K_FOREVER);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		net_buf_push_u8(buf, HCI_ACL);
		break;
	case BT_BUF_CMD:
		net_buf_push_u8(buf, HCI_CMD);
		break;
	default:
		BT_ERR("             Unsupported type");
		k_sem_give(&sem_busy);
		return -EINVAL;
	}

	/* Transmit the message */
	for (int c = 0; c < buf->len; c++) {
		uart_poll_out(uart_dev, buf->data[c]);
	}
	k_sem_give(&sem_busy);

#if defined(CONFIG_BT_BLUENRG_UART)
	/*
	 * Since a RESET has been requested, the chip will now restart.
	 * Unfortunately the BlueNRG will reply with "reset received" but
	 * since it does not send back a NOP, we have no way to tell when the
	 * RESET has actually taken place.  Instead, we use the vendor command
	 * EVT_BLUE_INITIALIZED as an indication that it is safe to proceed.
	 */
	if (get_cmd(buf->data) == BT_HCI_OP_RESET) {
		BT_DBG("Waiting for EVT_INIT after sending reset command");
		k_sem_take(&sem_initialised, K_FOREVER);
	}
#endif /* CONFIG_BT_BLUENRG_UART */

	net_buf_unref(buf);

	return ret;
}

int bt_hci_transport_setup(const struct device *dev)
{
	int ret = 0;

	bt_uart_drain(uart_dev);

	return ret;
}


static int bluenrg_uart_open(void)
{
	int ret = 0;
	k_tid_t tid;

	bt_uart_drain(uart_dev);

	/* Start RX thread & Enable RX interrupt */
	tid = k_thread_create(&rx_thread_data, rx_stack,
			K_KERNEL_STACK_SIZEOF(rx_stack),
			(k_thread_entry_t)uart_rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(tid, "HCI recv");

	/* Deactivate reset condition on BlueNRG */
	gpio_pin_set(blerst.port, blerst.pin, 0);

	/* Power-up sequence, from the datasheet */
	k_msleep(2);

	/* Device will let us know when it's ready */
	k_sem_take(&sem_initialised, K_FOREVER);

	BT_DBG("BlueNRG start");

	return ret;
}

static const struct bt_hci_driver drv = {
	.name		= "BlueNRG:UART",
	.bus		= BT_HCI_DRIVER_BUS_UART,
#if defined(CONFIG_BT_BLUENRG_ACI)
	.quirks		= BT_QUIRK_NO_RESET,
#endif /* CONFIG_BT_BLUENRG_ACI */
	.open		= bluenrg_uart_open,
	.send		= bluenrg_uart_send,
};

#if defined(CONFIG_BT_HCI_VS_EVT_USER)
static bool vendor_event_cb(struct net_buf_simple *buf)
{
	BT_WARN("VS event: %s", bt_hex(buf->data, buf->len));

	return true;
}
#endif /* CONFIG_BT_HCI_VS_EVT_USER */

static int bluenrg_uart_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	/* Setup BLE reset with reset active state */
	if (blerst.port && !device_is_ready(blerst.port)) {
		LOG_ERR("Dev %s not ready", blerst.port->name);
		return -1;
	}
	gpio_pin_configure_dt(&blerst, GPIO_OUTPUT_ACTIVE);

#if HAS_WAKE_GPIO
	/* Setup BLE wake with inactive state */
	if (blewake.port && !device_is_ready(blewake.port)) {
		LOG_ERR("Dev %s not ready", blewake.port->name);
		return -1;
	}
	gpio_pin_configure_dt(&blewake, GPIO_OUTPUT_INACTIVE);
#endif	/* HAS_WAKE_GPIO */

	/* Transport device init */
	uart_dev = device_get_binding(DT_INST_BUS_LABEL(0));
	if (!uart_dev) {
		return -EINVAL;
	}
	if (!device_is_ready(uart_dev)) {
		return -EIO;
	}

	uart_irq_rx_disable(uart_dev);
	uart_irq_tx_disable(uart_dev);
	uart_irq_callback_set(uart_dev, bt_uart_isr);

	if (bt_hci_transport_setup(uart_dev) < 0) {
		return -EIO;
	}

	bt_hci_driver_register(&drv);

#if defined(CONFIG_BT_HCI_VS_EVT_USER)
	bt_hci_register_vnd_evt_cb(vendor_event_cb);
#endif /* CONFIG_BT_HCI_VS_EVT_USER */

	LOG_DBG("HCI init done");

	return 0;
}

SYS_INIT(bluenrg_uart_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
