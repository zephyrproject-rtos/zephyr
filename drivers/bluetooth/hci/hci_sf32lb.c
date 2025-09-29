/*
 * Copyright (c) SiFli Technologies(Nanjing) Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_bt_mbox

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>

#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth.h>

#include "common/bt_str.h"

#include <ipc_queue.h>
#include <ipc_hw.h>
#include <bf0_mbox_common.h> 


#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_driver);

#define IPC_TIMEOUT_MS      10

struct h4_data
{
    struct
    {
        struct net_buf *buf;
        struct k_fifo   fifo;

        uint16_t        remaining;
        uint16_t        discard;

        bool            have_hdr;
        bool            discardable;

        uint8_t         hdr_len;

        uint8_t         type;
        union
        {
            struct bt_hci_evt_hdr evt;
            struct bt_hci_acl_hdr acl;
            struct bt_hci_iso_hdr iso;
            struct bt_hci_sco_hdr sco;
            uint8_t hdr[4];
        };
    } rx;

    struct
    {
        uint8_t         type;
        struct net_buf *buf;
        struct k_fifo   fifo;
    } tx;

    struct k_sem sem;
    bt_hci_recv_t recv;
    ipc_queue_handle_t ipc_port;
};

struct h4_config
{
    const struct device *mbox;
    k_thread_stack_t *rx_thread_stack;
    size_t rx_thread_stack_size;
    struct k_thread *rx_thread;
    ipc_queue_handle_t ipc_port;
};

static inline void process_tx(const struct device *dev);
static inline void process_rx(const struct device *dev);

static int32_t mbox_rx_ind(ipc_queue_handle_t handle, size_t size)
{
    const struct device *dev=DEVICE_DT_GET(DT_NODELABEL(mbox));
    struct h4_data *h4 = dev->data;
    
    LOG_DBG("mbox_rx_ind");
    k_sem_give(&(h4->sem));    
    return 0;
}

static void mbox_sf32lb_isr(const struct device *dev)
{
    LOG_DBG("mbox_sf32lb_isr %p", (void *)dev);
    LCPU2HCPU_IRQHandler();    
}

static void zbt_config_mailbox(const struct device *dev)
{
    ipc_queue_cfg_t q_cfg;
    struct h4_data *h4 = dev->data;

    
#ifndef CONFIG_SIFLI_LXT_DISABLE    
    /* Enable LXT32K */
    HAL_PMU_EnableXTAL32();
    if (HAL_PMU_LXTReady() != HAL_OK) {
        HAL_ASSERT(0);
    }
    /* RTC/GTIME/LPTIME Using same low power clock source */
    HAL_RTC_ENABLE_LXT();        
#endif    
    HAL_HPAON_StartGTimer();

    k_sem_init(&(h4->sem), 0, 1);   

    q_cfg.qid = 0;
    q_cfg.tx_buf_size = HCPU2LCPU_MB_CH1_BUF_SIZE;
    q_cfg.tx_buf_addr = HCPU2LCPU_MB_CH1_BUF_START_ADDR;
    q_cfg.tx_buf_addr_alias = HCPU_ADDR_2_LCPU_ADDR(HCPU2LCPU_MB_CH1_BUF_START_ADDR);;
#ifndef SF32LB52X
    /* Config IPC queue. */
    q_cfg.rx_buf_addr = LCPU_ADDR_2_HCPU_ADDR(LCPU2HCPU_MB_CH1_BUF_START_ADDR);
#else 
    uint8_t rev_id = __HAL_SYSCFG_GET_REVID();
    if (rev_id < HAL_CHIP_REV_ID_A4)
    {
        q_cfg.rx_buf_addr = LCPU_ADDR_2_HCPU_ADDR(LCPU2HCPU_MB_CH1_BUF_START_ADDR);
    }
    else
    {
        q_cfg.rx_buf_addr = LCPU_ADDR_2_HCPU_ADDR(LCPU2HCPU_MB_CH1_BUF_REV_B_START_ADDR);
    }
#endif

    q_cfg.rx_ind = NULL;
    q_cfg.user_data = 0;

    if (q_cfg.rx_ind == NULL) {
        q_cfg.rx_ind = mbox_rx_ind;
    }
    
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), mbox_sf32lb_isr, DEVICE_DT_INST_GET(0),0);
    h4->ipc_port = ipc_queue_init(&q_cfg);
    __ASSERT(IPC_QUEUE_INVALID_HANDLE != h4->ipc_port, "Invalid Handle");
    if (ipc_queue_open(h4->ipc_port))
        __ASSERT(0,"Could not open IPC");
    
}

#ifdef CONFIG_BT_DEBUG_MONITOR_SF32LB

#include "zephyr/drivers/uart.h"
#define HCI_TRACE_HEADER_LEN 16
#define H4TL_PACKET_HOST 0x61
#define H4TL_PACKET_CTRL 0x62

static uint16_t s_hci_trace_seq;
static const struct device *const monitor_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static void poll_out(char c)
{
	uart_poll_out(monitor_dev, c);
}

void sf32lb_hci_trace(uint8_t type, const uint8_t *data, uint16_t len, uint8_t h4tl_packet) {
  uint8_t trace_hdr[HCI_TRACE_HEADER_LEN];

  /* Magic for SiFli HCI, 'PBTS' */
  trace_hdr[0] = 0x50U;
  trace_hdr[1] = 0x42U;
  trace_hdr[2] = 0x54U;
  trace_hdr[3] = 0x53U;
  trace_hdr[4] = 0x06U;
  trace_hdr[5] = 0x01U;
  trace_hdr[6] = (len + 8U) & 0xFFU;
  trace_hdr[7] = (len + 8U) >> 8U;
  trace_hdr[8] = s_hci_trace_seq & 0xFFU;
  trace_hdr[9] = s_hci_trace_seq >> 8U;
  trace_hdr[10] = 0U;
  trace_hdr[11] = 0U;
  trace_hdr[12] = 0U;
  trace_hdr[13] = 0U;
  trace_hdr[14] = h4tl_packet;
  trace_hdr[15] = type;

  s_hci_trace_seq++;

  unsigned int key=irq_lock();
  for (uint8_t i = 0U; i < HCI_TRACE_HEADER_LEN; i++) {
    poll_out(trace_hdr[i]);
  }

  for (uint16_t i = 0U; i < len; i++) {
    poll_out(data[i]);
  }
  irq_unlock(key);
}
#else
#define sf32lb_hci_trace(type, data, len, h4tl_packet)
#endif /* CONFIG_BT_DEBUG_MONITOR_SF32LB */

static inline void h4_get_type(const struct device *dev)
{
    struct h4_data *h4 = dev->data;

    /* Get packet type */
    if (ipc_queue_read(h4->ipc_port, &h4->rx.type, 1) != 1)
    {
        LOG_WRN("Unable to read H:4 packet type");
        h4->rx.type = BT_HCI_H4_NONE;
        return;
    }

    switch (h4->rx.type)
    {
    case BT_HCI_H4_EVT:
        h4->rx.remaining = sizeof(h4->rx.evt);
        h4->rx.hdr_len = h4->rx.remaining;
        break;
    case BT_HCI_H4_ACL:
        h4->rx.remaining = sizeof(h4->rx.acl);
        h4->rx.hdr_len = h4->rx.remaining;
        break;
    case BT_HCI_H4_SCO:
        if (IS_ENABLED(CONFIG_BT_CLASSIC))
        {        
            h4->rx.remaining = sizeof(h4->rx.sco);
            h4->rx.hdr_len = h4->rx.remaining;
        }
        else 
        {
            goto error;      
        }
                        
        break;        
    case BT_HCI_H4_ISO:
        if (IS_ENABLED(CONFIG_BT_ISO))
        {
            h4->rx.remaining = sizeof(h4->rx.iso);
            h4->rx.hdr_len = h4->rx.remaining;
            break;
        }
        else 
        {
            goto error;
        }
        break;
	default:
		goto error;
    }
    return;    
error:
    LOG_ERR("Unknown H:4 type 0x%02x", h4->rx.type);
    h4->rx.type = BT_HCI_H4_NONE;    
}

static void h4_read_hdr(const struct device *dev)
{
    struct h4_data *h4 = dev->data;
    int bytes_read = h4->rx.hdr_len - h4->rx.remaining;
    int ret;

    ret = ipc_queue_read(h4->ipc_port, h4->rx.hdr + bytes_read, h4->rx.remaining);
    if (unlikely(ret < 0))
    {
        LOG_ERR("Unable to read from UART (ret %d)", ret);
    }
    else
    {
        h4->rx.remaining -= ret;
    }
}

static inline void get_acl_hdr(const struct device *dev)
{
    struct h4_data *h4 = dev->data;

    h4_read_hdr(dev);

    if (!h4->rx.remaining)
    {
        struct bt_hci_acl_hdr *hdr = &h4->rx.acl;

        h4->rx.remaining = sys_le16_to_cpu(hdr->len);
        LOG_DBG("Got ACL header. Payload %u bytes", h4->rx.remaining);
        h4->rx.have_hdr = true;
    }
}

static inline void get_sco_hdr(const struct device *dev)
{
    struct h4_data *h4 = dev->data;

    h4_read_hdr(dev);

    if (!h4->rx.remaining)
    {
        struct bt_hci_sco_hdr *hdr = &h4->rx.sco;

        h4->rx.remaining = hdr->len;
        LOG_DBG("Got SCO header. Payload %u bytes", h4->rx.remaining);
        h4->rx.have_hdr = true;
    }
}


static inline void get_iso_hdr(const struct device *dev)
{
    struct h4_data *h4 = dev->data;

    h4_read_hdr(dev);

    if (!h4->rx.remaining)
    {
        struct bt_hci_iso_hdr *hdr = &h4->rx.iso;

        h4->rx.remaining = bt_iso_hdr_len(sys_le16_to_cpu(hdr->len));
        LOG_DBG("Got ISO header. Payload %u bytes", h4->rx.remaining);

        h4->rx.have_hdr = true;
    }
}

static inline void get_evt_hdr(const struct device *dev)
{
    struct h4_data *h4 = dev->data;

    struct bt_hci_evt_hdr *hdr = &h4->rx.evt;

    h4_read_hdr(dev);

    if (h4->rx.hdr_len == sizeof(*hdr) && h4->rx.remaining < sizeof(*hdr))
    {
        switch (h4->rx.evt.evt)
        {
        case BT_HCI_EVT_LE_META_EVENT:
            h4->rx.remaining++;
            h4->rx.hdr_len++;
            break;
#if defined(CONFIG_BT_CLASSIC)
        case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
        case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
            h4->rx.discardable = true;
            break;
#endif
        default:
            ;
        }
    }

    if (!h4->rx.remaining)
    {
        if (h4->rx.evt.evt == BT_HCI_EVT_LE_META_EVENT &&
                 ((h4->rx.hdr[sizeof(*hdr)] == BT_HCI_EVT_LE_ADVERTISING_REPORT)
                 || (h4->rx.hdr[sizeof(*hdr)] == BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT)))
        {
            LOG_DBG("Marking adv report as discardable");
            h4->rx.discardable = true;
        }

        h4->rx.remaining = hdr->len - (h4->rx.hdr_len - sizeof(*hdr));
        LOG_DBG("Got event header. Payload %u bytes", hdr->len);
        h4->rx.have_hdr = true;
    }
}

static inline void copy_hdr(struct h4_data *h4)
{
    net_buf_add_mem(h4->rx.buf, h4->rx.hdr, h4->rx.hdr_len);
}

static void reset_rx(struct h4_data *h4)
{
    h4->rx.type = BT_HCI_H4_NONE;
    h4->rx.remaining = 0U;
    h4->rx.have_hdr = false;
    h4->rx.hdr_len = 0U;
    h4->rx.discardable = false;
}

static struct net_buf *get_rx(struct h4_data *h4, k_timeout_t timeout)
{
    LOG_DBG("type 0x%02x, evt 0x%02x", h4->rx.type, h4->rx.evt.evt);

    switch (h4->rx.type)
    {
    case BT_HCI_H4_EVT:
        return bt_buf_get_evt(h4->rx.evt.evt, h4->rx.discardable, timeout);
    case BT_HCI_H4_ACL:
        return bt_buf_get_rx(BT_BUF_ACL_IN, timeout);
    case BT_HCI_H4_SCO:
        if (IS_ENABLED(CONFIG_BT_CLASSIC))
        {
            LOG_ERR("SCO not supported by host stack.");
        }
        break;
    case BT_HCI_H4_ISO:
        if (IS_ENABLED(CONFIG_BT_ISO))
        {
            return bt_buf_get_rx(BT_BUF_ISO_IN, timeout);
        }
        break;
    default:
        LOG_ERR("Invalid rx type 0x%02x", h4->rx.type);
    }

    return NULL;
}

static void rx_thread(void *p1, void *p2, void *p3)
{
    const struct device *dev = p1;
    struct h4_data *data = dev->data;

    while (1)
    {
        int len;
        k_sem_take(&(data->sem), K_FOREVER);

        len=ipc_queue_get_rx_size(data->ipc_port);
        while (len)
        {
            LOG_DBG("rx_thread len %d", len);
            struct h4_data *h4 = dev->data;
            struct net_buf *buf;
            process_rx(dev);
            buf = k_fifo_get(&h4->rx.fifo, K_NO_WAIT);
            while (buf)
            {
                LOG_DBG("Calling bt_recv(%p),len=%d,data=%p", (void*)buf, buf->len, (void*)buf->data);

                if (buf->len==0 || buf->data==NULL)
                {
                    break;
                }
                
                if (h4->recv) 
                {
                    sf32lb_hci_trace(buf->data[0], &buf->data[1], buf->len-1, H4TL_PACKET_CTRL);
                    h4->recv(dev, buf);
                }
                else
                {
                    net_buf_unref(buf);
                }

                buf = k_fifo_get(&h4->rx.fifo, K_NO_WAIT);
            };
            len=ipc_queue_get_rx_size(data->ipc_port);
        }
        process_tx(dev);
    }
}

static size_t h4_discard(const struct device *dev, size_t len)
{
    struct h4_data *h4 = dev->data;
    uint8_t buf[33];
    int err;

    err = ipc_queue_read(h4->ipc_port, buf, MIN(len, sizeof(buf)));
    if (unlikely(err < 0))
    {
        LOG_ERR("Unable to read from UART (err %d)", err);
        err=0;
    }
    return err;
}

static inline void read_payload(const struct device *dev)
{
    struct h4_data *h4 = dev->data;
    struct net_buf *buf;
    int read;

    if (!h4->rx.buf)
    {
        size_t buf_tailroom;

        h4->rx.buf = get_rx(h4, K_NO_WAIT);
        if (!h4->rx.buf)
        {
            if (h4->rx.discardable)
            {
                LOG_WRN("Discarding event 0x%02x", h4->rx.evt.evt);
                h4->rx.discard = h4->rx.remaining;
                reset_rx(h4);
                return;
            }
            LOG_WRN("Failed to allocate, deferring to rx_thread");
            return;
        }
        LOG_DBG("Allocated rx.buf %p", (void*)h4->rx.buf);

        buf_tailroom = net_buf_tailroom(h4->rx.buf);
        if (buf_tailroom < h4->rx.remaining)
        {
            LOG_ERR("Not enough space in buffer %u/%zu", h4->rx.remaining,
                    buf_tailroom);
            h4->rx.discard = h4->rx.remaining;
            reset_rx(h4);
            return;
        }
        copy_hdr(h4);
    }

    read = ipc_queue_read(h4->ipc_port, net_buf_tail(h4->rx.buf), h4->rx.remaining);
    if (unlikely(read < 0))
    {
        LOG_ERR("Failed to read UART (err %d)", read);
        return;
    }

    net_buf_add(h4->rx.buf, read);
    h4->rx.remaining -= read;

    LOG_DBG("got %d bytes, remaining %u", read, h4->rx.remaining);
    LOG_DBG("Payload (len %u): %s", h4->rx.buf->len,
            bt_hex(h4->rx.buf->data, h4->rx.buf->len));

    if (h4->rx.remaining)
    {
        return;
    }

    buf = h4->rx.buf;
    h4->rx.buf = NULL;

    reset_rx(h4);

    LOG_DBG("Putting buf %p to rx fifo", (void*)buf);
    k_fifo_put(&h4->rx.fifo, buf);
}

static inline void read_header(const struct device *dev)
{
    struct h4_data *h4 = dev->data;

    switch (h4->rx.type)
    {
    case BT_HCI_H4_NONE:
        h4_get_type(dev);
        return;
    case BT_HCI_H4_EVT:
        get_evt_hdr(dev);
        break;
    case BT_HCI_H4_ACL:
        get_acl_hdr(dev);
        break;
    case BT_HCI_H4_SCO:
        if (IS_ENABLED(CONFIG_BT_CLASSIC))
        {
            get_sco_hdr(dev);
            break;
        }
        else
        {
            LOG_ERR("SCO got unexpected\n");
        }        
        break;
    case BT_HCI_H4_ISO:
        if (IS_ENABLED(CONFIG_BT_ISO))
        {
            get_iso_hdr(dev);
            break;
        }
        else
        {
            LOG_ERR("ISO got unexpected\n");
        }
        __fallthrough;
    default:
        CODE_UNREACHABLE;
        return;
    }

    if (h4->rx.have_hdr && h4->rx.buf)
    {
        if (h4->rx.remaining > net_buf_tailroom(h4->rx.buf))
        {
            LOG_ERR("Not enough space in buffer");
            h4->rx.discard = h4->rx.remaining;
            reset_rx(h4);
        }
        else
        {
            copy_hdr(h4);
        }
    }
}

static inline void process_tx(const struct device *dev)
{
    struct h4_data *h4 = dev->data;
    int bytes;

    if (!h4->tx.buf)
    {
        h4->tx.buf = k_fifo_get(&h4->tx.fifo, K_NO_WAIT);
        if (!h4->tx.buf)
        {
            return;
        }
    }
    else
    {
        LOG_WRN("Other tx is running");
        return;
    }

    while (h4->tx.buf)
    {
        LOG_DBG("process_tx data %p, type %d len %d", (void*)h4->tx.buf->data, h4->tx.buf->data[0], h4->tx.buf->len);
        sf32lb_hci_trace(h4->tx.buf->data[0], &h4->tx.buf->data[1], h4->tx.buf->len-1, H4TL_PACKET_HOST);
        bytes = ipc_queue_write(h4->ipc_port, h4->tx.buf->data, h4->tx.buf->len, IPC_TIMEOUT_MS);
        if (unlikely(bytes < 0))
        {
            LOG_ERR("Unable to write to UART (err %d)", bytes);
        }
        else
        {
            LOG_DBG("process_tx bytes %d", bytes);
            net_buf_pull(h4->tx.buf, bytes);
        }
        if (h4->tx.buf->len)
        {
            return;
        }
        h4->tx.type = BT_HCI_H4_NONE;
        LOG_DBG("process_tx net_buf_unref");
        net_buf_unref(h4->tx.buf);
        h4->tx.buf = k_fifo_get(&h4->tx.fifo, K_NO_WAIT);
    }
}

static inline void process_rx(const struct device *dev)
{
    struct h4_data *h4 = dev->data;

    LOG_DBG("remaining %u discard %u have_hdr %u rx.buf %p len %u",
            h4->rx.remaining, h4->rx.discard, h4->rx.have_hdr, (void*)h4->rx.buf,
            h4->rx.buf ? h4->rx.buf->len : 0);

    if (h4->rx.discard)
    {
        h4->rx.discard -= h4_discard(dev, h4->rx.discard);
        return;
    }

    if (h4->rx.have_hdr)
    {
        read_payload(dev);
    }
    else
    {
        read_header(dev);
    }
}


static int h4_send(const struct device *dev, struct net_buf *buf)
{
    struct h4_data *h4 = dev->data;

    LOG_DBG("buf %p type %u len %u", (void*)buf, buf->data[0], buf->len);

    k_fifo_put(&h4->tx.fifo, buf);
    k_sem_give(&(h4->sem));

    return 0;
}

static int h4_open(const struct device *dev, bt_hci_recv_t recv)
{
    struct h4_data *h4 = dev->data;
    const struct h4_config *cfg = dev->config;
    k_tid_t tid;

    h4->recv = recv;

    zbt_config_mailbox(dev);
    lcpu_power_on();
    k_sleep(K_MSEC(50));
    
    LOG_DBG("h4 open %p", (void*)recv);
    tid = k_thread_create(cfg->rx_thread, cfg->rx_thread_stack,
                          cfg->rx_thread_stack_size,
                          rx_thread, (void *)dev, NULL, NULL,
                          K_PRIO_COOP(CONFIG_BT_RX_PRIO),
                          0, K_NO_WAIT);
    k_thread_name_set(tid, "hci_rx_th");       
    return 0;
}

static const DEVICE_API(bt_hci, h4_driver_api) =
{
    .open = h4_open,
    .send = h4_send,
};

#define BT_MBOX_DEVICE_INIT(inst) \
    static K_KERNEL_STACK_DEFINE(rx_thread_stack_##inst, CONFIG_BT_DRV_RX_STACK_SIZE); \
    static struct k_thread rx_thread_##inst; \
    static const struct h4_config h4_config_##inst = { \
        .rx_thread_stack = rx_thread_stack_##inst, \
        .rx_thread_stack_size = K_KERNEL_STACK_SIZEOF(rx_thread_stack_##inst), \
        .rx_thread = &rx_thread_##inst, \
    }; \
    static struct h4_data h4_data_##inst= { \
		.rx = { \
			.fifo = Z_FIFO_INITIALIZER(h4_data_##inst.rx.fifo), \
		}, \
		.tx = { \
			.fifo = Z_FIFO_INITIALIZER(h4_data_##inst.tx.fifo), \
		}, \
    }; \
    DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &h4_data_##inst, &h4_config_##inst, \
              POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &h4_driver_api)

BT_MBOX_DEVICE_INIT(0)


