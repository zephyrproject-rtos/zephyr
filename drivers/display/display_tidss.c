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
#define VP_IRQ_VSYNC_EVEN               BIT(1)  /* start of VSYNC pulse       */
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
#define VID_ATTR_ENABLE_BIT             0       /* [0]    plane enable        */
#define VID_ATTR_FORMAT_START           6       /* [6:1]  pixel format        */
#define VID_ATTR_FORMAT_END             1
#define VID_ATTR_CSC_EN_BIT             9       /* [9]    CSC enable          */
#define VID_ATTR_BUFPRELOAD_BIT         19      /* [19]   0 = preload mode    */
#define VID_ATTR_PREMULTIPLYALPHA_BIT   28      /* [28]   premultiplualpha data color component enable      */

/*  OVR (overlay) register offsets  (base = "ovr" region) */
#define DSS_OVR_CONFIG       		0x0 /* layer n attributes */
#define DSS_OVR_DEFAULT_COLOR           0x08
#define DSS_OVR_DEFAULT_COLOR2          0x0c
#define DSS_OVR_TRANS_COLOR_MAX         0x10
#define DSS_OVR_ATTRIBUTES(n)           (0x20 + (n) * 4)

/* DSS_OVR_ATTRIBUTES(layer) bit fields */
#define OVR_ATTR_ENABLE_BIT             0       /* [0]    layer enable        */
#define OVR_ATTR_CHANNELIN_START        4       /* [4:1]  source VID channel  */
#define OVR_ATTR_CHANNELIN_END          1
#define OVR_ATTR_XPOS_START             17      /* [17:6] X position          */
#define OVR_ATTR_XPOS_END               6
#define OVR_ATTR_YPOS_START             30      /* [30:19] Y position         */
#define OVR_ATTR_YPOS_END               19

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
#define VP_CTRL_ENABLE_BIT              0       /* [0]      VP output enable  */
#define VP_CTRL_GO_BIT                  5       /* [5]      shadow latch      */
#define VP_CTRL_DPIENABLE_BIT		6
#define VP_CTRL_DATALINES_START         10      /* [10:8]   data line count   */
#define VP_CTRL_DATALINES_END           8

/* DSS_VP_CONFIG bit field */
#define VP_CFG_GAMMA_SHADOW_BIT         2       /* [2] enable gamma shadow reg*/

/*  Pixel format code for ARGB8888 */
#define DSS_FORMAT_ARGB8888             0x7

/* Gamma LUT size for 8-bit gamma */
#define DSS_GAMMA_SIZE                  256

/* Bytes per pixel for ARGB8888 (matches Zephyr PIXEL_FORMAT_ARGB_8888) */
#define DSS_BPP                         4

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
};

/* Driver runtime data */
struct tidss_data {
	/* MMIO bases for each DSS sub-region */
	DEVICE_MMIO_NAMED_RAM(common);
	DEVICE_MMIO_NAMED_RAM(vid);      /* one video plane (VID or VIDL)   */
	DEVICE_MMIO_NAMED_RAM(ovr);      /* one overlay                     */
	DEVICE_MMIO_NAMED_RAM(vp);       /* one video port                  */

	/* Base address of the framebuffer */
	uintptr_t fb_base_addr;

	/* Synchronisation for tear-free flips */
	struct k_sem vsync_sem;
	bool         flip_pending;

	enum display_pixel_format pixel_format;
	bool blanking;
};

#define DEV_CFG(dev)     ((const struct tidss_config *)dev->config)
#define DEV_DATA(dev)     ((struct tidss_data *)dev->data)

/*  Bit-field manipulation helpers  (mirrors U-Boot FLD_* macros) */
static inline uint32_t fld_mask(uint32_t start, uint32_t end)
{
	return ((1U << (start - end + 1U)) - 1U) << end;
}

static inline uint32_t fld_val(uint32_t val, uint32_t start, uint32_t end)
{
	return (val << end) & fld_mask(start, end);
}

static inline uint32_t fld_get(uint32_t val, uint32_t start, uint32_t end)
{
	return (val & fld_mask(start, end)) >> end;
}

static inline uint32_t fld_mod(uint32_t orig, uint32_t val,
				uint32_t start, uint32_t end)
{
	return (orig & ~fld_mask(start, end)) | fld_val(val, start, end);
}

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
				uint32_t val, uint32_t s, uint32_t e)
{
	dss_write(base, off, fld_mod(dss_read(base, off), val, s, e));
}

#define common_write(dev, off, v)           dss_write((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, common), off, v)
#define common_read(dev, off)               dss_read((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, common), off)
#define common_fld_mod(dev, off, v, s, e)   dss_fld_mod((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, common), off, v, s, e)

#define vid_write(dev, off, v)              dss_write((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, vid), off, v)
#define vid_read(dev, off)                  dss_read((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, vid), off)
#define vid_fld_mod(dev, off, v, s, e)      dss_fld_mod((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, vid), off, v, s, e)

#define ovr_write(dev, off, v)              dss_write((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, ovr), off, v)
#define ovr_fld_mod(dev, off, v, s, e)      dss_fld_mod((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, ovr), off, v, s, e)

#define vp_write(dev, off, v)               dss_write((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, vp), off, v)
#define vp_read(dev, off)                   dss_read((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, vp), off)
#define vp_fld_mod(dev, off, v, s, e)       dss_fld_mod((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, vp), off, v, s, e)

/*  GO bit helpers */
static void dss_vp_go(const struct device *dev)
{
	vp_fld_mod(dev, DSS_VP_CONTROL, 1U, VP_CTRL_GO_BIT, VP_CTRL_GO_BIT);
}

static bool dss_vp_go_busy(const struct device *dev)
{
	return (vp_read(dev, DSS_VP_CONTROL) & BIT(VP_CTRL_GO_BIT)) != 0U;
}

/*
 *  Shadow framebuffer base-address update
 *
 *  Programs both ping (BA_0) and pong (BA_1) shadow registers to the
 *  same physical address.  Takes effect at the next VFP after GO is set.
 */
 static void dss_vid_set_fb(const struct device *dev, struct tidss_data *data)
{
	/* Get the physical address of the frame buffer */
	uintptr_t phys = k_mem_phys_addr((void *)data->fb_base_addr);

	vid_write(dev, DSS_VID_BA_0,     (uint32_t)(phys & 0xFFFFFFFFU));
	vid_write(dev, DSS_VID_BA_EXT_0, (uint32_t)(phys >> 32U));
	vid_write(dev, DSS_VID_BA_1,     (uint32_t)(phys & 0xFFFFFFFFU));
	vid_write(dev, DSS_VID_BA_EXT_1, (uint32_t)(phys >> 32U));
}

/* VSYNC interrupt enable */
static void dss_enable_irqs(const struct device *dev)
{
	const struct tidss_config *cfg = DEV_CFG(dev);
	uint32_t vp_irq_bit = BIT(cfg->vp_idx);

	/* Enable VP VSYNC (both fields) and SYNC_LOST for error detection */
	common_write(dev, DSS_VP_IRQENABLE(cfg->vp_idx),
		     VP_IRQ_VSYNC_EVEN | VP_IRQ_VSYNC_ODD | VP_IRQ_SYNC_LOST);
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
	dss_vid_set_fb(dev, data);

	vid_fld_mod(dev, DSS_VID_ATTRIBUTES,
		    DSS_FORMAT_ARGB8888, VID_ATTR_FORMAT_START, VID_ATTR_FORMAT_END);

	vid_write(dev, DSS_VID_PICTURE_SIZE,
		  fld_val(DEV_CFG(dev)->hactive - 1U, 11, 0) |
		  fld_val(DEV_CFG(dev)->vactive - 1U, 27, 16));

	vid_write(dev, DSS_VID_GLOBAL_ALPHA, 0xFFU);
	vid_fld_mod(dev, DSS_VID_ATTRIBUTES, 0U, VID_ATTR_CSC_EN_BIT,    VID_ATTR_CSC_EN_BIT);
	vid_fld_mod(dev, DSS_VID_ATTRIBUTES, 0U, VID_ATTR_BUFPRELOAD_BIT, VID_ATTR_BUFPRELOAD_BIT);
	vid_fld_mod(dev, DSS_VID_ATTRIBUTES, 0U, VID_ATTR_PREMULTIPLYALPHA_BIT,   VID_ATTR_PREMULTIPLYALPHA_BIT);
	vid_fld_mod(dev, DSS_VID_ATTRIBUTES, 1U, VID_ATTR_ENABLE_BIT,    VID_ATTR_ENABLE_BIT);
}


/*  Plane FIFO thresholds and MFLAG (memory flag / QoS) configuration */
static void dss_plane_init(const struct device *dev)
{
	uint32_t size, thr_high, thr_low, mflag_high, mflag_low;

	common_fld_mod(dev, DSS_CBA_CFG, 1U, 2, 0);
	common_fld_mod(dev, DSS_CBA_CFG, 0U, 5, 3);

	common_fld_mod(dev, DSS_GLOBAL_MFLAG_ATTRIBUTE, 2U, 1, 0);
	common_fld_mod(dev, DSS_GLOBAL_MFLAG_ATTRIBUTE, 0U, 6, 6);

	size = fld_get(vid_read(dev, DSS_VID_BUF_SIZE_STATUS), 15, 0);
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
		  fld_val(thr_high, 31, 16) | fld_val(thr_low, 15, 0));

	vid_write(dev, DSS_VID_MFLAG_THRESHOLD,
		  fld_val(mflag_high, 31, 16) | fld_val(mflag_low, 15, 0));

	vid_write(dev, DSS_VID_PRELOAD, thr_low);
}

/*  Overlay setup */
static void dss_ovr_setup(const struct device *dev)
{
	const struct tidss_config *cfg = DEV_CFG(dev);
	uint32_t attr = 0;

	attr = fld_mod(attr, cfg->vid_channel, OVR_ATTR_CHANNELIN_START,
		       OVR_ATTR_CHANNELIN_END);
	attr = fld_mod(attr, 0U, OVR_ATTR_XPOS_START, OVR_ATTR_XPOS_END);
	attr = fld_mod(attr, 0U, OVR_ATTR_YPOS_START, OVR_ATTR_YPOS_END);
	attr = fld_mod(attr, 1U, OVR_ATTR_ENABLE_BIT,  OVR_ATTR_ENABLE_BIT);


	ovr_write(dev, DSS_OVR_ATTRIBUTES(0), attr);
	ovr_write(dev, DSS_OVR_DEFAULT_COLOR,  0x00000000U);
	ovr_write(dev, DSS_OVR_DEFAULT_COLOR2, 0x00000000U);

}

/*  VP data-line count  (DSS_VP_CONTROL[10:8]) */
static void dss_vp_set_num_datalines(const struct device *dev)
{
	uint32_t v;

	switch (DEV_CFG(dev)->data_width) {
	case 12: v = 0U; break;
	case 16: v = 1U; break;
	case 18: v = 2U; break;
	case 24: v = 3U; break;
	case 30: v = 4U; break;
	case 36: v = 5U; break;
	default:
		LOG_WRN("Unsupported data_width %u; defaulting to 24-bit", DEV_CFG(dev)->data_width);
		v = 3U;
		break;
	}

	vp_fld_mod(dev, DSS_VP_CONTROL, v, VP_CTRL_DATALINES_START, VP_CTRL_DATALINES_END);
}

/*  VP timing registers and output enable */
static void dss_vp_enable(const struct device *dev)
{
	dss_vp_set_num_datalines(dev);

	vp_write(dev, DSS_VP_TIMING_H,
		 fld_val(DEV_CFG(dev)->hsync - 1U, 7,  0)  |
		 fld_val(DEV_CFG(dev)->hfp   - 1U, 19, 8)  |
		 fld_val(DEV_CFG(dev)->hbp   - 1U, 31, 20));

	vp_write(dev, DSS_VP_TIMING_V,
		 fld_val(DEV_CFG(dev)->vsync - 1U, 7,  0)  |
		 fld_val(DEV_CFG(dev)->vfp,        19, 8)  |
		 fld_val(DEV_CFG(dev)->vbp,        31, 20));

	vp_write(dev, DSS_VP_POL_FREQ,
		 fld_val(1U,                    18, 18) |
		 fld_val(1U,                    17, 17) |
		 fld_val(1U,                    16, 16) |
		 fld_val(0U,                    15, 15) |
		 fld_val(0U,                    14, 14) |
		 fld_val(DEV_CFG(dev)->ihs,   13, 13) |
		 fld_val(DEV_CFG(dev)->ivs,   12, 12));

	vp_write(dev, DSS_VP_SIZE_SCREEN,
		 fld_val(DEV_CFG(dev)->hactive - 1U, 11, 0)  |
		 fld_val(DEV_CFG(dev)->vactive - 1U, 27, 16));

	vp_fld_mod(dev, DSS_VP_CONTROL, 1U, VP_CTRL_DPIENABLE_BIT, VP_CTRL_DPIENABLE_BIT);
	vp_fld_mod(dev, DSS_VP_CONTROL, 1U, VP_CTRL_ENABLE_BIT, VP_CTRL_ENABLE_BIT);

}

/*  VP gamma shadow register enable  (DSS_VP_CONFIG[2]) */
static void dss_vp_init(const struct device *dev)
{
	vp_fld_mod(dev, DSS_VP_CONFIG, 1U, VP_CFG_GAMMA_SHADOW_BIT, VP_CFG_GAMMA_SHADOW_BIT);
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

	if (!(vp_stat & (VP_IRQ_VSYNC_EVEN | VP_IRQ_VSYNC_ODD))) {
		LOG_ERR("VP0 IRQ without VSYNC? stat=0x%08x", vp_stat);
		return;
	}


	if (!data->flip_pending) {
		return;
	}

	/*
	 * Missed-VFP race: GO is still set so the latch has not happened yet.
	 * It will happen at the next VFP; defer to the following VSYNC.
	 */
	if (dss_vp_go_busy(dev)) {
		LOG_ERR("VSYNC: GO still set — latch deferred to next frame");
		return;
	}

	data->flip_pending = false;

	k_sem_give(&data->vsync_sem);
}

/* Write pixel data to the display. */
static int tidss_write(const struct device *dev,
		       const uint16_t x, const uint16_t y,
		       const struct display_buffer_descriptor *desc,
		       const void *buf)
{
	const struct tidss_config *cfg  = DEV_CFG(dev);
	struct tidss_data         *data = DEV_DATA(dev);
	unsigned int   irq_key;

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

	/* Cache flush of the frame buffer region. */
	sys_cache_data_flush_range((void *)buf,
				   (size_t)desc->height *
				   (size_t)desc->pitch * DSS_BPP);

	/*Set the frame buffer base address. */
	data->fb_base_addr = (uintptr_t)buf;

	irq_key = irq_lock();
	dss_vid_set_fb(dev, data);
	dss_vp_go(dev);
	data->flip_pending = true;
	irq_unlock(irq_key);

	/* Block until the ISR confirms the latch */
	k_sem_take(&data->vsync_sem, K_FOREVER);
	return 0;
}

/* Disable VP output (blanking on). */
static int tidss_blanking_on(const struct device *dev)
{
	struct tidss_data         *data = DEV_DATA(dev);

	vp_fld_mod(dev, DSS_VP_CONTROL, 0U, VP_CTRL_ENABLE_BIT, VP_CTRL_ENABLE_BIT);
	data->blanking = true;
	return 0;
}

/* Re-enable VP output (blanking off). */
static int tidss_blanking_off(const struct device *dev)
{
	struct tidss_data         *data = DEV_DATA(dev);

	vp_fld_mod(dev, DSS_VP_CONTROL, 1U, VP_CTRL_ENABLE_BIT, VP_CTRL_ENABLE_BIT);
	data->blanking = false;
	return 0;
}


/* Return a pointer to the framebuffer for direct CPU access. */
static void *tidss_get_framebuffer(const struct device *dev)
{
	struct tidss_data *data = DEV_DATA(dev);

	return (void *)data->fb_base_addr;
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

static const struct display_driver_api tidss_api = {
	.blanking_on      = tidss_blanking_on,
	.blanking_off     = tidss_blanking_off,
	.write            = tidss_write,
	.get_framebuffer  = tidss_get_framebuffer,
	.get_capabilities = tidss_get_capabilities,
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

	ret = pinctrl_apply_state(cfg->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("failed to apply pinctrl");
		return ret;
	}

	k_sem_init(&data->vsync_sem, 0, 1);

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

	ret = clock_control_set_rate(cfg->vp_clk_dev, cfg->vp_clk_subsys, (clock_control_subsys_rate_t)(uintptr_t)(cfg->pixel_clk_hz));
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

	dss_vp_go(dev);

	data->pixel_format = PIXEL_FORMAT_ARGB_8888;
	data->blanking     = false;

	/* 9. Enable VSYNC interrupts and connect the IRQ line */
	dss_enable_irqs(dev);
	cfg->irq_config_func(dev);

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
	PINCTRL_DT_INST_DEFINE(n);                                                  \
                                                                                   \
	static struct tidss_data tidss_data_##n = {                                \
		.flip_pending = false,                                             \
		.pixel_format = PIXEL_FORMAT_ARGB_8888,                            \
		.blanking     = true,                                              \
	};                                                                         \
                                                                                   \
	static const struct tidss_config tidss_config_##n = {                      \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(common, DT_DRV_INST(n)),        \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(vid, DT_DRV_INST(n)),           \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(ovr, DT_DRV_INST(n)),           \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(vp, DT_DRV_INST(n)),            \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                      \
		.func_clk_dev    = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, fclk)),   \
		.func_clk_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, fclk, name), \
		.vp_clk_dev    = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, vp)),       \
		.vp_clk_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, vp, name),    \
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

