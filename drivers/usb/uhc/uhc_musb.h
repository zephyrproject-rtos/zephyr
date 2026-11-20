/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UHC_UHC_MUSB_H
#define ZEPHYR_DRIVERS_USB_UHC_UHC_MUSB_H

#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

/* Vendor glue configuration, must be the device config */
struct uhc_musb_config {
	/* Base address of the MUSB core registers */
	mem_addr_t mbase;
	/* Number of endpoints, including endpoint 0 */
	uint8_t num_eps;
	/* Size of the FIFO RAM in bytes */
	uint16_t fifo_size;
	bool high_speed;
	/* Power up the wrapper and the PHY */
	int (*init)(const struct device *dev);
	/* Power down the wrapper and the PHY */
	int (*shutdown)(const struct device *dev);
	/* Select the host role and switch on VBUS */
	int (*vbus_on)(const struct device *dev);
	/* Switch off VBUS */
	int (*vbus_off)(const struct device *dev);
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	void (*make_thread)(const struct device *dev);
};

/* Hardware endpoints, endpoint 0 is the control endpoint */
#define UHC_MUSB_MAX_EPS 16

struct uhc_musb_toggle {
	struct usb_device *udev;
	uint8_t ep;
	uint8_t next;
};

/* Driver private data, the vendor glue sets it as uhc_data.priv */
struct uhc_musb_data {
	struct k_thread thread;
	struct k_sem irq_sem;
	/* Interrupt status latched by the ISR, the registers are clear-on-read */
	atomic_t intrusb;
	atomic_t intrtx;
	atomic_t intrrx;
	/* Transfer owning each hardware endpoint, NULL if the endpoint is free */
	struct uhc_transfer *xfers[UHC_MUSB_MAX_EPS];
	/* Length of the OUT packet loaded into the FIFO of each endpoint */
	uint16_t packet_len[UHC_MUSB_MAX_EPS];
	/* Software data toggles, the endpoints are reprogrammed on every claim */
	struct uhc_musb_toggle toggles[32];
	/* Bytes left in the control data stage */
	uint16_t ctrl_remaining;
	uint8_t ctrl_stage;
	uint8_t vbus_retries;
	bool connected;
};

extern const struct uhc_driver_api uhc_musb_api;

void uhc_musb_isr(const struct device *dev);
void uhc_musb_thread(void *arg0, void *arg1, void *arg2);

#endif /* ZEPHYR_DRIVERS_USB_UHC_UHC_MUSB_H */
