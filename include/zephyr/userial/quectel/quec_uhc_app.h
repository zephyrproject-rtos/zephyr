/*
 * Copyright (c) 2025 Quectel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stdio.h"
#include "string.h"

#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>


/*===========================================================================
 * 								define
 ===========================================================================*/
#define QUEC_UHC_DRIVER_NAME	"quec_uhc"
#define QUEC_UHC_DRIVER_ID		 quec_uhc

#define quec_print(format, ...)	LOG_DBG("[%d - %d][%s][%d]: "format"\n", (uint32_t)k_uptime_get() / 1000, (uint32_t)k_uptime_get() % 1000 , __func__, __LINE__, ##__VA_ARGS__)


#define UHC_MIN(a,b) 			(a < b ? a : b)
/*===========================================================================
 * 								enum
 ===========================================================================*/
typedef enum
{
	QUEC_AT_PORT		= 1,
	QUEC_MODEM_PORT		= 2,
	QUEC_PORT_MAX,
}quec_cdc_port_e;

typedef enum
{
	QUEC_DEVICE_CONNECT 	= 1 << 0,
	QUEC_DEVICE_DISCONNECT	= 1 << 1,

	QUEC_RX_ARRIVE			= 1 << 5,
	QUEC_RX_ERROR			= 1 << 6,

	QUEC_TX_COMPLETE		= 1 << 15,
	QUEC_TX_ERROR			= 1 << 16
}uhc_rx_event_e;

typedef enum
{
	QUEC_STATUS_CONNECT 	= 0,
	QUEC_STATUS_DISCONNECT	= 1,
	QUEC_STATUS_DEBOUNCE,
}uhc_status_e;

typedef enum
{
	QUEC_GET_DEVICE_STATUS	 = 0,	/* param: NULL  return: Is it connected to a USB device --- uhc_status_e */

	QUEC_SET_USER_CALLBACK	 = 1, /* param: quec_uhc_callback  When the USB host receives data/successfully sends data, 
							   		disconnects or successfully connects events, this callback function will be called*/
}quec_ioctl_cmd_e;

/*===========================================================================
 * 								typedef
 ===========================================================================*/
typedef void (*quec_uhc_callback)(uint32_t event, uint8_t port_id, uint32_t size);

typedef struct
{
	int (*open)(const struct device *dev, quec_cdc_port_e port);
	int (*read)(const struct device *dev, quec_cdc_port_e port, uint8_t *buffer, uint32_t size);
	int (*write)(const struct device *dev, quec_cdc_port_e port, uint8_t *buffer, uint32_t size);
	int (*close)(const struct device *dev, quec_cdc_port_e port);
	int (*read_aviable)(const struct device *dev, quec_cdc_port_e port_id);
	int (*write_aviable)(const struct device *dev, quec_cdc_port_e port_id);
	int (*ioctl)(const struct device *dev, quec_ioctl_cmd_e cmd, void *param);
}quec_uhc_api_t;


