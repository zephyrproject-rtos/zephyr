/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <whd_buffer_api.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/wifi_mgmt.h>

#ifdef CONFIG_AIROC_WIFI_BUS_SDIO
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sdio.h>
#endif
#ifdef CONFIG_AIROC_WIFI_BUS_SPI
#include <zephyr/drivers/spi.h>
#endif

#include <cy_utils.h>

#define DT_DRV_COMPAT infineon_airoc_wifi

#if DT_PROP(DT_DRV_INST(0), spi_data_irq_shared)
#define SPI_DATA_IRQ_SHARED
#include <zephyr/drivers/pinctrl.h>

#define PINCTRL_STATE_HOST_WAKE PINCTRL_STATE_PRIV_START
#endif

#if defined(CONFIG_AIROC_WIFI_BUS_SPI)
#define AIROC_WIFI_SPI_OPERATION (SPI_WORD_SET(DT_PROP_OR(DT_DRV_INST(0), spi_word_size, 8)) \
			| (DT_PROP(DT_DRV_INST(0), spi_half_duplex) \
				? SPI_HALF_DUPLEX : SPI_FULL_DUPLEX) \
			| SPI_TRANSFER_MSB)
#endif

struct airoc_wifi_data {
#if defined(CONFIG_AIROC_WIFI_BUS_SDIO)
	struct sd_card card;
	struct sdio_func sdio_func1;
	struct sdio_func sdio_func2;
#endif
#if defined(SPI_DATA_IRQ_SHARED)
	uint8_t prev_irq_state;
#endif
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

union airoc_wifi_bus {
#if defined(CONFIG_AIROC_WIFI_BUS_SDIO)
	const struct device *bus_sdio;
#endif
#if defined(CONFIG_AIROC_WIFI_BUS_SPI)
	const struct spi_dt_spec bus_spi;
#endif
};

struct airoc_wifi_config {
	const union airoc_wifi_bus bus_dev;
	struct gpio_dt_spec wifi_reg_on_gpio;
	struct gpio_dt_spec wifi_host_wake_gpio;
	struct gpio_dt_spec wifi_dev_wake_gpio;
#if defined(CONFIG_AIROC_WIFI_BUS_SPI)
	struct gpio_dt_spec bus_select_gpio;
#if defined(SPI_DATA_IRQ_SHARED)
	const struct pinctrl_dev_config *pcfg;
#endif
#endif
};

/**
 * \brief This function returns pointer type to handle instance
 *        of whd interface (whd_interface_t) which allocated in
 *        Zephyr AIROC driver (drivers/wifi/infineon/airoc_wifi.c)
 */

whd_interface_t airoc_wifi_get_whd_interface(void);
