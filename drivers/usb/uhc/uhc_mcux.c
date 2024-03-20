/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/usb/uhc.h>

#include "uhc_common.h"
#include "usb.h"
#include "usb_host_config.h"
#include "usb_host_mcux_drv_port.h"
#include "usbh_device.h"
#ifdef CONFIG_USB_UHC_NXP_PHY
#include "usb_phy.h"
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_mcux, CONFIG_UHC_DRIVER_LOG_LEVEL);

static K_KERNEL_STACK_DEFINE(drv_stack, CONFIG_UHC_MCUX_THREAD_STACK_SIZE);

K_MEM_SLAB_DEFINE_STATIC(mcux_uhc_transfer_pool, sizeof(usb_host_transfer_t),
			 USB_HOST_CONFIG_MAX_TRANSFERS, sizeof(void *));

#if defined(CONFIG_NOCACHE_MEMORY)
K_HEAP_DEFINE_NOCACHE(mcux_transfer_alloc_pool, USB_HOST_CONFIG_MAX_TRANSFERS * 8u +
	1024u * USB_HOST_CONFIG_MAX_TRANSFERS);
#endif

#define PRV_DATA_HANDLE(_handle) CONTAINER_OF(_handle, struct uhc_mcux_data, host_instance)

#ifdef CONFIG_USB_UHC_NXP_EHCI
#include "usb_host_ehci.h"
static const usb_host_controller_interface_t mcux_ehci_usb_iface = {
	USB_HostEhciCreate,	USB_HostEhciDestory,  USB_HostEhciOpenPipe, USB_HostEhciClosePipe,
	USB_HostEhciWritePipe, USB_HostEhciReadpipe, USB_HostEhciIoctl,
};
#endif

#ifdef CONFIG_USB_UHC_NXP_KHCI
#include "usb_host_khci.h"
static const usb_host_controller_interface_t mcux_khci_usb_iface = {
	USB_HostKhciCreate,	USB_HostKhciDestory,  USB_HostKhciOpenPipe, USB_HostKhciClosePipe,
	USB_HostKhciWritePipe, USB_HostKhciReadpipe, USB_HostKciIoctl,
};
#endif

struct uhc_mcux_ep_handle {
	uint8_t ep;
	void *udev;
	usb_host_pipe_handle mcux_ep_handle;
};

struct uhc_mcux_data {
	const struct device *dev;
	struct uhc_mcux_ep_handle ep_handles[USB_HOST_CONFIG_MAX_PIPES];
	usb_host_instance_t host_instance;
	struct k_thread drv_stack_data;
	bool need_update_addr;
	uint8_t top_device_speed;
	uint8_t controller_id;
};

struct uhc_mcux_config {
	const usb_host_controller_interface_t *mcux_if;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	uint32_t base;
#ifdef CONFIG_USB_UHC_NXP_PHY
	uint8_t need_to_init_phy;
	uint8_t D_CAL;
	uint8_t TXCAL45DP;
	uint8_t TXCAL45DM;
#endif
};

#if defined(CONFIG_NOCACHE_MEMORY)
/* allocate non-cached buffer for usb */
static void *uhc_mcux_nocache_alloc(uint32_t size)
{
	void *p = (void *)k_heap_alloc(&mcux_transfer_alloc_pool, size, K_NO_WAIT);

	if (p != NULL) {
		(void)memset(p, 0, size);
	}

	return p;
}

/* free the allocated non-cached buffer */
static void uhc_mcux_nocache_free(void *p)
{
	k_heap_free(&mcux_transfer_alloc_pool, p);
}
#endif

static int uhc_mcux_bus_control(const struct device *dev, usb_host_bus_control_t type)
{
	const struct uhc_mcux_config *config = dev->config;
	struct uhc_data *data = dev->data;
	struct uhc_mcux_data *priv = data->priv;
	usb_status_t status;

	status = config->mcux_if->controllerIoctl(
			priv->host_instance.controllerHandle, kUSB_HostBusControl, &type);
	if (status != kStatus_USB_Success) {
		return -EIO;
	}
	return 0;
}

/*!
 * @brief Controller driver calls this function when device attach.
 *
 * This function will be called by the MCUX SDK host controller layer.
 *
 * @param hostHandle    Host instance handle.
 * @param speed         Device speed.
 * @param hubNumber     Device hub no. root device's hub no. is 0.
 * @param portNumber    Device port no. root device's port no. is 0.
 * @param level         Device level. root device's level is 1.
 * @param deviceHandle  Return device handle.
 *
 * @return kStatus_USB_Success or error codes.
 */
usb_status_t USB_HostAttachDevice(usb_host_handle hostHandle,
				uint8_t speed,
				uint8_t hubNumber,
				uint8_t portNumber,
				uint8_t level,
				usb_device_handle *deviceHandle)
{
	enum uhc_event_type type;
	struct uhc_mcux_data *priv;

	if (speed == USB_SPEED_HIGH) {
		type = UHC_EVT_DEV_CONNECTED_HS;
	} else if (speed == USB_SPEED_FULL) {
		type = UHC_EVT_DEV_CONNECTED_FS;
	} else {
		type = UHC_EVT_DEV_CONNECTED_LS;
	}

	priv = (struct uhc_mcux_data *)(PRV_DATA_HANDLE(hostHandle));
	priv->top_device_speed = speed;
	uhc_submit_event(priv->dev, type, 0);
	return kStatus_USB_Success;
}

/*!
 * @brief Controller driver calls this function when device detaches.
 *
 * This function will be called by the MCUX SDK host controller layer.
 *
 * @param hostHandle   Host instance handle.
 * @param hubNumber    Device hub no. root device's hub no. is 0.
 * @param portNumber   Device port no. root device's port no. is 0.
 *
 * @return kStatus_USB_Success or error codes.
 */
usb_status_t USB_HostDetachDevice(usb_host_handle hostHandle, uint8_t hubNumber, uint8_t portNumber)
{
	struct uhc_mcux_data *priv;

	priv = (struct uhc_mcux_data *)(PRV_DATA_HANDLE(hostHandle));
	uhc_submit_event(priv->dev, UHC_EVT_DEV_REMOVED, 0);
	uhc_mcux_bus_control(priv->dev, kUSB_HostBusEnableAttach);
	return kStatus_USB_Success;
}

/*!
 * @brief Controller driver calls this function to get the device information.
 *
 * This function gets the device information.
 * This function will be called by the MCUX SDK host controller layer.
 *
 * @param[in] deviceHandle   Removing device handle.
 * @param[in] infoCode       See the enumeration host_dev_info_t.
 * @param[out] infoValue     Return the information value.
 *
 * @retval kStatus_USB_Success           Close successfully.
 * @retval kStatus_USB_InvalidParameter  The deviceHandle or info_value is a NULL pointer.
 * @retval kStatus_USB_Error             The info_code is not the host_dev_info_t value.
 */
usb_status_t USB_HostHelperGetPeripheralInformation(usb_device_handle deviceHandle,
				uint32_t infoCode,
				uint32_t *infoValue)
{
	struct usb_device *device = (struct usb_device *)deviceHandle;
	const struct device *dev = device->ctx->dev;
	struct uhc_data *data = dev->data;
	struct uhc_mcux_data *priv = data->priv;
	usb_host_dev_info_t devInfo;

	if ((deviceHandle == NULL) || (infoValue == NULL)) {
		return kStatus_USB_InvalidParameter;
	}
	devInfo = (usb_host_dev_info_t)infoCode;
	switch (devInfo) {
	case kUSB_HostGetDeviceAddress: /* device address */
		*infoValue = device->addr;
		break;

	case kUSB_HostGetDeviceHubNumber:   /* device hub address */
	case kUSB_HostGetDevicePortNumber:  /* device port no */
	case kUSB_HostGetDeviceHSHubNumber: /* device high-speed hub address */
	case kUSB_HostGetDeviceHSHubPort:   /* device high-speed hub port no */
	case kUSB_HostGetHubThinkTime:	  /* device hub think time */
		*infoValue = 0;
		break;
	case kUSB_HostGetDeviceLevel: /* device level */
		*infoValue = 1;
		break;

	case kUSB_HostGetDeviceSpeed: /* device speed */
		/* current usb host stack doesn't support hub,
		 * so only one device can connect, so it is OK.
		 */
		*infoValue = (uint32_t)priv->top_device_speed;
		break;

	default:
		/*no action*/
		break;
	}

	return kStatus_USB_Success;
}

/*!
 * @brief Host callback function.
 *
 * This function will be called by the MCUX SDK host controller layer.
 * This callback function is used to notify application device attach/detach event.
 * This callback pointer is passed when initializing the host.
 *
 * @param deviceHandle		The device handle, which indicates the attached device.
 * @param configurationHandle	The configuration handle contains the attached device's
 *							   configuration information.
 * @param event_code		The callback event code; See the enumeration host_event_t.
 *
 * @return A USB error code or kStatus_USB_Success.
 * @retval kStatus_USB_Success		Application handles the attached device successfully.
 * @retval kStatus_USB_NotSupported	Application don't support the attached device.
 * @retval kStatus_USB_Error		Application handles the attached device falsely.
 */
static usb_status_t mcux_host_callback(usb_device_handle deviceHandle,
				usb_host_configuration_handle configurationHandle,
				uint32_t eventCode)
{
	/* TODO: nothing need to do here currently.
	 * It will be used when implementing suspend/resume in future.
	 */
	return kStatus_USB_Success;
}

#ifdef CONFIG_USB_UHC_NXP_EHCI
static void uhc_mcux_ehci_thread(void *p1, void *p2, void *p3)
{
	struct uhc_data *data = ((struct device *)(p1))->data;
	struct uhc_mcux_data *priv = data->priv;

	while (true) {
		USB_HostEhciTaskFunction((void *)(&priv->host_instance));
	}
}

static void uhc_mcux_ehci_isr(const struct device *dev)
{
	struct uhc_data *data = (dev)->data;
	struct uhc_mcux_data *priv = data->priv;

	USB_HostEhciIsrFunction((void *)(&priv->host_instance));
}
#endif

#ifdef CONFIG_USB_UHC_NXP_KHCI
static void uhc_mcux_khci_thread(void *p1, void *p2, void *p3)
{
	struct uhc_data *data = ((struct device *)(p1))->data;
	struct uhc_mcux_data *priv = data->priv;

	while (true) {
		USB_HostKhciTaskFunction((void *)(&priv->host_instance));
	}
}

static void uhc_mcux_khci_isr(const struct device *dev)
{
	struct uhc_data *data = ((struct device *)(dev))->data;
	struct uhc_mcux_data *priv = data->priv;

	USB_HostKhciIsrFunction((void *)(&priv->host_instance));
}
#endif

static int uhc_mcux_lock(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_lock(&data->mutex, K_FOREVER);
}

static int uhc_mcux_unlock(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_unlock(&data->mutex);
}

static int uhc_mcux_init(const struct device *dev)
{
#ifdef CONFIG_USB_UHC_NXP_PHY
	usb_phy_config_struct_t phy_config;
#endif
	const struct uhc_mcux_config *config = dev->config;
	struct uhc_data *data = dev->data;
	struct uhc_mcux_data *priv = data->priv;
	k_thread_entry_t thread_entry = NULL;
	usb_status_t status;
	uint8_t index;
#ifdef CONFIG_USB_UHC_NXP_EHCI
#if defined(USBHS_STACK_BASE_ADDRS)
	uint32_t usb_base_addrs[] = USBHS_STACK_BASE_ADDRS;
#else
	uint32_t usb_base_addrs[] = USBHS_BASE_ADDRS;
#endif
#endif
#ifdef CONFIG_USB_UHC_NXP_KHCI
	uint32_t usb_base_addrs[] = USB_BASE_ADDRS;
#endif

	/* get the right controller id */
	priv->controller_id = 0xFFu; /* invalid value */
#ifdef CONFIG_USB_UHC_NXP_EHCI
	for (index = 0; index < ARRAY_SIZE(usb_base_addrs); index++) {
		if (usb_base_addrs[index] == config->base) {
			priv->controller_id = kUSB_ControllerEhci0 + index;
			break;
		}
	}

	if ((priv->controller_id >= kUSB_ControllerEhci0) &&
		(priv->controller_id <= kUSB_ControllerEhci1)) {
		thread_entry = uhc_mcux_ehci_thread;
	}
#endif
#ifdef CONFIG_USB_UHC_NXP_KHCI
	for (index = 0; index < ARRAY_SIZE(usb_base_addrs); index++) {
		if (usb_base_addrs[index] == config->base) {
			priv->controller_id = kUSB_ControllerKhci0 + index;
			break;
		}
	}

	if ((priv->controller_id >= kUSB_ControllerKhci0) &&
		(priv->controller_id <= kUSB_ControllerKhci1)) {
		thread_entry = uhc_mcux_khci_thread;
	}
#endif
	if (thread_entry == NULL) {
		return -ENOMEM;
	}

#ifdef CONFIG_USB_UHC_NXP_PHY
	if (config->need_to_init_phy) {
		phy_config.D_CAL = config->D_CAL;
		phy_config.TXCAL45DP = config->TXCAL45DP;
		phy_config.TXCAL45DM = config->TXCAL45DM;
		USB_EhciPhyInit(priv->controller_id, 0u, &phy_config);
	}
#endif

	priv->dev = dev;
	/* Init MCUX USB host driver. */
	priv->host_instance.deviceCallback = mcux_host_callback;
	status = config->mcux_if->controllerCreate(priv->controller_id,
		&priv->host_instance, &(priv->host_instance.controllerHandle));
	if (status != kStatus_USB_Success) {
		return -ENOMEM;
	}

	/* Create MCUX USB host driver task */
	k_mutex_init(&data->mutex);
	if (thread_entry != NULL) {
		k_thread_create(&priv->drv_stack_data, drv_stack,
				K_KERNEL_STACK_SIZEOF(drv_stack),
				thread_entry,
				(void *)dev, NULL, NULL,
				K_PRIO_COOP(2), 0, K_NO_WAIT);
		k_thread_name_set(&priv->drv_stack_data, "uhc_mcux");
	}

	return 0;
}

static int uhc_mcux_enable(const struct device *dev)
{
	const struct uhc_mcux_config *config = dev->config;

	/* enable interrupt. */
	config->irq_enable_func(dev);
	return 0;
}

static int uhc_mcux_disable(const struct device *dev)
{
	const struct uhc_mcux_config *config = dev->config;

	/* disable interrupt. */
	config->irq_disable_func(dev);
	return 0;
}

static int uhc_mcux_shutdown(const struct device *dev)
{
	const struct uhc_mcux_config *config = dev->config;
	struct uhc_data *data = dev->data;
	struct uhc_mcux_data *priv = data->priv;

	/* disable interrupt. */
	config->irq_disable_func(dev);
	/* disable MCUX USB host drive task */
	k_thread_abort(&priv->drv_stack_data);
	/* de-init USB Host driver */
	config->mcux_if->controllerDestory(priv->host_instance.controllerHandle);
	return 0;
}

/* Signal bus reset, 50ms SE0 signal */
static int uhc_mcux_bus_reset(const struct device *dev)
{
	struct uhc_data *data = dev->data;
	struct uhc_mcux_data *priv = data->priv;
	int ret;

	ret = uhc_mcux_bus_control(dev, kUSB_HostBusReset);
	if (ret == 0) {
		ret = uhc_mcux_bus_control(dev, kUSB_HostBusRestart);
	}

	if (ret == 0) {
		priv->need_update_addr = true;
	}
	return ret;
}

/* Enable SOF generator */
static int uhc_mcux_sof_enable(const struct device *dev)
{
	/* MCUX doesn't need it. */
	return 0;
}

/* Disable SOF generator and suspend bus */
static int uhc_mcux_bus_suspend(const struct device *dev)
{
	return uhc_mcux_bus_control(dev, kUSB_HostBusSuspend);
}

/* Signal bus resume event, 20ms K-state + low-speed EOP */
static int uhc_mcux_bus_resume(const struct device *dev)
{
	return uhc_mcux_bus_control(dev, kUSB_HostBusResume);
}

static void uhc_mcux_transfer_callback(void *param, usb_host_transfer_t *transfer,
		usb_status_t status)
{
	const struct device *dev = (const struct device *)param;
	struct uhc_transfer *const xfer = (struct uhc_transfer *const)(transfer->uhc_xfer);
	struct uhc_data *data = dev->data;
	struct uhc_mcux_data *priv = data->priv;
	int err = -EIO;

	if (status == kStatus_USB_Success) {
		err = 0;

		/* TODO: temp workaround because current stack doesn't support it. */
		if ((xfer->ep == 0) && (transfer->setupPacket->bRequest == USB_SREQ_SET_ADDRESS)) {
			priv->need_update_addr = true;
		}
	}

#if defined(CONFIG_NOCACHE_MEMORY)
	if (transfer->setupPacket != NULL) {
		uhc_mcux_nocache_free(transfer->setupPacket);
	}
#endif
	if ((xfer->buf != NULL) && (transfer->transferBuffer != NULL)) {
		if (transfer->transferSofar > 0) {
#if defined(CONFIG_NOCACHE_MEMORY)
			memcpy(xfer->buf->__buf, transfer->transferBuffer, transfer->transferSofar);
#endif
			net_buf_add(xfer->buf, transfer->transferSofar);
		}
#if defined(CONFIG_NOCACHE_MEMORY)
		uhc_mcux_nocache_free(transfer->transferBuffer);
#endif
	}

	transfer->setupPacket = NULL;
	transfer->transferBuffer = NULL;
	k_mem_slab_free(&mcux_uhc_transfer_pool, transfer);
	uhc_xfer_return(dev, xfer, err);
}

static int uhc_mcux_enqueue(const struct device *dev,
				struct uhc_transfer *const xfer)
{
	uint8_t index;
	usb_host_pipe_handle mcux_ep_handle = NULL;
	const struct uhc_mcux_config *config = dev->config;
	struct uhc_data *data = dev->data;
	struct uhc_mcux_data *priv = data->priv;
	usb_status_t status;
	usb_host_transfer_t *mcux_xfer;
	uint8_t ep;

	uhc_xfer_append(dev, xfer);

	/* firstly check and init the mcux endpoint handle */
	for (index = 0; index < USB_HOST_CONFIG_MAX_PIPES; ++index) {
		ep = xfer->ep;
		if (ep == 0x80) {
			ep = 0;
		}

		if ((priv->ep_handles[index].mcux_ep_handle != NULL) &&
			(priv->ep_handles[index].ep == ep) &&
			(priv->ep_handles[index].udev == xfer->udev)) {
			mcux_ep_handle = priv->ep_handles[index].mcux_ep_handle;
			break;
		}
	}

	/* Initialize mcux endpoint pipe */
	if (mcux_ep_handle == NULL) {
		usb_host_pipe_init_t pipe_init;

		for (index = 0; index < USB_HOST_CONFIG_MAX_PIPES; ++index) {
			if (priv->ep_handles[index].mcux_ep_handle == NULL) {
				break;
			}
		}

		if (index >= USB_HOST_CONFIG_MAX_PIPES) {
			return -ENOMEM;
		}

		pipe_init.devInstance = xfer->udev;
		pipe_init.nakCount = USB_HOST_CONFIG_MAX_NAK;
		pipe_init.maxPacketSize = xfer->mps;
		pipe_init.endpointAddress = xfer->ep & 0x7Fu;
		if (xfer->ep & 0x80u) {
			pipe_init.direction = USB_IN;
		} else {
			pipe_init.direction = USB_OUT;
		}
		/* Current Zephyr Host stack is experimental, the endpoint's interval,
		 * 'number per uframe' and the endpoint type cannot be got yet.
		 */
		pipe_init.numberPerUframe = 0; /* TODO: need right way to implement it. */
		pipe_init.interval = 0; /* TODO: need right way to implement it. */
		/* TODO: need right way to implement it. */
		if (xfer->ep == 0) {
			pipe_init.pipeType = USB_ENDPOINT_CONTROL;
		} else {
			pipe_init.pipeType = USB_ENDPOINT_BULK;
		}

		status = config->mcux_if->controllerOpenPipe(priv->host_instance.controllerHandle,
			&mcux_ep_handle, &pipe_init);
		priv->ep_handles[index].mcux_ep_handle = mcux_ep_handle;
		priv->ep_handles[index].udev = xfer->udev;

		if (status != kStatus_USB_Success) {
			return -ENOMEM;
		}
	}

	/* give the transfer to MCUX drv. */
	if (k_mem_slab_alloc(&mcux_uhc_transfer_pool, (void **)&mcux_xfer, K_NO_WAIT)) {
		return -ENOMEM;
	}
	mcux_xfer->uhc_xfer = xfer;
	mcux_xfer->transferPipe = mcux_ep_handle;
	mcux_xfer->transferSofar = 0;
	mcux_xfer->next = NULL;
	mcux_xfer->setupStatus = 0;
	mcux_xfer->callbackFn = uhc_mcux_transfer_callback;
	mcux_xfer->callbackParam = (void *)dev;
#if defined(CONFIG_NOCACHE_MEMORY)
	mcux_xfer->setupPacket = uhc_mcux_nocache_alloc(8u);
	memcpy(mcux_xfer->setupPacket, xfer->setup_pkt, 8u);
#else
	mcux_xfer->setupPacket = (usb_setup_struct_t *)&xfer->setup_pkt[0];
#endif
	if (xfer->buf != NULL) {
		mcux_xfer->transferLength = xfer->buf->size;
#if defined(CONFIG_NOCACHE_MEMORY)
		mcux_xfer->transferBuffer = uhc_mcux_nocache_alloc(mcux_xfer->transferLength);
#else
		mcux_xfer->transferBuffer = xfer->buf->__buf;
#endif
	} else {
		mcux_xfer->transferBuffer = NULL;
		mcux_xfer->transferLength = 0;
	}

	if ((xfer->ep == 0) || (xfer->ep == 0x80)) {
		if (priv->need_update_addr) {
			priv->need_update_addr = false;
			(void)config->mcux_if->controllerIoctl(
					priv->host_instance.controllerHandle,
					kUSB_HostUpdateControlEndpointAddress,
					mcux_ep_handle);
		}
		if ((mcux_xfer->setupPacket->bmRequestType & USB_REQUEST_TYPE_DIR_MASK) ==
			USB_REQUEST_TYPE_DIR_IN) {
			mcux_xfer->direction = USB_IN;
		} else {
			mcux_xfer->direction = USB_OUT;
		}
		uhc_mcux_lock(dev);
		status = config->mcux_if->controllerWritePipe(priv->host_instance.controllerHandle,
			mcux_ep_handle, mcux_xfer);
		uhc_mcux_unlock(dev);
		if (status != kStatus_USB_Success) {
			return -ENOMEM;
		}
	}

	return 0;
}

static int uhc_mcux_dequeue(const struct device *dev,
				struct uhc_transfer *const xfer)
{
	/* TODO */
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
	.sof_enable  = uhc_mcux_sof_enable,
	.bus_suspend = uhc_mcux_bus_suspend,
	.bus_resume = uhc_mcux_bus_resume,

	.ep_enqueue = uhc_mcux_enqueue,
	.ep_dequeue = uhc_mcux_dequeue,
};

static int mcux_driver_preinit(const struct device *dev)
{
	LOG_DBG("MCUX controller initialized");

	return 0;
}

#ifdef CONFIG_USB_UHC_NXP_EHCI
#define DT_DRV_COMPAT nxp_uhc_ehci

#ifdef CONFIG_USB_UHC_NXP_PHY
#define UHC_MCUX_PHY_CONFIGURE(n) \
	.need_to_init_phy = DT_NODE_HAS_PROP(DT_DRV_INST(n), phy_handle) ? 1u : 0u,	\
	.D_CAL = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), d_cal, 0),			\
	.TXCAL45DP = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), txcal45dp, 0),		\
	.TXCAL45DM = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), txcal45dm, 0),
#else
#define UHC_MCUX_PHY_CONFIGURE(n)
#endif


#define UHC_MCUX_EHCI_DEVICE_DEFINE(n)							\
	static void uhc_irq_enable_func##n(const struct device *dev)			\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n),						\
				DT_INST_IRQ(n, priority),				\
				uhc_mcux_ehci_isr,					\
				DEVICE_DT_INST_GET(n), 0);				\
											\
		irq_enable(DT_INST_IRQN(n));						\
	}										\
											\
	static void uhc_irq_disable_func##n(const struct device *dev)			\
	{										\
		irq_disable(DT_INST_IRQN(n));						\
	}										\
											\
	static struct uhc_mcux_config priv_config_##n = {				\
		UHC_MCUX_PHY_CONFIGURE(n)						\
		.irq_enable_func = uhc_irq_enable_func##n,				\
		.irq_disable_func = uhc_irq_disable_func##n,				\
		.base = (uint32_t) DT_INST_REG_ADDR(n),					\
		.mcux_if = &mcux_ehci_usb_iface,					\
	};										\
											\
	static struct uhc_mcux_data priv_data_##n = {					\
	};										\
											\
	static struct uhc_data uhc_data_##n = {						\
		.mutex = Z_MUTEX_INITIALIZER(uhc_data_##n.mutex),			\
		.priv = &priv_data_##n,							\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n, mcux_driver_preinit, NULL,				\
				  &uhc_data_##n, &priv_config_##n,			\
				  POST_KERNEL,						\
				  CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
				  &mcux_uhc_api);

DT_INST_FOREACH_STATUS_OKAY(UHC_MCUX_EHCI_DEVICE_DEFINE)

#undef DT_DRV_COMPAT
#endif

#ifdef CONFIG_USB_UHC_NXP_KHCI
#define DT_DRV_COMPAT nxp_uhc_khci

#ifdef CONFIG_USB_UHC_NXP_PHY
#define UHC_MCUX_PHY_CONFIGURE(n)							\
	.need_to_init_phy = 0u,
#else
#define UHC_MCUX_PHY_CONFIGURE(n)
#endif

#define UHC_MCUX_KHCI_DEVICE_DEFINE(n)							\
	static void uhc_irq_enable_func##n(const struct device *dev)			\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n),						\
				DT_INST_IRQ(n, priority),				\
				uhc_mcux_khci_isr,					\
				DEVICE_DT_INST_GET(n), 0);				\
											\
		irq_enable(DT_INST_IRQN(n));						\
	}										\
											\
	static void uhc_irq_disable_func##n(const struct device *dev)			\
	{										\
		irq_disable(DT_INST_IRQN(n));						\
	}										\
											\
	static struct uhc_mcux_config priv_config_##n = {				\
		UHC_MCUX_PHY_CONFIGURE(n)						\
		.irq_enable_func = uhc_irq_enable_func##n,				\
		.irq_disable_func = uhc_irq_disable_func##n,				\
		.base = (uint32_t) DT_INST_REG_ADDR(n),					\
		.mcux_if = &mcux_khci_usb_iface,					\
	};										\
											\
	static struct uhc_mcux_data priv_data_##n = {					\
	};										\
											\
	static struct uhc_data uhc_data_##n = {						\
		.mutex = Z_MUTEX_INITIALIZER(uhc_data_##n.mutex),			\
		.priv = &priv_data_##n,							\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n, mcux_driver_preinit, NULL,				\
				  &uhc_data_##n, &priv_config_##n,			\
				  POST_KERNEL,						\
				  CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
				  &mcux_uhc_api);

DT_INST_FOREACH_STATUS_OKAY(UHC_MCUX_KHCI_DEVICE_DEFINE)
#endif
