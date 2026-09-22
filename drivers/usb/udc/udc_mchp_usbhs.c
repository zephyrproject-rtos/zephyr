/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Arkadiusz Grzelka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * USB device controller for the PIC32CZ CA "USBHS" peripheral.
 *
 * The register map is a Mentor Graphics Inventra (MUSB) core wrapped in a
 * Microchip front end: the wrapper owns the low offsets (CTRLA, SYNCBUSY,
 * STATUS, INTFLAG), the MUSB core starts at +0x1000 and a PHY control block
 * sits at +0x1500. usbhs_registers_t is a union of two views of the same
 * addresses - ENDPOINT0 names the registers as they read when INDEX selects
 * endpoint 0, ENDPOINTX as they read for endpoints 1 and up. Everything
 * outside the indexed window (FADDR, POWER, INTRTX, the wrapper, the PHY) is
 * identical in both, and this driver reaches it through the ENDPOINT0 view.
 *
 * Scope: device role only, PIO only. The DMA engine the core reports in
 * RAMINFO is not used, host mode and OTG role switching are not implemented,
 * and the device identity is forced to B-plug in software so no pad or strap
 * is consulted. Both bus speeds are implemented; which one is advertised
 * comes from the devicetree maximum-speed property.
 *
 * See docs/specs/013-ca90-usbhs-udc.md for the bring-up sequence, its
 * sources, and the two errata this file works around.
 */

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_ch9.h>

#include <string.h>

#include "udc_common.h"

LOG_MODULE_REGISTER(udc_mchp_usbhs, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT microchip_pic32cz_usbhs

/* Endpoint-to-bitmap index mapping, as in the sibling G3 driver: OUT
 * endpoints occupy 0-15 and IN endpoints 16-31 of a single 32-bit word.
 */
#define USBHS_EP_IN_OFFSET 16U

#define USBHS_TIMEOUT_SYNCBUSY_US 1000U

/*
 * How long the PHY may take to answer. Measured on a PIC32CZ CA90 Curiosity
 * Ultra by driving the enable sequence over SWD and polling STATUS: PHYON
 * asserts within a couple of milliseconds, PHYRDY follows another 10 ms or so
 * later. A 10 ms bound sat right on that edge and failed the poll on a part
 * whose PHY was coming up perfectly well, so the bound is an order of
 * magnitude clear of the measurement. It is only ever waited out on a real
 * failure.
 */
#define USBHS_TIMEOUT_PHY_US 100000U

/*
 * The additional voltage regulator needs 55 us to settle before any USBHS
 * register may be touched. Round up rather than sit on the published bound.
 */
#define USBHS_AVREG_SETTLE_US 60U

/* Remote wakeup resume signalling: the specification asks for 1 ms to 15 ms. */
#define USBHS_RESUME_HOLD_MS 10U

/*
 * FIFO RAM the core carries, from the datasheet. RAMINFO reports the width of
 * the RAM address bus rather than a byte count, and the pack header's EPINFO
 * reset value is wrong (0xFF against the datasheet's 0x77), so neither is
 * trusted for sizing: the pool is the documented figure and the endpoint
 * count comes from the devicetree.
 */
#define USBHS_FIFO_RAM_BYTES (9U * 1024U)

/* Endpoint 0 keeps a fixed 64-byte FIFO at the bottom of the pool. */
#define USBHS_EP0_FIFO_BYTES USB_CONTROL_EP_MPS

/* FIFO addresses are 13-bit and expressed in units of 8 bytes. */
#define USBHS_FIFO_ADDR_UNIT 8U

/*
 * The largest packet an endpoint may advertise. This is a USB limit, not a
 * FIFO one: udc_ep_config.caps.mps carries a wMaxPacketSize, whose size field
 * is 11 bits wide (USB_MPS_EP_SIZE), and the UDC core compares requests
 * against it through that macro. A driver that advertises the FIFO's 4096
 * there has every endpoint rejected, because 4096 masks to zero and no
 * request is ever small enough - which reads as "Failed to assign endpoint
 * addresses" from the stack and says nothing about why.
 */
#define USBHS_EP_MPS_MAX 1024U

/* Sizes are encoded as log2(bytes) - 3 over the range 8 B to 4096 B. */
#define USBHS_FIFO_SIZE_MIN         8U
#define USBHS_FIFO_SIZE_MAX         4096U
#define USBHS_FIFO_SIZE_LOG2_OFFSET 3U

/* The PHY trim word in the CAL OTP area, datasheet Table 11-6. */
#define USBHS_PHY_TRIM_ADDR 0x0A00718CUL

/*
 * PHY24 bit 1 is OTGPDN (the Harmony driver calls it OTGOFF): it powers down
 * the OTG VBus and session comparators, and both the datasheet and the vendor
 * driver set it in device mode. The Microchip pack header defines no field
 * there - it jumps from bit 0 to bit 2 - and USBHS_PHY24_Msk is 0xFD, which
 * masks the bit out. Code that writes PHY24 through the header mask therefore
 * drops this write and says nothing. The bit is defined here and the header
 * mask is never applied to PHY24.
 */
#define USBHS_PHY24_OTGPDN_Msk BIT(1)

/*
 * SUPC gives each USBHS instance its own additional voltage regulator:
 * instance n is enabled by VREGCTRL.AVREGEN bit 16+n and reports ready in
 * STATUS.ADDVREGRDY bit 8+n. The binding carries no property for the index,
 * so it is derived from the register address - the two instances sit at
 * 0x4F010000 and 0x4F012000.
 */
#define USBHS_INSTANCE_BASE   0x4F010000UL
#define USBHS_INSTANCE_STRIDE 0x2000UL

/*
 * SUPC.VREGCTRL and SUPC.STATUS are not per-instance registers: both USBHS
 * instances own one bit each of the same two words. A read-modify-write on
 * VREGCTRL from one instance therefore reads and writes the other's AVREGEN
 * bit as well, and two instances initialising or shutting down concurrently
 * can each drop the other's regulator. The lock is driver-wide for that
 * reason; a per-instance lock would serialise nothing.
 */
static struct k_spinlock usbhs_supc_lock;

/*
 * Which FIFO slot an endpoint address owns. IN and OUT of the same index are
 * separate allocations because the core gives them separate FIFOs.
 */
#define USBHS_FIFO_SLOTS(num_eps) ((num_eps) * 2U)

struct usbhs_fifo_block {
	uint16_t offset; /* Byte offset into the FIFO RAM. */
	uint16_t size;   /* Allocated bytes, zero when the slot is free. */
};

/*
 * One row of the CAL-OTP PHY trim map, datasheet Table 11-6. The datasheet
 * carries the note that these values must be loaded from the CAL OTP area
 * into the PHY registers before the USB is enabled to reach the specified
 * accuracy. The Harmony 3 usbhsv2 driver never does it; that is treated here
 * as a gap in the vendor driver rather than permission to skip the step.
 */
struct usbhs_phy_trim {
	uint16_t reg_off; /* Offset of the PHY register from the peripheral base. */
	uint8_t src_lsb;  /* First bit of the field in the OTP word. */
	uint8_t width;    /* Field width in bits. */
	uint8_t dst_lsb;  /* First bit of the field in the PHY register. */
};

static const struct usbhs_phy_trim usbhs_phy_trim_map[] = {
	{0x1504, 0, 3, 5},  /* PHY04[7:5] RxSQUELCH[2:0] */
	{0x1508, 3, 1, 0},  /* PHY08[0]   RxSQUELCH[3]   */
	{0x150C, 4, 3, 5},  /* PHY0C[7:5] TUNE[2:0]      */
	{0x1510, 7, 5, 0},  /* PHY10[4:0] TUNE[7:3]      */
	{0x1514, 12, 1, 7}, /* PHY14[7]   ODT[0]         */
	{0x1518, 13, 2, 0}, /* PHY18[1:0] ODT[2:1]       */
	{0x1520, 15, 2, 6}, /* PHY20[7:6] HSSLEW[1:0]    */
	{0x1524, 17, 1, 0}, /* PHY24[0]   HSSLEW[2]      */
	{0x1528, 18, 4, 1}, /* PHY28[4:1] DISCONDET[3:0] */
	{0x1528, 22, 3, 5}, /* PHY28[7:5] HSDRVCOMP[2:0] */
};

/*
 * Static configuration of one USBHS instance: the register window, the
 * endpoint tables, the peripheral clock, and the per-instance IRQ and thread
 * helpers the devicetree instantiation generates.
 */
struct udc_usbhs_config {
	usbhs_registers_t *base;
	size_t num_of_eps;               /* Bidirectional endpoints, EP0 included. */
	struct udc_ep_config *ep_cfg_in; /* IN endpoint configuration array. */
	struct udc_ep_config *ep_cfg_out;
	struct usbhs_fifo_block *fifo; /* FIFO allocator slots, 2 per endpoint. */
	uint8_t avreg_idx;             /* Additional regulator index in SUPC. */
	bool high_speed;               /* maximum-speed = "high-speed". */
	/*
	 * Board switches on the connector, both optional: a port is absent
	 * when its spec carries no device.
	 */
	struct gpio_dt_spec drd;
	struct gpio_dt_spec vbus_enable;
	struct {
		const struct device *clock_dev;
		clock_control_subsys_t mclk_subsys;
	} clock;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	void (*make_thread)(const struct device *dev);
};

enum usbhs_event_type {
	USBHS_EVT_SETUP,         /* A SETUP packet is waiting in priv->setup. */
	USBHS_EVT_SETUP_ABORT,   /* The core reported SetupEnd; the stage is void. */
	USBHS_EVT_XFER_NEW,      /* A transfer was queued on an idle endpoint. */
	USBHS_EVT_XFER_FINISHED, /* A transfer completed and needs submitting. */
};

/*
 * Stage of the control transfer in flight. The core does not track this for
 * us: DataEnd has to ride with the last data packet, and which packet that is
 * depends on the direction and length the SETUP asked for.
 */
enum usbhs_ep0_stage {
	USBHS_EP0_SETUP,    /* Waiting for the next SETUP packet. */
	USBHS_EP0_DATA_IN,  /* Sending the data stage of a control IN. */
	USBHS_EP0_DATA_OUT, /* Receiving the data stage of a control OUT. */
	USBHS_EP0_STATUS,   /* DataEnd is set; the core runs the status stage. */
};

/*
 * How far the bring-up sequence got. Every step that has to be undone gets a
 * value, in the order the steps run, so the unwind is a fall-through from
 * wherever the sequence stopped. Anything below the stage reached is untouched
 * hardware, and touching it - a CTRLA write into an unclocked window, say - is
 * a bus fault rather than a tidy-up.
 */
enum usbhs_stage {
	USBHS_STAGE_NONE = 0, /* Nothing has been done yet. */
	USBHS_STAGE_CONNECTOR,
	USBHS_STAGE_REGULATOR,
	USBHS_STAGE_CLOCK,
	USBHS_STAGE_ENABLED, /* CTRLA.ENABLE is set. */
};

/*
 * Runtime state of one instance. INDEX is a shared selector for the per
 * endpoint registers, so every path that writes it takes lock and restores
 * the previous value; the FIFO allocator is under the same lock.
 */
struct udc_usbhs_data {
	struct k_thread thread_data;
	struct k_event events;
	struct k_spinlock lock;
	atomic_t xfer_new;
	atomic_t xfer_finished;
	uint8_t setup[sizeof(struct usb_setup_packet)];
	/* How far init() got, so shutdown() undoes exactly that much. */
	enum usbhs_stage stage;
	enum usbhs_ep0_stage ep0_stage;
	uint16_t ep0_len; /* wLength of the control transfer in flight. */
	/*
	 * The status stage runs in hardware and the stack still queues a
	 * zero-length buffer for it. Whichever of the two arrives second
	 * completes that buffer, so both directions of the race are recorded.
	 */
	bool ep0_status_done;
};

/* Converts an endpoint address into its bit in the event bitmaps. */
static uint8_t usbhs_ep_to_bnum(const uint8_t ep)
{
	uint8_t idx = USB_EP_GET_IDX(ep);

	return (USB_EP_GET_DIR(ep) == USB_EP_DIR_IN) ? (USBHS_EP_IN_OFFSET + idx) : idx;
}

/*
 * Returns one pending endpoint address from an event bitmap and clears its
 * bit. The lowest set bit is taken first.
 */
static uint8_t usbhs_pull_ep_from_bmsk(uint32_t *const bitmap)
{
	unsigned int bit_idx;

	__ASSERT_NO_MSG(bitmap && *bitmap);

	bit_idx = find_lsb_set(*bitmap) - 1U;
	*bitmap &= ~BIT(bit_idx);

	if (bit_idx >= USBHS_EP_IN_OFFSET) {
		return USB_EP_DIR_IN | (uint8_t)(bit_idx - USBHS_EP_IN_OFFSET);
	}

	return USB_EP_DIR_OUT | (uint8_t)bit_idx;
}

/* The MUSB core registers, named as they read with INDEX selecting EP0. */
static inline usbhs_endpoint0_registers_t *usbhs_core(const struct device *dev)
{
	const struct udc_usbhs_config *cfg = dev->config;

	return &cfg->base->ENDPOINT0;
}

/* The same window, named as it reads with INDEX selecting endpoints 1 and up. */
static inline usbhs_endpointx_registers_t *usbhs_epx(const struct device *dev)
{
	const struct udc_usbhs_config *cfg = dev->config;

	return &cfg->base->ENDPOINTX;
}

/*
 * Every access to the PHY subspace needs a dummy read of STATUS after it, per
 * errata 2.14.2. Keeping that in one accessor pair is the only way to stop a
 * later refactor losing it silently, so nothing else in this file touches a
 * PHY register directly.
 */
static uint32_t usbhs_phy_read(const struct device *dev, uint16_t reg_off)
{
	const struct udc_usbhs_config *cfg = dev->config;
	volatile uint32_t *reg = (volatile uint32_t *)((uintptr_t)cfg->base + reg_off);
	uint32_t val = *reg;

	(void)cfg->base->ENDPOINT0.USBHS_STATUS;

	return val;
}

static void usbhs_phy_write(const struct device *dev, uint16_t reg_off, uint32_t val)
{
	const struct udc_usbhs_config *cfg = dev->config;
	volatile uint32_t *reg = (volatile uint32_t *)((uintptr_t)cfg->base + reg_off);

	*reg = val;

	(void)cfg->base->ENDPOINT0.USBHS_STATUS;
}

/*
 * Moves a packet into the transmit FIFO of the given endpoint. Word accesses
 * while a whole word remains, then the tail a byte at a time; the source
 * buffer carries no alignment guarantee, so the word is assembled with memcpy
 * rather than dereferenced.
 */
static void usbhs_fifo_write(const struct device *dev, uint8_t idx, const uint8_t *data,
			     uint16_t len)
{
	volatile uint32_t *const fifo = &usbhs_core(dev)->USBHS_FIFOX[idx];
	volatile uint8_t *const fifo8 = (volatile uint8_t *)fifo;

	while (len >= sizeof(uint32_t)) {
		uint32_t word;

		memcpy(&word, data, sizeof(word));
		*fifo = word;
		data += sizeof(uint32_t);
		len -= sizeof(uint32_t);
	}

	while (len != 0U) {
		*fifo8 = *data++;
		len--;
	}
}

/* The reverse: drains len bytes of the packet at the head of the receive FIFO. */
static void usbhs_fifo_read(const struct device *dev, uint8_t idx, uint8_t *data, uint16_t len)
{
	volatile uint32_t *const fifo = &usbhs_core(dev)->USBHS_FIFOX[idx];
	volatile uint8_t *const fifo8 = (volatile uint8_t *)fifo;

	while (len >= sizeof(uint32_t)) {
		uint32_t word = *fifo;

		memcpy(data, &word, sizeof(word));
		data += sizeof(uint32_t);
		len -= sizeof(uint32_t);
	}

	while (len != 0U) {
		*data++ = *fifo8;
		len--;
	}
}

/* Drops bytes the caller's buffer had no room for, so the FIFO stays in step. */
static void usbhs_fifo_discard(const struct device *dev, uint8_t idx, uint16_t len)
{
	volatile uint8_t *const fifo8 = (volatile uint8_t *)&usbhs_core(dev)->USBHS_FIFOX[idx];

	while (len != 0U) {
		(void)*fifo8;
		len--;
	}
}

/*
 * Flushing a FIFO. The Inventra core only acts on FlushFIFO while the
 * corresponding PktRdy is set - a write with it clear does nothing at all -
 * and the write has to carry PktRdy with it. A double-buffered FIFO holds two
 * packets and discards one per write, so the sequence runs up to twice and
 * stops as soon as PktRdy has gone.
 */
#define USBHS_FIFO_FLUSH_PASSES 2U

/* Endpoint 0 shares one single-buffered FIFO, so one pass covers it. */
static void usbhs_ep0_flush(const struct device *dev)
{
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);

	if ((core->USBHS_CSR0L &
	     (USBHS_ENDPOINT0_CSR0L_RXPKTRDY_Msk | USBHS_ENDPOINT0_CSR0L_TXPKTRDY_Msk)) == 0U) {
		return;
	}

	core->USBHS_CSR0H = USBHS_ENDPOINT0_CSR0H_FLUSHFIFO_Msk;
}

/* Called with INDEX already on the endpoint. */
static void usbhs_ep_tx_flush(const struct device *dev)
{
	usbhs_endpointx_registers_t *const epx = usbhs_epx(dev);

	for (uint8_t pass = 0U; pass < USBHS_FIFO_FLUSH_PASSES; pass++) {
		if ((epx->USBHS_TXCSRL & USBHS_ENDPOINTX_TXCSRL_TXPKTRDY_Msk) == 0U) {
			return;
		}

		epx->USBHS_TXCSRL =
			USBHS_ENDPOINTX_TXCSRL_TXPKTRDY_Msk | USBHS_ENDPOINTX_TXCSRL_FLUSHFIFO_Msk;
	}
}

/* Called with INDEX already on the endpoint. */
static void usbhs_ep_rx_flush(const struct device *dev)
{
	usbhs_endpointx_registers_t *const epx = usbhs_epx(dev);

	for (uint8_t pass = 0U; pass < USBHS_FIFO_FLUSH_PASSES; pass++) {
		if ((epx->USBHS_RXCSRL & USBHS_ENDPOINTX_RXCSRL_RXPKTRDY_Msk) == 0U) {
			return;
		}

		epx->USBHS_RXCSRL =
			USBHS_ENDPOINTX_RXCSRL_RXPKTRDY_Msk | USBHS_ENDPOINTX_RXCSRL_FLUSHFIFO_Msk;
	}
}

/* Rounds a maximum packet size up to a FIFO size the core can encode. */
static uint16_t usbhs_fifo_size(uint16_t mps)
{
	uint16_t size = USBHS_FIFO_SIZE_MIN;

	while (size < mps) {
		size *= 2U;
	}

	return size;
}

/* TXFIFOSZ and RXFIFOSZ take log2(bytes) - 3. */
static uint8_t usbhs_fifo_size_encode(uint16_t size)
{
	return (uint8_t)(find_msb_set(size) - 1U - USBHS_FIFO_SIZE_LOG2_OFFSET);
}

/*
 * First-fit allocation out of the FIFO RAM, over the blocks currently in use.
 * The candidate offset is pushed past every block it overlaps and the sweep
 * repeats until it overlaps nothing, which walks it into the first gap large
 * enough.
 *
 * The endpoint's own block is released before the sweep, because otherwise a
 * re-enable without a disable in between would step over its stale allocation
 * and push the new one up the pool. The old block is put back when the new one
 * does not fit, so a failure leaves the allocator exactly as it was.
 *
 * Called with the driver lock held.
 */
static int usbhs_fifo_alloc(const struct device *dev, uint8_t ep, uint16_t size)
{
	const struct udc_usbhs_config *cfg = dev->config;
	const size_t slots = USBHS_FIFO_SLOTS(cfg->num_of_eps);
	uint16_t cand = USBHS_EP0_FIFO_BYTES;
	uint8_t slot = (uint8_t)((USB_EP_GET_IDX(ep) << 1) | (USB_EP_DIR_IS_IN(ep) ? 1U : 0U));
	const struct usbhs_fifo_block prev = cfg->fifo[slot];
	bool moved;

	cfg->fifo[slot].offset = 0U;
	cfg->fifo[slot].size = 0U;

	do {
		moved = false;

		for (size_t i = 0; i < slots; i++) {
			const struct usbhs_fifo_block *blk = &cfg->fifo[i];

			if (blk->size == 0U) {
				continue;
			}

			if ((cand < (blk->offset + blk->size)) && (blk->offset < (cand + size))) {
				cand = blk->offset + blk->size;
				moved = true;
			}
		}
	} while (moved);

	if ((uint32_t)cand + size > USBHS_FIFO_RAM_BYTES) {
		LOG_ERR("No FIFO space for ep 0x%02x (%u bytes)", ep, size);
		cfg->fifo[slot] = prev;
		return -ENOMEM;
	}

	cfg->fifo[slot].offset = cand;
	cfg->fifo[slot].size = size;

	return 0;
}

/* Returns an endpoint's FIFO block to the pool. Called with the lock held. */
static void usbhs_fifo_free(const struct device *dev, uint8_t ep)
{
	const struct udc_usbhs_config *cfg = dev->config;
	uint8_t slot = (uint8_t)((USB_EP_GET_IDX(ep) << 1) | (USB_EP_DIR_IS_IN(ep) ? 1U : 0U));

	cfg->fifo[slot].size = 0U;
	cfg->fifo[slot].offset = 0U;
}

static uint16_t usbhs_fifo_offset(const struct device *dev, uint8_t ep)
{
	const struct udc_usbhs_config *cfg = dev->config;
	uint8_t slot = (uint8_t)((USB_EP_GET_IDX(ep) << 1) | (USB_EP_DIR_IS_IN(ep) ? 1U : 0U));

	return cfg->fifo[slot].offset;
}

/*
 * Loads the PHY trim from the CAL OTP word into the fifteen PHY registers the
 * datasheet lists, one field at a time. Read-modify-write, so fields the map
 * does not name keep their reset values.
 */
static void usbhs_load_phy_trim(const struct device *dev)
{
	uint32_t otp = sys_read32(USBHS_PHY_TRIM_ADDR);

	for (size_t i = 0; i < ARRAY_SIZE(usbhs_phy_trim_map); i++) {
		const struct usbhs_phy_trim *t = &usbhs_phy_trim_map[i];
		uint32_t field_msk = BIT_MASK(t->width);
		uint32_t val = (otp >> t->src_lsb) & field_msk;
		uint32_t reg = usbhs_phy_read(dev, t->reg_off);

		reg &= ~(field_msk << t->dst_lsb);
		reg |= val << t->dst_lsb;

		usbhs_phy_write(dev, t->reg_off, reg);
	}

	LOG_DBG("PHY trim word 0x%08x applied", otp);
}

/* Waits for the wrapper's write synchronisation to finish. */
static int usbhs_wait_syncbusy(const struct device *dev)
{
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);

	if (!WAIT_FOR(core->USBHS_SYNCBUSY == 0U, USBHS_TIMEOUT_SYNCBUSY_US, NULL)) {
		return -ETIMEDOUT;
	}

	return 0;
}

/*
 * Puts the connector into the state a device port has to be in before the
 * PHY comes up. Both pins are optional and a board that needs neither
 * declares neither.
 *
 *   - The Type-C CC pull-downs are not always fitted. Where a board routes
 *     them through an analog switch instead, nothing on the connector says
 *     "device" until that switch is thrown, and no host ever sees an attach.
 *     A pull resistor on the select net gives it a defined level but not
 *     necessarily the right one, so the line is driven rather than left.
 *   - The VBUS switch on the connector must be off. A device that sources
 *     5 V into a host puts two supplies against each other.
 *
 * Which electrical level each of those is comes from the GPIO flags in the
 * devicetree, so no board's wiring is encoded here.
 */
static int usbhs_connector_setup(const struct device *dev)
{
	const struct udc_usbhs_config *cfg = dev->config;
	int ret;

	if (cfg->vbus_enable.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->vbus_enable, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to deassert the VBUS switch: %d", ret);
			return ret;
		}
	}

	if (cfg->drd.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->drd, GPIO_OUTPUT_ACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to select the device role on the connector: %d", ret);
			return ret;
		}
	}

	return 0;
}

/*
 * Releases the connector. The role select goes back to its inactive level;
 * the VBUS switch stays driven off rather than being released, because a
 * floating enable is exactly what must not happen to a switch that could
 * source 5 V into a host.
 */
static void usbhs_connector_release(const struct device *dev)
{
	const struct udc_usbhs_config *cfg = dev->config;

	if (cfg->drd.port != NULL) {
		(void)gpio_pin_set_dt(&cfg->drd, 0);
	}

	if (cfg->vbus_enable.port != NULL) {
		(void)gpio_pin_set_dt(&cfg->vbus_enable, 0);
	}
}

/*
 * Reverses the bring-up as far as it got. Safe to call from any stage,
 * including USBHS_STAGE_NONE, and it never touches a register the stage it is
 * given has not proven to be powered and clocked.
 */
static void usbhs_unwind(const struct device *dev, enum usbhs_stage stage)
{
	const struct udc_usbhs_config *cfg = dev->config;

	if (stage >= USBHS_STAGE_ENABLED) {
		usbhs_core(dev)->USBHS_CTRLA = 0U;
		(void)usbhs_wait_syncbusy(dev);
	}

	if (stage >= USBHS_STAGE_CLOCK) {
		(void)clock_control_off(cfg->clock.clock_dev, cfg->clock.mclk_subsys);
	}

	if (stage >= USBHS_STAGE_REGULATOR) {
		K_SPINLOCK(&usbhs_supc_lock) {
			SUPC_REGS->SUPC_VREGCTRL &=
				~BIT(SUPC_VREGCTRL_AVREGEN_Pos + cfg->avreg_idx);
		}
	}

	if (stage >= USBHS_STAGE_CONNECTOR) {
		usbhs_connector_release(dev);
	}
}

/*
 * Brings the peripheral out of reset, in the order the datasheet and the
 * errata sheet require. *stage reports how far it got, whether it succeeded or
 * not, and is what the caller hands usbhs_unwind(). Step by step:
 *
 *  0. Put the connector into its device-port state, before anything on the
 *     port is powered or clocked.
 *  1. Enable this instance's additional voltage regulator in SUPC and wait
 *     for it, then let 55 us pass before any USBHS register is touched.
 *  2. Enable the peripheral clock. The devicetree names MCLK peripheral
 *     HUSBn, and clock_control_on() sets the matching MCLK.CLKMSK bit;
 *     whether reset leaves it set is unverified, so it is set unconditionally.
 *  3. Check that the board crystal reaches the USB PLL reference. The board
 *     devicetree owns that routing; nothing here can still change it.
 *  4. Dummy read of STATUS, per errata 2.14.2.
 *  5. Software reset, then wait for the wrapper and for the PHY to go down.
 *  6. Load the CAL OTP PHY trim.
 *  7. Select the 12 MHz reference and force a B-plug identity, then enable in
 *     a separate write - every other CTRLA bit is enable-protected.
 *  8. Wait for the PHY to come up.
 *  9. Power down the OTG comparators (PHY24 bit 1), which device mode does not
 *     need and which the pack header cannot express.
 * 10. Give endpoint 0 its 64-byte FIFO.
 *
 * The vendor driver's PLIB_USBHS_SoftResetEnable() writes CTRLA at the MUSB
 * core base rather than the wrapper base, commented as an errata workaround.
 * That errata is not in DS80001023 and an unexplained write to the wrong
 * address is not carried on faith: CTRLA is written at offset 0 here. This
 * note is the record of the alternative, should reset misbehave on silicon.
 */
static int usbhs_bringup(const struct device *dev, enum usbhs_stage *stage)
{
	const struct udc_usbhs_config *cfg = dev->config;
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	uint32_t ctrla;
	int ret;

	*stage = USBHS_STAGE_NONE;

	ret = usbhs_connector_setup(dev);
	if (ret != 0) {
		return ret;
	}

	*stage = USBHS_STAGE_CONNECTOR;

	K_SPINLOCK(&usbhs_supc_lock) {
		SUPC_REGS->SUPC_VREGCTRL |= BIT(SUPC_VREGCTRL_AVREGEN_Pos + cfg->avreg_idx);
	}

	*stage = USBHS_STAGE_REGULATOR;

	if (!WAIT_FOR((SUPC_REGS->SUPC_STATUS & BIT(SUPC_STATUS_ADDVREGRDY_Pos + cfg->avreg_idx)) !=
			      0U,
		      USBHS_TIMEOUT_PHY_US, NULL)) {
		LOG_ERR("Additional voltage regulator %u did not come up", cfg->avreg_idx);
		return -EIO;
	}

	k_busy_wait(USBHS_AVREG_SETTLE_US);

	ret = clock_control_on(cfg->clock.clock_dev, cfg->clock.mclk_subsys);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable the USBHS peripheral clock: %d", ret);
		return ret;
	}

	*stage = USBHS_STAGE_CLOCK;

	/*
	 * XOSCCTRLA.USBHSDIV is what routes the board crystal to the USB PLL,
	 * and the controller has no other reference. It cannot be set from
	 * here: the register is enable-protected and the oscillator is already
	 * running by the time any driver initialises - measured on a PIC32CZ
	 * CA90 Curiosity Ultra, a write of DIV1 over SWD to the running part
	 * reads back unchanged. It comes from the board devicetree instead,
	 * where the clock driver writes it in the same word that enables the
	 * oscillator. Check it rather than assume it, because the symptom of a
	 * missing reference is a CTRLA.SWRST whose SYNCBUSY never clears and
	 * then an imprecise bus error somewhere else entirely.
	 */
	if ((OSCCTRL_REGS->OSCCTRL_XOSCCTRLA & OSCCTRL_XOSCCTRLA_USBHSDIV_Msk) ==
	    OSCCTRL_XOSCCTRLA_USBHSDIV_DIS) {
		LOG_ERR("The XOSC does not drive the USB PLL reference: the board devicetree "
			"needs xosc-usb-ref-clock-div on its XOSC node");
		return -EIO;
	}

	/* Errata 2.14.2: a dummy read before the first real access. */
	(void)core->USBHS_STATUS;

	core->USBHS_CTRLA = USBHS_CTRLA_SWRST_Msk;

	ret = usbhs_wait_syncbusy(dev);
	if (ret != 0) {
		LOG_ERR("Timeout on the software reset");
		return ret;
	}

	if (!WAIT_FOR((core->USBHS_STATUS & USBHS_STATUS_PHYRDY_Msk) == 0U, USBHS_TIMEOUT_PHY_US,
		      NULL)) {
		LOG_ERR("PHY still ready after the software reset");
		return -EIO;
	}

	usbhs_load_phy_trim(dev);

	/*
	 * REFCLKSEL = 0 selects the 12 MHz reference. IDOVEN with IDVAL set
	 * presents a B-plug identity whatever the USBID pad reads, which is
	 * what makes this a fixed-role device and removes the whole role
	 * detection state machine.
	 */
	ctrla = USBHS_CTRLA_IDOVEN_Msk | USBHS_CTRLA_IDVAL_Msk;
	core->USBHS_CTRLA = ctrla;
	core->USBHS_CTRLA = ctrla | USBHS_CTRLA_ENABLE_Msk;
	*stage = USBHS_STAGE_ENABLED;

	ret = usbhs_wait_syncbusy(dev);
	if (ret != 0) {
		LOG_ERR("Timeout enabling the peripheral");
		return ret;
	}

	if (!WAIT_FOR((core->USBHS_STATUS & (USBHS_STATUS_PHYON_Msk | USBHS_STATUS_PHYRDY_Msk)) ==
			      (USBHS_STATUS_PHYON_Msk | USBHS_STATUS_PHYRDY_Msk),
		      USBHS_TIMEOUT_PHY_US, NULL)) {
		LOG_ERR("PHY did not become ready");
		return -EIO;
	}

	usbhs_phy_write(dev, 0x1524, usbhs_phy_read(dev, 0x1524) | USBHS_PHY24_OTGPDN_Msk);

	/*
	 * Endpoint 0 shares one 64-byte FIFO between both directions, at the
	 * bottom of the pool. The allocator starts above it, so this is the
	 * only allocation that is not taken from a slot.
	 */
	core->USBHS_INDEX = 0U;
	core->USBHS_TXFIFOSZ = usbhs_fifo_size_encode(USBHS_EP0_FIFO_BYTES);
	core->USBHS_RXFIFOSZ = usbhs_fifo_size_encode(USBHS_EP0_FIFO_BYTES);
	core->USBHS_TXFIFOADD = 0U;
	core->USBHS_RXFIFOADD = 0U;

	LOG_DBG("EPINFO 0x%02x RAMINFO 0x%02x", core->USBHS_EPINFO, core->USBHS_RAMINFO);

	return 0;
}

/*
 * Writes one packet of a control IN data stage and reports whether that was
 * the last one. DataEnd rides with the last packet, which is what tells the
 * core to run the status stage itself.
 *
 * Called with the driver lock held.
 */
static bool usbhs_ep0_write(const struct device *dev, struct net_buf *const buf)
{
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	uint16_t len = MIN(buf->len, USB_CONTROL_EP_MPS);
	uint8_t csr = USBHS_ENDPOINT0_CSR0L_TXPKTRDY_Msk;
	bool more;

	usbhs_fifo_write(dev, 0U, buf->data, len);
	net_buf_pull(buf, len);

	more = buf->len != 0U;

	/*
	 * A transfer that ends on a full packet and asked for a terminating
	 * zero length packet needs one more, empty, packet before DataEnd.
	 */
	if (!more && (len == USB_CONTROL_EP_MPS) && udc_ep_buf_has_zlp(buf)) {
		udc_ep_buf_clear_zlp(buf);
		more = true;
	}

	if (!more) {
		csr |= USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_DATAEND_Msk;
	}

	core->USBHS_CSR0L = csr;

	return !more;
}

/*
 * Drains one packet of a control OUT data stage and reports whether the
 * transfer is over: a short packet, the length the SETUP asked for, or a full
 * buffer all end it. DataEnd goes out with the acknowledgment of the last
 * packet.
 *
 * Called with the driver lock held.
 */
static bool usbhs_ep0_read(const struct device *dev, struct net_buf *const buf)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	uint16_t count = core->USBHS_COUNT0;
	uint16_t len = MIN(count, net_buf_tailroom(buf));
	uint8_t csr = USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SERVICEDRXPKTRDY_Msk;
	bool done;

	usbhs_fifo_read(dev, 0U, net_buf_tail(buf), len);
	net_buf_add(buf, len);

	if (count > len) {
		LOG_ERR("Control OUT overflow: %u bytes dropped", count - len);
		usbhs_fifo_discard(dev, 0U, count - len);
	}

	done = (count < USB_CONTROL_EP_MPS) || (buf->len >= priv->ep0_len) ||
	       (net_buf_tailroom(buf) == 0U);

	if (done) {
		csr |= USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_DATAEND_Msk;
	}

	core->USBHS_CSR0L = csr;

	return done;
}

/* Marks a transfer finished and wakes the worker thread. */
static void usbhs_xfer_finished(const struct device *dev, uint8_t ep)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);

	atomic_set_bit(&priv->xfer_finished, usbhs_ep_to_bnum(ep));
	k_event_post(&priv->events, BIT(USBHS_EVT_XFER_FINISHED));
}

/*
 * Completes the zero-length buffer the stack queues for a status stage the
 * core has already run. Called both from the interrupt that reports the stage
 * over and from the thread that queues the buffer, whichever comes second.
 *
 * Called with the driver lock held. Returns true when a buffer was matched.
 */
static bool usbhs_ep0_finish_status(const struct device *dev)
{
	static const uint8_t eps[] = {USB_CONTROL_EP_IN, USB_CONTROL_EP_OUT};

	for (size_t i = 0; i < ARRAY_SIZE(eps); i++) {
		struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, eps[i]);
		struct net_buf *buf = udc_buf_peek(ep_cfg);
		struct udc_buf_info *bi;

		if (buf == NULL) {
			continue;
		}

		bi = udc_get_buf_info(buf);
		if (!bi->status) {
			continue;
		}

		usbhs_xfer_finished(dev, eps[i]);
		return true;
	}

	return false;
}

/*
 * Endpoint 0 interrupt. The core raises it when RxPktRdy is set, when
 * TxPktRdy is cleared, when SentStall is set, when SetupEnd is set and when
 * DataEnd is cleared at the end of a status stage.
 *
 * CSR0L mixes write-one-to-act bits with write-zero-to-clear ones, so every
 * write here is a computed value rather than a read-modify-write.
 */
/*
 * Handles a SETUP packet sitting in the FIFO. Split out of the interrupt
 * because a stage that ends inside the same interrupt has to come back here:
 * INTRTX is clear-on-read and was consumed before this function ran, so a
 * SETUP that arrived alongside the end of a status stage raises no further
 * interrupt of its own and would be lost until the next bus reset.
 *
 * Called with the driver lock held and INDEX on endpoint 0.
 */
static void usbhs_ep0_setup(const struct device *dev)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	const struct usb_setup_packet *setup;
	uint8_t csr = USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SERVICEDRXPKTRDY_Msk;

	if (core->USBHS_COUNT0 != sizeof(struct usb_setup_packet)) {
		LOG_ERR("SETUP packet is %u bytes", core->USBHS_COUNT0);
		core->USBHS_CSR0L = USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SERVICEDRXPKTRDY_Msk |
				    USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SENDSTALL_Msk;
		return;
	}

	usbhs_fifo_read(dev, 0U, priv->setup, sizeof(priv->setup));

	setup = (const struct usb_setup_packet *)priv->setup;
	priv->ep0_len = sys_le16_to_cpu(setup->wLength);

	if (priv->ep0_len == 0U) {
		/*
		 * No data stage: DataEnd goes out with the acknowledgment and
		 * the core runs the status stage on its own.
		 */
		csr |= USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_DATAEND_Msk;
		priv->ep0_stage = USBHS_EP0_STATUS;
	} else if (USB_REQTYPE_GET_DIR(setup->bmRequestType) == USB_REQTYPE_DIR_TO_HOST) {
		priv->ep0_stage = USBHS_EP0_DATA_IN;
	} else {
		priv->ep0_stage = USBHS_EP0_DATA_OUT;
	}

	core->USBHS_CSR0L = csr;

	priv->ep0_status_done = false;
	k_event_post(&priv->events, BIT(USBHS_EVT_SETUP));
}

/*
 * Picks up a SETUP packet that arrived in the same interrupt as the event that
 * ended the previous transfer. Called after any transition back to
 * USBHS_EP0_SETUP that did not itself consume RxPktRdy.
 */
static void usbhs_ep0_poll_setup(const struct device *dev)
{
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);

	if (core->USBHS_CSR0L & USBHS_ENDPOINT0_CSR0L_RXPKTRDY_Msk) {
		usbhs_ep0_setup(dev);
	}
}

static void usbhs_ep0_isr(const struct device *dev)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	struct net_buf *buf;
	uint8_t csr0;

	core->USBHS_INDEX = 0U;
	csr0 = core->USBHS_CSR0L;

	if (csr0 & USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SENTSTALL_Msk) {
		/* Writing zero clears SentStall without acting on anything. */
		core->USBHS_CSR0L = 0U;
		priv->ep0_stage = USBHS_EP0_SETUP;
		usbhs_ep0_poll_setup(dev);
		return;
	}

	if (csr0 & USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SETUPEND_Msk) {
		/*
		 * The host ended the transfer before the driver did. Retire
		 * the stage and let the stack cancel whatever it had queued.
		 */
		core->USBHS_CSR0L = USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SERVICEDSETUPEND_Msk;
		priv->ep0_stage = USBHS_EP0_SETUP;
		priv->ep0_status_done = false;
		k_event_post(&priv->events, BIT(USBHS_EVT_SETUP_ABORT));
		return;
	}

	switch (priv->ep0_stage) {
	case USBHS_EP0_SETUP:
		if ((csr0 & USBHS_ENDPOINT0_CSR0L_RXPKTRDY_Msk) == 0U) {
			break;
		}

		usbhs_ep0_setup(dev);
		break;

	case USBHS_EP0_DATA_IN:
		if (csr0 & USBHS_ENDPOINT0_CSR0L_TXPKTRDY_Msk) {
			break;
		}

		buf = udc_buf_peek(udc_get_ep_cfg(dev, USB_CONTROL_EP_IN));
		if (buf == NULL) {
			LOG_ERR("No buffer for the control IN data stage");
			break;
		}

		if (usbhs_ep0_write(dev, buf)) {
			priv->ep0_stage = USBHS_EP0_STATUS;
			usbhs_xfer_finished(dev, USB_CONTROL_EP_IN);
		}
		break;

	case USBHS_EP0_DATA_OUT:
		if ((csr0 & USBHS_ENDPOINT0_CSR0L_RXPKTRDY_Msk) == 0U) {
			break;
		}

		buf = udc_buf_peek(udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT));
		if (buf == NULL) {
			/*
			 * The host can send the data stage before the stack has
			 * queued a buffer for it. The packet stays in the FIFO
			 * and is read when the buffer is enqueued.
			 */
			LOG_DBG("Control OUT data before a buffer, deferred");
			break;
		}

		if (usbhs_ep0_read(dev, buf)) {
			priv->ep0_stage = USBHS_EP0_STATUS;
			usbhs_xfer_finished(dev, USB_CONTROL_EP_OUT);
		}
		break;

	case USBHS_EP0_STATUS:
		priv->ep0_stage = USBHS_EP0_SETUP;

		if (!usbhs_ep0_finish_status(dev)) {
			/* The stack has not queued the status buffer yet. */
			priv->ep0_status_done = true;
		}

		usbhs_ep0_poll_setup(dev);
		break;
	}
}

/*
 * Transmit interrupt for a data endpoint: the packet handed to the core has
 * gone out and the FIFO is free again.
 *
 * Called with the driver lock held and INDEX already on this endpoint.
 */
static void usbhs_ep_in_isr(const struct device *dev, const uint8_t idx)
{
	usbhs_endpointx_registers_t *const epx = usbhs_epx(dev);
	const uint8_t ep = USB_EP_DIR_IN | idx;
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	struct net_buf *buf;
	uint8_t csr = epx->USBHS_TXCSRL;

	if (csr & USBHS_ENDPOINTX_TXCSRL_PERIPHERAL_EPX_SENTSTALL_Msk) {
		/* Clear the sent-stall record and leave the queue alone. */
		epx->USBHS_TXCSRL = csr & ~(USBHS_ENDPOINTX_TXCSRL_PERIPHERAL_EPX_SENTSTALL_Msk |
					    USBHS_ENDPOINTX_TXCSRL_PERIPHERAL_EPX_UNDERRUN_Msk);
		return;
	}

	if (csr & USBHS_ENDPOINTX_TXCSRL_PERIPHERAL_EPX_UNDERRUN_Msk) {
		/* An IN token arrived with nothing loaded; only a record. */
		epx->USBHS_TXCSRL = csr & ~USBHS_ENDPOINTX_TXCSRL_PERIPHERAL_EPX_UNDERRUN_Msk;
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for ep 0x%02x", ep);
		return;
	}

	if (buf->len != 0U) {
		uint16_t len = MIN(buf->len, udc_mps_ep_size(ep_cfg));

		usbhs_fifo_write(dev, idx, buf->data, len);
		net_buf_pull(buf, len);
		epx->USBHS_TXCSRL = USBHS_ENDPOINTX_TXCSRL_TXPKTRDY_Msk;
		return;
	}

	if (udc_ep_buf_has_zlp(buf)) {
		udc_ep_buf_clear_zlp(buf);
		epx->USBHS_TXCSRL = USBHS_ENDPOINTX_TXCSRL_TXPKTRDY_Msk;
		return;
	}

	usbhs_xfer_finished(dev, ep);
}

/*
 * Receive interrupt for a data endpoint, and also the path that drains a
 * packet already waiting when a transfer is queued. The transfer ends on a
 * short packet or a full buffer.
 *
 * Called with the driver lock held and INDEX already on this endpoint.
 */
static void usbhs_ep_out_isr(const struct device *dev, const uint8_t idx)
{
	usbhs_endpointx_registers_t *const epx = usbhs_epx(dev);
	const uint8_t ep = USB_EP_DIR_OUT | idx;
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
	uint8_t csr = epx->USBHS_RXCSRL;
	struct net_buf *buf;
	uint16_t count;
	uint16_t len;

	if (csr & USBHS_ENDPOINTX_RXCSRL_PERIPHERAL_EPX_SENTSTALL_Msk) {
		epx->USBHS_RXCSRL = csr & ~(USBHS_ENDPOINTX_RXCSRL_PERIPHERAL_EPX_SENTSTALL_Msk |
					    USBHS_ENDPOINTX_RXCSRL_PERIPHERAL_EPX_OVERRUN_Msk);
		return;
	}

	if ((csr & USBHS_ENDPOINTX_RXCSRL_RXPKTRDY_Msk) == 0U) {
		return;
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		/*
		 * Nothing to receive into. RxPktRdy is left set, which makes
		 * the core NAK further OUT tokens until a transfer is queued;
		 * udc_usbhs_start_xfer() drains the packet then.
		 */
		return;
	}

	count = epx->USBHS_RXCOUNT;
	len = MIN(count, net_buf_tailroom(buf));

	usbhs_fifo_read(dev, idx, net_buf_tail(buf), len);
	net_buf_add(buf, len);

	if (count > len) {
		/*
		 * The packet does not fit the queued buffer. The remainder is
		 * drained so the FIFO stays in step with the core, and the
		 * loss is reported as UDC_EVT_ERROR with -ENOBUFS.
		 *
		 * No in-tree driver reports this case. udc_sam0.c truncates
		 * silently, and its UDC_EVT_ERROR/-ENOBUFS sites, like those
		 * in udc_kinetis.c and udc_max32.c, are the different case of
		 * an OUT packet arriving with no buffer queued at all. It is
		 * reported here because delivering a short packet to a class
		 * that asked for more is data loss nothing above the driver
		 * can see: the transfer completes, the length looks like a
		 * legitimate short packet, and the bytes are simply gone.
		 *
		 * Not a stall: no in-tree controller driver halts an endpoint
		 * for this, and the fault is a buffer sized below the
		 * endpoint's maximum packet size rather than anything the host
		 * got wrong.
		 */
		LOG_ERR("ep 0x%02x overflow: %u bytes dropped", ep, count - len);
		usbhs_fifo_discard(dev, idx, count - len);
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
	}

	/* Writing zero clears RxPktRdy and hands the FIFO back to the core. */
	epx->USBHS_RXCSRL = 0U;

	if ((count < udc_mps_ep_size(ep_cfg)) || (net_buf_tailroom(buf) == 0U)) {
		usbhs_xfer_finished(dev, ep);
	}
}

/*
 * Bus events out of INTRUSB. Reset also restores the parts of the core state
 * a bus reset clears: the address goes back to zero and endpoint 0 returns to
 * waiting for a SETUP packet.
 */
static void usbhs_bus_isr(const struct device *dev, uint8_t intrusb)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);

	if (intrusb & USBHS_INTRUSB_RESET_Msk) {
		core->USBHS_FADDR = 0U;
		core->USBHS_INTRTXE = BIT(0);
		core->USBHS_INTRRXE = 0U;
		priv->ep0_stage = USBHS_EP0_SETUP;
		priv->ep0_status_done = false;
		udc_submit_event(dev, UDC_EVT_RESET, 0);
	}

	if ((intrusb & USBHS_INTRUSB_SUSPEND_Msk) && !udc_is_suspended(dev)) {
		udc_set_suspended(dev, true);
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
	}

	if ((intrusb & USBHS_INTRUSB_RESUME_Msk) && udc_is_suspended(dev)) {
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
	}

	if (intrusb & USBHS_INTRUSB_DISCON_Msk) {
		/*
		 * Not a VBUS event. caps.can_detect_vbus is false because this
		 * driver has no way to report VBUS coming back: the OTG VBUS
		 * and session comparators are powered down by PHY24.OTGPDN in
		 * device mode, and the board's VBUS sense is not wired to
		 * anything the driver reads. Submitting UDC_EVT_VBUS_REMOVED
		 * while UDC_EVT_VBUS_READY can never follow is worse than
		 * submitting nothing: an application that disables on the
		 * first and waits for the second never comes back.
		 */
		LOG_DBG("Disconnect on %s", dev->name);
	}

	if (intrusb & USBHS_INTRUSB_SOF_Msk) {
		udc_submit_sof_event(dev);
	}
}

/*
 * Interrupt service routine.
 *
 * INTRTX, INTRRX and INTRUSB all clear on read - including on a read from a
 * debugger. Each is read exactly once here, into a local; a live register
 * window over those three addresses eats interrupts and the failure looks
 * like a driver bug (errata 2.14.1).
 */
static void udc_usbhs_isr_handler(const struct device *dev)
{
	const struct udc_usbhs_config *cfg = dev->config;
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	k_spinlock_key_t key;
	uint16_t intrtx;
	uint16_t intrrx;
	uint8_t intrusb;
	uint8_t index;

	key = k_spin_lock(&priv->lock);

	/*
	 * The wrapper latches its own flag in front of the core's. Reading the
	 * core's three interrupt registers below clears them, but INTFLAG.USB
	 * stays set until it is written back, and an interrupt line that is
	 * never deasserted re-enters the moment this function returns. The
	 * board then makes no further progress at all: the symptom is not a USB
	 * fault but an image that stops 23 ms into boot, with the log ending
	 * mid-enumeration and every other thread starved.
	 *
	 * Clear it first, so a source that asserts while this handler runs
	 * latches again rather than being lost.
	 */
	core->USBHS_INTFLAG = USBHS_INTFLAG_USB_Msk;

	intrtx = core->USBHS_INTRTX;
	intrrx = core->USBHS_INTRRX;
	intrusb = core->USBHS_INTRUSB;

	index = core->USBHS_INDEX;

	if (intrtx & BIT(0)) {
		usbhs_ep0_isr(dev);
	}

	for (uint8_t idx = 1U; idx < cfg->num_of_eps; idx++) {
		if (intrtx & BIT(idx)) {
			core->USBHS_INDEX = idx;
			usbhs_ep_in_isr(dev, idx);
		}

		if (intrrx & BIT(idx)) {
			core->USBHS_INDEX = idx;
			usbhs_ep_out_isr(dev, idx);
		}
	}

	core->USBHS_INDEX = index;

	k_spin_unlock(&priv->lock, key);

	if (intrusb != 0U) {
		usbhs_bus_isr(dev, intrusb);
	}
}

/*
 * Starts the transfer at the head of an endpoint's queue, if the endpoint is
 * idle and something is waiting. Runs on the worker thread.
 */
static void usbhs_start_xfer(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	usbhs_endpointx_registers_t *const epx = usbhs_epx(dev);
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	const uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	struct net_buf *buf = udc_buf_peek(ep_cfg);
	struct udc_buf_info *bi;
	k_spinlock_key_t key;
	uint8_t index;

	if (buf == NULL) {
		return;
	}

	bi = udc_get_buf_info(buf);

	/*
	 * The buffer the stack arms for the next SETUP packet is not a
	 * transfer: the core delivers SETUP data with no action from us.
	 */
	if (bi->setup) {
		return;
	}

	udc_ep_set_busy(ep_cfg, true);

	key = k_spin_lock(&priv->lock);
	index = core->USBHS_INDEX;
	core->USBHS_INDEX = idx;

	if (idx == 0U) {
		/*
		 * The status stage of a control transfer is run by the core -
		 * DataEnd was set with the last data packet, or with the
		 * acknowledgment of a SETUP that asked for no data. All this
		 * buffer does is report it, so it completes as soon as the
		 * core says the stage is over, in whichever order the two
		 * arrive.
		 */
		if (bi->status) {
			if (priv->ep0_status_done) {
				priv->ep0_status_done = false;
				usbhs_xfer_finished(dev, ep_cfg->addr);
			}
		} else if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
			if (priv->ep0_stage == USBHS_EP0_DATA_IN) {
				if (usbhs_ep0_write(dev, buf)) {
					priv->ep0_stage = USBHS_EP0_STATUS;
					usbhs_xfer_finished(dev, ep_cfg->addr);
				}
			}
		} else if (priv->ep0_stage == USBHS_EP0_DATA_OUT) {
			/* A packet may already be waiting in the FIFO. */
			if (core->USBHS_CSR0L & USBHS_ENDPOINT0_CSR0L_RXPKTRDY_Msk) {
				if (usbhs_ep0_read(dev, buf)) {
					priv->ep0_stage = USBHS_EP0_STATUS;
					usbhs_xfer_finished(dev, ep_cfg->addr);
				}
			}
		}
	} else if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		uint16_t len = MIN(buf->len, udc_mps_ep_size(ep_cfg));

		usbhs_fifo_write(dev, idx, buf->data, len);
		net_buf_pull(buf, len);
		epx->USBHS_TXCSRL = USBHS_ENDPOINTX_TXCSRL_TXPKTRDY_Msk;
	} else {
		/*
		 * Nothing to arm for a receive: the core takes OUT packets
		 * into the FIFO on its own. It may already hold one that
		 * arrived while no buffer was queued, in which case the
		 * interrupt for it is long gone and it has to be drained here.
		 */
		usbhs_ep_out_isr(dev, idx);
	}

	core->USBHS_INDEX = index;
	k_spin_unlock(&priv->lock, key);
}

/* Submits a completed transfer and starts whatever is queued behind it. */
static void usbhs_handle_xfer_finished(const struct device *dev)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	uint32_t eps = atomic_clear(&priv->xfer_finished);

	while (eps != 0U) {
		uint8_t ep = usbhs_pull_ep_from_bmsk(&eps);
		struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);
		struct net_buf *buf = udc_buf_get(ep_cfg);

		if (buf == NULL) {
			LOG_ERR("No buffer for ep 0x%02x", ep);
			udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
			continue;
		}

		udc_ep_set_busy(ep_cfg, false);
		udc_submit_ep_event(dev, buf, 0);

		usbhs_start_xfer(dev, ep_cfg);
	}
}

/* Starts transfers queued on endpoints that were idle. */
static void usbhs_handle_xfer_new(const struct device *dev)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	uint32_t eps = atomic_clear(&priv->xfer_new);

	while (eps != 0U) {
		uint8_t ep = usbhs_pull_ep_from_bmsk(&eps);
		struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep);

		if (udc_ep_is_busy(ep_cfg)) {
			continue;
		}

		usbhs_start_xfer(dev, ep_cfg);
	}
}

/*
 * Worker thread body. Everything that may take the UDC lock or call into the
 * stack runs here rather than in the interrupt.
 */
static void usbhs_thread_handler(const struct device *const dev)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	uint32_t evt = k_event_wait(&priv->events, UINT32_MAX, false, K_FOREVER);

	if (evt & BIT(USBHS_EVT_SETUP_ABORT)) {
		k_event_clear(&priv->events, BIT(USBHS_EVT_SETUP_ABORT));

		/* No valid SETUP data: this only retires the stale stages. */
		udc_setup_received(dev, NULL);
	}

	if (evt & BIT(USBHS_EVT_SETUP)) {
		k_event_clear(&priv->events, BIT(USBHS_EVT_SETUP));

		udc_setup_received(dev, priv->setup);
	}

	udc_lock_internal(dev, K_FOREVER);

	if (evt & BIT(USBHS_EVT_XFER_FINISHED)) {
		k_event_clear(&priv->events, BIT(USBHS_EVT_XFER_FINISHED));

		usbhs_handle_xfer_finished(dev);
	}

	if (evt & BIT(USBHS_EVT_XFER_NEW)) {
		k_event_clear(&priv->events, BIT(USBHS_EVT_XFER_NEW));

		usbhs_handle_xfer_new(dev);
	}

	udc_unlock_internal(dev);
}

static void usbhs_thread(void *dev, void *arg1, void *arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	while (true) {
		usbhs_thread_handler(dev);
	}
}

/*
 * Queues a transfer. Safe from interrupt context: it only appends to the
 * endpoint queue and posts an event.
 */
static int udc_usbhs_ep_enqueue(const struct device *dev, struct udc_ep_config *const ep_cfg,
				struct net_buf *buf)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);

	LOG_DBG("%s enqueue 0x%02x %p", dev->name, ep_cfg->addr, (void *)buf);
	udc_buf_put(ep_cfg, buf);

	if (!ep_cfg->stat.halted) {
		atomic_set_bit(&priv->xfer_new, usbhs_ep_to_bnum(ep_cfg->addr));
		k_event_post(&priv->events, BIT(USBHS_EVT_XFER_NEW));
	}

	return 0;
}

/*
 * Cancels everything queued on an endpoint. A transfer already in flight has
 * left a partial packet in the FIFO, so the FIFO is flushed too - otherwise
 * the next transfer inherits it.
 */
static int udc_usbhs_ep_dequeue(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	const uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	k_spinlock_key_t key;
	uint8_t index;

	key = k_spin_lock(&priv->lock);
	index = core->USBHS_INDEX;
	core->USBHS_INDEX = idx;

	if (idx == 0U) {
		/*
		 * Only the direction being dequeued is retired. The control
		 * state machine is shared between both halves of endpoint 0,
		 * and resetting it from the direction that is not in flight
		 * would throw away a data stage that is still running.
		 */
		const enum usbhs_ep0_stage stage =
			USB_EP_DIR_IS_IN(ep_cfg->addr) ? USBHS_EP0_DATA_IN : USBHS_EP0_DATA_OUT;

		if (priv->ep0_stage == stage) {
			priv->ep0_stage = USBHS_EP0_SETUP;
			usbhs_ep0_flush(dev);
		}
	} else if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		usbhs_ep_tx_flush(dev);
	} else {
		usbhs_ep_rx_flush(dev);
	}

	core->USBHS_INDEX = index;
	k_spin_unlock(&priv->lock, key);

	udc_ep_cancel_queued(dev, ep_cfg);
	udc_ep_set_busy(ep_cfg, false);

	return 0;
}

/*
 * Configures an endpoint and gives it FIFO space.
 *
 * Endpoint 0 has its FIFO from the bring-up sequence and needs only its
 * interrupt. Every other endpoint gets a block out of the pool, its maximum
 * payload and a cleared data toggle. INDEX is restored on the way out,
 * including on the -ENOMEM path, and the allocator is left untouched when the
 * allocation does not fit.
 */
static int udc_usbhs_ep_enable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	usbhs_endpointx_registers_t *const epx = usbhs_epx(dev);
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	const uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	const uint16_t mps = udc_mps_ep_size(ep_cfg);
	uint16_t size;
	k_spinlock_key_t key;
	uint8_t index;
	int ret = 0;

	key = k_spin_lock(&priv->lock);
	index = core->USBHS_INDEX;

	if (idx == 0U) {
		core->USBHS_INTRTXE |= BIT(0);
		goto out;
	}

	if (mps > USBHS_FIFO_SIZE_MAX) {
		ret = -EINVAL;
		goto out;
	}

	size = usbhs_fifo_size(mps);

	ret = usbhs_fifo_alloc(dev, ep_cfg->addr, size);
	if (ret != 0) {
		goto out;
	}

	core->USBHS_INDEX = idx;

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		epx->USBHS_TXMAXP = mps;
		core->USBHS_TXFIFOSZ = usbhs_fifo_size_encode(size);
		core->USBHS_TXFIFOADD = usbhs_fifo_offset(dev, ep_cfg->addr) / USBHS_FIFO_ADDR_UNIT;
		/* MODE selects the transmit direction for the indexed endpoint. */
		epx->USBHS_TXCSRH = USBHS_ENDPOINTX_TXCSRH_MODE_Msk;
		epx->USBHS_TXCSRL = USBHS_ENDPOINTX_TXCSRL_CLRDATATOG_Msk;
		usbhs_ep_tx_flush(dev);
		core->USBHS_INTRTXE |= BIT(idx);
	} else {
		epx->USBHS_RXMAXP = mps;
		core->USBHS_RXFIFOSZ = usbhs_fifo_size_encode(size);
		core->USBHS_RXFIFOADD = usbhs_fifo_offset(dev, ep_cfg->addr) / USBHS_FIFO_ADDR_UNIT;
		epx->USBHS_RXCSRH = 0U;
		epx->USBHS_RXCSRL = USBHS_ENDPOINTX_RXCSRL_CLRDATATOG_Msk;
		usbhs_ep_rx_flush(dev);
		core->USBHS_INTRRXE |= BIT(idx);
	}

	LOG_DBG("Enable ep 0x%02x mps %u fifo %u at %u", ep_cfg->addr, mps, size,
		usbhs_fifo_offset(dev, ep_cfg->addr));

out:
	core->USBHS_INDEX = index;
	k_spin_unlock(&priv->lock, key);

	return ret;
}

/* Masks an endpoint, flushes its FIFO and returns its block to the pool. */
static int udc_usbhs_ep_disable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	const uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	k_spinlock_key_t key;
	uint8_t index;

	key = k_spin_lock(&priv->lock);
	index = core->USBHS_INDEX;
	core->USBHS_INDEX = idx;

	if (idx == 0U) {
		core->USBHS_INTRTXE &= ~BIT(0);
		usbhs_ep0_flush(dev);
	} else if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		core->USBHS_INTRTXE &= ~BIT(idx);
		usbhs_ep_tx_flush(dev);
		usbhs_fifo_free(dev, ep_cfg->addr);
	} else {
		core->USBHS_INTRRXE &= ~BIT(idx);
		usbhs_ep_rx_flush(dev);
		usbhs_fifo_free(dev, ep_cfg->addr);
	}

	core->USBHS_INDEX = index;
	k_spin_unlock(&priv->lock, key);

	LOG_DBG("Disable ep 0x%02x", ep_cfg->addr);

	return 0;
}

/* Makes an endpoint answer with a STALL handshake until the halt is cleared. */
static int udc_usbhs_ep_set_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	usbhs_endpointx_registers_t *const epx = usbhs_epx(dev);
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	const uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	k_spinlock_key_t key;
	uint8_t index;

	key = k_spin_lock(&priv->lock);
	index = core->USBHS_INDEX;
	core->USBHS_INDEX = idx;

	if (idx == 0U) {
		/*
		 * Stalling a control transfer also acknowledges the SETUP
		 * packet that caused it, which is what ends the transfer.
		 */
		core->USBHS_CSR0L = USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SENDSTALL_Msk |
				    USBHS_ENDPOINT0_CSR0L_PERIPHERAL_EP0_SERVICEDRXPKTRDY_Msk;
		priv->ep0_stage = USBHS_EP0_SETUP;
	} else if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		epx->USBHS_TXCSRL = USBHS_ENDPOINTX_TXCSRL_PERIPHERAL_EPX_SENDSTALL_Msk;
		ep_cfg->stat.halted = true;
	} else {
		epx->USBHS_RXCSRL = USBHS_ENDPOINTX_RXCSRL_PERIPHERAL_EPX_SENDSTALL_Msk;
		ep_cfg->stat.halted = true;
	}

	core->USBHS_INDEX = index;
	k_spin_unlock(&priv->lock, key);

	LOG_DBG("Set halt ep 0x%02x", ep_cfg->addr);

	return 0;
}

/*
 * Clears the halt and, with it, the data toggle. The specification requires
 * the toggle to restart at DATA0, and forgetting it is the usual cause of an
 * endpoint that works right up to its first stall.
 */
static int udc_usbhs_ep_clear_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	usbhs_endpointx_registers_t *const epx = usbhs_epx(dev);
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	const uint8_t idx = USB_EP_GET_IDX(ep_cfg->addr);
	k_spinlock_key_t key;
	uint8_t index;

	if (idx == 0U) {
		return 0;
	}

	key = k_spin_lock(&priv->lock);
	index = core->USBHS_INDEX;
	core->USBHS_INDEX = idx;

	/*
	 * SendStall and SentStall are write-zero-to-clear, so clearing the halt
	 * is the act of writing them as zero - not a side effect of the write
	 * being a bare assignment. They are named here so that a later change
	 * to a read-modify-write cannot quietly leave the endpoint stalled.
	 */
	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		uint8_t csr = epx->USBHS_TXCSRL;

		csr &= ~(USBHS_ENDPOINTX_TXCSRL_PERIPHERAL_EPX_SENDSTALL_Msk |
			 USBHS_ENDPOINTX_TXCSRL_PERIPHERAL_EPX_SENTSTALL_Msk |
			 USBHS_ENDPOINTX_TXCSRL_PERIPHERAL_EPX_UNDERRUN_Msk);
		csr |= USBHS_ENDPOINTX_TXCSRL_CLRDATATOG_Msk;

		epx->USBHS_TXCSRL = csr;
	} else {
		uint8_t csr = epx->USBHS_RXCSRL;

		csr &= ~(USBHS_ENDPOINTX_RXCSRL_PERIPHERAL_EPX_SENDSTALL_Msk |
			 USBHS_ENDPOINTX_RXCSRL_PERIPHERAL_EPX_SENTSTALL_Msk |
			 USBHS_ENDPOINTX_RXCSRL_PERIPHERAL_EPX_OVERRUN_Msk);
		csr |= USBHS_ENDPOINTX_RXCSRL_CLRDATATOG_Msk;

		epx->USBHS_RXCSRL = csr;
	}

	core->USBHS_INDEX = index;
	k_spin_unlock(&priv->lock, key);

	ep_cfg->stat.halted = false;

	if (!udc_ep_is_busy(ep_cfg) && (udc_buf_peek(ep_cfg) != NULL)) {
		atomic_set_bit(&priv->xfer_new, usbhs_ep_to_bnum(ep_cfg->addr));
		k_event_post(&priv->events, BIT(USBHS_EVT_XFER_NEW));
	}

	LOG_DBG("Clear halt ep 0x%02x", ep_cfg->addr);

	return 0;
}

/*
 * Writes the device address.
 *
 * The stack calls this once the status stage of the SET_ADDRESS transfer has
 * completed, which is the only moment it may be applied: an address written
 * before the status stage leaves the host seeing a device that answers at
 * address 0 and then disappears. The driver does not complete the status
 * buffer until the core reports the stage over, so this ordering holds
 * without any further deferral here.
 */
static int udc_usbhs_set_address(const struct device *dev, const uint8_t addr)
{
	usbhs_core(dev)->USBHS_FADDR = addr;

	LOG_DBG("Set new address %u for %s", addr, dev->name);

	return 0;
}

/* Drives resume signalling to ask a suspended host to wake the bus. */
static int udc_usbhs_host_wakeup(const struct device *dev)
{
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);

	LOG_DBG("Remote wakeup from %s", dev->name);

	core->USBHS_POWER |= USBHS_POWER_RESUME_Msk;
	k_msleep(USBHS_RESUME_HOLD_MS);
	core->USBHS_POWER &= ~USBHS_POWER_RESUME_Msk;

	return 0;
}

/*
 * Reports the speed the bus actually settled on. HSMODE is set by the core
 * only when the high-speed handshake succeeded during the bus reset, so this
 * is the negotiated speed rather than the configured one.
 */
static enum udc_bus_speed udc_usbhs_device_speed(const struct device *dev)
{
	if (usbhs_core(dev)->USBHS_POWER & USBHS_POWER_HSMODE_Msk) {
		return UDC_BUS_SPEED_HS;
	}

	return UDC_BUS_SPEED_FS;
}

/*
 * The 53-byte high-speed test packet of USB 2.0 section 7.1.20, table 7-4.
 * The core does not synthesise it: TESTMODE.TESTPACKET makes it retransmit
 * whatever endpoint 0's transmit FIFO holds, so the pattern has to be there
 * first. Datasheet 37.7.18, bit 3: "Test packet must be loaded into the
 * Endpoint 0 FIFO before the test mode is entered."
 */
static const uint8_t usbhs_test_packet[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	0xAA, 0xAA, 0xAA, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xFE, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xBF, 0xDF, 0xEF, 0xF7,
	0xFB, 0xFD, 0xFC, 0x7E, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0x7E,
};

/*
 * Enters one of the USB 2.0 high-speed compliance test modes, on the
 * SetFeature(TEST_MODE) request the stack has already answered. mode is the
 * test selector out of wIndex, so it is a USB_SFS_TEST_MODE_* value rather
 * than a register encoding; TESTMODE at offset 0x100F carries one bit per
 * mode rather than a field, so the two are mapped explicitly. With dryrun set
 * the stack is only asking whether the mode could be entered and nothing may
 * be written.
 *
 * Only a power cycle leaves a test mode again, which is what the
 * specification requires of a device, so there is no path back from here.
 *
 * Test_Force_Enable is a host-side test, and the core's remaining TESTMODE
 * bits (FORCEHOST, FORCEHS, FORCEFS, FIFOACCESS) are host-mode and loopback
 * controls this driver does not implement, so anything else is refused rather
 * than half answered.
 */
static int udc_usbhs_test_mode(const struct device *dev, const uint8_t mode, const bool dryrun)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	k_spinlock_key_t key;
	uint8_t testmode;
	uint8_t index;

	switch (mode) {
	case USB_SFS_TEST_MODE_J:
		testmode = USBHS_TESTMODE_TESTJ_Msk;
		break;
	case USB_SFS_TEST_MODE_K:
		testmode = USBHS_TESTMODE_TESTK_Msk;
		break;
	case USB_SFS_TEST_MODE_SE0_NAK:
		testmode = USBHS_TESTMODE_TESTSE0NAK_Msk;
		break;
	case USB_SFS_TEST_MODE_PACKET:
		testmode = USBHS_TESTMODE_TESTPACKET_Msk;
		break;
	default:
		return -EINVAL;
	}

	/* Every one of these bits is only active with the core in high speed. */
	if ((core->USBHS_POWER & USBHS_POWER_HSMODE_Msk) == 0U) {
		LOG_ERR("Test mode %u needs the bus at high speed", mode);
		return -EPERM;
	}

	if (core->USBHS_TESTMODE != 0U) {
		return -EALREADY;
	}

	if (dryrun) {
		LOG_DBG("Test mode %u supported", mode);
		return 0;
	}

	key = k_spin_lock(&priv->lock);

	if (mode == USB_SFS_TEST_MODE_PACKET) {
		index = core->USBHS_INDEX;
		core->USBHS_INDEX = 0U;
		usbhs_fifo_write(dev, 0U, usbhs_test_packet, sizeof(usbhs_test_packet));
		core->USBHS_CSR0L = USBHS_ENDPOINT0_CSR0L_TXPKTRDY_Msk;
		core->USBHS_INDEX = index;
	}

	core->USBHS_TESTMODE = testmode;

	k_spin_unlock(&priv->lock, key);

	LOG_DBG("Enter test mode %u on %s", mode, dev->name);

	return 0;
}

/*
 * Claims the interrupt, runs the bring-up sequence and registers the control
 * endpoints. The device is left detached: nothing appears on the bus until
 * udc_usbhs_enable() sets SOFTCONN.
 */
static int udc_usbhs_init(const struct device *dev)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	int ret;

	priv->ep0_stage = USBHS_EP0_SETUP;
	priv->ep0_status_done = false;

	ret = usbhs_bringup(dev, &priv->stage);
	if (ret != 0) {
		/*
		 * Leave nothing half-enabled behind, and undo only what was
		 * actually done: a failure before the clock was enabled must
		 * not write a register in an unclocked window.
		 */
		usbhs_unwind(dev, priv->stage);
		priv->stage = USBHS_STAGE_NONE;
		return ret;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, USB_CONTROL_EP_MPS,
				   0) != 0) {
		LOG_ERR("Failed to enable the control OUT endpoint");
		goto unwind;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, USB_CONTROL_EP_MPS,
				   0) != 0) {
		LOG_ERR("Failed to enable the control IN endpoint");
		(void)udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT);
		goto unwind;
	}

	LOG_DBG("Init device %s", dev->name);

	return 0;

unwind:
	/* An init that fails leaves nothing enabled behind it. */
	usbhs_unwind(dev, priv->stage);
	priv->stage = USBHS_STAGE_NONE;

	return -EIO;
}

/*
 * Restores the endpoint interrupt masks from the endpoints the stack still
 * holds enabled. disable() clears the masks, and ep_enable() - the only other
 * place they are written - is not called again across a disable and enable
 * cycle, so without this the device attaches and then ignores every packet.
 * Endpoint 0 is always unmasked: its interrupt is what carries SETUP.
 */
static void usbhs_restore_ep_irq(const struct device *dev)
{
	const struct udc_usbhs_config *cfg = dev->config;
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	uint16_t intrtxe = BIT(0);
	uint16_t intrrxe = 0U;

	for (uint8_t idx = 1U; idx < cfg->num_of_eps; idx++) {
		if (cfg->ep_cfg_in[idx].stat.enabled) {
			intrtxe |= BIT(idx);
		}

		if (cfg->ep_cfg_out[idx].stat.enabled) {
			intrrxe |= BIT(idx);
		}
	}

	core->USBHS_INTRTXE = intrtxe;
	core->USBHS_INTRRXE = intrrxe;
}

/*
 * Attaches to the bus. HSEN decides whether the core answers the host's
 * high-speed chirp; with it clear the device stays full speed whatever the
 * host offers. Idempotent - every write here is a assignment of the state the
 * driver wants, not a toggle.
 */
static int udc_usbhs_enable(const struct device *dev)
{
	const struct udc_usbhs_config *cfg = dev->config;
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	uint8_t power = core->USBHS_POWER & ~USBHS_POWER_HSENABLE_Msk;

	if (cfg->high_speed) {
		power |= USBHS_POWER_HSENABLE_Msk;
	}

	core->USBHS_POWER = power;

	core->USBHS_INTRUSBE = USBHS_INTRUSBE_RESETEN_Msk | USBHS_INTRUSBE_SUSPENDEN_Msk |
			       USBHS_INTRUSBE_RESUMEEN_Msk | USBHS_INTRUSBE_DISCONEN_Msk;

	usbhs_restore_ep_irq(dev);

	/*
	 * The masks above are the MUSB core's own. They decide which sources
	 * raise INTFLAG.USB, and nothing more: the Microchip wrapper around the
	 * core has a second mask in front of the interrupt line, and with it
	 * clear the flags simply accumulate and the NVIC never hears about them.
	 * Unmasking the core alone produces a device that attaches, is seen by
	 * the host, and answers no control transfer ever - which is exactly what
	 * this looked like on the bench, with INTFLAG reading 0x24 and INTENSET
	 * reading zero.
	 */
	core->USBHS_INTENSET = USBHS_INTENSET_USB_Msk;

	cfg->irq_enable_func(dev);

	core->USBHS_POWER = power | USBHS_POWER_SOFTCONN_Msk;

	LOG_DBG("Enable device %s (%s)", dev->name, cfg->high_speed ? "high speed" : "full speed");

	return 0;
}

/*
 * Detaches from the bus and masks every interrupt. The FIFO allocations stay
 * as they are, so a later enable does not have to lay them out again; the
 * masks do not survive, and udc_usbhs_enable() puts them back.
 */
static int udc_usbhs_disable(const struct device *dev)
{
	const struct udc_usbhs_config *cfg = dev->config;
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);

	core->USBHS_POWER &= ~USBHS_POWER_SOFTCONN_Msk;

	core->USBHS_INTRUSBE = 0U;
	core->USBHS_INTRTXE = 0U;
	core->USBHS_INTRRXE = 0U;

	core->USBHS_INTENCLR = USBHS_INTENCLR_USB_Msk;

	cfg->irq_disable_func(dev);

	LOG_DBG("Disable device %s", dev->name);

	return 0;
}

/*
 * Releases the hardware. After this the register window is not valid until
 * udc_usbhs_init() has run again: the peripheral is disabled and its voltage
 * regulator is off.
 */
static int udc_usbhs_shutdown(const struct device *dev)
{
	const struct udc_usbhs_config *cfg = dev->config;
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	usbhs_endpoint0_registers_t *const core = usbhs_core(dev);
	int ret = 0;

	cfg->irq_disable_func(dev);

	if (priv->stage >= USBHS_STAGE_ENABLED) {
		core->USBHS_POWER &= ~USBHS_POWER_SOFTCONN_Msk;
		core->USBHS_CTRLA = 0U;

		ret = usbhs_wait_syncbusy(dev);
		if (ret != 0) {
			/*
			 * Report it, but carry on: returning here would leave
			 * the regulator, the clock and the connector pins on
			 * with no second chance to release them.
			 */
			LOG_ERR("Timeout disabling the peripheral");
		}
	}

	/* CTRLA is already cleared above, so the unwind starts below it. */
	usbhs_unwind(dev, MIN(priv->stage, USBHS_STAGE_CLOCK));
	priv->stage = USBHS_STAGE_NONE;

	LOG_DBG("Shutdown device %s", dev->name);

	return ret;
}

/* Declares one direction's worth of endpoints to the stack. */
static int udc_usbhs_register_eps(const struct device *dev, struct udc_ep_config *ep_cfg,
				  size_t num_eps, uint8_t dir, uint16_t mps)
{
	int err;

	for (size_t i = 0; i < num_eps; i++) {
		if (dir == USB_EP_DIR_IN) {
			ep_cfg[i].caps.in = 1;
		} else {
			ep_cfg[i].caps.out = 1;
		}

		if (i == 0) {
			ep_cfg[i].caps.control = 1;
			ep_cfg[i].caps.mps = USB_CONTROL_EP_MPS;
		} else {
			ep_cfg[i].caps.bulk = 1;
			ep_cfg[i].caps.interrupt = 1;
			/*
			 * Isochronous is not advertised, because nothing in
			 * this driver services it. An isochronous endpoint has
			 * one transaction per (micro)frame and no handshake, so
			 * it needs three things none of which exists here: a
			 * per-SOF transmit path driven by INTRUSB.SOF rather
			 * than by the transfer completion interrupt, double
			 * buffering so the next frame's packet is loaded while
			 * the current one goes out, and an underrun and overrun
			 * policy that reports the lost frame to the class
			 * instead of dropping it. Today underrun and overrun
			 * are cleared and discarded in usbhs_ep_in_isr() and
			 * usbhs_ep_out_isr(), and an OUT packet that arrives
			 * with no buffer queued is left in the FIFO to be
			 * NAKed - correct for bulk, wrong for a stream with no
			 * retry.
			 *
			 * Whoever sets this to 1 also has to restore the ISO
			 * bit in TXCSRH and RXCSRH in ep_enable(), which was
			 * removed with this flag because udc_ep_try_config()
			 * can never hand an isochronous endpoint to a driver
			 * that does not advertise one.
			 */
			ep_cfg[i].caps.iso = 0;
			ep_cfg[i].caps.mps = mps;
		}

		ep_cfg[i].addr = dir | i;

		err = udc_register_ep(dev, &ep_cfg[i]);
		if (err) {
			LOG_ERR("Failed to register endpoint 0x%02x", ep_cfg[i].addr);
			return err;
		}
	}

	return 0;
}

/*
 * Brings up the driver instance: clears the runtime state, declares the
 * endpoints and starts the worker thread. No hardware is touched here - that
 * waits for udc_usbhs_init().
 */
static int udc_usbhs_driver_preinit(const struct device *dev)
{
	const struct udc_usbhs_config *cfg = dev->config;
	struct udc_usbhs_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	int err;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);
	atomic_clear(&priv->xfer_new);
	atomic_clear(&priv->xfer_finished);

	for (size_t i = 0; i < USBHS_FIFO_SLOTS(cfg->num_of_eps); i++) {
		cfg->fifo[i].offset = 0U;
		cfg->fifo[i].size = 0U;
	}

	data->caps.rwup = true;
	data->caps.mps0 = UDC_MPS0_64;
	data->caps.can_detect_vbus = false;
	data->caps.hs = cfg->high_speed;

	err = udc_usbhs_register_eps(dev, cfg->ep_cfg_out, cfg->num_of_eps, USB_EP_DIR_OUT,
				     USBHS_EP_MPS_MAX);
	if (err) {
		return err;
	}

	err = udc_usbhs_register_eps(dev, cfg->ep_cfg_in, cfg->num_of_eps, USB_EP_DIR_IN,
				     USBHS_EP_MPS_MAX);
	if (err) {
		return err;
	}

	cfg->make_thread(dev);

	return 0;
}

static void udc_usbhs_lock(const struct device *dev)
{
	k_sched_lock();
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_usbhs_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
	k_sched_unlock();
}

static const struct udc_api udc_usbhs_api = {
	.lock = udc_usbhs_lock,
	.unlock = udc_usbhs_unlock,
	.device_speed = udc_usbhs_device_speed,
	.init = udc_usbhs_init,
	.enable = udc_usbhs_enable,
	.disable = udc_usbhs_disable,
	.shutdown = udc_usbhs_shutdown,
	.set_address = udc_usbhs_set_address,
	.test_mode = udc_usbhs_test_mode,
	.host_wakeup = udc_usbhs_host_wakeup,
	.ep_enable = udc_usbhs_ep_enable,
	.ep_disable = udc_usbhs_ep_disable,
	.ep_set_halt = udc_usbhs_ep_set_halt,
	.ep_clear_halt = udc_usbhs_ep_clear_halt,
	.ep_enqueue = udc_usbhs_ep_enqueue,
	.ep_dequeue = udc_usbhs_ep_dequeue,
};

#define UDC_USBHS_IRQ_ENABLE(i, n)                                                                 \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, i, irq), DT_INST_IRQ_BY_IDX(n, i, priority),             \
		    udc_usbhs_isr_handler, DEVICE_DT_INST_GET(n), 0);                              \
	irq_enable(DT_INST_IRQ_BY_IDX(n, i, irq));

#define UDC_USBHS_IRQ_DISABLE(i, n) irq_disable(DT_INST_IRQ_BY_IDX(n, i, irq));

#define UDC_USBHS_IRQ_ENABLE_DEFINE(n)                                                             \
	static void udc_usbhs_irq_enable_func_##n(const struct device *dev)                        \
	{                                                                                          \
		LISTIFY(DT_INST_NUM_IRQS(n), UDC_USBHS_IRQ_ENABLE, (), n) \
	}

#define UDC_USBHS_IRQ_DISABLE_DEFINE(n)                                                            \
	static void udc_usbhs_irq_disable_func_##n(const struct device *dev)                       \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		LISTIFY(DT_INST_NUM_IRQS(n), UDC_USBHS_IRQ_DISABLE, (), n) \
	}

#define UDC_USBHS_THREAD_DEFINE(n)                                                                 \
	K_THREAD_STACK_DEFINE(udc_usbhs_stack_##n, CONFIG_UDC_MCHP_USBHS_STACK_SIZE);              \
                                                                                                   \
	static void udc_usbhs_make_thread_##n(const struct device *dev)                            \
	{                                                                                          \
		struct udc_usbhs_data *priv = udc_get_private(dev);                                \
                                                                                                   \
		k_thread_create(&priv->thread_data, udc_usbhs_stack_##n,                           \
				K_THREAD_STACK_SIZEOF(udc_usbhs_stack_##n), usbhs_thread,          \
				(void *)dev, NULL, NULL,                                           \
				K_PRIO_COOP(CONFIG_UDC_MCHP_USBHS_THREAD_PRIORITY), K_ESSENTIAL,   \
				K_NO_WAIT);                                                        \
		k_thread_name_set(&priv->thread_data, dev->name);                                  \
	}

/*
 * SUPC gives instance n of the peripheral its own additional voltage
 * regulator, and the binding has no property naming it, so the index comes
 * from where the instance sits in the register map.
 */
#define UDC_USBHS_AVREG_IDX(n)                                                                     \
	((uint8_t)((DT_INST_REG_ADDR(n) - USBHS_INSTANCE_BASE) / USBHS_INSTANCE_STRIDE))

/*
 * maximum-speed enumerates low, full, high, super in that order, so index 1
 * is full speed, which is what an instance that does not name a speed gets.
 */
#define UDC_USBHS_HIGH_SPEED(n) (DT_INST_ENUM_IDX_OR(n, maximum_speed, 1) >= 2)

#define UDC_USBHS_CONFIG_DEFINE(n)                                                                 \
	BUILD_ASSERT(UDC_USBHS_AVREG_IDX(n) < 3, "USBHS instance outside the known address map");  \
                                                                                                   \
	static struct udc_ep_config                                                                \
		udc_usbhs_ep_cfg_out_##n[DT_INST_PROP(n, num_bidir_endpoints)];                    \
	static struct udc_ep_config udc_usbhs_ep_cfg_in_##n[DT_INST_PROP(n, num_bidir_endpoints)]; \
	static struct usbhs_fifo_block                                                             \
		udc_usbhs_fifo_##n[USBHS_FIFO_SLOTS(DT_INST_PROP(n, num_bidir_endpoints))];        \
                                                                                                   \
	static const struct udc_usbhs_config udc_usbhs_config_##n = {                              \
		.base = (usbhs_registers_t *)DT_INST_REG_ADDR(n),                                  \
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),                                \
		.ep_cfg_in = udc_usbhs_ep_cfg_in_##n,                                              \
		.ep_cfg_out = udc_usbhs_ep_cfg_out_##n,                                            \
		.fifo = udc_usbhs_fifo_##n,                                                        \
		.avreg_idx = UDC_USBHS_AVREG_IDX(n),                                               \
		.high_speed = UDC_USBHS_HIGH_SPEED(n),                                             \
		.drd = GPIO_DT_SPEC_INST_GET_OR(n, drd_gpios, {0}),                                \
		.vbus_enable = GPIO_DT_SPEC_INST_GET_OR(n, vbus_enable_gpios, {0}),                \
		.clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                             \
		.clock.mclk_subsys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem),      \
		.irq_enable_func = udc_usbhs_irq_enable_func_##n,                                  \
		.irq_disable_func = udc_usbhs_irq_disable_func_##n,                                \
		.make_thread = udc_usbhs_make_thread_##n,                                          \
	}

#define UDC_USBHS_DATA_DEFINE(n)                                                                   \
	static struct udc_usbhs_data udc_usbhs_priv_##n = {};                                      \
	static struct udc_data udc_usbhs_data_##n = {                                              \
		.mutex = Z_MUTEX_INITIALIZER(udc_usbhs_data_##n.mutex),                            \
		.priv = &udc_usbhs_priv_##n,                                                       \
	};

#define UDC_USBHS_DEVICE_DEFINE(n)                                                                 \
	UDC_USBHS_IRQ_ENABLE_DEFINE(n);                                                            \
	UDC_USBHS_IRQ_DISABLE_DEFINE(n);                                                           \
	UDC_USBHS_THREAD_DEFINE(n);                                                                \
	UDC_USBHS_DATA_DEFINE(n);                                                                  \
	UDC_USBHS_CONFIG_DEFINE(n);                                                                \
	DEVICE_DT_INST_DEFINE(n, udc_usbhs_driver_preinit, NULL, &udc_usbhs_data_##n,              \
			      &udc_usbhs_config_##n, POST_KERNEL,                                  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_usbhs_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_USBHS_DEVICE_DEFINE)
