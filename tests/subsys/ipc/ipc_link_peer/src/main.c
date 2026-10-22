/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The device under test of the Python peer: an IPC link instance over the second PTY UART,
 * with an endpoint that echoes what it receives and one that only counts. Every event is
 * printed on the console for the test to read.
 */

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

struct test_ept {
	struct ipc_ept ept;
	struct ipc_ept_cfg cfg;
	bool echo;
};

static void ept_bound(void *priv)
{
	struct test_ept *t = priv;

	printk("ipc_link_peer: bound %s\n", t->cfg.name);
}

static void ept_unbound(void *priv)
{
	struct test_ept *t = priv;

	printk("ipc_link_peer: unbound %s\n", t->cfg.name);
}

static void ept_received(const void *data, size_t len, void *priv)
{
	struct test_ept *t = priv;

	printk("ipc_link_peer: received %s %zu\n", t->cfg.name, len);

	if (t->echo) {
		int ret = ipc_service_send(&t->ept, data, len);

		if (ret < 0) {
			printk("ipc_link_peer: echo failed %d\n", ret);
		}
	}
}

static struct test_ept epts[] = {
	{.cfg = {.name = "echo"}, .echo = true},
	{.cfg = {.name = "sink"}, .echo = false},
};

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	int ret;

	ret = ipc_service_open_instance(dev);
	if (ret < 0) {
		printk("ipc_link_peer: open failed %d\n", ret);
		return 0;
	}

	for (size_t i = 0; i < ARRAY_SIZE(epts); i++) {
		epts[i].cfg.priv = &epts[i];
		epts[i].cfg.cb.bound = ept_bound;
		epts[i].cfg.cb.unbound = ept_unbound;
		epts[i].cfg.cb.received = ept_received;

		ret = ipc_service_register_endpoint(dev, &epts[i].ept, &epts[i].cfg);
		if (ret < 0) {
			printk("ipc_link_peer: register %s failed %d\n", epts[i].cfg.name, ret);
			return 0;
		}
	}

	printk("ipc_link_peer: ready\n");

	return 0;
}
