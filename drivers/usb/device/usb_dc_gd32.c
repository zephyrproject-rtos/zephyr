
/**
 * @file
 * @brief USB device controller for GD32 devices
 */

#include <soc.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>

#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(usb_dc_gd32);

#include "drv_usb_core.h"
#include "drv_usb_dev.h"
#include "drv_usbd_int.h"

#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_otghs)
#define DT_DRV_COMPAT    gd_gd32_otghs
#define USB_IRQ_NAME     otghs
#define USB_VBUS_SENSING (DT_NODE_EXISTS(DT_CHILD(DT_NODELABEL(pinctrl), usb_otg_hs_vbus_pa9)) || \
			  DT_NODE_EXISTS(DT_CHILD(DT_NODELABEL(pinctrl), usb_otg_hs_vbus_pb13)))
#elif DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_otgfs)
#define DT_DRV_COMPAT    gd_gd32_otgfs
#define USB_IRQ_NAME     otgfs
#define USB_VBUS_SENSING DT_NODE_EXISTS(DT_CHILD(DT_NODELABEL(pinctrl), usb_otg_fs_vbus_pa9))
#elif DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_usb)
#define DT_DRV_COMPAT    gd_gd32_usb
#define USB_IRQ_NAME     usb
#define USB_VBUS_SENSING false
#endif

#define USB_BASE_ADDRESS		DT_INST_REG_ADDR(0)
#define USB_IRQ					DT_INST_IRQ_BY_NAME(0, USB_IRQ_NAME, irq)
#define USB_IRQ_PRI				DT_INST_IRQ_BY_NAME(0, USB_IRQ_NAME, priority)
#define USB_NUM_BIDIR_ENDPOINTS	DT_INST_PROP(0, num_bidir_endpoints)
#define USB_RAM_SIZE			DT_INST_PROP(0, ram_size)


#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_otghs)
#define EP_MPS USBHS_MAX_PACKET_SIZE
#elif DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_otgfs) || DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_usb)
#define EP_MPS USBFS_MAX_PACKET_SIZE
#endif


/* Helper macros to make it easier to work with endpoint numbers */
#define EP0_IDX 0
#define EP0_MPS USB_CONTROL_EP_MPS

/* Size of a USB SETUP packet */
#define SETUP_SIZE 8

// struct usb_device_ep_state {
// 	uint16_t ep_mps;		/** Endpoint max packet size */
// 	uint16_t ep_pma_buf_len;	/** Previously allocated buffer size */
// 	uint8_t ep_type;		/** Endpoint type (STM32 HAL enum) */
// 	uint8_t ep_stalled;	/** Endpoint stall flag */
// 	usb_dc_ep_callback cb;	/** Endpoint callback function */
// 	uint32_t read_count;	/** Number of bytes in read buffer  */
// 	uint32_t read_offset;	/** Current offset in read buffer */
// 	struct k_sem write_sem;	/** Write boolean semaphore */
// };

/* Driver state */
struct usb_device_state {
    usb_core_driver usb_driver_st;
	usb_dc_status_callback status_cb; /* Status callback */
	// struct usb_device_ep_state out_ep_state[USB_NUM_BIDIR_ENDPOINTS];
	// struct usb_device_ep_state in_ep_state[USB_NUM_BIDIR_ENDPOINTS];
 	usb_dc_ep_callback cb[2][USB_NUM_BIDIR_ENDPOINTS];	/** Endpoint callback function */
 	uint32_t read_offset[USB_NUM_BIDIR_ENDPOINTS];	/** Current offset in read buffer */
	uint8_t ep_buf[USB_NUM_BIDIR_ENDPOINTS][EP_MPS];

#if defined(USB) || defined(USB_DRD_FS)
	uint32_t pma_offset;
#endif /* USB */
};
static struct usb_device_state usb_device_st;

/* Internal functions */
static usb_transc *usb_dc_gd32_get_ep_transc(uint8_t ep_addr)
{
	usb_transc *transc;

	if (USB_EP_GET_IDX(ep_addr) >= USB_NUM_BIDIR_ENDPOINTS) {
		return NULL;
	}

	// if (USB_EP_DIR_IS_OUT(ep_addr)) {
	// 	ep_state_base = &usb_device_st.out_ep_state[USB_EP_GET_IDX(ep_addr)];
	// } else {
	// 	ep_state_base = &usb_device_st.in_ep_state[USB_EP_GET_IDX(ep_addr)];
	// }

	if (USB_EP_DIR_IS_OUT(ep_addr)) {
		transc = &usb_device_st.usb_driver_st.dev.transc_out[USB_EP_GET_IDX(ep_addr)];
	} else {
		transc = &usb_device_st.usb_driver_st.dev.transc_in[USB_EP_GET_IDX(ep_addr)];
	}

	return transc;
}

static int usb_dc_reset_enum(void)
{
	usb_transc *transc = usb_dc_gd32_get_ep_transc(EP0_IN);
	usb_status status;

	if (!transc) {
		return -EINVAL;
	}

	transc->ep_type = USB_EPTYPE_CTRL;
	transc->max_len = USB_FS_EP0_MAX_LEN;

	status = usb_transc_active(&usb_device_st.usb_driver_st, transc);
	if (status != USB_OK) {
		LOG_ERR("usb_transc_active failed(0x%02x), %d", EP0_IN, (int)status);
		return -EIO;
	}

	transc = usb_dc_gd32_get_ep_transc(EP0_OUT);

	if (!transc) {
		return -EINVAL;
	}
	
	transc->ep_type = USB_EPTYPE_CTRL;
	transc->max_len = USB_FS_EP0_MAX_LEN;

	status = usb_transc_active(&usb_device_st.usb_driver_st, transc);
	if (status != USB_OK) {
		LOG_ERR("usb_transc_active failed(0x%02x), %d", EP0_OUT, (int)status);
		return -EIO;
	}
}

static void usb_dc_usbfs_gd32_isr(const void *arg)
{
	usb_core_driver *udev = &usb_device_st.usb_driver_st;
	static uint32_t intr = 0, intr_prev = 0, count = 0, count_1 = 0;
count++;
    if (HOST_MODE != (udev->regs.gr->GINTF & GINTF_COPM)) {
		intr = udev->regs.gr->GINTF;
        intr &= udev->regs.gr->GINTEN;

        /* there are no interrupts, avoid spurious interrupt */
        if (!intr) {
            return;
        }
		count_1++;
		if (intr != intr_prev){
			LOG_DBG("%02X, %02x, %d, %d", intr, intr_prev, count, count_1);
			intr_prev = intr;
			count=0;
			count_1=0;
		}

		usbd_isr(&usb_device_st.usb_driver_st);
        /* suspend interrupt */
        if (intr & GINTF_SP) {
			LOG_DBG("USB_DC_SUSPEND");
			if (usb_device_st.status_cb) {
				usb_device_st.status_cb(USB_DC_SUSPEND, NULL);
			}
        }

        /* wakeup interrupt */
        if (intr & GINTF_WKUPIF) {
			if (usb_device_st.status_cb) {
				usb_device_st.status_cb(USB_DC_RESUME, NULL);
			}			
        }

        /* enumeration has been done interrupt */
        if (intr & GINTF_ENUMFIF) {
			LOG_DBG("USB_DC_RESET");
			(void)usb_dc_reset_enum();
			if (usb_device_st.status_cb) {
				usb_device_st.status_cb(USB_DC_RESET, NULL);
			}
			usb_device_st.usb_driver_st.regs.er_out[0]->DOEPINTF = DOEPINTF_TF;
        }

        /* session request interrupt */
        if (intr & GINTF_SESIF) {
			LOG_DBG("USB_DC_CONNECTED");
			if (usb_device_st.status_cb) {
				usb_device_st.status_cb(USB_DC_CONNECTED, NULL);
			}
        }

        /* OTG mode interrupt */
        if (intr & GINTF_OTGIF) {
            if(udev->regs.gr->GOTGINTF & GOTGINTF_SESEND) {
			LOG_DBG("USB_DC_DISCONNECTED");
				if (usb_device_st.status_cb) {
					usb_device_st.status_cb(USB_DC_DISCONNECTED, NULL);
				}
            }
        }


		intr = udev->regs.gr->GINTF;
        intr &= udev->regs.gr->GINTEN;
		if (intr == intr_prev)
			LOG_DBG("it toujours presente %02X", intr);
    }
	intr = 0;
}

static void usb_dc_usbhs_gd32_isr(const void *arg)
{
    
}

static int usb_dc_gd32_clock_enable(void)
{
#ifdef USE_USB_FS
    // rcu_pll48m_clock_config(RCU_PLL48MSRC_PLLQ);
    // rcu_ck48m_clock_config(RCU_CK48MSRC_PLL48M);

    // rcu_periph_clock_enable(RCU_USBFS);
	rcu_ck48m_clock_config(RCU_CK48MSRC_IRC48M); //RCU_CK48MSRC_PLL48M);
    rcu_osci_on(RCU_IRC48M);
    rcu_periph_clock_enable(RCU_USBFS);
#elif defined(USE_USB_HS)
    #ifdef USE_EMBEDDED_PHY
        rcu_pll48m_clock_config(RCU_PLL48MSRC_PLLQ);
        rcu_ck48m_clock_config(RCU_CK48MSRC_PLL48M);
    #elif defined(USE_ULPI_PHY)
        rcu_periph_clock_enable(RCU_USBHSULPI);
    #endif /* USE_EMBEDDED_PHY */

    rcu_periph_clock_enable(RCU_USBHS);
#endif /* USB_USBFS */
	return 0;
}

static int usb_dc_gd32_clock_disable(void)
{
#ifdef USE_USB_FS
    rcu_periph_clock_disable(RCU_USBFS);
#elif defined(USE_USB_HS)
    #if defined(USE_ULPI_PHY)
        rcu_periph_clock_disable(RCU_USBHSULPI);
    #endif /* USE_EMBEDDED_PHY */
    rcu_periph_clock_disable(RCU_USBHS);
#endif /* USB_USBFS */
 
	return 0;
}

/*!
    \brief      device connect
    \param[in]  udev: pointer to USB device instance
    \param[out] none
    \retval     none
*/
static void usb_dc_connect (usb_core_driver *udev)
{
#ifndef USE_OTG_MODE
    /* connect device */
    usb_dev_connect(udev);

	k_busy_wait(3000);
#endif /* USE_OTG_MODE */
}

/*!
    \brief      device disconnect
    \param[in]  udev: pointer to USB device instance
    \param[out] none
    \retval     none
*/
static void usb_dc_disconnect (usb_core_driver *udev)
{
#ifndef USE_OTG_MODE
    /* disconnect device for 3ms */
    usb_dev_disconnect(udev);

	k_busy_wait(3000);
#endif /* USE_OTG_MODE */
}

uint32_t usb_dc_ep_send (usb_core_driver *udev, uint8_t ep_addr, uint8_t *pbuf, uint32_t len)
{
    usb_transc *transc = &udev->dev.transc_in[EP_ID(ep_addr)];

    /* setup the transfer */
    transc->xfer_buf = pbuf;
    transc->xfer_len = len;
    transc->xfer_count = 0U;

    if ((uint8_t)USB_USE_DMA == udev->bp.transfer_mode) {
        transc->dma_addr = (uint32_t)pbuf;
    }

    /* start the transfer */
    (void)usb_transc_inxfer (udev, transc);

    return 0U;
}

uint32_t usb_dc_ep_recev (usb_core_driver *udev, uint8_t ep_addr, uint8_t *pbuf, uint32_t len)
{
    usb_transc *transc = &udev->dev.transc_out[EP_ID(ep_addr)];

    /* setup the transfer */
    transc->xfer_buf = pbuf;
    transc->xfer_len = len;
    transc->xfer_count = 0U;

    if ((uint8_t)USB_USE_DMA == udev->bp.transfer_mode) {
        transc->dma_addr = (uint32_t)pbuf;
    }

    /* start the transfer */
    (void)usb_transc_outxfer (udev, transc);

    return 0U;
}

static int usb_dc_gd32_init(void)
{
    /* configure USB capabilities */
    (void)usb_basic_init(&usb_device_st.usb_driver_st.bp, &usb_device_st.usb_driver_st.regs, USB_CORE_ENUM_FS);

    usb_globalint_disable(&usb_device_st.usb_driver_st.regs);

    /* initializes the USB core*/
    (void)usb_core_init(usb_device_st.usb_driver_st.bp, &usb_device_st.usb_driver_st.regs);

    /* set device disconnect */
    usb_dc_disconnect(&usb_device_st.usb_driver_st);

#ifndef USE_OTG_MODE
    usb_curmode_set(&usb_device_st.usb_driver_st.regs, DEVICE_MODE);
#endif /* USE_OTG_MODE */

    /* initializes device mode */
    (void)usb_devcore_init(&usb_device_st.usb_driver_st);

    usb_globalint_enable(&usb_device_st.usb_driver_st.regs);

    /* set device connect */
    usb_dc_connect(&usb_device_st.usb_driver_st);

	// usb_device_st.out_ep_state[EP0_IDX].ep_mps = EP0_MPS;
	// usb_device_st.out_ep_state[EP0_IDX].ep_type = USB_DC_EP_CONTROL;
	// usb_device_st.in_ep_state[EP0_IDX].ep_mps = EP0_MPS;
	// usb_device_st.in_ep_state[EP0_IDX].ep_type = USB_DC_EP_CONTROL;

	/* TODO: make this dynamic (depending usage) */
	// for (int i = 0U; i < USB_NUM_BIDIR_ENDPOINTS; i++) {
	// 	k_sem_init(&usb_device_st.in_ep_state[i].write_sem, 1, 1);
	// }

    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);

#ifdef USE_USB_FS
    nvic_irq_enable((uint8_t)USBFS_IRQn, 2U, 0U);
    /* add an handler of the exti USBFS interrupt   */
    IRQ_CONNECT(USBFS_IRQn, 2, usb_dc_usbfs_gd32_isr, 0, 0);
	irq_enable(USBFS_IRQn);

    #if USBFS_LOW_POWER
        /* enable the power module clock */
        rcu_periph_clock_enable(RCU_PMU);

        /* USB wakeup EXTI line configuration */
        exti_interrupt_flag_clear(EXTI_18);
        exti_init(EXTI_18, EXTI_INTERRUPT, EXTI_TRIG_RISING);
        exti_interrupt_enable(EXTI_18);

        nvic_irq_enable((uint8_t)USBFS_WKUP_IRQn, 0U, 0U);
        /* add an handler of the exti USBFS interrupt   */
        IRQ_CONNECT(USBFS_WKUP_IRQn, 2, USBFS_WKUP_IRQHandler, 0, 0);
        irq_enable(USBFS_WKUP_IRQn);
    #endif /* USBFS_LOW_POWER */
#elif defined(USE_USB_HS)
    nvic_irq_enable((uint8_t)USBHS_IRQn, 2U, 0U);

    #if USBHS_LOW_POWER
        /* enable the power module clock */
        rcu_periph_clock_enable(RCU_PMU);

        /* USB wakeup EXTI line configuration */
        exti_interrupt_flag_clear(EXTI_20);
        exti_init(EXTI_20, EXTI_INTERRUPT, EXTI_TRIG_RISING);
        exti_interrupt_enable(EXTI_20);

        nvic_irq_enable((uint8_t)USBHS_WKUP_IRQn, 0U, 0U);
    #endif /* USBHS_LOW_POWER */
#endif /* USE_USB_FS */

#ifdef USB_HS_DEDICATED_EP1_ENABLED
    nvic_irq_enable(USBHS_EP1_Out_IRQn, 1, 0);
    nvic_irq_enable(USBHS_EP1_In_IRQn, 1, 0);
#endif /* USB_HS_DEDICATED_EP1_ENABLED */

	return 0;
}

/* Zephyr USB device controller API implementation */

int usb_dc_attach(void)
{
	int ret;

	LOG_DBG("");

	ret = usb_dc_gd32_clock_enable();
	if (ret) {
		return ret;
	}

	ret = usb_dc_gd32_init();
	if (ret) {
		return ret;
	}

	return 0;
}

int usb_dc_ep_set_callback(const uint8_t ep_addr, const usb_dc_ep_callback cb)
{
	LOG_DBG("ep_addr 0x%02x", ep_addr);

	uint8_t for_in = USB_EP_GET_DIR(ep_addr);
	uint8_t ep_idx = USB_EP_GET_IDX(ep_addr);

	if (ep_idx >= USB_NUM_BIDIR_ENDPOINTS) {
		LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	usb_device_st.cb[for_in ? 1 : 0][ep_idx] = cb;

	return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	LOG_DBG("");

	usb_device_st.status_cb = cb;
}

int usb_dc_set_address(const uint8_t addr)
{
	LOG_DBG("addr %u (0x%02x)", addr, addr);
	
	usb_devaddr_set(&usb_device_st.usb_driver_st, addr);
	return 0;
}

int usb_dc_ep_start_read(const uint8_t ep_addr, uint8_t *const data, const uint32_t max_data_len)
{
	int ret;
	uint32_t nb_data = max_data_len;

	LOG_DBG("ep_addr 0x%02x, len %u", ep_addr, nb_data);

	/* we flush EP0_IN by doing a 0 length receive on it */
	if (!USB_EP_DIR_IS_OUT(ep_addr) && (ep_addr != EP0_IN || nb_data)) {
		LOG_ERR("invalid ep_addr 0x%02x", ep_addr);
		return -EINVAL;
	}

	if (nb_data > EP_MPS) {
		nb_data = EP_MPS;
	}

	usb_dc_ep_recev(&usb_device_st.usb_driver_st, ep_addr, usb_device_st.ep_buf[USB_EP_GET_IDX(ep_addr)], nb_data);

    return 0U;
}

int usb_dc_ep_get_read_count(uint8_t ep_addr, uint32_t *read_bytes)
{
	if (!USB_EP_DIR_IS_OUT(ep_addr) || !read_bytes) {
		LOG_ERR("invalid ep_addr 0x%02x", ep_addr);
		return -EINVAL;
	}

	*read_bytes = usb_device_st.usb_driver_st.dev.transc_out[ep_addr].xfer_count;

	return 0;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);

	LOG_DBG("ep_addr %x, mps %d, type %d", cfg->ep_addr, cfg->ep_mps,
		cfg->ep_type);

	if ((cfg->ep_type == USB_DC_EP_CONTROL) && ep_idx) {
		LOG_ERR("invalid endpoint configuration");
		return -1;
	}

	if (ep_idx > (USB_NUM_BIDIR_ENDPOINTS - 1)) {
		LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data * const ep_cfg)
{

    uint8_t ep_addr = ep_cfg->ep_addr;
    uint16_t max_len = ep_cfg->ep_mps;
    usb_transc *transc = usb_dc_gd32_get_ep_transc(ep_addr);

	if (!transc) {
		return -EINVAL;
	}

    transc->ep_addr.num = EP_ID(ep_addr);
    transc->max_len = max_len;

	switch (ep_cfg->ep_type) {
	case USB_DC_EP_CONTROL:
		transc->ep_type = USB_EPTYPE_CTRL;
		break;
	case USB_DC_EP_ISOCHRONOUS:
		transc->ep_type = USB_EPTYPE_ISOC;
		break;
	case USB_DC_EP_BULK:
		transc->ep_type = USB_EPTYPE_BULK;
		break;
	case USB_DC_EP_INTERRUPT:
		transc->ep_type = USB_EPTYPE_INTR;
		break;
	default:
		return -EINVAL;
	}

 	LOG_DBG("ep_addr 0x%02x, previous ep_mps %u, ep_mps %u, ep_type %u",
		ep_cfg->ep_addr, transc->max_len, ep_cfg->ep_mps,
		ep_cfg->ep_type);

   /* active USB endpoint function */
    (void)usb_transc_active (&usb_device_st.usb_driver_st, transc);

	return 0;
}

int usb_dc_ep_set_stall(const uint8_t ep_addr)
{
	usb_transc *transc = usb_dc_gd32_get_ep_transc(ep_addr);

	if (!transc) {
		return -EINVAL;
	}

    transc->ep_stall = 1U;

    (void)usb_transc_stall (&usb_device_st.usb_driver_st, transc);

    return (0U);
}

int usb_dc_ep_clear_stall(const uint8_t ep_addr)
{
	usb_transc *transc = usb_dc_gd32_get_ep_transc(ep_addr);

	if (!transc) {
		return -EINVAL;
	}

    transc->ep_stall = 0U;

    (void)usb_transc_clrstall (&usb_device_st.usb_driver_st, transc);

    return (0U);
}

int usb_dc_ep_is_stalled(const uint8_t ep_addr, uint8_t *const stalled)
{
	usb_transc *transc = usb_dc_gd32_get_ep_transc(ep_addr);

	LOG_DBG("ep_addr 0x%02x", ep_addr);

	if (!transc || !stalled) {
		return -EINVAL;
	}

	*stalled = transc->ep_stall;

	return 0;
}

int usb_dc_ep_enable(const uint8_t ep_addr)
{
	usb_transc *transc = usb_dc_gd32_get_ep_transc(ep_addr);
	usb_status status;

	LOG_DBG("ep_addr 0x%02x", ep_addr);

	if (!transc) {
		return -EINVAL;
	}

	LOG_DBG("usb_transc_active(0x%02x, %u, %u)", ep_addr, transc->max_len,
		transc->ep_type);

	status = usb_transc_active(&usb_device_st.usb_driver_st, transc);
	if (status != USB_OK) {
		LOG_ERR("usb_transc_active failed(0x%02x), %d", ep_addr, (int)status);
		return -EIO;
	}

	if (USB_EP_DIR_IS_OUT(ep_addr) && ep_addr != EP0_OUT) {
		return usb_dc_ep_start_read(ep_addr, usb_device_st.ep_buf[USB_EP_GET_IDX(ep_addr)], EP_MPS);
	}

	return 0;
}

int usb_dc_ep_disable(const uint8_t ep_addr)
{
	usb_transc *transc = usb_dc_gd32_get_ep_transc(ep_addr);
	usb_status status;

	LOG_DBG("ep_addr 0x%02x", ep_addr);

	if (!transc) {
		return -EINVAL;
	}

	status = usb_transc_deactivate(&usb_device_st.usb_driver_st, transc);
	if (status != USB_OK) {
		LOG_ERR("usb_transc_deactivate failed(0x%02x), %d", ep_addr,
			(int)status);
		return -EIO;
	}

	return 0;
}

int usb_dc_ep_write(const uint8_t ep_addr, const uint8_t *const data,
		    const uint32_t data_len, uint32_t * const ret_bytes)
{
	usb_transc *transc = usb_dc_gd32_get_ep_transc(ep_addr);
	usb_status status;
	uint32_t len = data_len;
	int ret = 0;

	LOG_DBG("ep_addr 0x%02x, len %u", ep_addr, data_len);

	if (!transc || !USB_EP_DIR_IS_IN(ep_addr)) {
		LOG_ERR("invalid ep_addr 0x%02x", ep_addr);
		return -EINVAL;
	}

	if (!k_is_in_isr()) {
		irq_disable(USBFS_IRQn);
	}

	if (ep_addr == EP0_IN && len > USB_MAX_CTRL_MPS) {
		len = USB_MAX_CTRL_MPS;
	}

	status = usb_dc_ep_send(&usb_device_st.usb_driver_st, ep_addr, (void *)data, len);
	if (status != USB_OK) {
		LOG_ERR("c failed(0x%02x), %d", ep_addr, (int)status);
		ret = -EIO;
	}
	
	if (!ret && ep_addr == EP0_IN && len > 0) {
		/* Wait for an empty package as from the host.
		 * This also flushes the TX FIFO to the host.
		 */
		usb_dc_ep_start_read(ep_addr, NULL, 0);
	}

	if (!k_is_in_isr()) {
		irq_enable(USBFS_IRQn);
	}

	if (!ret && ret_bytes) {
		*ret_bytes = len;
	}

	return ret;
}

int usb_dc_ep_read_wait(uint8_t ep_addr, uint8_t *data, uint32_t max_data_len,
			uint32_t *read_bytes)
{
	usb_transc *transc = usb_dc_gd32_get_ep_transc(ep_addr);
	uint32_t read_count, offset;
	

	if (!transc) {
		LOG_ERR("Invalid Endpoint %x", ep_addr);
		return -EINVAL;
	}

	read_count = transc->xfer_count;
	offset = usb_device_st.read_offset[USB_EP_GET_IDX(ep_addr)];

	LOG_DBG("ep_addr 0x%02x, %u bytes, %u+%u, %p", ep_addr, max_data_len,
		offset, read_count, data);

	if (!USB_EP_DIR_IS_OUT(ep_addr)) { /* check if OUT ep_addr */
		LOG_ERR("Wrong endpoint direction: 0x%02x", ep_addr);
		return -EINVAL;
	}

	/* When both buffer and max data to read are zero, just ignore reading
	 * and return available data in buffer. Otherwise, return data
	 * previously stored in the buffer.
	 */
	if (data) {
		read_count = MIN(read_count, max_data_len);
		memcpy(data, usb_device_st.ep_buf[USB_EP_GET_IDX(ep_addr)] + offset, read_count);
		transc->xfer_count -= read_count;
		offset += read_count;
		LOG_HEXDUMP_DBG(usb_device_st.ep_buf[USB_EP_GET_IDX(ep_addr)],  read_count, "ep_buf");
	} else if (max_data_len) {
		LOG_ERR("Wrong arguments");
	}

	if (read_bytes) {
		*read_bytes = read_count;
	}

	return 0;
}

int usb_dc_ep_read_continue(uint8_t ep_addr)
{
    usb_transc *transc = usb_dc_gd32_get_ep_transc(ep_addr);

	if (!transc || !USB_EP_DIR_IS_OUT(ep_addr)) { /* Check if OUT ep_addr */
		LOG_ERR("Not valid endpoint: %02x", ep_addr);
		return -EINVAL;
	}

	/* If no more data in the buffer, start a new read transaction.
	 * DataOutStageCallback will called on transaction complete.
	 */
	if (!transc->xfer_count) {
		usb_dc_ep_start_read(ep_addr, usb_device_st.ep_buf[USB_EP_GET_IDX(ep_addr)], EP_MPS);
	}

	return 0;
}

int usb_dc_ep_read(const uint8_t ep_addr, uint8_t *const data, const uint32_t max_data_len,
		   uint32_t * const read_bytes)
{
	if (usb_dc_ep_read_wait(ep_addr, data, max_data_len, read_bytes) != 0) {
		return -EINVAL;
	}

	if (usb_dc_ep_read_continue(ep_addr) != 0) {
		return -EINVAL;
	}

	return 0;
}

int usb_dc_ep_halt(const uint8_t ep_addr)
{
	return usb_dc_ep_set_stall(ep_addr);
}

int usb_dc_ep_flush(const uint8_t ep_addr)
{
    if (EP_DIR(ep_addr)) {
        (void)usb_txfifo_flush (&usb_device_st.usb_driver_st.regs, EP_ID(ep_addr));
    } else {
        (void)usb_rxfifo_flush (&usb_device_st.usb_driver_st.regs);
    }

    return (0U);
}

int usb_dc_ep_mps(const uint8_t ep_addr)
{
	usb_transc *transc = usb_dc_gd32_get_ep_transc(ep_addr);

	if (!transc) {
		return -EINVAL;
	}

	return transc->max_len;
}

int usb_dc_wakeup_request(void)
{


	return 0;
}

int usb_dc_detach(void)
{

	return 0;
}

int usb_dc_reset(void)
{
	return 0;
}

/* delay in micro seconds */
void usb_udelay (const uint32_t usec)
{
    k_busy_wait(usec);
}

/* delay in milliseconds */
void usb_mdelay (const uint32_t msec)
{
    usb_udelay(msec * 1000);
}


uint8_t usbd_setup_transc (usb_core_driver *udev)
{
	struct usb_setup_packet *setup = (void *)&udev->dev.control.req;
	usb_transc *transc = usb_dc_gd32_get_ep_transc(EP0_OUT);
	uint8_t for_in = USB_EP_GET_DIR(EP0_OUT);

	LOG_DBG("");

	transc->xfer_count = SETUP_SIZE;
	usb_device_st.read_offset[EP0_IDX] = 0;
	//usb_rxfifo_read(&udev->regs, usb_device_st.ep_buf[EP0_IDX], transc->xfer_count);
	memcpy(&usb_device_st.ep_buf[EP0_IDX], &udev->dev.control.req, transc->xfer_count);

	if ((usb_device_st.cb[for_in ? 1 : 0][EP0_IDX])) {
		(usb_device_st.cb[for_in ? 1 : 0][EP0_IDX])(EP0_OUT, USB_DC_EP_SETUP);

		if (!(setup->wLength == 0U) && usb_reqtype_is_to_device(setup)) {
			usb_dc_ep_start_read(EP0_OUT,
					     usb_device_st.ep_buf[EP0_IDX],
					     setup->wLength);
		}
	}

	return (uint8_t)0;
}

uint8_t usbd_out_transc (usb_core_driver *udev, uint8_t ep_num)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep_num);
	uint8_t ep = ep_idx | USB_EP_DIR_OUT;
	usb_transc *transc = usb_dc_gd32_get_ep_transc(ep);
	uint8_t for_in = USB_EP_GET_DIR(ep);

	/* Transaction complete, data is now stored in the buffer and ready
	 * for the upper stack (usb_dc_ep_read to retrieve).
	 */
	//usb_dc_ep_get_read_count(ep, &transc->read_count);
	usb_device_st.read_offset[ep_idx] = 0;

	LOG_DBG("epnum 0x%02x, rx_count %u", ep_num, transc->xfer_count);
		
	if (usb_device_st.cb[for_in ? 1 : 0][ep_idx]) {
		usb_device_st.cb[for_in ? 1 : 0][ep_idx](ep, USB_DC_EP_DATA_OUT);
	}
    return (uint8_t)0;
}

uint8_t usbd_in_transc (usb_core_driver *udev, uint8_t ep_num)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep_num);
	uint8_t ep = ep_idx | USB_EP_DIR_IN;
	uint8_t for_in = USB_EP_GET_DIR(ep);

	LOG_DBG("epnum 0x%02x", ep_num);

	if (usb_device_st.cb[for_in ? 1 : 0][ep_idx]) {
		usb_device_st.cb[for_in ? 1 : 0][ep_idx](ep, USB_DC_EP_DATA_IN);
	}
    return (uint8_t)0;
}

