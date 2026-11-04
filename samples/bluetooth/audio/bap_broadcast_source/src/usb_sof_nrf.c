/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * USB Start-of-Frame time stamping on nRF54LM20: the USBHS wrapper publishes the SOF event on
 * DPPI, which triggers a capture of the GRTC SYSCOUNTER, the time base of the Bluetooth
 * controller. The capture is read and re-armed from the UAC2 SOF callback.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <zephyr/sys/printk.h>

#include <nrfx.h>
#include <helpers/nrfx_gppi.h>

#include "usb_sof.h"

#define USBHS_WRAPPER ((NRF_USBHS_Type *)DT_REG_ADDR(DT_NODELABEL(usbhs_wrapper)))

/* The USBHS wrapper has a PUBLISH_SOF register but no EVENTS_SOF register; GPPI locates the
 * publish register at the event address plus the publish offset.
 */
#define USBHS_SOF_EVENT_ADDRESS                                                                    \
	((uint32_t)&USBHS_WRAPPER->PUBLISH_SOF -                                                   \
	 NRF_SUBSCRIBE_PUBLISH_OFFSET((uint32_t)&USBHS_WRAPPER->PUBLISH_SOF))

static int32_t grtc_chan = -1;

int usb_sof_init(void)
{
	nrfx_gppi_handle_t handle;
	int err;

	grtc_chan = z_nrf_grtc_timer_chan_alloc();
	if (grtc_chan < 0) {
		printk("USB SOF: no GRTC channel: %d\n", grtc_chan);
		return grtc_chan;
	}

	err = z_nrf_grtc_timer_capture_prepare(grtc_chan);
	if (err != 0) {
		printk("USB SOF: GRTC capture prepare failed: %d\n", err);
		return err;
	}

	err = nrfx_gppi_conn_alloc(USBHS_SOF_EVENT_ADDRESS,
				   z_nrf_grtc_timer_capture_task_address_get(grtc_chan), &handle);
	if (err != 0) {
		printk("USB SOF: GPPI connection failed: %d\n", err);
		return err;
	}

	nrfx_gppi_conn_enable(handle);

	printk("USB SOF: GRTC channel %d\n", grtc_chan);

	return 0;
}

int usb_sof_read(uint32_t *t_us)
{
	uint64_t captured;
	int err;

	if (grtc_chan < 0) {
		return -ENODEV;
	}

	err = z_nrf_grtc_timer_capture_read(grtc_chan, &captured);
	if (err != 0) {
		return err;
	}

	(void)z_nrf_grtc_timer_capture_prepare(grtc_chan);

	/* The controller uses the low 32 bits of the 1 MHz SYSCOUNTER */
	*t_us = (uint32_t)captured;

	return 0;
}
