/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uwb_nxp_host.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>

static bool init_flag = false;

const struct device *const s_i2c_device = DEVICE_DT_GET(DT_ALIAS(uwb_i2c_device));

#ifdef CONFIG_USB_DEVICE_STACK_NEXT
#if defined(CONFIG_NXP_UWB_USB_LOG)
static const struct device *cdc_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

USBD_DEVICE_DEFINE(my_usbd,
                   DEVICE_DT_GET(DT_NODELABEL(usb_otg)),
                   CONFIG_NXP_UWB_USB_VID,
                   CONFIG_NXP_UWB_USB_PID);

USBD_DESC_LANG_DEFINE(my_lang);
USBD_DESC_MANUFACTURER_DEFINE(my_mfr, "NXP");
USBD_DESC_SERIAL_NUMBER_DEFINE(my_sn);
USBD_CONFIGURATION_DEFINE(my_fs_config, 0, 100, NULL);
USBD_CONFIGURATION_DEFINE(my_hs_config, 0, 100, NULL);

int uwb_host_init_cdc_debug_console(void)
{
    int err;
	if (init_flag) {
        return 0;
    }
    err  = usbd_add_descriptor(&my_usbd, &my_lang);
    err |= usbd_add_descriptor(&my_usbd, &my_mfr);
    err |= usbd_add_descriptor(&my_usbd, &my_sn);
    err |= usbd_add_configuration(&my_usbd, USBD_SPEED_FS, &my_fs_config);
    err |= usbd_add_configuration(&my_usbd, USBD_SPEED_HS, &my_hs_config);
    err |= usbd_register_all_classes(&my_usbd, USBD_SPEED_FS, 1, NULL);
    err |= usbd_register_all_classes(&my_usbd, USBD_SPEED_HS, 1, NULL);
    err |= usbd_init(&my_usbd);
    err |= usbd_enable(&my_usbd);

    if (err) {
        return err;
    }

    uint32_t dtr = 0;
    while (!dtr) {
        uart_line_ctrl_get(cdc_dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }
	/* Now start the log backend — only logs from this point onwards */
    const struct log_backend *backend = log_backend_get_by_name("log_backend_uart");
    if (backend) {
        log_backend_enable(backend, backend->cb->ctx, LOG_LEVEL_DBG);
    }
	init_flag = true;
	return 0;
}

void uwb_host_deinit_cdc_debug_console(void)
{
	if (!init_flag) {
        return;
    }
    usbd_disable(&my_usbd);
    usbd_shutdown(&my_usbd);
	init_flag = false;
}

/* Enable the USB CDC debug console if configured
 * UWB subsystem is initialized with priority 999
 * We want USB to be initialized before because this is configured
 * for logging
 */
SYS_INIT(uwb_host_init_cdc_debug_console, POST_KERNEL, 998);

#endif /* CONFIG_NXP_UWB_USB_LOG */
#endif /* CONFIG_USB_DEVICE_STACK_NEXT */

void uwb_host_get_id(uint8_t *aOutUid16B, uint8_t *pOutLen)
{
	const uint8_t uid[] = {"NXP-UWB"};
	if (*pOutLen >= sizeof(uid)) {
		memcpy(aOutUid16B, (uint8_t *)uid, sizeof(uid));
		*pOutLen = sizeof(uid);
	} else {
		*pOutLen = 0;
	}
}

void AddDelayInMicroSec(int delay)
{
	k_sleep(K_USEC(delay));
}

void *uwb_bus_i2c_init(void)
{
	uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;

	if (!device_is_ready(s_i2c_device)) {
		return NULL;
	}
	if (i2c_configure(s_i2c_device, i2c_cfg)) {
		return NULL;
	}

	return (void *)s_i2c_device;
}

void uwb_bus_i2c_deinit(void *const context)
{
	(void)(context);
	return;
}

uwb_bus_status_t uwb_bus_i2c_data_tx(const void *const context, const uint8_t address,
				     const uint8_t *const txData, const uint32_t txLen)
{
	(void)(context);
	uwb_bus_status_t bus_status = kUWB_bus_Status_FAILED;
	int status = -1;

	if (txData == NULL || txLen == 0) {
		goto exit;
	}

	status = i2c_write(s_i2c_device, (uint8_t *)txData, txLen, (uint16_t)(address >> 1));
	if (status < 0) {
		goto exit;
	}
	status = 0;
	bus_status = kUWB_bus_Status_OK;

exit:
	return status;
}

uwb_bus_status_t uwb_bus_i2c_data_rx(const void *const context, const uint8_t address,
				     uint8_t *const rxData, const uint32_t rxLen)
{
	(void)(context);
	uwb_bus_status_t bus_status = kUWB_bus_Status_FAILED;
	int status = -1;

	if (rxData == NULL || rxLen == 0) {
		goto exit;
	}

	status = i2c_read(s_i2c_device, (uint8_t *)rxData, rxLen, (uint16_t)(address >> 1));
	if (status < 0) {
		goto exit;
	}

	status = 0;
	bus_status = kUWB_bus_Status_OK;
exit:
	return status;
}
