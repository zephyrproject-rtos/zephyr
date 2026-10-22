/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/settings/settings.h>
#include <zephyr/init.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/hdlc_rcp_if/hdlc_rcp_if.h>

LOG_MODULE_REGISTER(iw61x_init, LOG_LEVEL_INF);

static K_THREAD_STACK_DEFINE(iw61x_stack, 4096);
static struct k_thread iw61x_thread;
static K_SEM_DEFINE(bt_ready_sem, 0, 1);
static atomic_t bt_init_result = ATOMIC_INIT(-EAGAIN);
#define IW61X_FW_READY_TIMEOUT 20
#define IW61x_FW_STABILITY_WAIT 1

static void bt_ready_cb(int err)
{
	atomic_set(&bt_init_result, err);

	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		k_sem_give(&bt_ready_sem);
		return;
	}

	LOG_INF("IW61X BT+15.4 firmware loaded via UART H4");
	k_sem_give(&bt_ready_sem);
}

static int ioexp_enable_spi_m2(void)
{
	const struct device *i2c = DEVICE_DT_GET(DT_NODELABEL(lpi2c1));
	uint8_t buf[2] = {0x03, 0xFE};
	int err;

	if (!device_is_ready(i2c)) {
		LOG_ERR("I2C not ready");
		return -ENODEV;
	}

	err = i2c_write(i2c, buf, sizeof(buf), 0x20);

	if (err < 0) {
		LOG_ERR("IO Expander config failed: %d", err);
		return err;
	}

	LOG_INF("IO Expander configured: IW61X SPI M.2 active");
	return 0;
}

int iw61x_wait_ready(k_timeout_t timeout)
{
	int err;

	/* Wait for bt_ready_cb: BT+15.4 firmware uploaded */
	if (k_sem_take(&bt_ready_sem, timeout)) {
		LOG_ERR("IW61X BT init timeout");
		return -ETIMEDOUT;
	}

	err = (int)atomic_get(&bt_init_result);
	if (err) {
		return err;
	}

	/* Wait for 15.4 co-processor to be operational */
	LOG_INF("Waiting for IW61X 15.4 co-processor...");
	k_sleep(K_SECONDS(IW61x_FW_STABILITY_WAIT));

	/* IO Expander: enable physical SPI routing on M.2 connector */
	err = ioexp_enable_spi_m2();
	if (err) {
		LOG_ERR("IO Expander failed: %d", err);
		return err;
	}

	LOG_INF("IW61X fully ready: BT + 15.4 SPI operational");
	return 0;
}


static void iw61x_bringup_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(OPENTHREAD));
	const struct hdlc_api *api;
	int err;

	err = iw61x_wait_ready(K_SECONDS(IW61X_FW_READY_TIMEOUT));
	if (err) {
		LOG_ERR("IW61X bringup failed: %d", err);
		return;
	}

	if (iface == NULL) {
		LOG_ERR("OpenThread interface not found");
		return;
	}

	api = net_if_get_device(iface)->api;
	if (api != NULL && api->deferred_init != NULL) {
		api->deferred_init();
	}

	net_if_up(iface);
}

static int iw61x_init_fn(void)
{
	LOG_INF("IW61X bringup started (bt_enable)...");
	int err;

	err = bt_enable(bt_ready_cb);
	if (err) {
		LOG_ERR("bt_enable failed: %d", err);
		atomic_set(&bt_init_result, err);
		k_sem_give(&bt_ready_sem);
		return err;
	}

	k_thread_create(&iw61x_thread, iw61x_stack,
			K_THREAD_STACK_SIZEOF(iw61x_stack),
			iw61x_bringup_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
	k_thread_name_set(&iw61x_thread, "iw61x_bringup");

	return 0;
}

/*
 * APPLICATION/89: before network stack (APPLICATION/90)
 */
SYS_INIT(iw61x_init_fn, APPLICATION, 89);
