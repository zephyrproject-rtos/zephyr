/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tidss

#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/cache.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(display_tidss, CONFIG_DISPLAY_LOG_LEVEL);

/* Common region register offsets  (base = common) */
#define DSS_SYSSTATUS                   0x20
#define DSS_IRQ_EOI                     0x24    /* W: write 0 to re-arm IRQ   */
#define DSS_IRQSTATUS                   0x2c    /* R/W1C: aggregated status   */
#define DSS_IRQENABLE_SET               0x30    /* W: set enable bits         */
#define DSS_IRQENABLE_CLR               0x40    /* W: clear enable bits       */
#define DSS_VP_IRQENABLE(n)             (0x70 + (n) * 4U) /* per-VP enable   */
#define DSS_VP_IRQSTATUS(n)             (0x7c + (n) * 4U) /* per-VP W1C      */
#define DSS_GLOBAL_MFLAG_ATTRIBUTE      0x90
#define DSS_CBA_CFG                     0x9c

/* DSS_VP_IRQSTATUS / DSS_VP_IRQENABLE bits */
#define VP_IRQ_FRAMEDONE                BIT(0)  /* end of VBP                 */
#define VP_IRQ_VSYNC	                BIT(1)  /* start of VSYNC pulse       */
#define VP_IRQ_VSYNC_ODD                BIT(2)  /* interlaced odd field       */
#define VP_IRQ_SYNC_LOST                BIT(4)  /* pipeline stall             */
#define VP_IRQ_VSYNC_GO			BIT(11)

/*  VID plane register offsets  (base = "vid" region) */
#define DSS_VID_ATTRIBUTES              0x20
#define DSS_VID_BA_0                    0x28    /* Buffer base addr 0 [31:0]  */
#define DSS_VID_BA_1                    0x2c    /* Buffer base addr 1 [31:0]  */
#define DSS_VID_BUF_SIZE_STATUS         0x38    /* HW FIFO depth (read-only)  */
#define DSS_VID_BUF_THRESHOLD           0x3c    /* High[31:16] / Low[15:0]    */
#define DSS_VID_GLOBAL_ALPHA            0x1fc
#define DSS_VID_MFLAG_THRESHOLD         0x208   /* MFLAG high[31:16] low[15:0]*/
#define DSS_VID_PICTURE_SIZE            0x20c   /* (w-1)[11:0] (h-1)[27:16]   */
#define DSS_VID_PRELOAD                 0x218
#define DSS_VID_BA_EXT_0                0x22c   /* Buffer base addr 0 [35:32] */
#define DSS_VID_BA_EXT_1                0x230   /* Buffer base addr 1 [35:32] */

/* DSS_VID_ATTRIBUTES bit fields */
#define VID_ATTR_ENABLE             BIT(0)          /* [0]    plane enable   */
#define VID_ATTR_FORMAT_MASK        GENMASK(6, 1)   /* [6:1]  pixel format   */
#define VID_ATTR_CSC_EN             BIT(9)          /* [9]    CSC enable     */
#define VID_ATTR_BUFPRELOAD         BIT(19)         /* [19]   0=preload mode */
#define VID_ATTR_PREMULTIPLYALPHA   BIT(28)         /* [28]   premult alpha  */

/*  OVR (overlay) register offsets  (base = "ovr" region) */
#define DSS_OVR_CONFIG			0x0 /* layer n attributes */
#define DSS_OVR_DEFAULT_COLOR           0x08
#define DSS_OVR_DEFAULT_COLOR2          0x0c
#define DSS_OVR_TRANS_COLOR_MAX         0x10
#define DSS_OVR_ATTRIBUTES(n)           (0x20 + (n) * 4)

/* DSS_OVR_ATTRIBUTES(layer) bit fields */
#define OVR_ATTR_ENABLE             BIT(0)           /* [0]     layer enable      */
#define OVR_ATTR_CHANNELIN_MASK     GENMASK(4, 1)    /* [4:1]   source VID channel*/
#define OVR_ATTR_XPOS_MASK          GENMASK(17, 6)   /* [17:6]  X position        */
#define OVR_ATTR_YPOS_MASK          GENMASK(30, 19)  /* [30:19] Y position        */

/* OVR CHANNELIN values */
#define OVR_CHANNEL_VIDL1               1       /* vidl1 (lite plane)         */
#define OVR_CHANNEL_VID                 0       /* vid   (full plane)         */

/*  VP (video port) register offsets  (base = "vp" region) */
#define DSS_VP_CONFIG                   0x00
#define DSS_VP_CONTROL                  0x04
#define DSS_VP_POL_FREQ                 0x4c
#define DSS_VP_SIZE_SCREEN              0x50
#define DSS_VP_TIMING_H                 0x54
#define DSS_VP_TIMING_V                 0x58
#define DSS_VP_GAMMA_TABLE              0x120
#define DSS_VP_DSS_OLDI_CFG             0x160

/* DSS_VP_CONTROL bit fields */
#define VP_CTRL_ENABLE          BIT(0)          /* [0]    VP output enable    */
#define VP_CTRL_GO              BIT(5)          /* [5]    shadow latch        */
#define VP_CTRL_DPIENABLE       BIT(6)
#define VP_CTRL_DATALINES_MASK  GENMASK(10, 8)  /* [10:8] data line count     */

/* DSS_VP_CONFIG bit field */
#define VP_CFG_GAMMA_SHADOW     BIT(2)          /* [2] enable gamma shadow reg*/

/*  Pixel format code for ARGB8888 */
#define DSS_FORMAT_ARGB8888             0x7

/* Gamma LUT size for 8-bit gamma */
#define DSS_GAMMA_SIZE                  256

/* Bytes per pixel for ARGB8888 (matches Zephyr PIXEL_FORMAT_ARGB_8888) */
#define DSS_BPP                         4

/* VSYNC timing parameters for tear-free operation */
#define FRAME_PERIOD_MS          16   /* 60 Hz = ~16.67 ms per frame */
#define VSYNC_SAFE_THRESHOLD_MS  2    /* Don't update if < 2ms to next VSYNC */

/*  Driver configuration */
struct tidss_config {
	/* MMIO bases for each DSS sub-region */
	DEVICE_MMIO_NAMED_ROM(common);
	DEVICE_MMIO_NAMED_ROM(vid);      /* one video plane (VID or VIDL)   */
	DEVICE_MMIO_NAMED_ROM(ovr);      /* one overlay                     */
	DEVICE_MMIO_NAMED_ROM(vp);       /* one video port                  */

	const struct pinctrl_dev_config *pinctrl;

	/* VP pixel clock */
	const struct device *vp_clk_dev;

	/* vp_clk_subsys: platform-specific clock subsystem selector. */
	clock_control_subsys_t vp_clk_subsys;

	/* Functional clock */
	const struct device *func_clk_dev;

	/* Function clock subsys */
	clock_control_subsys_t func_clk_subsys;

	uint8_t vp_idx;
	uint8_t vid_channel;

	/* Nominal pixel clock in Hz */
	uint32_t pixel_clk_hz;

	/* Active video dimensions */
	uint16_t hactive;
	uint16_t vactive;

	/* Horizontal blanking (in pixel clocks) */
	uint16_t hfp;           /* front porch  */
	uint16_t hbp;           /* back porch   */
	uint16_t hsync;         /* sync width   */

	/* Vertical blanking (in lines) */
	uint16_t vfp;
	uint16_t vbp;
	uint16_t vsync;

	/* Sync polarities — true means the signal is active-low (inverted) */
	bool ivs;               /* invert VSYNC */
	bool ihs;               /* invert HSYNC */

	/* Output bus width in bits (12/16/18/24/30/36) */
	uint8_t data_width;

	/* Per-instance IRQ connect + enable function */
	void (*irq_config_func)(const struct device *dev);

	/* Pointer to driver-allocated framebuffer pool */
	uint8_t *fb_ptr;
};

/* Driver runtime data */
struct tidss_data {
	/* MMIO bases for each DSS sub-region */
	DEVICE_MMIO_NAMED_RAM(common);
	DEVICE_MMIO_NAMED_RAM(vid);      /* one video plane (VID or VIDL)   */
	DEVICE_MMIO_NAMED_RAM(ovr);      /* one overlay                     */
	DEVICE_MMIO_NAMED_RAM(vp);       /* one video port                  */

	/* Pointer to active framebuffer (currently being scanned by DSS DMA) */
	const uint8_t *active_fb;

#define TIDSS_REQ_Q_DEPTH  4   /* frames caller can submit ahead of display */
	struct k_msgq    req_q;
	const uint8_t   *req_q_buf[TIDSS_REQ_Q_DEPTH];
	const uint8_t   *cur_fb;        /* currQ: buffer in DSS VID pipe     */
	bool           is_push_safe;  /* TRUE during VSYNC blanking window  */
	struct k_spinlock lock;       /* protects is_push_safe + cur_fb     */

	/* Row stride in bytes (hactive * DSS_BPP) */
	uint32_t stride_bytes;
	/* Total framebuffer size in bytes (kept for cache-flush helpers) */
	size_t fb_bytes;

	uint64_t last_vsync_time;         /* timestamp of last VSYNC interrupt */

	const uint8_t *scan_fb;

	/* Event callback — single VSYNC subscriber (LVGL or application) */
	display_event_cb_t vsync_cb;
	void              *vsync_cb_user_data;
	uint32_t           vsync_reg_handle; /* 0 = nothing registered */

	enum display_pixel_format pixel_format;
	bool blanking;
};

#define DEV_CFG(dev)     ((const struct tidss_config *)dev->config)
#define DEV_DATA(dev)     ((struct tidss_data *)dev->data)

/*  Register read / write — one helper set per sub-region */
static inline void dss_write(uintptr_t base, uint32_t off, uint32_t val)
{
	sys_write32(val, base + off);
}

static inline uint32_t dss_read(uintptr_t base, uint32_t off)
{
	return sys_read32(base + off);
}

static inline void dss_fld_mod(uintptr_t base, uint32_t off,
				uint32_t mask, uint32_t val)
{
	dss_write(base, off, (dss_read(base, off) & ~mask) | FIELD_PREP(mask, val));
}

#define common_write(dev, off, v)             dss_write((uintptr_t)DEVICE_MMIO_NAMED_GET(dev,   \
								common), off, v)
#define common_read(dev, off)                 dss_read((uintptr_t)DEVICE_MMIO_NAMED_GET(dev,    \
								common), off)
#define common_fld_mod(dev, off, mask, v)     dss_fld_mod((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, \
								common), off, mask, v)

#define vid_write(dev, off, v)                dss_write((uintptr_t)DEVICE_MMIO_NAMED_GET(dev,   \
								vid), off, v)
#define vid_read(dev, off)                    dss_read((uintptr_t)DEVICE_MMIO_NAMED_GET(dev,    \
								vid), off)
#define vid_fld_mod(dev, off, mask, v)        dss_fld_mod((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, \
								vid), off, mask, v)

#define ovr_write(dev, off, v)                dss_write((uintptr_t)DEVICE_MMIO_NAMED_GET(dev,   \
								ovr), off, v)
#define ovr_fld_mod(dev, off, mask, v)        dss_fld_mod((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, \
								ovr), off, mask, v)

#define vp_write(dev, off, v)                 dss_write((uintptr_t)DEVICE_MMIO_NAMED_GET(dev,   \
								vp), off, v)
#define vp_read(dev, off)                     dss_read((uintptr_t)DEVICE_MMIO_NAMED_GET(dev,    \
								vp), off)
#define vp_fld_mod(dev, off, mask, v)         dss_fld_mod((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, \
								vp), off, mask, v)

/*  GO bit helpers */
static void dss_vp_go(const struct device *dev)
{
	vp_fld_mod(dev, DSS_VP_CONTROL, VP_CTRL_GO, 1U);
}

/*
 *  Shadow framebuffer base-address update
 *
 *  Programs both ping (BA_0) and pong (BA_1) shadow registers to the
 *  same physical address.  Takes effect at the next VFP after GO is set.
 */
static void dss_vid_set_fb(const struct device *dev, struct tidss_data *data)
{
	uintptr_t phys = k_mem_phys_addr((void *)(uintptr_t)data->cur_fb);

	vid_write(dev, DSS_VID_BA_0,     (uint32_t)(phys & 0xFFFFFFFFU));
	vid_write(dev, DSS_VID_BA_EXT_0, (uint32_t)(phys >> 32U));
	vid_write(dev, DSS_VID_BA_1,     (uint32_t)(phys & 0xFFFFFFFFU));
	vid_write(dev, DSS_VID_BA_EXT_1, (uint32_t)(phys >> 32U));

	/* Enable the VID plane on the first write only */
	if (data->scan_fb == NULL) {
		vid_fld_mod(dev, DSS_VID_ATTRIBUTES, VID_ATTR_ENABLE, 1U);
	}
}

/* VSYNC interrupt enable */
static void dss_enable_irqs(const struct device *dev)
{
	const struct tidss_config *cfg = DEV_CFG(dev);
	uint32_t vp_irq_bit = BIT(cfg->vp_idx);

	common_write(dev, DSS_VP_IRQENABLE(cfg->vp_idx),
		     VP_IRQ_VSYNC_GO | VP_IRQ_SYNC_LOST);
	/* Unmask the selected VP in the top-level enable register */
	common_write(dev, DSS_IRQENABLE_CLR, vp_irq_bit);
	common_write(dev, DSS_IRQENABLE_SET, vp_irq_bit);
}

/*  VP gamma LUT — linear ramp 0..255 on all three channels */
static void dss_vp_setup_gamma(const struct device *dev)
{
	uint32_t hwlen  = DSS_GAMMA_SIZE;
	uint32_t hwbits = 8;
	uint32_t i;

	for (i = 0; i < hwlen; i++) {
		uint32_t val_u16 = (65535U * i) / (hwlen - 1U);
		uint32_t val     = val_u16 >> (16U - hwbits);
		uint32_t entry   = (val << 16U) | (val << 8U) | val;

		vp_write(dev, DSS_VP_GAMMA_TABLE, (i << 24U) | entry);
	}
}

/*  Plane (VID) setup — address, format, picture size, alpha, enable */
static void dss_plane_setup(const struct device *dev, struct tidss_data *data)
{
	vid_fld_mod(dev, DSS_VID_ATTRIBUTES, VID_ATTR_FORMAT_MASK, DSS_FORMAT_ARGB8888);

	vid_write(dev, DSS_VID_PICTURE_SIZE,
		  FIELD_PREP(GENMASK(11,  0), DEV_CFG(dev)->hactive - 1U) |
		  FIELD_PREP(GENMASK(27, 16), DEV_CFG(dev)->vactive - 1U));

	vid_write(dev, DSS_VID_GLOBAL_ALPHA, 0xFFU);
	vid_fld_mod(dev, DSS_VID_ATTRIBUTES, VID_ATTR_CSC_EN,          0U);
	vid_fld_mod(dev, DSS_VID_ATTRIBUTES, VID_ATTR_BUFPRELOAD,       0U);
	vid_fld_mod(dev, DSS_VID_ATTRIBUTES, VID_ATTR_PREMULTIPLYALPHA, 0U);

	vid_fld_mod(dev, DSS_VID_ATTRIBUTES, VID_ATTR_ENABLE, 0U);
}


/* VSYNC timing parameters for tear-free operation */
#define FRAME_PERIOD_MS          16   /* 60 Hz = ~16.67 ms per frame */
#define VSYNC_SAFE_THRESHOLD_MS  2    /* Don't update if < 2ms to next VSYNC */

/*  Plane FIFO thresholds and MFLAG (memory flag / QoS) configuration */
static void dss_plane_init(const struct device *dev)
{
	uint32_t size, thr_high, thr_low, mflag_high, mflag_low;

	common_fld_mod(dev, DSS_CBA_CFG, GENMASK(2, 0), 1U);
	common_fld_mod(dev, DSS_CBA_CFG, GENMASK(5, 3), 0U);

	common_fld_mod(dev, DSS_GLOBAL_MFLAG_ATTRIBUTE, GENMASK(1, 0), 2U);
	common_fld_mod(dev, DSS_GLOBAL_MFLAG_ATTRIBUTE, BIT(6),        0U);

	size = FIELD_GET(GENMASK(15, 0), vid_read(dev, DSS_VID_BUF_SIZE_STATUS));
	if (size == 0U) {
		LOG_WRN("VID BUF_SIZE_STATUS reads 0; falling back to 512");
		size = 512U;
	}

	thr_high   = size - 1U;
	thr_low    = size / 2U;
	mflag_high = (size * 2U) / 3U;
	mflag_low  = size / 3U;

	LOG_DBG("VID FIFO size=%u thr=%u/%u mflag=%u/%u preload=%u",
		size, thr_high, thr_low, mflag_high, mflag_low, thr_low);

	vid_write(dev, DSS_VID_BUF_THRESHOLD,
		  FIELD_PREP(GENMASK(31, 16), thr_high)   | FIELD_PREP(GENMASK(15, 0), thr_low));

	vid_write(dev, DSS_VID_MFLAG_THRESHOLD,
		  FIELD_PREP(GENMASK(31, 16), mflag_high) | FIELD_PREP(GENMASK(15, 0), mflag_low));

	vid_write(dev, DSS_VID_PRELOAD, thr_low);
}

/*  Overlay setup */
static void dss_ovr_setup(const struct device *dev)
{
	const struct tidss_config *cfg = DEV_CFG(dev);
	uint32_t attr = 0;

	attr = FIELD_PREP(OVR_ATTR_CHANNELIN_MASK, cfg->vid_channel) |
	       FIELD_PREP(OVR_ATTR_XPOS_MASK, 0U) |
	       FIELD_PREP(OVR_ATTR_YPOS_MASK, 0U) |
	       OVR_ATTR_ENABLE;


	ovr_write(dev, DSS_OVR_ATTRIBUTES(0), attr);
	ovr_write(dev, DSS_OVR_DEFAULT_COLOR,  0x00000000U);
	ovr_write(dev, DSS_OVR_DEFAULT_COLOR2, 0x00000000U);
}

/*  VP data-line count  (DSS_VP_CONTROL[10:8]) */
static void dss_vp_set_num_datalines(const struct device *dev)
{
	uint32_t v;

	switch (DEV_CFG(dev)->data_width) {
	case 12:
		v = 0U;
		break;
	case 16:
		v = 1U;
		break;
	case 18:
		v = 2U;
		break;
	case 24:
		v = 3U;
		break;
	case 30:
		v = 4U;
		break;
	case 36:
		v = 5U;
		break;
	default:
		LOG_WRN("Unsupported data_width %u; defaulting to 24-bit",
			DEV_CFG(dev)->data_width);
		v = 3U;
		break;
	}

	vp_fld_mod(dev, DSS_VP_CONTROL, VP_CTRL_DATALINES_MASK, v);
}

/*  VP timing registers and output enable */
static void dss_vp_enable(const struct device *dev)
{
	dss_vp_set_num_datalines(dev);

	vp_write(dev, DSS_VP_TIMING_H,
		 FIELD_PREP(GENMASK(7,   0), DEV_CFG(dev)->hsync - 1U) |
		 FIELD_PREP(GENMASK(19,  8), DEV_CFG(dev)->hfp   - 1U) |
		 FIELD_PREP(GENMASK(31, 20), DEV_CFG(dev)->hbp   - 1U));

	vp_write(dev, DSS_VP_TIMING_V,
		 FIELD_PREP(GENMASK(7,   0), DEV_CFG(dev)->vsync - 1U) |
		 FIELD_PREP(GENMASK(19,  8), DEV_CFG(dev)->vfp)        |
		 FIELD_PREP(GENMASK(31, 20), DEV_CFG(dev)->vbp));

	vp_write(dev, DSS_VP_POL_FREQ,
		 BIT(18) | BIT(17) | BIT(16) |
		 FIELD_PREP(BIT(13), DEV_CFG(dev)->ihs) |
		 FIELD_PREP(BIT(12), DEV_CFG(dev)->ivs));

	vp_write(dev, DSS_VP_SIZE_SCREEN,
		 FIELD_PREP(GENMASK(11,  0), DEV_CFG(dev)->hactive - 1U) |
		 FIELD_PREP(GENMASK(27, 16), DEV_CFG(dev)->vactive - 1U));

	vp_fld_mod(dev, DSS_VP_CONTROL, VP_CTRL_DPIENABLE, 1U);
	vp_fld_mod(dev, DSS_VP_CONTROL, VP_CTRL_ENABLE,    1U);
}

/*  VP gamma shadow register enable  (DSS_VP_CONFIG[2]) */
static void dss_vp_init(const struct device *dev)
{
	vp_fld_mod(dev, DSS_VP_CONFIG, VP_CFG_GAMMA_SHADOW, 1U);
}

/*  VSYNC interrupt service routine */
static void tidss_isr(const struct device *dev)
{
	const struct tidss_config *cfg  = DEV_CFG(dev);
	struct tidss_data         *data = DEV_DATA(dev);
	uint32_t vp_stat;

	/* Read and W1C the VP-specific status register */
	vp_stat = common_read(dev, DSS_VP_IRQSTATUS(cfg->vp_idx));
	common_write(dev, DSS_VP_IRQSTATUS(cfg->vp_idx), vp_stat);

	/* W1C top-level aggregated status */
	common_write(dev, DSS_IRQSTATUS, common_read(dev, DSS_IRQSTATUS));

	/* Re-arm interrupt controller */
	common_write(dev, DSS_IRQ_EOI, 0U);

	if (vp_stat & VP_IRQ_SYNC_LOST) {
		LOG_ERR("VP0 SYNC_LOST — DSS pipeline stalled");
		return;
	}

	/* Update the last vsync time to avoid frame tearing. */
	data->last_vsync_time = k_uptime_get();

	/* Function as per the callback approach. */
	const uint8_t *old_scan_fb = data->scan_fb;

	data->scan_fb = data->cur_fb;

	if (data->scan_fb != NULL &&
	    data->scan_fb != old_scan_fb &&
	    data->vsync_cb != NULL) {
		struct display_event_data evt = {
			.timestamp = k_cycle_get_64(),
		};

		data->vsync_cb(dev, DISPLAY_EVENT_VSYNC, &evt,
				data->vsync_cb_user_data);
	}

	data->active_fb = old_scan_fb;  /* safe to render into now */

	/* Phase 2: open safe window, commit next queued buffer to shadow regs. */
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->is_push_safe = true;

	const uint8_t *next_fb;

	if (k_msgq_get(&data->req_q, &next_fb, K_NO_WAIT) == 0) {
		data->cur_fb = next_fb;
		dss_vid_set_fb(dev, data);
		dss_vp_go(dev);
		data->is_push_safe = false;
	}

	k_spin_unlock(&data->lock, key);
}

/* Write pixel data to the display. */
static int tidss_write(const struct device *dev,
		       const uint16_t x, const uint16_t y,
		       const struct display_buffer_descriptor *desc,
		       const void *buf)
{
	const struct tidss_config *cfg  = DEV_CFG(dev);
	struct tidss_data         *data = DEV_DATA(dev);

	if (data->blanking) {
		return 0;
	}

	if ((x + desc->width)  > cfg->hactive ||
	    (y + desc->height) > cfg->vactive) {
		LOG_ERR("Write [%u,%u %ux%u] out of framebuffer [%ux%u]",
			x, y, desc->width, desc->height,
			cfg->hactive, cfg->vactive);
		return -EINVAL;
	}

	LOG_DBG("W=%d, H=%d @%d,%d", desc->width, desc->height, x, y);

	/* If we are too close to the next VSYNC, wait for it to pass to avoid tearing. */
	if (data->last_vsync_time != 0) {
		int64_t now = k_uptime_get();
		int64_t time_since_last_vsync = now - data->last_vsync_time;
		int64_t time_to_next_vsync = FRAME_PERIOD_MS - time_since_last_vsync;

		/* If less than the threshold to next VSYNC, wait for the VSYNC to occur.*/
		if (time_to_next_vsync < VSYNC_SAFE_THRESHOLD_MS) {
			/* Sleep for a short duration to avoid busy waiting */
			k_msleep(VSYNC_SAFE_THRESHOLD_MS);
		}
	}

	const uint8_t *fb = (const uint8_t *)buf;

	if (k_msgq_put(&data->req_q, &fb, K_NO_WAIT) != 0) {
		k_msgq_purge(&data->req_q);
		(void)k_msgq_put(&data->req_q, &fb, K_NO_WAIT);
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->is_push_safe) {
		const uint8_t *next_fb;

		if (k_msgq_get(&data->req_q, &next_fb, K_NO_WAIT) == 0) {
			data->cur_fb = next_fb;
			dss_vid_set_fb(dev, data);
			dss_vp_go(dev);
			data->is_push_safe = false;
		}
	}
	k_spin_unlock(&data->lock, key);

	if (data->vsync_cb != NULL) {
		return 0;
	}

	return 0;
}

/* Disable VP output (blanking on). */
static int tidss_blanking_on(const struct device *dev)
{
	struct tidss_data         *data = DEV_DATA(dev);

	vp_fld_mod(dev, DSS_VP_CONTROL, VP_CTRL_ENABLE, 0U);
	data->blanking = true;
	return 0;
}

/* Re-enable VP output (blanking off). */
static int tidss_blanking_off(const struct device *dev)
{
	struct tidss_data         *data = DEV_DATA(dev);

	vp_fld_mod(dev, DSS_VP_CONTROL, VP_CTRL_ENABLE, 1U);
	data->blanking = false;
	return 0;
}


/* Return a pointer to the framebuffer for direct CPU access. */
static void *tidss_get_framebuffer(const struct device *dev)
{
	struct tidss_data *data = DEV_DATA(dev);

	return (void *)(uintptr_t)data->active_fb;
}

/* Report display capabilities. */
static void tidss_get_capabilities(const struct device *dev,
				   struct display_capabilities *caps)
{
	const struct tidss_config *cfg  = DEV_CFG(dev);
	struct tidss_data         *data = DEV_DATA(dev);

	caps->x_resolution            = cfg->hactive;
	caps->y_resolution            = cfg->vactive;
	caps->supported_pixel_formats = PIXEL_FORMAT_ARGB_8888;
	caps->screen_info             = SCREEN_INFO_DOUBLE_BUFFER;
	caps->current_pixel_format    = data->pixel_format;
	caps->current_orientation     = DISPLAY_ORIENTATION_NORMAL;
}

/* Register a display event callback. Only DISPLAY_EVENT_VSYNC is supported. */
static int tidss_register_event_cb(const struct device *dev,
				   display_event_cb_t cb, void *user_data,
				   uint32_t event_mask, bool in_isr,
				   uint32_t *out_reg_handle)
{
	struct tidss_data *data = DEV_DATA(dev);

	if (cb == NULL || out_reg_handle == NULL) {
		return -EINVAL;
	}
	if (data->vsync_reg_handle != 0U) {
		return -EBUSY;
	}
	if (!(event_mask & DISPLAY_EVENT_VSYNC)) {
		return -ENOTSUP;
	}
	/* Only ISR-context delivery is implemented. */
	if (!in_isr) {
		return -ENOTSUP;
	}

	data->vsync_cb           = cb;
	data->vsync_cb_user_data = user_data;
	data->vsync_reg_handle   = 1U;
	*out_reg_handle          = 1U;
	return 0;
}

/* Unregister a previously registered display event callback. */
static int tidss_unregister_event_cb(const struct device *dev, uint32_t reg_handle)
{
	struct tidss_data *data = DEV_DATA(dev);

	if (reg_handle == 0U) {
		return -EINVAL;
	}
	if (reg_handle != data->vsync_reg_handle) {
		return -EPERM;
	}

	data->vsync_cb           = NULL;
	data->vsync_cb_user_data = NULL;
	data->vsync_reg_handle   = 0U;
	return 0;
}

static DEVICE_API(display, tidss_api) = {
	.blanking_on         = tidss_blanking_on,
	.blanking_off        = tidss_blanking_off,
	.write               = tidss_write,
	.get_framebuffer     = tidss_get_framebuffer,
	.get_capabilities    = tidss_get_capabilities,
	.register_event_cb   = tidss_register_event_cb,
	.unregister_event_cb = tidss_unregister_event_cb,
};

static int tidss_init(const struct device *dev)
{
	const struct tidss_config *cfg  = DEV_CFG(dev);
	struct tidss_data         *data = DEV_DATA(dev);
	int ret;

	DEVICE_MMIO_NAMED_MAP(dev, common, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, vid, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, ovr, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, vp, K_MEM_CACHE_NONE);

	/* Apply pinctrl if defined in the device tree */
	if (cfg->pinctrl != NULL) {
		ret = pinctrl_apply_state(cfg->pinctrl, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("failed to apply pinctrl (%d)", ret);
			return ret;
		}
	}

	/* Initialise the request queue */
	k_msgq_init(&data->req_q, (char *)data->req_q_buf,
		    sizeof(const uint8_t *), TIDSS_REQ_Q_DEPTH);

	data->cur_fb    = NULL;   /* shadow regs: nothing committed yet */
	data->scan_fb   = NULL;   /* active regs: DSS not scanning yet  */
	data->active_fb = NULL;   /* freed buffer: none yet             */
	data->is_push_safe = false;

	/* Compute framebuffer stride and total size */
	data->stride_bytes = (uint32_t)cfg->hactive * DSS_BPP;
	data->fb_bytes     = (size_t)data->stride_bytes * cfg->vactive;

	/* Initialize last_vsync_time to 0 (no VSYNC yet) */
	data->last_vsync_time = 0;

	/* Enable functional clock */
	if (!device_is_ready(cfg->func_clk_dev)) {
		LOG_ERR("Functional clock device not ready");
		return -ENODEV;
	}

	/* Enable VP pixel clock */
	if (!device_is_ready(cfg->vp_clk_dev)) {
		LOG_ERR("VP clock device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(cfg->vp_clk_dev, cfg->vp_clk_subsys);
	if (ret != 0) {
		LOG_ERR("clock_control_on(vp_clk) failed: %d", ret);
		return ret;
	}

	ret = clock_control_set_rate(cfg->vp_clk_dev, cfg->vp_clk_subsys,
		 (clock_control_subsys_rate_t)(uintptr_t)(cfg->pixel_clk_hz));
	if (ret != 0) {
		LOG_ERR("clock_control_set_rate(vp_clk) failed: %d", ret);
		return ret;
	}

	ret = clock_control_on(cfg->func_clk_dev, cfg->func_clk_subsys);
	if (ret != 0) {
		LOG_ERR("clock_control_on(func_clk) failed: %d", ret);
		return ret;
	}

	/* Plane (VID) — point hardware at the front buffer */
	dss_plane_setup(dev, data);

	/* Plane FIFO thresholds + MFLAG + CBA priority */
	dss_plane_init(dev);

	/* Overlay — connect video plane -> ovr layer 0 at (0,0) */
	dss_ovr_setup(dev);

	/* VP gamma shadow enable + linear LUT. */
	dss_vp_init(dev);
	dss_vp_setup_gamma(dev);

	/* VP timing registers + enable output */
	dss_vp_enable(dev);

	/* Enable VSYNC interrupts and connect the IRQ line */
	dss_enable_irqs(dev);
	cfg->irq_config_func(dev);

	data->pixel_format = PIXEL_FORMAT_ARGB_8888;
	data->blanking     = false;

	LOG_INF("TIDSS ready: %ux%u @ %u Hz (%u-bit bus) [double-buffer VSYNC]",
		cfg->hactive, cfg->vactive, cfg->pixel_clk_hz, cfg->data_width);

	return 0;
}

#define TIDSS_IRQ_CONFIGURE(n)                                                     \
	static void tidss_irq_configure_##n(const struct device *dev)              \
	{                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n),                                       \
			    DT_INST_IRQ(n, priority),                              \
			    tidss_isr,                                             \
			    DEVICE_DT_INST_GET(n),                                 \
			    0);                                                    \
		irq_enable(DT_INST_IRQN(n));                                       \
	}

#define TIDSS_DEVICE_DEFINE(n)                                                     \
	TIDSS_IRQ_CONFIGURE(n);                                                     \
	COND_CODE_1(DT_INST_PINCTRL_HAS_IDX(n, 0),                                \
		    (PINCTRL_DT_INST_DEFINE(n);), ())                              \
                                                                                   \
	static struct tidss_data tidss_data_##n = {                                \
		.pixel_format = PIXEL_FORMAT_ARGB_8888,                            \
		.blanking     = true,                                              \
	};                                                                         \
                                                                                   \
	static const struct tidss_config tidss_config_##n = {                      \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(common, DT_DRV_INST(n)),        \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(vid, DT_DRV_INST(n)),           \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(ovr, DT_DRV_INST(n)),           \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(vp, DT_DRV_INST(n)),            \
		COND_CODE_1(DT_INST_PINCTRL_HAS_IDX(n, 0),                         \
			    (.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(n),),    \
			    ())                                                  \
		.func_clk_dev    = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, fclk)),   \
		.func_clk_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, fclk,   \
			name), \
		.vp_clk_dev    = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, vp)),       \
		.vp_clk_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, vp, name), \
		.vp_idx      = DT_INST_PROP(n, ti_vp_index),                       \
		.vid_channel = DT_INST_PROP(n, ti_vid_channel),                    \
		.pixel_clk_hz = DT_PROP(DT_INST_CHILD(n, display_timings),        \
					clock_frequency),                          \
                                                                                   \
		.hactive = DT_INST_PROP(n, width),                                 \
		.vactive = DT_INST_PROP(n, height),                                \
		.hfp  = DT_PROP(DT_INST_CHILD(n, display_timings), hfront_porch), \
		.hbp  = DT_PROP(DT_INST_CHILD(n, display_timings), hback_porch),  \
		.hsync = DT_PROP(DT_INST_CHILD(n, display_timings), hsync_len),   \
		.vfp  = DT_PROP(DT_INST_CHILD(n, display_timings), vfront_porch), \
		.vbp  = DT_PROP(DT_INST_CHILD(n, display_timings), vback_porch),  \
		.vsync = DT_PROP(DT_INST_CHILD(n, display_timings), vsync_len),   \
                                                                                   \
		.ivs = 1 - DT_PROP(DT_INST_CHILD(n, display_timings),             \
				    vsync_active),                                 \
		.ihs = 1 - DT_PROP(DT_INST_CHILD(n, display_timings),             \
				    hsync_active),                                 \
                                                                                   \
		.data_width = DT_INST_PROP(n, data_width),                         \
                                                                                   \
		.irq_config_func = tidss_irq_configure_##n,                        \
	};                                                                         \
                                                                                   \
	DEVICE_DT_INST_DEFINE(n,                                                   \
			      tidss_init,                                          \
			      NULL,                                                \
			      &tidss_data_##n,                                     \
			      &tidss_config_##n,                                   \
			      POST_KERNEL,                                         \
			      CONFIG_DISPLAY_INIT_PRIORITY,                        \
			      &tidss_api);

DT_INST_FOREACH_STATUS_OKAY(TIDSS_DEVICE_DEFINE)
