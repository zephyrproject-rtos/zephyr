/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT generic_ohci

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/drivers/pcie/pcie.h>

#include "uhc_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_ohci, CONFIG_UHC_DRIVER_LOG_LEVEL);

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev) ((const struct ohci_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct ohci_data *)(uhc_get_private(_dev)))

BUILD_ASSERT(DT_ALL_INST_HAS_BOOL_STATUS_OKAY(dma_coherent),
	     "This driver currently only supports dma-coherent devices. Feel free to open a PR to "
	     "remove this assertion and add support for non-coherent devices if needed.");

#define OHCI_REVISION                  0x00U
#define OHCI_CONTROL                   0x04U
#define OHCI_CMDSTATUS                 0x08U
#define OHCI_INTRSTATUS                0x0cU
#define OHCI_INTRENABLE                0x10U
#define OHCI_INTRDISABLE               0x14U
#define OHCI_HCCA                      0x18U
#define OHCI_CONTROL_HEAD_ED           0x20U
#define OHCI_BULK_HEAD_ED              0x28U
#define OHCI_FM_INTERVAL               0x34U
#define OHCI_PERIODIC_START            0x40U
#define OHCI_LS_THRESHOLD              0x44U
#define OHCI_RH_DESC_A                 0x48U
#define OHCI_RH_STATUS                 0x50U
#define OHCI_RH_PORT_STATUS(n)         (0x54U + ((n) * 4U))

#define OHCI_CONTROL_PLE               BIT(2)
#define OHCI_CONTROL_CLE               BIT(4)
#define OHCI_CONTROL_BLE               BIT(5)
#define OHCI_CONTROL_HCFS_MASK         GENMASK(7, 6)
#define OHCI_CONTROL_HCFS_RESET        (0U << 6)
#define OHCI_CONTROL_HCFS_RESUME       (1U << 6)
#define OHCI_CONTROL_HCFS_OPERATIONAL  (2U << 6)
#define OHCI_CONTROL_HCFS_SUSPEND      (3U << 6)

#define OHCI_CMDSTATUS_HCR             BIT(0)
#define OHCI_CMDSTATUS_CLF             BIT(1)
#define OHCI_CMDSTATUS_BLF             BIT(2)

#define OHCI_INTR_WDH                  BIT(1)
#define OHCI_INTR_RD                   BIT(3)
#define OHCI_INTR_UE                   BIT(4)
#define OHCI_INTR_RHSC                 BIT(6)
#define OHCI_INTR_MIE                  BIT(31)

#define OHCI_RHDA_NDP_MASK             GENMASK(7, 0)

#define OHCI_RHPS_CCS                  BIT(0)
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

#define OHCI_ED_FA_SHIFT               0U
#define OHCI_ED_EN_SHIFT               7U
#define OHCI_ED_D_SHIFT                11U
#define OHCI_ED_D_FROM_TD              0U
#define OHCI_ED_D_OUT                  1U
#define OHCI_ED_D_IN                   2U
#define OHCI_ED_SPEED                  BIT(13)
#define OHCI_ED_SKIP                   BIT(14)
#define OHCI_ED_MPS_SHIFT              16U
#define OHCI_ED_HEAD_HALTED            BIT(0)
#define OHCI_ED_HEAD_CARRY             BIT(1)
#define OHCI_ED_HEAD_PTR_MASK          GENMASK(31, 4)

#define OHCI_TD_CC_SHIFT               28U
#define OHCI_TD_CC_MASK                GENMASK(31, 28)
#define OHCI_TD_CC_NO_ERROR            0U
#define OHCI_TD_CC_DATA_UNDERRUN       9U
#define OHCI_TD_CC_NOT_ACCESSED        15U
#define OHCI_TD_T_SHIFT                24U
#define OHCI_TD_T_TOGGLE_ED            0U
#define OHCI_TD_T_DATA0                2U
#define OHCI_TD_T_DATA1                3U
#define OHCI_TD_DI_SHIFT               21U
#define OHCI_TD_DI_NO_INTERRUPT        7U
#define OHCI_TD_DP_SHIFT               19U
#define OHCI_TD_DP_SETUP               0U
#define OHCI_TD_DP_OUT                 1U
#define OHCI_TD_DP_IN                  2U
#define OHCI_TD_R                      BIT(18)
#define OHCI_TD_PTR_MASK               GENMASK(31, 4)

#define OHCI_RESET_TIMEOUT_US          10000U
#define OHCI_BUS_RESET_TIME_MS         50U
#define OHCI_DEFAULT_FMINTERVAL        0x2edfU
#define OHCI_DEFAULT_PERIODIC_START    0x2a2fU
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
};

/*
 * slot[OHCI_CTRL_SLOT] = control transfers (OHCI control list)
 * slot[1 .. OHCI_MAX_SLOTS-1] = bulk/interrupt transfers (OHCI bulk list)
 */
#define OHCI_CTRL_SLOT  0U
#define OHCI_MAX_SLOTS  (1U + CONFIG_UHC_OHCI_BULK_SLOTS)

struct ohci_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	struct ohci_hcca      hcca;
	struct ohci_xfer_slot slots[OHCI_MAX_SLOTS];
	uint8_t               port_count;
	uint8_t               port_connected;
	uint8_t               bus_suspended;
	uint8_t               bulk_toggle[128][32];
};

static inline uintptr_t ohci_base(const struct device *dev)
{
	return DEVICE_MMIO_NAMED_GET(dev, reg_base);
}

static inline uint32_t ohci_read(const struct device *dev, uint32_t reg)
{
	return sys_read32(ohci_base(dev) + reg);
}

static inline void ohci_write(const struct device *dev, uint32_t reg, uint32_t value)
{
	sys_write32(value, ohci_base(dev) + reg);
}

static inline uint32_t ohci_dma_read32(const uint32_t *value)
{
	return *(volatile const uint32_t *)value;
}

static inline uint32_t ohci_head_ptr(const struct ohci_hw_ed *ed)
{
	return ohci_dma_read32(&ed->headp) & OHCI_ED_HEAD_PTR_MASK;
}

static inline uint32_t ohci_td_cc(const struct ohci_hw_td *td)
{
	return FIELD_GET(OHCI_TD_CC_MASK, ohci_dma_read32(&td->flags));
}

static inline uint32_t ohci_td_phys(const struct ohci_td *td)
{
	return (uint32_t)k_mem_phys_addr((void *)&td->hw);
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

static inline uint32_t ohci_phys(const void *addr)
{
	return (uint32_t)k_mem_phys_addr((void *)addr);
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
	case 4:
		return -EPIPE;
	case 8:
		return -EOVERFLOW;
	case 3:
		return -EAGAIN;
	case 5:
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

	cbp = ohci_dma_read32(&td->hw.cbp);
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
	for (unsigned int i = 0U; i < OHCI_MAX_SLOTS; i++) {
		ohci_clear_slot(&data->slots[i]);
	}
}

static int ohci_wait_reset(const struct device *dev)
{
	for (int i = 0; i < OHCI_RESET_TIMEOUT_US; i += 10) {
		if ((ohci_read(dev, OHCI_CMDSTATUS) & OHCI_CMDSTATUS_HCR) == 0U) {
			return 0;
		}

		k_busy_wait(10);
	}

	return -ETIMEDOUT;
}

static size_t ohci_chunk_len(uintptr_t addr, size_t remaining, uint16_t mps)
{
	size_t page_remaining = 0x1000U - (addr & 0xfffU);
	size_t chunk = MIN(remaining, (size_t)mps);

	return MIN(chunk, page_remaining);
}

static void ohci_fill_td(struct ohci_td *td, uintptr_t data, size_t len,
				 uint32_t dp, uint32_t toggle, bool allow_short,
				 bool dir_in, bool interrupt_on_done)
{
	uint32_t flags = FIELD_PREP(OHCI_TD_CC_MASK, OHCI_TD_CC_NOT_ACCESSED) |
			 FIELD_PREP(GENMASK(25, 24), toggle) |
			 FIELD_PREP(GENMASK(20, 19), dp) |
			 FIELD_PREP(GENMASK(23, 21),
					interrupt_on_done ? 0U : OHCI_TD_DI_NO_INTERRUPT);

	if (allow_short) {
		flags |= OHCI_TD_R;
	}

	td->hw.flags = flags;
	td->hw.cbp = len ? ohci_phys((void *)data) : 0U;
	td->hw.be = len ? ohci_phys((void *)(data + len - 1U)) : 0U;
	td->data = len ? ohci_phys((void *)data) : 0U;
	td->len = (uint16_t)len;
	td->allow_short = allow_short;
	td->dir_in = dir_in;
	td->hw.next = 0U;
}

static int ohci_build_control_chain(struct ohci_xfer_slot *slot, struct uhc_transfer *xfer)
{
	uint8_t *buffer = NULL;
	size_t remaining = 0U;
	uintptr_t ptr = 0U;
	uint16_t td_idx = 0U;
	uint32_t toggle = OHCI_TD_T_DATA1;
	bool dir_in = USB_EP_DIR_IS_IN(xfer->ep);
	const struct usb_setup_packet *setup = (const struct usb_setup_packet *)xfer->setup_pkt;
	uint16_t w_length = sys_le16_to_cpu(setup->wLength);

	if (CONFIG_UHC_OHCI_MAX_TDS < 3) {
		return -ENOMEM;
	}

	ohci_fill_td(&slot->tds[td_idx++], (uintptr_t)xfer->setup_pkt, sizeof(xfer->setup_pkt),
		     OHCI_TD_DP_SETUP, OHCI_TD_T_DATA0, false, false, false);

	if (xfer->buf != NULL) {
		if (dir_in) {
			buffer = net_buf_tail(xfer->buf);
			remaining = MIN((size_t)w_length, net_buf_tailroom(xfer->buf));
		} else {
			buffer = xfer->buf->data;
			remaining = MIN((size_t)w_length, (size_t)xfer->buf->len);
		}

		ptr = (uintptr_t)buffer;
		while (remaining != 0U) {
			size_t chunk;

			if (td_idx >= (CONFIG_UHC_OHCI_MAX_TDS - 1)) {
				return -ENOMEM;
			}

			chunk = ohci_chunk_len(ptr, remaining, xfer->mps);
			ohci_fill_td(&slot->tds[td_idx++], ptr, chunk,
				     dir_in ? OHCI_TD_DP_IN : OHCI_TD_DP_OUT,
				     toggle, dir_in, dir_in, false);
			ptr += chunk;
			remaining -= chunk;
			toggle = (toggle == OHCI_TD_T_DATA1) ? OHCI_TD_T_DATA0 : OHCI_TD_T_DATA1;
		}
	}

	if (!xfer->no_status) {
		if (td_idx >= (CONFIG_UHC_OHCI_MAX_TDS - 1)) {
			return -ENOMEM;
		}

		ohci_fill_td(&slot->tds[td_idx++], 0U, 0U,
			     dir_in ? OHCI_TD_DP_OUT : OHCI_TD_DP_IN,
			     OHCI_TD_T_DATA1, false, !dir_in, true);
	} else {
		slot->tds[td_idx - 1U].hw.flags &= ~GENMASK(23, 21);
	}

	slot->td_count = td_idx + 1U;
	return 0;
}

static int ohci_build_bulk_chain(struct ohci_xfer_slot *slot, struct uhc_transfer *xfer)
{
	uint8_t *buffer;
	size_t remaining;
	uintptr_t ptr;
	uint16_t td_idx = 0U;
	bool dir_in = USB_EP_DIR_IS_IN(xfer->ep);

	if (xfer->buf == NULL) {
		return -EINVAL;
	}

	if (dir_in) {
		buffer = net_buf_tail(xfer->buf);
		remaining = net_buf_tailroom(xfer->buf);
	} else {
		buffer = xfer->buf->data;
		remaining = xfer->buf->len;
	}

	ptr = (uintptr_t)buffer;
	while (remaining != 0U) {
		size_t chunk;

		if (td_idx >= (CONFIG_UHC_OHCI_MAX_TDS - 1)) {
			return -ENOMEM;
		}

		/*
		 * Bulk TDs can carry multiple USB packets; splitting by endpoint MPS
		 * causes excessive TD chaining and may coalesce multiple Ethernet
		 * frames into one completion. Keep TDs page-bounded, not MPS-bounded.
		 */
		chunk = ohci_chunk_len(ptr, remaining, UINT16_MAX);
		/*
		 * For IN transfers every TD must have DI=0 (interrupt on done).
		 * When a short packet arrives the OHCI HC sets headP = tailP and
		 * adds the TD to the done list.  If that TD has DI=7 (no
		 * interrupt) the WritebackDoneHead interrupt never fires and the
		 * driver never learns data arrived.  Setting DI=0 on every IN TD
		 * ensures WDH fires on the very first short-packet completion.
		 * For OUT transfers only the last TD generates an interrupt.
		 */
		ohci_fill_td(&slot->tds[td_idx], ptr, chunk,
			     dir_in ? OHCI_TD_DP_IN : OHCI_TD_DP_OUT,
			     OHCI_TD_T_TOGGLE_ED,
			     dir_in, dir_in, dir_in);
		ptr += chunk;
		remaining -= chunk;
		td_idx++;
	}

	if (td_idx == 0U) {
		return -EINVAL;
	}

	slot->tds[td_idx - 1U].hw.flags &= ~GENMASK(23, 21);
	slot->td_count = td_idx + 1U;
	return 0;
}

static void ohci_link_chain(struct ohci_data *data, struct ohci_xfer_slot *slot,
			    struct uhc_transfer *xfer)
{
	uint32_t ed_flags = FIELD_PREP(GENMASK(6, 0), xfer->udev->addr) |
			    FIELD_PREP(GENMASK(10, 7), USB_EP_GET_IDX(xfer->ep)) |
			    FIELD_PREP(GENMASK(31, 16), xfer->mps) |
			    FIELD_PREP(GENMASK(12, 11), OHCI_ED_D_FROM_TD);

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
	if (xfer->type == USB_EP_TYPE_BULK && ohci_bulk_toggle_get(data, xfer) == OHCI_TD_T_DATA1) {
		slot->ed.headp |= OHCI_ED_HEAD_CARRY;
	}
	slot->ed.tailp = ohci_td_phys(&slot->tds[slot->td_count - 1U]);
}

/* Return true if xfer is currently being processed in any slot. */
static bool ohci_xfer_is_active(const struct ohci_data *data,
				const struct uhc_transfer *xfer)
{
	for (unsigned int i = 0U; i < OHCI_MAX_SLOTS; i++) {
		if (data->slots[i].xfer == xfer) {
			return true;
		}
	}
	return false;
}

/* Return true if endpoint ep already has an active slot. */
static bool ohci_ep_is_active(const struct ohci_data *data, uint8_t ep)
{
	for (unsigned int i = 0U; i < OHCI_MAX_SLOTS; i++) {
		if (data->slots[i].xfer != NULL && data->slots[i].xfer->ep == ep) {
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
	struct ohci_data *data = uhc_get_private(dev);

	for (unsigned int i = 1U; i < OHCI_MAX_SLOTS; i++) {
		data->slots[i].ed.flags = OHCI_ED_SKIP;
		data->slots[i].ed.headp = 0U;
		data->slots[i].ed.tailp = 0U;
		data->slots[i].ed.next =
			(i + 1U < OHCI_MAX_SLOTS)
			? ohci_phys(&data->slots[i + 1U].ed)
			: 0U;
	}
	compiler_barrier();
	ohci_write(dev, OHCI_BULK_HEAD_ED,
		   (OHCI_MAX_SLOTS > 1U) ? ohci_phys(&data->slots[1U].ed) : 0U);
}

static int ohci_finish_slot(const struct device *dev, struct ohci_xfer_slot *slot)
{
	struct ohci_data *data = uhc_get_private(dev);
	struct uhc_transfer *xfer = slot->xfer;
	uint32_t ed_headp;
	int err = 0;
	size_t actual_in = 0U;
	bool is_ctrl;

	if (xfer == NULL) {
		return 0;
	}

	compiler_barrier();
	ed_headp = ohci_dma_read32(&slot->ed.headp);
	if ((ed_headp & OHCI_ED_HEAD_HALTED) == 0U &&
	    (ed_headp & OHCI_ED_HEAD_PTR_MASK) != slot->ed.tailp) {
		return 0;
	}

	for (uint16_t i = 0U; i < (slot->td_count - 1U); i++) {
		uint32_t cc = ohci_td_cc(&slot->tds[i].hw);

		/*
		 * NOT_ACCESSED means the HC never processed this TD — normal
		 * when a short-packet on an earlier TD terminated the transfer
		 * by setting headP = tailP.  Treat as end-of-transfer, no error.
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

	if (err == 0 && xfer->type == USB_EP_TYPE_BULK) {
		ohci_bulk_toggle_save(data, xfer, ed_headp);
	}

	if (err == 0 && xfer->buf != NULL && USB_EP_DIR_IS_IN(xfer->ep) && actual_in != 0U) {
		net_buf_add(xfer->buf, actual_in);
	}

	is_ctrl = (slot == &data->slots[OHCI_CTRL_SLOT]);
	LOG_DBG("Complete xfer ep 0x%02x err %d td_count %u actual_in %zu slot %u",
		xfer->ep, err, slot->td_count, actual_in,
		(unsigned int)(slot - data->slots));

	if (is_ctrl) {
		ohci_write(dev, OHCI_CONTROL_HEAD_ED, 0U);
	}

	/*
	 * ohci_clear_slot() preserves ed.next and sets OHCI_ED_SKIP, so the
	 * HC will skip this ED cleanly after the clear.
	 */
	compiler_barrier();
	ohci_clear_slot(slot);
	uhc_xfer_return(dev, xfer, err);

	return err;
}

/* Find a free slot for the given transfer type.  Returns NULL if none free. */
static struct ohci_xfer_slot *ohci_alloc_slot(struct ohci_data *data, bool is_ctrl)
{
	if (is_ctrl) {
		return (data->slots[OHCI_CTRL_SLOT].xfer == NULL)
			? &data->slots[OHCI_CTRL_SLOT] : NULL;
	}
	for (unsigned int i = 1U; i < OHCI_MAX_SLOTS; i++) {
		if (data->slots[i].xfer == NULL) {
			return &data->slots[i];
		}
	}
	return NULL;
}

static int ohci_try_schedule_next(const struct device *dev)
{
	struct ohci_data *data = uhc_get_private(dev);
	struct uhc_data *uhc = dev->data;
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
	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&uhc->ctrl_xfers, xfer, tmp, node) {
		bool is_ctrl = (xfer->type == USB_EP_TYPE_CONTROL);

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
		if (ohci_ep_is_active(data, xfer->ep)) {
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

		ohci_link_chain(data, slot, xfer);
		slot->xfer = xfer;

		LOG_DBG("ED flags=0x%08x head=0x%08x tail=0x%08x count=%u",
			slot->ed.flags, slot->ed.headp, slot->ed.tailp, slot->td_count);

		/* Ensure all TD/ED writes reach hardware before kicking the HC. */
		compiler_barrier();

		if (is_ctrl) {
			ohci_write(dev, OHCI_CONTROL_HEAD_ED, ohci_phys(&slot->ed));
			ohci_write(dev, OHCI_CMDSTATUS, OHCI_CMDSTATUS_CLF);
		} else {
			/* Clear the SKIP bit to make this ED visible to the HC. */
			slot->ed.flags &= ~OHCI_ED_SKIP;
			compiler_barrier();
			ohci_write(dev, OHCI_CMDSTATUS, OHCI_CMDSTATUS_BLF);
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

static void ohci_handle_root_hub_change(const struct device *dev)
{
	struct ohci_data *data = uhc_get_private(dev);
	bool connected_now = false;
	bool low_speed = false;
	enum uhc_event_type evt;

	if (data->port_count == 0U) {
		return;
	}

	for (uint8_t port = 0U; port < data->port_count; port++) {
		uint32_t status = ohci_read(dev, OHCI_RH_PORT_STATUS(port));
		uint32_t clear = status & OHCI_RHPS_W1C_MASK;

		LOG_DBG("RH port%u status 0x%08x (connected=%u)", port, status,
			data->port_connected);

		if ((status & OHCI_RHPS_CCS) != 0U) {
			connected_now = true;
			if ((status & OHCI_RHPS_LSDA) != 0U) {
				low_speed = true;
			}
		}

		if (clear != 0U) {
			ohci_write(dev, OHCI_RH_PORT_STATUS(port), clear);
		}
	}

	if (connected_now && data->port_connected == 0U) {
		data->port_connected = 1U;
		evt = low_speed ? UHC_EVT_DEV_CONNECTED_LS : UHC_EVT_DEV_CONNECTED_FS;
		LOG_DBG("Submit connect event %d from RH scan", evt);
		uhc_submit_event(dev, evt, 0);
	} else if (!connected_now && data->port_connected != 0U) {
		data->port_connected = 0U;
		uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
	}
}

static void ohci_irq_handler(const struct device *dev, uint32_t irqs)
{
	struct ohci_data *data = uhc_get_private(dev);

	if ((irqs & OHCI_INTR_RHSC) != 0U) {
		ohci_handle_root_hub_change(dev);
	}

	if ((irqs & OHCI_INTR_WDH) != 0U) {
		uint32_t done_head = ohci_dma_read32(&data->hcca.done_head);

		data->hcca.done_head = 0U;
		compiler_barrier();

		if (done_head != 0U) {
			/* Check all slots; more than one may have completed. */
			for (unsigned int i = 0U; i < OHCI_MAX_SLOTS; i++) {
				ohci_finish_slot(dev, &data->slots[i]);
			}
		}
	}

	if ((irqs & OHCI_INTR_RD) != 0U) {
		uhc_submit_event(dev, UHC_EVT_RWUP, 0);
	}

	if ((irqs & OHCI_INTR_UE) != 0U) {
		uhc_submit_event(dev, UHC_EVT_ERROR, -EIO);
	}

	(void)ohci_try_schedule_next(dev);
}

static void ohci_isr(const struct device *dev)
{
	uint32_t irqs;

	while (true) {
		irqs = ohci_read(dev, OHCI_INTRSTATUS);

		if (irqs == UINT32_MAX) {
			/* Spurious interrupt with all bits set, ignore */
			return;
		}
		/* Only handle interrupts that are currently enabled (Linux pattern) */
		irqs &= ohci_read(dev, OHCI_INTRENABLE);
		if (irqs == 0U) {
			return;
		}

		ohci_irq_handler(dev, irqs);

		ohci_write(dev, OHCI_INTRSTATUS, irqs);
	}
}

static int ohci_init(const struct device *dev)
{
	struct ohci_data *data = uhc_get_private(dev);

	memset(&data->hcca, 0, sizeof(data->hcca));
	memset(data->bulk_toggle, 0, sizeof(data->bulk_toggle));
	ohci_clear_all_slots(data);
	data->port_connected = 0U;
	data->bus_suspended = 0U;

	return 0;
}

static int ohci_enable(const struct device *dev)
{
	struct ohci_data *data = uhc_get_private(dev);
	uint32_t control;

	ohci_write(dev, OHCI_CMDSTATUS, OHCI_CMDSTATUS_HCR);
	if (ohci_wait_reset(dev) != 0) {
		return -ETIMEDOUT;
	}

	memset(&data->hcca, 0, sizeof(data->hcca));
	memset(data->bulk_toggle, 0, sizeof(data->bulk_toggle));
	ohci_clear_all_slots(data);
	ohci_write(dev, OHCI_INTRDISABLE, 0xffffffffU);
	ohci_write(dev, OHCI_INTRSTATUS, 0xffffffffU);
	ohci_write(dev, OHCI_FM_INTERVAL, OHCI_DEFAULT_FMINTERVAL);
	ohci_write(dev, OHCI_PERIODIC_START, OHCI_DEFAULT_PERIODIC_START);
	ohci_write(dev, OHCI_LS_THRESHOLD, OHCI_DEFAULT_LS_THRESHOLD);
	ohci_write(dev, OHCI_HCCA, ohci_phys(&data->hcca));

	/* Set up the permanent bulk-list chain before enabling the HC. */
	ohci_setup_bulk_list(dev);

	control = ohci_read(dev, OHCI_CONTROL);
	control &= ~OHCI_CONTROL_HCFS_MASK;
	control |= OHCI_CONTROL_HCFS_OPERATIONAL | OHCI_CONTROL_CLE | OHCI_CONTROL_BLE;
	ohci_write(dev, OHCI_CONTROL, control);

	for (uint8_t port = 0U; port < data->port_count; port++) {
		ohci_write(dev, OHCI_RH_PORT_STATUS(port), OHCI_RHPS_PPS);
	}

	ohci_write(dev, OHCI_INTRENABLE,
		   OHCI_INTR_MIE | OHCI_INTR_WDH | OHCI_INTR_RD | OHCI_INTR_RHSC | OHCI_INTR_UE);

	ohci_handle_root_hub_change(dev);

	return ohci_try_schedule_next(dev);
}

static int ohci_disable(const struct device *dev)
{
	struct ohci_data *data = uhc_get_private(dev);

	ohci_write(dev, OHCI_INTRDISABLE, UINT32_MAX);
	ohci_write(dev, OHCI_CONTROL,
		   (ohci_read(dev, OHCI_CONTROL) & ~OHCI_CONTROL_HCFS_MASK) |
		   OHCI_CONTROL_HCFS_RESET);

	for (unsigned int i = 0U; i < OHCI_MAX_SLOTS; i++) {
		if (data->slots[i].xfer != NULL) {
			struct uhc_transfer *xfer = data->slots[i].xfer;

			ohci_clear_slot(&data->slots[i]);
			uhc_xfer_return(dev, xfer, -ECONNRESET);
		}
	}

	return 0;
}

static int ohci_shutdown(const struct device *dev)
{
	return 0;
}

static int ohci_bus_reset(const struct device *dev)
{
	struct ohci_data *data = uhc_get_private(dev);
	int ret;

	if (data->port_count == 0U) {
		return -ENODEV;
	}

	for (uint8_t port = 0U; port < data->port_count; port++) {
		uint32_t status = ohci_read(dev, OHCI_RH_PORT_STATUS(port));

		if ((status & OHCI_RHPS_CCS) != 0U) {
			ohci_write(dev, OHCI_RH_PORT_STATUS(port), OHCI_RHPS_PRS);
		}
	}

	k_msleep(OHCI_BUS_RESET_TIME_MS);

	for (uint8_t port = 0U; port < data->port_count; port++) {
		ohci_write(dev, OHCI_RH_PORT_STATUS(port), OHCI_RHPS_PRSC);
	}

	ret = uhc_submit_event(dev, UHC_EVT_RESETED, 0);
	if (ret != 0) {
		return ret;
	}

	/* Re-check port status after reset to catch delayed presence updates. */
	ohci_handle_root_hub_change(dev);

	return 0;
}

static int ohci_sof_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int ohci_bus_suspend(const struct device *dev)
{
	struct ohci_data *data = uhc_get_private(dev);
	uint32_t control;

	if (data->bus_suspended != 0U) {
		return -EALREADY;
	}

	control = ohci_read(dev, OHCI_CONTROL) & ~OHCI_CONTROL_HCFS_MASK;
	ohci_write(dev, OHCI_CONTROL, control | OHCI_CONTROL_HCFS_SUSPEND);
	data->bus_suspended = 1U;

	return uhc_submit_event(dev, UHC_EVT_SUSPENDED, 0);
}

static int ohci_bus_resume(const struct device *dev)
{
	struct ohci_data *data = uhc_get_private(dev);
	uint32_t control;

	if (data->bus_suspended == 0U) {
		return -EALREADY;
	}

	control = ohci_read(dev, OHCI_CONTROL) & ~OHCI_CONTROL_HCFS_MASK;
	ohci_write(dev, OHCI_CONTROL, control | OHCI_CONTROL_HCFS_RESUME);
	k_msleep(20);
	ohci_write(dev, OHCI_CONTROL, control | OHCI_CONTROL_HCFS_OPERATIONAL |
		   OHCI_CONTROL_CLE | OHCI_CONTROL_BLE);
	data->bus_suspended = 0U;

	return uhc_submit_event(dev, UHC_EVT_RESUMED, 0);
}

static int ohci_ep_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	int ret;

	ret = uhc_xfer_append(dev, xfer);
	if (ret != 0) {
		LOG_DBG("Append xfer ep 0x%02x failed %d", xfer->ep, ret);
		return ret;
	}

	ret = ohci_try_schedule_next(dev);
	if (ret != 0) {
		LOG_DBG("Schedule xfer ep 0x%02x failed %d", xfer->ep, ret);
	}

	return ret;
}

static int ohci_ep_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct ohci_data *data = uhc_get_private(dev);

	for (unsigned int i = 0U; i < OHCI_MAX_SLOTS; i++) {
		if (data->slots[i].xfer != xfer) {
			continue;
		}
		/*
		 * CH9 request timeout paths expect an in-flight transfer to be
		 * cancelable. Stop the relevant list, free the slot, and return
		 * the transfer with -ECONNRESET so upper layers can recover.
		 */
		if (i == OHCI_CTRL_SLOT) {
			ohci_write(dev, OHCI_CONTROL_HEAD_ED, 0U);
		} else {
			data->slots[i].ed.flags |= OHCI_ED_SKIP;
			compiler_barrier();
		}
		ohci_clear_slot(&data->slots[i]);
		uhc_xfer_return(dev, xfer, -ECONNRESET);
		(void)ohci_try_schedule_next(dev);
		return 0;
	}

	if (xfer->queued != 0U) {
		uhc_xfer_return(dev, xfer, -ECONNRESET);
		return 0;
	}

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
	struct ohci_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;
	int ret;

	ret = ohci_driver_init_register(dev);
	if (ret < 0) {
		return ret;
	}

	k_mutex_init(&data->mutex);

	priv->port_count = (uint8_t)(ohci_read(dev, OHCI_RH_DESC_A) & OHCI_RHDA_NDP_MASK);

	LOG_DBG("OHCI revision 0x%08x ports %u", ohci_read(dev, OHCI_REVISION), priv->port_count);

	config->irq_enable_func(dev);

	return 0;
}

static const struct uhc_api ohci_api = {
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

#define OHCI_IRQ_FLAGS(n) COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, sense), (DT_INST_IRQ(n, sense)), (0))

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

#define OHCI_DEVICE_DEFINE(n)                                                                      \
	OHCI_DECLARE_PCIE(n);                                                                      \
	OHCI_IRQ_ENABLE(n);                                                                        \
                                                                                                   \
	static const struct ohci_config ohci_config_##n = {                                        \
		OHCI_REG_INIT(n),                                                                  \
		.irq_enable_func = ohci_irq_enable_func_##n,                                       \
	};                                                                                         \
                                                                                                   \
	static struct ohci_data ohci_priv_##n;                                                     \
	static struct uhc_data ohci_data_##n = {                                                   \
		.priv = &ohci_priv_##n,                                                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ohci_driver_init, NULL, &ohci_data_##n, &ohci_config_##n,         \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ohci_api)

DT_INST_FOREACH_STATUS_OKAY(OHCI_DEVICE_DEFINE)
