/*
 * Copyright (c) 2018 Linaro Limited
 * Copyright (c) 2018 NXP Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * -----------------------------------------------------------------------------
 *
 * HCI Driver Interface functions to the KW41Z BLE link layer controller library
 * as defined by Zephyr.
 *
 * -----------------------------------------------------------------------------
 */
#define LOG_MODULE_NAME BLE_KW41Z
#define LOG_LEVEL CONFIG_LOG_BT_HCI_DRIVER_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <errno.h>
#include <stddef.h>

#include <zephyr.h>
#include <init.h>
#include <misc/byteorder.h>

#include <bluetooth/hci_driver.h>

#include "fsl_os_abstraction.h"
#include "fsl_xcvr.h"
#include "nxp_private.h"

/*
 * -----------------------------------------------------------------------------
 *                               Debug support
 * -----------------------------------------------------------------------------
 */
#if LOG_LEVEL == LOG_LEVEL_DBG
#include <bluetooth/hci_vs.h>

static void ble_decode_cmd(struct net_buf *buf)
{
	struct bt_hci_cmd_hdr *hdr;
	u16_t opcode;
	u16_t ocf;

	hdr    = (struct bt_hci_cmd_hdr *)(buf->data);
	opcode = sys_le16_to_cpu(hdr->opcode);
	ocf    = BT_OCF(opcode);

	printk("Hci_SendPacketToController(type: HCI_CMD (%d), len: %d):\n",
	       BT_BUF_CMD, buf->len);

	switch (BT_OGF(opcode)) {
	case BT_OGF_LINK_CTRL:
		printk("--->LINK CTRL::");
		switch (ocf) {
		case BT_OCF(BT_HCI_OP_DISCONNECT):
			printk("BT_HCI_OP_DISCONNECT<--\n");
			break;
		case BT_OCF(BT_HCI_OP_READ_REMOTE_VERSION_INFO):
			printk("BT_HCI_OP_READ_REMOTE_VERSION_INFO<--\n");
			break;
		default:
			printk("??? 0x%04X\n", ocf);
		}

		break;
	case BT_OGF_BASEBAND:
		printk("--->BASEBAND::");
		switch (ocf) {
		case BT_OCF(BT_HCI_OP_SET_EVENT_MASK):
			printk("BT_HCI_OP_SET_EVENT_MASK<--\n");
			break;

		case BT_OCF(BT_HCI_OP_RESET):
			printk("BT_HCI_OP_RESET<--\n");
			break;

		case BT_OCF(BT_HCI_OP_SET_EVENT_MASK_PAGE_2):
			printk("BT_HCI_OP_SET_EVENT_MASK_PAGE_2<--\n");
			break;

		case BT_OCF(BT_HCI_OP_READ_TX_POWER_LEVEL):
			printk("BT_HCI_OP_READ_TX_POWER_LEVEL<--\n");
			break;

		case BT_OCF(BT_HCI_OP_SET_CTL_TO_HOST_FLOW):
			printk("BT_HCI_OP_SET_CTL_TO_HOST_FLOW<--\n");
			break;

		case BT_OCF(BT_HCI_OP_HOST_BUFFER_SIZE):
			printk("BT_HCI_OP_HOST_BUFFER_SIZE<--\n");
			break;

		case BT_OCF(BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS):
			printk("BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS<--\n");
			break;

		case BT_OCF(BT_HCI_OP_READ_AUTH_PAYLOAD_TIMEOUT):
			printk("BT_HCI_OP_READ_AUTH_PAYLOAD_TIMEOUT<--\n");
			break;

		case BT_OCF(BT_HCI_OP_WRITE_AUTH_PAYLOAD_TIMEOUT):
			printk("BT_HCI_OP_WRITE_AUTH_PAYLOAD_TIMEOUT<--\n");
			break;

		default:
			printk("??? 0x%04X\n", ocf);
		}

		break;
	case BT_OGF_INFO:
		printk("--->INFO::");
		switch (ocf) {
		case BT_OCF(BT_HCI_OP_READ_LOCAL_VERSION_INFO):
			printk("BT_HCI_OP_READ_LOCAL_VERSION_INFO<--\n");
			break;

		case BT_OCF(BT_HCI_OP_READ_SUPPORTED_COMMANDS):
			printk("BT_HCI_OP_READ_SUPPORTED_COMMANDS<--\n");
			break;

		case BT_OCF(BT_HCI_OP_READ_LOCAL_FEATURES):
			printk("BT_HCI_OP_READ_LOCAL_FEATURES<--\n");
			break;

		case BT_OCF(BT_HCI_OP_READ_BD_ADDR):
			printk("BT_HCI_OP_READ_BD_ADDR<--\n");
			break;

		default:
			printk("??? 0x%04X\n", ocf);
		}
		break;
	case BT_OGF_STATUS:
		printk("--->STATUS::");
		switch (ocf) {
		case BT_OCF(BT_HCI_OP_READ_RSSI):
			printk("BT_HCI_OP_READ_RSSI<--\n");
			break;

		default:
			printk("??? 0x%04X\n", ocf);
		}
		break;
	case BT_OGF_LE:
		printk("--->LE::");
		switch (ocf) {
		case BT_OCF(BT_HCI_OP_LE_SET_EVENT_MASK):
			printk("BT_HCI_OP_LE_SET_EVENT_MASK<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_READ_BUFFER_SIZE):
			printk("BT_HCI_OP_LE_READ_BUFFER_SIZE<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_READ_LOCAL_FEATURES):
			printk("BT_HCI_OP_LE_READ_LOCAL_FEATURES<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_SET_RANDOM_ADDRESS):
			printk("BT_HCI_OP_LE_SET_RANDOM_ADDRESS<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_READ_WL_SIZE):
			printk("BT_HCI_OP_LE_READ_WL_SIZE<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_CLEAR_WL):
			printk("BT_HCI_OP_LE_CLEAR_WL<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_ADD_DEV_TO_WL):
			printk("BT_HCI_OP_LE_ADD_DEV_TO_WL<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_REM_DEV_FROM_WL):
			printk("BT_HCI_OP_LE_REM_DEV_FROM_WL<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_ENCRYPT):
			printk("BT_HCI_OP_LE_ENCRYPT<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_RAND):
			printk("BT_HCI_OP_LE_RAND<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_READ_SUPP_STATES):
			printk("BT_HCI_OP_LE_READ_SUPP_STATES<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_SET_ADV_PARAM):
			printk("BT_HCI_OP_LE_SET_ADV_PARAM<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_READ_ADV_CHAN_TX_POWER):
			printk("BT_HCI_OP_LE_READ_ADV_CHAN_TX_POWER<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_SET_ADV_DATA):
			printk("BT_HCI_OP_LE_SET_ADV_DATA<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_SET_SCAN_RSP_DATA):
			printk("BT_HCI_OP_LE_SET_SCAN_RSP_DATA<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_SET_ADV_ENABLE):
			printk("BT_HCI_OP_LE_SET_ADV_ENABLE<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_SET_SCAN_PARAM):
			printk("BT_HCI_OP_LE_SET_SCAN_PARAM<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_SET_SCAN_ENABLE):
			printk("BT_HCI_OP_LE_SET_SCAN_ENABLE<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_CREATE_CONN):
			printk("BT_HCI_OP_LE_CREATE_CONN<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_CREATE_CONN_CANCEL):
			printk("BT_HCI_OP_LE_CREATE_CONN_CANCEL<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_SET_HOST_CHAN_CLASSIF):
			printk("BT_HCI_OP_LE_SET_HOST_CHAN_CLASSIF<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_START_ENCRYPTION):
			printk("BT_HCI_OP_LE_START_ENCRYPTION<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_LTK_REQ_REPLY):
			printk("BT_HCI_OP_LE_LTK_REQ_REPLY<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY):
			printk("BT_HCI_OP_LE_LTK_REQ_NEG_REPLY<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_READ_CHAN_MAP):
			printk("BT_HCI_OP_LE_READ_CHAN_MAP<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_READ_REMOTE_FEATURES):
			printk("BT_HCI_OP_LE_READ_REMOTE_FEATURES<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_CONN_UPDATE):
			printk("BT_HCI_OP_LE_CONN_UPDATE<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY):
			printk("BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY):
			printk("BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_SET_DATA_LEN):
			printk("BT_HCI_OP_LE_SET_DATA_LEN<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_READ_DEFAULT_DATA_LEN):
			printk("BT_HCI_OP_LE_READ_DEFAULT_DATA_LEN<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_WRITE_DEFAULT_DATA_LEN):
			printk("BT_HCI_OP_LE_WRITE_DEFAULT_DATA_LEN<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_READ_MAX_DATA_LEN):
			printk("BT_HCI_OP_LE_READ_MAX_DATA_LEN<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_READ_PHY):
			printk("BT_HCI_OP_LE_READ_PHY<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_SET_DEFAULT_PHY):
			printk("BT_HCI_OP_LE_SET_DEFAULT_PHY<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_SET_PHY):
			printk("BT_HCI_OP_LE_SET_PHY<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_ADD_DEV_TO_RL):
			printk("BT_HCI_OP_LE_ADD_DEV_TO_RL<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_REM_DEV_FROM_RL):
			printk("BT_HCI_OP_LE_REM_DEV_FROM_RL<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_CLEAR_RL):
			printk("BT_HCI_OP_LE_CLEAR_RL<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_READ_RL_SIZE):
			printk("BT_HCI_OP_LE_READ_RL_SIZE<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_READ_PEER_RPA):
			printk("BT_HCI_OP_LE_READ_PEER_RPA<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_READ_LOCAL_RPA):
			printk("BT_HCI_OP_LE_READ_LOCAL_RPA<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_SET_ADDR_RES_ENABLE):
			printk("BT_HCI_OP_LE_SET_ADDR_RES_ENABLE<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_SET_RPA_TIMEOUT):
			printk("BT_HCI_OP_LE_SET_RPA_TIMEOUT<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_SET_PRIVACY_MODE):
			printk("BT_HCI_OP_LE_SET_PRIVACY_MODE<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_READ_TX_POWER):
			printk("BT_HCI_OP_LE_READ_TX_POWER<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_RX_TEST):
			printk("BT_HCI_OP_LE_RX_TEST<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_TX_TEST):
			printk("BT_HCI_OP_LE_TX_TEST<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_TEST_END):
			printk("BT_HCI_OP_LE_TEST_END<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_ENH_RX_TEST):
			printk("BT_HCI_OP_LE_ENH_RX_TEST<--\n");
			break;
		case BT_OCF(BT_HCI_OP_LE_ENH_TX_TEST):
			printk("BT_HCI_OP_LE_ENH_TX_TEST<--\n");
			break;
		default:
			printk("??? 0x%04X\n", ocf);
		}

		break;
	case BT_OGF_VS:
		printk("--->VS::");
		switch (ocf) {
		case BT_OCF(BT_HCI_OP_VS_READ_VERSION_INFO):
			printk("BT_HCI_OP_VS_READ_VERSION_INFO<--\n");
			break;
		case BT_OCF(BT_HCI_OP_VS_READ_SUPPORTED_COMMANDS):
			printk("BT_HCI_OP_VS_READ_SUPPORTED_COMMANDS<--\n");
			break;
		case BT_OCF(BT_HCI_OP_VS_READ_SUPPORTED_FEATURES):
			printk("BT_HCI_OP_VS_READ_SUPPORTED_FEATURES<--\n");
			break;
		case BT_OCF(BT_HCI_OP_VS_READ_BUILD_INFO):
			printk("BT_HCI_OP_VS_READ_BUILD_INFO<--\n");
			break;
		case BT_OCF(BT_HCI_OP_VS_WRITE_BD_ADDR):
			printk("BT_HCI_OP_VS_WRITE_BD_ADDR<--\n");
			break;
		case BT_OCF(BT_HCI_OP_VS_READ_STATIC_ADDRS):
			printk("BT_HCI_OP_VS_READ_STATIC_ADDRS<--\n");
			break;
		case BT_OCF(BT_HCI_OP_VS_READ_KEY_HIERARCHY_ROOTS):
			printk("BT_HCI_OP_VS_READ_KEY_HIERARCHY_ROOTS<--\n");
			break;
		default:
			printk("??? 0x%04X\n", ocf);
		}

		break;
	default:
		printk("---> ??? 0x%04X\n", opcode);
		break;
	}

	_hexdump(buf->data, buf->len);

}

#else
#define ble_decode_cmd(buf)
#endif


/*
 * -----------------------------------------------------------------------------
 *                              Constants
 * -----------------------------------------------------------------------------
 */

#define RADIO_0_IRQ_PRIO        0x0

/*
 * Organizationally Unique Identifier used in BD_ADDR. The 24 most significant
 * bits in BD_ADDR
 */
#define BD_ADDR_OUI     { 0x37, 0x60, 0x00 }

/*
 * -----------------------------------------------------------------------------
 *                           GLOBAL VARIABLES
 * -----------------------------------------------------------------------------
 */

/*
 * BLE Specific symbols required to be instantiated for use by the
 * NXP KW41Z Link Layer Controller Library
 */

/*
 * Event token the internal link layer task thread uses to wait on.
 * Note that the link layer controller library uses the fsl_os_abstraction to
 * perform event set/wait/clear/destroy operations. These operations are mapped
 * to Zephyr functions.
 */
osaEventId_t mControllerTaskEvent;

/* BLE Address the controller is to use */
uint8_t gBD_ADDR[6];

/*
 * Scan FIFO lockup detection interval in milliseconds. If no
 * advertising frame is reported over this period of time, then the
 * scan FIFO is flushed and the scan is restarted. This value needs to
 * be increased if:
 *  - few advertisers or
 *  - few frames reported after frame filtering using the white list or
 *  - scan window is much smaller than the scan interval
 */
uint32_t gScanFifoLockupCheckIntervalMilliSeconds = 2500;

/*
 * Time between the beginning of two consecutive advertising PDU's
 * Time = ADVERTISING_PACKET_INTERVAL * 0.625msec
 * Time range <= 10msec
 */
const uint8_t gAdvertisingPacketInterval_c =
	CONFIG_KW41_ADVERTISING_PACKET_INTERVAL;

/*
 * Advertising channels that are enabled for scanning operation.
 * Range 0x01 - 0x07
 * 0x01 - Enables channel 37 for use.
 * 0x02 - Enables channel 38 for use.
 * 0x04 - Enables channel 39 for use.
 */
const uint8_t gScanChannelMap_c = CONFIG_KW41_SCAN_CHANNEL_MAP;

/*
 * Advertising channels that are enabled for initiator scanning
 * operation.
 * Range 0x01 - 0x07
 * 0x01 - Enables channel 37 for use.
 * 0x02 - Enables channel 38 for use.
 * 0x04 - Enables channel 39 for use.
 */
const uint8_t gInitiatorChannelMap_c = CONFIG_KW41_INITIATOR_CHANNEL_MAP;

/*
 * Offset to the first instant register. Units in 625uS time slots.
 *
 * Ex. If current clock value is 0x0004 and offset is 0x0008, then the first
 *     event will begin when clock value becomes 0x000C.
 *
 * If mcOffsetToFirstInstant_c is 0xFFFF the value will be ignored and default
 * value (0x0006) will be set.
 */
const uint16_t gOffsetToFirstInstant_c = CONFIG_KW41_OFFSET_TO_FIRST_INSTANT;

/* Radio system clock selection. */
#if (RF_OSC_26MHZ == 1)
const u8_t gRfSysClk26MHz_c = 1;     /* 26MHz radio clock. */
#else
const u8_t gRfSysClk26MHz_c;         /* 32MHz radio clock. (value == 0) */
#endif

typedef enum dtmBaudrate_tag {
    gDTM_BaudRate_1200_c = 0,
    gDTM_BaudRate_2400_c,
    gDTM_BaudRate_9600_c,
    gDTM_BaudRate_14400_c,
    gDTM_BaudRate_19200_c,
    gDTM_BaudRate_38400_c,
    gDTM_BaudRate_57600_c,
    gDTM_BaudRate_115200_c
} dtmBaudrate_t;

/* Default value for the DTM 2 wire serial connection. Can be changed also by using Controller_SetDTMBaudrate defined in "controller_interface.h". */
const dtmBaudrate_t gDefaultDTMBaudrate = gDTM_BaudRate_115200_c;

/*
 * -----------------------------------------------------------------------------
 *                            Local Variables
 * -----------------------------------------------------------------------------
 */
/*
 * The BLE Link Layer controller expects to be run in a standalone thread
 */
K_THREAD_STACK_DEFINE(kw41z_controller_stack, CONFIG_KW41_CTRL_STACK_SIZE);
static struct k_thread kw41z_controller_thread_data;

/*
 * BLE host stack expects two threads of differing priority to handle normal and
 * high priority receives from the link layer controller. These threads will
 * wait on their respective fifo defined in the rx structure below.
 */
static K_THREAD_STACK_DEFINE(kw41z_rx_thread_stack, CONFIG_BT_RX_STACK_SIZE);
static struct k_thread kw41z_rx_thread_data;

static K_THREAD_STACK_DEFINE(kw41z_rx_prio_thread_stack,
				CONFIG_BT_RX_STACK_SIZE);
static struct k_thread kw41z_rx_prio_thread_data;

static struct {
	struct k_fifo fifo;
	struct k_fifo fifo_prio;
} rx = {
	.fifo = _K_FIFO_INITIALIZER(rx.fifo),
	.fifo_prio = _K_FIFO_INITIALIZER(rx.fifo_prio),
};

/*
 * -----------------------------------------------------------------------------
 *                            HCI Interface Functions
 * -----------------------------------------------------------------------------
 */

/*
 * Wrapper controller thread task which calls the Controller_TaskHandler()
 * function defined in the KW41Z BLE link layer controller library.
 */
static void ControllerTask(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	Controller_TaskHandler((void *) NULL);
}

/*
 * Common RX processing thread for regular and priority received frames.
 * The fifo parameter points to the relevant fifle from the 'rx'
 * structure above.
 */
static void rx_thread(void *fifo, void *p2, void *p3)
{
	struct net_buf *buf;
	struct bt_hci_evt_hdr *evt_hdr;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_DBG("started");

	while (1) {
		buf = net_buf_get(fifo, K_FOREVER);

		do {
			evt_hdr = (struct bt_hci_evt_hdr *)(buf->data);

			switch (bt_buf_get_type(buf)) {
			case BT_BUF_EVT:
				if (bt_hci_evt_is_prio(evt_hdr->evt)) {
					bt_recv_prio(buf);
					break;
				}
			/* Let it fall through to the default */
			default:
				bt_recv(buf);
			}

			/* Give other threads a chance to run if the ISR
			 * is receiving data so fast that rx.fifo never
			 * or very rarely goes empty.
			 */
			k_yield();

			buf = net_buf_get(fifo, K_NO_WAIT);
		} while (buf);
	}
}

/*
 * BLE link layer library RX callback function.
 */
static bleResult_t kw41z_hci_recv(hciPacketType_t type, void *pPacket, uint16_t packetSize)
{
	/* Need to get pointer to evt header (past type) */
	struct bt_hci_evt_hdr *evt_hdr;
	struct net_buf *buf;
	bool prio = FALSE;
	bleResult_t result = gBleSuccess_c;

	evt_hdr = (struct bt_hci_evt_hdr *)((u8_t *)(pPacket));

	LOG_DBG("type: 0x%02X, evt_hdr->evt: 0x%02X, evt_hdr->len: %d",
		type, evt_hdr->evt, evt_hdr->len);
	LOG_HEXDUMP_DBG(pPacket, packetSize, "");

	/*
	 * Buffer management in this system requires that we get a
	 * net_buf from the host layer to carry the HCI data.
	 */
	if (type == gHciEventPacket_c &&
	    (evt_hdr->evt == BT_HCI_EVT_CMD_COMPLETE ||
	     evt_hdr->evt == BT_HCI_EVT_CMD_STATUS)) {
		prio = bt_hci_evt_is_prio(evt_hdr->evt);
		buf = bt_buf_get_cmd_complete(K_FOREVER);
	} else if (type == gHciDataPacket_c) {
		buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	} else {
		prio = bt_hci_evt_is_prio(evt_hdr->evt);
		buf =  bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);
	}

	/*
	 * N.B. the bt_buf_get_rx() and get_cmd_complete() will set the
	 * type field of the buf but it is also in the pPacket so we can
	 * just copy the whole thing instead of trying to work around
	 * it.
	 */
	net_buf_add_mem(buf, pPacket, packetSize);

	if (prio) {
		net_buf_put(&rx.fifo_prio, buf);
	} else {
		net_buf_put(&rx.fifo, buf);
	}

	return result;
}

static int kw41z_send(struct net_buf *buf)
{
	u8_t type = 0;
	int err = 0;

	LOG_DBG("enter");

	if (!buf->len) {
		LOG_ERR("Empty HCI packet");
		return -EINVAL;
	}

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_CMD:
		ble_decode_cmd(buf);
		type = gHciCommandPacket_c;
		break;
	case BT_BUF_ACL_OUT:
		type = gHciDataPacket_c;
		break;
	default:
		type = 0;
	}

	if (!type) {
		LOG_ERR("Unknown BT buff type: 0x%X", bt_buf_get_type(buf));
		net_buf_unref(buf);
	} else {
		LOG_DBG("Hci_SendPacketToController(type: gHciDataPacket_c (%d)"
			", len: %d):", type, buf->len);
		LOG_HEXDUMP_DBG(buf->data, buf->len, "");

		err = Hci_SendPacketToController(type, buf->data, buf->len);
	}
	if (err == gCtrlSuccess_c) {
		net_buf_unref(buf);
	} else {
		LOG_ERR("Hci_SendPacketToController error: 0x%x", err);
	}

	return err;
}

static int kw41z_open(void)
{
	LOG_DBG("Entry");

	k_thread_create(&kw41z_rx_thread_data, kw41z_rx_thread_stack,
			K_THREAD_STACK_SIZEOF(kw41z_rx_thread_stack),
			rx_thread, &rx.fifo, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_RX_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&kw41z_rx_thread_data, "KW41Z RX");

	k_thread_create(&kw41z_rx_prio_thread_data, kw41z_rx_prio_thread_stack,
			K_THREAD_STACK_SIZEOF(kw41z_rx_prio_thread_stack),
			rx_thread, &rx.fifo_prio, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_RX_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&kw41z_rx_prio_thread_data, "KW41Z RX PRIO");

	return 0;
}

/* BLE HCI driver interface structure */
static const struct bt_hci_driver kw41z_ble_drv = {
	.name   = "KW41Z_BT42",
	.bus    = BT_HCI_DRIVER_BUS_UART,
	.open   = kw41z_open,
	.send   = kw41z_send,
};

#if !defined(CONFIG_BT_HCI_VS_EXT)
uint8_t bt_read_static_addrxxx(bt_addr_le_t *addr)
{
	return 0;
}
#endif /* !CONFIG_BT_HCI_VS_EXT */


/*
 * KW41Z BLE link layer controller task initialization.
 */
static int Controller_TaskInit(void)
{
	k_tid_t taskID;

	LOG_INF("Entry");

	/* create the event token the controller task will use for thread
	 * activation
	 */
	mControllerTaskEvent = OSA_EventCreate(TRUE);

	if (mControllerTaskEvent == NULL) {
		return osaStatus_Error;
	}

	/*
	 * Link Layer controller Task creation
	 */
	taskID = k_thread_create(
		&kw41z_controller_thread_data,
		kw41z_controller_stack,
		K_THREAD_STACK_SIZEOF(kw41z_controller_stack),
		ControllerTask,
		NULL, NULL, NULL,
		K_PRIO_PREEMPT(CONFIG_KW41_CTRL_PRIO),
		0, K_NO_WAIT);
	k_thread_name_set(&kw41z_controller_thread_data, "KW41Z LL CTRL");

#if (defined(CPU_MKW41Z256VHT4) || defined(CPU_MKW41Z512VHT4))

	/* Select BLE protocol on RADIO0_IRQ */
	XCVR_MISC->XCVR_CTRL =
		(uint32_t)((XCVR_MISC->XCVR_CTRL &
		(uint32_t) ~(uint32_t)(XCVR_CTRL_XCVR_CTRL_RADIO0_IRQ_SEL_MASK)) |
		(uint32_t)((0 << XCVR_CTRL_XCVR_CTRL_RADIO0_IRQ_SEL_SHIFT)));

	/* Configure Radio IRQ */
	NVIC_ClearPendingIRQ(Radio_0_IRQn);
	IRQ_CONNECT(Radio_0_IRQn, RADIO_0_IRQ_PRIO,
		    Controller_InterruptHandler, 0, 0);

	NVIC_ClearPendingIRQ(Radio_0_IRQn);
	NVIC_EnableIRQ(Radio_0_IRQn);
	NVIC_SetPriority(Radio_0_IRQn, 0x80 >> (8 - __NVIC_PRIO_BITS));

#else
	#warning "No valid CPU defined!"
#endif

	/* Set Default Tx Power Level */
	Controller_SetTxPowerLevel(
		CONFIG_KW41_ADVERTISING_DEFAULT_TX_POWER, gAdvTxChannel_c);
	Controller_SetTxPowerLevel(
		CONFIG_KW41_CONNECTION_DEFAULT_TX_POWER, gConnTxChannel_c);

	return osaStatus_Success;
}

static int _bt_kw41z_init(struct device *unused)
{
	ARG_UNUSED(unused);
	uint8_t gBD_ADDR_OUI_c[3] = BD_ADDR_OUI;
	uint64_t mac;

	LOG_INF("Entry");

	/* BLE Radio Init */
	XCVR_Init(BLE_MODE, DR_1MBPS);
	XCVR_SetXtalTrim(CONFIG_KW41_XTAL_TRIM);

	/* BLE Controller Task Init */
	if (osaStatus_Success != Controller_TaskInit()) {
		return gBleOsError_c;
	}

	gEnableSingleAdvertisement = TRUE;
	gMCUSleepDuringBleEvents = CONFIG_KW41_MCU_SLEEP_DURING_BLE_EVENTS;

	/* Configure BD_ADDR before calling the Controller_Init */

	mac = (RSIM->MAC_MSB);
	mac = mac << 32;
	mac |= (RSIM->MAC_LSB);


	memcpy(gBD_ADDR, (uint8_t *)&mac, 3);
	memcpy(&gBD_ADDR[3], (void *)gBD_ADDR_OUI_c, 3);

	LOG_HEXDUMP_DBG(&gBD_ADDR[0], 6, "Set BD_ADDR: ");

	/* BLE Controller Init */
	if (osaStatus_Success != Controller_Init(kw41z_hci_recv)) {
		LOG_ERR("ERROR initializing controller");
		return gBleOsError_c;
	}

	bt_hci_driver_register(&kw41z_ble_drv);
	return 0;
}

SYS_INIT(_bt_kw41z_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);


