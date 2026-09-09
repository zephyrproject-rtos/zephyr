/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal xHCI hardware definitions for control + bulk transfers.
 * Derived from xHCI drivers/usb/host/xhci.h
 */

#ifndef ZEPHYR_USB_XHCI_HW_H
#define ZEPHYR_USB_XHCI_HW_H

#include <stdint.h>

/* Capability register offsets (from MMIO base) */
#define XHCI_CAP_CAPBASE    0x00
#define XHCI_CAP_HCSPARAMS1 0x04
#define XHCI_CAP_HCSPARAMS2 0x08
#define XHCI_CAP_HCSPARAMS3 0x0C
#define XHCI_CAP_HCCPARAMS1 0x10
#define XHCI_CAP_DBOFF      0x14
#define XHCI_CAP_RTSOFF     0x18

#define XHCI_CAPLENGTH(p)           ((uint32_t)(p) & 0xffU)
#define XHCI_HCIVERSION(p)          (((uint32_t)(p) >> 16) & 0xffffU)
#define XHCI_HCS1_MAX_SLOTS(p)      (((uint32_t)(p)) & 0xffU)
#define XHCI_HCS1_MAX_INTRS(p)      (((uint32_t)(p) >> 8) & 0x7ffU)
#define XHCI_HCS1_MAX_PORTS(p)      (((uint32_t)(p) >> 24) & 0xffU)
#define XHCI_HCS2_MAX_SP_HI(p)      (((uint32_t)(p) >> 21) & 0x1fU)
#define XHCI_HCS2_MAX_SP_LO(p)      (((uint32_t)(p) >> 27) & 0x1fU)
#define XHCI_HCS2_MAX_SCRATCHPAD(p) ((XHCI_HCS2_MAX_SP_HI(p) << 5) | XHCI_HCS2_MAX_SP_LO(p))

#define XHCI_HCC1_64BIT_ADDR BIT(0)
#define XHCI_HCC1_64BYTE_CTX BIT(2)
#define XHCI_HCC1_PPC        BIT(3)
#define XHCI_CTX_BYTES(hcc)  (((hcc) & XHCI_HCC1_64BYTE_CTX) ? 64U : 32U)

#define XHCI_DBOFF_MASK  0xfffffffcU
#define XHCI_RTSOFF_MASK (~0x1fU)

/* Operational register offsets (from cap_base + CAPLENGTH) */
#define XHCI_OP_USBCMD      0x00
#define XHCI_OP_USBSTS      0x04
#define XHCI_OP_PAGESIZE    0x08
#define XHCI_OP_DNCTRL      0x14
#define XHCI_OP_CRCR        0x18 /* 64-bit */
#define XHCI_OP_DCBAAP      0x30 /* 64-bit */
#define XHCI_OP_CONFIG      0x38
#define XHCI_OP_PORTSC_BASE 0x400

#define XHCI_PORT_REGS_STRIDE 16U

/* USBCMD */
#define XHCI_USBCMD_RUN   BIT(0)
#define XHCI_USBCMD_HCRST BIT(1)
#define XHCI_USBCMD_INTE  BIT(2)
#define XHCI_USBCMD_HSEE  BIT(3)
#define XHCI_USBCMD_EWE   BIT(10)

/* USBSTS */
#define XHCI_USBSTS_HCH  BIT(0)
#define XHCI_USBSTS_HSE  BIT(2)
#define XHCI_USBSTS_EINT BIT(3)
#define XHCI_USBSTS_PCD  BIT(4)
#define XHCI_USBSTS_CNR  BIT(11)

/* PORTSC (xHCI 1.x Table 5-25). PED is status: set by HW after reset completes. */
#define XHCI_PORTSC_CCS        BIT(0)
#define XHCI_PORTSC_PED        BIT(1)
#define XHCI_PORTSC_PR         BIT(4)
#define XHCI_PORTSC_PLS_MASK   (0xfU << 5)
#define XHCI_PORTSC_PP         BIT(9)
#define XHCI_PORTSC_SPEED_MASK (0xfU << 10)
#define XHCI_PORTSC_LWS        BIT(16)
#define XHCI_PORTSC_CSC        BIT(17)
#define XHCI_PORTSC_PEC        BIT(18)
#define XHCI_PORTSC_WRC        BIT(19)
#define XHCI_PORTSC_OCC        BIT(20)
#define XHCI_PORTSC_PRC        BIT(21)
#define XHCI_PORTSC_PLC        BIT(22)
#define XHCI_PORTSC_CEC        BIT(23)
#define XHCI_PORTSC_WR         BIT(31)

/* Write-1-to-clear status bits (Table 5-25) — ack only bits set in portsc */
#define XHCI_PORTSC_W1C_MASK                                                                       \
	(XHCI_PORTSC_CSC | XHCI_PORTSC_PEC | XHCI_PORTSC_WRC | XHCI_PORTSC_OCC | XHCI_PORTSC_PRC | \
	 XHCI_PORTSC_PLC | XHCI_PORTSC_CEC)

#define XHCI_PORTSC_SPEED(p) (((uint32_t)(p) >> 10) & 0xfU)

/*
 * PORTSC write-safe masks — include/usb/xhci.h, xHCI-hub.c
 * xhci_port_state_to_neutral() preserves RO + RWS so SET_FEATURE RESET/POWER
 * and W1C clears do not disturb link state / port power or spuriously clear
 * other change bits (same pattern as xhci_submit_root() in xhci.c).
 */
#define XHCI_PORTSC_RO_MASK                                                                        \
	(UINT32_C(1) << 0 | UINT32_C(1) << 3 | UINT32_C(0xf) << 10 | UINT32_C(1) << 30)
#define XHCI_PORTSC_RWS_MASK                                                                       \
	(UINT32_C(0xf) << 5 | UINT32_C(1) << 9 | UINT32_C(0x3) << 14 | UINT32_C(0x7) << 25)

static inline uint32_t xhci_port_state_to_neutral(uint32_t state)
{
	return (state & XHCI_PORTSC_RO_MASK) | (state & XHCI_PORTSC_RWS_MASK);
}

/* Port link states */
#define XHCI_PLS_U0        0x0U
#define XHCI_PLS_U3        0x3U
#define XHCI_PLS_RX_DETECT 0x5U

/* Runtime register offsets (from RTSOFF base) */
#define XHCI_RT_MFINDEX  0x00
#define XHCI_RT_IR0      0x20
#define XHCI_INTR_STRIDE 32U

#define XHCI_IR_IMAN   0x00
#define XHCI_IR_IMOD   0x04
#define XHCI_IR_ERSTSZ 0x08
#define XHCI_IR_ERSTBA 0x10 /* 64-bit */
#define XHCI_IR_ERDP   0x18 /* 64-bit */

#define XHCI_IMAN_IP  BIT(0)
#define XHCI_IMAN_IE  BIT(1)
#define XHCI_ERDP_EHB BIT(3)

/* Event ring segment table — xhci-mem.c / include/usb/xhci.h */
#define XHCI_ERST_SIZE_MASK (0xffffU << 16)
#define XHCI_ERST_PTR_MASK  UINT64_C(0xf)

/* Doorbell */
#define XHCI_DB_STRIDE           4U
#define XHCI_DB_HOST             0U
/*
 * xHCI §5.6: DB Target (DCI) for the default control pipe is 1. DCI 0 is not a
 * valid endpoint index for device doorbells (slot 0 + XHCI_DB_HOST is the host
 * command ring only).
 */
#define XHCI_DCI_DEFAULT_CONTROL 1U
#define XHCI_DB_TARGET(ep, stream)                                                                 \
	((((uint32_t)(ep) + 1U) & 0xffU) | (((uint32_t)(stream) & 0xffffU) << 16))

/* xHCI 6.2.3: EP TR Dequeue Pointer — DCS in bit 0; bits 3:1 RsvdZ; 63:4 segment */
static inline uint64_t xhci_tr_deq_ptr(uint64_t segment_addr, unsigned int dcs)
{
	return (segment_addr & ~(uint64_t)0xFULL) | ((uint64_t)(dcs) & 1U);
}

/* CRCR — low bits preserved on init (CMD_RING_RSVD_BITS) */
#define XHCI_CRCR_PRESERVE_MASK UINT64_C(0x3f)

/* CRCR */
#define XHCI_CRCR_RCS BIT64(0)
#define XHCI_CRCR_CS  BIT64(1)
#define XHCI_CRCR_CA  BIT64(2)
#define XHCI_CRCR_CRR BIT64(3)

/* Port speeds */
#define XHCI_SPEED_FULL  1U
#define XHCI_SPEED_LOW   2U
#define XHCI_SPEED_HIGH  3U
#define XHCI_SPEED_SUPER 4U

/* TRB types */
#define XHCI_TRB_NORMAL             1U
#define XHCI_TRB_SETUP              2U
#define XHCI_TRB_DATA               3U
#define XHCI_TRB_STATUS             4U
#define XHCI_TRB_LINK               6U
#define XHCI_TRB_NOOP               8U
#define XHCI_TRB_ENABLE_SLOT        9U
#define XHCI_TRB_DISABLE_SLOT       10U
#define XHCI_TRB_ADDRESS_DEVICE     11U
#define XHCI_TRB_CONFIGURE_EP       12U
#define XHCI_TRB_EVAL_CONTEXT       13U
#define XHCI_TRB_RESET_EP           14U
#define XHCI_TRB_STOP_RING          15U
#define XHCI_TRB_SET_TR_DEQUEUE     16U
#define XHCI_TRB_RESET_DEVICE       17U
#define XHCI_TRB_TRANSFER_EVENT     32U
#define XHCI_TRB_COMMAND_COMPLETION 33U
#define XHCI_TRB_PORT_STATUS        34U

/* TRB control bits */
#define XHCI_TRB_CYCLE           BIT(0)
/* Transfer Event TRB DW3 (xHCI Table 6-95): Parameter is Event Data, not TRB address. */
#define XHCI_TRB_TRANSFER_EVT_ED BIT(1)
#define XHCI_TRB_ISP             BIT(2)
#define XHCI_TRB_CHAIN           BIT(4)
#define XHCI_TRB_IOC             BIT(5)
#define XHCI_TRB_IDT             BIT(6)
#define XHCI_TRB_BSR             BIT(9)
/* Reset Endpoint command uses bit 9 as TSP (Transfer State Preserve); xHCI TRB_TSP */
#define XHCI_TRB_RESET_EP_TSP    XHCI_TRB_BSR
#define XHCI_TRB_DIR_IN          BIT(16)

#define XHCI_TRB_TYPE(t)          (((uint32_t)(t) & 0x3fU) << 10)
#define XHCI_TRB_TYPE_MASK        (0x3fU << 10)
#define XHCI_TRB_FIELD_TO_TYPE(f) (((f) & XHCI_TRB_TYPE_MASK) >> 10)

#define XHCI_TRB_LEN(n)         ((uint32_t)(n) & 0x1ffffU)
#define XHCI_TRB_TD_SIZE(n)     (((uint32_t)(n) & 0x1fU) << 17)
#define XHCI_TRB_INTR_TARGET(n) (((uint32_t)(n) & 0x3ffU) << 22)

/* Setup TRB transfer type */
#define XHCI_TRB_TRT_NO_DATA 0U
#define XHCI_TRB_TRT_OUT     2U
#define XHCI_TRB_TRT_IN      3U
#define XHCI_TRB_TRT(t)      (((uint32_t)(t) & 3U) << 16)

/* Link TRB toggle cycle */
#define XHCI_TRB_LINK_TC BIT(1)

/* Completion codes */
#define XHCI_COMP_SUCCESS                1U
#define XHCI_COMP_TRB_ERROR              5U
#define XHCI_COMP_SHORT_PACKET           13U
#define XHCI_COMP_STALL_ERROR            6U
#define XHCI_COMP_BABBLE_DETECTED        3U
#define XHCI_COMP_COMMAND_RING_STOPPED   24U
/*
 * Stop Endpoint transfer events (xHCI.h / xHCI 1.2 Table 6-96). Do not
 * treat as the IOC completion for the doorbelled TD — ignore for xfer_sem.
 * DWC3 reports 26; some stacks/docs also list 27 (stopped / length invalid).
 */
#define XHCI_COMP_STOPPED                26U
#define XHCI_COMP_STOPPED_LENGTH_INVALID 27U

#define XHCI_TRB_GET_COMP_CODE(p)         (((uint32_t)(p) >> 24) & 0xffU)
#define XHCI_TRB_TO_SLOT_ID(p)            (((uint32_t)(p) >> 24) & 0xffU)
#define XHCI_TRB_TO_EP_ID(p)              (((uint32_t)(p) >> 16) & 0x1fU)
#define XHCI_TRB_SLOT_ID(id)              (((uint32_t)(id) & 0xffU) << 24)
/* Command TRB EP index field (bits 16–20): xHCI uses ep_index + 1 (DCI 1 = EP0 ctrl). */
#define XHCI_TRB_EP_INDEX_FOR_CMD(ep_idx) (((uint32_t)((ep_idx) + 1U) & 0x1fU) << 16)
#define XHCI_TRB_PORT_ID(p)               (((uint32_t)(p) >> 24) & 0xffU)

/* Slot context fields */
#define XHCI_SLOT_LAST_CTX(n)      (((uint32_t)(n) & 0x1fU) << 27)
#define XHCI_SLOT_SPEED(n)         (((uint32_t)(n) & 0xfU) << 20)
#define XHCI_SLOT_ROOT_HUB_PORT(p) (((uint32_t)(p) & 0xffU) << 16)

/* Endpoint context fields */
#define XHCI_EP_CTX_TYPE(t)             (((uint32_t)(t) & 7U) << 3)
#define XHCI_EP_CTX_TYPE_ISO_OUT        1U
#define XHCI_EP_CTX_TYPE_BULK_OUT       2U
#define XHCI_EP_CTX_TYPE_INT_OUT        3U
#define XHCI_EP_CTX_TYPE_CTRL           4U
#define XHCI_EP_CTX_TYPE_ISO_IN         5U
#define XHCI_EP_CTX_TYPE_BULK_IN        6U
#define XHCI_EP_CTX_TYPE_INT_IN         7U
#define XHCI_EP_CTX_CERR(n)             (((uint32_t)(n) & 3U) << 1)
#define XHCI_EP_CTX_MAX_BURST(n)        (((uint32_t)(n) & 0xffU) << 8)
#define XHCI_EP_CTX_MAX_PACKET(n)       (((uint32_t)(n) & 0xffffU) << 16)
/*
 * Output EP Context: Endpoint State in ep_info bits 2:0 (xHCI1.2 Table 6-7).
 * Must match xHCI.h / hardware — not 1..5 with RUNNING=2 (that encodes Halt).
 */
#define XHCI_EP_CTX_EP_STATE_MASK       0x7U
#define XHCI_EP_CTX_EP_STATE_INVALID    0U
#define XHCI_EP_CTX_EP_STATE_DISABLED   0U /* input: EP not in schedule; same as Invalid */
#define XHCI_EP_CTX_EP_STATE_RUNNING    1U
#define XHCI_EP_CTX_EP_STATE_HALTED     2U
#define XHCI_EP_CTX_EP_STATE_STOPPED    3U
#define XHCI_EP_CTX_EP_STATE_ERROR      4U
/* ep_info dword: Interval for interrupt/isoch (bits 16–23), not ep_info2 */
#define XHCI_EP_CTX_EP_INFO_INTERVAL(n) (((uint32_t)(n) & 0xffU) << 16)
#define XHCI_EP_CTX_DEQ_CYCLE           BIT64(0)
#define XHCI_EP_AVG_TRB_LEN(n)          ((uint32_t)(n) & 0xffffU)

/* Input control context flags */
#define XHCI_CTX_FLAG_SLOT BIT(0)
#define XHCI_CTX_FLAG_EP0  BIT(1)

/*
 * Input Control Context is 32 bytes (xHCI §6.2.2). The next context entry
 * (Device Slot) is usually at +32 when Csz==32. When Csz==64, several
 * integrated xHCI/DWC3 hosts align the first device context to the next 64 B
 * boundary (ICC + 32 B pad), so the slot context starts at offset 64 — not 32.
 * EP0 follows immediately after the slot entry: slot_off + ctx_bytes.
 */
#define XHCI_INPUT_CTRL_CTX_SIZE 32U

#define XHCI_COMP_USB_TRANSACTION_ERROR 4U
#define XHCI_COMP_PARAMETER_ERROR       17U
#define XHCI_COMP_CONTEXT_STATE_ERROR   19U

static inline uint32_t xhci_input_ctx_slot_offset(uint32_t ctx_bytes)
{
	const uint32_t icc = XHCI_INPUT_CTRL_CTX_SIZE;

	return (ctx_bytes > icc) ? ctx_bytes : icc;
}

static inline uint32_t xhci_input_ctx_ep0_offset(uint32_t ctx_bytes)
{
	return xhci_input_ctx_slot_offset(ctx_bytes) + ctx_bytes;
}

static inline uint32_t xhci_ep_ctx_ep_state(uint32_t ep_info)
{
	return ep_info & XHCI_EP_CTX_EP_STATE_MASK;
}

/* Endpoint Type field in ep_info2 bits 5:3 (same encoding as XHCI_EP_CTX_TYPE). */
static inline uint32_t xhci_ep_ctx_ep_type_from_ep_info2(uint32_t ep_info2)
{
	return (ep_info2 >> 3) & 7U;
}

/* CONFIG register */
#define XHCI_CONFIG_SLOTS_MASK 0xffU

/* Generic TRB (16 bytes, little-endian in memory) */
struct xhci_trb {
	uint32_t param_lo;
	uint32_t param_hi;
	uint32_t status;
	uint32_t control;
} __packed;

/* Event Ring Segment Table Entry */
struct xhci_erst_entry {
	uint64_t seg_addr;
	uint32_t seg_size;
	uint32_t rsvd;
} __packed;

/* Slot context (32 or 64 bytes depending on HCC) */
struct xhci_slot_ctx {
	uint32_t dev_info;
	uint32_t dev_info2;
	uint32_t tt_info;
	uint32_t dev_state;
	uint32_t reserved[4];
} __packed;

/* Endpoint context */
struct xhci_ep_ctx {
	uint32_t ep_info;
	uint32_t ep_info2;
	uint64_t deq;
	uint32_t tx_info;
	uint32_t reserved[3];
} __packed;

/* Input control context */
struct xhci_input_ctrl_ctx {
	uint32_t drop_flags;
	uint32_t add_flags;
	uint32_t reserved[6];
} __packed;

#endif /* ZEPHYR_USB_XHCI_HW_H */
