/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <whd_buffer_api.h>
#include <whd_chip_constants.h>
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
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
#include <driver.h>
#include <driver_zephyr.h>
#include <whd_chip_constants.h>
#include <whd_utils.h>
#include <whd_wlioctl.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_nm.h>
#endif

#define DT_DRV_COMPAT infineon_airoc_wifi

#if DT_PROP(DT_DRV_INST(0), spi_data_irq_shared)
#define SPI_DATA_IRQ_SHARED
#include <zephyr/drivers/pinctrl.h>

#define PINCTRL_STATE_HOST_WAKE PINCTRL_STATE_PRIV_START
#endif

#if defined(CONFIG_AIROC_WIFI_BUS_SPI)
#define AIROC_WIFI_SPI_OPERATION                                                                   \
	(SPI_WORD_SET(DT_PROP_OR(DT_DRV_INST(0), spi_word_size, 8)) |                              \
	 (DT_PROP(DT_DRV_INST(0), spi_half_duplex) ? SPI_HALF_DUPLEX : SPI_FULL_DUPLEX) |          \
	 SPI_TRANSFER_MSB)
#endif

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
/* Securty Settings (Associate) */
typedef struct whd_wpa_assoc_security {
	unsigned int wpa_version;
	unsigned int auth_alg;
	unsigned int pairwise_suite;
	unsigned int group_suite;
	unsigned int key_mgmt_suite;
	uint32_t auth_assoc_res_status;
} whd_wpa_assoc_security_t;
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
#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT)
	wl_bss_info_t scan_bss_info;
	void *supp_if_ctx;
	void *hapd_if_ctx;
	struct zep_wpa_supp_dev_callbk_fns airoc_wifi_wpa_supp_dev_callbk_fns;
	struct wpa_scan_results supp_scan_res;
	struct wpa_driver_set_key_params *key_params;
	struct wpa_driver_capa capa;

	whd_wpa_assoc_security_t assoc_sec;
	whd_assoc_info_t assoc_info;
	whd_auth_req_status_t sae_auth_req_info;
	uint16_t chanspec;
	uint16_t assoc_chanspec;
	uint8_t *mgmt_tx_data;
	size_t mgmt_tx_data_len;
	uint8_t bssid[6];
	int mode;
	uint32_t key_mgmt_suite;
#endif
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
