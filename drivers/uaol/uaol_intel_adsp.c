/*
 * Copyright (c) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_adsp_uaol

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <adsp_shim.h>
#include <adsp_timestamp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uaol.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include "uaol_intel_adsp.h"

#define UAOL_UFRAMES_PER_SEC			8000
#define UAOL_UFRAMES_PER_MSEC			(UAOL_UFRAMES_PER_SEC / MSEC_PER_SEC)

#define UAOL_SERVICE_INTERVAL_BASE_USEC		125

#define UAOL_CLOCKS_PER_SEC			CONFIG_UAOL_INTEL_ADSP_CLOCK_FREQUENCY
#define UAOL_CLOCKS_PER_UFRAME			(UAOL_CLOCKS_PER_SEC / UAOL_UFRAMES_PER_SEC)

#define XHCI_CLOCKS_PER_SEC			60000000
#define XHCI_CLOCKS_PER_UFRAME			(XHCI_CLOCKS_PER_SEC / UAOL_UFRAMES_PER_SEC)

#define UAOL_POWER_CHANGE_TIMEOUT_USEC		32000
#define UAOL_STREAM_STATE_CHANGE_TIMEOUT_USEC	32000
#define UAOL_FRAME_ADJUST_TIMEOUT_USEC		32000
#define XHCI_MSG_TIMEOUT_USEC			10000

#define UAOL_STREAM_TO_SIO_PIN(stream)		(stream + 1)

/* Device run time data */
struct uaol_intel_adsp_data {
	mem_addr_t ip_base;
	mem_addr_t shim_base;
	mem_addr_t hdaml_base;
	uint32_t link;
	bool is_powered_up;
	bool is_initialized;
	uint16_t art_divider_m;
	uint16_t art_divider_n;
};

/* Helper macros for accessing registers */
#define UAOLCAP_ADDR(dp)			((dp)->hdaml_base + UAOLCAP_OFFSET)
#define UAOLCTL_ADDR(dp)			((dp)->hdaml_base + UAOLCTL_OFFSET)
#define UAOLOSIDV_ADDR(dp)			((dp)->hdaml_base + UAOLOSIDV_OFFSET)
#define UAOLSDIID_ADDR(dp)			((dp)->hdaml_base + UAOLSDIID_OFFSET)
#define UAOLEPTR_ADDR(dp)			((dp)->hdaml_base + UAOLEPTR_OFFSET)
#define UAOLxPCMSCAP_ADDR(dp)			((dp)->shim_base + UAOLxPCMSCAP_OFFSET)
#define UAOLxPCMSyCHC_ADDR(dp, y)		((dp)->shim_base + UAOLxPCMSyCHC_OFFSET(y))
#define UAOLxPCMSyCM_ADDR(dp, y)		((dp)->shim_base + UAOLxPCMSyCM_OFFSET(y))
#define UAOLxTBDF_ADDR(dp)			((dp)->ip_base + UAOLxTBDF_OFFSET)
#define UAOLxSUV_ADDR(dp)			((dp)->ip_base + UAOLxSUV_OFFSET)
#define UAOLxOPC_ADDR(dp)			((dp)->ip_base + UAOLxOPC_OFFSET)
#define UAOLxIPC_ADDR(dp)			((dp)->ip_base + UAOLxIPC_OFFSET)
#define UAOLxFC_ADDR(dp)			((dp)->ip_base + UAOLxFC_OFFSET)
#define UAOLxFA_ADDR(dp)			((dp)->ip_base + UAOLxFA_OFFSET)
#define UAOLxIC_ADDR(dp)			((dp)->ip_base + UAOLxIC_OFFSET)
#define UAOLxIR_ADDR(dp)			((dp)->ip_base + UAOLxIR_OFFSET)
#define UAOLxICPy_ADDR(dp, y)			((dp)->ip_base + UAOLxICPy_OFFSET(y))
#define UAOLxIRPy_ADDR(dp, y)			((dp)->ip_base + UAOLxIRPy_OFFSET(y))
#define UAOLxPCMSyCTL_ADDR(dp, y)		((dp)->ip_base + UAOLxPCMSyCTL_OFFSET(y))
#define UAOLxPCMSySTS_ADDR(dp, y)		((dp)->ip_base + UAOLxPCMSySTS_OFFSET(y))
#define UAOLxPCMSyRA_ADDR(dp, y)		((dp)->ip_base + UAOLxPCMSyRA_OFFSET(y))
#define UAOLxPCMSyFSA_ADDR(dp, y)		((dp)->ip_base + UAOLxPCMSyFSA_OFFSET(y))

enum xhci_msg_type {
	XHCI_MSG_TYPE_CMD  = 1,
	XHCI_MSG_TYPE_DATA = 2,
	XHCI_MSG_TYPE_RESP = 3,
};

enum xhci_cmd_recipient {
	XHCI_CMD_RECIPIENT_DEVICE    = 0,
	XHCI_CMD_RECIPIENT_INTERFACE = 1,
	XHCI_CMD_RECIPIENT_ENDPOINT  = 2,
	XHCI_CMD_RECIPIENT_OTHER     = 3,
};

enum xhci_cmd_type {
	XHCI_CMD_TYPE_STANDARD = 0,
	XHCI_CMD_TYPE_CLASS    = 1,
	XHCI_CMD_TYPE_VENDOR   = 2,
	XHCI_CMD_TYPE_RESERVED = 3,
};

enum xhci_cmd_dir {
	XHCI_CMD_DIR_HOST_TO_DEVICE = 0,
	XHCI_CMD_DIR_DEVICE_TO_HOST = 1,
};

enum xhci_cmd_request {
	XHCI_CMD_REQUEST_GET_EP_TABLE_ENTRY = 0x80,
	XHCI_CMD_REQUEST_SET_EP_TABLE_ENTRY = 0x81,
	XHCI_CMD_REQUEST_GET_HH_TIMESTAMP   = 0x82,
};

/* xHCI command message */
struct xhci_cmd_msg {
	uint8_t type;
	uint16_t length;
	struct {
		uint8_t recipient : 5;
		uint8_t type      : 2;
		uint8_t direction : 1;
		uint8_t bRequest;
		uint16_t wValue;
		uint16_t wIndex;
		uint16_t wLength;
	} __packed __aligned(1) payload;
} __packed __aligned(4);

/* xHCI response message */
struct xhci_resp_msg {
	uint8_t type;
	uint16_t length;
	uint8_t completion_code;
} __packed __aligned(4);

/* xHCI EP_TABLE_ENTRY message */
struct xhci_ep_table_entry_msg {
	uint8_t type;
	uint16_t length;
	struct {
		uint16_t usb_ep_address     : 5;
		uint16_t device_slot_number : 8;
		uint16_t split_ep           : 1;
		uint16_t rsvd0              : 1;
		uint16_t valid              : 1;
		uint8_t sio_pin_number;
		uint8_t rsvd;
	} __packed __aligned(1) payload;
} __packed __aligned(4);

/* xHCI HH_TIMESTAMP message */
struct xhci_hh_timestamp_msg {
	uint8_t type;
	uint16_t length;
	struct {
		uint16_t cmfb  : 13;
		uint16_t rsvd0 : 3;
		uint16_t cmfi  : 14;
		uint16_t rsvd1 : 2;
		uint64_t global_time;
	} __packed __aligned(1) payload;
} __packed __aligned(4);

static struct k_spinlock lock;

/*
 * Set power state (up/down) for UAOL link.
 */
static int uaol_intel_adsp_set_power(const struct device *dev, bool power)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	union UAOLCTL ctl;
	uint32_t start_time;
	uint32_t timeout = k_us_to_cyc_ceil32(UAOL_POWER_CHANGE_TIMEOUT_USEC);
	uint32_t link_mask = BIT(dp->link);

	ctl.full = sys_read32(UAOLCTL_ADDR(dp));
	if ((ctl.part.cpa & link_mask) != (ctl.part.spa & link_mask)) {
		return -EBUSY;
	}
	ctl.part.spa = (power) ? (ctl.part.spa | link_mask) : (ctl.part.spa & ~link_mask);
	sys_write32(ctl.full, UAOLCTL_ADDR(dp));

	start_time = k_cycle_get_32();
	while (1) {
		ctl.full = sys_read32(UAOLCTL_ADDR(dp));
		if ((ctl.part.cpa & link_mask) == (ctl.part.spa & link_mask)) {
			break;
		}
		if (k_cycle_get_32() - start_time > timeout) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}

	dp->is_powered_up = power;

	return 0;
}

/*
 * Read HW capabilities for UAOL link.
 */
static void uaol_intel_adsp_read_capabilities(const struct device *dev,
					      struct uaol_capabilities *caps)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	union UAOLxPCMSCAP pcmscap;
	union UAOLxOPC opc;
	union UAOLxIPC ipc;

	pcmscap.full = sys_read16(UAOLxPCMSCAP_ADDR(dp));
	opc.full = sys_read16(UAOLxOPC_ADDR(dp));
	ipc.full = sys_read16(UAOLxIPC_ADDR(dp));

	caps->input_streams = pcmscap.part.iss;
	caps->output_streams = pcmscap.part.oss;
	caps->bidirectional_streams = pcmscap.part.bss;
	caps->max_tx_fifo_size = opc.part.opc;
	caps->max_rx_fifo_size = ipc.part.ipc;
}

/*
 * Program the target destination ID of the xHCI Controller for UAOL link.
 */
static void uaol_intel_adsp_program_bdf(const struct device *dev, uint8_t bus,
					uint8_t device, uint8_t function)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	union UAOLxTBDF tbdf = {
		.part.busn = bus,
		.part.devn = device,
		.part.fncn = function,
	};

	sys_write16(tbdf.full, UAOLxTBDF_ADDR(dp));
}

/*
 * Send xHCI message using the Immediate Command mechanism for UAOL link.
 */
static int uaol_intel_adsp_send_xhci_msg(const struct device *dev, uint32_t *msg, size_t msg_size)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	union UAOLxIC ic;
	uint32_t start_time;
	uint32_t timeout = k_us_to_cyc_ceil32(XHCI_MSG_TIMEOUT_USEC);
	size_t word_idx = 0;
	size_t words_left = msg_size / sizeof(uint32_t);
	int i, chunk_words;

	do {
		/* write up to 4x32-bit message chunk */
		chunk_words = MIN(words_left, 4);
		words_left -= chunk_words;
		for (i = 0; i < chunk_words; i++) {
			sys_write32(msg[word_idx++], UAOLxICPy_ADDR(dp, i));
		}

		/* issue command over the link */
		ic.full = sys_read32(UAOLxIC_ADDR(dp));
		ic.part.icb = 1;
		sys_write32(ic.full, UAOLxIC_ADDR(dp));

		/* wait for command to be sent */
		start_time = k_cycle_get_32();
		while (1) {
			ic.full = sys_read32(UAOLxIC_ADDR(dp));
			if (ic.part.icb == 0) {
				break;
			}
			if (k_cycle_get_32() - start_time > timeout) {
				return -ETIMEDOUT;
			}
			k_busy_wait(1);
		}

	/* loop more if HW expects more payload and there's more data in buffer */
	} while (ic.part.icmp == 1 && words_left);

	if (ic.part.icmp == 1 || words_left) {
		return -EIO;
	}

	return 0;
}

/*
 * Receive xHCI message using the Immediate Response mechanism for UAOL link.
 */
static int uaol_intel_adsp_receive_xhci_msg(const struct device *dev,
					    uint32_t *msg, size_t msg_size)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	union UAOLxIR ir;
	uint32_t start_time;
	uint32_t timeout = k_us_to_cyc_ceil32(XHCI_MSG_TIMEOUT_USEC);
	size_t word_idx = 0;
	size_t words_left = msg_size / sizeof(uint32_t);
	int i, chunk_words;

	do {
		/* wait for response message chunk to be available */
		start_time = k_cycle_get_32();
		while (1) {
			ir.full = sys_read32(UAOLxIR_ADDR(dp));
			if (ir.part.irvi) {
				break;
			}
			if (k_cycle_get_32() - start_time > timeout) {
				return -ETIMEDOUT;
			}
			k_busy_wait(1);
		}

		/* read up to 4x32-bit message chunk */
		chunk_words = MIN(words_left, 4);
		words_left -= chunk_words;
		for (i = 0; i < chunk_words; i++) {
			msg[word_idx++] = sys_read32(UAOLxIRPy_ADDR(dp, i));
		}

		/* clear IRVI (RW/1C) bit */
		ir.full = sys_read32(UAOLxIR_ADDR(dp));
		ir.part.irvi = 1;
		sys_write32(ir.full, UAOLxIR_ADDR(dp));

	/* loop more if HW anticipates more payload and there's free space in buffer */
	} while (ir.part.irmp == 1 && words_left);

	if (ir.part.irmp == 1 || words_left) {
		return -EIO;
	}

	return 0;
}

/*
 * Convert service interval from a value in microseconds to 4-bit code
 * writable to UAOLxPCMSyCTL register. A valid service interval should be
 * equal to (2 ^ N) * 125 usec, where N is a 4-bit code.
 */
static uint8_t uaol_intel_adsp_encode_service_interval(uint32_t service_interval_usec)
{
	return (31 - __builtin_clz(service_interval_usec / UAOL_SERVICE_INTERVAL_BASE_USEC)) & 0xF;
}

/*
 * Program operation format for UAOL stream.
 */
static void uaol_intel_adsp_program_format(const struct device *dev, int stream,
					   uint32_t sample_rate, uint32_t channels,
					   uint32_t sample_bits, uint32_t sio_credit_size,
					   uint32_t service_interval_usec)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	union UAOLxPCMSyCTL pcms_ctl;
	uint32_t sample_size;
	uint32_t sample_block_size;
	uint32_t payload_size;

	sample_size = sample_bits / 8;
	sample_block_size = sample_size * channels;
	payload_size = sample_block_size * ((sample_rate * service_interval_usec) / USEC_PER_SEC);

	pcms_ctl.full = sys_read64(UAOLxPCMSyCTL_ADDR(dp, stream));
	pcms_ctl.part.si = uaol_intel_adsp_encode_service_interval(service_interval_usec);
	pcms_ctl.part.ass = sample_size - 1;
	pcms_ctl.part.asbs = sample_block_size;
	pcms_ctl.part.aps = payload_size;
	pcms_ctl.part.mps = sio_credit_size;
	pcms_ctl.part.pm = DIV_ROUND_UP(payload_size, sio_credit_size);
	sys_write64(pcms_ctl.full, UAOLxPCMSyCTL_ADDR(dp, stream));
}

/*
 * Calculate unsigned greatest common divisor.
 */
static uint32_t gcd(uint32_t m, uint32_t n)
{
	uint32_t temp;

	while (m) {
		temp = m;
		m = n % m;
		n = temp;
	}
	return n;
}

/*
 * Program M/N rate adjustment for UAOL stream.
 */
static void uaol_intel_adsp_program_rate_adjustment(const struct device *dev, int stream,
						    uint32_t sample_rate,
						    uint32_t service_interval_usec)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	union UAOLxPCMSyRA pcms_ra;
	uint32_t fcadivm, fcadivn, divisor;

	/* get a fractional part of samples count per service interval */
	fcadivm = (sample_rate * service_interval_usec) % USEC_PER_SEC;
	fcadivn = USEC_PER_SEC;

	/* reduce the fraction M/N */
	divisor = gcd(fcadivm, fcadivn);
	fcadivm /= divisor;
	fcadivn /= divisor;

	pcms_ra.full = sys_read32(UAOLxPCMSyRA_ADDR(dp, stream));
	pcms_ra.part.fcadivm = fcadivm;
	pcms_ra.part.fcadivn = fcadivn;
	sys_write32(pcms_ra.full, UAOLxPCMSyRA_ADDR(dp, stream));
}

/*
 * Set start/stop state for UAOL stream.
 */
static int uaol_intel_adsp_set_stream_state(const struct device *dev, int stream, bool start)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	union UAOLxPCMSyCTL pcms_ctl;
	union UAOLxPCMSySTS pcms_sts;
	uint32_t start_time;
	uint32_t timeout = k_us_to_cyc_ceil32(UAOL_STREAM_STATE_CHANGE_TIMEOUT_USEC);

	pcms_ctl.full = sys_read64(UAOLxPCMSyCTL_ADDR(dp, stream));
	pcms_sts.full = sys_read32(UAOLxPCMSySTS_ADDR(dp, stream));
	if (pcms_ctl.part.sen != pcms_sts.part.sbusy) {
		return -EBUSY;
	}
	pcms_ctl.part.sen = start;
	sys_write64(pcms_ctl.full, UAOLxPCMSyCTL_ADDR(dp, stream));

	start_time = k_cycle_get_32();
	while (1) {
		pcms_sts.full = sys_read32(UAOLxPCMSySTS_ADDR(dp, stream));
		if (pcms_ctl.part.sen == pcms_sts.part.sbusy) {
			break;
		}
		if (k_cycle_get_32() - start_time > timeout) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}

	return 0;
}

/*
 * Assert or de-assert reset for UAOL stream.
 */
static void uaol_intel_adsp_reset_stream(const struct device *dev, int stream, bool reset)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	union UAOLxPCMSyCTL pcms_ctl;

	pcms_ctl.full = sys_read64(UAOLxPCMSyCTL_ADDR(dp, stream));
	pcms_ctl.part.srst = reset;
	sys_write64(pcms_ctl.full, UAOLxPCMSyCTL_ADDR(dp, stream));
}

/*
 * Perform a one time adjustment to the frame counter of UAOL link.
 */
static int uaol_intel_adsp_adjust_frame_counter(const struct device *dev, uint32_t direction,
						uint32_t uframe_adj, uint32_t clk_adj)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	union UAOLxFA fa;
	uint32_t start_time;
	uint32_t timeout = k_us_to_cyc_ceil32(UAOL_FRAME_ADJUST_TIMEOUT_USEC);

	fa.full = sys_read32(UAOLxFA_ADDR(dp));
	if (fa.part.adj) {
		return -EBUSY;
	}
	fa.part.acimfcnt = clk_adj;
	fa.part.amfcnt = uframe_adj % UAOL_UFRAMES_PER_MSEC;
	fa.part.afcnt = uframe_adj / UAOL_UFRAMES_PER_MSEC;
	fa.part.adir = direction;
	fa.part.adj = 1;
	sys_write32(fa.full, UAOLxFA_ADDR(dp));

	start_time = k_cycle_get_32();
	while (1) {
		fa.full = sys_read32(UAOLxFA_ADDR(dp));
		if (fa.part.adj == 0) {
			break;
		}
		if (k_cycle_get_32() - start_time > timeout) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}

	return 0;
}

/*
 * Get the system-wide (HH) timestamp of xHCI Frame Counter.
 */
static int uaol_intel_adsp_get_xhci_timestamp(const struct device *dev, uint64_t *timestamp)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	struct xhci_cmd_msg cmd = {0};
	struct xhci_hh_timestamp_msg resp = {0};
	uint64_t art;
	uint32_t uframe;
	uint32_t clk_in_uframe;
	int ret;

	BUILD_ASSERT(sizeof(cmd) % sizeof(uint32_t) == 0);
	BUILD_ASSERT(sizeof(resp) % sizeof(uint32_t) == 0);

	cmd.type = XHCI_MSG_TYPE_CMD;
	cmd.length = sizeof(cmd.payload);
	cmd.payload.recipient = XHCI_CMD_RECIPIENT_DEVICE;
	cmd.payload.type = XHCI_CMD_TYPE_CLASS;
	cmd.payload.direction = XHCI_CMD_DIR_DEVICE_TO_HOST;
	cmd.payload.bRequest = XHCI_CMD_REQUEST_GET_HH_TIMESTAMP;
	cmd.payload.wValue = 0;
	cmd.payload.wIndex = 0;
	cmd.payload.wLength = sizeof(resp.payload);
	ret = uaol_intel_adsp_send_xhci_msg(dev, (uint32_t *)&cmd, sizeof(cmd));
	if (ret) {
		return ret;
	}

	ret = uaol_intel_adsp_receive_xhci_msg(dev, (uint32_t *)&resp, sizeof(resp));
	if (ret) {
		return ret;
	}

	art = resp.payload.global_time * dp->art_divider_m / dp->art_divider_n;
	uframe = resp.payload.cmfi;
	clk_in_uframe = resp.payload.cmfb * UAOL_CLOCKS_PER_UFRAME / XHCI_CLOCKS_PER_UFRAME;

	/* Recalculate the timestamp to refer to when the Frame Counter was 0 */
	*timestamp = art - (uframe * UAOL_CLOCKS_PER_UFRAME + clk_in_uframe);

	return 0;
}

/*
 * Get the system-wide (HH) timestamp of UAOL link Frame Counter.
 */
static int uaol_intel_adsp_get_link_timestamp(const struct device *dev, uint64_t *timestamp)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	uint32_t tsctrl = 0;
	struct intel_adsp_timestamp adsp_timestamp;
	union UAOLxFC fc;
	uint64_t art;
	uint32_t uframe;
	uint32_t clk_in_uframe;
	int ret;

	if (dp->link != 0) {
		return -EINVAL;
	}

	/* Use an option of additional link wallclock capturing */
	tsctrl |= FIELD_PREP(ADSP_SHIM_TSCTRL_LWCS, 1);

	/* Select which link wallclock to capture - UAOL */
	tsctrl |= FIELD_PREP(ADSP_SHIM_TSCTRL_CLNKS, 0);

	/* Request system-wide (HH) timestamping */
	tsctrl |= FIELD_PREP(ADSP_SHIM_TSCTRL_HHTSE, 1);

	ret = intel_adsp_get_timestamp(tsctrl, &adsp_timestamp);
	if (ret) {
		return ret;
	}

	art = adsp_timestamp.artcs;
	fc.full = adsp_timestamp.lwccs;
	uframe = (fc.part.frm << 3) | fc.part.mfrm;
	clk_in_uframe = fc.part.cimfrm;

	/* Recalculate the timestamp to refer to when the Frame Counter was 0 */
	*timestamp = art - (uframe * UAOL_CLOCKS_PER_UFRAME + clk_in_uframe);

	return 0;
}

static int uaol_intel_adsp_align_frame(const struct device *dev)
{
	uint64_t timestamp_usb;
	uint64_t timestamp_uaol;
	int64_t timestamp_diff;
	uint32_t adj_direction;
	uint32_t uframe_adj;
	uint32_t clk_adj;
	int ret;

	ret = uaol_intel_adsp_get_xhci_timestamp(dev, &timestamp_usb);
	if (ret) {
		return ret;
	}

	ret = uaol_intel_adsp_get_link_timestamp(dev, &timestamp_uaol);
	if (ret) {
		return ret;
	}

	timestamp_diff = timestamp_uaol - timestamp_usb;
	adj_direction = (timestamp_diff < 0);
	uframe_adj = llabs(timestamp_diff) / UAOL_CLOCKS_PER_UFRAME;
	clk_adj = llabs(timestamp_diff) % UAOL_CLOCKS_PER_UFRAME;

	ret = uaol_intel_adsp_adjust_frame_counter(dev, adj_direction, uframe_adj, clk_adj);
	if (ret) {
		return ret;
	}

	return 0;
}

static int uaol_intel_adsp_config(const struct device *dev, int stream, struct uaol_config *cfg)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	key = k_spin_lock(&lock);

	if (!dp->is_powered_up) {
		ret = -EIO;
		goto out;
	}

	if (!dp->is_initialized) {
		dp->art_divider_m = cfg->art_divider_m;
		dp->art_divider_n = cfg->art_divider_n;

		uaol_intel_adsp_program_bdf(dev, cfg->xhci_bus, cfg->xhci_device,
					    cfg->xhci_function);

		ret = uaol_intel_adsp_align_frame(dev);
		if (ret) {
			goto out;
		}

		dp->is_initialized = true;
	}

	/* Program the FIFO Start Address Offset and Channel Mapping */
	sys_write16(cfg->fifo_start_offset, UAOLxPCMSyFSA_ADDR(dp, stream));
	sys_write16(cfg->channel_map, UAOLxPCMSyCM_ADDR(dp, stream));

	uaol_intel_adsp_program_format(dev, stream, cfg->sample_rate, cfg->channels,
				       cfg->sample_bits, cfg->sio_credit_size,
				       cfg->service_interval);

	uaol_intel_adsp_program_rate_adjustment(dev, stream, cfg->sample_rate,
						cfg->service_interval);

out:
	k_spin_unlock(&lock, key);

	return ret;
}

static int uaol_intel_adsp_start(const struct device *dev, int stream)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&lock);

	if (!dp->is_powered_up) {
		ret = -EIO;
		goto out;
	}

	ret = uaol_intel_adsp_set_stream_state(dev, stream, true);
	if (ret) {
		goto out;
	}

out:
	k_spin_unlock(&lock, key);

	return ret;
}

static int uaol_intel_adsp_stop(const struct device *dev, int stream)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&lock);

	if (!dp->is_powered_up) {
		ret = -EIO;
		goto out;
	}

	ret = uaol_intel_adsp_set_stream_state(dev, stream, false);
	if (ret) {
		uaol_intel_adsp_reset_stream(dev, stream, true);
		uaol_intel_adsp_reset_stream(dev, stream, false);
		ret = 0;
	}

out:
	k_spin_unlock(&lock, key);

	return ret;
}

static int uaol_intel_adsp_program_ep_table(const struct device *dev, int stream,
					    struct uaol_ep_table_entry entry, bool valid)
{
	struct uaol_intel_adsp_data *dp = dev->data;
	struct xhci_cmd_msg cmd = {0};
	struct xhci_ep_table_entry_msg data = {0};
	struct xhci_resp_msg resp = {0};
	uint32_t sio_pin_number = UAOL_STREAM_TO_SIO_PIN(stream);
	k_spinlock_key_t key;
	int ret = 0;

	BUILD_ASSERT(sizeof(cmd) % sizeof(uint32_t) == 0);
	BUILD_ASSERT(sizeof(data) % sizeof(uint32_t) == 0);
	BUILD_ASSERT(sizeof(resp) % sizeof(uint32_t) == 0);

	key = k_spin_lock(&lock);

	if (!dp->is_powered_up) {
		ret = -EIO;
		goto out;
	}

	/* Send command message */
	cmd.type = XHCI_MSG_TYPE_CMD;
	cmd.length = sizeof(cmd.payload);
	cmd.payload.recipient = XHCI_CMD_RECIPIENT_DEVICE;
	cmd.payload.type = XHCI_CMD_TYPE_CLASS;
	cmd.payload.direction = XHCI_CMD_DIR_HOST_TO_DEVICE;
	cmd.payload.bRequest = XHCI_CMD_REQUEST_SET_EP_TABLE_ENTRY;
	cmd.payload.wValue = 0;
	cmd.payload.wIndex = sio_pin_number;
	cmd.payload.wLength = sizeof(data.payload);
	ret = uaol_intel_adsp_send_xhci_msg(dev, (uint32_t *)&cmd, sizeof(cmd));
	if (ret) {
		ret = -EIO;
		goto out;
	}

	/* Send data message */
	data.type = XHCI_MSG_TYPE_DATA;
	data.length = sizeof(data.payload);
	data.payload.usb_ep_address = entry.usb_ep_address;
	data.payload.device_slot_number = entry.device_slot_number;
	data.payload.split_ep = entry.split_ep;
	data.payload.valid = valid;
	data.payload.sio_pin_number = sio_pin_number;
	ret = uaol_intel_adsp_send_xhci_msg(dev, (uint32_t *)&data, sizeof(data));
	if (ret) {
		ret = -EIO;
		goto out;
	}

	/* Receive response message */
	ret = uaol_intel_adsp_receive_xhci_msg(dev, (uint32_t *)&resp, sizeof(resp));
	if (ret) {
		ret = -EIO;
		goto out;
	}

out:
	k_spin_unlock(&lock, key);

	return ret;
}

static int uaol_intel_adsp_get_capabilities(const struct device *dev,
					    struct uaol_capabilities *caps)
{
	k_spinlock_key_t key;
	int ret;

	ret = pm_device_runtime_get(dev);
	if (ret) {
		return -EIO;
	}

	key = k_spin_lock(&lock);

	uaol_intel_adsp_read_capabilities(dev, caps);

	k_spin_unlock(&lock, key);

	ret = pm_device_runtime_put(dev);
	if (ret) {
		return -EIO;
	}

	return ret;
}

static const struct uaol_driver_api uaol_intel_adsp_api_funcs = {
	.config = uaol_intel_adsp_config,
	.start = uaol_intel_adsp_start,
	.stop = uaol_intel_adsp_stop,
	.program_ep_table = uaol_intel_adsp_program_ep_table,
	.get_capabilities = uaol_intel_adsp_get_capabilities,
};

static int uaol_intel_adsp_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = uaol_intel_adsp_set_power(dev, true);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = uaol_intel_adsp_set_power(dev, false);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_TURN_OFF:
		/* All device pm is handled during resume and suspend */
		break;
	default:
		return -ENOTSUP;
	}
	return ret;
}

static int uaol_intel_adsp_init_device(const struct device *dev)
{
	pm_device_init_suspended(dev);

	return pm_device_runtime_enable(dev);
};

#define UAOL_INTEL_ADSP_INIT_DEVICE(n)							\
	static struct uaol_intel_adsp_data uaol_intel_adsp_data_##n = {			\
		.shim_base = DT_INST_REG_ADDR(n) + DT_INST_PROP(n, shim_offset),	\
		.ip_base = DT_INST_REG_ADDR(n) + DT_INST_PROP(n, ip_offset),		\
		IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(hdamluaol)),			\
			(.hdaml_base = DT_REG_ADDR(DT_NODELABEL(hdamluaol)),))		\
		.link = DT_INST_PROP(n, link),						\
	};										\
											\
	PM_DEVICE_DT_INST_DEFINE(n, uaol_intel_adsp_pm_action);				\
											\
	DEVICE_DT_INST_DEFINE(n,							\
		uaol_intel_adsp_init_device,						\
		PM_DEVICE_DT_INST_GET(n),						\
		&uaol_intel_adsp_data_##n,						\
		NULL,									\
		POST_KERNEL,								\
		CONFIG_UAOL_INIT_PRIORITY,						\
		&uaol_intel_adsp_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(UAOL_INTEL_ADSP_INIT_DEVICE)
