/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>

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

#if defined(CONFIG_BT_MAX_CONN)
#define MAX_CONN CONFIG_BT_MAX_CONN
#else
#define MAX_CONN 0
#endif

#if defined(CONFIG_BT_CTLR_RL_SIZE)
#define CTLR_RL_SIZE CONFIG_BT_CTLR_RL_SIZE
#else
#define CTLR_RL_SIZE 0
#endif

static K_KERNEL_STACK_DEFINE(slz_ll_stack, CONFIG_BT_SILABS_EFR32_LINK_LAYER_STACK_SIZE);
static struct k_thread slz_ll_thread;

static K_KERNEL_STACK_DEFINE(slz_rx_stack, CONFIG_BT_DRV_RX_STACK_SIZE);
static struct k_thread slz_rx_thread;

/* Semaphore for Link Layer */
K_SEM_DEFINE(slz_ll_sem, 0, 1);

/* Events mask for Link Layer */
static atomic_t sli_btctrl_events;

/* FIFO for received HCI packets */
static struct k_fifo slz_rx_fifo;

/* FIXME: these functions should come from the SiSDK headers! */
void BTLE_LL_EventRaise(uint32_t events);
void BTLE_LL_Process(uint32_t events);
int16_t BTLE_LL_SetMaxPower(int16_t power);
bool sli_pending_btctrl_events(void);

#define RADIO_IRQN(name)     DT_IRQ_BY_NAME(DT_NODELABEL(radio), name, irq)
#define RADIO_IRQ_PRIO(name) DT_IRQ_BY_NAME(DT_NODELABEL(radio), name, priority)

void rail_isr_installer(void)
{
	IRQ_CONNECT(RADIO_IRQN(agc), RADIO_IRQ_PRIO(agc), AGC_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(bufc), RADIO_IRQ_PRIO(bufc), BUFC_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(frc_pri), RADIO_IRQ_PRIO(frc_pri), FRC_PRI_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(frc), RADIO_IRQ_PRIO(frc), FRC_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(modem), RADIO_IRQ_PRIO(modem), MODEM_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(protimer), RADIO_IRQ_PRIO(protimer), PROTIMER_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(rac_rsm), RADIO_IRQ_PRIO(rac_rsm), RAC_RSM_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(rac_seq), RADIO_IRQ_PRIO(rac_seq), RAC_SEQ_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(synth), RADIO_IRQ_PRIO(synth), SYNTH_IRQHandler, NULL, 0);

	/* Depending on the chip family, either HOSTMAILBOX, RDMAILBOX or neither is present */
	IF_ENABLED(DT_IRQ_HAS_NAME(DT_NODELABEL(radio), hostmailbox), ({
		IRQ_CONNECT(RADIO_IRQN(hostmailbox),
			    RADIO_IRQ_PRIO(hostmailbox),
			    HOSTMAILBOX_IRQHandler,
			    NULL, 0);
	}));
	IF_ENABLED(DT_IRQ_HAS_NAME(DT_NODELABEL(radio), rdmailbox), ({
		IRQ_CONNECT(RADIO_IRQN(rdmailbox),
			    RADIO_IRQ_PRIO(rdmailbox),
			    RDMAILBOX_IRQHandler,
			    NULL, 0);
	}));
}

static bool slz_is_evt_discardable(const struct bt_hci_evt_hdr *hdr, const uint8_t *params,
				   int16_t params_len)
{
	switch (hdr->evt) {
	case BT_HCI_EVT_LE_META_EVENT: {
		struct bt_hci_evt_le_meta_event *meta_evt = (void *)params;

		if (params_len < sizeof(*meta_evt)) {
			return false;
		}
		params += sizeof(*meta_evt);
		params_len -= sizeof(*meta_evt);

		switch (meta_evt->subevent) {
		case BT_HCI_EVT_LE_ADVERTISING_REPORT:
			return true;
		case BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT: {
			struct bt_hci_evt_le_ext_advertising_report *evt = (void *)params;

			if (!IS_ENABLED(CONFIG_BT_EXT_ADV)) {
				return false;
			}

			if (params_len < sizeof(*evt) + sizeof(*evt->adv_info)) {
				return false;
			}

			/* Never discard if the event could be part of a multi-part report event,
			 * because the missing part could confuse the BT host.
			 */
			return (evt->num_reports == 1) &&
			       ((evt->adv_info[0].evt_type & BT_HCI_LE_ADV_EVT_TYPE_LEGACY) != 0);
		}
		default:
			return false;
		}
	}
	default:
		return false;
	}
}

static struct net_buf *slz_bt_recv_evt(const uint8_t *data, const int16_t len)
{
	struct net_buf *buf;
	bool discardable;
	const struct bt_hci_evt_hdr *hdr = (void *)data;
	const uint8_t *params = &data[sizeof(*hdr)];
	const int16_t params_len = len - sizeof(*hdr);

	if (len < sizeof(*hdr)) {
		LOG_ERR("Event header is missing");
		return NULL;
	}

	discardable = slz_is_evt_discardable(hdr, params, params_len);
	buf = bt_buf_get_evt(hdr->evt, discardable, discardable ? K_NO_WAIT : K_FOREVER);
	if (!buf) {
		LOG_DBG("Discardable buffer pool full, ignoring event");
		return buf;
	}

	net_buf_add_mem(buf, data, len);

	return buf;
}

static struct net_buf *slz_bt_recv_acl(const uint8_t *data, const int16_t len)
{
	struct net_buf *buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);

	net_buf_add_mem(buf, data, len);

	return buf;
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
	uint8_t packet_type;

	LOG_HEXDUMP_DBG(data, len, "host packet data:");

	if (len < 1) {
		LOG_ERR("HCI packet type is missing");
		return -EINVAL;
	}

	packet_type = data[0];
	/* drop packet type from the frame buffer - it is no longer needed */
	data += 1;
	len -= 1;

	switch (packet_type) {
	case BT_HCI_H4_EVT:
		buf = slz_bt_recv_evt(data, len);
		break;
	case BT_HCI_H4_ACL:
		buf = slz_bt_recv_acl(data, len);
		break;
	default:
		LOG_ERR("Unknown HCI type: %d", packet_type);
		return -EINVAL;
	}

	if (buf) {
		k_fifo_put(&slz_rx_fifo, buf);
	}

	sl_btctrl_hci_transmit_complete(0);

	return 0;
}

static int slz_bt_send(const struct device *dev, struct net_buf *buf)
{
	int rv;

	ARG_UNUSED(dev);

	rv = hci_common_transport_receive(buf->data, buf->len, true);

	if (rv != 0) {
		return rv;
	}

	net_buf_unref(buf);
	return 0;
}

/**
 * The HCI driver thread simply waits for the LL semaphore to signal that
 * it has an event to handle, whether it's from the radio, its own scheduler,
 * or an HCI event to pass upstairs. The BTLE_LL_Process function call will
 * take care of all of them, and add HCI events to the HCI queue when applicable.
 */
static void slz_ll_thread_func(void *p1, void *p2, void *p3)
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

static void slz_rx_thread_func(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct hci_data *hci = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		struct net_buf *buf = k_fifo_get(&slz_rx_fifo, K_FOREVER);

		hci->recv(dev, buf);
	}
}

static void slz_set_tx_power(int16_t max_power_dbm)
{
	const int16_t max_power_cbm = max_power_dbm * 10;
	const int16_t actual_max_power_cbm = BTLE_LL_SetMaxPower(max_power_cbm);
	const int16_t actual_max_power_dbm = DIV_ROUND_CLOSEST(actual_max_power_cbm, 10);

	if (actual_max_power_dbm != max_power_dbm) {
		LOG_WRN("Unable to set max TX power to %d dBm, actual max is %d dBm", max_power_dbm,
			actual_max_power_dbm);
	}
}

static int slz_bt_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct hci_data *hci = dev->data;
	int ret;
	sl_status_t sl_status;

	BUILD_ASSERT(CONFIG_NUM_METAIRQ_PRIORITIES > 0,
		     "Config NUM_METAIRQ_PRIORITIES must be greater than 0");
	BUILD_ASSERT(CONFIG_BT_SILABS_EFR32_LL_THREAD_PRIO < CONFIG_NUM_METAIRQ_PRIORITIES,
		     "Config BT_SILABS_EFR32_LL_THREAD_PRIO must be a meta-IRQ priority");

	k_fifo_init(&slz_rx_fifo);

	k_thread_create(&slz_ll_thread, slz_ll_stack, K_KERNEL_STACK_SIZEOF(slz_ll_stack),
			slz_ll_thread_func, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_SILABS_EFR32_LL_THREAD_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(&slz_ll_thread, "EFR32 LL");

	k_thread_create(&slz_rx_thread, slz_rx_stack, K_KERNEL_STACK_SIZEOF(slz_rx_stack),
			slz_rx_thread_func, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(&slz_rx_thread, "EFR32 HCI RX");

	sl_rail_util_pa_init();

	/* Initialize Controller features based on Kconfig values */
	sl_status = sl_btctrl_init();
	if (sl_status != SL_STATUS_OK) {
		LOG_ERR("sl_bt_controller_init failed, status=%d", sl_status);
		ret = -EIO;
		goto deinit;
	}

	slz_set_tx_power(CONFIG_BT_CTLR_TX_PWR_ANTENNA);

	if (IS_ENABLED(CONFIG_PM)) {
		RAIL_ConfigSleep(sli_btctrl_get_radio_context_handle(),
				 RAIL_SLEEP_CONFIG_TIMERSYNC_ENABLED);
		RAIL_Status_t status = RAIL_InitPowerManager();

		if (status != RAIL_STATUS_NO_ERROR) {
			LOG_ERR("RAIL: failed to initialize power management, status=%d",
					status);
			ret = -EIO;
			goto deinit;
		}
	}

	/* Set up interrupts after Controller init, because it will overwrite them. */
	rail_isr_installer();

	hci->recv = recv;

	LOG_DBG("SiLabs BT HCI started");

	return 0;
deinit:
	sl_btctrl_deinit(); /* No-op if controller initialization failed */
	return ret;
}

static int slz_bt_close(const struct device *dev)
{
	k_thread_abort(&slz_ll_thread);
	k_thread_abort(&slz_rx_thread);

	sl_btctrl_deinit();

	LOG_DBG("SiLabs BT HCI stopped");

	return 0;
}

bool sli_pending_btctrl_events(void)
{
	return false; /* TODO: check if this should really return false! */
}

void sli_btctrl_events_init(void)
{
	atomic_clear(&sli_btctrl_events);
}

/* Store event flags and increment the LL semaphore */
void BTLE_LL_EventRaise(uint32_t events)
{
	atomic_or(&sli_btctrl_events, events);
	k_sem_give(&slz_ll_sem);
}

static DEVICE_API(bt_hci, drv) = {
	.open           = slz_bt_open,
	.close          = slz_bt_close,
	.send           = slz_bt_send,
};

#define HCI_DEVICE_INIT(inst) \
	static struct hci_data hci_data_##inst = { \
	}; \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &hci_data_##inst, NULL, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv)

/* Only one instance supported right now */
HCI_DEVICE_INIT(0)
