/*
 * Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cy_result.h>
#include <cyhal_sdio.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/pinctrl/ifx_cat1-pinctrl.h>

/* WIFI interface types */
#define CYBSP_SDIO_INTERFACE             (0)
#define CYBSP_SPI_INTERFACE              (1)
#define CYBSP_M2M_INTERFACE              (2)


#define CYBSP_WIFI_INTERFACE_TYPE        CYBSP_SDIO_INTERFACE

/**
 * \brief Get the initialized sdio object used for
 *        communicating with the WiFi Chip.
 * \note This function should only be called after cybsp_init();
 * \returns The initialized sdio object.
 */
cyhal_sdio_t *cybsp_get_wifi_sdio_obj(void);


/* WiFi host-wake IRQ event */
#define CYBSP_WIFI_HOST_WAKE_IRQ_EVENT  (CYHAL_GPIO_IRQ_RISE)

/* WiFi host-wake IRQ event */
#define DT_WIFI_SDIO_NODE               DT_COMPAT_GET_ANY_STATUS_OKAY(infineon_cyw43xxx_wifi_sdio)

/* WiFi control pins */
#if !defined(CYBSP_WIFI_WL_REG_ON)
	#if DT_NODE_HAS_PROP(DT_WIFI_SDIO_NODE, wifi_reg_on_gpios)
		#define CYBSP_WIFI_WL_REG_ON \
	DT_GET_CYHAL_GPIO_FROM_DT_GPIOS(DT_WIFI_SDIO_NODE, wifi_reg_on_gpios)
	#else
		#define CYBSP_WIFI_WL_REG_ON   NC
	#endif
#endif

#if !defined(CYBSP_WIFI_HOST_WAKE)
	#if DT_NODE_HAS_PROP(DT_WIFI_SDIO_NODE, wifi_host_wake_gpios)
		#define CYBSP_WIFI_HOST_WAKE \
	DT_GET_CYHAL_GPIO_FROM_DT_GPIOS(DT_WIFI_SDIO_NODE, wifi_host_wake_gpios)
	#else
		#define CYBSP_WIFI_HOST_WAKE   NC
	#endif
#endif

#if !defined(CYBSP_WIFI_DEV_WAKE)
	#if DT_NODE_HAS_PROP(DT_WIFI_SDIO_NODE, wifi_dev_wake_gpios)
		#define CYBSP_WIFI_DEV_WAKE \
	DT_GET_CYHAL_GPIO_FROM_DT_GPIOS(DT_WIFI_SDIO_NODE, wifi_dev_wake_gpios)
	#else
		#define CYBSP_WIFI_DEV_WAKE   NC
	#endif
#endif
