/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_uhc_ip3516hs

#include <soc.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/drivers/pinctrl.h>

#include "uhc_common.h"
#include "usb.h"
#include "usb_host_config.h"
#include "usb_host_mcux_drv_port.h"
#include "uhc_mcux_common.h"
#include "usb_host_ip3516hs.h"
#include "usb_phy.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uhc_mcux);

K_MEM_SLAB_DEFINE_STATIC(mcux_uhc_transfer_pool, sizeof(usb_host_transfer_t),
			 USB_HOST_CONFIG_MAX_TRANSFERS, sizeof(void *));

#define PRV_DATA_HANDLE(_handle) CONTAINER_OF(_handle, struct uhc_mcux_data, mcux_host)

struct uhc_mcux_ip3516hs_config {
	struct uhc_mcux_config uhc_config;
	usb_phy_config_struct_t *phy_config;
};

static void uhc_mcux_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct uhc_mcux_data *priv = uhc_get_private(dev);

	while (true) {
		USB_HostIp3516HsTaskFunction((void *)(&priv->mcux_host));
	}
}

static void uhc_mcux_isr(const struct device *dev)
{
	struct uhc_mcux_data *priv = uhc_get_private(dev);

	USB_HostIp3516HsIsrFunction((void *)(&priv->mcux_host));
}

/* MCUX controller dirver uses this callback to notify upper layer suspend related event */
static usb_status_t mcux_host_callback(usb_device_handle deviceHandle,
				       usb_host_configuration_handle configurationHandle,
				       uint32_t eventCode)
{
	/* TODO: nothing need to do here currently.
	 * It will be used when implementing suspend/resume in future.
	 */
	return kStatus_USB_Success;
}

static int uhc_mcux_init(const struct device *dev)
{
	const struct uhc_mcux_config *config = dev->config;
	usb_phy_config_struct_t *phy_config;
	struct uhc_mcux_data *priv = uhc_get_private(dev);
	k_thread_entry_t thread_entry = NULL;
	usb_status_t status;

	if ((priv->controller_id >= kUSB_ControllerIp3516Hs0) &&
	    (priv->controller_id <= kUSB_ControllerIp3516Hs1)) {
		thread_entry = uhc_mcux_thread;
	}

	if (thread_entry == NULL) {
		return -ENOMEM;
	}

#ifdef CONFIG_DT_HAS_NXP_USBPHY_ENABLED
	phy_config = ((const struct uhc_mcux_ip3516hs_config *)dev->config)->phy_config;
	if (phy_config != NULL) {
		USB_EhciPhyInit(priv->controller_id, 0u, phy_config);
	}
#endif

	priv->dev = dev;
	/* Init MCUX USB host driver. */
	priv->mcux_host.deviceCallback = mcux_host_callback;
	status = priv->mcux_if->controllerCreate(priv->controller_id, &priv->mcux_host,
						 &(priv->mcux_host.controllerHandle));
	if (status != kStatus_USB_Success) {
		return -ENOMEM;
	}

	/* Create MCUX USB host driver task */
	k_thread_create(&priv->drv_stack_data, config->drv_stack, CONFIG_UHC_NXP_THREAD_STACK_SIZE,
			thread_entry, (void *)dev, NULL, NULL, K_PRIO_COOP(2), 0, K_NO_WAIT);
	k_thread_name_set(&priv->drv_stack_data, "uhc_mcux_ip3516hs");

	return 0;
}

/* MCUX hal controller driver transfer callback */
static void uhc_mcux_transfer_callback(void *param, usb_host_transfer_t *transfer,
				       usb_status_t status)
{
	const struct device *dev = (const struct device *)param;
	struct uhc_transfer *const xfer = (struct uhc_transfer *const)(transfer->uhc_xfer);
	int err = -EIO;

	if (status == kStatus_USB_Success) {
		err = 0;

		if ((xfer->ep == 0) && (transfer->setupPacket->bRequest == USB_SREQ_SET_ADDRESS)) {
			usb_host_pipe_t *mcux_ep;

			/* MCUX hal controller driver needs set the ep0's address
			 * by using kUSB_HostUpdateControlEndpointAddress after set_address.
			 * When executing kUSB_HostUpdateControlEndpointAddress,
			 * USB_HostHelperGetPeripheralInformation is called from MCUX hal
			 * controller driver to get the address. But
			 * USB_HostHelperGetPeripheralInformation can't get right address for udev
			 * before this set_address control transfer result call back to host stack.
			 * So, add one flag and execute kUSB_HostUpdateControlEndpointAddress later
			 * when there is ep0 transfer.
			 */
			mcux_ep = uhc_mcux_init_hal_ep(dev, xfer);
			if (mcux_ep != NULL) {
				mcux_ep->update_addr = 1U;
			}
		}
	}

	if ((xfer->buf != NULL) && (transfer->transferBuffer != NULL)) {
		if (transfer->transferSofar > 0) {
			net_buf_add(xfer->buf, transfer->transferSofar);
		}
	}

	transfer->setupPacket = NULL;
	transfer->transferBuffer = NULL;
	k_mem_slab_free(&mcux_uhc_transfer_pool, transfer);
	uhc_xfer_return(dev, xfer, err);
}

static usb_host_transfer_t *uhc_mcux_hal_init_transfer(const struct device *dev,
						       usb_host_pipe_handle mcux_ep_handle,
						       struct uhc_transfer *const xfer)
{
	usb_host_transfer_t *mcux_xfer;

	if (k_mem_slab_alloc(&mcux_uhc_transfer_pool, (void **)&mcux_xfer, K_NO_WAIT)) {
		return NULL;
	}
	(void)uhc_mcux_hal_init_transfer_common(dev, mcux_xfer, mcux_ep_handle, xfer,
						uhc_mcux_transfer_callback);

	return mcux_xfer;
}

static int uhc_mcux_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	usb_host_pipe_t *mcux_ep;
	struct uhc_mcux_data *priv = uhc_get_private(dev);
	usb_status_t status;
	usb_host_transfer_t *mcux_xfer;

	uhc_xfer_append(dev, xfer);

	/* firstly check and init the mcux endpoint handle */
	mcux_ep = uhc_mcux_init_hal_ep(dev, xfer);
	if (mcux_ep == NULL) {
		return -ENOMEM;
	}

	/* Initialize MCUX hal transfer. */
	mcux_xfer = uhc_mcux_hal_init_transfer(dev, mcux_ep, xfer);
	if (mcux_xfer == NULL) {
		return -ENOMEM;
	}

	if (mcux_ep->endpointAddress == 0 && mcux_ep->update_addr) {
		mcux_ep->update_addr = 0;
		/* update control endpoint address here */
		uhc_mcux_control(dev, kUSB_HostUpdateControlEndpointAddress, mcux_ep);
	}

	uhc_mcux_lock(dev);
	/* give the transfer to MCUX drv. */
	status = priv->mcux_if->controllerWritePipe(priv->mcux_host.controllerHandle,
						    mcux_ep, mcux_xfer);
	uhc_mcux_unlock(dev);
	if (status != kStatus_USB_Success) {
		return -ENOMEM;
	}

	return 0;
}

static inline void uhc_mcux_get_hal_driver_id(struct uhc_mcux_data *priv,
					      const struct uhc_mcux_config *config)
{
	/*
	 * MCUX USB controller drivers use an ID to tell the HAL drivers
	 * which controller is being used. This part of the code converts
	 * the base address to the ID value.
	 */
	uintptr_t usb_base_addrs[] = USBHSH_BASE_ADDRS;

	/* get the right controller id */
	priv->controller_id = 0xFFu; /* invalid value */
	for (uint8_t i = 0; i < ARRAY_SIZE(usb_base_addrs); i++) {
		if (usb_base_addrs[i] == config->base) {
			priv->controller_id = kUSB_ControllerIp3516Hs0 + i;
			break;
		}
	}
}

static int uhc_mcux_driver_preinit(const struct device *dev)
{
	const struct uhc_mcux_config *config = dev->config;
	struct uhc_data *data = dev->data;
	struct uhc_mcux_data *priv = uhc_get_private(dev);

	uhc_mcux_get_hal_driver_id(priv, config);
	if (priv->controller_id == 0xFFu) {
		return -ENOMEM;
	}

	k_mutex_init(&data->mutex);
	pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	LOG_DBG("MCUX controller initialized");

	return 0;
}

static const struct uhc_api mcux_uhc_api = {
	.lock = uhc_mcux_lock,
	.unlock = uhc_mcux_unlock,
	.init = uhc_mcux_init,
	.enable = uhc_mcux_enable,
	.disable = uhc_mcux_disable,
	.shutdown = uhc_mcux_shutdown,

	.bus_reset = uhc_mcux_bus_reset,
	.sof_enable = uhc_mcux_sof_enable,
	.bus_suspend = uhc_mcux_bus_suspend,
	.bus_resume = uhc_mcux_bus_resume,

	.ep_enqueue = uhc_mcux_enqueue,
	.ep_dequeue = uhc_mcux_dequeue,
};

static const usb_host_controller_interface_t uhc_mcux_if = {
	USB_HostIp3516HsCreate,    USB_HostIp3516HsDestory,   USB_HostIp3516HsOpenPipe,
	USB_HostIp3516HsClosePipe, USB_HostIp3516HsWritePipe, USB_HostIp3516HsReadPipe,
	USB_HostIp3516HsIoctl,
};

#define UHC_MCUX_PHY_DEFINE(n)                                                                     \
	static usb_phy_config_struct_t phy_config_##n = {                                          \
		.D_CAL = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), tx_d_cal, 0),                  \
		.TXCAL45DP = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), tx_cal_45_dp_ohms, 0),     \
		.TXCAL45DM = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), tx_cal_45_dm_ohms, 0),     \
	}

#define UHC_MCUX_PHY_DEFINE_OR(n)                                                                  \
	COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), phy_handle), (UHC_MCUX_PHY_DEFINE(n)), ())

#define UHC_MCUX_PHY_CFG_PTR_OR_NULL(n)                                                            \
	COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), phy_handle), (&phy_config_##n), (NULL))

#define UHC_MCUX_IP3516HS_DEVICE_DEFINE(n)                                                         \
	UHC_MCUX_PHY_DEFINE_OR(n);                                                                 \
                                                                                                   \
	static void uhc_irq_enable_func##n(const struct device *dev)                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uhc_mcux_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static void uhc_irq_disable_func##n(const struct device *dev)                              \
	{                                                                                          \
		irq_disable(DT_INST_IRQN(n));                                                      \
	}                                                                                          \
                                                                                                   \
	static K_KERNEL_STACK_DEFINE(drv_stack_##n, CONFIG_UHC_NXP_THREAD_STACK_SIZE);             \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static struct uhc_mcux_ip3516hs_config priv_config_##n = {                                 \
		.uhc_config = {                                                                    \
			.base = DT_INST_REG_ADDR(n),                                               \
			.irq_enable_func = uhc_irq_enable_func##n,                                 \
			.irq_disable_func = uhc_irq_disable_func##n,                               \
			.drv_stack = drv_stack_##n,                                                \
			.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                               \
		},                                                                                 \
		.phy_config = UHC_MCUX_PHY_CFG_PTR_OR_NULL(n),                                     \
	};                                                                                         \
                                                                                                   \
	static struct uhc_mcux_data priv_data_##n = {                                              \
		.mcux_if = &uhc_mcux_if,                                                           \
	};                                                                                         \
                                                                                                   \
	static struct uhc_data uhc_data_##n = {                                                    \
		.mutex = Z_MUTEX_INITIALIZER(uhc_data_##n.mutex),                                  \
		.priv = &priv_data_##n,                                                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uhc_mcux_driver_preinit, NULL, &uhc_data_##n, &priv_config_##n,   \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mcux_uhc_api);

DT_INST_FOREACH_STATUS_OKAY(UHC_MCUX_IP3516HS_DEVICE_DEFINE)
