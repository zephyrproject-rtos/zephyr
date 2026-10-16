/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Arkadiusz Grzelka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_MUSB_H
#define ZEPHYR_DRIVERS_USB_UDC_MUSB_H

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usb_ch9.h>

#define UDC_MUSB_FIFO_SLOTS(num_eps) ((num_eps) * 2U)

struct udc_musb_fifo_block {
	uint16_t offset;
	uint16_t size;
};

/* The glue embeds this as the first member of its own config. */
struct udc_musb_config {
	mem_addr_t mbase;
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	struct udc_musb_fifo_block *fifo;
	/* Endpoint FIFO RAM, a synthesis parameter of the core. */
	uint16_t fifo_ram_bytes;
	bool high_speed;
	/* Power, clock and reset the controller until the core registers respond. */
	int (*init)(const struct device *dev);
	int (*shutdown)(const struct device *dev);
	/* Unmask and mask the controller interrupt, glue mask included. */
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	void (*make_thread)(const struct device *dev);
};

enum udc_musb_ep0_stage {
	MUSB_EP0_SETUP,
	MUSB_EP0_DATA_IN,
	MUSB_EP0_DATA_OUT,
	MUSB_EP0_STATUS,
};

/* The glue embeds this as the first member of its own private data. */
struct udc_musb_data {
	struct k_thread thread_data;
	struct k_event events;
	struct k_spinlock lock;
	atomic_t xfer_new;
	atomic_t xfer_finished;
	uint8_t setup[sizeof(struct usb_setup_packet)];
	enum udc_musb_ep0_stage ep0_stage;
	uint16_t ep0_len;
	/* Status stage ended before the stack queued its buffer. */
	bool ep0_status_done;
	/* No-data request: ServicedRxPktRdy and DataEnd wait for the stack. */
	bool ep0_ack_pending;
	/* OUT endpoints whose last packet is read but RxPktRdy still set. */
	uint16_t rx_held;
};

extern const struct udc_api udc_musb_api;

int udc_musb_preinit(const struct device *dev);

/* Called by the glue ISR once it has acknowledged its own interrupt flag. */
void udc_musb_isr(const struct device *dev);

/* Entry point of the driver thread the glue creates in make_thread. */
void udc_musb_thread(void *dev, void *arg1, void *arg2);

#endif /* ZEPHYR_DRIVERS_USB_UDC_MUSB_H */
