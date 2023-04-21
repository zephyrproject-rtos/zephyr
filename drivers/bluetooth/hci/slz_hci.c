/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/bluetooth/hci_driver.h>

#include <sl_btctrl_linklayer.h>
#include <sl_hci_common_transport.h>
#include <pa_conversions_efr32.h>
#include <sl_bt_ll_zephyr.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_driver_slz);

#define SL_BT_CONFIG_ACCEPT_LIST_SIZE	1
#define SL_BT_CONFIG_MAX_CONNECTIONS	1
#define SL_BT_CONFIG_USER_ADVERTISERS	1
#define SL_BT_CONTROLLER_BUFFER_MEMORY  CONFIG_BT_SILABS_HCI_BUFFER_MEMORY
#define SL_BT_SILABS_LL_STACK_SIZE	1024

static K_KERNEL_STACK_DEFINE(slz_ll_stack, SL_BT_SILABS_LL_STACK_SIZE);
static struct k_thread slz_ll_thread;

void rail_isr_installer(void)
{
	IRQ_CONNECT(RDMAILBOX_IRQn, 0, RDMAILBOX_IRQHandler, NULL, 0);
	IRQ_CONNECT(RAC_SEQ_IRQn, 0, RAC_SEQ_IRQHandler, NULL, 0);
	IRQ_CONNECT(RAC_RSM_IRQn, 0, RAC_RSM_IRQHandler, NULL, 0);
	IRQ_CONNECT(PROTIMER_IRQn, 0, PROTIMER_IRQHandler, NULL, 0);
	IRQ_CONNECT(MODEM_IRQn, 0, MODEM_IRQHandler, NULL, 0);
	IRQ_CONNECT(FRC_IRQn, 0, FRC_IRQHandler, NULL, 0);
	IRQ_CONNECT(BUFC_IRQn, 0, BUFC_IRQHandler, NULL, 0);
	IRQ_CONNECT(AGC_IRQn, 0, AGC_IRQHandler, NULL, 0);
}

/**
 * @brief Transmit HCI message using the currently used transport layer.
 * The HCI calls this function to transmit a full HCI message.
 * @param[in] data Packet type followed by HCI packet data.
 * @param[in] len Length of the `data` parameter
 * @return 0 - on success, or non-zero on failure.
 */
uint32_t hci_common_transport_transmit(uint8_t *data, int16_t len)
{
	struct net_buf *buf;
	uint8_t packet_type = data[0];
	uint8_t flags;
	uint8_t event_code;

	LOG_HEXDUMP_DBG(data, len, "host packet data:");

	/* drop packet type from the frame buffer - it is no longer needed */
	data = &data[1];
	len -= 1;

	switch (packet_type) {
	case h4_event:
		event_code = data[0];
		flags = bt_hci_evt_get_flags(event_code);
		buf = bt_buf_get_evt(event_code, false, K_FOREVER);
		break;
	case h4_acl:
		buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
		break;
	default:
		LOG_ERR("Unknown HCI type: %d", packet_type);
		return -EINVAL;
	}

	net_buf_add_mem(buf, data, len);
	if ((packet_type == h4_event) && (flags & BT_HCI_EVT_FLAG_RECV_PRIO)) {
		bt_recv_prio(buf);
	} else {
		bt_recv(buf);
	}

	sl_btctrl_hci_transmit_complete(0);

	return 0;
}

static int slz_bt_send(struct net_buf *buf)
{
	int rv = 0;

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		net_buf_push_u8(buf, h4_acl);
		break;
	case BT_BUF_CMD:
		net_buf_push_u8(buf, h4_command);
		break;
	default:
		rv = -EINVAL;
		goto done;
	}

	rv = hci_common_transport_receive(buf->data, buf->len, true);
	if (!rv) {
		goto done;
	}

done:
	net_buf_unref(buf);
	return rv;
}

static int slz_bt_open(void)
{
	int ret;

	/* Start RX thread */
	k_thread_create(&slz_ll_thread, slz_ll_stack,
			K_KERNEL_STACK_SIZEOF(slz_ll_stack),
			(k_thread_entry_t)slz_ll_thread_func, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO), 0,
			K_NO_WAIT);

	rail_isr_installer();
	sl_rail_util_pa_init();

	/* sl_btctrl_init_mem returns the number of memory buffers allocated */
	ret = sl_btctrl_init_mem(SL_BT_CONTROLLER_BUFFER_MEMORY);
	if (!ret) {
		LOG_ERR("Failed to allocate memory %d", ret);
		return -ENOMEM;
	}

	ret = sl_btctrl_init_ll();
	if (ret) {
		LOG_ERR("Bluetooth link layer init failed %d", ret);
		goto deinit;
	}

	sl_btctrl_init_scan();
	sl_btctrl_init_adv();
	sl_btctrl_init_conn();

	sl_btctrl_hci_parser_init_adv();
	sl_btctrl_hci_parser_init_conn();

	ret = sl_btctrl_init_basic(SL_BT_CONFIG_MAX_CONNECTIONS,
			SL_BT_CONFIG_USER_ADVERTISERS,
			SL_BT_CONFIG_ACCEPT_LIST_SIZE);
	if (ret) {
		LOG_ERR("Failed to initialize the controller %d", ret);
		goto deinit;
	}

	sl_bthci_init_upper();

	LOG_DBG("SiLabs BT HCI started");

	return 0;
deinit:
	sli_btctrl_deinit_mem();
	return ret;
}

static const struct bt_hci_driver drv = {
	.name           = "sl:bt",
	.bus            = BT_HCI_DRIVER_BUS_UART,
	.open           = slz_bt_open,
	.send           = slz_bt_send,
	.quirks         = BT_QUIRK_NO_RESET
};

static int slz_bt_init(void)
{
	int ret;

	ret = bt_hci_driver_register(&drv);
	if (ret) {
		LOG_ERR("Failed to register SiLabs BT HCI %d", ret);
	}

	return ret;
}

SYS_INIT(slz_bt_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
