/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <whd_buffer_api.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/wifi_mgmt.h>
#include <cyhal.h>

struct airoc_wifi_data {
	struct sd_card card;
	struct sdio_func sdio_func1;
	struct sdio_func sdio_func2;
	struct net_if *iface;
	bool second_interface_init;
	bool is_ap_up;
	bool is_sta_connected;
	uint8_t mac_addr[6];
	scan_result_cb_t scan_rslt_cb;
	whd_ssid_t ssid;
	whd_scan_result_t scan_result;
	struct k_sem sema_common;
	struct k_sem sema_scan;
#if defined(CONFIG_NET_STATISTICS_WIFI)
	struct net_stats_wifi stats;
#endif
	whd_driver_t whd_drv;
	struct gpio_callback host_oob_pin_cb;
	uint8_t frame_buf[NET_ETH_MAX_FRAME_SIZE];
};

struct airoc_wifi_config {
	const struct device *sdhc_dev;
	struct gpio_dt_spec wifi_reg_on_gpio;
	struct gpio_dt_spec wifi_host_wake_gpio;
	struct gpio_dt_spec wifi_dev_wake_gpio;
};

/**
 * \brief This function returns pointer type to handle instance
 *        of whd interface (whd_interface_t) which allocated in
 *        Zephyr AIROC driver (drivers/wifi/infineon/airoc_wifi.c)
 */

whd_interface_t airoc_wifi_get_whd_interface(void);
