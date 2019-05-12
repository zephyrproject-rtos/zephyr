/*
 * Copyright (c) 2021 Nikos Oikonomou <nikoikonomou92@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_S2LP_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_S2LP_H_

#include <kernel.h>
#include <S2LP_Radio.h>
#include <S2LP_Gpio.h>
#include <S2LP_Csma.h>
#include <S2LP_Qi.h>
#include <S2LP_PktBasic.h>

struct s2lp_802154_data {
	/* Pointer to the network interface. */
	struct net_if *iface;

	/* 802.15.4 HW address. */
	uint8_t mac[8];

	/* Radio Interface */
	const struct device *spi;
	struct spi_config spi_cfg;
	struct spi_cs_control cs_ctrl;
	const struct device *sdn_gpio;
	const struct device *rx_rdy_irq_gpio;
	struct gpio_callback rx_rdy_cb;

	/* Radio Configuration */
	SRadioInit x_radio_init;
	PktBasicInit x_basic_init;
	SCsmaInit x_csma_init;
	SRssiInit x_rssi_init;
	/* Programmable GPIO Configuration */
	SGpioInit x_gpio_rx_rdy_irq;

	/* Synchronization */
	struct k_mutex phy_mutex;
	struct k_sem isr_sem;

	/* Rx Thread */
	K_THREAD_STACK_MEMBER(rx_stack, CONFIG_IEEE802154_S2LP_RX_STACK_SIZE);
	struct k_thread rx_thread;
};

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_S2LP_H_ */
