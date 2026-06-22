/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zperf sample.
 */
#include <zephyr/usb/usbd.h>
#include <zephyr/net/net_config.h>

LOG_MODULE_REGISTER(zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

#ifdef CONFIG_NET_LOOPBACK_SIMULATE_PACKET_DROP
#include <zephyr/net/loopback.h>
#endif

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
#include <sample_usbd.h>
#endif

int main(void)
{
#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
	struct usbd_context *sample_usbd;
	int err;

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		return -ENODEV;
	}

	err = usbd_enable(sample_usbd);
	if (err) {
		return err;
	}

	(void)net_config_init_app(NULL, "Initializing network");
#endif /* CONFIG_USB_DEVICE_STACK_NEXT */

#ifdef CONFIG_NET_LOOPBACK_SIMULATE_PACKET_DROP
	loopback_set_packet_drop_ratio(1);
#endif

#if defined(CONFIG_NET_DHCPV4) && !defined(CONFIG_NET_CONFIG_SETTINGS)
	net_dhcpv4_start(net_if_get_default());
#endif
	return 0;
}
