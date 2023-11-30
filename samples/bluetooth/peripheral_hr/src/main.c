/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_DIS_VAL))
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
};

static void bas_notify(void)
{
	uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level) {
		battery_level = 100U;
	}

	bt_bas_set_battery_level(battery_level);
}

static void hrs_notify(void)
{
	static uint8_t heartrate = 90U;

	/* Heartrate measurements simulation */
	heartrate++;
	if (heartrate == 160U) {
		heartrate = 90U;
	}

	bt_hrs_notify(heartrate);
}

/* Re-declaring ISRs used by BT_LL_SW_SPLIT does not work as the ISR are
 * statically assigned at application link time.
 *
 * #define DOES_NOT_WORK_WITH_BT_LL_SW_SPLIT 1
 *
 */

#if defined(DOES_NOT_WORK_WITH_BT_LL_SW_SPLIT)
ISR_DIRECT_DECLARE(xyz_radio_nrf5_isr)
{
	/* Reset the DISABLED event we wished to generate */
	NRF_RADIO->EVENTS_DISABLED = 0U;

	printk("%s: here\n", __func__);

	ISR_DIRECT_PM();

	return 1;
}
#else
static void xyz_radio_nrf5_isr(const void *param)
{
	/* BT_LL_SW_SPLIT pass-through its ISR_DIRECT_CONNECT to here */

	/* Reset the DISABLED event we generated */
	NRF_RADIO->EVENTS_DISABLED = 0U;

	/* Hey we are here */
	printk("%s: here\n", __func__);
}
#endif

int main(void)
{
	int timeout = 10;
	int err;

	if (NRF_POWER->GPREGRET != 0U) {
		printk("Xyz initialized.\n");

#if defined(CONFIG_DYNAMIC_DIRECT_INTERRUPTS)
		irq_connect_dynamic(RADIO_IRQn, 2, xyz_radio_nrf5_isr, NULL, 0U);

#else /* !CONFIG_DYNAMIC_DIRECT_INTERRUPTS */
#if defined(DOES_NOT_WORK_WITH_BT_LL_SW_SPLIT)
		IRQ_DIRECT_CONNECT(RADIO_IRQn, 2, xyz_radio_nrf5_isr, 0);
#else
		/* Use the internal interface of BT_LL_SW_SPLIT to set our
		 * custom Radio ISR. Here, the priority is set by
		 * BT_LL_SW_SPLIT implementation and can not be changed without
		 * modification to BT_LL_SW_SPLIT implementation.
		 */
		extern void radio_isr_set(void (*cb)(void *), void *param);

		radio_isr_set(xyz_radio_nrf5_isr, NULL);

		/* Other ISRs declared/connected by BT_LL_SW_SPLIT needs to be
		 * ported in a PR to be dynamic, use irq_connect_dynamic and
		 * irq_disconnect_dynamic so that custom ISRs can be set for
		 * them.
		 */
#endif
#endif /* !CONFIG_DYNAMIC_DIRECT_INTERRUPTS */

		NRF_RADIO->INTENSET = RADIO_INTENSET_DISABLED_Msk;

		irq_enable(RADIO_IRQn);

		NRF_RADIO->TASKS_DISABLE = 1U;

		while (1) {
			k_sleep(K_SECONDS(1));

			timeout--;
			if (!timeout) {
				NRF_POWER->GPREGRET = 0U;
				NVIC_SystemReset();
			}
		}
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	bt_ready();

	bt_conn_auth_cb_register(&auth_cb_display);

	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	while (1) {
		k_sleep(K_SECONDS(1));

		/* Heartrate measurements simulation */
		hrs_notify();

		/* Battery level simulation */
		bas_notify();

		timeout--;
		if (!timeout) {
			NRF_POWER->GPREGRET = 1U;
			NVIC_SystemReset();
		}
	}

	return 0;
}
