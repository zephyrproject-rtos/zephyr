/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF5_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF5_CLOCK_CONTROL_H_

#if defined(CONFIG_USB) && defined(CONFIG_SOC_NRF52840)
#include <device.h>
#endif

/* TODO: move all these to clock_control.h ? */

/* Define 32KHz clock source */
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_RC
#define CLOCK_CONTROL_NRF5_K32SRC 0
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_XTAL
#define CLOCK_CONTROL_NRF5_K32SRC 1
#endif

/* Define 32KHz clock accuracy */
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_500PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 0
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_250PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 1
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_150PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 2
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_100PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 3
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_75PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 4
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_50PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 5
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_30PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 6
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_20PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 7
#endif

#if defined(CONFIG_USB) && defined(CONFIG_SOC_NRF52840)
struct usbd_power_nrf5_api {
	void (*usb_power_int_enable)(bool enable);
	bool (*vbusdet_get)(void);
	bool (*outrdy_get)(void);
};

void nrf5_power_usb_power_int_enable(struct device *dev, bool enable);
bool nrf5_power_clock_usb_vbusdet(struct device *dev);
bool nrf5_power_clock_usb_outrdy(struct device *dev);
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF5_CLOCK_CONTROL_H_ */
