/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief nRF70 Wi-Fi radio test shell shared types
 */

#ifndef NRF_WIFI_RADIO_TEST_SHELL_H__
#define NRF_WIFI_RADIO_TEST_SHELL_H__

#include <zephyr/kernel.h>
#include <host_rpu_sys_if.h>
#include <radio_test/fmac_structs.h>
#include <zephyr/drivers/wifi/nrf_wifi/bus/rpu_hw_if.h>

enum nrf_wifi_frequency_bands {
	NRF_WIFI_FREQ_BAND_2_4_GHZ = 0,
	NRF_WIFI_FREQ_BAND_5_GHZ,
	NRF_WIFI_FREQ_BAND_6_GHZ,
};

/* RX capture sample and display constants */
#define RX_CAP_BYTES_PER_SAMPLE 3
#define SAMPLES_PER_LINE 16
#define BYTES_PER_SAMPLE RX_CAP_BYTES_PER_SAMPLE
#define BYTES_PER_LINE (SAMPLES_PER_LINE * BYTES_PER_SAMPLE)

/* 24-bit IQ sample: 12-bit Q (imag), 12-bit I (real), 3 bytes packed. */
struct rx_cap_iq_sample {
	uint8_t bytes[RX_CAP_BYTES_PER_SAMPLE];
} __packed;

struct nrf_wifi_ctx_zep_rt {
	struct nrf_wifi_fmac_priv *fmac_priv;
	struct rpu_conf_params conf_params;
};

#endif /* NRF_WIFI_RADIO_TEST_SHELL_H__ */
