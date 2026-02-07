/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT microchip_sam_udphs

#include "udc_common.h"

#include <zephyr/cache.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_mchp_sam, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define UDC_SAM_DMA(addr)	(USB_EP_GET_IDX(addr) - 1)
#define UDC_SAM_MAX_DMA_LEN \
	((UDPHS_DMACONTROL_BUFF_LENGTH_Msk >> UDPHS_DMACONTROL_BUFF_LENGTH_Pos) + 1)
#define UDC_SAM_BYTE_COUNT(x) \
	(((x) & UDPHS_EPTSTA_BYTE_COUNT_Msk) >> UDPHS_EPTSTA_BYTE_COUNT_Pos)
#define UDC_SAM_BUSY_BANKS(x) \
	(((x) & UDPHS_EPTSTA_BUSY_BANK_STA_Msk) >> UDPHS_EPTSTA_BUSY_BANK_STA_Pos)
#define UDC_SAM_BUFF_COUNT(x) \
	(((x) & UDPHS_DMASTATUS_BUFF_COUNT_Msk) >> UDPHS_DMASTATUS_BUFF_COUNT_Pos)

struct udc_sam_config {
	udphs_registers_t *base;
	uint8_t *fifo;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pincfg;
	const struct udphs_ep_desc *ep_desc;
	int speed_idx;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	struct gpio_dt_spec vbus_gpio;
	void (*irq_config_func)(const struct device *dev);
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	void (*make_thread)(const struct device *dev);
};

enum udc_thread_event_type {
	/* Setup packet received */
	UDC_THREAD_EVT_SETUP,
	/* Trigger new transfer (except control OUT) */
	UDC_THREAD_EVT_XFER_NEW,
	/* Transfer for specific endpoint is finished */
	UDC_THREAD_EVT_XFER_FINISHED,
};

struct udc_sam_data {
	const struct device *dev;
	struct k_thread thread_data;
	struct k_event events;
	atomic_t xfer_new;
	atomic_t xfer_running;
	atomic_t xfer_finished;
	atomic_t xfer_zero;

	struct gpio_callback vbus_cb;
	const struct gpio_dt_spec *vbus_gpio;
	uint8_t vbus_state;

	enum udc_bus_speed speed;
	uint8_t setup[sizeof(struct usb_setup_packet)];
};

struct udphs_ep_config {
	uint8_t type:2;
	uint8_t dir:1;
	uint8_t size:3;
	uint8_t banks_num:2;
	uint8_t trans_num:2;
	uint8_t dma:1;
};

struct udphs_request {
	uint8_t is_in:1;
	char *buf;
	uint32_t len;
};

struct udphs_ep_desc {
	uint8_t nr_banks:2;
	uint8_t can_dma:1;
	uint8_t high_bw:1;
	uint8_t ep_size:4;
#define SZ_64   6  /* 1 << 6  */
#define SZ_512  9  /* 1 << 9  */
#define SZ_1024 10 /* 1 << 10 */
};

static const struct udphs_ep_desc sam_ep_desc[UDPHS_EPT_NUMBER] = {
	{ .nr_banks = 1, .ep_size = SZ_64 },                               /* ep 0 */
	{ .nr_banks = 3, .ep_size = SZ_1024, .can_dma = 0, .high_bw = 1 }, /* ep 1 */
	{ .nr_banks = 3, .ep_size = SZ_1024, .can_dma = 0, .high_bw = 1 }, /* ep 2 */
	{ .nr_banks = 2, .ep_size = SZ_1024, .can_dma = 0 },               /* ep 3 */
	{ .nr_banks = 2, .ep_size = SZ_512,  .can_dma = 0 },               /* ep 4 */
	{ .nr_banks = 2, .ep_size = SZ_512,  .can_dma = 0 },               /* ep 5 */
	{ .nr_banks = 2, .ep_size = SZ_512,  .can_dma = 0 },               /* ep 6 */
	{ .nr_banks = 2, .ep_size = SZ_512,  .can_dma = 0 },               /* ep 7 */
	{ .nr_banks = 1, .ep_size = SZ_512 },                              /* ep 8 */
	{ .nr_banks = 1, .ep_size = SZ_512 },                              /* ep 9 */
	{ .nr_banks = 1, .ep_size = SZ_512 },                              /* ep 10 */
	{ .nr_banks = 1, .ep_size = SZ_512 },                              /* ep 11 */
	{ .nr_banks = 1, .ep_size = SZ_512 },                              /* ep 12 */
	{ .nr_banks = 1, .ep_size = SZ_512 },                              /* ep 13 */
	{ .nr_banks = 1, .ep_size = SZ_512 },                              /* ep 14 */
	{ .nr_banks = 1, .ep_size = SZ_512 },                              /* ep 15 */
};


static inline int udc_ep_to_bnum(const uint8_t ep);

static inline udphs_registers_t *base_reg(const struct device *const dev)
{
	const struct udc_sam_config *config = dev->config;

	return config->base;
}

static inline udphs_ept_registers_t *ep_reg(const struct device *const dev, uint8_t idx)
{
	const struct udc_sam_config *config = dev->config;
	udphs_registers_t *const udphs = config->base;

	return &udphs->UDPHS_EPT[idx];
}

static inline udphs_dma_registers_t *dma_reg(const struct device *const dev, const uint8_t dma)
{
	const struct udc_sam_config *config = dev->config;
	udphs_registers_t *const udphs = config->base;

	return &udphs->UDPHS_DMA[dma];
}

static inline uint8_t *fifo_addr(const struct device *const dev, uint8_t idx)
{
	const struct udc_sam_config *config = dev->config;

	return config->fifo + (idx << 16);
}

static inline int ep_banks(const struct device *const dev, uint8_t idx)
{
	const struct udc_sam_config *config = dev->config;
	const struct udphs_ep_desc *ep_desc = &config->ep_desc[idx];

	return ep_desc->nr_banks;
}

static inline int ep_size(const struct device *const dev, uint8_t idx)
{
	const struct udc_sam_config *config = dev->config;
	const struct udphs_ep_desc *ep_desc = &config->ep_desc[idx];

	return BIT(ep_desc->ep_size);
}

static inline bool ep_can_dma(const struct device *const dev, uint8_t idx)
{
	const struct udc_sam_config *config = dev->config;
	const struct udphs_ep_desc *ep_desc = &config->ep_desc[idx];

	return ep_desc->can_dma;
}

static void udphs_reset(udphs_registers_t *const udphs)
{
	udphs_ept_registers_t *ept;
	udphs_dma_registers_t *dma;

	udphs->UDPHS_CTRL = UDPHS_CTRL_DETACH_Msk;
	udphs->UDPHS_CTRL |= UDPHS_CTRL_EN_UDPHS_Msk;
	udphs->UDPHS_IEN = UDPHS_IEN_ENDRESET_Msk;
	udphs->UDPHS_CLRINT = UDPHS_CLRINT_Msk;
	udphs->UDPHS_EPTRST = UDPHS_EPTRST_EPT__Msk;

	for (int i = 0; i < UDPHS_DMA_NUMBER; i++) {
		ept = &udphs->UDPHS_EPT[i + 1];
		dma = &udphs->UDPHS_DMA[i];

		dma->UDPHS_DMACONTROL = 0;
		ept->UDPHS_EPTCTLDIS = UDPHS_EPTCTLDIS_Msk;
		ept->UDPHS_EPTCLRSTA = UDPHS_EPTCLRSTA_Msk;
		ept->UDPHS_EPTCFG = 0;
		dma->UDPHS_DMACONTROL = UDPHS_DMACONTROL_LDNXT_DSC_Msk;
		dma->UDPHS_DMACONTROL = 0;
		dma->UDPHS_DMASTATUS = dma->UDPHS_DMASTATUS;
	}

	udphs->UDPHS_CTRL = UDPHS_CTRL_DETACH_Msk;
}

static void udphs_start(udphs_registers_t *const udphs)
{
	udphs->UDPHS_CTRL = UDPHS_CTRL_PULLD_DIS_Msk |
			    UDPHS_CTRL_DETACH_Msk |
			    UDPHS_CTRL_EN_UDPHS_Msk;
	udphs->UDPHS_IEN = UDPHS_IEN_ENDRESET_Msk;
	udphs->UDPHS_CLRINT = UDPHS_CLRINT_ENDOFRSM_Msk |
			      UDPHS_CLRINT_WAKE_UP_Msk |
			      UDPHS_CLRINT_ENDRESET_Msk |
			      UDPHS_CLRINT_DET_SUSPD_Msk;
}

static void udphs_stop(udphs_registers_t *const udphs)
{
	udphs->UDPHS_CTRL = UDPHS_CTRL_DETACH_Msk |
			    UDPHS_CTRL_EN_UDPHS_Msk;
}

static void udphs_pullup_en(udphs_registers_t *const udphs)
{
	udphs->UDPHS_CTRL |= UDPHS_CTRL_PULLD_DIS_Msk;
	udphs->UDPHS_CTRL &= ~UDPHS_CTRL_DETACH_Msk;
}

#define SPEED_NORMAL		0
#define SPEED_FORCE_HIGH	2
#define SPEED_FORCE_FULL	3
static void udphs_speed_mode(udphs_registers_t *const udphs, uint32_t mode)
{
	udphs->UDPHS_TST &= ~UDPHS_TST_SPEED_CFG_Msk;
	udphs->UDPHS_TST |= UDPHS_TST_SPEED_CFG(mode);
}

static void udphs_set_address(udphs_registers_t *const udphs, unsigned char addr)
{
	udphs->UDPHS_CTRL &= ~UDPHS_CTRL_DEV_ADDR_Msk;
	if (addr != 0) {
		udphs->UDPHS_CTRL |= UDPHS_CTRL_FADDR_EN_Msk |
				     UDPHS_CTRL_DEV_ADDR(addr);
	}
}

static void udphs_send_wakeup(udphs_registers_t *const udphs)
{
	udphs->UDPHS_CTRL |= UDPHS_CTRL_REWAKEUP_Msk;
}

static void udphs_reset_ep(udphs_registers_t *const udphs, uint8_t idx)
{
	udphs_ept_registers_t *const ept = &udphs->UDPHS_EPT[idx];

	ept->UDPHS_EPTCTLDIS = UDPHS_EPTCTLDIS_Msk;
	ept->UDPHS_EPTCFG = 0;
	ept->UDPHS_EPTCLRSTA = UDPHS_EPTCLRSTA_TOGGLESQ_Msk |
			       UDPHS_EPTCLRSTA_FRCESTALL_Msk;
	udphs->UDPHS_EPTRST = UDPHS_EPTRST_EPT_0_Msk << idx;
	udphs->UDPHS_IEN &= ~(UDPHS_IEN_EPT_0_Msk << idx);
}

static void udphs_reset_ep_all(udphs_registers_t *const udphs)
{
	for (int i = 0; i < UDPHS_EPT_NUMBER; i++) {
		udphs_reset_ep(udphs, i);
	}
}

static void udphs_clear_ep_status(udphs_registers_t *const udphs, uint8_t idx)
{
	udphs->UDPHS_EPTRST = UDPHS_EPTRST_EPT_0_Msk << idx;
}

static int udphs_enable_ep(udphs_registers_t *const udphs, uint8_t idx,
			   struct udphs_ep_config *cfg)
{
	udphs_ept_registers_t *const ept =  &udphs->UDPHS_EPT[idx];

	ept->UDPHS_EPTCFG = UDPHS_EPTCFG_NB_TRANS(cfg->trans_num) |
			    UDPHS_EPTCFG_BK_NUMBER(cfg->banks_num) |
			    UDPHS_EPTCFG_EPT_TYPE(cfg->type) |
			    UDPHS_EPTCFG_EPT_DIR(cfg->dir) |
			    UDPHS_EPTCFG_EPT_SIZE(cfg->size);

	if ((ept->UDPHS_EPTCFG & UDPHS_EPTCFG_EPT_MAPD_Msk) == 0) {
		return -1;
	}

	if (idx == 0) {
		ept->UDPHS_EPTCTLENB = UDPHS_EPTCTLENB_RX_SETUP_Msk;
	}
	if (cfg->dma) {
		ept->UDPHS_EPTCTLENB = UDPHS_EPTCTLENB_INTDIS_DMA_Msk |
				       UDPHS_EPTCTLENB_AUTO_VALID_Msk;
	}
	ept->UDPHS_EPTCTLENB = UDPHS_EPTCTLENB_EPT_ENABL_Msk;

	if (cfg->dma) {
		udphs->UDPHS_IEN |= UDPHS_IEN_DMA_1_Msk << (idx - 1);
	}
	udphs->UDPHS_IEN |= UDPHS_IEN_EPT_0_Msk << idx;

	return 0;
}

static void udphs_disable_ep(udphs_registers_t *const udphs, uint8_t idx)
{
	udphs_ept_registers_t *const ept = &udphs->UDPHS_EPT[idx];

	ept->UDPHS_EPTCTLDIS = UDPHS_EPTCTLDIS_EPT_DISABL_Msk;
	udphs->UDPHS_IEN &= ~(UDPHS_IEN_EPT_0_Msk << idx);
}

static void udphs_rx_setup(udphs_registers_t *const udphs)
{
	udphs_ept_registers_t *const ept =  &udphs->UDPHS_EPT[0];

	ept->UDPHS_EPTCTLENB = UDPHS_EPTCTLENB_RX_SETUP_Msk;
}

static void udphs_ep_fifo_in(udphs_ept_registers_t *const ept, bool enable)
{
	if (enable) {
		ept->UDPHS_EPTCTLENB = UDPHS_EPTCTLENB_TXRDY_Msk;
	} else {
		ept->UDPHS_EPTCTLDIS = UDPHS_EPTCTLDIS_TXRDY_Msk;
	}
}

static void udphs_ep_fifo_out(udphs_ept_registers_t *const ept, bool enable)
{
	if (enable) {
		ept->UDPHS_EPTCTLENB = UDPHS_EPTCTLENB_RXRDY_TXKL_Msk;
	} else {
		ept->UDPHS_EPTCTLDIS = UDPHS_EPTCTLDIS_RXRDY_TXKL_Msk;
	}
}

static int udphs_ep_dma_start(udphs_ept_registers_t *const ept, udphs_dma_registers_t *const dma,
			      struct udphs_request *req)
{
	uint32_t ctrl;

	if (req->len > UDC_SAM_MAX_DMA_LEN) {
		return -1;
	}

	if (req->len == 0) {
		if (req->is_in) {
			ept->UDPHS_EPTCTLENB = UDPHS_EPTCTLENB_TXRDY_Msk;
		}

		return 0;
	}

	ctrl = UDPHS_DMACONTROL_BUFF_LENGTH(req->len) |
	       UDPHS_DMACONTROL_END_BUFFIT_Msk |
	       UDPHS_DMACONTROL_END_B_EN_Msk |
	       UDPHS_DMACONTROL_CHANN_ENB_Msk;
	if (!req->is_in) {
		ctrl |= UDPHS_DMACONTROL_END_TR_IT_Msk |
			UDPHS_DMACONTROL_END_TR_EN_Msk;
	}
#ifdef CONFIG_UDC_MCHP_SAM_BURST_LOCK
	ctrl |= UDPHS_DMACONTROL_BURST_LCK_Msk;
#endif
	(void)dma->UDPHS_DMASTATUS;
	dma->UDPHS_DMAADDRESS = (uint32_t)req->buf;
	dma->UDPHS_DMACONTROL = ctrl;

	return 0;
}

static int udphs_ep_dma_stop(udphs_dma_registers_t *const dma)
{
	uint32_t timeout = 100;

	/* Stop it if the DMA channel is running */
	if (dma->UDPHS_DMASTATUS & UDPHS_DMASTATUS_CHANN_ENB_Msk) {
		dma->UDPHS_DMACONTROL = 0;

		while ((dma->UDPHS_DMASTATUS & UDPHS_DMASTATUS_CHANN_ENB_Msk) && timeout--) {
			k_usleep(1);
		}
	}

	dma->UDPHS_DMACONTROL = 0;
	dma->UDPHS_DMAADDRESS = 0;
	if (dma->UDPHS_DMASTATUS & UDPHS_DMASTATUS_CHANN_ENB_Msk) {
		return -1;
	}

	return 0;
}

static void udphs_ep_set_halt(udphs_ept_registers_t *const ept)
{
	ept->UDPHS_EPTSETSTA = UDPHS_EPTSETSTA_FRCESTALL_Msk;
}

static void udphs_ep_clear_halt(udphs_ept_registers_t *const ept, uint8_t idx)
{
	if (idx == 0) {
		ept->UDPHS_EPTCLRSTA = UDPHS_EPTCLRSTA_FRCESTALL_Msk;
	} else {
		ept->UDPHS_EPTCLRSTA = UDPHS_EPTCLRSTA_TOGGLESQ_Msk |
				       UDPHS_EPTCLRSTA_FRCESTALL_Msk;
	}
}

static void sam_rx_setup_pkt(const struct device *dev)
{
	const struct udc_sam_config *config = dev->config;
	udphs_registers_t *const udphs = config->base;

	udphs_rx_setup(udphs);
}

static int sam_prep_out(const struct device *dev,
			struct net_buf *const buf, struct udc_ep_config *const ep_cfg)
{
	struct udc_sam_data *const priv = udc_get_private(dev);
	uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	udphs_ept_registers_t *const ept = ep_reg(dev, idx);
	unsigned int lock_key;

	LOG_DBG("Prep OUT ep%02x %u %s", ep_cfg->addr, buf->size,
					 ep_can_dma(dev, idx) ? "dma" : "fifo");

	if (ep_can_dma(dev, idx)) {
		udphs_dma_registers_t *const dma = dma_reg(dev, UDC_SAM_DMA(ep_cfg->addr));
		struct udphs_request req = {0};

		sys_cache_data_invd_range(buf->data, buf->size);
		req.buf = buf->data;
		req.len = MIN(buf->size, UDC_SAM_MAX_DMA_LEN);

		lock_key = irq_lock();
		udphs_ep_dma_start(ept, dma, &req);
	} else {
		lock_key = irq_lock();
		udphs_ep_fifo_out(ept, 1);
	}

	atomic_set_bit(&priv->xfer_running, udc_ep_to_bnum(idx | USB_EP_DIR_OUT));

	irq_unlock(lock_key);

	return 0;
}

static int sam_prep_in(const struct device *dev,
		       struct net_buf *const buf, struct udc_ep_config *const ep_cfg)
{
	struct udc_sam_data *const priv = udc_get_private(dev);
	uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	udphs_ept_registers_t *const ept = ep_reg(dev, idx);
	unsigned int lock_key;

	LOG_DBG("Prep IN ep%02x %u %s %s", ep_cfg->addr, buf->len,
					   ep_can_dma(dev, idx) ? "dma" : "fifo",
					   udc_ep_buf_has_zlp(buf) ? "zlp" : "");

	if (ep_can_dma(dev, idx)) {
		udphs_dma_registers_t *const dma = dma_reg(dev, UDC_SAM_DMA(ep_cfg->addr));
		struct udphs_request req = {0};

		sys_cache_data_flush_range(buf->data, buf->len);

		req.is_in = 1;
		req.buf = buf->data;
		req.len = MIN(buf->len, UDC_SAM_MAX_DMA_LEN);

		lock_key = irq_lock();
		udphs_ep_dma_start(ept, dma, &req);
	} else {
		lock_key = irq_lock();
		udphs_ep_fifo_in(ept, 1);
	}

	if (buf->len == 0) {
		atomic_set_bit(&priv->xfer_zero, udc_ep_to_bnum(idx | USB_EP_DIR_IN));
	}
	atomic_set_bit(&priv->xfer_running, udc_ep_to_bnum(idx | USB_EP_DIR_IN));

	irq_unlock(lock_key);

	return 0;
}

static void udc_ep0_internal(const struct device *dev, const bool enable)
{
	struct udc_ep_config *const out_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct udc_ep_config *const in_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);

	if ((out_cfg->stat.enabled) &&
	    (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT))) {
		LOG_ERR("Failed to disable control endpoint");
	}
	if ((in_cfg->stat.enabled) &&
	    (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN))) {
		LOG_ERR("Failed to disable control endpoint");
	}

	if (enable) {
		if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT,
					   USB_EP_TYPE_CONTROL,
					   ep_size(dev, 0), 0)) {
			LOG_ERR("Failed to enable control endpoint");
		}

		if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN,
					   USB_EP_TYPE_CONTROL,
					   ep_size(dev, 0), 0)) {
			LOG_ERR("Failed to enable control endpoint");
		}
	}
}

static inline int udc_ep_to_bnum(const uint8_t ep)
{
	if (USB_EP_DIR_IS_IN(ep)) {
		return 16UL + USB_EP_GET_IDX(ep);
	}

	return USB_EP_GET_IDX(ep);
}

static inline uint8_t udc_pull_ep_from_bmsk(uint32_t *const bitmap)
{
	unsigned int bit;

	__ASSERT_NO_MSG(bitmap && *bitmap);

	bit = find_lsb_set(*bitmap) - 1;
	*bitmap &= ~BIT(bit);

	if (bit >= 16U) {
		return USB_EP_DIR_IN | (bit - 16U);
	} else {
		return USB_EP_DIR_OUT | bit;
	}
}

static int udc_ctrl_feed_dout(const struct device *dev, const size_t length)
{
	struct udc_ep_config *const ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	udc_buf_put(ep_cfg, buf);

	return sam_prep_out(dev, buf, ep_cfg);
}

static void udc_drop_control_transfers(const struct device *dev)
{
	struct net_buf *buf;

	buf = udc_buf_get_all(udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT));
	if (buf != NULL) {
		net_buf_unref(buf);
	}

	buf = udc_buf_get_all(udc_get_ep_cfg(dev, USB_CONTROL_EP_IN));
	if (buf != NULL) {
		net_buf_unref(buf);
	}
}

static int udc_handle_evt_setup(const struct device *dev, char *setup)
{
	struct net_buf *buf;
	int ret;

	udc_drop_control_transfers(dev);

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, sizeof(struct usb_setup_packet));
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, setup, sizeof(struct usb_setup_packet));
	udc_ep_buf_set_setup(buf);

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		/*  Allocate and feed buffer for data OUT stage */
		ret = udc_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (ret == -ENOMEM) {
			udc_submit_ep_event(dev, buf, ret);
		} else {
			return ret;
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		ret = udc_ctrl_submit_s_in_status(dev);
	} else {
		ret = udc_ctrl_submit_s_status(dev);
	}

	return ret;
}

static int udc_handle_evt_din(const struct device *dev,
			      struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf;

	buf = udc_buf_get(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for ep %02x", ep_cfg->addr);
		return -ENOBUFS;
	}

	udc_ep_set_busy(ep_cfg, false);

	if (ep_cfg->addr == USB_CONTROL_EP_IN) {
		if (udc_ctrl_stage_is_status_in(dev) ||
		    udc_ctrl_stage_is_no_data(dev)) {
			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);
		}

		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_out(dev)) {
			int ret;

			/* IN transfer finished, submit buffer for status stage */
			net_buf_unref(buf);

			ret = udc_ctrl_feed_dout(dev, 0);
			if (ret == -ENOMEM) {
				udc_submit_ep_event(dev, buf, ret);
			} else {
				return ret;
			}
		}

		if ((udc_ctrl_stage_is_setup(dev)) && (buf->data)) {
			net_buf_unref(buf);
		}

		return 0;
	}

	return udc_submit_ep_event(dev, buf, 0);
}

static inline int udc_handle_evt_dout(const struct device *dev,
				      struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf;
	int ret = 0;

	buf = udc_buf_get(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for OUT ep %02x", ep_cfg->addr);
		return -ENODATA;
	}

	udc_ep_set_busy(ep_cfg, false);

	if (ep_cfg->addr == USB_CONTROL_EP_OUT) {
		if (udc_ctrl_stage_is_status_out(dev)) {
			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);
		}

		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_in(dev)) {
			ret = udc_ctrl_submit_s_out_status(dev, buf);
		}
	} else {
		ret = udc_submit_ep_event(dev, buf, 0);
	}

	return ret;
}

static void udc_handle_xfer_next(const struct device *dev,
				 struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf;
	int ret;

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		return;
	}

	if (USB_EP_DIR_IS_OUT(ep_cfg->addr)) {
		ret = sam_prep_out(dev, buf, ep_cfg);
	} else {
		ret = sam_prep_in(dev, buf, ep_cfg);
	}

	if (ret != 0) {
		buf = udc_buf_get(ep_cfg);
		udc_submit_ep_event(dev, buf, -ECONNREFUSED);
	} else {
		udc_ep_set_busy(ep_cfg, true);
	}
}

static ALWAYS_INLINE void udc_thread_handler(const struct device *const dev)
{
	struct udc_sam_data *const priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	uint32_t evt;
	uint32_t eps;
	uint8_t ep;
	int ret;

	evt = k_event_wait(&priv->events, UINT32_MAX, false, K_FOREVER);
	udc_lock_internal(dev, K_FOREVER);

	if (evt & BIT(UDC_THREAD_EVT_XFER_FINISHED)) {
		k_event_clear(&priv->events, BIT(UDC_THREAD_EVT_XFER_FINISHED));

		eps = atomic_clear(&priv->xfer_finished);

		while (eps) {
			ep = udc_pull_ep_from_bmsk(&eps);
			ep_cfg = udc_get_ep_cfg(dev, ep);

			if (USB_EP_DIR_IS_IN(ep)) {
				ret = udc_handle_evt_din(dev, ep_cfg);
			} else {
				ret = udc_handle_evt_dout(dev, ep_cfg);
			}

			if (ret) {
				udc_submit_event(dev, UDC_EVT_ERROR, ret);
			}

			if (!udc_ep_is_busy(ep_cfg)) {
				udc_handle_xfer_next(dev, ep_cfg);
			} else {
				LOG_ERR("Endpoint %02x busy", ep);
			}
		}
	}

	if (evt & BIT(UDC_THREAD_EVT_XFER_NEW)) {
		k_event_clear(&priv->events, BIT(UDC_THREAD_EVT_XFER_NEW));

		eps = atomic_clear(&priv->xfer_new);

		while (eps) {
			ep = udc_pull_ep_from_bmsk(&eps);
			ep_cfg = udc_get_ep_cfg(dev, ep);

			if (ep_cfg->stat.halted) {
				continue;
			}

			if (!udc_ep_is_busy(ep_cfg)) {
				udc_handle_xfer_next(dev, ep_cfg);
			} else {
				LOG_ERR("Endpoint %02x busy", ep);
			}
		}
	}

	if (evt & BIT(UDC_THREAD_EVT_SETUP)) {
		k_event_clear(&priv->events, BIT(UDC_THREAD_EVT_SETUP));

		ret = udc_handle_evt_setup(dev, priv->setup);
		if (ret) {
			udc_submit_event(dev, UDC_EVT_ERROR, ret);
		}
	}

	if (udc_ctrl_stage_is_setup(dev)) {
		/* Start receiving the next setup packet */
		sam_rx_setup_pkt(dev);
	}

	udc_unlock_internal(dev);
}

static int ALWAYS_INLINE handle_dma_irq(const struct device *dev, const uint8_t chan)
{
	struct udc_sam_data *const priv = udc_get_private(dev);
	udphs_dma_registers_t *const dma = dma_reg(dev, chan);
	uint8_t idx = chan + 1;
	udphs_ept_registers_t *const ept = ep_reg(dev, idx);
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	uint32_t status, ctrl;
	int size;

	status = dma->UDPHS_DMASTATUS;
	ctrl = dma->UDPHS_DMACONTROL;

	LOG_DBG(" isr dma%d, s/%08x c/%08x", chan + 1, status, ctrl);

	if (status & UDPHS_DMASTATUS_CHANN_ENB_Msk) {
		/*
		 * The END_TR_ST interrupt comes from the last transfmission,
		 * just ignore it.
		 */
		return 0;
	}

	if ((status & ctrl) & (UDPHS_DMASTATUS_END_BF_ST_Msk | UDPHS_DMASTATUS_END_TR_ST_Msk)) {
		if (ctrl & UDPHS_DMACONTROL_END_TR_EN_Msk) { /* Is OUT */
			if (!atomic_test_bit(&priv->xfer_running,
					     udc_ep_to_bnum(idx | USB_EP_DIR_OUT))) {
				return 0;
			}

			ep_cfg = udc_get_ep_cfg(dev, idx | USB_EP_DIR_OUT);

			buf = udc_buf_peek(ep_cfg);
			if (buf == NULL) {
				LOG_ERR("No buffer for ep%02x", idx | USB_EP_DIR_OUT);
				udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
				return -1;
			}

			size = MIN(net_buf_tailroom(buf), UDC_SAM_MAX_DMA_LEN);
			net_buf_add(buf, size - UDC_SAM_BUFF_COUNT(status));
			size = MIN(net_buf_tailroom(buf), UDC_SAM_MAX_DMA_LEN);

			if ((status & UDPHS_DMASTATUS_END_TR_ST_Msk) || (size == 0)) {
				sys_cache_data_invd_range(buf->data, buf->len);

				atomic_clear_bit(&priv->xfer_running,
					       udc_ep_to_bnum(idx | USB_EP_DIR_OUT));
				atomic_set_bit(&priv->xfer_finished,
					       udc_ep_to_bnum(idx | USB_EP_DIR_OUT));
				k_event_post(&priv->events, BIT(UDC_THREAD_EVT_XFER_FINISHED));
				return 0;
			}

			dma->UDPHS_DMAADDRESS = (uint32_t)buf->data + buf->len;
			dma->UDPHS_DMACONTROL = UDPHS_DMACONTROL_BUFF_LENGTH(size) |
						UDPHS_DMACONTROL_END_BUFFIT_Msk |
						UDPHS_DMACONTROL_END_TR_IT_Msk |
						UDPHS_DMACONTROL_END_B_EN_Msk |
						UDPHS_DMACONTROL_END_TR_EN_Msk |
						UDPHS_DMACONTROL_CHANN_ENB_Msk;
		} else { /* Is IN */
			if (!atomic_test_bit(&priv->xfer_running,
					     udc_ep_to_bnum(idx | USB_EP_DIR_IN))) {
				return 0;
			}

			ep_cfg = udc_get_ep_cfg(dev, idx | USB_EP_DIR_IN);

			buf = udc_buf_peek(ep_cfg);
			if (buf == NULL) {
				LOG_ERR("No buffer for ep%02x", idx | USB_EP_DIR_IN);
				udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
				return -1;
			}

			size = MIN(buf->len, UDC_SAM_MAX_DMA_LEN);
			net_buf_pull(buf, size);

			if (buf->len == 0) {
				if (udc_ep_buf_has_zlp(buf)) {
					ept->UDPHS_EPTCTLENB = UDPHS_EPTCTLENB_TXRDY_Msk;
					return 1;
				}

				atomic_clear_bit(&priv->xfer_running,
					       udc_ep_to_bnum(idx | USB_EP_DIR_IN));
				atomic_set_bit(&priv->xfer_finished,
					       udc_ep_to_bnum(idx | USB_EP_DIR_IN));
				k_event_post(&priv->events,
					     BIT(UDC_THREAD_EVT_XFER_FINISHED));
				return 0;
			}

			size = MIN(buf->len, UDC_SAM_MAX_DMA_LEN);
			dma->UDPHS_DMAADDRESS = (uint32_t)buf->data;
			dma->UDPHS_DMACONTROL = UDPHS_DMACONTROL_BUFF_LENGTH(size) |
						UDPHS_DMACONTROL_END_BUFFIT_Msk |
						UDPHS_DMACONTROL_END_B_EN_Msk |
						UDPHS_DMACONTROL_CHANN_ENB_Msk;
		}
	}

	return 0;
}

static void ALWAYS_INLINE handle_ep_irq(const struct device *dev, uint8_t idx)
{
	struct udc_sam_data *const priv = udc_get_private(dev);
	udphs_ept_registers_t *const ept = ep_reg(dev, idx);
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	uint32_t status, ctrl;
	int size;

	status = ept->UDPHS_EPTSTA;
	ctrl = ept->UDPHS_EPTCTL;

	LOG_DBG(" isr ep%d, s/%08x c/%08x", idx, status, ctrl);

	/* Is IN */
	while ((ctrl & UDPHS_EPTCTL_TXRDY_Msk) && !(status & UDPHS_EPTSTA_TXRDY_Msk)) {
		ep_cfg = udc_get_ep_cfg(dev, idx | USB_EP_DIR_IN);

		buf = udc_buf_peek(ep_cfg);
		if (buf == NULL) {
			LOG_ERR("No buffer for ep%02x", idx | USB_EP_DIR_IN);
			udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
			return;
		}

		if (buf->len == 0) {
			if (atomic_test_bit(&priv->xfer_zero,
					    udc_ep_to_bnum(idx | USB_EP_DIR_IN))) {
				LOG_DBG(" ep%02x send zero", idx | USB_EP_DIR_IN);
				ept->UDPHS_EPTSETSTA = UDPHS_EPTSETSTA_TXRDY_Msk;
				atomic_clear_bit(&priv->xfer_zero,
						 udc_ep_to_bnum(idx | USB_EP_DIR_IN));
			} else if (udc_ep_buf_has_zlp(buf)) {
				LOG_DBG(" ep%02x send zlp", idx | USB_EP_DIR_IN);
				ept->UDPHS_EPTSETSTA = UDPHS_EPTSETSTA_TXRDY_Msk;
				udc_ep_buf_clear_zlp(buf);
			} else {
				ept->UDPHS_EPTCTLDIS = UDPHS_EPTCTLDIS_TXRDY_Msk;
				ept->UDPHS_EPTCLRSTA = UDPHS_EPTCLRSTA_TX_COMPLT_Msk;

				atomic_set_bit(&priv->xfer_finished,
					       udc_ep_to_bnum(idx | USB_EP_DIR_IN));
				k_event_post(&priv->events, BIT(UDC_THREAD_EVT_XFER_FINISHED));
				break;
			}
		} else {
			size = MIN(buf->len, udc_mps_ep_size(ep_cfg));

			memcpy(fifo_addr(dev, idx), buf->data, size);
			ept->UDPHS_EPTSETSTA = UDPHS_EPTSETSTA_TXRDY_Msk;
			net_buf_pull(buf, size);
		}

		status = ept->UDPHS_EPTSTA;
	}

	/* Is OUT */
	if ((status & ctrl) & UDPHS_EPTSTA_RXRDY_TXKL_Msk) {
		ep_cfg = udc_get_ep_cfg(dev, idx | USB_EP_DIR_OUT);

		buf = udc_buf_peek(ep_cfg);
		if (buf == NULL) {
			LOG_ERR("No buffer for ep%02x", idx | USB_EP_DIR_OUT);
			udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
			return;
		}

		while (UDC_SAM_BUSY_BANKS(status) > 0) {
			size = MIN(net_buf_tailroom(buf), UDC_SAM_BYTE_COUNT(status));

			if (size > 0) {
				net_buf_add_mem(buf, fifo_addr(dev, idx), size);
				size = net_buf_tailroom(buf);
			}

			ept->UDPHS_EPTCLRSTA = UDPHS_EPTCLRSTA_RXRDY_TXKL_Msk;
			if ((status & UDPHS_EPTSTA_SHRT_PCKT_Msk) || (size == 0)) {
				ept->UDPHS_EPTCTLDIS = UDPHS_EPTCTLDIS_RXRDY_TXKL_Msk;

				atomic_set_bit(&priv->xfer_finished,
					       udc_ep_to_bnum(idx | USB_EP_DIR_OUT));
				k_event_post(&priv->events, BIT(UDC_THREAD_EVT_XFER_FINISHED));
				break;
			}

			status = ept->UDPHS_EPTSTA;
		}
	}

	/* Is SETUP */
	if ((idx == 0) && ((status & ctrl) & UDPHS_EPTSTA_RX_SETUP_Msk)) {
		if (UDC_SAM_BYTE_COUNT(status) != sizeof(struct usb_setup_packet)) {
			LOG_ERR("Wrong byte count %u for setup packet",
				UDC_SAM_BYTE_COUNT(status));
			return;
		}
		/* Stop receiving the next setup packet until the complete of the stages */
		ept->UDPHS_EPTCTLDIS = UDPHS_EPTCTLDIS_RX_SETUP_Msk;

		memcpy(priv->setup, fifo_addr(dev, idx), sizeof(struct usb_setup_packet));
		ept->UDPHS_EPTCLRSTA = UDPHS_EPTCLRSTA_RX_SETUP_Msk;

		k_event_post(&priv->events, BIT(UDC_THREAD_EVT_SETUP));
	}
}

static void sam_isr_handler(const struct device *dev)
{
	struct udc_sam_data *const priv = udc_get_private(dev);
	udphs_registers_t *const udphs = base_reg(dev);
	uint32_t status, mask;
	int i;

	mask = udphs->UDPHS_IEN | UDPHS_INTSTA_SPEED_Msk;
	status = udphs->UDPHS_INTSTA & mask;

	LOG_DBG(" isr, s/%08x", status);

	if (status & UDPHS_INTSTA_DET_SUSPD_Msk) {
		udphs->UDPHS_CLRINT = UDPHS_CLRINT_WAKE_UP_Msk | UDPHS_CLRINT_DET_SUSPD_Msk;
		udphs->UDPHS_IEN |= UDPHS_IEN_WAKE_UP_Msk | UDPHS_IEN_ENDOFRSM_Msk;
		udphs->UDPHS_IEN &= ~UDPHS_IEN_DET_SUSPD_Msk;
		LOG_DBG("Suspend detected %s", dev->name);

		if (!udc_is_suspended(dev)) {
			udc_set_suspended(dev, true);
			udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
		}
	}

	if (status & UDPHS_INTSTA_WAKE_UP_Msk) {
		udphs->UDPHS_CLRINT = UDPHS_CLRINT_WAKE_UP_Msk;
		LOG_DBG("Wake Up detected %s", dev->name);
	}

	if (status & UDPHS_INTSTA_ENDOFRSM_Msk) {
		udphs->UDPHS_CLRINT = UDPHS_CLRINT_ENDOFRSM_Msk;
		udphs->UDPHS_IEN &= ~UDPHS_IEN_WAKE_UP_Msk;
		udphs->UDPHS_IEN |= UDPHS_IEN_DET_SUSPD_Msk;
		LOG_DBG("Resume detected %s", dev->name);

		if (udc_is_suspended(dev)) {
			udc_set_suspended(dev, false);
			udc_submit_event(dev, UDC_EVT_RESUME, 0);
		}
	}

	if (status & UDPHS_INTSTA_DMA__Msk) {
		udphs->UDPHS_IEN |= UDPHS_IEN_DET_SUSPD_Msk;

		for (i = 0; i < UDPHS_DMA_NUMBER; i++) {
			if (status & (UDPHS_INTSTA_DMA_1_Msk << i)) {
				if (handle_dma_irq(dev, i) == 1) {
					/* Update interrupt status for EPT */
					status = udphs->UDPHS_INTSTA & mask;
				}
			}
		}
	}

	if (status & UDPHS_INTSTA_EPT__Msk) {
		udphs->UDPHS_IEN |= UDPHS_IEN_DET_SUSPD_Msk;

		for (i = 0; i < UDPHS_EPT_NUMBER; i++) {
			if (status & (UDPHS_INTSTA_EPT_0_Msk << i)) {
				handle_ep_irq(dev, i);
			}
		}
	}

	if (status & UDPHS_INTSTA_ENDRESET_Msk) {
		udphs->UDPHS_CLRINT = UDPHS_CLRINT_ENDOFRSM_Msk |
				      UDPHS_CLRINT_WAKE_UP_Msk |
				      UDPHS_CLRINT_ENDRESET_Msk |
				      UDPHS_CLRINT_DET_SUSPD_Msk;

		/* Reset and clear all endpionts */
		udphs_reset_ep_all(udphs);

		if (status & UDPHS_INTSTA_SPEED_Msk) {
			priv->speed = UDC_BUS_SPEED_HS;
		} else {
			priv->speed = UDC_BUS_SPEED_FS;
		}

		udc_ep0_internal(dev, 1);
		udc_submit_event(dev, UDC_EVT_RESET, 0);
	}
}

static int udc_sam_ep_enqueue(const struct device *dev,
			      struct udc_ep_config *const ep_cfg, struct net_buf *buf)
{
	struct udc_sam_data *const priv = udc_get_private(dev);

	LOG_DBG("Enqueue ep%02x", ep_cfg->addr);

	udc_buf_put(ep_cfg, buf);

	if (!ep_cfg->stat.halted) {
		atomic_set_bit(&priv->xfer_new, udc_ep_to_bnum(ep_cfg->addr));
		k_event_post(&priv->events, BIT(UDC_THREAD_EVT_XFER_NEW));
	}

	return 0;
}

static int udc_sam_ep_dequeue(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	udphs_ept_registers_t *const ept = ep_reg(dev, idx);
	struct net_buf *buf;
	unsigned int lock_key;

	LOG_DBG("Dequeue ep%02x", ep_cfg->addr);

	lock_key = irq_lock();

	if (ep_can_dma(dev, idx)) {
		udphs_registers_t *const udphs = base_reg(dev);
		udphs_dma_registers_t *const dma = dma_reg(dev, UDC_SAM_DMA(ep_cfg->addr));

		if (udphs_ep_dma_stop(dma)) {
			LOG_ERR("Failed to stop EP %02x DMA", ep_cfg->addr);
		}
		udphs_clear_ep_status(udphs, idx);
	} else {
		if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
			udphs_ep_fifo_in(ept, 0);
		} else {
			udphs_ep_fifo_out(ept, 0);
		}
	}

	buf = udc_buf_get_all(ep_cfg);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
		udc_ep_set_busy(ep_cfg, false);
	}

	irq_unlock(lock_key);

	return 0;
}

static int udc_sam_ep_enable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	udphs_registers_t *const udphs = base_reg(dev);
	uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	struct udphs_ep_config cfg = {0};
	int ret = 0;
	unsigned int lock_key;

	LOG_DBG("Enable ep%02x %s", ep_cfg->addr, ep_can_dma(dev, idx) ? "dma" : "fifo");

	udphs_reset_ep(udphs, idx);

	switch (ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_CONTROL:
		cfg.type = UDPHS_EPTCFG_EPT_TYPE_CTRL8_Val;
		break;
	case USB_EP_TYPE_ISO:
		cfg.type = UDPHS_EPTCFG_EPT_TYPE_ISO_Val;
		break;
	case USB_EP_TYPE_BULK:
		cfg.type = UDPHS_EPTCFG_EPT_TYPE_BULK_Val;
		break;
	case USB_EP_TYPE_INTERRUPT:
		cfg.type = UDPHS_EPTCFG_EPT_TYPE_INT_Val;
		break;
	default:
		return -EINVAL;
	}

	if (idx == 0) {
		cfg.dir = UDPHS_EPTCFG_EPT_DIR_0_Val;
	} else {
		if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
			cfg.dir = UDPHS_EPTCFG_EPT_DIR_1_Val;
		} else {
			cfg.dir = UDPHS_EPTCFG_EPT_DIR_0_Val;
		}
	}

	cfg.size = find_msb_set(ep_cfg->mps) - 4;
	cfg.banks_num = ep_banks(dev, idx);
	cfg.trans_num = 0;
	cfg.dma = ep_can_dma(dev, idx);

	lock_key = irq_lock();

	if (udphs_enable_ep(udphs, idx, &cfg)) {
		LOG_ERR("Failed to config ep %d", idx);
		ret = -EINVAL;
	}

	irq_unlock(lock_key);

	return ret;
}

static int udc_sam_ep_disable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	udphs_registers_t *const udphs = base_reg(dev);
	uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	unsigned int lock_key;

	LOG_DBG("Disable ep%02x", ep_cfg->addr);

	lock_key = irq_lock();

	if (ep_can_dma(dev, idx)) {
		udphs_dma_registers_t *const dma = dma_reg(dev, UDC_SAM_DMA(ep_cfg->addr));

		if (udphs_ep_dma_stop(dma)) {
			LOG_ERR("Failed to stop EP %02x DMA", ep_cfg->addr);
		}
		udphs_clear_ep_status(udphs, idx);
	}
	udphs_disable_ep(udphs, idx);

	irq_unlock(lock_key);

	return 0;
}

static int udc_sam_ep_set_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	udphs_ept_registers_t *const ept = ep_reg(dev, USB_EP_GET_IDX(ep_cfg->addr));
	unsigned int lock_key;

	LOG_DBG("Set halt ep%02x", ep_cfg->addr);

	lock_key = irq_lock();

	udphs_ep_set_halt(ept);
	if (USB_EP_GET_IDX(ep_cfg->addr) != 0) {
		ep_cfg->stat.halted = true;
	} else {
		udphs_rx_setup(base_reg(dev));
	}

	irq_unlock(lock_key);

	return 0;
}

static int udc_sam_ep_clear_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_sam_data *const priv = udc_get_private(dev);
	udphs_ept_registers_t *const ept = ep_reg(dev, USB_EP_GET_IDX(ep_cfg->addr));
	unsigned int lock_key;

	LOG_DBG("Clear halt ep%02x", ep_cfg->addr);


	lock_key = irq_lock();

	udphs_ep_clear_halt(ept, USB_EP_GET_IDX(ep_cfg->addr));
	ep_cfg->stat.halted = false;

	irq_unlock(lock_key);

	if ((USB_EP_GET_IDX(ep_cfg->addr) != 0) &&
	    !udc_ep_is_busy(ep_cfg) &&
	    udc_buf_peek(ep_cfg)) {
		atomic_set_bit(&priv->xfer_new, udc_ep_to_bnum(ep_cfg->addr));
		k_event_post(&priv->events, BIT(UDC_THREAD_EVT_XFER_NEW));
	}

	return 0;
}

static int udc_sam_set_address(const struct device *dev, const uint8_t addr)
{
	udphs_registers_t *const udphs = base_reg(dev);
	unsigned int lock_key;

	LOG_DBG("Set new address %u", addr);

	lock_key = irq_lock();

	udphs_set_address(udphs, addr);

	irq_unlock(lock_key);

	return 0;
}

static int udc_sam_host_wakeup(const struct device *dev)
{
	udphs_registers_t *const udphs = base_reg(dev);
	unsigned int lock_key;

	LOG_DBG("Remote wakeup from %s", dev->name);

	lock_key = irq_lock();

	udphs_send_wakeup(udphs);

	irq_unlock(lock_key);

	return 0;
}

static enum udc_bus_speed udc_sam_device_speed(const struct device *dev)
{
	struct udc_sam_data *const priv = udc_get_private(dev);

	return priv->speed;
}

static int udc_sam_enable(const struct device *dev)
{
	const struct udc_sam_config *config = dev->config;
	udphs_registers_t *const udphs = config->base;

	LOG_DBG("Enable device %s", dev->name);

	if (clock_control_on(SAM_DT_PMC_CONTROLLER, (void *)&config->clock_cfg) != 0) {
		LOG_ERR("Failed to enable pclk");
		return -EIO;
	}

	if (config->speed_idx == UDC_BUS_SPEED_FS) {
		/* Force Full-Speed */
		udphs_speed_mode(udphs, SPEED_FORCE_FULL);
	} else {
		udphs_speed_mode(udphs, SPEED_NORMAL);
	}

	udphs_start(udphs);
	udphs_reset_ep_all(udphs);
	udphs_pullup_en(udphs);

	config->irq_enable_func(dev);

	return 0;
}

static int udc_sam_disable(const struct device *dev)
{
	const struct udc_sam_config *config = dev->config;
	udphs_registers_t *const udphs = config->base;

	LOG_DBG("Disable device %s", dev->name);

	config->irq_disable_func(dev);

	udc_ep0_internal(dev, 0);

	udphs_reset_ep_all(udphs);
	udphs_stop(udphs);

	if (clock_control_off(SAM_DT_PMC_CONTROLLER, (void *)&config->clock_cfg) != 0) {
		LOG_ERR("Failed to disable pclk");
		return -EIO;
	}

	return 0;
}

static void check_vbus(struct udc_sam_data *priv)
{
	const struct gpio_dt_spec *gpio = priv->vbus_gpio;
	int val = gpio_pin_get(gpio->port, gpio->pin);

	if (val < 0) {
		LOG_ERR("Failed to get vbus gpio state %d", val);
		return;
	}

	if (priv->vbus_state != val) {
		priv->vbus_state = val;

		LOG_DBG("Vbus %s", priv->vbus_state ? "detected" : "removed");
		udc_submit_event(priv->dev,
				 priv->vbus_state ? UDC_EVT_VBUS_READY : UDC_EVT_VBUS_REMOVED,
				 0);
	}
}

static void vbus_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct udc_sam_data *priv = CONTAINER_OF(cb, struct udc_sam_data, vbus_cb);

	check_vbus(priv);
}

static int udc_sam_init(const struct device *dev)
{
	const struct udc_sam_config *config = dev->config;
	udphs_registers_t *const udphs = config->base;
	struct udc_sam_data *priv = udc_get_private(dev);
	int ret;

	LOG_DBG("Init device %s", dev->name);

	udphs_stop(udphs);

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	if (config->vbus_gpio.port) {
		gpio_init_callback(&priv->vbus_cb, vbus_callback,
				   BIT(config->vbus_gpio.pin));
		ret = gpio_add_callback(config->vbus_gpio.port, &priv->vbus_cb);
		if (ret) {
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->vbus_gpio,
						      GPIO_INT_EDGE_BOTH);
		if (ret) {
			return ret;
		}

		check_vbus(priv);
	}

	return 0;
}

static int udc_sam_shutdown(const struct device *dev)
{
	LOG_DBG("Shutdown device %s", dev->name);

	return 0;
}

static int udc_sam_driver_preinit(const struct device *dev)
{
	const struct udc_sam_config *config = dev->config;
	struct udc_sam_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	udphs_registers_t *const udphs = config->base;
	int ret;
	int i;

	LOG_DBG("Driver preinit %s", dev->name);

	/* Make sure we start from a clean slate */
	if (clock_control_on(SAM_DT_PMC_CONTROLLER, (void *)&config->clock_cfg) != 0) {
		LOG_ERR("Failed to enable pclk");
		return -EIO;
	}

	udphs_reset(udphs);

	if (clock_control_off(SAM_DT_PMC_CONTROLLER, (void *)&config->clock_cfg) != 0) {
		LOG_ERR("Failed to disable pclk");
		return -EIO;
	}

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);
	atomic_clear(&priv->xfer_new);
	atomic_clear(&priv->xfer_finished);

	priv->dev = dev;
	data->caps.rwup = true;
	data->caps.out_ack = true;
	data->caps.mps0 = UDC_MPS0_64;
	if (config->vbus_gpio.port) {
		data->caps.can_detect_vbus = true;
	}
	if (config->speed_idx == UDC_BUS_SPEED_HS) {
		data->caps.hs = true;
	}

	for (i = 0; i < UDPHS_EPT_NUMBER / 2; i++) {
		config->ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			config->ep_cfg_out[i].caps.control = 1;
			config->ep_cfg_out[i].caps.mps = ep_size(dev, i);
			config->ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		} else {
			config->ep_cfg_out[i].caps.bulk = 1;
			config->ep_cfg_out[i].caps.interrupt = 1;
			config->ep_cfg_out[i].caps.iso = 1;
			config->ep_cfg_out[i].caps.mps = ep_size(dev, i * 2 - 1);
			config->ep_cfg_out[i].addr = USB_EP_DIR_OUT | (i * 2 - 1);
		}

		LOG_DBG("Register OUT ep %02x", config->ep_cfg_out[i].addr);
		ret = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (ret != 0) {
			LOG_ERR("Failed to register endpoint");
			return ret;
		}
	}

	for (i = 0; i < UDPHS_EPT_NUMBER / 2; i++) {
		config->ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			config->ep_cfg_in[i].caps.control = 1;
			config->ep_cfg_in[i].caps.mps = ep_size(dev, i);
		} else {
			config->ep_cfg_in[i].caps.bulk = 1;
			config->ep_cfg_in[i].caps.interrupt = 1;
			config->ep_cfg_in[i].caps.iso = 1;
			config->ep_cfg_in[i].caps.mps = ep_size(dev, i * 2);
		}

		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | (i * 2);
		LOG_DBG("Regisger IN ep %02x", config->ep_cfg_in[i].addr);
		ret = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (ret != 0) {
			LOG_ERR("Failed to register endpoint");
			return ret;
		}
	}

	config->make_thread(dev);
	config->irq_config_func(dev);

	return 0;
}

static void udc_sam_lock(const struct device *dev)
{
	k_sched_lock();
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_sam_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
	k_sched_unlock();
}

static const struct udc_api udc_sam_api = {
	.lock = udc_sam_lock,
	.unlock = udc_sam_unlock,
	.device_speed = udc_sam_device_speed,
	.init = udc_sam_init,
	.enable = udc_sam_enable,
	.disable = udc_sam_disable,
	.shutdown = udc_sam_shutdown,
	.set_address = udc_sam_set_address,
	.host_wakeup = udc_sam_host_wakeup,
	.ep_enable = udc_sam_ep_enable,
	.ep_disable = udc_sam_ep_disable,
	.ep_set_halt = udc_sam_ep_set_halt,
	.ep_clear_halt = udc_sam_ep_clear_halt,
	.ep_enqueue = udc_sam_ep_enqueue,
	.ep_dequeue = udc_sam_ep_dequeue,
};

#define UDC_SAM_IRQ_CONFIG(i, n)					\
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, i, irq),			\
		    DT_INST_IRQ_BY_IDX(n, i, priority),			\
		    sam_isr_handler, DEVICE_DT_INST_GET(n), 0);

#define UDC_SAM_IRQ_ENABLE(i, n)					\
	irq_enable(DT_INST_IRQ_BY_IDX(n, i, irq));

#define UDC_SAM_IRQ_DISABLE(i, n)					\
	irq_disable(DT_INST_IRQ_BY_IDX(n, i, irq));

#define UDC_SAM_IRQ_CONFIG_DEFINE(i, n)					\
static void udc_sam_irq_config_func_##n(const struct device *dev)	\
{									\
	LISTIFY(DT_INST_NUM_IRQS(n), UDC_SAM_IRQ_CONFIG, (), n)		\
}

#define UDC_SAM_IRQ_ENABLE_DEFINE(i, n)					\
static void udc_sam_irq_enable_func_##n(const struct device *dev)	\
{									\
	LISTIFY(DT_INST_NUM_IRQS(n), UDC_SAM_IRQ_ENABLE, (), n)		\
}

#define UDC_SAM_IRQ_DISABLE_DEFINE(i, n)				\
static void udc_sam_irq_disable_func_##n(const struct device *dev)	\
{									\
	LISTIFY(DT_INST_NUM_IRQS(n), UDC_SAM_IRQ_DISABLE, (), n)	\
}

#define UDC_SAM_PINCTRL_DT_INST_DEFINE(n)				\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),		\
		    (PINCTRL_DT_INST_DEFINE(n)), ())

#define UDC_SAM_PINCTRL_DT_INST_DEV_CONFIG_GET(n)			\
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),		\
		    ((void *)PINCTRL_DT_INST_DEV_CONFIG_GET(n)),	\
		    (NULL))

#define UDC_SAM_DEVICE_DEFINE(n)						\
	UDC_SAM_PINCTRL_DT_INST_DEFINE(n);					\
	UDC_SAM_IRQ_CONFIG_DEFINE(i, n);					\
	UDC_SAM_IRQ_ENABLE_DEFINE(i, n);					\
	UDC_SAM_IRQ_DISABLE_DEFINE(i, n);					\
										\
	K_THREAD_STACK_DEFINE(udc_sam_stack_##n,				\
			      CONFIG_UDC_MCHP_SAM_STACK_SIZE);			\
										\
	static void udc_sam_thread_##n(void *dev, void *arg1, void *arg2)	\
	{									\
		while (true) {							\
			udc_thread_handler(dev);				\
		}								\
	}									\
										\
	static void udc_sam_make_thread_##n(const struct device *dev)		\
	{									\
		struct udc_sam_data *priv = udc_get_private(dev);		\
										\
		k_thread_create(&priv->thread_data,				\
				udc_sam_stack_##n,				\
				K_THREAD_STACK_SIZEOF(udc_sam_stack_##n),	\
				udc_sam_thread_##n,				\
				(void *)dev, NULL, NULL,			\
				K_PRIO_COOP(CONFIG_UDC_MCHP_SAM_THREAD_PRI),	\
				K_ESSENTIAL,					\
				K_NO_WAIT);					\
		k_thread_name_set(&priv->thread_data, dev->name);		\
	}									\
										\
	static struct udc_ep_config						\
		ep_cfg_out[UDPHS_EPT_NUMBER];					\
	static struct udc_ep_config						\
		ep_cfg_in[UDPHS_EPT_NUMBER];					\
										\
	static const struct udc_sam_config udc_sam_config_##n = {		\
		.base = (udphs_registers_t *)DT_INST_REG_ADDR_BY_IDX(n, 1),	\
		.fifo = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(n, 0),		\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),			\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.ep_desc = sam_ep_desc,						\
		.speed_idx = DT_ENUM_IDX(DT_DRV_INST(n), maximum_speed),	\
		.ep_cfg_in = ep_cfg_in,						\
		.ep_cfg_out = ep_cfg_out,					\
		.irq_config_func = udc_sam_irq_config_func_##n,			\
		.irq_enable_func = udc_sam_irq_enable_func_##n,			\
		.irq_disable_func = udc_sam_irq_disable_func_##n,		\
		.vbus_gpio = GPIO_DT_SPEC_INST_GET_OR(n, vbus_gpios, {0}),	\
		.make_thread = udc_sam_make_thread_##n,				\
	};									\
										\
	static struct udc_sam_data udc_priv_##n = {				\
		.vbus_gpio = &udc_sam_config_##n.vbus_gpio,			\
		.speed = UDC_BUS_UNKNOWN,					\
	};									\
										\
	static struct udc_data udc_data_##n = {					\
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),		\
		.priv = &udc_priv_##n,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, udc_sam_driver_preinit, NULL,			\
			      &udc_data_##n, &udc_sam_config_##n,		\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &udc_sam_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_SAM_DEVICE_DEFINE)
