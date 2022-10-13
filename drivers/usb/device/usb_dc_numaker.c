/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/dt-bindings/usb/usb.h>
#include <zephyr/sys/math_extras.h>
#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_SCC
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#endif
#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_dc_numaker, CONFIG_USB_DRIVER_LOG_LEVEL);

#include "NuMicro.h"

/* USBD notes
 *
 * 1. Require 48MHz clock source
 *    (1) Not support HIRC48 as clock source. It involves trim with USB SOF packets
 *        and isn't suitable in HAL.
 *    (2) Instead of HICR48, core clock is required to be multiple of 48MHz e.g. 192MHz,
 *        to generate necessary 48MHz.
 * 2. Some chip series disallows ISO IN/OUT to be assigned the same endpoint number.
 *    There is one workaround but it can only work for not more than one ISO IN
 *    endpoint enabled, or behavior is undefined.
 *    See CONFIG_USB_DC_NUMAKER_USBD_WORKAROUND_DISALLOW_ISO_IN_OUT_SAME_NUM.
 */

/* Not yet support HSUSBD */
#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_hsusbd) && defined(CONFIG_USB_DC_NUMAKER_HSUSBD)
#error "Not yet support HSUSBD"
#endif

/* Implementation notes
 *
 * 1. Use static-initialized mutex to synchronize whole device's data structure, or we will
 *    get into cart-before-the-horse situation because we cannot guarantee dynamic-initializing
 *    this mutex is thread-safe.
 * 2. Kernel services like thread, semaphore, etc. are kept persistent after their first-time
 *    initialization because zephyr doesn't provide clear un-initialize for them.
 * 3. Support interrupt top/bottom halves processing to:
 *    (1) Not run Zephyr USB device stack and callbacks in interrupt context because callbacks
 *        from this stack may use mutex or other kernel functions which are not supported
 *        in interrupt context
 *    (2) Not run heavily in interrupt context
 * 4. Callback registered (via usb_dc_set_status_callback()) is kept persistent because
 *    the Zephyr USB device stack doesn't re-register.
 * 5. EP callbacks registered (via usb_dc_ep_set_callback()) are kept persistent because
 *    the Zephyr USB device stack doesn't re-register.
 * 6. DMA buffer management is allocate-only, no de-allocate, except re-initialize.
 */

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
#define DT_DRV_COMPAT nuvoton_numaker_usbd
#endif

/* Max number of endpoint slots
 *
 * This define must be largest of all instances' num_bidir_endpoints, though, Zephyr
 * USB device stack just supports only one instance.
 */
#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
#define NU_USB_DC_MAX_NUM_EP_SLOTS      DT_INST_PROP(0, num_bidir_endpoints)
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
BUILD_ASSERT(DT_INST_PROP(0, num_bidir_endpoints) == USBD_MAX_EP,
             "num_bidir_endpoints doesn't match BSP USBD driver");
#endif

/* Forward declarations */
struct usb_dc_numaker_device;

/* Message type */
#define NU_USB_DC_MSG_TYPE_SW_RECONN    0   // S/W reconnect
#define NU_USB_DC_MSG_TYPE_CB_STATE     1   // Callback for enum usb_dc_status_code
#define NU_USB_DC_MSG_TYPE_CB_EP        2   // Callback for usb_dc_ep_cb_status_code

/* Message structure */
struct nu_usb_dc_msg {
    uint32_t                                msg_type;
    union {
        struct {
            enum usb_dc_status_code         status_code;
        } cb_state;
        struct {
            uint8_t                         ep_addr;
            enum usb_dc_ep_cb_status_code   status_code;
        } cb_ep;
    };
};

/* Immutable device context */
struct usb_dc_numaker_config {
#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    USBD_T *    usbd_base;
#endif
    uint32_t    id_rst;
    uint32_t    clk_modidx;
    uint32_t    clk_src;
    uint32_t    clk_div;
#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_SCC
    const struct device *               clkctrl_dev;
#endif
    void (*irq_config_func)(struct usb_dc_numaker_device *dev);
    void (*irq_unconfig_func)(struct usb_dc_numaker_device *dev);
#ifdef CONFIG_PINCTRL
    const struct pinctrl_dev_config *   pincfg;
#endif
    uint32_t                            num_bidir_endpoints;
    uint32_t                            dmabuf_size;
    int                                 maximum_speed;
    struct k_mutex *                    sync_mutex_p;
};

/* Endpoint context */
struct nu_usb_dc_ep {    
    bool                        valid;          // Valid

    struct usb_dc_numaker_device *  dev;        // Pointer to the containing device

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    uint8_t                     usbd_hw_ep_hndl;    // EP0, EP1, EP2, etc.
#endif

    bool                        dmabuf_valid;   // Endpoint DMA buffer valid
    uint32_t                    dmabuf_base;
    uint32_t                    dmabuf_size;

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* For USBD, S/W FIFO */
    uint32_t                    read_fifo_pos;
    uint32_t                    read_fifo_used;
    uint32_t                    write_fifo_pos;
    uint32_t                    write_fifo_free;
#endif

    /* NOTE: On USBD, Setup and CTRL OUT are not completely separated. CTRL OUT's MXPLD
     *       can be overridden to 8 by Setup. As workaround, we make one copy of CTRL OUT'
     *       MXPLD on its interrupt. However, this strategy can just decrease the chance. */
#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    uint32_t                    mxpld_ctrlout;
#endif

    /* Following fields are used for binding to endpoint address */
    bool                        ep_addr_valid;  // Endpoint address valid
    uint8_t                     ep_addr;        // Endpoint address

    bool                        ep_mps_valid;   // Endpoint max packet size valid
    uint16_t                    ep_mps;         // Endpoint max packet size

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    uint32_t                    usbd_hw_ep_cfg; // Saved for easy control with BSP driver
#endif

    bool                        FIFO_need_own;  // For USBD, FIFO cannot access simultaneously by CPU and H/W, and needs ownership management

    usb_dc_ep_callback          cb;             // Endpoint callback function

    uint32_t                    zero_end;       // Mark the end of fields which can initialize to zero

    bool                        FIFO_own_sem_valid; // FIFO ownership semaphore valid
    struct k_sem                FIFO_own_sem;       // FIFO ownership semaphore    
};

/* Endpoint management context */
struct nu_usb_dc_ep_mgmt {
    bool                        ep_slot_idx_valid;  // Free EP slot index valid
    uint8_t                     ep_slot_idx;        // Free EP slot index

    bool                        dmabuf_pos_valid;   // Free DMA buffer offset valid
    uint32_t                    dmabuf_pos;         // Free DMA buffer offset

    bool                        dmabuf_setup_valid; // DMA buffer for Setup packet valid
    bool                        new_setup;          // New Setup packet ready
    struct usb_setup_packet     setup_packet;       // Cache setup packet

    uint32_t                    zero_end;           // Mark the end of fields which can initialize to zero

    struct nu_usb_dc_ep         ep_slots[NU_USB_DC_MAX_NUM_EP_SLOTS];
};

/* Mutable device context */
struct usb_dc_numaker_data {
    uint8_t         addr;                       // Host assigned USB device address

    uint32_t        zero_end;                   // Mark the end of fields which can initialize to zero

    bool            attached;

    /* Structure for enabling interrupt top/bottom halves processing */
    bool                        msgq_valid;
    struct k_msgq               msgq;
    struct nu_usb_dc_msg        msgq_buf[CONFIG_USB_DC_NUMAKER_MSG_QUEUE_SIZE];

    K_KERNEL_STACK_MEMBER(msg_hdlr_thread_stack, CONFIG_USB_DC_NUMAKER_MSG_HANDLER_THREAD_STACK_SIZE);
    bool            msg_hdlr_thread_valid;
    struct k_thread msg_hdlr_thread;

    usb_dc_status_callback      status_cb;      // Status callback function

    struct nu_usb_dc_ep_mgmt    ep_mgmt;        // EP management
};

/* Device context */
struct usb_dc_numaker_device {
    const struct usb_dc_numaker_config *    config;
    struct usb_dc_numaker_data *            data;
};

/* Declarations of internal functions */
static struct usb_dc_numaker_device *usb_dc_numaker_device_inst_0(void);
static void usb_dc_numaker_isr(struct usb_dc_numaker_device *dev);
static void nu_usb_dc_lock(const struct usb_dc_numaker_device *dev);
static void nu_usb_dc_unlock(const struct usb_dc_numaker_device *dev);
static void nu_usb_dc_msg_hdlr_thread_main(void *arg1, void *arg2, void *arg3);
static int nu_usb_dc_send_msg(const struct usb_dc_numaker_device *dev, const struct nu_usb_dc_msg *msg);
static int nu_usb_dc_hw_setup(struct usb_dc_numaker_device *dev);
static void nu_usb_dc_hw_shutdown(struct usb_dc_numaker_device *dev);
static void nu_usb_dc_sw_connect(struct usb_dc_numaker_device *dev);
static void nu_usb_dc_sw_disconnect(struct usb_dc_numaker_device *dev);
static void nu_usb_dc_sw_reconnect(struct usb_dc_numaker_device *dev);
static void nu_usb_dc_bus_reset_th(struct usb_dc_numaker_device *dev);
static void nu_usb_dc_bus_reset_bh(struct usb_dc_numaker_device *dev);
static void nu_usb_dc_reset_addr(struct usb_dc_numaker_device *dev);
static void nu_usb_dc_remote_wakeup(struct usb_dc_numaker_device *dev);
static void nu_usb_dc_ep_mgmt_init(struct usb_dc_numaker_device *dev);
static struct nu_usb_dc_ep *nu_usb_dc_ep_mgmt_bind_ep(struct usb_dc_numaker_device *dev, const uint8_t ep_addr);
static struct nu_usb_dc_ep *nu_usb_dc_ep_mgmt_find_ep(struct usb_dc_numaker_device *dev, const uint8_t ep_addr);
static struct nu_usb_dc_ep *nu_usb_dc_ep_mgmt_alloc_ep(struct usb_dc_numaker_device *dev);
static int nu_usb_dc_ep_mgmt_alloc_dmabuf(struct usb_dc_numaker_device *dev, uint32_t size, uint32_t *dmabuf_base_p, uint32_t *dmabuf_size_p);
static void nu_usb_dc_setup_config_dmabuf(struct usb_dc_numaker_device *dev);
static void nu_usb_dc_setup_fifo_copy_to_user(struct usb_dc_numaker_device *dev, uint8_t *usrbuf);
static void nu_usb_dc_ep_bh(struct nu_usb_dc_ep *ep_cur, enum usb_dc_ep_cb_status_code status_code);
static void nu_usb_dc_ep_config_dmabuf(struct nu_usb_dc_ep *ep_cur, uint32_t dmabuf_base, uint32_t dmabuf_size);
static void nu_usb_dc_ep_fifo_reset(struct nu_usb_dc_ep *ep_cur);
static int nu_usb_dc_ep_fifo_copy_to_user(struct nu_usb_dc_ep *ep_cur, uint8_t *usrbuf, uint32_t *size_p);
static int nu_usb_dc_ep_fifo_copy_from_user(struct nu_usb_dc_ep *ep_cur, const uint8_t *usrbuf, uint32_t *size_p);
static void nu_usb_dc_ep_fifo_update(struct nu_usb_dc_ep *ep_cur);
static uint32_t nu_usb_dc_ep_fifo_max(struct nu_usb_dc_ep *ep_cur);
static uint32_t nu_usb_dc_ep_fifo_used(struct nu_usb_dc_ep *ep_cur);
static void nu_usb_dc_ep_config_major(struct nu_usb_dc_ep *ep_cur, const struct usb_dc_ep_cfg_data * const ep_cfg);
static void nu_usb_dc_ep_set_stall(struct nu_usb_dc_ep *ep_cur);
static void nu_usb_dc_ep_clear_stall(struct nu_usb_dc_ep *ep_cur);
static bool nu_usb_dc_ep_is_stalled(struct nu_usb_dc_ep *ep_cur);
static void nu_usb_dc_ep_enable(struct nu_usb_dc_ep *ep_cur);
static void nu_usb_dc_ep_disable(struct nu_usb_dc_ep *ep_cur);
static void nu_usb_dc_ep_out_wait(struct nu_usb_dc_ep *ep_cur);
static void nu_usb_dc_ep_trigger(struct nu_usb_dc_ep *ep_cur, uint32_t len);
static void nu_usb_dc_ep_abort(struct nu_usb_dc_ep *ep_cur);

/* Zephyr USB device controller API implementation */

int usb_dc_attach(void)
{
    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    struct usb_dc_numaker_data *data = dev->data;
    int rc = 0;

    nu_usb_dc_lock(dev);

    if (data->attached) {
        LOG_WRN("Already attached");
        goto cleanup;
    }

    /* Initialize all fields to zero except persistent */
    memset(data, 0x00, offsetof(struct usb_dc_numaker_data, zero_end));

    /* Set up interrupt top/bottom halves processing */

    if (!data->msgq_valid) {
        k_msgq_init(&data->msgq,                                    // Address of the message queue
                    (char *) data->msgq_buf,                        // Pointer to ring buffer that holds queued messages
                    sizeof(struct nu_usb_dc_msg),                   // Message size in bytes
                    CONFIG_USB_DC_NUMAKER_MSG_QUEUE_SIZE);          // Maximum number of messages

        data->msgq_valid = true;
    }

    if (!data->msg_hdlr_thread_valid) {
        k_thread_create(&data->msg_hdlr_thread,                     // Pointer to uninitialized struct k_thread
                        data->msg_hdlr_thread_stack,                // Pointer to the stack space
                        CONFIG_USB_DC_NUMAKER_MSG_HANDLER_THREAD_STACK_SIZE,    // Stack size in bytes
                        nu_usb_dc_msg_hdlr_thread_main,             // Thread entry function
                        dev,                                        // 1st entry point parameter
                        NULL,                                       // 2nd entry point parameter
                        NULL,                                       // 3rd entry point parameter
                        K_PRIO_COOP(2),                             // Thread priority
                        0,                                          // Thread options
                        K_NO_WAIT);                                 // Scheduling delay, or K_NO_WAIT (for no delay)

        data->msg_hdlr_thread_valid = true;
    }

    /* Initialize USB DC H/W */
    rc = nu_usb_dc_hw_setup(dev);
    if (rc < 0) {
        LOG_ERR("Set up H/W");
        goto cleanup;
    }

    /* USB device address defaults to 0 */
    nu_usb_dc_reset_addr(dev);

    /* Initialize endpoints */
    nu_usb_dc_ep_mgmt_init(dev);

    /* S/W connect */
    nu_usb_dc_sw_connect(dev);

    data->attached = true;
    LOG_INF("attached");

cleanup:

    if (rc < 0) {
        usb_dc_detach();
    }

    nu_usb_dc_unlock(dev);

    return rc;
}

int usb_dc_detach(void)
{
    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    struct usb_dc_numaker_data *data = dev->data;

    nu_usb_dc_lock(dev);

    data->attached = false;
    LOG_INF("detached");

    /* S/W disconnect */
    nu_usb_dc_sw_disconnect(dev);

    /* Un-initialize USB DC H/W */
    nu_usb_dc_hw_shutdown(usb_dc_numaker_device_inst_0());

    /* Purge message queue */
    if ( data->msgq_valid) {
        k_msgq_purge(&data->msgq);
    }

    nu_usb_dc_unlock(dev);

    return 0;
}

int usb_dc_reset(void)
{
    LOG_INF("usb_dc_reset");

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();

    nu_usb_dc_lock(dev);

    usb_dc_detach();
    usb_dc_attach();

    nu_usb_dc_unlock(dev);

    return 0;
}

int usb_dc_set_address(const uint8_t addr)
{
    LOG_INF("USB device address=%u (0x%02x)", addr, addr);

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    struct usb_dc_numaker_data *data = dev->data;

    nu_usb_dc_lock(dev);

    /* NOTE: Timing for configuring USB device address into H/W is critical. It must be done
     *       in-between SET_ADDRESS control transfer and next transfer. For this, it is done
     *       in IN ACK ISR of SET_ADDRESS control transfer. */
    data->addr = addr;

    nu_usb_dc_unlock(dev);

    return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
    LOG_DBG("cb=%p", cb);

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    struct usb_dc_numaker_data *data = dev->data;

    nu_usb_dc_lock(dev);

    data->status_cb = cb;

    nu_usb_dc_unlock(dev);
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
    /* For safe, require EP number for control transfer to be 0 */
    if ((cfg->ep_type == USB_DC_EP_CONTROL) && USB_EP_GET_IDX(cfg->ep_addr) != 0) {
        LOG_ERR("EP number for control transfer must be 0");
        return -ENOTSUP;
    }

    return 0;
}

int usb_dc_ep_set_callback(const uint8_t ep_addr, const usb_dc_ep_callback cb)
{
    LOG_DBG("ep_addr=0x%02x, cb=%p", ep_addr, cb);

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    int rc = 0;

    nu_usb_dc_lock(dev);

    /* Bind EP context to EP address */
    struct nu_usb_dc_ep *ep_cur = nu_usb_dc_ep_mgmt_bind_ep(dev, ep_addr);
    if (!ep_cur) {
        LOG_ERR("Bind EP context: ep_addr=0x%02x", ep_addr);
        rc = -ENOMEM;
        goto cleanup;
    }

    ep_cur->cb = cb;

cleanup:

    nu_usb_dc_unlock(dev);

    return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data * const ep_cfg)
{
    LOG_INF("Configure: ep_addr=0x%02x, ep_mps=%d, ep_type=%d",
            ep_cfg->ep_addr, ep_cfg->ep_mps, ep_cfg->ep_type);

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    int rc = 0;
    uint32_t dmabuf_base = 0;
    uint32_t dmabuf_size = 0;

    nu_usb_dc_lock(dev);

    /* Bind EP context to EP address */
    struct nu_usb_dc_ep *ep_cur = nu_usb_dc_ep_mgmt_bind_ep(dev, ep_cfg->ep_addr);
    if (!ep_cur) {
        LOG_ERR("Bind EP context: ep_addr=0x%02x", ep_cfg->ep_addr);
        rc = -ENOMEM;
        goto cleanup;
    }

    /* Allocate DMA buffer */
    if (!ep_cur->dmabuf_valid || ep_cur->dmabuf_size < ep_cfg->ep_mps) {
        rc = nu_usb_dc_ep_mgmt_alloc_dmabuf(dev, ep_cfg->ep_mps, &dmabuf_base, &dmabuf_size);
        if (rc < 0) {
            LOG_ERR("Allocate DMA buffer failed");
            goto cleanup;
        }

        /* Configure EP DMA buffer */
        nu_usb_dc_ep_config_dmabuf(ep_cur, dmabuf_base, dmabuf_size);

        LOG_DBG("DMA buffer: ep_addr=0x%02x, dmabuf_base=%d, dmabuf_size=%d",
            ep_cfg->ep_addr, dmabuf_base, dmabuf_size);
    }

    /* Configure EP */
    nu_usb_dc_ep_config_major(ep_cur, ep_cfg);

cleanup:

    nu_usb_dc_unlock(dev);

    return rc;
}

int usb_dc_ep_set_stall(const uint8_t ep_addr)
{
    LOG_INF("Set stall: ep_addr=0x%02x", ep_addr);

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    int rc = 0;

    nu_usb_dc_lock(dev);

    /* Bind EP context to EP address */
    struct nu_usb_dc_ep *ep_cur = nu_usb_dc_ep_mgmt_bind_ep(dev, ep_addr);
    if (!ep_cur) {
        LOG_ERR("Bind EP context: ep_addr=0x%02x", ep_addr);
        rc = -ENOMEM;
        goto cleanup;
    }

    /* Set EP to stalled */
    nu_usb_dc_ep_set_stall(ep_cur);

cleanup:

    nu_usb_dc_unlock(dev);

    return rc;
}

int usb_dc_ep_clear_stall(const uint8_t ep_addr)
{
    LOG_INF("Clear stall: ep_addr=0x%02x", ep_addr);

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    int rc = 0;

    nu_usb_dc_lock(dev);

    /* Bind EP context to EP address */
    struct nu_usb_dc_ep *ep_cur = nu_usb_dc_ep_mgmt_bind_ep(dev, ep_addr);
    if (!ep_cur) {
        LOG_ERR("Bind EP context: ep_addr=0x%02x", ep_addr);
        rc = -ENOMEM;
        goto cleanup;
    }

    /* Clear EP to unstalled */
    nu_usb_dc_ep_clear_stall(ep_cur);

cleanup:

    nu_usb_dc_unlock(dev);

    return rc;
}

int usb_dc_ep_is_stalled(const uint8_t ep_addr, uint8_t *const stalled)
{
    LOG_DBG("ep_addr=0x%02x", ep_addr);

    if (!stalled) {
        return -EINVAL;
    }

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    int rc = 0;

    nu_usb_dc_lock(dev);

    /* Bind EP context to EP address */
    struct nu_usb_dc_ep *ep_cur = nu_usb_dc_ep_mgmt_bind_ep(dev, ep_addr);
    if (!ep_cur) {
        LOG_ERR("Bind EP context: ep_addr=0x%02x", ep_addr);
        rc = -ENOMEM;
        goto cleanup;
    }

    *stalled = nu_usb_dc_ep_is_stalled(ep_cur);

    LOG_DBG("ep_addr=0x%02x, stalled=%d", ep_addr, *stalled);

cleanup:

    nu_usb_dc_unlock(dev);

    return rc;
}

int usb_dc_ep_halt(const uint8_t ep_addr)
{
    return usb_dc_ep_set_stall(ep_addr);
}

int usb_dc_ep_enable(const uint8_t ep_addr)
{
    LOG_INF("Enable: ep_addr=0x%02x", ep_addr);

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    int rc = 0;

    nu_usb_dc_lock(dev);

    /* Bind EP context to EP address */
    struct nu_usb_dc_ep *ep_cur = nu_usb_dc_ep_mgmt_bind_ep(dev, ep_addr);
    if (!ep_cur) {
        LOG_ERR("Bind EP context: ep_addr=0x%02x", ep_addr);
        rc = -ENOMEM;
        goto cleanup;
    }

    nu_usb_dc_ep_enable(ep_cur);

    /* Trigger OUT transaction manually, or H/W will continue to reply NAK because
     * Zephyr USB device stack is unclear on kicking off by invoking usb_dc_ep_read()
     * or friends. We needn't do this for CTRL OUT because Setup sequence will involve
     * this. */
    if (USB_EP_DIR_IS_OUT(ep_addr) && USB_EP_GET_IDX(ep_addr) != 0) {
        rc = usb_dc_ep_read_continue(ep_addr);
        if (rc < 0) {
            goto cleanup;
        }
    }

cleanup:

    nu_usb_dc_unlock(dev);

    return rc;
}

int usb_dc_ep_disable(const uint8_t ep_addr)
{
    LOG_INF("Disable: ep_addr=0x%02x", ep_addr);

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    int rc = 0;

    nu_usb_dc_lock(dev);

    /* Bind EP context to EP address */
    struct nu_usb_dc_ep *ep_cur = nu_usb_dc_ep_mgmt_bind_ep(dev, ep_addr);
    if (!ep_cur) {
        LOG_ERR("Bind EP context: ep_addr=0x%02x", ep_addr);
        rc = -ENOMEM;
        goto cleanup;
    }

    nu_usb_dc_ep_disable(ep_cur);

cleanup:

    nu_usb_dc_unlock(dev);

    return rc;
}

int usb_dc_ep_flush(const uint8_t ep_addr)
{
    LOG_INF("ep_addr=0x%02x", ep_addr);

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    int rc = 0;

    nu_usb_dc_lock(dev);

    /* Bind EP context to EP address */
    struct nu_usb_dc_ep *ep_cur = nu_usb_dc_ep_mgmt_bind_ep(dev, ep_addr);
    if (!ep_cur) {
        LOG_ERR("Bind EP context: ep_addr=0x%02x", ep_addr);
        rc = -ENOMEM;
        goto cleanup;
    }

    nu_usb_dc_ep_fifo_reset(ep_cur);

cleanup:

    nu_usb_dc_unlock(dev);

    return rc;
}

int usb_dc_ep_write(const uint8_t ep_addr, const uint8_t *const data_buf,
            const uint32_t data_len, uint32_t * const ret_bytes)
{
    LOG_DBG("ep_addr=0x%02x, to_write=%d bytes", ep_addr, data_len);

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    int rc = 0;
    uint32_t data_len_act = 0;

    nu_usb_dc_lock(dev);

    /* Bind EP context to EP address */
    struct nu_usb_dc_ep *ep_cur = nu_usb_dc_ep_mgmt_bind_ep(dev, ep_addr);
    if (!ep_cur) {
        LOG_ERR("ep_addr=0x%02x", ep_addr);
        rc = -ENOMEM;
        goto cleanup;
    }

    if (!USB_EP_DIR_IS_IN(ep_addr)) {
        LOG_ERR("Invalid EP address 0x%02x for write", ep_addr);
        rc = -EINVAL;
        goto cleanup;
    }

    /* Try to acquire EP DMA buffer ownership on behalf of H/W */
    if (ep_cur->FIFO_need_own) {
        rc = k_sem_take(&ep_cur->FIFO_own_sem, K_NO_WAIT);
        if (rc < 0) {
            LOG_WRN("ep_addr 0x%02x busy", ep_addr);
            rc = -EAGAIN;
            goto cleanup;
        }
    }

    /* Write FIFO not empty
     *
     * For USBD, don't trigger next DATA IN for one-shot implementation.
     */
    if (nu_usb_dc_ep_fifo_used(ep_cur)) {
        LOG_WRN("ep_addr 0x%02x: Write FIFO not empty for one-shot implementation", ep_addr);
        rc = -EAGAIN;
        goto cleanup;
    }

    /* NOTE: Null data or zero data length are valid, used for ZLP */
    if (data_buf && data_len) {
        data_len_act = data_len;
        rc = nu_usb_dc_ep_fifo_copy_from_user(ep_cur, data_buf, &data_len_act);
        if (rc < 0) {
            LOG_ERR("Copy to FIFO from user buffer");
            goto cleanup;
        }
    } else {
        data_len_act = 0;
    }

    /* Now H/W actually owns EP DMA buffer */
    nu_usb_dc_ep_trigger(ep_cur, data_len_act);

    /* NOTE: On 'ret_bytes' being null, write all bytes is expected, but for one-shot
     *       implementation, only at most MPS size is supported. */
    if (ret_bytes) {
        *ret_bytes = data_len_act;
    } else if (data_len_act != data_len) {
        LOG_ERR("Expected write all %d bytes, but actual %d bytes written", data_len, data_len_act);
        rc = -EIO;
        goto cleanup;
    }

    LOG_DBG("ep_addr=0x%02x, written=%d bytes", ep_addr, data_len_act);

cleanup:

    nu_usb_dc_unlock(dev);

    return rc;
}

int usb_dc_ep_read(const uint8_t ep_addr, uint8_t *const data, const uint32_t max_data_len,
           uint32_t * const read_bytes)
{
    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    int rc = 0;

    nu_usb_dc_lock(dev);

    rc = usb_dc_ep_read_wait(ep_addr, data, max_data_len, read_bytes);
    if (rc < 0) {
        goto cleanup;
    }

    rc = usb_dc_ep_read_continue(ep_addr);
    if (rc < 0) {
        goto cleanup;
    }

cleanup:

    nu_usb_dc_unlock(dev);

    return rc;
}

int usb_dc_ep_read_wait(uint8_t ep_addr, uint8_t *data_buf, uint32_t max_data_len,
            uint32_t *read_bytes)
{
    LOG_DBG("ep_addr=0x%02x, to_read=%d bytes", ep_addr, max_data_len);

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    struct usb_dc_numaker_data *data = dev->data;
    struct nu_usb_dc_ep_mgmt *ep_mgmt = &data->ep_mgmt;
    int rc = 0;
    uint32_t data_len_act = 0;

    /* Suppress 'unused variable' warning on some build with assert */
    ARG_UNUSED(ep_mgmt);

    nu_usb_dc_lock(dev);

    /* Bind EP context to EP address */
    struct nu_usb_dc_ep *ep_cur = nu_usb_dc_ep_mgmt_bind_ep(dev, ep_addr);
    if (!ep_cur) {
        LOG_ERR("Bind EP context: ep_addr=0x%02x", ep_addr);
        rc = -ENOMEM;
        goto cleanup;
    }

    if (!USB_EP_DIR_IS_OUT(ep_addr)) {
        LOG_ERR("Invalid EP address 0x%02x for read", ep_addr);
        rc = -EINVAL;
        goto cleanup;
    }

    /* Special handling for USB_CONTROL_EP_OUT on Setup packet */
    if (ep_addr == USB_CONTROL_EP_OUT && ep_mgmt->new_setup) {
        if (!data_buf || max_data_len != 8) {
            LOG_ERR("Invalid parameter for reading Setup packet");
            rc = -EINVAL;
            goto cleanup;
        }

        memcpy(data_buf, &ep_mgmt->setup_packet, 8);
        ep_mgmt->new_setup = false;

        if (read_bytes) {
            *read_bytes = 8;
        }

        LOG_DBG("ep_addr=0x%02x, read setup packet=8 bytes", ep_addr);

        goto cleanup;
    }

    /* We cannot read on FIFO being owned by H/W */
    if (ep_cur->FIFO_need_own) {
        if (!k_sem_count_get(&ep_cur->FIFO_own_sem)) {
            LOG_WRN("ep_addr 0x%02x busy", ep_addr);
            rc = -EAGAIN;
            goto cleanup;
        }
    }

    /* NOTE: Null data and zero data length is valid, used for returning number of
     *       available bytes for read */
    if (data_buf) {
        data_len_act = max_data_len;
        rc = nu_usb_dc_ep_fifo_copy_to_user(ep_cur, data_buf, &data_len_act);
        if (rc < 0) {
            LOG_ERR("Copy from FIFO to user buffer");
            goto cleanup;
        }

        if (read_bytes) {
            *read_bytes = data_len_act;
        }
    } else if (max_data_len) {
        LOG_ERR("Null data but non-zero data length");
        rc = -EINVAL;
        goto cleanup;
    } else {
        if (read_bytes) {
            *read_bytes = nu_usb_dc_ep_fifo_used(ep_cur);
        }
    }

    /* Suppress further USB_DC_EP_DATA_OUT events by replying NAK or disabling interrupt */
    nu_usb_dc_ep_out_wait(ep_cur);

    LOG_DBG("ep_addr=0x%02x, read=%d bytes", ep_addr, data_len_act);

cleanup:

    nu_usb_dc_unlock(dev);

    return rc;
}

int usb_dc_ep_read_continue(uint8_t ep_addr)
{
    LOG_DBG("ep_addr=0x%02x", ep_addr);

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    int rc = 0;

    nu_usb_dc_lock(dev);

    /* Bind EP context to EP address */
    struct nu_usb_dc_ep *ep_cur = nu_usb_dc_ep_mgmt_bind_ep(dev, ep_addr);
    if (!ep_cur) {
        LOG_ERR("Bind EP context: ep_addr=0x%02x", ep_addr);
        rc = -ENOMEM;
        goto cleanup;
    }

    if (!USB_EP_DIR_IS_OUT(ep_addr)) {
        LOG_ERR("Invalid EP address 0x%02x for read", ep_addr);
        rc = -EINVAL;
        goto cleanup;
    }

    /* Try to acquire EP FIFO ownership on behalf of H/W */
    if (ep_cur->FIFO_need_own) {
        rc = k_sem_take(&ep_cur->FIFO_own_sem, K_NO_WAIT);
        if (rc < 0) {
            LOG_DBG("ep_addr 0x%02x has triggered", ep_addr);
            rc = 0;
            goto cleanup;
        }
    }

    /* Read FIFO not empty
     *
     * For USBD, don't trigger next DATA OUT for one-shot implementation, or overwrite.
     */
    if (nu_usb_dc_ep_fifo_used(ep_cur)) {
        goto cleanup;
    }

    __ASSERT_NO_MSG(ep_cur->ep_mps_valid);
    nu_usb_dc_ep_trigger(ep_cur, ep_cur->ep_mps);

cleanup:

    nu_usb_dc_unlock(dev);

    return rc;
}

int usb_dc_ep_mps(const uint8_t ep_addr)
{
    LOG_DBG("ep_addr=0x%02x", ep_addr);

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    int rc = 0;
    uint16_t ep_mps = 0;

    nu_usb_dc_lock(dev);

    /* Bind EP context to EP address */
    struct nu_usb_dc_ep *ep_cur = nu_usb_dc_ep_mgmt_bind_ep(dev, ep_addr);
    if (!ep_cur) {
        LOG_ERR("Bind EP context: ep_addr=0x%02x", ep_addr);
        rc = -ENOMEM;
        goto cleanup;
    }

    __ASSERT_NO_MSG(ep_cur->ep_mps_valid);
    ep_mps = ep_cur->ep_mps;

cleanup:

    nu_usb_dc_unlock(dev);

    return ep_mps;
}

int usb_dc_wakeup_request(void)
{
    LOG_INF("Remote wakeup");

    struct usb_dc_numaker_device *dev = usb_dc_numaker_device_inst_0();
    int rc = 0;

    nu_usb_dc_lock(dev);

    nu_usb_dc_remote_wakeup(dev);

    nu_usb_dc_unlock(dev);

    return rc;
}

#ifdef CONFIG_PINCTRL
/* Take care of the case, e.g. M460 USB 2.0 device controller,
 * pinouts are hard-wired and needn't pinmux. */
#if DT_NODE_HAS_PROP(DT_DRV_INST(0), pinctrl_0)
PINCTRL_DT_INST_DEFINE(0);
#endif
#endif

/* Declaration of IRQ configure/unconfigure function instance 0 */
static void usb_dc_numaker_irq_config_func_inst_0(struct usb_dc_numaker_device *dev);
static void usb_dc_numaker_irq_unconfig_func_inst_0(struct usb_dc_numaker_device *dev);

/* Implementation of device synchronization mutex instance 0 */
K_MUTEX_DEFINE(sync_mutex_inst_0);

static const struct usb_dc_numaker_config s_usb_dc_numaker_config_inst_0 = {
#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    .usbd_base      = (USBD_T *) DT_INST_REG_ADDR(0),
#endif
    .id_rst         = DT_INST_PROP(0, reset),
    .clk_modidx     = DT_INST_CLOCKS_CELL(0, clock_module_index),
    .clk_src        = DT_INST_CLOCKS_CELL(0, clock_source),
    .clk_div        = DT_INST_CLOCKS_CELL(0, clock_divider),
#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_SCC
    .clkctrl_dev    = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(0))),
#endif
    .irq_config_func        = usb_dc_numaker_irq_config_func_inst_0,
    .irq_unconfig_func      = usb_dc_numaker_irq_unconfig_func_inst_0,
#ifdef CONFIG_PINCTRL
#if DT_NODE_HAS_PROP(DT_DRV_INST(0), pinctrl_0)
    .pincfg         = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
#else
    .pincfg         = NULL,
#endif
#endif
    .num_bidir_endpoints    = DT_INST_PROP(0, num_bidir_endpoints),
    .dmabuf_size    = DT_INST_PROP(0, dma_buffer_size),
    .maximum_speed  = DT_INST_ENUM_IDX_OR(0, maximum_speed, DT_USB_MAXIMUM_SPEED_FULL_SPEED),

    .sync_mutex_p   = &sync_mutex_inst_0,
};

static struct usb_dc_numaker_data s_usb_dc_numaker_data_0;

/* USB DC device context instance 0 */
static struct usb_dc_numaker_device s_usb_dc_numaker_device_inst_0 = {
    .config     = &s_usb_dc_numaker_config_inst_0,
    .data       = &s_usb_dc_numaker_data_0,
};

/* Implementation of IRQ configure/unconfigure function instance 0 */

static void usb_dc_numaker_irq_config_func_inst_0(struct usb_dc_numaker_device *dev)
{
    IRQ_CONNECT(DT_INST_IRQN(0),
                DT_INST_IRQ(0, priority),
                usb_dc_numaker_isr,
                &s_usb_dc_numaker_device_inst_0,
                0);

    irq_enable(DT_INST_IRQN(0));
}

static void usb_dc_numaker_irq_unconfig_func_inst_0(struct usb_dc_numaker_device *dev)
{
    irq_disable(DT_INST_IRQN(0));
}

/* Implementations of internal functions */

/* Get USB DC device context instance 0 */
static struct usb_dc_numaker_device *usb_dc_numaker_device_inst_0(void)
{
    return &s_usb_dc_numaker_device_inst_0;
}

/* ISR */
static void usb_dc_numaker_isr(struct usb_dc_numaker_device *dev)
{
    const struct usb_dc_numaker_config *config = dev->config;
    struct usb_dc_numaker_data *data = dev->data;
    struct nu_usb_dc_ep_mgmt *ep_mgmt = &data->ep_mgmt;

    struct nu_usb_dc_msg msg = { 0 };

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    uint32_t volatile u32IntSts = USBD_GET_INT_FLAG();
    uint32_t volatile u32State = USBD_GET_BUS_STATE();

//------------------------------------------------------------------
    if(u32IntSts & USBD_INTSTS_FLDET)
    {
        // Floating detect
        USBD_CLR_INT_FLAG(USBD_INTSTS_FLDET);

        if(USBD_IS_ATTACHED())
        {
            /* USB Plug In */
            USBD_ENABLE_USB();

            /* Message for bottom-half processing */
            /* NOTE: According to Zephyr USB device stack, USB_DC_CONNECTED means H/W
                     enumeration has completed and isn't consistent with VBUS attached
                     here. */

            LOG_INF("USB plug-in");
        }
        else
        {
            /* USB Un-plug */
            USBD_DISABLE_USB();

            /* Message for bottom-half processing */
            msg.msg_type = NU_USB_DC_MSG_TYPE_CB_STATE;
            msg.cb_state.status_code = USB_DC_DISCONNECTED;
            nu_usb_dc_send_msg(dev, &msg);

            LOG_INF("USB unplug");
        }
    }

//------------------------------------------------------------------
    if(u32IntSts & USBD_INTSTS_WAKEUP)
    {
        /* Clear event flag */
        USBD_CLR_INT_FLAG(USBD_INTSTS_WAKEUP);

        LOG_INF("USB wake-up");
    }

//------------------------------------------------------------------
    if(u32IntSts & USBD_INTSTS_BUS)
    {
        /* Clear event flag */
        USBD_CLR_INT_FLAG(USBD_INTSTS_BUS);

        if(u32State & USBD_STATE_USBRST)
        {
            /* Bus reset */
            USBD_ENABLE_USB();
            //USBD_SwReset();

            /* Bus reset top half */
            nu_usb_dc_bus_reset_th(dev);

            /* Message for bottom-half processing */
            msg.msg_type = NU_USB_DC_MSG_TYPE_CB_STATE;
            msg.cb_state.status_code = USB_DC_RESET;
            nu_usb_dc_send_msg(dev, &msg);

            LOG_INF("USB reset");
        }
        if(u32State & USBD_STATE_SUSPEND)
        {
            /* Enable USB but disable PHY */
            USBD_DISABLE_PHY();

            /* Message for bottom-half processing */
            msg.msg_type = NU_USB_DC_MSG_TYPE_CB_STATE;
            msg.cb_state.status_code = USB_DC_SUSPEND;
            nu_usb_dc_send_msg(dev, &msg);

            LOG_INF("USB suspend");
        }
        if(u32State & USBD_STATE_RESUME)
        {
            /* Enable USB and enable PHY */
            USBD_ENABLE_USB();

            /* Message for bottom-half processing */
            msg.msg_type = NU_USB_DC_MSG_TYPE_CB_STATE;
            msg.cb_state.status_code = USB_DC_RESUME;
            nu_usb_dc_send_msg(dev, &msg);

            LOG_INF("USB resume");
        }
    }

//------------------------------------------------------------------
    if(u32IntSts & USBD_INTSTS_SOFIF_Msk)
    {
        /* Clear event flag */
        USBD_CLR_INT_FLAG(USBD_INTSTS_SOFIF_Msk);

        /* Message for bottom-half processing */
        msg.msg_type = NU_USB_DC_MSG_TYPE_CB_STATE;
        msg.cb_state.status_code = USB_DC_SOF;
        nu_usb_dc_send_msg(dev, &msg);
    }

//------------------------------------------------------------------
    if(u32IntSts & USBD_INTSTS_USB)
    {
        // USB event
        if(u32IntSts & USBD_INTSTS_SETUP)
        {
            // Setup packet
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_SETUP);

            /* Clear the data IN/OUT ready flag of control end-points */
            USBD_STOP_TRANSACTION(EP0);
            USBD_STOP_TRANSACTION(EP1);

            /* NOTE: Following transactions regardless of Data/Status stage will always be DATA1 by USB spec */
            USBD_SET_DATA1(EP0);
            USBD_SET_DATA1(EP1);

            /* Message for bottom-half processing */
            msg.msg_type = NU_USB_DC_MSG_TYPE_CB_EP;
            msg.cb_ep.ep_addr = USB_EP_GET_ADDR(0, USB_EP_DIR_OUT);    // In Zephyr USB device stack, Setup is combined into CTRL OUT.
            msg.cb_ep.status_code = USB_DC_EP_SETUP;
            nu_usb_dc_send_msg(dev, &msg);
        }

        // EP events
        uint32_t epintsts = USBD_GET_EP_INT_FLAG();
        USBD_CLR_EP_INT_FLAG(epintsts);

        while (epintsts) {
            uint32_t hw_ep_idx = u32_count_trailing_zeros(epintsts);

            /* NOTE: We don't enable INNAKEN interrupt, so as long as EP event occurs, we can just regard
             *       one data transaction has completed (ACK for CTRL/BULK/INT or no-ACK for Iso), that is,
             *       no need to check EPSTS0, EPSTS1, etc. */

            /* EP direction, number, and address */
            uint8_t ep_dir = ((config->usbd_base->EP[hw_ep_idx].CFG & USBD_CFG_STATE_Msk) == USBD_CFG_EPMODE_IN)
                ? USB_EP_DIR_IN : USB_EP_DIR_OUT;
            uint8_t ep_idx = (config->usbd_base->EP[hw_ep_idx].CFG & USBD_CFG_EPNUM_Msk) >> USBD_CFG_EPNUM_Pos;
            uint8_t ep_addr = USB_EP_GET_ADDR(ep_idx, ep_dir);

            /* NOTE: See comment in usb_dc_set_address()'s implementation for safe place to
             *       change USB device address */
            if (ep_addr == USB_EP_GET_ADDR(0, USB_EP_DIR_IN)) {
                if ((USBD_GET_ADDR() != data->addr)) {
                    USBD_SET_ADDR(data->addr);
                }
            }

            /* NOTE: See comment on mxpld_ctrlout for why make one copy of CTRL OUT's MXPLD */
            if (ep_addr == USB_EP_GET_ADDR(0, USB_EP_DIR_OUT)) {
                struct nu_usb_dc_ep *ep_ctrlout = ep_mgmt->ep_slots + 0;
                ep_ctrlout->mxpld_ctrlout = USBD_GET_PAYLOAD_LEN(ep_ctrlout->usbd_hw_ep_hndl);
            }

#if defined(CONFIG_USB_DC_NUMAKER_USBD_WORKAROUND_DISALLOW_ISO_IN_OUT_SAME_NUM)
            if (config->usbd_base->EP[hw_ep_idx].CFG & USBD_CFG_TYPE_ISO) {
                /* Restore to not enabling the function */
                *((volatile uint32_t *) (config->usbd_base->RESERVE0 + 0)) |= 0x2;
            }
#endif

            /* Message for bottom-half processing */
            msg.msg_type = NU_USB_DC_MSG_TYPE_CB_EP;
            msg.cb_ep.ep_addr = ep_addr;
            msg.cb_ep.status_code = USB_EP_DIR_IS_IN(ep_addr) ? USB_DC_EP_DATA_IN : USB_DC_EP_DATA_OUT;
            nu_usb_dc_send_msg(dev, &msg);

            /* Have handled this EP and go next */
            epintsts &= ~BIT(hw_ep_idx);
        }
    }
#endif
}

/* Lock this device's data structure */
static void nu_usb_dc_lock(const struct usb_dc_numaker_device *dev)
{
    const struct usb_dc_numaker_config *config = dev->config;

    k_mutex_lock(config->sync_mutex_p, K_FOREVER);
}

/* Unlock this device's data structure */
static void nu_usb_dc_unlock(const struct usb_dc_numaker_device *dev)
{
    const struct usb_dc_numaker_config *config = dev->config;

    k_mutex_unlock(config->sync_mutex_p);
}

/* Interrupt bottom half processing
 *
 * This thread is used to not run Zephyr USB device stack and callbacks in interrupt
 * context. This is because callbacks from this stack may use mutex or other kernel functions
 * which are not supported in interrupt context.
 */
static void nu_usb_dc_msg_hdlr_thread_main(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    __ASSERT_NO_MSG(arg1);
    struct usb_dc_numaker_device *dev = (struct usb_dc_numaker_device *) arg1;
    struct usb_dc_numaker_data *data = dev->data;

    struct nu_usb_dc_msg msg;

    while (true) {
        if (k_msgq_get(&data->msgq, &msg, K_FOREVER)) {
            continue;
        }

        if (msg.msg_type == NU_USB_DC_MSG_TYPE_SW_RECONN) {
            /* S/W reconnect for error recovery */
            nu_usb_dc_lock(dev);
            nu_usb_dc_sw_reconnect(dev);
            nu_usb_dc_unlock(dev);
        } else if (msg.msg_type == NU_USB_DC_MSG_TYPE_CB_STATE) {
            /* Interrupt bottom half processing for bus reset */
            if (msg.cb_state.status_code == USB_DC_RESET) {
                nu_usb_dc_lock(dev);
                nu_usb_dc_bus_reset_bh(dev);
                nu_usb_dc_unlock(dev);
            }

            /* NOTE: Don't run callback with our mutex locked, or we may encounter deadlock
                     because the Zephyr USB device stack can have its own synchronization. */
            if (data->status_cb) {
                /* Disable too many SOF message log */
                if (msg.cb_state.status_code != USB_DC_SOF) {
                    LOG_DBG("Status callback: status_code=%d", msg.cb_state.status_code);
                }
                data->status_cb(msg.cb_state.status_code, NULL);
            } else {
                LOG_WRN("No status callback: status_code=%d", msg.cb_state.status_code);
            }
        } else if (msg.msg_type == NU_USB_DC_MSG_TYPE_CB_EP) {
            uint8_t ep_addr = msg.cb_ep.ep_addr;

            /* Bind EP context to EP address */
            struct nu_usb_dc_ep *ep_cur = nu_usb_dc_ep_mgmt_bind_ep(dev, ep_addr);
            if (!ep_cur) {
                LOG_ERR("Bind EP context: ep_addr=0x%02x", ep_addr);
                continue;
            }

             /* Interrupt bottom half processing for EP */
            nu_usb_dc_lock(dev);
            nu_usb_dc_ep_bh(ep_cur, msg.cb_ep.status_code);
            nu_usb_dc_unlock(dev);

            /* NOTE: Same as above, don't run callback with our mutex locked */
            if (ep_cur->cb) {
                LOG_DBG("EP callback: ep_addr=0x%02x, status_code=%d", ep_addr, msg.cb_ep.status_code);
                ep_cur->cb(ep_addr, msg.cb_ep.status_code);
            } else {
                LOG_WRN("No EP callback: ep_addr=0x%02x, status_code=%d", ep_addr, msg.cb_ep.status_code);
            }
        }
    }
}

/* Send message */
static int nu_usb_dc_send_msg(const struct usb_dc_numaker_device *dev, const struct nu_usb_dc_msg *msg)
{
    struct usb_dc_numaker_data *data = dev->data;
    int rc = 0;

    rc = k_msgq_put(&data->msgq, msg, K_NO_WAIT);
    if (rc < 0) {
        LOG_ERR("Message queue overflow");

        /* TODO: Think over this feature because Iso may accept message loss */
#if 0
        /* Discard all not yet received messages for error recovery below */
        k_msgq_purge(&data->msgq);

        /* Try to recover by S/W reconnect */
        struct nu_usb_dc_msg msg_reconn = {
            .msg_type = NU_USB_DC_MSG_TYPE_SW_RECONN,
        };
        rc = k_msgq_put(&data->msgq, &msg_reconn, K_NO_WAIT);
        if (rc < 0) {
            LOG_ERR("Message queue overflow again");
        }
#endif
    }

    return rc;
}

/* Set up H/W */
static int nu_usb_dc_hw_setup(struct usb_dc_numaker_device *dev)
{
    const struct usb_dc_numaker_config *config = dev->config;
    int rc = 0;

    SYS_UnlockReg();

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* For USBD */
    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) |
        (SYS_USBPHY_USBROLE_STD_USBD | SYS_USBPHY_USBEN_Msk | SYS_USBPHY_SBO_Msk);
#endif

#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_SCC
    struct numaker_scc_subsys scc_subsys;

    memset(&scc_subsys, 0x00, sizeof(scc_subsys));
    scc_subsys.subsys_id        = NUMAKER_SCC_SUBSYS_ID_PCC;
    scc_subsys.pcc.clk_modidx   = config->clk_modidx;
    scc_subsys.pcc.clk_src      = config->clk_src;
    scc_subsys.pcc.clk_div      = config->clk_div;

    /* Equivalent to CLK_EnableModuleClock() */
    rc = clock_control_on(config->clkctrl_dev, (clock_control_subsys_t) &scc_subsys);
    if (rc < 0) {
        goto cleanup;
    }
    /* Equivalent to CLK_SetModuleClock() */
    rc = clock_control_configure(config->clkctrl_dev, (clock_control_subsys_t) &scc_subsys, NULL);
    if (rc < 0) {
        goto cleanup;
    }
#else
    /* Enable module clock */
    CLK_EnableModuleClock(config->clk_modidx);

    /* Select module clock source/divider */
    CLK_SetModuleClock(config->clk_modidx, config->clk_src, config->clk_div);
#endif

    /* Configure pinmux (NuMaker's SYS MFP) */
#ifdef CONFIG_PINCTRL
    if (config->pincfg) {
        rc = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
        if (rc < 0) {
            goto cleanup;
        }
    }
#else
#error  "No separate pinmux function implementation. Enable CONFIG_PINCTRL instead."
#endif 

    SYS_ResetModule(config->id_rst);

    /* Initialize USB DC engine */
#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    config->usbd_base->ATTR = 0x7D0;

    /* Enable software disconnect */
    USBD_SET_SE0();

    /* TODO: Respect DT maximum_speed */
#endif

    /* Initialize IRQ */
    config->irq_config_func(dev);

cleanup:

    SYS_LockReg();

    return rc;
}

/* Un-initialize H/W */
static void nu_usb_dc_hw_shutdown(struct usb_dc_numaker_device *dev)
{
    const struct usb_dc_numaker_config *config = dev->config;

    SYS_UnlockReg();

    /* Uninitialize IRQ */
    config->irq_unconfig_func(dev);

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    USBD_SET_SE0();
    USBD_DISABLE_PHY();
#endif

#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_SCC
    struct numaker_scc_subsys scc_subsys;

    memset(&scc_subsys, 0x00, sizeof(scc_subsys));
    scc_subsys.subsys_id        = NUMAKER_SCC_SUBSYS_ID_PCC;
    scc_subsys.pcc.clk_modidx   = config->clk_modidx;

    /* Equivalent to CLK_DisableModuleClock() */
    clock_control_off(config->clkctrl_dev, (clock_control_subsys_t) &scc_subsys);
#else
    /* Disable module clock */
    CLK_DisableModuleClock(config->clk_modidx);
#endif

    SYS_ResetModule(config->id_rst);

    SYS_LockReg();
}

/* S/W connect */
static void nu_usb_dc_sw_connect(struct usb_dc_numaker_device *dev)
{
    const struct usb_dc_numaker_config *config = dev->config;

    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(config);

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* Clear all interrupts first for clean */
    USBD_CLR_INT_FLAG(USBD_GET_INT_FLAG());

    /* Enable relevant interrupts */
    USBD_ENABLE_INT(USBD_INT_BUS | USBD_INT_USB | USBD_INT_FLDET | USBD_INT_WAKEUP | USBD_INT_SOF);

    /* Clear SE0 (connect) */
    USBD_CLR_SE0();
#endif
}

/* S/W disconnect */
static void nu_usb_dc_sw_disconnect(struct usb_dc_numaker_device *dev)
{
#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* Set SE0 (disconnect) */
    USBD_SET_SE0();
#endif
}

/* S/W disconnect, then connect*/
static void nu_usb_dc_sw_reconnect(struct usb_dc_numaker_device *dev)
{
    /* Keep SE0 for 5 ms, just enough to trigger bus reset (USB spec: SE0 >= 2.5 ms) */
    nu_usb_dc_sw_disconnect(dev);
    k_busy_wait(5000);
    nu_usb_dc_sw_connect(dev);
}

/* Interrupt top half processing for bus reset */
static void nu_usb_dc_bus_reset_th(struct usb_dc_numaker_device *dev)
{
    const struct usb_dc_numaker_config *config = dev->config;
    uint32_t i;

    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(config);

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* Reference BSP USBD driver USBD_SwReset() and modify to fit this port */
    for (i = 0ul; i < USBD_MAX_EP; i++) {
        /* Cancel EP on-going transaction */
        USBD_STOP_TRANSACTION(EP0 + i);

        /* Reset EP to unstalled */
        USBD_CLR_EP_STALL(EP0 + i);

        /* Reset EP data toggle bit to 0 */
        USBD_SET_DATA0(EP0 + i);

        /* Except EP0/EP1 kept resident for CTRL OUT/IN, disable all other EPs */
        if (i >= 2) {
            USBD_CONFIG_EP(EP0 + i, 0);
        }
    }
#endif

    /* Reset USB device address to 0 */
    nu_usb_dc_reset_addr(dev);
}

/* Interrupt bottom half processing for bus reset */
static void nu_usb_dc_bus_reset_bh(struct usb_dc_numaker_device *dev)
{
    const struct usb_dc_numaker_config *config = dev->config;
    struct usb_dc_numaker_data *data = dev->data;
    struct nu_usb_dc_ep_mgmt *ep_mgmt = &data->ep_mgmt;

    struct nu_usb_dc_ep *ep_cur = ep_mgmt->ep_slots;
    struct nu_usb_dc_ep *ep_end = ep_mgmt->ep_slots + config->num_bidir_endpoints;

    for (; ep_cur != ep_end; ep_cur ++) {
        /* Reset EP FIFO */
        nu_usb_dc_ep_fifo_reset(ep_cur);

        /* Abort EP on-going transaction and signal H/W relinquishes DMA buffer ownership */
        nu_usb_dc_ep_abort(ep_cur);

        /* Reset EP to unstalled and data toggle bit to 0 */
        nu_usb_dc_ep_clear_stall(ep_cur);
    }

    /* Reset USB device address to 0 */
    nu_usb_dc_reset_addr(dev);
}

/* Reset USB device address to 0 */
static void nu_usb_dc_reset_addr(struct usb_dc_numaker_device *dev)
{
    struct usb_dc_numaker_data *data = dev->data;

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    USBD_SET_ADDR(0);
#endif
    data->addr = 0;
}

/* Signal remote wakeup */
static void nu_usb_dc_remote_wakeup(struct usb_dc_numaker_device *dev)
{
    const struct usb_dc_numaker_config *config = dev->config;

    /* Enable back USB/PHY first, then generate 'K' >= 1 ms (USB spec) */
#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    USBD_ENABLE_USB();

    config->usbd_base->ATTR |= USBD_ATTR_RWAKEUP_Msk;
    k_busy_wait(1000);
    config->usbd_base->ATTR ^= USBD_ATTR_RWAKEUP_Msk;
#endif
}

/* Initialize all endpoint-related */
static void nu_usb_dc_ep_mgmt_init(struct usb_dc_numaker_device *dev)
{
    const struct usb_dc_numaker_config *config = dev->config;
    struct usb_dc_numaker_data *data = dev->data;
    struct nu_usb_dc_ep_mgmt *ep_mgmt = &data->ep_mgmt;

    /* Initialize all fields to zero except persistent */
    memset(ep_mgmt, 0x00, offsetof(struct nu_usb_dc_ep_mgmt, zero_end));

    struct nu_usb_dc_ep *ep_cur = ep_mgmt->ep_slots;
    struct nu_usb_dc_ep *ep_end = ep_mgmt->ep_slots + config->num_bidir_endpoints;

    /* Initialize all EP slots */
    for (; ep_cur != ep_end; ep_cur ++) {
        /* Initialize all fields to zero except persistent */
        memset(ep_cur, 0x00, offsetof(struct nu_usb_dc_ep, zero_end));

        /* Pointer to the containing device */
        ep_cur->dev = dev;

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
        ep_cur->usbd_hw_ep_hndl = EP0 + (ep_cur - ep_mgmt->ep_slots);
#endif

        /* FIFO needs ownership or not */
#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
        ep_cur->FIFO_need_own = true;
#endif

        /* Initialize FIFO ownership semaphore if not yet, and signal H/W doesn't own it */
        if (!ep_cur->FIFO_own_sem_valid) {
            k_sem_init(&ep_cur->FIFO_own_sem, 1, 1);
            ep_cur->FIFO_own_sem_valid = true;
        } else {
            k_sem_give(&ep_cur->FIFO_own_sem);
        }
    }

    /* Reserve 1st/2nd EP slots (EP0/EP1) for CTRL OUT/In
     *
     * NOTE: This allocation is different than BSP USBD driver which configures EP0/EP1
     *       for CTRL IN/OUT.
     */
    ep_mgmt->ep_slot_idx = 2;
    ep_mgmt->ep_slot_idx_valid = true;

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* Reserve for Setup/CTRL IN/CTRL OUT */
    ep_mgmt->dmabuf_pos = 8 + 64 + 64;
    ep_mgmt->dmabuf_pos_valid = true;
#endif

    /* Configure DMA buffer for Setup packet */
    nu_usb_dc_setup_config_dmabuf(dev);

    /* Reserve 1st EP slot (EP0) for CTRL OUT */
    ep_cur = ep_mgmt->ep_slots + 0;
    ep_cur->valid = true;
    ep_cur->ep_addr_valid = true;
    ep_cur->ep_addr = USB_EP_GET_ADDR(0, USB_EP_DIR_OUT);
#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    nu_usb_dc_ep_config_dmabuf(ep_cur, 8, 64);
#endif
    ep_cur->ep_mps_valid = true;
    ep_cur->ep_mps = 64;

    /* Reserve 2nd EP slot (EP1) for CTRL IN */
    ep_cur = ep_mgmt->ep_slots + 1;
    ep_cur->valid = true;
    ep_cur->ep_addr_valid = true;
    ep_cur->ep_addr = USB_EP_GET_ADDR(0, USB_EP_DIR_IN);
#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    nu_usb_dc_ep_config_dmabuf(ep_cur, 8 + 64, 64);
#endif
    ep_cur->ep_mps_valid = true;
    ep_cur->ep_mps = 64;
}

/* Bind EP context to EP address */
static struct nu_usb_dc_ep *nu_usb_dc_ep_mgmt_bind_ep(struct usb_dc_numaker_device *dev, const uint8_t ep_addr)
{
    struct nu_usb_dc_ep *ep_cur = nu_usb_dc_ep_mgmt_find_ep(dev, ep_addr);
    if (!ep_cur) {
        ep_cur = nu_usb_dc_ep_mgmt_alloc_ep(dev);
        if (!ep_cur) {
            return NULL;
        }

        /* Bind EP context to EP address */
        ep_cur->ep_addr = ep_addr;
        ep_cur->ep_addr_valid = true;
    }

    /* Assert EP context bound to EP address */
    __ASSERT_NO_MSG(ep_cur->valid);
    __ASSERT_NO_MSG(ep_cur->ep_addr_valid);
    __ASSERT_NO_MSG(ep_cur->ep_addr == ep_addr);

    return ep_cur;
}

/* Find EP context by EP address */
static struct nu_usb_dc_ep *nu_usb_dc_ep_mgmt_find_ep(struct usb_dc_numaker_device *dev, const uint8_t ep_addr)
{
    const struct usb_dc_numaker_config *config = dev->config;
    struct usb_dc_numaker_data *data = dev->data;
    struct nu_usb_dc_ep_mgmt *ep_mgmt = &data->ep_mgmt;
    struct nu_usb_dc_ep *ep_cur = ep_mgmt->ep_slots;
    struct nu_usb_dc_ep *ep_end = ep_mgmt->ep_slots + config->num_bidir_endpoints;

    for (; ep_cur != ep_end; ep_cur ++) {
        if (!ep_cur->valid) {
            continue;
        }

        if (!ep_cur->ep_addr_valid) {
            continue;
        }

        if (ep_addr == ep_cur->ep_addr) {
            return ep_cur;
        }
    }

    return NULL;
}

/* Allocate EP context */
static struct nu_usb_dc_ep *nu_usb_dc_ep_mgmt_alloc_ep(struct usb_dc_numaker_device *dev)
{
    const struct usb_dc_numaker_config *config = dev->config;
    struct usb_dc_numaker_data *data = dev->data;
    struct nu_usb_dc_ep_mgmt *ep_mgmt = &data->ep_mgmt;
    struct nu_usb_dc_ep *ep_cur = NULL;

    __ASSERT_NO_MSG(ep_mgmt->ep_slot_idx_valid);

    if (ep_mgmt->ep_slot_idx < config->num_bidir_endpoints) {
        ep_cur = ep_mgmt->ep_slots + ep_mgmt->ep_slot_idx;
        ep_mgmt->ep_slot_idx ++;

        __ASSERT_NO_MSG(!ep_cur->valid);

        /* Indicate this EP slot is allocated */
        ep_cur->valid = true;
    }

    return ep_cur;
}

/* Allocate DMA buffer
 *
 * Return -ENOMEM  on OOM error, or 0 on success with DMA buffer base/size (rounded up) allocated
 */
static int nu_usb_dc_ep_mgmt_alloc_dmabuf(struct usb_dc_numaker_device *dev, uint32_t size, uint32_t *dmabuf_base_p, uint32_t *dmabuf_size_p)
{
    const struct usb_dc_numaker_config *config = dev->config;
    struct usb_dc_numaker_data *data = dev->data;
    struct nu_usb_dc_ep_mgmt *ep_mgmt = &data->ep_mgmt;

    __ASSERT_NO_MSG(dmabuf_base_p);
    __ASSERT_NO_MSG(dmabuf_size_p);

    __ASSERT_NO_MSG(ep_mgmt->dmabuf_pos_valid);

    /* Required to be 8-byte aligned */
    size = ROUND_UP(size, 8);

    ep_mgmt->dmabuf_pos += size;
    if (ep_mgmt->dmabuf_pos > config->dmabuf_size) {
        ep_mgmt->dmabuf_pos -= size;
        return -ENOMEM;                         // OOM error
    } else {
        *dmabuf_base_p = ep_mgmt->dmabuf_pos - size;
        *dmabuf_size_p = size;
        return 0;                               // Success
    }
}

/* Configure DMA buffer for Setup packet */
static void nu_usb_dc_setup_config_dmabuf(struct usb_dc_numaker_device *dev)
{
    const struct usb_dc_numaker_config *config = dev->config;
    struct usb_dc_numaker_data *data = dev->data;
    struct nu_usb_dc_ep_mgmt *ep_mgmt = &data->ep_mgmt;

    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(config);

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    config->usbd_base->STBUFSEG = 0;
#endif

    ep_mgmt->dmabuf_setup_valid = true;
}

/* Copy to user buffer from setup FIFO */
static void nu_usb_dc_setup_fifo_copy_to_user(struct usb_dc_numaker_device *dev, uint8_t *usrbuf)
{
    const struct usb_dc_numaker_config *config = dev->config;

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    uint32_t dmabuf_addr = USBD_BUF_BASE + (config->usbd_base->STBUFSEG & USBD_STBUFSEG_STBUFSEG_Msk);
    USBD_MemCopy(usrbuf, (uint8_t *) dmabuf_addr , 8ul);
#endif
}

/* Interrupt bottom half processing for Setup/EP data transaction */
static void nu_usb_dc_ep_bh(struct nu_usb_dc_ep *ep_cur, enum usb_dc_ep_cb_status_code status_code)
{
    struct usb_dc_numaker_device *dev = ep_cur->dev;
    struct usb_dc_numaker_data *data = dev->data;
    struct nu_usb_dc_ep_mgmt *ep_mgmt = &data->ep_mgmt;

    if (status_code == USB_DC_EP_SETUP) {
        /* Zephyr USB device stack logically uses CTRL OUT for Setup packet */
        __ASSERT_NO_MSG(ep_cur->ep_addr == USB_EP_GET_ADDR(0, USB_EP_DIR_OUT));

        if (nu_usb_dc_ep_fifo_used(ep_cur)) {
            LOG_WRN("New Setup will override previous Control OUT data");
        }

        /* We should have reserved 1st/2nd EP slots for CTRL OUT/IN */
        __ASSERT_NO_MSG(ep_cur->ep_addr == USB_EP_GET_ADDR(0, USB_EP_DIR_OUT));
        __ASSERT_NO_MSG((ep_cur + 1)->ep_addr == USB_EP_GET_ADDR(0, USB_EP_DIR_IN));

        /* Reset CTRL IN/OUT FIFO due to new Setup packet */
        nu_usb_dc_ep_fifo_reset(ep_cur);
        nu_usb_dc_ep_fifo_reset(ep_cur + 1);

        /* Relinquish CTRL OUT/IN DMA buffer ownership on behalf of H/W */
        nu_usb_dc_ep_abort(ep_cur);
        nu_usb_dc_ep_abort(ep_cur + 1);

        /* Mark new Setup packet for read */
        __ASSERT_NO_MSG(ep_mgmt->dmabuf_setup_valid);
        nu_usb_dc_setup_fifo_copy_to_user(dev, (uint8_t *) &ep_mgmt->setup_packet);
        ep_mgmt->new_setup = true;
    } else if (status_code == USB_DC_EP_DATA_OUT) {
        __ASSERT_NO_MSG(USB_EP_DIR_IS_OUT(ep_cur->ep_addr));

        /* Update EP read FIFO */
        nu_usb_dc_ep_fifo_update(ep_cur);

        /* Relinquish EP FIFO ownership on behalf of H/W */
        if (ep_cur->FIFO_need_own) {
            k_sem_give(&ep_cur->FIFO_own_sem);
        }
    } else if (status_code == USB_DC_EP_DATA_IN) {
        __ASSERT_NO_MSG(USB_EP_DIR_IS_IN(ep_cur->ep_addr));

        /* Update EP write FIFO */
        nu_usb_dc_ep_fifo_update(ep_cur);

        /* Relinquish EP FIFO ownership on behalf of H/W */
        if (ep_cur->FIFO_need_own) {
            k_sem_give(&ep_cur->FIFO_own_sem);
        }
    }        
}

/* Configure EP DMA buffer */
static void nu_usb_dc_ep_config_dmabuf(struct nu_usb_dc_ep *ep_cur, uint32_t dmabuf_base, uint32_t dmabuf_size)
{
#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(dmabuf_size);

    USBD_SET_EP_BUF_ADDR(ep_cur->usbd_hw_ep_hndl, dmabuf_base);
#endif

    ep_cur->dmabuf_valid = true;
    ep_cur->dmabuf_base = dmabuf_base;
    ep_cur->dmabuf_size = dmabuf_size;

    /* FIFO is implemented using DMA buffer. Reset it on DMA buffer (re)configure */
    nu_usb_dc_ep_fifo_reset(ep_cur);
}

/* Reset EP FIFO for e.g. initialize/flush operation
 *
 * NOTE: EP DMA buffer may not be configured yet at e.g. EP initialize stage.
 */
static void nu_usb_dc_ep_fifo_reset(struct nu_usb_dc_ep *ep_cur)
{
    struct usb_dc_numaker_device *dev = ep_cur->dev;
    const struct usb_dc_numaker_config *config = dev->config;

    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(config);

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    if (USB_EP_DIR_IS_OUT(ep_cur->ep_addr)) {
        /* Read FIFO */
        if (ep_cur->dmabuf_valid) {
            ep_cur->read_fifo_pos = ep_cur->dmabuf_base;
            ep_cur->read_fifo_used = 0;
        }
    } else {
        /* Write FIFO */
        if (ep_cur->dmabuf_valid) {
            ep_cur->write_fifo_pos = ep_cur->dmabuf_base;
            ep_cur->write_fifo_free = nu_usb_dc_ep_fifo_max(ep_cur);
        }
    }
#endif
}

/* Copy to user buffer from EP FIFO
 *
 * size_p holds size to copy/copied on input/output
 */
static int nu_usb_dc_ep_fifo_copy_to_user(struct nu_usb_dc_ep *ep_cur, uint8_t *usrbuf, uint32_t *size_p)
{
    struct usb_dc_numaker_device *dev = ep_cur->dev;
    const struct usb_dc_numaker_config *config = dev->config;

    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(config);

    __ASSERT_NO_MSG(size_p);
    __ASSERT_NO_MSG(ep_cur->dmabuf_valid);

    /* Clamp to read FIFO used count */
    *size_p = MIN(*size_p, nu_usb_dc_ep_fifo_used(ep_cur));

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    uint32_t dmabuf_addr = USBD_BUF_BASE + ep_cur->read_fifo_pos;
    USBD_MemCopy(usrbuf, (uint8_t *) dmabuf_addr, *size_p);

    /* Advance read FIFO */
    ep_cur->read_fifo_pos += *size_p;
    ep_cur->read_fifo_used -= *size_p;
    if (ep_cur->read_fifo_used == 0) {
        ep_cur->read_fifo_pos = ep_cur->dmabuf_base;
    }
#endif

    return 0;
}

/* Copy from user buffer data to EP FIFO
 *
 * size_p holds size to copy/copied on input/output
 */
static int nu_usb_dc_ep_fifo_copy_from_user(struct nu_usb_dc_ep *ep_cur, const uint8_t *usrbuf, uint32_t *size_p)
{
    struct usb_dc_numaker_device *dev = ep_cur->dev;
    const struct usb_dc_numaker_config *config = dev->config;

    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(config);

    __ASSERT_NO_MSG(size_p);
    __ASSERT_NO_MSG(ep_cur->dmabuf_valid);
    __ASSERT_NO_MSG(ep_cur->ep_mps_valid);
    __ASSERT_NO_MSG(ep_cur->ep_mps <= ep_cur->dmabuf_size);

    /* NOTE: For one-shot implementation, clamp to (MPS - used), instead of write FIFO free count */
    //*size_p = MIN(*size_p, nu_usb_dc_ep_fifo_max(ep_cur) - nu_usb_dc_ep_fifo_used(ep_cur));
    __ASSERT_NO_MSG(ep_cur->ep_mps >= nu_usb_dc_ep_fifo_used(ep_cur));
    *size_p = MIN(*size_p, ep_cur->ep_mps - nu_usb_dc_ep_fifo_used(ep_cur));

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    uint32_t dmabuf_addr = USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(ep_cur->usbd_hw_ep_hndl);
    USBD_MemCopy((uint8_t *) dmabuf_addr, (uint8_t *) usrbuf, *size_p);

    /* Advance write FIFO */
    ep_cur->write_fifo_pos += *size_p;
    ep_cur->write_fifo_free -= *size_p;
    if (ep_cur->write_fifo_free == 0) {
        ep_cur->write_fifo_pos = ep_cur->dmabuf_base;
    }
#endif

    return 0;
}

/* Update EP read/write FIFO on DATA OUT/IN completed */
static void nu_usb_dc_ep_fifo_update(struct nu_usb_dc_ep *ep_cur)
{
    __ASSERT_NO_MSG(ep_cur->ep_addr_valid);
    __ASSERT_NO_MSG(ep_cur->dmabuf_valid);

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    if (USB_EP_DIR_IS_OUT(ep_cur->ep_addr)) {
        /* Read FIFO */
        /* NOTE: For one-shot implementation, FIFO gets updated from reset state */
        ep_cur->read_fifo_pos = ep_cur->dmabuf_base;
        /* NOTE: See comment on mxpld_ctrlout for why make one copy of CTRL OUT's MXPLD */
        if (USB_EP_GET_IDX(ep_cur->ep_addr) == 0) {
            ep_cur->read_fifo_used = ep_cur->mxpld_ctrlout;
        } else {
            ep_cur->read_fifo_used = USBD_GET_PAYLOAD_LEN(ep_cur->usbd_hw_ep_hndl);
        }
    } else {
        /* Write FIFO */
        /* NOTE: For one-shot implementation, FIFO gets reset */
        ep_cur->write_fifo_pos = ep_cur->dmabuf_base;
        ep_cur->write_fifo_free = nu_usb_dc_ep_fifo_max(ep_cur);
    }
#endif
}

/* EP FIFO max count in bytes */
static uint32_t nu_usb_dc_ep_fifo_max(struct nu_usb_dc_ep *ep_cur)
{
    __ASSERT_NO_MSG(ep_cur->dmabuf_valid);

    return ep_cur->dmabuf_size;
}

/* EP FIFO used count in bytes */
static uint32_t nu_usb_dc_ep_fifo_used(struct nu_usb_dc_ep *ep_cur)
{
    struct usb_dc_numaker_device *dev = ep_cur->dev;
    const struct usb_dc_numaker_config *config = dev->config;

    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(config);

    __ASSERT_NO_MSG(ep_cur->dmabuf_valid);

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    if (USB_EP_DIR_IS_OUT(ep_cur->ep_addr)) {
        /* Read FIFO */
        return ep_cur->read_fifo_used;
    } else {
        /* Write FIFO */
        return nu_usb_dc_ep_fifo_max(ep_cur) - ep_cur->write_fifo_free;
    }
#endif
}

/* Configure EP major part */
static void nu_usb_dc_ep_config_major(struct nu_usb_dc_ep *ep_cur, const struct usb_dc_ep_cfg_data * const ep_cfg)
{
    ep_cur->ep_mps_valid = true;
    ep_cur->ep_mps = ep_cfg->ep_mps;

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* Configure EP transfer type, DATA0/1 toggle, direction, number, etc. */
    ep_cur->usbd_hw_ep_cfg = 0 |
        ((USB_EP_GET_IDX(ep_cfg->ep_addr) << USBD_CFG_EPNUM_Pos) &          // Endpoint index
            USBD_CFG_EPNUM_Msk) |
        ((ep_cfg->ep_type == USB_DC_EP_ISOCHRONOUS) ? USBD_CFG_TYPE_ISO :   // Isochronous or not
            0) |
         USBD_CFG_EPMODE_DISABLE |                                          // Endpoint IN/OUT, though, default to disabled
        (!USBD_CFG_DSQSYNC_Msk) |                                           // Default to DATA0
        ((ep_cfg->ep_type == USB_DC_EP_CONTROL) ? USBD_CFG_CSTALL : 0) |    // Clear STALL Response in SETUP stage        
        0;
    USBD_CONFIG_EP(ep_cur->usbd_hw_ep_hndl, ep_cur->usbd_hw_ep_cfg);
#endif
}

/* Set EP to stalled */
static void nu_usb_dc_ep_set_stall(struct nu_usb_dc_ep *ep_cur)
{
#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* Set EP to stalled */
    USBD_SET_EP_STALL(ep_cur->usbd_hw_ep_hndl);
#endif
}

/* Reset EP to unstalled and data toggle bit to 0 */
static void nu_usb_dc_ep_clear_stall(struct nu_usb_dc_ep *ep_cur)
{
    struct usb_dc_numaker_device *dev = ep_cur->dev;
    const struct usb_dc_numaker_config *config = dev->config;

    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(config);

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* For CTRL IN/OUT, auto-clear on receipt of next Setup token */

    /* Reset EP to unstalled */
    USBD_CLR_EP_STALL(ep_cur->usbd_hw_ep_hndl);

    /* Reset EP data toggle bit to 0 */
    USBD_SET_DATA0(ep_cur->usbd_hw_ep_hndl);
#endif
}

/* Is EP stalled */
static bool nu_usb_dc_ep_is_stalled(struct nu_usb_dc_ep *ep_cur)
{
    struct usb_dc_numaker_device *dev = ep_cur->dev;
    const struct usb_dc_numaker_config *config = dev->config;

    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(config);

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* Is EP stalled */
    return !!USBD_GET_EP_STALL(ep_cur->usbd_hw_ep_hndl);
#endif
}

/* Enable EP */
static void nu_usb_dc_ep_enable(struct nu_usb_dc_ep *ep_cur)
{
    struct usb_dc_numaker_device *dev = ep_cur->dev;
    const struct usb_dc_numaker_config *config = dev->config;

    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(config);

    /* For safe, EP (re-)enable from clean state */
    nu_usb_dc_ep_abort(ep_cur);
    nu_usb_dc_ep_clear_stall(ep_cur);
    nu_usb_dc_ep_fifo_reset(ep_cur);

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* Enable EP to IN/OUT */
    ep_cur->usbd_hw_ep_cfg = (ep_cur->usbd_hw_ep_cfg & ~USBD_CFG_STATE_Msk) |
        (USB_EP_DIR_IS_IN(ep_cur->ep_addr) ? USBD_CFG_EPMODE_IN : USBD_CFG_EPMODE_OUT);
    USBD_CONFIG_EP(ep_cur->usbd_hw_ep_hndl, ep_cur->usbd_hw_ep_cfg);

    /* No separate EP interrupt control */
#endif
}

/* Disable EP */
static void nu_usb_dc_ep_disable(struct nu_usb_dc_ep *ep_cur)
{
    struct usb_dc_numaker_device *dev = ep_cur->dev;
    const struct usb_dc_numaker_config *config = dev->config;

    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(config);

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* No separate EP interrupt control */

    /* Disable EP */
    ep_cur->usbd_hw_ep_cfg = (ep_cur->usbd_hw_ep_cfg & ~USBD_CFG_STATE_Msk) | USBD_CFG_EPMODE_DISABLE;
    USBD_CONFIG_EP(ep_cur->usbd_hw_ep_hndl, ep_cur->usbd_hw_ep_cfg);
#endif
}

/* Suppress further USB_DC_EP_DATA_OUT events by replying NAK or disabling interrupt */
static void nu_usb_dc_ep_out_wait(struct nu_usb_dc_ep *ep_cur)
{
    struct usb_dc_numaker_device *dev = ep_cur->dev;
    const struct usb_dc_numaker_config *config = dev->config;

    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(config);

    __ASSERT_NO_MSG(USB_EP_DIR_IS_OUT(ep_cur->ep_addr));

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* Needn't further control because NAK is replied until USBD_SET_PAYLOAD_LEN() */
#endif
}

/* Start EP data transaction */
static void nu_usb_dc_ep_trigger(struct nu_usb_dc_ep *ep_cur, uint32_t len)
{
    struct usb_dc_numaker_device *dev = ep_cur->dev;
    const struct usb_dc_numaker_config *config = dev->config;
    struct usb_dc_numaker_data *data = dev->data;
    struct nu_usb_dc_ep_mgmt *ep_mgmt = &data->ep_mgmt;

    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(config);
    ARG_UNUSED(ep_mgmt);

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    USBD_SET_PAYLOAD_LEN(ep_cur->usbd_hw_ep_hndl, len);

#if defined(CONFIG_USB_DC_NUMAKER_USBD_WORKAROUND_DISALLOW_ISO_IN_OUT_SAME_NUM)
    if (ep_cur->usbd_hw_ep_cfg & USBD_CFG_TYPE_ISO) {
        /* Enable forcing Iso In Tx on Iso In token arrival (even without above
         * USBD_SET_PAYLOAD_LEN()), to not get blocked by Iso Out */
        *((volatile uint32_t *) (config->usbd_base->RESERVE0 + 0)) &= 0xFFFFFFFD;
    }
#endif
#endif
}

/* Abort EP */
static void nu_usb_dc_ep_abort(struct nu_usb_dc_ep *ep_cur)
{
    struct usb_dc_numaker_device *dev = ep_cur->dev;
    const struct usb_dc_numaker_config *config = dev->config;

    /* Suppress 'unused variable' warning on some build */
    ARG_UNUSED(config);

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_numaker_usbd) && defined(CONFIG_USB_DC_NUMAKER_USBD)
    /* Abort EP on-going transaction */
    USBD_STOP_TRANSACTION(ep_cur->usbd_hw_ep_hndl);
    
#if defined(CONFIG_USB_DC_NUMAKER_USBD_WORKAROUND_DISALLOW_ISO_IN_OUT_SAME_NUM)
    if (ep_cur->usbd_hw_ep_cfg & USBD_CFG_TYPE_ISO) {
        /* Restore to not enabling the function */
        *((volatile uint32_t *) (config->usbd_base->RESERVE0 + 0)) |= 0x2;
    }
#endif
#endif

    /* Relinquish EP FIFO ownership on behalf of H/W */
    if (ep_cur->FIFO_need_own) {
        if (ep_cur->FIFO_own_sem_valid) {
            k_sem_give(&ep_cur->FIFO_own_sem);
        }
    }
}
