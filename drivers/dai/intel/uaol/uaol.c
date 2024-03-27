/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <adsp_shim.h>
#include <adsp_timestamp.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dai.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/arch/common/sys_io.h>
#include "uaol_regs.h"
#include "uaol.h"

#define DT_DRV_COMPAT		intel_uaol_dai

#define DAI_DIR_PLAYBACK	0
#define DAI_DIR_CAPTURE		1

#define US_PER_SECOND		1000000

static struct k_spinlock lock;

#define INTEL_UAOL_LINK_INST_DEFINE(node_id) {					\
	.ip_base = DT_REG_ADDR(node_id),					\
	.shim_base = DT_REG_ADDR(DT_PHANDLE(node_id, shim)),			\
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(hdamluaol)),			\
		(.hdaml_base = DT_REG_ADDR(DT_NODELABEL(hdamluaol)),))		\
	.link_idx = DT_PROP(node_id, link),					\
	.acquire_count = 0,							\
	.is_initialized = false,						\
},

static struct intel_uaol_link uaol_link_table[] = {
	DT_FOREACH_STATUS_OKAY(intel_uaol_link, INTEL_UAOL_LINK_INST_DEFINE)
};

static uint32_t uaol_get_link_count(void)
{
	return ARRAY_SIZE(uaol_link_table);
}

static struct intel_uaol_link *uaol_get_link(uint32_t link_idx)
{
	uint32_t link_count = uaol_get_link_count();
	uint32_t i;

	for (i = 0; i < link_count; i++) {
		if (uaol_link_table[i].link_idx == link_idx) {
			return &uaol_link_table[i];
		}
	}

	return NULL;
}

/*
 * Waits for the link power state transition (if any) to complete.
 * Returns immediately, if no power change in progress.
 */
static int uaol_wait_link_power_change(struct intel_uaol_link *link)
{
	union UAOLCTL ctl;
#if CONFIG_SOC_INTEL_ACE15_MTPM
	mem_addr_t reg_base = link->shim_base;
#elif CONFIG_SOC_INTEL_ACE20_LNL
	mem_addr_t reg_base = link->hdaml_base;
#endif
	uint64_t start_time = sys_clock_cycle_get_64();
	uint64_t timeout = k_us_to_cyc_ceil64(INTEL_UAOL_LINK_POWER_CHANGE_TIMEOUT);

	while (1) {
		ctl.full = sys_read32(reg_base + UAOLCTL_OFFSET);
		if (ctl.part.cpa == ctl.part.spa) {
			break;
		}
		if (sys_clock_cycle_get_64() - start_time > timeout) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}

	return 0;
}

/*
 * Sets power state (up/down) for UAOL link.
 */
static int uaol_set_link_power(struct intel_uaol_link *link, bool power)
{
	union UAOLCTL ctl;
#if CONFIG_SOC_INTEL_ACE15_MTPM
	mem_addr_t reg_base = link->shim_base;
#elif CONFIG_SOC_INTEL_ACE20_LNL
	mem_addr_t reg_base = link->hdaml_base;
#endif
	int ret;

	ret = uaol_wait_link_power_change(link);
	if (ret) {
		return ret;
	}

	ctl.full = sys_read32(reg_base + UAOLCTL_OFFSET);
	if (ctl.part.spa == power) {
		return 0;
	}
	ctl.part.spa = power;
	sys_write32(ctl.full, reg_base + UAOLCTL_OFFSET);

	ret = uaol_wait_link_power_change(link);
	if (ret) {
		return ret;
	}

	return 0;
}

/*
 * Gets HW capabilities for UAOL link.
 */
static void uaol_get_link_caps(struct intel_uaol_link *link, struct intel_uaol_link_caps *caps)
{
	union UAOLxPCMSCAP pcmscap;
	union UAOLxOPC opc;
	union UAOLxIPC ipc;

	pcmscap.full = sys_read16(link->shim_base + UAOLxPCMSCAP_OFFSET);
	opc.full = sys_read16(link->ip_base + UAOLxOPC_OFFSET);
	ipc.full = sys_read16(link->ip_base + UAOLxIPC_OFFSET);

	caps->input_streams_supported = pcmscap.part.iss;
	caps->output_streams_supported = pcmscap.part.oss;
	caps->bidirectional_streams_supported = pcmscap.part.bss;
	caps->rsvd = 0;
	caps->max_tx_fifo_size = opc.part.opc;
	caps->max_rx_fifo_size = ipc.part.ipc;
}

/*
 * Programs the target destination ID of the xHCI Controller for UAOL link.
 */
static void uaol_program_bdf(struct intel_uaol_link *link, uint8_t bus, uint8_t dev, uint8_t func)
{
	union UAOLxTBDF tbdf = {0};

	tbdf.part.busn = bus;
	tbdf.part.devn = dev;
	tbdf.part.fncn = func;
	sys_write16(tbdf.full, link->ip_base + UAOLxTBDF_OFFSET);
}

/*
 * Programs the FIFO Start Address Offset for each stream of UAOL link.
 */
static void uaol_program_fifo_sao(struct intel_uaol_link *link, struct intel_uaol_fifo_sao *fsao)
{
	sys_write16(fsao->tx0_fifo_sao, link->ip_base + UAOLxPCMSyFSA_OFFSET(0));
	sys_write16(fsao->tx1_fifo_sao, link->ip_base + UAOLxPCMSyFSA_OFFSET(1));
	sys_write16(fsao->rx0_fifo_sao, link->ip_base + UAOLxPCMSyFSA_OFFSET(2));
	sys_write16(fsao->rx1_fifo_sao, link->ip_base + UAOLxPCMSyFSA_OFFSET(3));
	sys_write16(0, link->ip_base + UAOLxPCMSyFSA_OFFSET(4));
	sys_write16(8, link->ip_base + UAOLxPCMSyFSA_OFFSET(5));
}

#if CONFIG_SOC_INTEL_ACE15_MTPM
/*
 * Programs individual PCM FIFO port mapping register for the UAOL stream
 * according to ALH stream number.
 */
static void uaol_associate_alh_stream(struct intel_uaol_link *link, uint32_t stream_idx,
				      uint32_t alh_stream)
{
	union UAOLxPCMSyCM pcms_cm;

	pcms_cm.full = sys_read16(link->shim_base + UAOLxPCMSyCM_OFFSET(stream_idx));
	pcms_cm.part.strm = alh_stream;
	pcms_cm.part.lchan = 0;
	pcms_cm.part.hchan = 0xF;
	sys_write16(pcms_cm.full, link->shim_base + UAOLxPCMSyCM_OFFSET(stream_idx));
}
#endif

#if CONFIG_SOC_INTEL_ACE20_LNL
/*
 * Programs individual PCM FIFO port mapping register for the UAOL stream
 * with arbitrary raw value.
 */
static void uaol_program_fifo_port_mapping(struct intel_uaol_link *link, uint32_t stream_idx,
					   uint16_t config_word)
{
	union UAOLxPCMSyCM pcms_cm_new;
	union UAOLxPCMSyCM pcms_cm;

	pcms_cm_new.full = config_word;
	pcms_cm.full = sys_read16(link->shim_base + UAOLxPCMSyCM_OFFSET(stream_idx));
	pcms_cm.part.strm = pcms_cm_new.part.strm;
	pcms_cm.part.lchan = pcms_cm_new.part.lchan;
	pcms_cm.part.hchan = pcms_cm_new.part.hchan;
	sys_write16(pcms_cm.full, link->shim_base + UAOLxPCMSyCM_OFFSET(stream_idx));
}
#endif

/*
 * Sends a xHCI message using the Immediate Command mechanism for UAOL link.
 */
static int uaol_send_xhci_msg(struct intel_uaol_link *link, uint32_t *msg, size_t msg_size)
{
	union UAOLxIC ic;
	uint64_t start_time;
	uint64_t timeout = k_us_to_cyc_ceil64(INTEL_XHCI_MSG_TIMEOUT);
	size_t dword_idx = 0;
	size_t dword_count = msg_size / sizeof(msg[0]);
	int i;

	do {
		/* write a 4x32-bit message chunk */
		for (i = 0; i < 4 && dword_idx < dword_count; i++) {
			sys_write32(msg[dword_idx++], link->ip_base + UAOLxICPy_OFFSET(i));
		}

		/* issue command over the link */
		ic.full = sys_read32(link->ip_base + UAOLxIC_OFFSET);
		ic.part.icb = 1;
		sys_write32(ic.full, link->ip_base + UAOLxIC_OFFSET);

		/* wait for command to be sent */
		start_time = sys_clock_cycle_get_64();
		while (1) {
			ic.full = sys_read32(link->ip_base + UAOLxIC_OFFSET);
			if (ic.part.icb == 0) {
				break;
			}
			if (sys_clock_cycle_get_64() - start_time > timeout) {
				return -ETIMEDOUT;
			}
			k_busy_wait(1);
		}

		/* loop more if HW expects more payload and there's more data in buffer */
	} while (ic.part.icmp == 1 && dword_idx < dword_count);

	if (ic.part.icmp == 1 || dword_idx < dword_count) {
		return -EIO;
	}

	return 0;
}

/*
 * Receives a xHCI message using the Immediate Response mechanism for UAOL link.
 */
static int uaol_receive_xhci_msg(struct intel_uaol_link *link, uint32_t *msg, size_t msg_size)
{
	union UAOLxIR ir;
	uint64_t start_time;
	uint64_t timeout = k_us_to_cyc_ceil64(INTEL_XHCI_MSG_TIMEOUT);
	size_t dword_idx = 0;
	size_t dword_count = msg_size / sizeof(msg[0]);
	int i;

	do {
		/* wait for response message chunk to be available */
		start_time = sys_clock_cycle_get_64();
		while (1) {
			ir.full = sys_read32(link->ip_base + UAOLxIR_OFFSET);
			if (ir.part.irvi) {
				break;
			}
			if (sys_clock_cycle_get_64() - start_time > timeout) {
				return -ETIMEDOUT;
			}
			k_busy_wait(1);
		}

		/* read a 4x32-bit message chunk */
		for (i = 0; i < 4 && dword_idx < dword_count; i++) {
			msg[dword_idx++] = sys_read32(link->ip_base + UAOLxIRPy_OFFSET(i));
		}

		/* clear IRVI (RW/1C) bit */
		ir.full = sys_read32(link->ip_base + UAOLxIR_OFFSET);
		ir.part.irvi = 1;
		sys_write32(ir.full, link->ip_base + UAOLxIR_OFFSET);

		/* loop more if HW anticipates more payload and there's free space in buffer */
	} while (ir.part.irmp == 1 && dword_idx < dword_count);

	if (ir.part.irmp == 1 || dword_idx < dword_count) {
		return -EIO;
	}

	return 0;
}

/*
 * Converts service interval from a value in microseconds to 4-bit code
 * writable to UAOLxPCMSyCTL register. A valid service interval should be
 * equal to (2 ^ N) * UFRAME_PERIOD, where N is a 4-bit code.
 */
static uint8_t uaol_convert_service_interval_us_to_4bit(uint32_t service_interval_us)
{
	return (31 - __builtin_clz(service_interval_us / INTEL_UAOL_UFRAME_PERIOD_US)) & 0xF;
}

/*
 * Converts service interval from 4-bit code to a value in microseconds.
 */
static uint32_t uaol_convert_service_interval_4bit_to_us(uint8_t service_interval_4bit)
{
	return (1U << (service_interval_4bit & 0xF)) * INTEL_UAOL_UFRAME_PERIOD_US;
}

/*
 * Programs operation format for UAOL individual PCM stream.
 */
static void uaol_program_stream_format(struct intel_uaol_link *link, uint32_t stream_idx,
				       uint32_t rate, uint32_t channels, uint32_t word_bits,
				       struct intel_uaol_usb_ep_info *ep_info,
				       uint32_t service_interval_us)
{
	union UAOLxPCMSyCTL pcms_ctl;
	uint32_t audio_sample_size;
	uint32_t audio_sample_block_size;
	uint32_t audio_packet_size;
	uint32_t max_packet_size;
	uint32_t mult;

	audio_sample_size = word_bits / 8;
	audio_sample_block_size = audio_sample_size * channels;
	audio_packet_size = audio_sample_block_size *
			    ((rate * service_interval_us) / US_PER_SECOND);
	max_packet_size = ep_info->usb_mps & 0x7FF;

	if (ep_info->direction == INTEL_UAOL_STREAM_DIRECTION_IN) {
		mult = ((ep_info->usb_mps >> 11) & 0x3) + 1;
		audio_packet_size = mult * max_packet_size;
	}

	if (ep_info->direction == INTEL_UAOL_STREAM_DIRECTION_OUT && ep_info->split_ep) {
		max_packet_size = MIN(max_packet_size, INTEL_UAOL_PCM_CTL_MPS_SPLIT_EP);
	}

	pcms_ctl.full = sys_read64(link->ip_base + UAOLxPCMSyCTL_OFFSET(stream_idx));
	pcms_ctl.part.si = uaol_convert_service_interval_us_to_4bit(service_interval_us);
	pcms_ctl.part.ass = audio_sample_size - 1;
	pcms_ctl.part.asbs = audio_sample_block_size;
	pcms_ctl.part.aps = audio_packet_size;
	pcms_ctl.part.mps = max_packet_size;
	pcms_ctl.part.pm = DIV_ROUND_UP(audio_packet_size, max_packet_size);
	sys_write64(pcms_ctl.full, link->ip_base + UAOLxPCMSyCTL_OFFSET(stream_idx));
}

/*
 * Programs operation format for UAOL feedback stream.
 */
static void uaol_program_feedback_format(struct intel_uaol_link *link, uint32_t stream_idx,
					 uint32_t service_interval_us)
{
	union UAOLxPCMSyCTL pcms_ctl;

	pcms_ctl.full = sys_read64(link->ip_base + UAOLxPCMSyCTL_OFFSET(stream_idx));
	pcms_ctl.part.si = uaol_convert_service_interval_us_to_4bit(service_interval_us);
	pcms_ctl.part.ass = INTEL_UAOL_FEEDBACK_SAMPLE_SIZE - 1;
	pcms_ctl.part.asbs = INTEL_UAOL_FEEDBACK_SAMPLE_SIZE; /* Audio Sample Block Size */
	pcms_ctl.part.aps = INTEL_UAOL_FEEDBACK_SAMPLE_SIZE; /* Audio Packet Size */
	pcms_ctl.part.mps = INTEL_UAOL_FEEDBACK_SAMPLE_SIZE; /* Max Packet Size */
	pcms_ctl.part.pm = 1; /* Packet Multiplier - CEILING(APS/MPS) */
	sys_write64(pcms_ctl.full, link->ip_base + UAOLxPCMSyCTL_OFFSET(stream_idx));
}

/*
 * Calculates the greatest common divisor of unsigned numbers.
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
 * Programs the M/N divider in the UAOL stream rate adjustment register.
 */
static void uaol_program_stream_rate_adjustment(struct intel_uaol_link *link, uint32_t stream_idx,
						uint32_t rate, uint32_t service_interval_us)
{
	union UAOLxPCMSyRA pcms_ra;
	uint32_t divisor;
	uint32_t fcadivm;
	uint32_t fcadivn;

	/* get a fractional part of samples count per service interval */
	fcadivm = (rate * service_interval_us) % US_PER_SECOND;
	fcadivn = US_PER_SECOND;

	/* reduce M/N fraction */
	divisor = gcd(fcadivm, fcadivn);
	fcadivm /= divisor;
	fcadivn /= divisor;

	pcms_ra.full = sys_read32(link->ip_base + UAOLxPCMSyRA_OFFSET(stream_idx));
	pcms_ra.part.fcadivm = fcadivm;
	pcms_ra.part.fcadivn = fcadivn;
	sys_write32(pcms_ra.full, link->ip_base + UAOLxPCMSyRA_OFFSET(stream_idx));
}

/*
 * Waits for the stream state transition (if any) to complete.
 * Returns immediately, if no stream state change in progress.
 */
static int uaol_wait_stream_state_change(struct intel_uaol_link *link, uint32_t stream_idx)
{
	union UAOLxPCMSyCTL pcms_ctl;
	union UAOLxPCMSySTS pcms_sts;
	uint64_t start_time = sys_clock_cycle_get_64();
	uint64_t timeout = k_us_to_cyc_ceil64(INTEL_UAOL_STREAM_STATE_CHANGE_TIMEOUT);

	while (1) {
		pcms_ctl.full = sys_read64(link->ip_base + UAOLxPCMSyCTL_OFFSET(stream_idx));
		pcms_sts.full = sys_read32(link->ip_base + UAOLxPCMSySTS_OFFSET(stream_idx));
		if (pcms_ctl.part.sen == pcms_sts.part.sbusy) {
			break;
		}
		if (sys_clock_cycle_get_64() - start_time > timeout) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}

	return 0;
}

/*
 * Sets start/stop state for UAOL stream.
 */
static int uaol_set_stream_state(struct intel_uaol_link *link, uint32_t stream_idx, bool start)
{
	union UAOLxPCMSyCTL pcms_ctl;
	int ret;

	ret = uaol_wait_stream_state_change(link, stream_idx);
	if (ret) {
		return ret;
	}

	pcms_ctl.full = sys_read64(link->ip_base + UAOLxPCMSyCTL_OFFSET(stream_idx));
	if (pcms_ctl.part.sen == start) {
		return 0;
	}
	pcms_ctl.part.sen = start;
	sys_write64(pcms_ctl.full, link->ip_base + UAOLxPCMSyCTL_OFFSET(stream_idx));

	ret = uaol_wait_stream_state_change(link, stream_idx);
	if (ret) {
		return ret;
	}

	return 0;
}

/*
 * Performs a one time adjustment to the frame counter of UAOL link.
 */
static int uaol_adjust_frame_counter(struct intel_uaol_link *link, uint32_t direction,
				      uint32_t uframe_adj, uint32_t clk_adj)
{
	union UAOLxFA fa;
	uint64_t start_time = sys_clock_cycle_get_64();
	uint64_t timeout = k_us_to_cyc_ceil64(INTEL_UAOL_FRAME_ADJUST_TIMEOUT);

	fa.full = sys_read32(link->ip_base + UAOLxFA_OFFSET);
	fa.part.acimfcnt = clk_adj;
	fa.part.amfcnt = uframe_adj % INTEL_UAOL_UFRAMES_PER_MILLISEC;
	fa.part.afcnt = uframe_adj / INTEL_UAOL_UFRAMES_PER_MILLISEC;
	fa.part.adir = direction;
	fa.part.adj = 1;
	sys_write32(fa.full, link->ip_base + UAOLxFA_OFFSET);

	while (1) {
		fa.full = sys_read32(link->ip_base + UAOLxFA_OFFSET);
		if (fa.part.adj == 0) {
			break;
		}
		if (sys_clock_cycle_get_64() - start_time > timeout) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}

	return 0;
}

/*
 * Asserts or de-asserts reset for a specified UAOL stream.
 */
static void uaol_reset_stream(struct intel_uaol_link *link, uint32_t stream_idx, bool reset)
{
	union UAOLxPCMSyCTL pcms_ctl;

	pcms_ctl.full = sys_read64(link->ip_base + UAOLxPCMSyCTL_OFFSET(stream_idx));
	pcms_ctl.part.srst = reset;
	sys_write64(pcms_ctl.full, link->ip_base + UAOLxPCMSyCTL_OFFSET(stream_idx));
}

/*
 * Gets the system-wide (HH) timestamp of xHCI Frame Counter.
 */
static int uaol_get_xhci_timestamp(struct intel_uaol_link *link, uint64_t *timestamp)
{
	struct intel_uaol_xhci_cmd cmd = {0};
	struct intel_uaol_xhci_hh_timestamp_resp resp = {0};
	uint64_t art;
	uint32_t uframe;
	uint32_t clk_in_uframe;
	int ret;

	cmd.type = XHCI_MSG_TYPE_CMD;
	cmd.length = sizeof(cmd.payload);
	cmd.payload.recipient = XHCI_CMD_RECIPIENT_DEVICE;
	cmd.payload.type = XHCI_CMD_TYPE_CLASS;
	cmd.payload.direction = XHCI_CMD_DIR_DEVICE_TO_HOST;
	cmd.payload.bRequest = XHCI_CMD_REQUEST_GET_HH_TIMESTAMP;
	cmd.payload.wValue = 0;
	cmd.payload.wIndex = 0;
	cmd.payload.wLength = sizeof(resp.payload);

	ret = uaol_send_xhci_msg(link, (uint32_t *)&cmd, sizeof(cmd));
	if (ret) {
		return ret;
	}

	ret = uaol_receive_xhci_msg(link, (uint32_t *)&resp, sizeof(resp));
	if (ret) {
		return ret;
	}

	art = resp.payload.global_time * link->art_divider.multiplier / link->art_divider.divider;
	uframe = resp.payload.cmfi;
	clk_in_uframe = resp.payload.cmfb * INTEL_UAOL_TIMESTAMP_CLK / INTEL_XHCI_TIMESTAMP_CLK;

	/* Recalculate the timestamp to refer to when the Frame Counter was 0 */
	*timestamp = art - (uframe * INTEL_UAOL_CLK_PER_UFRAME + clk_in_uframe);

	return 0;
}

/*
 * Gets the system-wide (HH) timestamp of UAOL link Frame Counter.
 */
static int uaol_get_link_timestamp(struct intel_uaol_link *link, uint64_t *timestamp)
{
	uint32_t tsctrl = 0;
	struct intel_adsp_timestamp adsp_timestamp;
	union UAOLxFC fc;
	uint64_t art;
	uint32_t uframe;
	uint32_t clk_in_uframe;
	int ret;

	if (link->link_idx != 0) {
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
	*timestamp = art - (uframe * INTEL_UAOL_CLK_PER_UFRAME + clk_in_uframe);

	return 0;
}

static int uaol_align_frame_to_xhci(struct intel_uaol_link *link)
{
	uint64_t timestamp_usb;
	uint64_t timestamp_uaol;
	int64_t timestamp_diff;
	uint32_t adj_direction;
	uint32_t uframe_adj;
	uint32_t clk_adj;
	int ret;

	ret = uaol_get_xhci_timestamp(link, &timestamp_usb);
	if (ret) {
		return ret;
	}

	ret = uaol_get_link_timestamp(link, &timestamp_uaol);
	if (ret) {
		return ret;
	}

	timestamp_diff = timestamp_uaol - timestamp_usb;
	adj_direction = (timestamp_diff < 0);
	uframe_adj = llabs(timestamp_diff) / INTEL_UAOL_CLK_PER_UFRAME;
	clk_adj = llabs(timestamp_diff) % INTEL_UAOL_CLK_PER_UFRAME;

	ret = uaol_adjust_frame_counter(link, adj_direction, uframe_adj, clk_adj);
	if (ret) {
		return ret;
	}

	return 0;
}

static bool uaol_link_is_acquired(struct intel_uaol_link *link)
{
	return (link->acquire_count != 0);
}

static int uaol_acquire_link(struct intel_uaol_link *link)
{
	int ret;

	if (link->acquire_count == 0) {
		ret = uaol_set_link_power(link, true);
		if (ret) {
			return ret;
		}
	}
	link->acquire_count++;

	return 0;
}

static void uaol_release_link(struct intel_uaol_link *link)
{
	if (link->acquire_count == 0) {
		return;
	}

	if (--link->acquire_count == 0) {
		uaol_set_link_power(link, false);
		link->is_initialized = false;
	}

}

static int uaol_initialize_link(struct intel_uaol_link *link)
{
	int ret;

	if (!uaol_link_is_acquired(link)) {
		return -EIO;
	}

	if (link->is_initialized) {
		return 0;
	}

	uaol_program_bdf(link, link->bdf.bus, link->bdf.device, link->bdf.function);

	uaol_program_fifo_sao(link, &link->fifo_sao);

	ret = uaol_align_frame_to_xhci(link);
	if (ret) {
		return ret;
	}

	link->is_initialized = true;

	return 0;
}

static int uaol_program_ep_table_entry(struct intel_uaol_link *link, uint32_t stream_idx,
				       struct intel_uaol_usb_ep_info *ep_info, bool valid)
{
	struct intel_uaol_xhci_cmd cmd = {0};
	struct intel_uaol_xhci_ep_table_entry_data data = {0};
	struct intel_uaol_xhci_resp resp = {0};
	uint32_t sio_pin_num = INTEL_UAOL_STREAM_TO_SIO_PIN(stream_idx);
	int ret = 0;

	/* Send command message */
	cmd.type = XHCI_MSG_TYPE_CMD;
	cmd.length = sizeof(cmd.payload);
	cmd.payload.recipient = XHCI_CMD_RECIPIENT_DEVICE;
	cmd.payload.type = XHCI_CMD_TYPE_CLASS;
	cmd.payload.direction = XHCI_CMD_DIR_HOST_TO_DEVICE;
	cmd.payload.bRequest = XHCI_CMD_REQUEST_SET_EP_TABLE_ENTRY;
	cmd.payload.wValue = 0;
	cmd.payload.wIndex = sio_pin_num;
	cmd.payload.wLength = sizeof(data.payload);
	ret = uaol_send_xhci_msg(link, (uint32_t *)&cmd, sizeof(cmd));
	if (ret) {
		return ret;
	}

	/* Send data message */
	data.type = XHCI_MSG_TYPE_DATA;
	data.length = sizeof(data.payload);
	data.payload.usb_ep_address = (ep_info->usb_ep_number << 1) | ep_info->direction;
	data.payload.device_slot_number = ep_info->device_slot_number;
	data.payload.valid = valid;
	data.payload.split_ep = ep_info->split_ep;
	data.payload.sio_pin_num = sio_pin_num;
	ret = uaol_send_xhci_msg(link, (uint32_t *)&data, sizeof(data));
	if (ret) {
		return ret;
	}

	/* Receive response message */
	ret = uaol_receive_xhci_msg(link, (uint32_t *)&resp, sizeof(resp));
	if (ret) {
		return ret;
	}

	return 0;
}

/*
 * Gets a valid AUX-config TLV from the buffer at the specified buffer position.
 * On success, the buffer position is updated to point to the next possible TLV.
 */
static struct intel_uaol_aux_tlv *uaol_get_aux_tlv(uint8_t *buf, size_t buf_size, size_t *buf_pos)
{
	struct intel_uaol_aux_tlv *tlv = (struct intel_uaol_aux_tlv *)(&buf[*buf_pos]);
	size_t bytes_avail = (*buf_pos <= buf_size) ? (buf_size - *buf_pos) : 0;
	size_t min_length;

	if (bytes_avail < offsetof(struct intel_uaol_aux_tlv, value)) {
		return NULL;
	}

	if (bytes_avail - offsetof(struct intel_uaol_aux_tlv, value) < tlv->length) {
		return NULL;
	}

	switch (tlv->type) {
	case INTEL_UAOL_AUX_XHCI_CONTROLLER_BDF:
		min_length = sizeof(tlv->value.xhci_controller_bdf);
		break;
	case INTEL_UAOL_AUX_UAOL_CONFIG:
		min_length = sizeof(tlv->value.uaol_config);
		break;
	case INTEL_UAOL_AUX_FIFO_SAO:
		min_length = sizeof(tlv->value.uaol_fifo_sao);
		break;
	case INTEL_UAOL_AUX_USB_EP_INFO:
	case INTEL_UAOL_AUX_USB_EP_FEEDBACK_INFO:
		min_length = sizeof(tlv->value.usb_ep_info);
		break;
	case INTEL_UAOL_AUX_USB_ART_DIVIDER:
		min_length = sizeof(tlv->value.art_divider);
		break;
	default:
		return NULL;
	}

	if (tlv->length < min_length) {
		return NULL;
	}

	*buf_pos += offsetof(struct intel_uaol_aux_tlv, value) + tlv->length;

	return tlv;
}

static int uaol_set_uaol_config(struct dai_intel_uaol *dp, struct intel_uaol_config *cfg)
{
	if (dp->link_idx != cfg->link_idx) {
		return -EINVAL;
	}

	if (dp->stream_idx == cfg->stream_idx) {
		dp->stream_type = INTEL_UAOL_STREAM_TYPE_NORMAL;
		dp->service_interval = INTEL_UAOL_SERVICE_INTERVAL_US;
		return 0;
	}

	if (dp->stream_idx == cfg->feedback_idx) {
		dp->stream_type = INTEL_UAOL_STREAM_TYPE_FEEDBACK;
		dp->service_interval = uaol_convert_service_interval_4bit_to_us(
						cfg->feedback_period);
		return 0;
	}

	return -EINVAL;
}

int uaol_set_ep_info(struct dai_intel_uaol *dp, struct intel_uaol_usb_ep_info ep_info)
{
	if (dp->stream_type == INTEL_UAOL_STREAM_TYPE_NORMAL) {
		dp->ep_info = ep_info;
	}

	return 0;
}

int uaol_set_ep_feedback_info(struct dai_intel_uaol *dp, struct intel_uaol_usb_ep_info ep_info)
{
	if (dp->stream_type == INTEL_UAOL_STREAM_TYPE_FEEDBACK) {
		dp->ep_info = ep_info;
	}

	return 0;
}

static int uaol_set_aux_config(struct dai_intel_uaol *dp, const void *data, size_t data_size)
{
	size_t data_offset = 0;
	struct intel_uaol_aux_tlv *tlv;
	int ret = 0;

	while (1) {
		tlv = uaol_get_aux_tlv((uint8_t *)data, data_size, &data_offset);
		if (!tlv) {
			break;
		}

		switch (tlv->type) {
		case INTEL_UAOL_AUX_XHCI_CONTROLLER_BDF:
			dp->link->bdf = tlv->value.xhci_controller_bdf;
			break;
		case INTEL_UAOL_AUX_UAOL_CONFIG:
			ret = uaol_set_uaol_config(dp, &tlv->value.uaol_config);
			break;
		case INTEL_UAOL_AUX_FIFO_SAO:
			dp->link->fifo_sao = tlv->value.uaol_fifo_sao;
			break;
		case INTEL_UAOL_AUX_USB_EP_INFO:
			ret = uaol_set_ep_info(dp, tlv->value.usb_ep_info);
			break;
		case INTEL_UAOL_AUX_USB_EP_FEEDBACK_INFO:
			ret = uaol_set_ep_feedback_info(dp, tlv->value.usb_ep_info);
			break;
		case INTEL_UAOL_AUX_USB_ART_DIVIDER:
			dp->link->art_divider = tlv->value.art_divider;
			break;
		default:
			break;
		}

		if (ret) {
			return ret;
		}
	}

	return 0;
}

/*
 * Gets a valid ioctl-type TLV from the buffer at the specified buffer position.
 * On success, the buffer position is updated to point to the next possible TLV.
 */
static struct intel_uaol_ioctl_tlv *uaol_get_ioctl_tlv(uint8_t *buf, size_t buf_size,
						       size_t *buf_pos)
{
	struct intel_uaol_ioctl_tlv *tlv = (struct intel_uaol_ioctl_tlv *)(&buf[*buf_pos]);
	size_t bytes_avail = (*buf_pos <= buf_size) ? (buf_size - *buf_pos) : 0;
	size_t min_length;

	if (bytes_avail < offsetof(struct intel_uaol_ioctl_tlv, value)) {
		return NULL;
	}

	if (bytes_avail - offsetof(struct intel_uaol_ioctl_tlv, value) < tlv->length) {
		return NULL;
	}

	switch (tlv->type) {
	case INTEL_UAOL_IOCTL_SET_EP_TABLE_ENTRY:
	case INTEL_UAOL_IOCTL_RESET_EP_TABLE_ENTRY:
		min_length = sizeof(tlv->value.ep_table_entry);
		break;
	case INTEL_UAOL_IOCTL_SET_EP_INFO:
	case INTEL_UAOL_IOCTL_SET_EP_FEEDBACK_INFO:
		min_length = sizeof(tlv->value.usb_ep_info);
		break;
	case INTEL_UAOL_IOCTL_SET_FEEDBACK_PERIOD:
		min_length = sizeof(tlv->value.feedback_period);
		break;
	case INTEL_UAOL_IOCTL_GET_HW_CAPS:
		min_length = 0;
		break;
	default:
		return NULL;
	}

	if (tlv->length < min_length) {
		return NULL;
	}

	*buf_pos += offsetof(struct intel_uaol_ioctl_tlv, value) + tlv->length;

	return tlv;
}

static int uaol_set_ep_table_entry(struct intel_uaol_ep_table_entry *entry)
{
	struct intel_uaol_link *link;
	int ret;

	link = uaol_get_link(entry->link_idx);
	if (!link) {
		return -EINVAL;
	}

	ret = uaol_acquire_link(link);
	if (ret) {
		return -EIO;
	}

	ret = uaol_program_ep_table_entry(link, entry->stream_idx, &entry->ep_info, true);
	if (ret) {
		return -EIO;
	}

	return 0;
}

static int uaol_reset_ep_table_entry(struct intel_uaol_ep_table_entry *entry)
{
	struct intel_uaol_link *link;
	int ret;

	link = uaol_get_link(entry->link_idx);
	if (!link) {
		return -EINVAL;
	}

	if (!uaol_link_is_acquired(link)) {
		return -EBUSY;
	}

	ret = uaol_program_ep_table_entry(link, entry->stream_idx, &entry->ep_info, false);
	if (ret) {
		return -EIO;
	}

	uaol_release_link(link);

	return 0;
}

int uaol_set_feedback_period(struct dai_intel_uaol *dp, uint32_t feedback_period)
{
	if (dp->stream_type == INTEL_UAOL_STREAM_TYPE_FEEDBACK) {
		dp->service_interval = uaol_convert_service_interval_4bit_to_us(feedback_period);
	}

	return 0;
}

/*
 * Writes UAOL HW capabilities to the buffer starting at the specified buffer position.
 * After the write, the buffer position is increased by the size of data written.
 * If there is not enough space in the buffer, no write occurs but the buffer position is
 * updated to reflect the required space for data.
 */
static int uaol_get_hw_caps(uint8_t *buf, size_t buf_size, size_t *buf_pos)
{
	uint32_t link_count = uaol_get_link_count();
	size_t caps_size = offsetof(struct intel_uaol_caps, link_caps[link_count]);
	size_t tlv_size = offsetof(struct intel_uaol_ioctl_resp_tlv, value) + caps_size;
	size_t avail_space = (*buf_pos <= buf_size) ? (buf_size - *buf_pos) : 0;
	struct intel_uaol_ioctl_resp_tlv *tlv;
	uint32_t link_idx;
	struct intel_uaol_link *link;
	int ret;

	if (tlv_size > avail_space) {
		*buf_pos += tlv_size;
		return 0;
	}

	tlv = (struct intel_uaol_ioctl_resp_tlv *)&buf[*buf_pos];

	tlv->type = INTEL_UAOL_IOCTL_GET_HW_CAPS;
	tlv->length = caps_size;
	tlv->value.uaol_caps.link_count = link_count;

	for (link_idx = 0; link_idx < link_count; link_idx++) {
		link = uaol_get_link(link_idx);
		if (!link) {
			return -EIO;
		}
		ret = uaol_acquire_link(link);
		if (ret) {
			return -EIO;
		}
		uaol_get_link_caps(link, &tlv->value.uaol_caps.link_caps[link_idx]);
		uaol_release_link(link);
	}

	*buf_pos += tlv_size;

	return 0;
}

static int dai_uaol_ioctl(const struct device *dev, const void *data, size_t data_size,
			  void *resp, size_t *resp_size)
{
	struct dai_intel_uaol *dp = (struct dai_intel_uaol *)dev->data;
	size_t data_offset = 0;
	size_t resp_buf_size = (resp_size) ? *resp_size : 0;
	size_t resp_offset = 0;
	struct intel_uaol_ioctl_tlv *tlv;
	int ret = 0;

	while (1) {
		tlv = uaol_get_ioctl_tlv((uint8_t *)data, data_size, &data_offset);
		if (!tlv) {
			break;
		}

		switch (tlv->type) {
		case INTEL_UAOL_IOCTL_SET_EP_TABLE_ENTRY:
			ret = uaol_set_ep_table_entry(&tlv->value.ep_table_entry);
			break;
		case INTEL_UAOL_IOCTL_RESET_EP_TABLE_ENTRY:
			ret = uaol_reset_ep_table_entry(&tlv->value.ep_table_entry);
			break;
		case INTEL_UAOL_IOCTL_SET_EP_INFO:
			ret = uaol_set_ep_info(dp, tlv->value.usb_ep_info);
			break;
		case INTEL_UAOL_IOCTL_SET_EP_FEEDBACK_INFO:
			ret = uaol_set_ep_feedback_info(dp, tlv->value.usb_ep_info);
			break;
		case INTEL_UAOL_IOCTL_SET_FEEDBACK_PERIOD:
			ret = uaol_set_feedback_period(dp, tlv->value.feedback_period);
			break;
		case INTEL_UAOL_IOCTL_GET_HW_CAPS:
			ret = uaol_get_hw_caps(resp, resp_buf_size, &resp_offset);
			break;
		default:
			break;
		}

		if (ret) {
			return ret;
		}
	}

	if (resp_size) {
		*resp_size = resp_offset;
	}

	return (resp_offset > resp_buf_size) ? -ENOMEM : 0;
}

static int dai_uaol_probe(const struct device *dev)
{
	struct dai_intel_uaol *dp = (struct dai_intel_uaol *)dev->data;
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&lock);

	if (dp->in_use) {
		ret = -EBUSY;
		goto out;
	}

	ret = uaol_acquire_link(dp->link);
	if (ret) {
		ret = -EIO;
		goto out;
	}

	dp->in_use = true;

out:
	k_spin_unlock(&lock, key);
	return ret;
}

static int dai_uaol_remove(const struct device *dev)
{
	struct dai_intel_uaol *dp = (struct dai_intel_uaol *)dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	key = k_spin_lock(&lock);

	if (!dp->in_use) {
		ret = -EBUSY;
		goto out;
	}

	uaol_release_link(dp->link);
	dp->in_use = false;

out:
	k_spin_unlock(&lock, key);
	return ret;
}

static int dai_uaol_config_set(const struct device *dev, const struct dai_config *cfg,
			       const void *bespoke_cfg)
{
	struct dai_intel_uaol *dp = (struct dai_intel_uaol *)dev->data;
	const struct intel_uaol_aux_config_blob *aux_cfg = bespoke_cfg;
	k_spinlock_key_t key;
	int ret = 0;

	if (!cfg) {
		return -EINVAL;
	}

	dp->config.rate = cfg->rate;
	dp->config.channels = cfg->channels;
	dp->config.word_size = cfg->word_size;
	dp->config.link_config = cfg->link_config;

	if (!aux_cfg) {
		return 0;
	}

	key = k_spin_lock(&lock);

	ret = uaol_set_aux_config(dp, aux_cfg->data, aux_cfg->length * sizeof(uint32_t));
	if (ret) {
		goto out;
	}

	ret = uaol_initialize_link(dp->link);
	if (ret) {
		goto out;
	}

	dp->state = DAI_STATE_PRE_RUNNING;

out:
	k_spin_unlock(&lock, key);
	return ret;
}

static int dai_uaol_config_get(const struct device *dev, struct dai_config *cfg, enum dai_dir dir)
{
	struct dai_intel_uaol *dp = (struct dai_intel_uaol *)dev->data;

	if (!cfg) {
		return -EINVAL;
	}

	*cfg = dp->config;

	return 0;
}

#if CONFIG_SOC_INTEL_ACE15_MTPM
static const mem_addr_t alh_base = DT_REG_ADDR(DT_NODELABEL(alh0));
#endif

static const struct dai_properties *dai_uaol_get_properties(const struct device *dev,
							    enum dai_dir dir,
							    int stream_id)
{
	struct dai_intel_uaol *dp = (struct dai_intel_uaol *)dev->data;
	struct dai_properties *props = &dp->props;

#if CONFIG_SOC_INTEL_ACE15_MTPM
	props->stream_id = dp->alh_stream;

	if (dir == DAI_DIR_PLAYBACK) {
		props->fifo_address = alh_base + STRMzTXDA_OFFSET(dp->alh_stream);
	} else {
		props->fifo_address = alh_base + STRMzRXDA_OFFSET(dp->alh_stream);
	}

	props->fifo_depth = INTEL_UAOL_GPDMA_BURST_LENGTH;
	props->dma_hs_id = dp->dma_handshake;
	props->reg_init_delay = 0;
#else
	props->stream_id = 0;
	props->fifo_address = 0;
	props->fifo_depth = 0;
	props->dma_hs_id = 0;
	props->reg_init_delay = 0;
#endif

	return props;
}

static void dai_uaol_start(struct dai_intel_uaol *dp)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&lock);

#if CONFIG_SOC_INTEL_ACE15_MTPM
	uaol_associate_alh_stream(dp->link, dp->stream_idx, dp->alh_stream);
#elif CONFIG_SOC_INTEL_ACE20_LNL
	uaol_program_fifo_port_mapping(dp->link, dp->stream_idx, dp->config.link_config);
#endif

	if (dp->stream_type == INTEL_UAOL_STREAM_TYPE_NORMAL) {
		uaol_program_stream_format(dp->link, dp->stream_idx, dp->config.rate,
				dp->config.channels, dp->config.word_size,
				&dp->ep_info, dp->service_interval);

		uaol_program_stream_rate_adjustment(dp->link, dp->stream_idx,
				dp->config.rate, dp->service_interval);
	} else {
		uaol_program_feedback_format(dp->link, dp->stream_idx, dp->service_interval);
	}

	uaol_set_stream_state(dp->link, dp->stream_idx, true);

	k_spin_unlock(&lock, key);
}

static void dai_uaol_stop(struct dai_intel_uaol *dp)
{
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&lock);

	ret = uaol_set_stream_state(dp->link, dp->stream_idx, false);
	if (ret) {
		uaol_reset_stream(dp->link, dp->stream_idx, true);
		uaol_reset_stream(dp->link, dp->stream_idx, false);
	}

	k_spin_unlock(&lock, key);
}

static int dai_uaol_trigger(const struct device *dev, enum dai_dir dir, enum dai_trigger_cmd cmd)
{
	struct dai_intel_uaol *dp = (struct dai_intel_uaol *)dev->data;

	switch (cmd) {
	case DAI_TRIGGER_PRE_START:
		break;
	case DAI_TRIGGER_START:
		if (dp->state == DAI_STATE_PAUSED ||
		    dp->state == DAI_STATE_PRE_RUNNING) {
			dai_uaol_start(dp);
			dp->state = DAI_STATE_RUNNING;
		}
		break;
	case DAI_TRIGGER_PAUSE:
		dai_uaol_stop(dp);
		dp->state = DAI_STATE_PAUSED;
		break;
	case DAI_TRIGGER_STOP:
		dp->state = DAI_STATE_PRE_RUNNING;
		break;
	case DAI_TRIGGER_COPY:
		break;
	default:
		break;
	}

	return 0;
}

static const struct dai_driver_api dai_intel_uaol_api_funcs = {
	.probe			= dai_uaol_probe,
	.remove			= dai_uaol_remove,
	.config_set		= dai_uaol_config_set,
	.config_get		= dai_uaol_config_get,
	.get_properties		= dai_uaol_get_properties,
	.trigger		= dai_uaol_trigger,
	.ioctl			= dai_uaol_ioctl
};

static int dai_intel_uaol_init_device(const struct device *dev)
{
	struct dai_intel_uaol *dp = (struct dai_intel_uaol *)dev->data;

	dp->link = uaol_get_link(dp->link_idx);

	return 0;
};

#define DAI_INTEL_UAOL_DEVICE_INIT(n)						\
	static struct dai_properties dai_intel_uaol_properties_##n;		\
										\
	static struct dai_intel_uaol dai_intel_uaol_data_##n =			\
	{	.config =							\
		{								\
			.type = DAI_INTEL_UAOL,					\
			.dai_index = DT_INST_REG_ADDR(n),			\
		},								\
		.link_idx = DT_INST_PROP(n, link),				\
		.stream_idx = DT_INST_PROP(n, stream),				\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, alh_stream),		\
			(.alh_stream = DT_INST_PROP(n, alh_stream),))		\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, dma_handshake),		\
			(.dma_handshake = DT_INST_PROP(n, dma_handshake),))	\
		.in_use = false,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
		dai_intel_uaol_init_device,					\
		NULL,								\
		&dai_intel_uaol_data_##n,					\
		&dai_intel_uaol_properties_##n,					\
		POST_KERNEL,							\
		CONFIG_DAI_INIT_PRIORITY,					\
		&dai_intel_uaol_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(DAI_INTEL_UAOL_DEVICE_INIT)
