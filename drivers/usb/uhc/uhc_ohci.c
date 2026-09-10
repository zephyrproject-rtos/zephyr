/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT generic_ohci

#include <errno.h>
#include <string.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_hub.h>
#include <zephyr/drivers/pcie/pcie.h>

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_rcc)
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#endif

#include "uhc_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_ohci, CONFIG_UHC_DRIVER_LOG_LEVEL);

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev) ((const struct ohci_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct ohci_data *)(_dev)->data)

/*
 * DMA coherency: on coherent controllers the descriptors and transfer buffers
 * are used as they are. On non-coherent controllers the descriptors (HCCA,
 * EDs, TDs) are placed in the nocache memory region while transfer buffers
 * are maintained with explicit cache operations.
 */
#define OHCI_DMA_COHERENT DT_ALL_INST_HAS_BOOL_STATUS_OKAY(dma_coherent)

#if OHCI_DMA_COHERENT
#define OHCI_DMA_MEM_SECTION
#else
BUILD_ASSERT(!IS_ENABLED(CONFIG_DCACHE) || IS_ENABLED(CONFIG_NOCACHE_MEMORY),
	     "Non-coherent OHCI controllers need the nocache memory region for the descriptors");
#define OHCI_DMA_MEM_SECTION __nocache
#endif

/* 7 Host Controller Operational Registers (Hc* Register Offsets) */
#define OHCI_REVISION                  0x00U
#define OHCI_CONTROL                   0x04U
#define OHCI_CMDSTATUS                 0x08U
#define OHCI_INTRSTATUS                0x0cU
#define OHCI_INTRENABLE                0x10U
#define OHCI_INTRDISABLE               0x14U
#define OHCI_HCCA                      0x18U
#define OHCI_CONTROL_HEAD_ED           0x20U
#define OHCI_CONTROL_CURRENT_ED        0x24U
#define OHCI_BULK_HEAD_ED              0x28U
#define OHCI_FM_INTERVAL               0x34U
#define OHCI_FM_NUMBER                 0x3cU
#define OHCI_PERIODIC_START            0x40U
#define OHCI_LS_THRESHOLD              0x44U
#define OHCI_RH_DESC_A                 0x48U
#define OHCI_RH_DESC_B                 0x4cU
#define OHCI_RH_STATUS                 0x50U
#define OHCI_RH_PORT_STATUS(n)         (0x54U + ((n) * 4U))

/* 7.1.2 HcControl Register Bit Definitions */
#define OHCI_CONTROL_PLE               BIT(2)
#define OHCI_CONTROL_CLE               BIT(4)
#define OHCI_CONTROL_BLE               BIT(5)
#define OHCI_CONTROL_HCFS_MASK         GENMASK(7, 6)
#define OHCI_CONTROL_HCFS_RESET        FIELD_PREP(OHCI_CONTROL_HCFS_MASK, 0U)
#define OHCI_CONTROL_HCFS_RESUME       FIELD_PREP(OHCI_CONTROL_HCFS_MASK, 1U)
#define OHCI_CONTROL_HCFS_OPERATIONAL  FIELD_PREP(OHCI_CONTROL_HCFS_MASK, 2U)
#define OHCI_CONTROL_HCFS_SUSPEND      FIELD_PREP(OHCI_CONTROL_HCFS_MASK, 3U)

/* 7.1.3 HcCommandStatus Register Bit Definitions */
#define OHCI_CMDSTATUS_HCR             BIT(0)
#define OHCI_CMDSTATUS_CLF             BIT(1)
#define OHCI_CMDSTATUS_BLF             BIT(2)

/* 7.1.4/5 HcInterruptStatus/HcInterruptEnable Interrupt Bits */
#define OHCI_INTR_WDH                  BIT(1)
#define OHCI_INTR_RD                   BIT(3)
#define OHCI_INTR_UE                   BIT(4)
#define OHCI_INTR_RHSC                 BIT(6)
#define OHCI_INTR_MIE                  BIT(31)

/* 7.4.1 HcRhDescriptorA Fields (NDP = Number of Downstream Ports) */
#define OHCI_RHDA_NDP_MASK             GENMASK(7, 0)
#define OHCI_RHDA_PSM                  BIT(8)
#define OHCI_RHDA_NPS                  BIT(9)
#define OHCI_RHDA_OCPM                 BIT(11)
#define OHCI_RHDA_NOCP                 BIT(12)
#define OHCI_RHDA_POTPGT_MASK          GENMASK(31, 24)
#define OHCI_RHDB_DR_MASK              GENMASK(15, 0)
#define OHCI_RHDB_PPCM_MASK            GENMASK(31, 16)
#define OHCI_RHS_OCI                   BIT(1)
#define OHCI_RHS_LPSC                  BIT(16)
#define OHCI_RHS_OCIC                  BIT(17)

/* 7.4.4 HcRhPortStatus Bit Definitions and Write-1-to-Clear Change Bits */
#define OHCI_RHPS_CCS                  BIT(0)
#define OHCI_RHPS_PES                  BIT(1)
#define OHCI_RHPS_PSS                  BIT(2)
#define OHCI_RHPS_POCI                 BIT(3)
#define OHCI_RHPS_PRS                  BIT(4)
#define OHCI_RHPS_PPS                  BIT(8)
#define OHCI_RHPS_LSDA                 BIT(9)
#define OHCI_RHPS_CSC                  BIT(16)
#define OHCI_RHPS_PESC                 BIT(17)
#define OHCI_RHPS_PSSC                 BIT(18)
#define OHCI_RHPS_OCIC                 BIT(19)
#define OHCI_RHPS_PRSC                 BIT(20)
#define OHCI_RHPS_W1C_MASK             (OHCI_RHPS_CSC | OHCI_RHPS_PESC | OHCI_RHPS_PSSC | \
					OHCI_RHPS_OCIC | OHCI_RHPS_PRSC)
#define OHCI_RHPS_STATUS_MASK  GENMASK(15, 0)
#define OHCI_RHPS_CHANGE_SHIFT 16U

/* 4.2.2 Endpoint Descriptor Field Definitions */
#define OHCI_ED_FA_MASK                GENMASK(6, 0)
#define OHCI_ED_EN_MASK                GENMASK(10, 7)
#define OHCI_ED_D_MASK                 GENMASK(12, 11)
#define OHCI_ED_D_FROM_TD              0U
#define OHCI_ED_D_OUT                  1U
#define OHCI_ED_D_IN                   2U
#define OHCI_ED_SPEED                  BIT(13)
#define OHCI_ED_SKIP                   BIT(14)
#define OHCI_ED_MPS_MASK               GENMASK(31, 16)
#define OHCI_ED_HEAD_HALTED            BIT(0)
#define OHCI_ED_HEAD_CARRY             BIT(1)
#define OHCI_ED_HEAD_PTR_MASK          GENMASK(31, 4)

/* 4.3.1 General Transfer Descriptor Field Definitions */
#define OHCI_TD_CC_MASK                GENMASK(31, 28)
#define OHCI_TD_CC_NO_ERROR            0U
#define OHCI_TD_CC_STALL               4U
#define OHCI_TD_CC_DEV_NOT_RESPONDING  5U
#define OHCI_TD_CC_DATA_UNDERRUN       9U
#define OHCI_TD_CC_NOT_ACCESSED        15U
#define OHCI_TD_T_MASK                 GENMASK(25, 24)
#define OHCI_TD_T_TOGGLE_ED            0U
#define OHCI_TD_T_DATA0                2U
#define OHCI_TD_T_DATA1                3U
#define OHCI_TD_DI_MASK                GENMASK(23, 21)
#define OHCI_TD_DI_NO_INTERRUPT        7U
#define OHCI_TD_DP_MASK                GENMASK(20, 19)
#define OHCI_TD_DP_SETUP               0U
#define OHCI_TD_DP_OUT                 1U
#define OHCI_TD_DP_IN                  2U
#define OHCI_TD_R                      BIT(18)
#define OHCI_TD_PTR_MASK               GENMASK(31, 4)

#define OHCI_RESET_TIMEOUT_US          10000U
#define OHCI_BUS_RESET_TIME_MS         50U
/*
 * 7.3.1 HcFmInterval: 12000 bit times per frame and the largest data packet
 * that fits into a frame (FSLargestDataPacket), 6/7 of the frame minus the
 * maximum overhead of 210 bit times. Without the latter the controller never
 * starts a transaction.
 */
#define OHCI_FRAME_INTERVAL            0x2edfU
#define OHCI_FSMPS                     ((6U * (OHCI_FRAME_INTERVAL - 210U)) / 7U)
#define OHCI_DEFAULT_FMINTERVAL        (OHCI_FRAME_INTERVAL | (OHCI_FSMPS << 16))
#define OHCI_DEFAULT_PERIODIC_START    0x3e67U
#define OHCI_DEFAULT_LS_THRESHOLD      0x0628U
#define OHCI_PCI_CMD_INTX_DISABLE      BIT(10)

struct ohci_hw_ed {
	uint32_t flags;
	uint32_t tailp;
	uint32_t headp;
	uint32_t next;
} __aligned(16);

struct ohci_hw_td {
	uint32_t flags;
	uint32_t cbp;
	uint32_t next;
	uint32_t be;
} __aligned(16);

struct ohci_hcca {
	uint32_t intr_table[32];
	uint16_t frame_no;
	uint16_t pad;
	uint32_t done_head;
} __aligned(256);

struct ohci_td {
	struct ohci_hw_td hw;
	uintptr_t data;
	uint16_t len;
	uint8_t allow_short;
	uint8_t dir_in;
};

struct ohci_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
#ifdef CONFIG_UHC_OHCI_PCI
	struct pcie_dev *pcie;
#endif
	/* Optional bus clock, NULL if the controller has no clocks property */
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	/* Optional reset line, dev is NULL if the controller has no resets property */
	struct reset_dt_spec reset;
	void (*irq_enable_func)(const struct device *dev);
};

/*
 * One transfer slot: an ED plus its private TD pool.
 * ed must be the first field so the struct's alignment satisfies the OHCI
 * hardware requirement of 16-byte-aligned EDs.
 */
struct ohci_xfer_slot {
	struct ohci_hw_ed    ed; /* Must be first — requires 16-byte alignment */
	struct ohci_td       tds[CONFIG_UHC_OHCI_MAX_TDS];
	struct uhc_transfer *xfer;
	uint16_t             td_count;
	/*
	 * Copy of the setup packet, in DMA-safe memory. Keep it aligned, the
	 * nocache memory may be mapped as device memory without support for
	 * unaligned accesses.
	 */
	uint8_t              setup[8] __aligned(8);
};

/*
 * slot[OHCI_CTRL_SLOT] = control transfers (OHCI control list)
 * slot[1 .. OHCI_MAX_SLOTS-1] = bulk/interrupt transfers (OHCI bulk list)
 */
#define OHCI_CTRL_SLOT  0U
#define OHCI_MAX_SLOTS  (1U + CONFIG_UHC_OHCI_BULK_SLOTS)

/* Memory accessed by the host controller, placed in DMA-safe memory */
struct ohci_dma_mem {
	struct ohci_hcca      hcca;
	struct ohci_xfer_slot slots[OHCI_MAX_SLOTS];
};

struct ohci_data {
	/* this needs to be first */
	struct uhc_data       uhc_data;
	DEVICE_MMIO_NAMED_RAM(reg_base);
	struct k_spinlock     lock;
	struct ohci_hcca      *hcca;
	struct ohci_xfer_slot *slots;
	uint8_t               bulk_toggle[128][32];
	uint8_t               port_count;
	bool                  bus_suspended;
	/* Root hub emulation, see below */
	struct usb_device *rh_udev;
	struct uhc_transfer *rh_int_xfer;
	bool rh_connected;
};

static inline uintptr_t ohci_base(const struct device *dev)
{
	return DEVICE_MMIO_NAMED_GET(dev, reg_base);
}

static inline bool ohci_rh_is_xfer(const struct ohci_data *data,
				   const struct uhc_transfer *const xfer);

static inline uint32_t ohci_td_cc(const struct ohci_hw_td *td)
{
	return FIELD_GET(OHCI_TD_CC_MASK, td->flags);
}

static inline uint32_t ohci_td_phys(struct ohci_td *td)
{
	return (uint32_t)k_mem_phys_addr(&td->hw);
}

static inline uint8_t ohci_ep_toggle_idx(uint8_t ep)
{
	return USB_EP_GET_IDX(ep) + (USB_EP_DIR_IS_IN(ep) ? 16U : 0U);
}

static inline uint32_t ohci_bulk_toggle_get(struct ohci_data *data, struct uhc_transfer *xfer)
{
	return data->bulk_toggle[xfer->udev->addr][ohci_ep_toggle_idx(xfer->ep)] != 0U ?
		OHCI_TD_T_DATA1 : OHCI_TD_T_DATA0;
}

static inline void ohci_bulk_toggle_save(struct ohci_data *data, struct uhc_transfer *xfer,
					 uint32_t ed_headp)
{
	data->bulk_toggle[xfer->udev->addr][ohci_ep_toggle_idx(xfer->ep)] =
		(ed_headp & OHCI_ED_HEAD_CARRY) != 0U;
}

static inline uint32_t ohci_phys_addr(void *addr)
{
	return (uint32_t)k_mem_phys_addr(addr);
}

/* Write back a buffer the host controller is going to read */
static inline void ohci_buf_flush(void *addr, size_t len)
{
	if (!OHCI_DMA_COHERENT && len != 0U) {
		sys_cache_data_flush_range(addr, len);
	}
}

/* Drop cached lines of a buffer the host controller is going to write */
static inline void ohci_buf_prepare_in(void *addr, size_t len)
{
	if (!OHCI_DMA_COHERENT && len != 0U) {
		sys_cache_data_flush_and_invd_range(addr, len);
	}
}

/* Drop cached lines of a buffer the host controller has written */
static inline void ohci_buf_invd(void *addr, size_t len)
{
	if (!OHCI_DMA_COHERENT && len != 0U) {
		sys_cache_data_invd_range(addr, len);
	}
}

static int ohci_lock(const struct device *dev)
{
	return uhc_lock_internal(dev, K_FOREVER);
}

static int ohci_unlock(const struct device *dev)
{
	return uhc_unlock_internal(dev);
}

static int ohci_td_status_to_errno(uint32_t cc, bool allow_short)
{
	switch (cc) {
	case OHCI_TD_CC_NO_ERROR:
		return 0;
	case OHCI_TD_CC_DATA_UNDERRUN:
		return allow_short ? 0 : -EIO;
	case OHCI_TD_CC_STALL:
		return -EPIPE;
	case OHCI_TD_CC_DEV_NOT_RESPONDING:
		return -ETIMEDOUT;
	default:
		return -EIO;
	}
}

static size_t ohci_td_actual_len(const struct ohci_td *td)
{
	uint32_t cbp;

	if (td->len == 0U) {
		return 0U;
	}

	cbp = td->hw.cbp;
	if (cbp == 0U) {
		return td->len;
	}

	if (cbp <= td->data) {
		return 0U;
	}

	return MIN((size_t)(cbp - td->data), (size_t)td->len);
}

static void ohci_clear_slot(struct ohci_xfer_slot *slot)
{
	/*
	 * Reset the ED so the HC skips it cleanly while it holds no TDs.
	 *
	 * Order matters:
	 *   1. Set OHCI_ED_SKIP first so the HC will skip this ED if it is
	 *      traversing the list concurrently.
	 *   2. Clear headp/tailp to mark the TD queue as empty.
	 *   3. Leave ed.next UNTOUCHED — bulk slot EDs are pre-linked into a
	 *      permanent chain by ohci_setup_bulk_list(); zeroing next would
	 *      break the chain the moment any slot completes a transfer.
	 *
	 * For the control slot (slot 0) ed.next is always 0, so no-op.
	 */
	slot->ed.flags = OHCI_ED_SKIP;
	compiler_barrier();
	slot->ed.headp = 0U;
	slot->ed.tailp = 0U;
	/* ed.next deliberately not modified */

	memset(slot->tds, 0, sizeof(slot->tds));
	slot->td_count = 0U;
	slot->xfer = NULL;
}

static void ohci_clear_all_slots(struct ohci_data *data)
{
	for (size_t i = 0U; i < OHCI_MAX_SLOTS; i++) {
		ohci_clear_slot(&data->slots[i]);
	}
}

static int ohci_wait_reset(const struct device *dev)
{
	for (uint32_t t = 0U; t < OHCI_RESET_TIMEOUT_US; t += 10U) {
		if ((sys_read32(ohci_base(dev) + OHCI_CMDSTATUS) & OHCI_CMDSTATUS_HCR) == 0U) {
			return 0;
		}

		k_busy_wait(10U);
	}

	return -ETIMEDOUT;
}

/*
 * A TD covers up to two physical pages, the current one from addr and the
 * following one, 4.3.1.2 Current Buffer Pointer and Buffer End. The buffers
 * are physically contiguous, so the second page is the next one.
 */
static size_t ohci_chunk_len(const uint8_t *addr, size_t remaining)
{
	size_t max_len = (0x1000U - ((uintptr_t)addr & 0xfffU)) + 0x1000U;

	return MIN(remaining, max_len);
}

static void ohci_fill_td(struct ohci_td *td, uint8_t *data, size_t len,
				 uint32_t dp, uint32_t toggle, bool allow_short,
				 bool dir_in, bool interrupt_on_done)
{
	uint32_t flags =
		FIELD_PREP(OHCI_TD_CC_MASK, OHCI_TD_CC_NOT_ACCESSED) |
		FIELD_PREP(OHCI_TD_T_MASK, toggle) | FIELD_PREP(OHCI_TD_DP_MASK, dp) |
		FIELD_PREP(OHCI_TD_DI_MASK, interrupt_on_done ? 0U : OHCI_TD_DI_NO_INTERRUPT);

	if (allow_short) {
		flags |= OHCI_TD_R;
	}

	td->hw.flags = flags;
	td->hw.cbp = (len != 0U) ? ohci_phys_addr(data) : 0U;
	td->hw.be = (len != 0U) ? ohci_phys_addr(&data[len - 1U]) : 0U;
	td->data = (len != 0U) ? ohci_phys_addr(data) : 0U;
	td->len = (uint16_t)len;
	td->allow_short = allow_short;
	td->dir_in = dir_in;
	td->hw.next = 0U;
}

static int ohci_build_control_chain(struct ohci_xfer_slot *slot, struct uhc_transfer *xfer)
{
	uint8_t *buffer = NULL;
	size_t remaining = 0U;
	uint8_t *ptr = NULL;
	uint16_t td_idx = 0U;
	uint32_t toggle = OHCI_TD_T_DATA1;
	bool dir_in = USB_EP_DIR_IS_IN(xfer->ep);
	const struct usb_setup_packet *setup = (const struct usb_setup_packet *)xfer->setup_pkt;
	uint16_t w_length = sys_le16_to_cpu(setup->wLength);

	memcpy(slot->setup, xfer->setup_pkt, sizeof(slot->setup));
	ohci_fill_td(&slot->tds[td_idx], slot->setup, sizeof(slot->setup),
		     OHCI_TD_DP_SETUP, OHCI_TD_T_DATA0, false, false, false);
	td_idx++;

	if (xfer->buf != NULL) {
		if (dir_in) {
			buffer = net_buf_tail(xfer->buf);
			remaining = MIN((size_t)w_length, net_buf_tailroom(xfer->buf));
			ohci_buf_prepare_in(buffer, remaining);
		} else {
			buffer = xfer->buf->data;
			remaining = MIN((size_t)w_length, (size_t)xfer->buf->len);
			ohci_buf_flush(buffer, remaining);
		}

		ptr = buffer;
		while (remaining != 0U) {
			size_t chunk;

			if (td_idx >= (CONFIG_UHC_OHCI_MAX_TDS - 1)) {
				return -ENOMEM;
			}

			/*
			 * The whole data stage fits into one TD in practice. A
			 * short IN packet retires the TD and the HC continues
			 * with the status stage TD, 4.3.1.3.5 Transfer Completion.
			 */
			chunk = ohci_chunk_len(ptr, remaining);
			ohci_fill_td(&slot->tds[td_idx], ptr, chunk,
				     dir_in ? OHCI_TD_DP_IN : OHCI_TD_DP_OUT,
				     toggle, dir_in, dir_in, false);
			td_idx++;
			ptr += chunk;
			remaining -= chunk;
			toggle = (toggle == OHCI_TD_T_DATA1) ? OHCI_TD_T_DATA0 : OHCI_TD_T_DATA1;
		}
	}

	if (!xfer->no_status) {
		if (td_idx >= (CONFIG_UHC_OHCI_MAX_TDS - 1)) {
			return -ENOMEM;
		}

		ohci_fill_td(&slot->tds[td_idx], NULL, 0U,
			     dir_in ? OHCI_TD_DP_OUT : OHCI_TD_DP_IN,
			     OHCI_TD_T_DATA1, false, !dir_in, true);
		td_idx++;
	} else {
		slot->tds[td_idx - 1U].hw.flags &= ~OHCI_TD_DI_MASK;
	}

	slot->td_count = (uint16_t)(td_idx + 1U);
	return 0;
}

static int ohci_build_bulk_chain(struct ohci_xfer_slot *slot, struct uhc_transfer *xfer)
{
	uint8_t *buffer;
	size_t remaining;
	uint8_t *ptr;
	uint16_t td_idx = 0U;
	bool dir_in = USB_EP_DIR_IS_IN(xfer->ep);

	if (xfer->buf == NULL) {
		return -EINVAL;
	}

	if (dir_in) {
		buffer = net_buf_tail(xfer->buf);
		remaining = net_buf_tailroom(xfer->buf);
		ohci_buf_prepare_in(buffer, remaining);
	} else {
		buffer = xfer->buf->data;
		remaining = xfer->buf->len;
		ohci_buf_flush(buffer, remaining);
	}

	if (remaining == 0U) {
		if (dir_in) {
			return -EINVAL;
		}

		/* Zero-length OUT packet: a single TD without a data buffer. */
		ohci_fill_td(&slot->tds[0], NULL, 0U, OHCI_TD_DP_OUT, OHCI_TD_T_TOGGLE_ED,
			     false, false, true);
		slot->td_count = 2U;
		return 0;
	}

	ptr = buffer;
	while (remaining != 0U) {
		size_t chunk;

		if (td_idx >= (CONFIG_UHC_OHCI_MAX_TDS - 1)) {
			return -ENOMEM;
		}

		/* Bulk TDs carry multiple packets, split by pages, not by MPS. */
		chunk = ohci_chunk_len(ptr, remaining);
		/*
		 * IN transfers complete on the first short packet, so every IN
		 * TD interrupts on completion. OUT transfers only interrupt on
		 * the last TD.
		 */
		ohci_fill_td(&slot->tds[td_idx], ptr, chunk,
			     dir_in ? OHCI_TD_DP_IN : OHCI_TD_DP_OUT,
			     OHCI_TD_T_TOGGLE_ED,
			     dir_in, dir_in, dir_in);
		ptr += chunk;
		remaining -= chunk;
		td_idx++;
	}

	/*
	 * A short packet must end the transfer, but with buffer rounding set
	 * the HC just retires the TD and continues with the next one. So only
	 * the last IN TD gets buffer rounding, a short packet on an earlier TD
	 * completes with DataUnderrun and halts the ED, which stops the chain.
	 * The completion handler treats it as a successful short transfer.
	 */
	if (dir_in) {
		for (uint16_t i = 0U; i < (td_idx - 1U); i++) {
			slot->tds[i].hw.flags &= ~OHCI_TD_R;
		}
	}

	slot->tds[td_idx - 1U].hw.flags &= ~OHCI_TD_DI_MASK;
	slot->td_count = (uint16_t)(td_idx + 1U);
	return 0;
}

static void ohci_link_chain(struct ohci_data *data, struct ohci_xfer_slot *slot,
			    struct uhc_transfer *xfer)
{
	uint32_t ed_flags = FIELD_PREP(OHCI_ED_FA_MASK, xfer->udev->addr) |
			    FIELD_PREP(OHCI_ED_EN_MASK, USB_EP_GET_IDX(xfer->ep)) |
			    FIELD_PREP(OHCI_ED_MPS_MASK, xfer->mps) |
			    FIELD_PREP(OHCI_ED_D_MASK, OHCI_ED_D_FROM_TD);

	for (uint16_t i = 0U; i < (slot->td_count - 1U); i++) {
		slot->tds[i].hw.next = ohci_td_phys(&slot->tds[i + 1U]);
	}
	memset(&slot->tds[slot->td_count - 1U], 0, sizeof(slot->tds[0]));

	if (xfer->udev->speed == USB_SPEED_SPEED_LS) {
		ed_flags |= OHCI_ED_SPEED;
	}

	/*
	 * Preserve the next pointer so the pre-linked bulk chain written at
	 * enable time is not clobbered.  The ed.next field is set up once in
	 * ohci_setup_bulk_list() and must not be touched here.
	 */
	slot->ed.flags = ed_flags;
	slot->ed.headp = ohci_td_phys(&slot->tds[0]);
	/* Bulk and interrupt endpoints keep their data toggle across transfers */
	if (xfer->type != USB_EP_TYPE_CONTROL &&
	    ohci_bulk_toggle_get(data, xfer) == OHCI_TD_T_DATA1) {
		slot->ed.headp |= OHCI_ED_HEAD_CARRY;
	}
	slot->ed.tailp = ohci_td_phys(&slot->tds[slot->td_count - 1U]);
}

/* Return true if xfer is currently being processed in any slot. */
static bool ohci_xfer_is_active(const struct ohci_data *data,
				const struct uhc_transfer *xfer)
{
	for (size_t i = 0U; i < OHCI_MAX_SLOTS; i++) {
		if (data->slots[i].xfer == xfer) {
			return true;
		}
	}
	return false;
}

/* Return true if the endpoint of the transfer's device already has an active slot. */
static bool ohci_ep_is_active(const struct ohci_data *data, const struct uhc_transfer *xfer)
{
	for (size_t i = 0U; i < OHCI_MAX_SLOTS; i++) {
		const struct uhc_transfer *active = data->slots[i].xfer;

		if (active != NULL && active->udev == xfer->udev && active->ep == xfer->ep) {
			return true;
		}
	}
	return false;
}

/*
 * Write the permanent bulk-list head once and pre-link all bulk slot EDs with
 * the SKIP bit set.  The head register never needs to be updated afterwards;
 * scheduling a new bulk transfer just clears the SKIP bit and sets BLF.
 */
static void ohci_setup_bulk_list(const struct device *dev)
{
	struct ohci_data *data = dev->data;

	for (size_t i = 1U; i < OHCI_MAX_SLOTS; i++) {
		data->slots[i].ed.flags = OHCI_ED_SKIP;
		data->slots[i].ed.headp = 0U;
		data->slots[i].ed.tailp = 0U;
		data->slots[i].ed.next =
			(i + 1U < OHCI_MAX_SLOTS)
			? ohci_phys_addr(&data->slots[i + 1U].ed)
			: 0U;
	}
	compiler_barrier();
	sys_write32((OHCI_MAX_SLOTS > 1U) ? ohci_phys_addr(&data->slots[1U].ed) : 0U,
		    ohci_base(dev) + OHCI_BULK_HEAD_ED);
}

static void ohci_finish_slot(const struct device *dev, struct ohci_xfer_slot *slot)
{
	struct ohci_data *data = dev->data;
	struct uhc_transfer *xfer = slot->xfer;
	uint32_t ed_headp;
	int err = 0;
	size_t actual_in = 0U;
	bool is_ctrl;

	if (xfer == NULL) {
		return;
	}

	compiler_barrier();
	ed_headp = slot->ed.headp;
	if ((ed_headp & OHCI_ED_HEAD_HALTED) == 0U &&
	    (ed_headp & OHCI_ED_HEAD_PTR_MASK) != slot->ed.tailp) {
		return;
	}

	for (uint16_t i = 0U; i < (slot->td_count - 1U); i++) {
		uint32_t cc = ohci_td_cc(&slot->tds[i].hw);

		/*
		 * NOT_ACCESSED means the HC never processed this TD, normal
		 * when a short packet on an earlier TD halted the ED. Treat it
		 * as the end of the transfer, no error.
		 */
		if (cc == OHCI_TD_CC_NOT_ACCESSED) {
			break;
		}

		err = ohci_td_status_to_errno(cc, slot->tds[i].allow_short != 0U);
		if (err != 0) {
			LOG_INF("TD %u completion code %u -> err %d", i, cc, err);
			break;
		}

		if (slot->tds[i].dir_in != 0U) {
			actual_in += ohci_td_actual_len(&slot->tds[i]);
		}
	}

	if (err == 0 && xfer->type != USB_EP_TYPE_CONTROL) {
		ohci_bulk_toggle_save(data, xfer, ed_headp);
	}

	if (err == 0 && xfer->buf != NULL && USB_EP_DIR_IS_IN(xfer->ep) && actual_in != 0U) {
		ohci_buf_invd(net_buf_tail(xfer->buf), actual_in);
		net_buf_add(xfer->buf, actual_in);
	}

	is_ctrl = (slot == &data->slots[OHCI_CTRL_SLOT]);
	LOG_DBG("Complete xfer ep 0x%02x err %d td_count %u actual_in %zu slot %u",
		xfer->ep, err, slot->td_count, actual_in,
		(unsigned int)(slot - data->slots));

	if (is_ctrl) {
		sys_write32(0U, ohci_base(dev) + OHCI_CONTROL_HEAD_ED);
	}

	/*
	 * ohci_clear_slot() preserves ed.next and sets OHCI_ED_SKIP, so the
	 * HC will skip this ED cleanly after the clear.
	 */
	compiler_barrier();
	ohci_clear_slot(slot);
	uhc_xfer_return(dev, xfer, err);
}

/* Find a free slot for the given transfer type.  Returns NULL if none free. */
static struct ohci_xfer_slot *ohci_alloc_slot(struct ohci_data *data, bool is_ctrl)
{
	if (is_ctrl) {
		return (data->slots[OHCI_CTRL_SLOT].xfer == NULL)
			? &data->slots[OHCI_CTRL_SLOT] : NULL;
	}
	for (size_t i = 1U; i < OHCI_MAX_SLOTS; i++) {
		if (data->slots[i].xfer == NULL) {
			return &data->slots[i];
		}
	}
	return NULL;
}

static int ohci_try_schedule_next(const struct device *dev)
{
	struct ohci_data *data = dev->data;
	struct uhc_transfer *xfer, *tmp;
	struct ohci_xfer_slot *slot;
	int ret;
	bool any_ctrl_free, any_bulk_free;

	if (!uhc_is_enabled(dev)) {
		return 0;
	}

	any_ctrl_free = (data->slots[OHCI_CTRL_SLOT].xfer == NULL);
	any_bulk_free = (ohci_alloc_slot(data, false) != NULL);

	if (!any_ctrl_free && !any_bulk_free) {
		return 0;
	}

	/*
	 * Walk the pending queue.  Use the _SAFE variant because
	 * uhc_xfer_return() may remove the node mid-iteration.
	 */
	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&data->uhc_data.ctrl_xfers, xfer, tmp, node) {
		bool is_ctrl = (xfer->type == USB_EP_TYPE_CONTROL);

		/* The root hub transfers are handled in software. */
		if (ohci_rh_is_xfer(data, xfer)) {
			continue;
		}

		/* Skip transfers already being processed in a slot. */
		if (ohci_xfer_is_active(data, xfer)) {
			continue;
		}

		/* Drain transfers cancelled before they were scheduled. */
		if (xfer->err == -ECONNRESET) {
			uhc_xfer_return(dev, xfer, -ECONNRESET);
			continue;
		}

		/* Check whether a suitable slot is available. */
		slot = ohci_alloc_slot(data, is_ctrl);
		if (slot == NULL) {
			continue;
		}

		/* Don't schedule a second transfer for the same endpoint. */
		if (ohci_ep_is_active(data, xfer)) {
			continue;
		}

		LOG_DBG("Schedule xfer ep 0x%02x type %u mps %u slot %u",
			xfer->ep, xfer->type, xfer->mps,
			(unsigned int)(slot - data->slots));

		if (is_ctrl) {
			ret = ohci_build_control_chain(slot, xfer);
		} else if (xfer->type == USB_EP_TYPE_BULK ||
			   xfer->type == USB_EP_TYPE_INTERRUPT) {
			ret = ohci_build_bulk_chain(slot, xfer);
		} else {
			ret = -ENOTSUP;
		}

		if (ret != 0) {
			LOG_DBG("Failed to build chain ep 0x%02x err %d", xfer->ep, ret);
			uhc_xfer_return(dev, xfer, ret);
			continue;
		}

		if (is_ctrl) {
			const struct usb_setup_packet *setup =
				(const struct usb_setup_packet *)xfer->setup_pkt;

			/*
			 * A new device gets a data toggle table without leftovers
			 * of a device that had the same address before.
			 */
			if (setup->bmRequestType == 0U && setup->bRequest == USB_SREQ_SET_ADDRESS) {
				uint16_t addr = sys_le16_to_cpu(setup->wValue);

				if (addr < ARRAY_SIZE(data->bulk_toggle)) {
					memset(data->bulk_toggle[addr], 0,
					       sizeof(data->bulk_toggle[addr]));
				}
			}
		}

		ohci_link_chain(data, slot, xfer);
		slot->xfer = xfer;

		LOG_DBG("ED flags=0x%08x head=0x%08x tail=0x%08x count=%u",
			slot->ed.flags, slot->ed.headp, slot->ed.tailp, slot->td_count);

		/* Ensure all TD/ED writes reach hardware before kicking the HC. */
		compiler_barrier();

		if (is_ctrl) {
			sys_write32(ohci_phys_addr(&slot->ed),
				    ohci_base(dev) + OHCI_CONTROL_HEAD_ED);
			sys_write32(OHCI_CMDSTATUS_CLF, ohci_base(dev) + OHCI_CMDSTATUS);
		} else {
			/*
			 * Clear the SKIP bit to make this ED visible to the HC.
			 * It is set by ohci_setup_bulk_list() and ohci_clear_slot().
			 */
			slot->ed.flags &= ~OHCI_ED_SKIP;
			compiler_barrier();
			sys_write32(OHCI_CMDSTATUS_BLF, ohci_base(dev) + OHCI_CMDSTATUS);
		}

		/* Refresh free-slot flags after allocation. */
		any_ctrl_free = (data->slots[OHCI_CTRL_SLOT].xfer == NULL);
		any_bulk_free = (ohci_alloc_slot(data, false) != NULL);
		if (!any_ctrl_free && !any_bulk_free) {
			break;
		}
	}

	return 0;
}

/*
 * Root hub emulation
 *
 * The host stack handles a single device behind a controller, so the root
 * hub of the controller is presented to it as a full-speed hub device, like
 * Linux does. The hub class then owns the root hub ports: it powers, resets
 * and polls them through the hub requests, which are answered from the root
 * hub registers here, so every port can carry a device at the same time.
 */
#define OHCI_RH_EP_IN_ADDR 0x81U
#define OHCI_RH_EP_IN_MPS  2U
#define OHCI_RH_MAX_PORTS  15U

BUILD_ASSERT(DIV_ROUND_UP(OHCI_RH_MAX_PORTS + 1U, 8U) <= OHCI_RH_EP_IN_MPS,
	     "Status change bitmap does not fit the endpoint");

static const uint8_t ohci_rh_device_desc[] = {
	18U,         USB_DESC_DEVICE,
	0x10U,       0x01U, /* bcdUSB 1.10 */
	USB_BCC_HUB, 0x00U,
	0x00U,       64U,   /* bMaxPacketSize0 */
	0x00U,       0x00U, /* idVendor */
	0x00U,       0x00U, /* idProduct */
	0x00U,       0x01U, /* bcdDevice */
	0U,          0U,
	0U, /* no strings */
	1U, /* bNumConfigurations */
};

static const uint8_t ohci_rh_config_desc[] = {
	/* Configuration */
	9U, USB_DESC_CONFIGURATION, 25U, 0x00U, /* wTotalLength */
	1U, 1U, 0U, 0xc0U,                      /* self powered */
	0U,
	/* Interface */
	9U, USB_DESC_INTERFACE, 0U, 0U, 1U, USB_BCC_HUB, 0x00U, 0x00U, 0U,
	/* Status change endpoint */
	7U, USB_DESC_ENDPOINT, OHCI_RH_EP_IN_ADDR, USB_EP_TYPE_INTERRUPT, OHCI_RH_EP_IN_MPS, 0x00U,
	255U, /* bInterval */
};

BUILD_ASSERT(sizeof(ohci_rh_config_desc) == 25U, "wTotalLength mismatch");

static size_t ohci_rh_hub_desc(const struct device *dev, uint8_t *const buf, const size_t size)
{
	struct ohci_data *data = dev->data;
	uint32_t rh_desc_a = sys_read32(ohci_base(dev) + OHCI_RH_DESC_A);
	uint32_t removable = sys_read32(ohci_base(dev) + OHCI_RH_DESC_B) & OHCI_RHDB_DR_MASK;
	size_t bitmap_len = (data->port_count / 8U) + 1U;
	size_t len = 7U + 2U * bitmap_len;
	uint16_t chars;

	if (size < len) {
		return 0U;
	}

	if ((rh_desc_a & OHCI_RHDA_NPS) != 0U) {
		chars = FIELD_PREP(USB_HUB_CHAR_LPSM_MASK, 2U);
	} else {
		chars = FIELD_PREP(USB_HUB_CHAR_LPSM_MASK,
				   (rh_desc_a & OHCI_RHDA_PSM) != 0U ? 1U : 0U);
	}

	if ((rh_desc_a & OHCI_RHDA_NOCP) != 0U) {
		chars |= FIELD_PREP(USB_HUB_CHAR_OCPM_MASK, 2U);
	} else {
		chars |= FIELD_PREP(USB_HUB_CHAR_OCPM_MASK,
				    (rh_desc_a & OHCI_RHDA_OCPM) != 0U ? 1U : 0U);
	}

	buf[0] = (uint8_t)len;
	buf[1] = USB_DESC_HUB;
	buf[2] = data->port_count;
	sys_put_le16(chars, &buf[3]);
	buf[5] = (uint8_t)FIELD_GET(OHCI_RHDA_POTPGT_MASK, rh_desc_a);
	buf[6] = 0U;

	for (size_t i = 0U; i < bitmap_len; i++) {
		/* DeviceRemovable, bit 0 is reserved, followed by PortPwrCtrlMask */
		buf[7U + i] = (uint8_t)(removable >> (8U * i));
		buf[7U + bitmap_len + i] = 0xffU;
	}

	return len;
}

/* Status change bitmap: bit 0 for the hub, bit n for port n */
static uint16_t ohci_rh_changes(const struct device *dev)
{
	struct ohci_data *data = dev->data;
	uint16_t bitmap = 0U;

	if ((sys_read32(ohci_base(dev) + OHCI_RH_STATUS) & OHCI_RHS_OCIC) != 0U) {
		bitmap |= BIT(0);
	}

	for (uint8_t port = 0U; port < data->port_count; port++) {
		uint32_t status = sys_read32(ohci_base(dev) + OHCI_RH_PORT_STATUS(port));

		if ((status & OHCI_RHPS_W1C_MASK) != 0U) {
			bitmap |= BIT(port + 1U);
		}
	}

	return bitmap;
}

static void ohci_rh_complete(const struct device *dev, struct uhc_transfer *const xfer,
			     const void *const src, size_t len, const int err)
{
	if ((err == 0) && (src != NULL) && (xfer->buf != NULL)) {
		len = MIN(len, net_buf_tailroom(xfer->buf));
		net_buf_add_mem(xfer->buf, src, len);
	}

	uhc_xfer_return(dev, xfer, err);
}

/*
 * Complete the parked status change transfer if there is anything to report,
 * otherwise wait for the next root hub status change interrupt. The
 * interrupt is disabled when it fires, as the change bits keep it asserted
 * until the hub class clears them.
 */
static void ohci_rh_status_changed(const struct device *dev)
{
	struct ohci_data *data = dev->data;
	struct uhc_transfer *xfer = data->rh_int_xfer;
	uint8_t bytes[OHCI_RH_EP_IN_MPS];
	uint16_t bitmap;

	if (xfer == NULL) {
		return;
	}

	bitmap = ohci_rh_changes(dev);
	if (bitmap == 0U) {
		sys_write32(OHCI_INTR_RHSC, ohci_base(dev) + OHCI_INTRENABLE);
		return;
	}

	LOG_DBG("Root hub status change 0x%04x", bitmap);
	data->rh_int_xfer = NULL;
	sys_put_le16(bitmap, bytes);
	ohci_rh_complete(dev, xfer, bytes, DIV_ROUND_UP(data->port_count + 1U, 8U), 0);
}

static int ohci_rh_port_feature(const struct device *dev, const uint8_t port,
				const uint16_t feature, const bool set)
{
	uint32_t val;

	if (set) {
		switch (feature) {
		case USB_HCFS_PORT_ENABLE:
			val = OHCI_RHPS_PES;
			break;
		case USB_HCFS_PORT_SUSPEND:
			val = OHCI_RHPS_PSS;
			break;
		case USB_HCFS_PORT_RESET:
			val = OHCI_RHPS_PRS;
			break;
		case USB_HCFS_PORT_POWER:
			val = OHCI_RHPS_PPS;
			break;
		default:
			return -EPIPE;
		}
	} else {
		switch (feature) {
		case USB_HCFS_PORT_ENABLE:
			val = OHCI_RHPS_CCS;
			break;
		case USB_HCFS_PORT_SUSPEND:
			val = OHCI_RHPS_POCI;
			break;
		case USB_HCFS_PORT_POWER:
			val = OHCI_RHPS_LSDA;
			break;
		case USB_HCFS_C_PORT_CONNECTION:
			val = OHCI_RHPS_CSC;
			break;
		case USB_HCFS_C_PORT_ENABLE:
			val = OHCI_RHPS_PESC;
			break;
		case USB_HCFS_C_PORT_SUSPEND:
			val = OHCI_RHPS_PSSC;
			break;
		case USB_HCFS_C_PORT_OVER_CURRENT:
			val = OHCI_RHPS_OCIC;
			break;
		case USB_HCFS_C_PORT_RESET:
			val = OHCI_RHPS_PRSC;
			break;
		default:
			return -EPIPE;
		}
	}

	/* The port status register is a set/clear register, only write the bit */
	sys_write32(val, ohci_base(dev) + OHCI_RH_PORT_STATUS(port));

	return 0;
}

static void ohci_rh_control(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct ohci_data *data = dev->data;
	const struct usb_setup_packet *setup = (const struct usb_setup_packet *)xfer->setup_pkt;
	uint16_t value = sys_le16_to_cpu(setup->wValue);
	uint16_t index = sys_le16_to_cpu(setup->wIndex);
	uint16_t length = sys_le16_to_cpu(setup->wLength);
	uint8_t type = (setup->bmRequestType >> 5) & 0x3U;
	uint8_t recipient = setup->bmRequestType & 0x1fU;
	uint8_t port = (uint8_t)index;
	uint8_t buf[7U + 2U * (OHCI_RH_MAX_PORTS / 8U + 1U)];
	const void *src = NULL;
	size_t len = 0U;
	uint32_t status;
	int err = 0;

	if (type == USB_REQTYPE_TYPE_STANDARD) {
		switch (setup->bRequest) {
		case USB_SREQ_GET_DESCRIPTOR:
			if ((value >> 8) == USB_DESC_DEVICE) {
				src = ohci_rh_device_desc;
				len = sizeof(ohci_rh_device_desc);
			} else if ((value >> 8) == USB_DESC_CONFIGURATION) {
				src = ohci_rh_config_desc;
				len = sizeof(ohci_rh_config_desc);
			} else {
				err = -EPIPE;
			}
			break;
		case USB_SREQ_GET_STATUS:
			/* Self powered, nothing else to report */
			sys_put_le16((recipient == USB_REQTYPE_RECIPIENT_DEVICE) ? 1U : 0U, buf);
			src = buf;
			len = 2U;
			break;
		case USB_SREQ_GET_CONFIGURATION:
			buf[0] = 1U;
			src = buf;
			len = 1U;
			break;
		case USB_SREQ_SET_ADDRESS:
			LOG_DBG("Root hub address %u", value);
			break;
		case USB_SREQ_SET_CONFIGURATION:
		case USB_SREQ_SET_INTERFACE:
		case USB_SREQ_SET_FEATURE:
		case USB_SREQ_CLEAR_FEATURE:
			break;
		default:
			err = -EPIPE;
			break;
		}
	} else if ((type == USB_REQTYPE_TYPE_CLASS) &&
		   ((recipient == USB_REQTYPE_RECIPIENT_DEVICE) || (port == 0U))) {
		switch (setup->bRequest) {
		case USB_HCREQ_GET_DESCRIPTOR:
			len = ohci_rh_hub_desc(dev, buf, sizeof(buf));
			src = buf;
			break;
		case USB_HCREQ_GET_STATUS:
			status = sys_read32(ohci_base(dev) + OHCI_RH_STATUS);
			sys_put_le16((status & OHCI_RHS_OCI) != 0U ? USB_HUB_STAT_OVER_CURRENT : 0U,
				     &buf[0]);
			sys_put_le16((status & OHCI_RHS_OCIC) != 0U ? USB_HUB_CHANGE_OVER_CURRENT
								    : 0U,
				     &buf[2]);
			src = buf;
			len = 4U;
			break;
		case USB_HCREQ_CLEAR_FEATURE:
			if (value == USB_HCFS_C_HUB_OVER_CURRENT) {
				sys_write32(OHCI_RHS_OCIC, ohci_base(dev) + OHCI_RH_STATUS);
			} else if (value != USB_HCFS_C_HUB_LOCAL_POWER) {
				err = -EPIPE;
			}
			break;
		default:
			err = -EPIPE;
			break;
		}
	} else if ((type == USB_REQTYPE_TYPE_CLASS) && (port <= data->port_count)) {
		switch (setup->bRequest) {
		case USB_HCREQ_GET_STATUS:
			status = sys_read32(ohci_base(dev) + OHCI_RH_PORT_STATUS(port - 1U));
			/* The port status and change bits match the hub class layout */
			sys_put_le16((uint16_t)(status & OHCI_RHPS_STATUS_MASK), &buf[0]);
			sys_put_le16((uint16_t)(status >> OHCI_RHPS_CHANGE_SHIFT), &buf[2]);
			src = buf;
			len = 4U;
			break;
		case USB_HCREQ_SET_FEATURE:
			err = ohci_rh_port_feature(dev, port - 1U, value, true);
			break;
		case USB_HCREQ_CLEAR_FEATURE:
			err = ohci_rh_port_feature(dev, port - 1U, value, false);
			break;
		default:
			err = -EPIPE;
			break;
		}
	} else {
		err = -EPIPE;
	}

	if (err != 0) {
		LOG_WRN("Root hub request 0x%02x 0x%02x not supported", setup->bmRequestType,
			setup->bRequest);
	}

	ohci_rh_complete(dev, xfer, src, MIN(len, length), err);
}

static void ohci_rh_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct ohci_data *data = dev->data;

	if (xfer->type == USB_EP_TYPE_CONTROL) {
		ohci_rh_control(dev, xfer);
	} else if (xfer->ep == OHCI_RH_EP_IN_ADDR) {
		if (data->rh_int_xfer != NULL) {
			uhc_xfer_return(dev, xfer, -EBUSY);
			return;
		}

		/* Report pending changes right away, otherwise wait for one */
		data->rh_int_xfer = xfer;
		ohci_rh_status_changed(dev);
	} else {
		uhc_xfer_return(dev, xfer, -EPIPE);
	}
}

static inline bool ohci_rh_is_xfer(const struct ohci_data *data,
				   const struct uhc_transfer *const xfer)
{
	return (data->rh_udev != NULL) && (xfer->udev == data->rh_udev);
}

/*
 * The ports are only enabled by the hub class once the root hub is
 * enumerated, so the first device at the default address is the root hub.
 */
static bool ohci_rh_adopt_xfer(struct ohci_data *data, const struct uhc_transfer *const xfer)
{
	if ((data->rh_udev == NULL) && data->rh_connected && (xfer->udev->addr == 0U)) {
		data->rh_udev = xfer->udev;
	}

	return ohci_rh_is_xfer(data, xfer);
}

static void ohci_rh_reset(const struct device *dev)
{
	struct ohci_data *data = dev->data;

	data->rh_udev = NULL;
	if (data->rh_int_xfer != NULL) {
		struct uhc_transfer *xfer = data->rh_int_xfer;

		data->rh_int_xfer = NULL;
		uhc_xfer_return(dev, xfer, -ECONNRESET);
	}
}

static void ohci_irq_handler(const struct device *dev, uint32_t irqs)
{
	struct ohci_data *data = dev->data;

	if ((irqs & OHCI_INTR_RHSC) != 0U) {
		sys_write32(OHCI_INTR_RHSC, ohci_base(dev) + OHCI_INTRDISABLE);
		ohci_rh_status_changed(dev);
	}

	if ((irqs & OHCI_INTR_WDH) != 0U) {
		uint32_t done_head = data->hcca->done_head;

		data->hcca->done_head = 0U;
		compiler_barrier();

		if (done_head != 0U) {
			/* Check all slots; more than one may have completed. */
			for (size_t i = 0U; i < OHCI_MAX_SLOTS; i++) {
				ohci_finish_slot(dev, &data->slots[i]);
			}
		}
	}

	if ((irqs & OHCI_INTR_RD) != 0U) {
		(void)uhc_submit_event(dev, UHC_EVT_RWUP, 0);
	}

	if ((irqs & OHCI_INTR_UE) != 0U) {
		(void)uhc_submit_event(dev, UHC_EVT_ERROR, -EIO);
	}

	(void)ohci_try_schedule_next(dev);
}

static void ohci_isr(const struct device *dev)
{
	struct ohci_data *data = dev->data;
	uint32_t irqs;

	while (true) {
		irqs = sys_read32(ohci_base(dev) + OHCI_INTRSTATUS);

		if (irqs == UINT32_MAX) {
			/* Spurious interrupt with all bits set, ignore */
			return;
		}
		/* Only handle interrupts that are currently enabled (Linux pattern) */
		irqs &= sys_read32(ohci_base(dev) + OHCI_INTRENABLE);
		if (irqs == 0U) {
			return;
		}

		/*
		 * Hold the spinlock for the entire handler + status-clear so
		 * that thread-context paths (ep_enqueue, ep_dequeue, disable)
		 * that also touch slots or the transfer list cannot race with
		 * ohci_finish_slot / ohci_try_schedule_next here.
		 */
		K_SPINLOCK(&data->lock) {
			ohci_irq_handler(dev, irqs);
			sys_write32(irqs, ohci_base(dev) + OHCI_INTRSTATUS);
		}
	}
}

static int ohci_init(const struct device *dev)
{
	struct ohci_data *data = dev->data;

	memset(data->hcca, 0, sizeof(*data->hcca));
	ohci_clear_all_slots(data);
	data->bus_suspended = false;
	data->rh_udev = NULL;
	data->rh_int_xfer = NULL;
	data->rh_connected = false;

	return 0;
}

/*
 * Power the root hub ports. Depending on the power switching mode of the
 * controller, the ports are powered by the global power switch (RhStatus
 * SetGlobalPower) or per port (RhPortStatus SetPortPower). Both are set, the
 * per port switching mode is selected for all ports when power switching is
 * supported. Then wait for the power to be good before the port status is
 * valid.
 */
static void ohci_power_ports(const struct device *dev)
{
	struct ohci_data *data = dev->data;
	uint32_t rh_desc_a = sys_read32(ohci_base(dev) + OHCI_RH_DESC_A);
	uint32_t potpgt_ms = FIELD_GET(OHCI_RHDA_POTPGT_MASK, rh_desc_a) * 2U;

	sys_write32(OHCI_RHS_LPSC, ohci_base(dev) + OHCI_RH_STATUS);

	if ((rh_desc_a & OHCI_RHDA_NPS) == 0U) {
		sys_write32(OHCI_RHDB_PPCM_MASK, ohci_base(dev) + OHCI_RH_DESC_B);
	}

	for (uint8_t port = 0U; port < data->port_count; port++) {
		sys_write32(OHCI_RHPS_PPS, ohci_base(dev) + OHCI_RH_PORT_STATUS(port));
	}

	LOG_DBG("RH desc A 0x%08x, power good after %u ms", rh_desc_a, potpgt_ms);
	if (potpgt_ms != 0U) {
		k_msleep(potpgt_ms);
	}
}

static int ohci_enable(const struct device *dev)
{
	struct ohci_data *data = dev->data;
	uint32_t control;

	sys_write32(OHCI_CMDSTATUS_HCR, ohci_base(dev) + OHCI_CMDSTATUS);
	if (ohci_wait_reset(dev) != 0) {
		return -ETIMEDOUT;
	}

	memset(data->hcca, 0, sizeof(*data->hcca));
	memset(data->bulk_toggle, 0, sizeof(data->bulk_toggle));
	ohci_clear_all_slots(data);
	sys_write32(0xffffffffU, ohci_base(dev) + OHCI_INTRDISABLE);
	sys_write32(0xffffffffU, ohci_base(dev) + OHCI_INTRSTATUS);
	sys_write32(OHCI_DEFAULT_FMINTERVAL, ohci_base(dev) + OHCI_FM_INTERVAL);
	sys_write32(OHCI_DEFAULT_PERIODIC_START, ohci_base(dev) + OHCI_PERIODIC_START);
	sys_write32(OHCI_DEFAULT_LS_THRESHOLD, ohci_base(dev) + OHCI_LS_THRESHOLD);
	sys_write32(ohci_phys_addr(data->hcca), ohci_base(dev) + OHCI_HCCA);

	/* Set up the permanent bulk-list chain before enabling the HC. */
	ohci_setup_bulk_list(dev);

	control = sys_read32(ohci_base(dev) + OHCI_CONTROL);
	control &= ~OHCI_CONTROL_HCFS_MASK;
	control |= OHCI_CONTROL_HCFS_OPERATIONAL | OHCI_CONTROL_CLE | OHCI_CONTROL_BLE;
	sys_write32(control, ohci_base(dev) + OHCI_CONTROL);

	ohci_power_ports(dev);

	sys_write32(OHCI_INTR_MIE | OHCI_INTR_WDH | OHCI_INTR_RD | OHCI_INTR_RHSC | OHCI_INTR_UE,
		    ohci_base(dev) + OHCI_INTRENABLE);

	/* The root hub is presented to the host stack as a permanently connected hub */
	data->rh_connected = true;

	return uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_FS, 0);
}

static int ohci_disable(const struct device *dev)
{
	struct ohci_data *data = dev->data;
	k_spinlock_key_t key;

	sys_write32(UINT32_MAX, ohci_base(dev) + OHCI_INTRDISABLE);
	sys_write32((sys_read32(ohci_base(dev) + OHCI_CONTROL) & ~OHCI_CONTROL_HCFS_MASK) |
		   OHCI_CONTROL_HCFS_RESET, ohci_base(dev) + OHCI_CONTROL);

	/*
	 * A pending IRQ may have been latched before INTRDISABLE took effect.
	 * Hold the spinlock so we cannot race with ohci_finish_slot in the ISR
	 * when clearing slots and returning transfers.
	 */
	key = k_spin_lock(&data->lock);
	for (size_t i = 0U; i < OHCI_MAX_SLOTS; i++) {
		if (data->slots[i].xfer != NULL) {
			struct uhc_transfer *xfer = data->slots[i].xfer;

			ohci_clear_slot(&data->slots[i]);
			uhc_xfer_return(dev, xfer, -ECONNRESET);
		}
	}
	ohci_rh_reset(dev);
	k_spin_unlock(&data->lock, key);

	if (data->rh_connected) {
		data->rh_connected = false;
		return uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
	}

	return 0;
}

static int ohci_shutdown(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/*
 * The host stack only resets the bus for the root device, which is the
 * emulated root hub. The ports are reset by the hub class instead.
 */
static int ohci_bus_reset(const struct device *dev)
{
	return uhc_submit_event(dev, UHC_EVT_RESETED, 0);
}

static int ohci_sof_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int ohci_bus_suspend(const struct device *dev)
{
	struct ohci_data *data = dev->data;
	uint32_t control;

	if (data->bus_suspended) {
		return -EALREADY;
	}

	control = sys_read32(ohci_base(dev) + OHCI_CONTROL) & ~OHCI_CONTROL_HCFS_MASK;
	sys_write32(control | OHCI_CONTROL_HCFS_SUSPEND, ohci_base(dev) + OHCI_CONTROL);
	data->bus_suspended = true;

	return uhc_submit_event(dev, UHC_EVT_SUSPENDED, 0);
}

static int ohci_bus_resume(const struct device *dev)
{
	struct ohci_data *data = dev->data;
	uint32_t control;

	if (!data->bus_suspended) {
		return -EALREADY;
	}

	control = sys_read32(ohci_base(dev) + OHCI_CONTROL) & ~OHCI_CONTROL_HCFS_MASK;
	sys_write32(control | OHCI_CONTROL_HCFS_RESUME, ohci_base(dev) + OHCI_CONTROL);
	k_msleep(20);
	sys_write32(control | OHCI_CONTROL_HCFS_OPERATIONAL |
		   OHCI_CONTROL_CLE | OHCI_CONTROL_BLE, ohci_base(dev) + OHCI_CONTROL);
	data->bus_suspended = false;

	return uhc_submit_event(dev, UHC_EVT_RESUMED, 0);
}

static int ohci_ep_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct ohci_data *data = DEV_DATA(dev);
	int ret;

	ret = uhc_xfer_append(dev, xfer);
	if (ret != 0) {
		LOG_DBG("Append xfer ep 0x%02x failed %d", xfer->ep, ret);
		return ret;
	}

	K_SPINLOCK(&data->lock) {
		if (ohci_rh_adopt_xfer(data, xfer)) {
			ohci_rh_enqueue(dev, xfer);
		} else {
			ret = ohci_try_schedule_next(dev);
		}
	}
	if (ret != 0) {
		LOG_DBG("Schedule xfer ep 0x%02x failed %d", xfer->ep, ret);
	}

	return ret;
}

static int ohci_ep_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct ohci_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (xfer == data->rh_int_xfer) {
		data->rh_int_xfer = NULL;
		uhc_xfer_return(dev, xfer, -ECONNRESET);
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	for (size_t i = 0U; i < OHCI_MAX_SLOTS; i++) {
		if (data->slots[i].xfer != xfer) {
			continue;
		}
		/*
		 * The host stack dequeues an in-flight transfer when a request
		 * times out. Stop the relevant list, free the slot, and return
		 * the transfer with -ECONNRESET so upper layers can recover.
		 */
		LOG_DBG("Cancel xfer ep 0x%02x slot %u: control 0x%08x cmdstatus 0x%08x "
			"intrstatus 0x%08x intrenable 0x%08x fmnumber 0x%08x hcca frame %u "
			"done 0x%08x current ed 0x%08x",
			xfer->ep, i, sys_read32(ohci_base(dev) + OHCI_CONTROL),
			sys_read32(ohci_base(dev) + OHCI_CMDSTATUS),
			sys_read32(ohci_base(dev) + OHCI_INTRSTATUS),
			sys_read32(ohci_base(dev) + OHCI_INTRENABLE),
			sys_read32(ohci_base(dev) + OHCI_FM_NUMBER), data->hcca->frame_no,
			data->hcca->done_head,
			sys_read32(ohci_base(dev) + OHCI_CONTROL_CURRENT_ED));
		LOG_DBG("ED flags 0x%08x head 0x%08x tail 0x%08x, TD0 flags 0x%08x cbp 0x%08x",
			data->slots[i].ed.flags, data->slots[i].ed.headp, data->slots[i].ed.tailp,
			data->slots[i].tds[0].hw.flags, data->slots[i].tds[0].hw.cbp);

		if (i == OHCI_CTRL_SLOT) {
			sys_write32(0U, ohci_base(dev) + OHCI_CONTROL_HEAD_ED);
		} else {
			data->slots[i].ed.flags |= OHCI_ED_SKIP;
			compiler_barrier();
		}
		ohci_clear_slot(&data->slots[i]);
		uhc_xfer_return(dev, xfer, -ECONNRESET);
		(void)ohci_try_schedule_next(dev);
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	if (xfer->queued != 0U) {
		uhc_xfer_return(dev, xfer, -ECONNRESET);
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	k_spin_unlock(&data->lock, key);
	return -ENOENT;
}

static int ohci_driver_init_register(const struct device *dev)
{
#ifdef CONFIG_UHC_OHCI_PCI
	const struct ohci_config *config = dev->config;

	if (config->pcie != NULL) {
		struct pcie_bar mbar;

		if (config->pcie->bdf == PCIE_BDF_NONE) {
			return -EINVAL;
		}

		pcie_probe_mbar(config->pcie->bdf, 0, &mbar);
		pcie_set_cmd(config->pcie->bdf, PCIE_CONF_CMDSTAT_MEM, true);
		device_map(DEVICE_MMIO_NAMED_RAM_PTR(dev, reg_base), mbar.phys_addr, mbar.size,
			   K_MEM_CACHE_NONE);
		pcie_set_cmd(config->pcie->bdf, PCIE_CONF_CMDSTAT_MASTER, true);

		/* Ensure INTx signaling is enabled for legacy PCI interrupt delivery. */
		uint32_t cmdstat = pcie_conf_read(config->pcie->bdf, PCIE_CONF_CMDSTAT);

		if ((cmdstat & OHCI_PCI_CMD_INTX_DISABLE) != 0U) {
			pcie_conf_write(config->pcie->bdf, PCIE_CONF_CMDSTAT,
					cmdstat & ~OHCI_PCI_CMD_INTX_DISABLE);
			cmdstat = pcie_conf_read(config->pcie->bdf, PCIE_CONF_CMDSTAT);
		}
		LOG_DBG("PCI cmdstat after init 0x%08x", cmdstat);
		return 0;
	}
#endif /* CONFIG_UHC_OHCI_PCI */
	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);
	return 0;
}

static int ohci_driver_init(const struct device *dev)
{
	const struct ohci_config *config = dev->config;
	struct ohci_data *data = dev->data;
	int ret;

	ret = ohci_driver_init_register(dev);
	if (ret < 0) {
		return ret;
	}

	if (config->clock_dev != NULL) {
		if (!device_is_ready(config->clock_dev)) {
			LOG_ERR("Clock controller not ready");
			return -ENODEV;
		}

		ret = clock_control_on(config->clock_dev, config->clock_subsys);
		if (ret < 0) {
			LOG_ERR("Failed to enable clock %d", ret);
			return ret;
		}
	}

	if (config->reset.dev != NULL) {
		ret = reset_line_toggle_dt(&config->reset);
		if (ret < 0) {
			LOG_ERR("Failed to reset controller %d", ret);
			return ret;
		}
	}

	k_mutex_init(&data->uhc_data.mutex);

	data->port_count =
		(uint8_t)(sys_read32(ohci_base(dev) + OHCI_RH_DESC_A) & OHCI_RHDA_NDP_MASK);

	LOG_DBG("OHCI revision 0x%08x ports %u", sys_read32(ohci_base(dev) + OHCI_REVISION),
		data->port_count);

	config->irq_enable_func(dev);

	return 0;
}

static DEVICE_API(uhc, ohci_api) = {
	.lock = ohci_lock,
	.unlock = ohci_unlock,
	.init = ohci_init,
	.enable = ohci_enable,
	.disable = ohci_disable,
	.shutdown = ohci_shutdown,
	.bus_reset = ohci_bus_reset,
	.sof_enable = ohci_sof_enable,
	.bus_suspend = ohci_bus_suspend,
	.bus_resume = ohci_bus_resume,
	.ep_enqueue = ohci_ep_enqueue,
	.ep_dequeue = ohci_ep_dequeue,
};

#define OHCI_DECLARE_PCIE(n) IF_ENABLED(DT_INST_ON_BUS(n, pcie), (DEVICE_PCIE_INST_DECLARE(n)))

#define OHCI_IRQ_FLAGS(n)                                                                          \
	COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, sense), (DT_INST_IRQ(n, sense)),                       \
		    (COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, flags), (DT_INST_IRQ(n, flags)), (0))))

#define OHCI_IRQ_ENABLE_PCIE0(n)                                                                   \
	static void ohci_irq_enable_func_##n(const struct device *dev)                             \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ohci_isr,                   \
			    DEVICE_DT_INST_GET(n), OHCI_IRQ_FLAGS(n));                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#ifdef CONFIG_PCIE_CONTROLLER
#define OHCI_IRQ_ENABLE_PCIE1(n) OHCI_IRQ_ENABLE_PCIE0(n)
#else
#define OHCI_IRQ_ENABLE_PCIE1(n)                                                                   \
	static void ohci_irq_enable_func_##n(const struct device *dev)                             \
	{                                                                                          \
		/* No firmware, IRQ assigned by BIOS/ACPI, read from PCI config at runtime */      \
		const struct ohci_config *config = dev->config;                                    \
		unsigned int irq = pcie_alloc_irq(config->pcie->bdf);                              \
		LOG_DBG("Allocated PCI IRQ %u for bdf 0x%x", irq, config->pcie->bdf);              \
		if (irq != PCIE_CONF_INTR_IRQ_NONE) {                                              \
			pcie_connect_dynamic_irq(config->pcie->bdf, irq, DT_INST_IRQ(n, priority), \
						 (void (*)(const void *))ohci_isr,                 \
						 DEVICE_DT_INST_GET(n), OHCI_IRQ_FLAGS(n));        \
			pcie_irq_enable(config->pcie->bdf, irq);                                   \
		} else {                                                                           \
			LOG_DBG("No PCI IRQ assigned for bdf 0x%x", config->pcie->bdf);            \
		}                                                                                  \
	}
#endif /* CONFIG_PCIE_CONTROLLER */

#define OHCI_IRQ_ENABLE(n)                                                                         \
	COND_CODE_1(DT_INST_ON_BUS(n, pcie), (OHCI_IRQ_ENABLE_PCIE1(n)), (OHCI_IRQ_ENABLE_PCIE0(n)))

#define OHCI_REG_INIT(n)                                                                           \
	COND_CODE_1(DT_INST_ON_BUS(n, pcie), (DEVICE_PCIE_INST_INIT(n, pcie)),                     \
		    (DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n))))

/*
 * Clock handling: the clock_control subsystem encoding depends on the clock
 * controller, so the encoding is selected by the compatible of the clock
 * controller referenced by the clocks property.
 */
#define OHCI_CLOCK_CTLR_IS(n, compat)                                                              \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),                                              \
		    (DT_NODE_HAS_COMPAT(DT_INST_CLOCKS_CTLR(n), compat)), (0))

#define OHCI_CLOCK_DEFINE(n)                                                                       \
	COND_CODE_1(OHCI_CLOCK_CTLR_IS(n, st_stm32_rcc),                                           \
		    (static const struct stm32_pclken ohci_pclken_##n =                            \
			     STM32_CLOCK_INFO(0, DT_DRV_INST(n));),                                \
		    (BUILD_ASSERT(!DT_INST_NODE_HAS_PROP(n, clocks),                             \
				  "Unsupported OHCI clock controller");))

#define OHCI_CLOCK_SUBSYS(n)                                                                       \
	COND_CODE_1(OHCI_CLOCK_CTLR_IS(n, st_stm32_rcc),                                           \
		    ((clock_control_subsys_t)&ohci_pclken_##n), (NULL))

#define OHCI_CLOCK_DEV(n)                                                                          \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),                                              \
		    (DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n))), (NULL))

#define OHCI_DEVICE_DEFINE(n)                                                                      \
	OHCI_DECLARE_PCIE(n);                                                                      \
	OHCI_IRQ_ENABLE(n);                                                                        \
	OHCI_CLOCK_DEFINE(n)                                                                       \
                                                                                                   \
	static const struct ohci_config ohci_config_##n = {                                        \
		OHCI_REG_INIT(n),                                                                  \
		.clock_dev = OHCI_CLOCK_DEV(n),                                                    \
		.clock_subsys = OHCI_CLOCK_SUBSYS(n),                                              \
		.reset = RESET_DT_SPEC_INST_GET_OR(n, {0}),                                        \
		.irq_enable_func = ohci_irq_enable_func_##n,                                       \
	};                                                                                         \
                                                                                                   \
	static struct ohci_dma_mem ohci_dma_##n OHCI_DMA_MEM_SECTION;                              \
	static struct ohci_data ohci_data_##n = {                                                  \
		.hcca = &ohci_dma_##n.hcca,                                                        \
		.slots = ohci_dma_##n.slots,                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ohci_driver_init, NULL, &ohci_data_##n, &ohci_config_##n,         \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ohci_api)

DT_INST_FOREACH_STATUS_OKAY(OHCI_DEVICE_DEFINE)
