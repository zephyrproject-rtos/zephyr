/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing OS specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "rpu_hw_if.h"
#include "shim.h"
#include "work.h"
#include "timer.h"
#include "osal_ops.h"
#include "qspi_if.h"

LOG_MODULE_REGISTER(wifi_nrf, CONFIG_WIFI_NRF70_LOG_LEVEL);

struct zep_shim_intr_priv *intr_priv;

static void *zep_shim_mem_alloc(size_t size)
{
	size_t size_aligned = ROUND_UP(size, 4);

	return k_malloc(size_aligned);
}

static void *zep_shim_mem_zalloc(size_t size)
{
	size_t size_aligned = ROUND_UP(size, 4);

	return k_calloc(size_aligned, sizeof(char));
}

static void *zep_shim_mem_cpy(void *dest, const void *src, size_t count)
{
	return memcpy(dest, src, count);
}

static void *zep_shim_mem_set(void *start, int val, size_t size)
{
	return memset(start, val, size);
}

static int zep_shim_mem_cmp(const void *addr1,
			    const void *addr2,
			    size_t size)
{
	return memcmp(addr1, addr2, size);
}

static unsigned int zep_shim_qspi_read_reg32(void *priv, unsigned long addr)
{
	unsigned int val;
	struct zep_shim_bus_qspi_priv *qspi_priv = priv;
	struct qspi_dev *dev;

	dev = qspi_priv->qspi_dev;

	if (addr < 0x0C0000) {
		dev->hl_read(addr, &val, 4);
	} else {
		dev->read(addr, &val, 4);
	}

	return val;
}

static void zep_shim_qspi_write_reg32(void *priv, unsigned long addr, unsigned int val)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = priv;
	struct qspi_dev *dev;

	dev = qspi_priv->qspi_dev;

	dev->write(addr, &val, 4);
}

static void zep_shim_qspi_cpy_from(void *priv, void *dest, unsigned long addr, size_t count)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = priv;
	struct qspi_dev *dev;
	size_t count_aligned = ROUND_UP(count, 4);

	dev = qspi_priv->qspi_dev;

	if (addr < 0x0C0000) {
		dev->hl_read(addr, dest, count_aligned);
	} else {
		dev->read(addr, dest, count_aligned);
	}
}

static void zep_shim_qspi_cpy_to(void *priv, unsigned long addr, const void *src, size_t count)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = priv;
	struct qspi_dev *dev;
	size_t count_aligned = ROUND_UP(count, 4);

	dev = qspi_priv->qspi_dev;

	dev->write(addr, src, count_aligned);
}

static void *zep_shim_spinlock_alloc(void)
{
	struct k_sem *lock = NULL;

	lock = k_malloc(sizeof(*lock));

	if (!lock) {
		LOG_ERR("%s: Unable to allocate memory for spinlock", __func__);
	}

	return lock;
}

static void zep_shim_spinlock_free(void *lock)
{
	k_free(lock);
}

static void zep_shim_spinlock_init(void *lock)
{
	k_sem_init(lock, 1, 1);
}

static void zep_shim_spinlock_take(void *lock)
{
	k_sem_take(lock, K_FOREVER);
}

static void zep_shim_spinlock_rel(void *lock)
{
	k_sem_give(lock);
}

static void zep_shim_spinlock_irq_take(void *lock, unsigned long *flags)
{
	k_sem_take(lock, K_FOREVER);
}

static void zep_shim_spinlock_irq_rel(void *lock, unsigned long *flags)
{
	k_sem_give(lock);
}

static int zep_shim_pr_dbg(const char *fmt, va_list args)
{
	static char buf[80];

	vsnprintf(buf, sizeof(buf), fmt, args);

	LOG_DBG("%s", buf);

	return 0;
}

static int zep_shim_pr_info(const char *fmt, va_list args)
{
	static char buf[80];

	vsnprintf(buf, sizeof(buf), fmt, args);

	LOG_INF("%s", buf);

	return 0;
}

static int zep_shim_pr_err(const char *fmt, va_list args)
{
	static char buf[256];

	vsnprintf(buf, sizeof(buf), fmt, args);

	LOG_ERR("%s", buf);

	return 0;
}

struct nwb {
	unsigned char *data;
	unsigned char *tail;
	int len;
	int headroom;
	void *next;
	void *priv;
	int iftype;
	void *ifaddr;
	void *dev;
	int hostbuffer;
	void *cleanup_ctx;
	void (*cleanup_cb)();
	unsigned char priority;
	bool chksum_done;
};

static void *zep_shim_nbuf_alloc(unsigned int size)
{
	struct nwb *nbuff;

	nbuff = (struct nwb *)k_calloc(sizeof(struct nwb), sizeof(char));

	if (!nbuff) {
		return NULL;
	}

	nbuff->priv = k_calloc(size, sizeof(char));

	if (!nbuff->priv) {
		k_free(nbuff);
		return NULL;
	}

	nbuff->data = (unsigned char *)nbuff->priv;
	nbuff->tail = nbuff->data;
	nbuff->len = 0;
	nbuff->headroom = 0;
	nbuff->next = NULL;

	return nbuff;
}

static void zep_shim_nbuf_free(void *nbuf)
{
	k_free(((struct nwb *)nbuf)->priv);
	k_free(nbuf);
}

static void zep_shim_nbuf_headroom_res(void *nbuf, unsigned int size)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	nwb->data += size;
	nwb->tail += size;
	nwb->headroom += size;
}

static unsigned int zep_shim_nbuf_headroom_get(void *nbuf)
{
	return ((struct nwb *)nbuf)->headroom;
}

static unsigned int zep_shim_nbuf_data_size(void *nbuf)
{
	return ((struct nwb *)nbuf)->len;
}

static void *zep_shim_nbuf_data_get(void *nbuf)
{
	return ((struct nwb *)nbuf)->data;
}

static void *zep_shim_nbuf_data_put(void *nbuf, unsigned int size)
{
	struct nwb *nwb = (struct nwb *)nbuf;
	unsigned char *data = nwb->tail;

	nwb->tail += size;
	nwb->len += size;

	return data;
}

static void *zep_shim_nbuf_data_push(void *nbuf, unsigned int size)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	nwb->data -= size;
	nwb->headroom -= size;
	nwb->len += size;

	return nwb->data;
}

static void *zep_shim_nbuf_data_pull(void *nbuf, unsigned int size)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	nwb->data += size;
	nwb->headroom += size;
	nwb->len -= size;

	return nwb->data;
}

static unsigned char zep_shim_nbuf_get_priority(void *nbuf)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	return nwb->priority;
}

static unsigned char zep_shim_nbuf_get_chksum_done(void *nbuf)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	return nwb->chksum_done;
}

static void zep_shim_nbuf_set_chksum_done(void *nbuf, unsigned char chksum_done)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	nwb->chksum_done = (bool)chksum_done;
}

#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_core.h>

void *net_pkt_to_nbuf(struct net_pkt *pkt)
{
	struct nwb *nbuff;
	unsigned char *data;
	unsigned int len;

	len = net_pkt_get_len(pkt);

	nbuff = zep_shim_nbuf_alloc(len + 100);

	if (!nbuff) {
		return NULL;
	}

	zep_shim_nbuf_headroom_res(nbuff, 100);

	data = zep_shim_nbuf_data_put(nbuff, len);

	net_pkt_read(pkt, data, len);

	nbuff->priority = net_pkt_priority(pkt);
	nbuff->chksum_done = (bool)net_pkt_is_chksum_done(pkt);

	return nbuff;
}

void *net_pkt_from_nbuf(void *iface, void *frm)
{
	struct net_pkt *pkt = NULL;
	unsigned char *data;
	unsigned int len;
	struct nwb *nwb = frm;

	if (!nwb) {
		return NULL;
	}

	len = zep_shim_nbuf_data_size(nwb);

	data = zep_shim_nbuf_data_get(nwb);

	pkt = net_pkt_rx_alloc_with_buffer(iface, len, AF_UNSPEC, 0, K_MSEC(100));

	if (!pkt) {
		goto out;
	}

	if (net_pkt_write(pkt, data, len)) {
		net_pkt_unref(pkt);
		pkt = NULL;
		goto out;
	}

out:
	zep_shim_nbuf_free(nwb);
	return pkt;
}

#if defined(CONFIG_NRF70_RAW_DATA_RX) || defined(CONFIG_NRF70_PROMISC_DATA_RX)
void *net_raw_pkt_from_nbuf(void *iface, void *frm,
			    unsigned short raw_hdr_len,
			    void *raw_rx_hdr,
			    bool pkt_free)
{
	struct net_pkt *pkt = NULL;
	unsigned char *nwb_data;
	unsigned char *data =  NULL;
	unsigned int nwb_len;
	unsigned int total_len;
	struct nwb *nwb = frm;

	if (!nwb) {
		LOG_ERR("%s: Received network buffer is NULL", __func__);
		return NULL;
	}

	nwb_len = zep_shim_nbuf_data_size(nwb);
	nwb_data = zep_shim_nbuf_data_get(nwb);
	total_len = raw_hdr_len + nwb_len;

	data = (unsigned char *)k_malloc(total_len);
	if (!data) {
		LOG_ERR("%s: Unable to allocate memory for sniffer data packet", __func__);
		goto out;
	}

	pkt = net_pkt_rx_alloc_with_buffer(iface, total_len, AF_PACKET, ETH_P_ALL, K_MSEC(100));
	if (!pkt) {
		LOG_ERR("%s: Unable to allocate net packet buffer", __func__);
		goto out;
	}

	memcpy(data, raw_rx_hdr, raw_hdr_len);
	memcpy((data+raw_hdr_len), nwb_data, nwb_len);

	if (net_pkt_write(pkt, data, total_len)) {
		net_pkt_unref(pkt);
		pkt = NULL;
		goto out;
	}
out:
	if (data != NULL) {
		k_free(data);
	}

	if (pkt_free) {
		zep_shim_nbuf_free(nwb);
	}

	return pkt;
}
#endif /* CONFIG_NRF70_RAW_DATA_RX || CONFIG_NRF70_PROMISC_DATA_RX */

static void *zep_shim_llist_node_alloc(void)
{
	struct zep_shim_llist_node *llist_node = NULL;

	llist_node = k_calloc(sizeof(*llist_node), sizeof(char));

	if (!llist_node) {
		LOG_ERR("%s: Unable to allocate memory for linked list node", __func__);
		return NULL;
	}

	sys_dnode_init(&llist_node->head);

	return llist_node;
}

static void zep_shim_llist_node_free(void *llist_node)
{
	k_free(llist_node);
}

static void *zep_shim_llist_node_data_get(void *llist_node)
{
	struct zep_shim_llist_node *zep_llist_node = NULL;

	zep_llist_node = (struct zep_shim_llist_node *)llist_node;

	return zep_llist_node->data;
}

static void zep_shim_llist_node_data_set(void *llist_node, void *data)
{
	struct zep_shim_llist_node *zep_llist_node = NULL;

	zep_llist_node = (struct zep_shim_llist_node *)llist_node;

	zep_llist_node->data = data;
}

static void *zep_shim_llist_alloc(void)
{
	struct zep_shim_llist *llist = NULL;

	llist = k_calloc(sizeof(*llist), sizeof(char));

	if (!llist) {
		LOG_ERR("%s: Unable to allocate memory for linked list", __func__);
	}

	return llist;
}

static void zep_shim_llist_free(void *llist)
{
	k_free(llist);
}

static void zep_shim_llist_init(void *llist)
{
	struct zep_shim_llist *zep_llist = NULL;

	zep_llist = (struct zep_shim_llist *)llist;

	sys_dlist_init(&zep_llist->head);
}

static void zep_shim_llist_add_node_tail(void *llist, void *llist_node)
{
	struct zep_shim_llist *zep_llist = NULL;
	struct zep_shim_llist_node *zep_node = NULL;

	zep_llist = (struct zep_shim_llist *)llist;
	zep_node = (struct zep_shim_llist_node *)llist_node;

	sys_dlist_append(&zep_llist->head, &zep_node->head);

	zep_llist->len += 1;
}

static void zep_shim_llist_add_node_head(void *llist, void *llist_node)
{
	struct zep_shim_llist *zep_llist = NULL;
	struct zep_shim_llist_node *zep_node = NULL;

	zep_llist = (struct zep_shim_llist *)llist;
	zep_node = (struct zep_shim_llist_node *)llist_node;

	sys_dlist_prepend(&zep_llist->head, &zep_node->head);

	zep_llist->len += 1;
}

static void *zep_shim_llist_get_node_head(void *llist)
{
	struct zep_shim_llist_node *zep_head_node = NULL;
	struct zep_shim_llist *zep_llist = NULL;

	zep_llist = (struct zep_shim_llist *)llist;

	if (!zep_llist->len) {
		return NULL;
	}

	zep_head_node = (struct zep_shim_llist_node *)sys_dlist_peek_head(&zep_llist->head);

	return zep_head_node;
}

static void *zep_shim_llist_get_node_nxt(void *llist, void *llist_node)
{
	struct zep_shim_llist_node *zep_node = NULL;
	struct zep_shim_llist_node *zep_nxt_node = NULL;
	struct zep_shim_llist *zep_llist = NULL;

	zep_llist = (struct zep_shim_llist *)llist;
	zep_node = (struct zep_shim_llist_node *)llist_node;

	zep_nxt_node = (struct zep_shim_llist_node *)sys_dlist_peek_next(&zep_llist->head,
									 &zep_node->head);

	return zep_nxt_node;
}

static void zep_shim_llist_del_node(void *llist, void *llist_node)
{
	struct zep_shim_llist_node *zep_node = NULL;
	struct zep_shim_llist *zep_llist = NULL;

	zep_llist = (struct zep_shim_llist *)llist;
	zep_node = (struct zep_shim_llist_node *)llist_node;

	sys_dlist_remove(&zep_node->head);

	zep_llist->len -= 1;
}

static unsigned int zep_shim_llist_len(void *llist)
{
	struct zep_shim_llist *zep_llist = NULL;

	zep_llist = (struct zep_shim_llist *)llist;

	return zep_llist->len;
}

static void *zep_shim_work_alloc(int type)
{
	return work_alloc(type);
}

static void zep_shim_work_free(void *item)
{
	return work_free(item);
}

static void zep_shim_work_init(void *item, void (*callback)(unsigned long data),
				  unsigned long data)
{
	work_init(item, callback, data);
}

static void zep_shim_work_schedule(void *item)
{
	work_schedule(item);
}

static void zep_shim_work_kill(void *item)
{
	work_kill(item);
}

static unsigned long zep_shim_time_get_curr_us(void)
{
	return k_uptime_get() * USEC_PER_MSEC;
}

static unsigned int zep_shim_time_elapsed_us(unsigned long start_time_us)
{
	unsigned long curr_time_us = 0;

	curr_time_us = zep_shim_time_get_curr_us();

	return curr_time_us - start_time_us;
}

static enum nrf_wifi_status zep_shim_bus_qspi_dev_init(void *os_qspi_dev_ctx)
{
	ARG_UNUSED(os_qspi_dev_ctx);

	return NRF_WIFI_STATUS_SUCCESS;
}

static void zep_shim_bus_qspi_dev_deinit(void *priv)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = priv;
	volatile struct qspi_dev *dev = qspi_priv->qspi_dev;

	dev->deinit();
}

static void *zep_shim_bus_qspi_dev_add(void *os_qspi_priv, void *osal_qspi_dev_ctx)
{
	struct zep_shim_bus_qspi_priv *zep_qspi_priv = os_qspi_priv;
	struct qspi_dev *dev = qspi_dev();
	int ret;
	enum nrf_wifi_status status;

	ret = rpu_init();
	if (ret) {
		LOG_ERR("%s: RPU init failed with error %d", __func__, ret);
		return NULL;
	}

	status = dev->init(qspi_defconfig());
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: QSPI device init failed", __func__);
		return NULL;
	}

	ret = rpu_enable();
	if (ret) {
		LOG_ERR("%s: RPU enable failed with error %d", __func__, ret);
		return NULL;
	}
	zep_qspi_priv->qspi_dev = dev;
	zep_qspi_priv->dev_added = true;

	return zep_qspi_priv;
}

static void zep_shim_bus_qspi_dev_rem(void *priv)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = priv;
	struct qspi_dev *dev = qspi_priv->qspi_dev;

	ARG_UNUSED(dev);

	/* TODO: Make qspi_dev a dynamic instance and remove it here */
	rpu_disable();
}

static void *zep_shim_bus_qspi_init(void)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = NULL;

	qspi_priv = k_calloc(sizeof(*qspi_priv), sizeof(char));

	if (!qspi_priv) {
		LOG_ERR("%s: Unable to allocate memory for qspi_priv", __func__);
		goto out;
	}
out:
	return qspi_priv;
}

static void zep_shim_bus_qspi_deinit(void *os_qspi_priv)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = NULL;

	qspi_priv = os_qspi_priv;

	k_free(qspi_priv);
}

#ifdef CONFIG_NRF_WIFI_LOW_POWER
static int zep_shim_bus_qspi_ps_sleep(void *os_qspi_priv)
{
	rpu_sleep();

	return 0;
}

static int zep_shim_bus_qspi_ps_wake(void *os_qspi_priv)
{
	rpu_wakeup();

	return 0;
}

static int zep_shim_bus_qspi_ps_status(void *os_qspi_priv)
{
	return rpu_sleep_status();
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

static void zep_shim_bus_qspi_dev_host_map_get(void *os_qspi_dev_ctx,
					       struct nrf_wifi_osal_host_map *host_map)
{
	if (!os_qspi_dev_ctx || !host_map) {
		LOG_ERR("%s: Invalid parameters", __func__);
		return;
	}

	host_map->addr = 0;
}

static void irq_work_handler(struct k_work *work)
{
	int ret = 0;

	ret = intr_priv->callbk_fn(intr_priv->callbk_data);

	if (ret) {
		LOG_ERR("%s: Interrupt callback failed", __func__);
	}
}


extern struct k_work_q zep_wifi_intr_q;

static void zep_shim_irq_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	k_work_schedule_for_queue(&zep_wifi_intr_q, &intr_priv->work, K_NO_WAIT);
}

static enum nrf_wifi_status zep_shim_bus_qspi_intr_reg(void *os_dev_ctx, void *callbk_data,
						       int (*callbk_fn)(void *callbk_data))
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int ret = -1;

	ARG_UNUSED(os_dev_ctx);

	intr_priv = k_calloc(sizeof(*intr_priv), sizeof(char));

	if (!intr_priv) {
		LOG_ERR("%s: Unable to allocate memory for intr_priv", __func__);
		goto out;
	}

	intr_priv->callbk_data = callbk_data;
	intr_priv->callbk_fn = callbk_fn;

	k_work_init_delayable(&intr_priv->work, irq_work_handler);

	ret = rpu_irq_config(&intr_priv->gpio_cb_data, zep_shim_irq_handler);

	if (ret) {
		LOG_ERR("%s: request_irq failed", __func__);
		k_free(intr_priv);
		intr_priv = NULL;
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;

out:
	return status;
}

static void zep_shim_bus_qspi_intr_unreg(void *os_qspi_dev_ctx)
{
	struct k_work_sync sync;
	int ret;

	ARG_UNUSED(os_qspi_dev_ctx);

	ret = rpu_irq_remove(&intr_priv->gpio_cb_data);
	if (ret) {
		LOG_ERR("%s: rpu_irq_remove failed", __func__);
		return;
	}

	k_work_cancel_delayable_sync(&intr_priv->work, &sync);

	k_free(intr_priv);
	intr_priv = NULL;
}

#ifdef CONFIG_NRF_WIFI_LOW_POWER
static void *zep_shim_timer_alloc(void)
{
	struct timer_list *timer = NULL;

	timer = k_malloc(sizeof(*timer));

	if (!timer) {
		LOG_ERR("%s: Unable to allocate memory for work", __func__);
	}

	return timer;
}

static void zep_shim_timer_init(void *timer, void (*callback)(unsigned long), unsigned long data)
{
	((struct timer_list *)timer)->function = callback;
	((struct timer_list *)timer)->data = data;

	init_timer(timer);
}

static void zep_shim_timer_free(void *timer)
{
	k_free(timer);
}

static void zep_shim_timer_schedule(void *timer, unsigned long duration)
{
	mod_timer(timer, duration);
}

static void zep_shim_timer_kill(void *timer)
{
	del_timer_sync(timer);
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

static void zep_shim_assert(int test_val, int val, enum nrf_wifi_assert_op_type op, char *msg)
{
	switch (op) {
	case NRF_WIFI_ASSERT_EQUAL_TO:
		NET_ASSERT(test_val == val, "%s", msg);
	break;
	case NRF_WIFI_ASSERT_NOT_EQUAL_TO:
		NET_ASSERT(test_val != val, "%s", msg);
	break;
	case NRF_WIFI_ASSERT_LESS_THAN:
		NET_ASSERT(test_val < val, "%s", msg);
	break;
	case NRF_WIFI_ASSERT_LESS_THAN_EQUAL_TO:
		NET_ASSERT(test_val <= val, "%s", msg);
	break;
	case NRF_WIFI_ASSERT_GREATER_THAN:
		NET_ASSERT(test_val > val, "%s", msg);
	break;
	case NRF_WIFI_ASSERT_GREATER_THAN_EQUAL_TO:
		NET_ASSERT(test_val >= val, "%s", msg);
	break;
	default:
		LOG_ERR("%s: Invalid assertion operation", __func__);
	}
}

static unsigned int zep_shim_strlen(const void *str)
{
	return strlen(str);
}

const struct nrf_wifi_osal_ops nrf_wifi_os_zep_ops = {
	.mem_alloc = zep_shim_mem_alloc,
	.mem_zalloc = zep_shim_mem_zalloc,
	.mem_free = k_free,
	.mem_cpy = zep_shim_mem_cpy,
	.mem_set = zep_shim_mem_set,
	.mem_cmp = zep_shim_mem_cmp,

	.qspi_read_reg32 = zep_shim_qspi_read_reg32,
	.qspi_write_reg32 = zep_shim_qspi_write_reg32,
	.qspi_cpy_from = zep_shim_qspi_cpy_from,
	.qspi_cpy_to = zep_shim_qspi_cpy_to,

	.spinlock_alloc = zep_shim_spinlock_alloc,
	.spinlock_free = zep_shim_spinlock_free,
	.spinlock_init = zep_shim_spinlock_init,
	.spinlock_take = zep_shim_spinlock_take,
	.spinlock_rel = zep_shim_spinlock_rel,

	.spinlock_irq_take = zep_shim_spinlock_irq_take,
	.spinlock_irq_rel = zep_shim_spinlock_irq_rel,

	.log_dbg = zep_shim_pr_dbg,
	.log_info = zep_shim_pr_info,
	.log_err = zep_shim_pr_err,

	.llist_node_alloc = zep_shim_llist_node_alloc,
	.llist_node_free = zep_shim_llist_node_free,
	.llist_node_data_get = zep_shim_llist_node_data_get,
	.llist_node_data_set = zep_shim_llist_node_data_set,

	.llist_alloc = zep_shim_llist_alloc,
	.llist_free = zep_shim_llist_free,
	.llist_init = zep_shim_llist_init,
	.llist_add_node_tail = zep_shim_llist_add_node_tail,
	.llist_add_node_head = zep_shim_llist_add_node_head,
	.llist_get_node_head = zep_shim_llist_get_node_head,
	.llist_get_node_nxt = zep_shim_llist_get_node_nxt,
	.llist_del_node = zep_shim_llist_del_node,
	.llist_len = zep_shim_llist_len,

	.nbuf_alloc = zep_shim_nbuf_alloc,
	.nbuf_free = zep_shim_nbuf_free,
	.nbuf_headroom_res = zep_shim_nbuf_headroom_res,
	.nbuf_headroom_get = zep_shim_nbuf_headroom_get,
	.nbuf_data_size = zep_shim_nbuf_data_size,
	.nbuf_data_get = zep_shim_nbuf_data_get,
	.nbuf_data_put = zep_shim_nbuf_data_put,
	.nbuf_data_push = zep_shim_nbuf_data_push,
	.nbuf_data_pull = zep_shim_nbuf_data_pull,
	.nbuf_get_priority = zep_shim_nbuf_get_priority,
	.nbuf_get_chksum_done = zep_shim_nbuf_get_chksum_done,
	.nbuf_set_chksum_done = zep_shim_nbuf_set_chksum_done,

	.tasklet_alloc = zep_shim_work_alloc,
	.tasklet_free = zep_shim_work_free,
	.tasklet_init = zep_shim_work_init,
	.tasklet_schedule = zep_shim_work_schedule,
	.tasklet_kill = zep_shim_work_kill,

	.sleep_ms = k_msleep,
	.delay_us = k_usleep,
	.time_get_curr_us = zep_shim_time_get_curr_us,
	.time_elapsed_us = zep_shim_time_elapsed_us,

	.bus_qspi_init = zep_shim_bus_qspi_init,
	.bus_qspi_deinit = zep_shim_bus_qspi_deinit,
	.bus_qspi_dev_add = zep_shim_bus_qspi_dev_add,
	.bus_qspi_dev_rem = zep_shim_bus_qspi_dev_rem,
	.bus_qspi_dev_init = zep_shim_bus_qspi_dev_init,
	.bus_qspi_dev_deinit = zep_shim_bus_qspi_dev_deinit,
	.bus_qspi_dev_intr_reg = zep_shim_bus_qspi_intr_reg,
	.bus_qspi_dev_intr_unreg = zep_shim_bus_qspi_intr_unreg,
	.bus_qspi_dev_host_map_get = zep_shim_bus_qspi_dev_host_map_get,

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	.timer_alloc = zep_shim_timer_alloc,
	.timer_init = zep_shim_timer_init,
	.timer_free = zep_shim_timer_free,
	.timer_schedule = zep_shim_timer_schedule,
	.timer_kill = zep_shim_timer_kill,

	.bus_qspi_ps_sleep = zep_shim_bus_qspi_ps_sleep,
	.bus_qspi_ps_wake = zep_shim_bus_qspi_ps_wake,
	.bus_qspi_ps_status = zep_shim_bus_qspi_ps_status,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	.assert = zep_shim_assert,
	.strlen = zep_shim_strlen,
};
