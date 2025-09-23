/*
 * Copyright (c) 2024 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Zephyr CYW20829 driver.
 *
 *  This driver uses btstack-integration asset as hosts platform adaptation layer
 *  (porting layer) for CYW20829. btstack-integration layer implements/
 *  invokes the interfaces defined by BTSTACK to enable communication
 *  with the BT controller by using IPC_BTSS (IPC Bluetooth sub-system interface).
 *  Zephyr CYW20829 driver implements wiced_bt_**** functions requreds for
 *  btstack-integration asset and Zephyr Bluetooth driver interface
 *  (defined in struct bt_hci_driver).
 *
 *                                               CM33 (application core)
 *                                   |=========================================|
 *                                   |            |-------------------------|  |
 *                                   |            |     Zephyr application  |  |
 *                                   |            |-------------------------|  |
 *                                   |                               |         |
 *                                   |                         |------------|  |
 *                                   |                         |  Zephyr    |  |
 *                                   |                         |  Bluetooth |  |
 *      CM33 (BTSS core)             |                         |  Host      |  |
 *  |=====================|          |                         |------------|  |
 *  |                     |          |                               |         |
 *  |  |---------------|  |          |   |--------------|      | -----------|  |
 *  |  | Bluetooth     |  | IPC_BTSS |   | btstack-     |      |  Zephyr    |  |
 *  |  | Controller FW |  | <--------|-> | integration  | ---- |  CYW20829  |  |
 *  |  |---------------|  |          |   | asset        |      |  driver    |  |
 *  |                     |          |   |--------------|      |------------|  |
 *  |=====================|          |                                         |
 *            |                      |=========================================|
 *  |====================|
 *  |     CYW20829       |
 *  |     Bluetooth      |
 *  |====================|
 *
 *  NOTE:
 *   cyw920829 requires fetch binary files of Bluetooth controller firmware.
 *   To fetch Binary Blobs:  west blobs fetch hal_infineon
 *
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <wiced_bt_stack_platform.h>
#include <cybt_platform_config.h>
#include <cybt_platform_trace.h>
#include <cybt_platform_hci.h>
#include <cybt_platform_task.h>

#include <cyabs_rtos.h>
#include <cybt_result.h>

#include "cyhal_syspm.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cyw208xx);

#define DT_DRV_COMPAT infineon_cyw208xx_hci

struct cyw208xx_data {
	bt_hci_recv_t recv;
};

enum {
	BT_HCI_VND_OP_DOWNLOAD_MINIDRIVER = 0xFC2E,
	BT_HCI_VND_OP_WRITE_RAM = 0xFC4C,
	BT_HCI_VND_OP_LAUNCH_RAM = 0xFC4E,
	BT_HCI_VND_OP_UPDATE_BAUDRATE = 0xFC18,
	BT_HCI_VND_OP_SET_LOCAL_DEV_ADDR = 0xFC01,
};

/* Externs for CY43xxx controller FW */
extern const uint8_t brcm_patchram_buf[];
extern const int brcm_patch_ram_length;

#define CYBSP_BT_PLATFORM_CFG_SLEEP_MODE_LP_ENABLED (1)
#define BTM_SET_LOCAL_DEV_ADDR_LENGTH               6

static K_SEM_DEFINE(hci_sem, 1, 1);
static K_SEM_DEFINE(cybt_platform_task_init_sem, 0, 1);

cy_en_syspm_status_t cyw208xx_syspm_callback(cy_stc_syspm_callback_params_t *callbackParams,
					     cy_en_syspm_callback_mode_t mode);

static cy_stc_syspm_callback_params_t cyw208xx_syspm_callback_param = {NULL, NULL};
static cy_stc_syspm_callback_t cyw208xx_syspm_callback_cfg = {
	.callback = &cyw208xx_syspm_callback,
	.type = (cy_en_syspm_callback_type_t)CY_SYSPM_DEEPSLEEP | CY_SYSPM_SLEEP,
	.callbackParams = &cyw208xx_syspm_callback_param,
	.order = 253u,
};

/* Extern btstack integration functions */
extern void host_stack_platform_interface_init(void);
extern void cybt_platform_hci_wait_for_boot_fully_up(bool is_from_isr);
extern uint8_t *host_stack_get_acl_to_lower_buffer(wiced_bt_transport_t transport, uint32_t size);
extern wiced_result_t host_stack_send_acl_to_lower(wiced_bt_transport_t transport, uint8_t *data,
						   uint16_t len);
extern wiced_result_t host_stack_send_cmd_to_lower(uint8_t *cmd, uint16_t cmd_len);
extern wiced_result_t host_stack_send_iso_to_lower(uint8_t *data, uint16_t len);
extern cybt_result_t cybt_platform_msg_to_bt_task(const uint16_t msg, bool is_from_isr);
extern void cybt_bttask_deinit(void);
uint8_t task_queue_utilization(void);

static int cyw208xx_bt_firmware_download(const uint8_t *firmware_image, uint32_t size)
{
	uint8_t *data = (uint8_t *)firmware_image;
	volatile uint32_t remaining_length = size;
	struct net_buf *buf;
	int err;

	LOG_DBG("Executing Fw downloading for CYW208xx device");

	/* The firmware image (.hcd format) contains a collection of hci_write_ram
	 * command + a block of the image, followed by a hci_write_ram image at the end.
	 * Parse and send each individual command and wait for the response. This is to
	 * ensure the integrity of the firmware image sent to the bluetooth chip.
	 */
	while (remaining_length) {
		size_t data_length = data[2]; /* data length from firmware image block */
		uint16_t op_code = *(uint16_t *)data;

		if (op_code == BT_HCI_VND_OP_LAUNCH_RAM) {
			/* set hf0 to 48MHz */
			Cy_SysClk_ClkHfSetSource(0U, CY_SYSCLK_CLKHF_IN_CLKPATH1);
		}

		/* Allocate buffer for hci_write_ram/hci_launch_ram command. */
		buf = bt_hci_cmd_alloc(K_FOREVER);
		if (buf == NULL) {
			LOG_ERR("Unable to allocate command buffer");
			return -ENOBUFS;
		}

		/* Add data part of packet */
		net_buf_add_mem(buf, &data[3], data_length);

		/* Send hci_write_ram command. */
		err = bt_hci_cmd_send_sync(op_code, buf, NULL);
		if (err) {
			return err;
		}

		switch (op_code) {
		case BT_HCI_VND_OP_WRITE_RAM:
			/* Update remaining length and data pointer:
			 * content of data length + 2 bytes of opcode and 1 byte of data length.
			 */
			data += data_length + 3;
			remaining_length -= data_length + 3;
			break;

		case BT_HCI_VND_OP_LAUNCH_RAM:
			remaining_length = 0;
			break;

		default:
			return -ENOMEM;
		}
	}

	LOG_DBG("Fw downloading complete");
	return 0;
}

static int cyw208xx_setup(const struct device *dev, const struct bt_hci_setup_params *params)
{
	ARG_UNUSED(dev);

	int err;
	struct net_buf *buf;

	/* Avoid sleep while downloading firmware */
	cyhal_syspm_lock_deepsleep();

	/* Send HCI_RESET */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_RESET, NULL, NULL);
	if (err) {
		cyhal_syspm_unlock_deepsleep();
		return err;
	}

	/* BT firmware download */
	err = cyw208xx_bt_firmware_download(brcm_patchram_buf, (uint32_t)brcm_patch_ram_length);
	if (err) {
		cyhal_syspm_unlock_deepsleep();
		return err;
	}

	/* Waiting when BLE up after firmware launch */
	cybt_platform_hci_wait_for_boot_fully_up(false);

	/* Set public address */
	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (buf == NULL) {
		LOG_ERR("Unable to allocate command buffer");
		cyhal_syspm_unlock_deepsleep();
		return -ENOMEM;
	}

	bt_addr_t *data = net_buf_add(buf, BTM_SET_LOCAL_DEV_ADDR_LENGTH);

	bt_addr_copy(data, &(params->public_addr));

	/* NOTE: By default, the CYW208xx controller sets some hard-coded static address.
	 * To avoid address duplication, let's always override the default address by using
	 * the HCI command BT_HCI_VND_OP_SET_LOCAL_DEV_ADDR. So
	 *
	 * 1. when cyw208xx_setup gets BT_ADDR_ANY from the host, it will overwrite the
	 *    default address, and the host will switch to using a random address (set in
	 *    the hci_init function).
	 *
	 * 2. If user set the static address (by using bt_id_create) before bt_enable,
	 *    cyw208xx_setup will set user defined static address.
	 */

	err = bt_hci_cmd_send_sync(BT_HCI_VND_OP_SET_LOCAL_DEV_ADDR, buf, NULL);
	if (err) {
		LOG_ERR("Failed to set public address (%d)", err);
		cyhal_syspm_unlock_deepsleep();
		return err;
	}

	cyhal_syspm_unlock_deepsleep();

	return 0;
}

static int cyw208xx_open(const struct device *dev, bt_hci_recv_t recv)
{
	int err;
	struct cyw208xx_data *hci = dev->data;

	hci->recv = recv;

	/* Initialize Bluetooth platform related OS tasks. */
	err = cybt_platform_task_init((void *)NULL);
	if (err) {
		return err;
	}

	/* Wait until cybt platform task starts */
	k_sem_take(&cybt_platform_task_init_sem, K_FOREVER);

	return 0;
}

static int cyw208xx_close(const struct device *dev)
{
	struct cyw208xx_data *hci = dev->data;

	/* Send SHUTDOWN event, BT task will release resources and tervinate task */
	cybt_platform_msg_to_bt_task(BT_EVT_TASK_SHUTDOWN, false);

	cybt_bttask_deinit();

	k_sem_reset(&cybt_platform_task_init_sem);
	hci->recv = NULL;

	return 0;
}

static int cyw208xx_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type;

	ARG_UNUSED(dev);

	int ret;

	k_sem_take(&hci_sem, K_FOREVER);

	type = net_buf_pull_u8(buf);

	LOG_DBG("buf %p type %u len %u", buf, type, buf->len);

	switch (type) {
	case BT_HCI_H4_ACL:
		uint8_t *bt_msg = host_stack_get_acl_to_lower_buffer(BT_TRANSPORT_LE, buf->len);

		memcpy(bt_msg, buf->data, buf->len);
		ret = host_stack_send_acl_to_lower(BT_TRANSPORT_LE, bt_msg, buf->len);
		break;

	case BT_HCI_H4_CMD:
		ret = host_stack_send_cmd_to_lower(buf->data, buf->len);
		break;

	case BT_HCI_H4_ISO:
		ret = host_stack_send_iso_to_lower(buf->data, buf->len);
		break;

	default:
		LOG_ERR("Unknown type %u", type);
		ret = EIO;
		goto done;
	}

	LOG_HEXDUMP_DBG(buf->data, buf->len, "Final HCI buffer:");

	if (ret) {
		LOG_ERR("SPI write error %d", ret);
	}

done:
	k_sem_give(&hci_sem);

	if (ret != 0) {
		return -EIO;
	}

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, drv) = {
	.open = cyw208xx_open,
	.close = cyw208xx_close,
	.send = cyw208xx_send,
	.setup = cyw208xx_setup
};

static int cyw208xx_hci_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	const cybt_platform_config_t cybsp_bt_platform_cfg = {
		.hci_config = {
			.hci_transport = CYBT_HCI_IPC,
			},

		.controller_config = {
			.sleep_mode = {
				.sleep_mode_enabled =
				CYBSP_BT_PLATFORM_CFG_SLEEP_MODE_LP_ENABLED,
				},
		}};

	/* Configure platform specific settings for the BT device */
	cybt_platform_config_init(&cybsp_bt_platform_cfg);

	if (!Cy_SysPm_RegisterCallback(&cyw208xx_syspm_callback_cfg)) {
		LOG_ERR("Syspm Callback registering failed!");
		CY_ASSERT(0);
	}
	return 0;
}

/* Implements wiced_bt_**** functions requreds for the btstack-integration asset */

wiced_result_t
wiced_bt_dev_vendor_specific_command(uint16_t opcode, uint8_t param_len, uint8_t *param_buf,
				     wiced_bt_dev_vendor_specific_command_complete_cback_t cback)

{
	/*
	 * This function is using only by btstack-integration asset
	 * for enable LPM.
	 */
	struct net_buf *buf = NULL;

	/* Allocate a HCI command buffer */
	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (!buf) {
		LOG_ERR("Unable to allocate buffer");
		return WICED_NO_MEMORY;
	}

	/* Add data part of packet */
	net_buf_add_mem(buf, param_buf, param_len);
	bt_hci_cmd_send(opcode, buf);

	return WICED_BT_SUCCESS;
}

void wiced_bt_process_hci(hci_packet_type_t pti, uint8_t *data, uint32_t length)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct cyw208xx_data *hci = dev->data;
	struct net_buf *buf = NULL;
	size_t buf_tailroom = 0;

	switch (pti) {
	case HCI_PACKET_TYPE_EVENT:
		buf = bt_buf_get_evt(data[0], 0, K_NO_WAIT);
		if (!buf) {
			LOG_ERR("Failed to allocate the buffer for RX: EVENT ");
			return;
		}
		break;

	case HCI_PACKET_TYPE_ACL:
		buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
		if (!buf) {
			LOG_ERR("Failed to allocate the buffer for RX: ACL ");
			return;
		}
		break;

	case HCI_PACKET_TYPE_SCO:
		/* NA */
		break;

	case HCI_PACKET_TYPE_ISO:
		buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
		if (!buf) {
			LOG_ERR("Failed to allocate the buffer for RX: ISO ");
			return;
		}
		break;

	default:
		return;
	}

	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < length) {
		LOG_WRN("Not enough space for rx data");
		return;
	}
	net_buf_add_mem(buf, data, length);

	/* Provide the buffer to the host */
	hci->recv(dev, buf);
}

void wiced_bt_process_hci_events(uint8_t *data, uint32_t length)
{
	wiced_bt_process_hci(HCI_PACKET_TYPE_EVENT, data, length);
}

void wiced_bt_process_acl_data(uint8_t *data, uint32_t length)
{
	wiced_bt_process_hci(HCI_PACKET_TYPE_ACL, data, length);
}

void wiced_bt_process_isoc_data(uint8_t *data, uint32_t length)
{
	wiced_bt_process_hci(HCI_PACKET_TYPE_ISO, data, length);
}

void wiced_bt_stack_init_internal(wiced_bt_management_cback_t mgmt_cback,
				  wiced_bt_internal_post_stack_init_cb post_stack_cb,
				  wiced_bt_internal_stack_evt_handler_cb evt_handler_cb)
{
	k_sem_give(&cybt_platform_task_init_sem);
}

/* Keep below empty functions, used in the btstack_integration assets for Wiced BT stack. */
void wiced_bt_stack_indicate_lower_tx_complete(void)
{
	/* NA for Zephyr */
}

void wiced_bt_stack_shutdown(void)
{
	/* NA for Zephyr */
}

void wiced_bt_process_timer(void)
{
	/* NA for Zephyr */
}

cy_en_syspm_status_t cyw208xx_syspm_callback(cy_stc_syspm_callback_params_t *callbackParams,
					     cy_en_syspm_callback_mode_t mode)
{
	cy_en_syspm_status_t retVal = CY_SYSPM_FAIL;

	CY_UNUSED_PARAMETER(callbackParams);

	switch (mode) {
	case CY_SYSPM_CHECK_READY:
	case CY_SYSPM_BEFORE_TRANSITION: {
		retVal = (task_queue_utilization() == 0) ? CY_SYSPM_SUCCESS : CY_SYSPM_FAIL;
		break;
	}

	case CY_SYSPM_CHECK_FAIL:
	case CY_SYSPM_AFTER_TRANSITION: {

		retVal = CY_SYSPM_SUCCESS;
		break;
	}

	default:
		break;
	}

	return retVal;
}

#define CYW208XX_DEVICE_INIT(inst)                                                                 \
	static struct cyw208xx_data cyw208xx_data_##inst = {};                                     \
	DEVICE_DT_INST_DEFINE(inst, cyw208xx_hci_init, NULL, &cyw208xx_data_##inst, NULL,          \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv)

/* Only one instance supported */
CYW208XX_DEVICE_INIT(0)
