/*
 * Copyright (c) 2025 Quectel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <string.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include "stm32_hsem.h"
#include "quec_uhc_driver.h"
#include "stm32h5xx_hal_rcc_ex.h"

#define DT_DRV_COMPAT    st_stm32_usb
LOG_MODULE_REGISTER(quec_uhc_drv, LOG_LEVEL_DBG);

/*===========================================================================
 * 							  variables
 ===========================================================================*/
static quec_udrv_mgr_t uhc_control = {0};
static void uhc_debounce_callback(struct k_timer *timer);

K_MSGQ_DEFINE(uhc_urb_msgq, sizeof(quec_uhc_msg_t), 10, 4);
K_TIMER_DEFINE(uhc_dec_timer, uhc_debounce_callback, NULL);


/*===========================================================================
 * 							  Functions
 ===========================================================================*/
void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd)
{
	quec_udrv_mgr_t *driver = (quec_udrv_mgr_t *)hhcd->pData;
	if(driver == NULL)
	{
		quec_print("driver err");
		return;
	}

	driver->status = QUEC_STATUS_DEBOUNCE;
	k_timer_start(&uhc_dec_timer, K_MSEC(50), K_MSEC(0));
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd)
{
	quec_udrv_mgr_t *driver = (quec_udrv_mgr_t *)hhcd->pData;
	if(driver == NULL)
	{
		quec_print("driver err");
		return;
	}

	if(driver->status == QUEC_STATUS_DEBOUNCE)
	{
		k_timer_stop(&uhc_dec_timer);
	}
	else if(driver->status != QUEC_STATUS_DISCONNECT)
	{
		if(driver->callback)
		{
			driver->callback(QUEC_DEVICE_DISCONNECT, NULL);
		}
	}

	driver->status = QUEC_STATUS_DISCONNECT;
}

void uhc_debounce_callback(struct k_timer *timer)
{
	uhc_control.status = QUEC_STATUS_CONNECT;
	if(uhc_control.callback)
	{
		uhc_control.callback(QUEC_DEVICE_CONNECT, NULL);
	}
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum, HCD_URBStateTypeDef urb_state)
{
	quec_udrv_mgr_t *driver = (quec_udrv_mgr_t *)hhcd->pData;
	if(driver == NULL || chnum > 16)
	{
		quec_print("driver err %p %d", driver, chnum);
		return;
	}

	quec_udrv_port_t *port = &driver->port[chnum];
	if(port->xfer == NULL || port->xfer->callback == NULL)
	{
		quec_print("port err %d %d %p", chnum, urb_state, port->xfer);
		return;
	}

	if(urb_state == URB_DONE || urb_state == URB_STALL)
	{
		uint32_t irq_hd = irq_lock();
		port->xfer->status = (urb_state == URB_DONE ? 0 : -1);
		port->xfer->actual = HAL_HCD_HC_GetXferCount(hhcd, chnum);
		port->xfer->callback(port->xfer);
		irq_unlock(irq_hd);
	}
}

static void uhc_stm32_isr(const void *arg)
{
	HAL_HCD_IRQHandler(&uhc_control.hcd);
}

static int uhc_stm32_hw_init(HCD_HandleTypeDef *hcd)
{
	quec_print("start init uhc...");
	
	hcd->Instance = USB_DRD_FS;
	hcd->Init.Host_channels = 8;
    hcd->Init.speed = HCD_SPEED_FULL;
    hcd->Init.dma_enable = DISABLE;
    hcd->Init.phy_itface = HCD_PHY_EMBEDDED;
    hcd->Init.Sof_enable = ENABLE;
	hcd->pData = (void *)&uhc_control;

	if (HAL_HCD_Init(hcd) != HAL_OK)
	{
		quec_print("HCD init failed");
		return -1;
	}

	if(HAL_HCD_Start(hcd) != HAL_OK)
	{
		quec_print("HCD start failed");
		return -1;		
	}

	IRQ_CONNECT(USB_DRD_FS_IRQn, 0, uhc_stm32_isr, 0, 0);
	irq_enable(USB_DRD_FS_IRQn);
	
	quec_print("uhc init ok");
	return 0;
}

static int uhc_stm32_clock_enable(void)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct stm32_pclken pclken[] = STM32_DT_INST_CLOCKS(0);

	if (!device_is_ready(clk)) 
	{
		quec_print("clock control device not ready");
		return -1;
	}

	LL_PWR_EnableVDDUSB();

	if (DT_INST_NUM_CLOCKS(0) > 1)
	{
		if (clock_control_configure(clk, (clock_control_subsys_t)&pclken[1], NULL) != 0) 
		{
			quec_print("Could not select USB domain clock");
			return -1;
		}
	}

	if (clock_control_on(clk, (clock_control_subsys_t)&pclken[0]) != 0)
	{
		quec_print("Unable to enable USB clock");
		return -1;
	}

	quec_print("uhc clock enabled");
	return 0;
}

static int quec_stm32_chip_init(const struct device *dev)
{
	if(uhc_stm32_clock_enable() != 0)
	{
		return -1;
	}

	return 0;
}

static uint8_t quec_stm32_alloc_port(quec_udrv_mgr_t *uhc_mgr)
{
    uint8_t index;
	
    for (index = 1; index <= 16 ; index++)
    {
		if(uhc_mgr->port[index].occupied == false)
		{
			uhc_mgr->port[index].occupied = true;
			uhc_mgr->port[index].xfer = NULL;

			return index;
		}
    }
	
    return UHC_PORT_INVALID;
}

static void quec_stm32_free_port(quec_udrv_mgr_t *uhc_mgr, uint8_t port_id)
{
	uhc_mgr->port[port_id].occupied = false;
	uhc_mgr->port[port_id].xfer = NULL;

	quec_print("free port %d", port_id);
}

static int quec_stm32_enqueue(const struct device *dev, quec_uhc_xfer_t *xfer)
{
	quec_udrv_mgr_t *uhc_cfg = dev->data;
	int ret = 0;

	uhc_cfg->port[xfer->port_num].xfer = xfer;

	ret = HAL_HCD_HC_SubmitRequest(&uhc_cfg->hcd,
								 xfer->port_num,
								 (xfer->ep_desc->bEndpointAddress & 0x80) >> 7,
								 xfer->ep_desc->bmAttributes,
								 xfer->token,
								 xfer->buffer,
								 xfer->nbytes,
								 0);
	if(ret != HAL_OK)
	{
		uhc_cfg->port[xfer->port_num].xfer = NULL;
		quec_print("transfer request fail");
		return -1;
	}

	return 0;
}

static int quec_stm32_reset(const struct device *dev)
{
	quec_udrv_mgr_t *uhc_cfg = dev->data;
	
	HAL_HCD_ResetPort(&uhc_cfg->hcd);
	quec_print("stm32 reset usb");
	return 0;
}

static int quec_stm32_init(const struct device *dev, quec_uhc_drv_cb_t callback)
{
	quec_udrv_mgr_t *uhc_cfg = dev->data;

	uhc_cfg->callback = callback;
	if(uhc_stm32_hw_init(&uhc_cfg->hcd) != 0)
	{
		return -1;
	}

	return 0;
}

static int quec_stm32_deinit(const struct device *dev)
{
	quec_udrv_mgr_t *uhc_cfg = dev->data;

	uhc_cfg->callback = NULL;
	irq_disable(USB_DRD_FS_IRQn);
	HAL_HCD_Stop(&uhc_cfg->hcd);	
	HAL_HCD_DeInit(&uhc_cfg->hcd);

	return 0;
}

static int quec_stm32_set_address(const struct device *dev, uint8_t address)
{
	quec_udrv_mgr_t *uhc_cfg = dev->data;
	
	uhc_cfg->dev_address = address;
	return 0;
}

static int quec_stm32_ep_enable(const struct device *dev, usb_endp_desc_t *ep_desc)
{
	int ret = 0;
	uint8_t port_num = 0;
	quec_udrv_mgr_t *uhc_cfg = dev->data;

	if(ep_desc->bEndpointAddress == 0x80 || ep_desc->bEndpointAddress == 0)
	{
		port_num = 0;
	}
	else
	{
		port_num = quec_stm32_alloc_port(uhc_cfg);
	}

	if(port_num == UHC_PORT_INVALID)
	{
		quec_print("no valid port");
		return -1;
	}

	ret = HAL_HCD_HC_Init(&uhc_cfg->hcd,
						  port_num,
						  ep_desc->bEndpointAddress,
						  uhc_cfg->dev_address,
						  USB_DRD_SPEED_FS,
						  ep_desc->bmAttributes,
						  ep_desc->wMaxPacketSize);
	if(ret != 0)
	{
		quec_print("ep 0x%x enable failed", ep_desc->bEndpointAddress);
		return -1;
	}

	return port_num;
}

static int quec_stm32_ep_disable(const struct device *dev, uint8_t port)
{
	quec_udrv_mgr_t *uhc_cfg = dev->data;
	
    HAL_HCD_HC_Halt(&uhc_cfg->hcd, port);
	HAL_HCD_HC_Close(&uhc_cfg->hcd, port);
	quec_stm32_free_port(uhc_cfg, port);

	return 0;
}

static const quec_udrv_api_t uhc_stm32_api = {
	.init = quec_stm32_init,
	.deinit = quec_stm32_deinit,
	.reset = quec_stm32_reset,
	.set_address = quec_stm32_set_address,
	.ep_enable = quec_stm32_ep_enable,
	.ep_disable = quec_stm32_ep_disable,
	.enqueue = quec_stm32_enqueue,
};

DEVICE_DEFINE(QCX216, "QCX216", quec_stm32_chip_init,
					NULL, &uhc_control, NULL,
					POST_KERNEL, 98,
					&uhc_stm32_api);

