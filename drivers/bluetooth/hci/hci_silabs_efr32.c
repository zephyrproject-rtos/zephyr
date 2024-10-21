/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/bluetooth.h>

#include <sl_btctrl_linklayer.h>
#include <sl_hci_common_transport.h>
#include <pa_conversions_efr32.h>
#include <rail.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_driver_efr32);

#define DT_DRV_COMPAT silabs_bt_hci_efr32

struct hci_data {
	bt_hci_recv_t recv;
};

#define SL_BT_CONFIG_ACCEPT_LIST_SIZE				1
#define SL_BT_CONFIG_MAX_CONNECTIONS				1
#define SL_BT_CONFIG_USER_ADVERTISERS				1
#define SL_BT_CONTROLLER_BUFFER_MEMORY				CONFIG_BT_SILABS_EFR32_BUFFER_MEMORY
#define SL_BT_CONTROLLER_LE_BUFFER_SIZE_MAX			CONFIG_BT_BUF_ACL_TX_COUNT
#define SL_BT_CONTROLLER_COMPLETED_PACKETS_THRESHOLD		1
#define SL_BT_CONTROLLER_COMPLETED_PACKETS_EVENTS_TIMEOUT	3
#define SL_BT_SILABS_LL_STACK_SIZE				1024

static K_KERNEL_STACK_DEFINE(slz_ll_stack, SL_BT_SILABS_LL_STACK_SIZE);
static struct k_thread slz_ll_thread;

/* Semaphore for Link Layer */
K_SEM_DEFINE(slz_ll_sem, 0, 1);

/* Events mask for Link Layer */
static atomic_t sli_btctrl_events;

/* FIXME: these functions should come from the SiSDK headers! */
void BTLE_LL_EventRaise(uint32_t events);
void BTLE_LL_Process(uint32_t events);
bool sli_pending_btctrl_events(void);
RAIL_Handle_t BTLE_LL_GetRadioHandle(void);

void rail_isr_installer(void)
{
#ifdef CONFIG_SOC_SERIES_EFR32MG24
	IRQ_CONNECT(SYNTH_IRQn, 0, SYNTH_IRQHandler, NULL, 0);
#else
	IRQ_CONNECT(RDMAILBOX_IRQn, 0, RDMAILBOX_IRQHandler, NULL, 0);
#endif
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
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct hci_data *hci = dev->data;
	struct net_buf *buf;
	uint8_t packet_type = data[0];
	uint8_t event_code;

	LOG_HEXDUMP_DBG(data, len, "host packet data:");

	/* drop packet type from the frame buffer - it is no longer needed */
	data = &data[1];
	len -= 1;

	switch (packet_type) {
	case BT_HCI_H4_EVT:
		event_code = data[0];
		buf = bt_buf_get_evt(event_code, false, K_FOREVER);
		break;
	case BT_HCI_H4_ACL:
		buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
		break;
	default:
		LOG_ERR("Unknown HCI type: %d", packet_type);
		return -EINVAL;
	}

	net_buf_add_mem(buf, data, len);
	hci->recv(dev, buf);

	sl_btctrl_hci_transmit_complete(0);

	return 0;
}

static int slz_bt_send(const struct device *dev, struct net_buf *buf)
{
	int rv = 0;

	ARG_UNUSED(dev);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		net_buf_push_u8(buf, BT_HCI_H4_ACL);
		break;
	case BT_BUF_CMD:
		net_buf_push_u8(buf, BT_HCI_H4_CMD);
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

/**
 * The HCI driver thread simply waits for the LL semaphore to signal that
 * it has an event to handle, whether it's from the radio, its own scheduler,
 * or an HCI event to pass upstairs. The BTLE_LL_Process function call will
 * take care of all of them, and add HCI events to the HCI queue when applicable.
 */
static void slz_thread_func(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		uint32_t events;

		k_sem_take(&slz_ll_sem, K_FOREVER);
		events = atomic_clear(&sli_btctrl_events);
		BTLE_LL_Process(events);
	}
}

static int slz_bt_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct hci_data *hci = dev->data;
	int ret;

	/* Start RX thread */
	k_thread_create(&slz_ll_thread, slz_ll_stack,
			K_KERNEL_STACK_SIZEOF(slz_ll_stack),
			slz_thread_func, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO), 0,
			K_NO_WAIT);

	rail_isr_installer();
	sl_rail_util_pa_init();

	/* Disable 2M and coded PHYs, they do not work with the current configuration */
	sl_btctrl_disable_2m_phy();
	sl_btctrl_disable_coded_phy();

	/* sl_btctrl_init_mem returns the number of memory buffers allocated */
	ret = sl_btctrl_init_mem(SL_BT_CONTROLLER_BUFFER_MEMORY);
	if (!ret) {
		LOG_ERR("Failed to allocate memory %d", ret);
		return -ENOMEM;
	}

	sl_btctrl_configure_le_buffer_size(SL_BT_CONTROLLER_LE_BUFFER_SIZE_MAX);

	ret = sl_btctrl_init_ll();
	if (ret) {
		LOG_ERR("Bluetooth link layer init failed %d", ret);
		goto deinit;
	}

	sl_btctrl_init_adv();
	sl_btctrl_init_scan();
	sl_btctrl_init_conn();
	sl_btctrl_init_phy();
	sl_btctrl_init_adv_ext();
	sl_btctrl_init_scan_ext();

	ret = sl_btctrl_init_basic(SL_BT_CONFIG_MAX_CONNECTIONS,
			SL_BT_CONFIG_USER_ADVERTISERS,
			SL_BT_CONFIG_ACCEPT_LIST_SIZE);
	if (ret) {
		LOG_ERR("Failed to initialize the controller %d", ret);
		goto deinit;
	}

	sl_btctrl_configure_completed_packets_reporting(
		SL_BT_CONTROLLER_COMPLETED_PACKETS_THRESHOLD,
		SL_BT_CONTROLLER_COMPLETED_PACKETS_EVENTS_TIMEOUT);

	sl_bthci_init_upper();
	sl_btctrl_hci_parser_init_default();
	sl_btctrl_hci_parser_init_conn();
	sl_btctrl_hci_parser_init_adv();
	sl_btctrl_hci_parser_init_phy();

#ifdef CONFIG_PM
	{
		RAIL_ConfigSleep(BTLE_LL_GetRadioHandle(), RAIL_SLEEP_CONFIG_TIMERSYNC_ENABLED);
		RAIL_Status_t status = RAIL_InitPowerManager();

		if (status != RAIL_STATUS_NO_ERROR) {
			LOG_ERR("RAIL: failed to initialize power management, status=%d",
					status);
			ret = -EIO;
			goto deinit;
		}
	}
#endif

	hci->recv = recv;

	LOG_DBG("SiLabs BT HCI started");

	return 0;
deinit:
	sli_btctrl_deinit_mem();
	return ret;
}

bool sli_pending_btctrl_events(void)
{
	return false; /* TODO: check if this should really return false! */
}

/* Store event flags and increment the LL semaphore */
void BTLE_LL_EventRaise(uint32_t events)
{
	atomic_or(&sli_btctrl_events, events);
	k_sem_give(&slz_ll_sem);
}

void sl_bt_controller_init(void)
{
	/* No extra initialization procedure required */
}

static const struct bt_hci_driver_api drv = {
	.open           = slz_bt_open,
	.send           = slz_bt_send,
};

#define HCI_DEVICE_INIT(inst) \
	static struct hci_data hci_data_##inst = { \
	}; \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &hci_data_##inst, NULL, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv)

/* Only one instance supported right now */
HCI_DEVICE_INIT(0)
