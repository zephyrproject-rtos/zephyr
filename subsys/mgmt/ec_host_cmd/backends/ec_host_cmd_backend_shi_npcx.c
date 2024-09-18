/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ec_host_cmd_backend_shi.h"

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/ec_host_cmd/backend.h>
#include <zephyr/mgmt/ec_host_cmd/ec_host_cmd.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>

#include <soc_miwu.h>

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_npcx_shi)
#define DT_DRV_COMPAT nuvoton_npcx_shi
#elif DT_HAS_COMPAT_STATUS_OKAY(nuvoton_npcx_shi_enhanced)
#define DT_DRV_COMPAT nuvoton_npcx_shi_enhanced
#endif
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "Invalid number of NPCX SHI peripherals");
BUILD_ASSERT(!(DT_HAS_COMPAT_STATUS_OKAY(nuvoton_npcx_shi) &&
	       DT_HAS_COMPAT_STATUS_OKAY(nuvoton_npcx_shi_enhanced)));

LOG_MODULE_REGISTER(host_cmd_shi_npcx, CONFIG_EC_HC_LOG_LEVEL);

/* Driver convenience defines */
#define HAL_INSTANCE(dev) (struct shi_reg *)(((const struct shi_npcx_config *)(dev)->config)->base)

/* Full output buffer size */
#define SHI_OBUF_FULL_SIZE     DT_INST_PROP(0, buffer_tx_size)
/* Full input buffer size  */
#define SHI_IBUF_FULL_SIZE     DT_INST_PROP(0, buffer_rx_size)
/* Configure the IBUFLVL2 = the size of V3 protocol header */
#define SHI_IBUFLVL2_THRESHOLD (sizeof(struct ec_host_cmd_request_header))
/* Half output buffer size */
#define SHI_OBUF_HALF_SIZE     (SHI_OBUF_FULL_SIZE / 2)
/* Half input buffer size */
#define SHI_IBUF_HALF_SIZE     (SHI_IBUF_FULL_SIZE / 2)

/*
 * Timeout to wait for SHI request packet
 *
 * This affects the slowest SPI clock we can support. A delay of 8192 us permits a 512-byte request
 * at 500 KHz, assuming the SPI controller starts sending bytes as soon as it asserts chip select.
 * That's as slow as we would practically want to run the SHI interface, since running it slower
 * significantly impacts firmware update times.
 */
#define EC_SHI_CMD_RX_TIMEOUT_US 8192

/*
 * The AP blindly clocks back bytes over the SPI interface looking for a framing byte.
 * So this preamble must always precede the actual response packet.
 */
#define EC_SHI_OUT_PREAMBLE_LENGTH 2

/*
 * Space allocation of the past-end status byte (EC_SHI_PAST_END) in the out_msg buffer.
 */
#define EC_SHI_PAST_END_LENGTH 1

/*
 * Space allocation of the frame status byte (EC_SHI_FRAME_START) in the out_msg buffer.
 */
#define EC_SHI_FRAME_START_LENGTH 1

/*
 * Offset of output parameters needs to account for pad and framing bytes and
 * one last past-end byte at the end so any additional bytes clocked out by
 * the AP will have a known and identifiable value.
 */
#define EC_SHI_PROTO3_OVERHEAD (EC_SHI_PAST_END_LENGTH + EC_SHI_FRAME_START_LENGTH)

/*
 * Our input and output msg buffers. These must be large enough for our largest
 * message, including protocol overhead. The pointers after the protocol
 * overhead, as passed to the host command handler, must be 32-bit aligned.
 */
#define SHI_OUT_START_PAD (4 * (EC_SHI_FRAME_START_LENGTH / 4 + 1))
#define SHI_OUT_END_PAD   (4 * (EC_SHI_PAST_END_LENGTH / 4 + 1))

enum shi_npcx_state {
	SHI_STATE_NONE = -1,
	/* SHI not enabled (initial state, and when chipset is off) */
	SHI_STATE_DISABLED = 0,
	/* Ready to receive next request */
	SHI_STATE_READY_TO_RECV,
	/* Receiving request */
	SHI_STATE_RECEIVING,
	/* Processing request */
	SHI_STATE_PROCESSING,
	/* Canceling response since CS deasserted and output NOT_READY byte */
	SHI_STATE_CNL_RESP_NOT_RDY,
	/* Sending response */
	SHI_STATE_SENDING,
	/* Received data is invalid */
	SHI_STATE_BAD_RECEIVED_DATA,
};

enum shi_npcx_pm_policy_state_flag {
	SHI_NPCX_PM_POLICY_FLAG,
	SHI_NPCX_PM_POLICY_FLAG_COUNT,
};

/* Device config */
struct shi_npcx_config {
	/* Serial Host Interface (SHI) base address */
	uintptr_t base;
	/* Clock configuration */
	struct npcx_clk_cfg clk_cfg;
	/* Pin control configuration */
	const struct pinctrl_dev_config *pcfg;
	/* Chip-select interrupts */
	int irq;
	struct npcx_wui shi_cs_wui;
};

struct shi_npcx_data {
	struct ec_host_cmd_rx_ctx *rx_ctx;
	struct ec_host_cmd_tx_buf *tx;
	/* Communication status */
	enum shi_npcx_state state;
	enum shi_npcx_state last_error_state;
	uint8_t *rx_msg;          /* Entry pointer of msg rx buffer   */
	uint8_t *tx_msg;          /* Entry pointer of msg tx buffer   */
	volatile uint8_t *rx_buf; /* Entry pointer of receive buffer  */
	volatile uint8_t *tx_buf; /* Entry pointer of transmit buffer */
	uint16_t sz_sending;      /* Size of sending data in bytes    */
	uint16_t sz_request;      /* Request bytes need to receive    */
	uint16_t sz_response;     /* Response bytes need to receive   */
	uint64_t rx_deadline;     /* Deadline of receiving            */
	/* Buffers */
	uint8_t out_msg_padded[SHI_OUT_START_PAD + CONFIG_EC_HOST_CMD_BACKEND_SHI_MAX_RESPONSE +
			       SHI_OUT_END_PAD] __aligned(4);
	uint8_t *const out_msg;
	uint8_t in_msg[CONFIG_EC_HOST_CMD_BACKEND_SHI_MAX_REQUEST] __aligned(4);
	ATOMIC_DEFINE(pm_policy_state_flag, SHI_NPCX_PM_POLICY_FLAG_COUNT);
};

struct ec_host_cmd_shi_npcx_ctx {
	/* SHI device instance */
	const struct device *dev;
};

#define EC_HOST_CMD_SHI_NPCX_DEFINE(_name)                                                         \
	static struct ec_host_cmd_shi_npcx_ctx _name##_hc_shi_npcx;                                \
	struct ec_host_cmd_backend _name = {                                                       \
		.api = &ec_host_cmd_api,                                                           \
		.ctx = (struct ec_host_cmd_shi_npcx_ctx *)&_name##_hc_shi_npcx,                    \
	}

/* Forward declaration */
static void shi_npcx_reset_prepare(const struct device *dev);

static void shi_npcx_pm_policy_state_lock_get(struct shi_npcx_data *data,
					      enum shi_npcx_pm_policy_state_flag flag)
{
	if (atomic_test_and_set_bit(data->pm_policy_state_flag, flag) == 0) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}

static void shi_npcx_pm_policy_state_lock_put(struct shi_npcx_data *data,
					      enum shi_npcx_pm_policy_state_flag flag)
{
	if (atomic_test_and_clear_bit(data->pm_policy_state_flag, flag) == 1) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}

/* Read pointer of input or output buffer by consecutive reading */
static uint32_t shi_npcx_read_buf_pointer(struct shi_reg *const inst)
{
	uint8_t stat;

	/* Wait for two consecutive equal values read */
	do {
		stat = inst->IBUFSTAT;
	} while (stat != inst->IBUFSTAT);

	return (uint32_t)stat;
}

/*
 * Write pointer of output buffer by consecutive reading
 * Note: this function (OBUFSTAT) should only be usd in Enhanced Buffer Mode.
 */
static uint32_t shi_npcx_write_buf_pointer(struct shi_reg *const inst)
{
	uint8_t stat;

	/* Wait for two consecutive equal values are read */
	do {
		stat = inst->OBUFSTAT;
	} while (stat != inst->OBUFSTAT);

	return stat;
}

/*
 * Valid offset of SHI output buffer to write.
 * - In Simultaneous Standard FIFO Mode (SIMUL = 1 and EBUFMD = 0):
 *   OBUFPTR cannot be used. IBUFPTR can be used instead because it points to
 *   the same location as OBUFPTR.
 * - In Simultaneous Enhanced FIFO Mode (SIMUL = 1 and EBUFMD = 1):
 *   IBUFPTR may not point to the same location as OBUFPTR.
 *   In this case OBUFPTR reflects the 128-byte payload buffer pointer only
 *   during the SPI transaction.
 */
static uint32_t shi_npcx_valid_obuf_offset(struct shi_reg *const inst)
{
	if (IS_ENABLED(CONFIG_EC_HOST_CMD_BACKEND_SHI_NPCX_ENHANCED_BUF_MODE)) {
		return shi_npcx_write_buf_pointer(inst) % SHI_OBUF_FULL_SIZE;
	} else {
		return (shi_npcx_read_buf_pointer(inst) + EC_SHI_OUT_PREAMBLE_LENGTH) %
		       SHI_OBUF_FULL_SIZE;
	}
}

/*
 * This routine write SHI next half output buffer from msg buffer
 */
static void shi_npcx_write_half_outbuf(const struct device *dev)
{
	struct shi_npcx_data *data = dev->data;

	const uint32_t size = MIN(SHI_OBUF_HALF_SIZE, data->sz_response - data->sz_sending);
	uint8_t *obuf_ptr = (uint8_t *)data->tx_buf;
	const uint8_t *obuf_end = obuf_ptr + size;
	uint8_t *msg_ptr = data->tx_msg;

	/* Fill half output buffer */
	while (obuf_ptr != obuf_end) {
		*obuf_ptr++ = *msg_ptr++;
	}

	data->sz_sending += size;
	data->tx_buf = obuf_ptr;
	data->tx_msg = msg_ptr;
}

/*
 * This routine read SHI input buffer to msg buffer until
 * we have received a certain number of bytes
 */
static int shi_npcx_read_inbuf_wait(const struct device *dev, uint32_t szbytes)
{
	struct shi_npcx_data *data = dev->data;
	struct shi_reg *const inst = HAL_INSTANCE(dev);

	/* Copy data to msg buffer from input buffer */
	for (uint32_t i = 0; i < szbytes; i++, data->rx_ctx->len++) {
		/*
		 * If input buffer pointer equals pointer which wants to read,
		 * it means data is not ready.
		 */
		while (data->rx_buf == inst->IBUF + shi_npcx_read_buf_pointer(inst)) {
			if (k_cycle_get_64() >= data->rx_deadline) {
				return 0;
			}
		}

		/* Copy data to msg buffer */
		*data->rx_msg++ = *data->rx_buf++;
	}
	return 1;
}

/* This routine fills out all SHI output buffer with status byte */
static void shi_npcx_fill_out_status(struct shi_reg *const inst, uint8_t status)
{
	uint8_t start, end;
	volatile uint8_t *fill_ptr;
	volatile uint8_t *fill_end;
	volatile uint8_t *obuf_end;

	if (IS_ENABLED(CONFIG_EC_HOST_CMD_BACKEND_SHI_NPCX_ENHANCED_BUF_MODE)) {
		/*
		 * In Enhanced Buffer Mode, SHI module outputs the status code
		 * in SBOBUF repeatedly.
		 */
		inst->SBOBUF = status;

		return;
	}

	/*
	 * Disable interrupts in case the interfere by the other interrupts.
	 * Use __disable_irq/__enable_irq instead of using irq_lock/irq_unlock
	 * here because irq_lock/irq_unlock leave some system exceptions (like
	 * SVC, NMI, and faults) still enabled.
	 */
	__disable_irq();

	/*
	 * Fill out output buffer with status byte and leave a gap for PREAMBLE.
	 * The gap guarantees the synchronization. The critical section should
	 * be done within this gap. No racing happens.
	 */
	start = shi_npcx_valid_obuf_offset(inst);
	end = (start + SHI_OBUF_FULL_SIZE - EC_SHI_OUT_PREAMBLE_LENGTH) % SHI_OBUF_FULL_SIZE;

	fill_ptr = inst->OBUF + start;
	fill_end = inst->OBUF + end;
	obuf_end = inst->OBUF + SHI_OBUF_FULL_SIZE;
	while (fill_ptr != fill_end) {
		*fill_ptr++ = status;
		if (fill_ptr == obuf_end) {
			fill_ptr = inst->OBUF;
		}
	}

	/* End of critical section */
	__enable_irq();
}

/* This routine handles shi received unexpected data */
static void shi_npcx_bad_received_data(const struct device *dev)
{
	struct shi_npcx_data *data = dev->data;
	struct shi_reg *const inst = HAL_INSTANCE(dev);

	if (IS_ENABLED(CONFIG_EC_HOST_CMD_BACKEND_SHI_NPCX_ENHANCED_BUF_MODE)) {
		inst->EVENABLE &= ~IBF_IBHF_EN_MASK;
	}

	/* State machine mismatch, timeout, or protocol we can't handle. */
	shi_npcx_fill_out_status(inst, EC_SHI_RX_BAD_DATA);
	data->state = SHI_STATE_BAD_RECEIVED_DATA;

	LOG_ERR("SHI bad data recv");
	LOG_DBG("BAD-");
	LOG_HEXDUMP_DBG(data->in_msg, data->rx_ctx->len, "in_msg=");

	/* Reset shi's state machine for error recovery */
	shi_npcx_reset_prepare(dev);

	LOG_DBG("END");
}

/*
 * This routine write SHI output buffer from msg buffer over halt of it.
 * It make sure we have enough time to handle next operations.
 */
static void shi_npcx_write_first_pkg_outbuf(const struct device *dev, uint16_t szbytes)
{
	struct shi_npcx_data *data = dev->data;
	struct shi_reg *const inst = HAL_INSTANCE(dev);
	uint8_t size, offset;
	volatile uint8_t *obuf_ptr;
	volatile uint8_t *obuf_end;
	uint8_t *msg_ptr;
	uint32_t half_buf_remain; /* Remains in half buffer are free to write */

	/* Start writing at our current OBUF position */
	offset = shi_npcx_valid_obuf_offset(inst);
	obuf_ptr = inst->OBUF + offset;
	msg_ptr = data->tx_msg;

	/* Fill up to OBUF mid point, or OBUF end */
	half_buf_remain = SHI_OBUF_HALF_SIZE - (offset % SHI_OBUF_HALF_SIZE);
	size = MIN(half_buf_remain, szbytes - data->sz_sending);
	obuf_end = obuf_ptr + size;
	while (obuf_ptr != obuf_end) {
		*obuf_ptr++ = *msg_ptr++;
	}

	/* Track bytes sent for later accounting */
	data->sz_sending += size;

	/* Write data to beginning of OBUF if we've reached the end */
	if (obuf_ptr == inst->OBUF + SHI_IBUF_FULL_SIZE) {
		obuf_ptr = inst->OBUF;
	}

	/* Fill next half output buffer */
	size = MIN(SHI_OBUF_HALF_SIZE, szbytes - data->sz_sending);
	obuf_end = obuf_ptr + size;
	while (obuf_ptr != obuf_end) {
		*obuf_ptr++ = *msg_ptr++;
	}

	/* Track bytes sent / last OBUF position written for later accounting */
	data->sz_sending += size;
	data->tx_buf = obuf_ptr;
	data->tx_msg = msg_ptr;
}

static void shi_npcx_handle_host_package(const struct device *dev)
{
	struct shi_npcx_data *data = dev->data;
	struct shi_reg *const inst = HAL_INSTANCE(dev);
	uint32_t sz_inbuf_int = data->sz_request / SHI_IBUF_HALF_SIZE;
	uint32_t cnt_inbuf_int = data->rx_ctx->len / SHI_IBUF_HALF_SIZE;

	if (sz_inbuf_int - cnt_inbuf_int) {
		/* Need to receive data from buffer */
		return;
	}

	uint32_t remain_bytes = data->sz_request - data->rx_ctx->len;

	/* Read remaining bytes from input buffer */
	if (!shi_npcx_read_inbuf_wait(dev, remain_bytes)) {
		return shi_npcx_bad_received_data(dev);
	}

	/* Move to processing state */
	data->state = SHI_STATE_PROCESSING;
	LOG_DBG("PRC-");

	if (IS_ENABLED(CONFIG_EC_HOST_CMD_BACKEND_SHI_NPCX_ENHANCED_BUF_MODE)) {
		inst->EVENABLE &= ~IBF_IBHF_EN_MASK;
	}

	/* Fill output buffer to indicate we`re processing request */
	shi_npcx_fill_out_status(inst, EC_SHI_PROCESSING);
	data->out_msg[0] = EC_SHI_FRAME_START;

	/* Wake-up the HC handler thread */
	ec_host_cmd_rx_notify();
}

static int shi_npcx_host_request_expected_size(const struct ec_host_cmd_request_header *r)
{
	/* Check host request version */
	if (r->prtcl_ver != EC_HOST_REQUEST_VERSION) {
		return 0;
	}

	/* Reserved byte should be 0 */
	if (r->reserved) {
		return 0;
	}

	return sizeof(*r) + r->data_len;
}

static void shi_npcx_parse_header(const struct device *dev)
{
	struct shi_npcx_data *data = dev->data;

	/* We're now inside a transaction */
	data->state = SHI_STATE_RECEIVING;
	LOG_DBG("RV-");

	/* Setup deadline time for receiving */
	data->rx_deadline = k_cycle_get_64() + k_us_to_cyc_near64(EC_SHI_CMD_RX_TIMEOUT_US);

	/* Wait for version, command, length bytes */
	if (!shi_npcx_read_inbuf_wait(dev, 3)) {
		return shi_npcx_bad_received_data(dev);
	}

	if (data->in_msg[0] == EC_HOST_REQUEST_VERSION) {
		/* Protocol version 3 */
		struct ec_host_cmd_request_header *r =
			(struct ec_host_cmd_request_header *)data->in_msg;
		int pkt_size;

		/*
		 * If request is over half of input buffer, we need to modify the algorithm again.
		 */
		__ASSERT_NO_MSG(sizeof(*r) < SHI_IBUF_HALF_SIZE);

		/* Wait for the rest of the command header */
		if (!shi_npcx_read_inbuf_wait(dev, sizeof(*r) - 3)) {
			return shi_npcx_bad_received_data(dev);
		}

		/* Check how big the packet should be */
		pkt_size = shi_npcx_host_request_expected_size(r);
		if (pkt_size == 0 || pkt_size > sizeof(data->in_msg)) {
			return shi_npcx_bad_received_data(dev);
		}

		/* Computing total bytes need to receive */
		data->sz_request = pkt_size;

		shi_npcx_handle_host_package(dev);
	} else {
		/* Invalid version number */
		return shi_npcx_bad_received_data(dev);
	}
}

static void shi_npcx_sec_ibf_int_enable(struct shi_reg *const inst, int enable)
{
	if (enable) {
		/* Setup IBUFLVL2 threshold and enable it */
		inst->SHICFG5 |= BIT(NPCX_SHICFG5_IBUFLVL2DIS);
		SET_FIELD(inst->SHICFG5, NPCX_SHICFG5_IBUFLVL2, SHI_IBUFLVL2_THRESHOLD);
		inst->SHICFG5 &= ~BIT(NPCX_SHICFG5_IBUFLVL2DIS);

		/* Enable IBHF2 event */
		inst->EVENABLE2 |= BIT(NPCX_EVENABLE2_IBHF2EN);
	} else {
		/* Disable IBHF2 event first */
		inst->EVENABLE2 &= ~BIT(NPCX_EVENABLE2_IBHF2EN);

		/* Disable IBUFLVL2 and set threshold back to zero */
		inst->SHICFG5 |= BIT(NPCX_SHICFG5_IBUFLVL2DIS);
		SET_FIELD(inst->SHICFG5, NPCX_SHICFG5_IBUFLVL2, 0);
	}
}

/* This routine copies SHI half input buffer data to msg buffer */
static void shi_npcx_read_half_inbuf(const struct device *dev)
{
	struct shi_npcx_data *data = dev->data;

	/*
	 * Copy to read buffer until reaching middle/top address of
	 * input buffer or completing receiving data
	 */
	do {
		/* Restore data to msg buffer */
		*data->rx_msg++ = *data->rx_buf++;
		data->rx_ctx->len++;
	} while (data->rx_ctx->len % SHI_IBUF_HALF_SIZE && data->rx_ctx->len != data->sz_request);
}

/*
 * Avoid spamming the console with prints every IBF / IBHF interrupt, if
 * we find ourselves in an unexpected state.
 */
static void shi_npcx_log_unexpected_state(const struct device *dev, char *isr_name)
{
	struct shi_npcx_data *data = dev->data;

	if (data->state != data->last_error_state) {
		LOG_ERR("Unexpected state %d in %s ISR", data->state, isr_name);
	}

	data->last_error_state = data->state;
}

static void shi_npcx_handle_cs_assert(const struct device *dev)
{
	struct shi_reg *const inst = HAL_INSTANCE(dev);
	struct shi_npcx_data *data = dev->data;

	/* If not enabled, ignore glitches on SHI_CS_L */
	if (data->state == SHI_STATE_DISABLED) {
		return;
	}

	/* NOT_READY should be sent and there're no spi transaction now. */
	if (data->state == SHI_STATE_CNL_RESP_NOT_RDY) {
		return;
	}

	/* Chip select is low = asserted */
	if (data->state != SHI_STATE_READY_TO_RECV) {
		/* State machine should be reset in EVSTAT_EOR ISR */
		LOG_ERR("Unexpected state %d in CS ISR", data->state);
		return;
	}

	LOG_DBG("CSL-");

	/*
	 * Clear possible EOR event from previous transaction since it's
	 * irrelevant now that CS is re-asserted.
	 */
	inst->EVSTAT = BIT(NPCX_EVSTAT_EOR);

	shi_npcx_pm_policy_state_lock_get(data, SHI_NPCX_PM_POLICY_FLAG);
}

static void shi_npcx_handle_cs_deassert(const struct device *dev)
{
	struct shi_reg *const inst = HAL_INSTANCE(dev);
	struct shi_npcx_data *data = dev->data;

	/*
	 * If the buffer is still used by the host command.
	 * Change state machine for response handler.
	 */
	if (data->state == SHI_STATE_PROCESSING) {
		/*
		 * Mark not ready to prevent the other
		 * transaction immediately
		 */
		shi_npcx_fill_out_status(inst, EC_SHI_NOT_READY);

		data->state = SHI_STATE_CNL_RESP_NOT_RDY;

		/*
		 * Disable SHI interrupt, it will remain disabled until shi_send_response_packet()
		 * is called and CS is asserted for a new transaction.
		 */
		irq_disable(DT_INST_IRQN(0));

		LOG_DBG("CNL-");
		return;
		/* Next transaction but we're not ready */
	} else if (data->state == SHI_STATE_CNL_RESP_NOT_RDY) {
		return;
	}

	/* Error state for checking*/
	if (data->state != SHI_STATE_SENDING) {
		shi_npcx_log_unexpected_state(dev, "CSNRE");
	}

	/* reset SHI and prepare to next transaction again */
	shi_npcx_reset_prepare(dev);
	LOG_DBG("END\n");
}

static void shi_npcx_handle_input_buf_half_full(const struct device *dev)
{
	struct shi_reg *const inst = HAL_INSTANCE(dev);
	struct shi_npcx_data *data = dev->data;

	if (data->state == SHI_STATE_RECEIVING) {
		/* Read data from input to msg buffer */
		shi_npcx_read_half_inbuf(dev);
		return shi_npcx_handle_host_package(dev);
	} else if (data->state == SHI_STATE_SENDING) {
		/* Write data from msg buffer to output buffer */
		if (data->tx_buf == inst->OBUF + SHI_OBUF_FULL_SIZE) {
			/* Write data from bottom address again */
			data->tx_buf = inst->OBUF;
			shi_npcx_write_half_outbuf(dev);
		}
	} else if (data->state == SHI_STATE_PROCESSING) {
		/* Wait for host to handle request */
	} else {
		/* Unexpected status */
		shi_npcx_log_unexpected_state(dev, "IBHF");
	}
}

static void shi_npcx_handle_input_buf_full(const struct device *dev)
{
	struct shi_npcx_data *data = dev->data;
	struct shi_reg *const inst = HAL_INSTANCE(dev);

	if (data->state == SHI_STATE_RECEIVING) {
		/* read data from input to msg buffer */
		shi_npcx_read_half_inbuf(dev);
		/* Read to bottom address again */
		data->rx_buf = inst->IBUF;
		return shi_npcx_handle_host_package(dev);
	} else if (data->state == SHI_STATE_SENDING) {
		/* Write data from msg buffer to output buffer */
		if (data->tx_buf == inst->OBUF + SHI_OBUF_HALF_SIZE) {
			shi_npcx_write_half_outbuf(dev);
		}

		return;
	} else if (data->state == SHI_STATE_PROCESSING) {
		/* Wait for host to handle request */
		return;
	}

	/* Unexpected status */
	shi_npcx_log_unexpected_state(dev, "IBF");
}

static void shi_npcx_isr(const struct device *dev)
{
	struct shi_reg *const inst = HAL_INSTANCE(dev);
	uint8_t stat;
	uint8_t stat2;

	/* Read status register and clear interrupt status early */
	stat = inst->EVSTAT;
	inst->EVSTAT = stat;
	stat2 = inst->EVSTAT2;

	/* SHI CS pin is asserted in EVSTAT2 */
	if (IS_BIT_SET(stat2, NPCX_EVSTAT2_CSNFE)) {
		/* Clear pending bit of CSNFE */
		inst->EVSTAT2 = BIT(NPCX_EVSTAT2_CSNFE);
		LOG_DBG("CSNFE-");

		/*
		 * BUSY bit is set when SHI_CS is asserted. If not, leave it for
		 * SHI_CS de-asserted event.
		 */
		if (!IS_BIT_SET(inst->SHICFG2, NPCX_SHICFG2_BUSY)) {
			LOG_DBG("CSNB-");
			return;
		}

		shi_npcx_handle_cs_assert(dev);
	}

	/*
	 * End of data for read/write transaction. i.e. SHI_CS is deasserted.
	 * Host completed or aborted transaction
	 *
	 * EOR has the limitation that it will not be set even if the SHI_CS is deasserted without
	 * SPI clocks. The new SHI module introduce the CSNRE bit which will be set when SHI_CS is
	 * deasserted regardless of SPI clocks.
	 */
	if (IS_BIT_SET(stat2, NPCX_EVSTAT2_CSNRE)) {
		/* Clear pending bit of CSNRE */
		inst->EVSTAT2 = BIT(NPCX_EVSTAT2_CSNRE);

		/*
		 * We're not in proper state.
		 * Mark not ready to abort next transaction
		 */
		LOG_DBG("CSH-");
		return shi_npcx_handle_cs_deassert(dev);
	}

	/*
	 * The number of bytes received reaches the size of
	 * protocol V3 header(=8) after CS asserted.
	 */
	if (IS_BIT_SET(stat2, NPCX_EVSTAT2_IBHF2)) {
		/* Clear IBHF2 */
		inst->EVSTAT2 = BIT(NPCX_EVSTAT2_IBHF2);
		LOG_DBG("HDR-");

		/* Disable second IBF interrupt and start to parse header */
		shi_npcx_sec_ibf_int_enable(inst, 0);
		shi_npcx_parse_header(dev);
	}

	/*
	 * Indicate input/output buffer pointer reaches the half buffer size.
	 * Transaction is processing.
	 */
	if (IS_BIT_SET(stat, NPCX_EVSTAT_IBHF)) {
		return shi_npcx_handle_input_buf_half_full(dev);
	}

	/*
	 * Indicate input/output buffer pointer reaches the full buffer size.
	 * Transaction is processing.
	 */
	if (IS_BIT_SET(stat, NPCX_EVSTAT_IBF)) {
		return shi_npcx_handle_input_buf_full(dev);
	}
}

static void shi_npcx_reset_prepare(const struct device *dev)
{
	struct shi_reg *const inst = HAL_INSTANCE(dev);
	struct shi_npcx_data *data = dev->data;
	uint32_t i;

	data->state = SHI_STATE_DISABLED;

	irq_disable(DT_INST_IRQN(0));

	/* Disable SHI unit to clear all status bits */
	inst->SHICFG1 &= ~BIT(NPCX_SHICFG1_EN);

	/* Initialize parameters of next transaction */
	data->rx_msg = data->in_msg;
	data->tx_msg = data->out_msg;
	data->rx_buf = inst->IBUF;
	data->tx_buf = inst->OBUF;
	if (data->rx_ctx) {
		data->rx_ctx->len = 0;
	}
	data->sz_sending = 0;
	data->sz_request = 0;
	data->sz_response = 0;

	if (IS_ENABLED(CONFIG_EC_HOST_CMD_BACKEND_SHI_NPCX_ENHANCED_BUF_MODE)) {
		inst->SBOBUF = EC_SHI_RX_READY;
		inst->SBOBUF = EC_SHI_RECEIVING;
		inst->EVENABLE |= IBF_IBHF_EN_MASK;
		inst->EVENABLE &= ~(BIT(NPCX_EVENABLE_OBEEN) | BIT(NPCX_EVENABLE_OBHEEN));
	} else {
		/*
		 * Fill output buffer to indicate we`re ready to receive next transaction.
		 */
		for (i = 1; i < SHI_OBUF_FULL_SIZE; i++) {
			inst->OBUF[i] = EC_SHI_RECEIVING;
		}
		inst->OBUF[0] = EC_SHI_RX_READY;
	}

	/* SHI/Host Write/input buffer wrap-around enable */
	inst->SHICFG1 = BIT(NPCX_SHICFG1_IWRAP) | BIT(NPCX_SHICFG1_WEN) | BIT(NPCX_SHICFG1_EN);

	data->state = SHI_STATE_READY_TO_RECV;
	data->last_error_state = SHI_STATE_NONE;

	shi_npcx_sec_ibf_int_enable(inst, 1);
	irq_enable(DT_INST_IRQN(0));

	shi_npcx_pm_policy_state_lock_put(data, SHI_NPCX_PM_POLICY_FLAG);

	LOG_DBG("RDY-");
}

static int shi_npcx_enable(const struct device *dev)
{
	const struct device *clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	const struct shi_npcx_config *const config = dev->config;
	int ret;

	ret = clock_control_on(clk_dev, (clock_control_subsys_t)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on SHI clock fail %d", ret);
		return ret;
	}

	shi_npcx_reset_prepare(dev);
	npcx_miwu_irq_disable(&config->shi_cs_wui);

	/* Configure pin control for SHI */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("shi_npcx pinctrl setup failed (%d)", ret);
		return ret;
	}

	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
	npcx_miwu_irq_enable(&config->shi_cs_wui);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static int shi_npcx_disable(const struct device *dev)
{
	const struct device *clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	const struct shi_npcx_config *const config = dev->config;
	struct shi_npcx_data *data = dev->data;
	int ret;

	data->state = SHI_STATE_DISABLED;

	irq_disable(DT_INST_IRQN(0));
	npcx_miwu_irq_disable(&config->shi_cs_wui);

	/* Configure pin control back to GPIO */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
	if (ret < 0) {
		LOG_ERR("KB Raw pinctrl setup failed (%d)", ret);
		return ret;
	}

	ret = clock_control_off(clk_dev, (clock_control_subsys_t)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn off SHI clock fail %d", ret);
		return ret;
	}

	/*
	 * Allow deep sleep again in case CS dropped before ec was
	 * informed in hook function and turn off SHI's interrupt in time.
	 */
	shi_npcx_pm_policy_state_lock_put(data, SHI_NPCX_PM_POLICY_FLAG);

	return 0;
}

static int shi_npcx_init_registers(const struct device *dev)
{
	int ret;
	const struct shi_npcx_config *const config = dev->config;
	struct shi_reg *const inst = HAL_INSTANCE(dev);
	const struct device *clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);

	/* Turn on shi device clock first */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on SHI clock fail %d", ret);
		return ret;
	}

	/*
	 * SHICFG1 (SHI Configuration 1) setting
	 * [7] - IWRAP	= 1: Wrap input buffer to the first address
	 * [6] - CPOL	= 0: Sampling on rising edge and output on falling edge
	 * [5] - DAS	= 0: return STATUS reg data after Status command
	 * [4] - AUTOBE	= 0: Automatically update the OBES bit in STATUS reg
	 * [3] - AUTIBF	= 0: Automatically update the IBFS bit in STATUS reg
	 * [2] - WEN    = 0: Enable host write to input buffer
	 * [1] - Reserved 0
	 * [0] - ENABLE	= 0: Disable SHI at the beginning
	 */
	inst->SHICFG1 = BIT(NPCX_SHICFG1_IWRAP);

	/*
	 * SHICFG2 (SHI Configuration 2) setting
	 * [7] - Reserved 0
	 * [6] - REEVEN = 0: Restart events are not used
	 * [5] - Reserved 0
	 * [4] - REEN   = 0: Restart transactions are not used
	 * [3] - SLWU   = 0: Seem-less wake-up is enabled by default
	 * [2] - ONESHOT= 0: WEN is cleared at the end of a write transaction
	 * [1] - BUSY   = 0: SHI bus is busy 0: idle.
	 * [0] - SIMUL	= 1: Turn on simultaneous Read/Write
	 */
	inst->SHICFG2 = BIT(NPCX_SHICFG2_SIMUL);

	/*
	 * EVENABLE (Event Enable) setting
	 * [7] - IBOREN = 0: Input buffer overrun interrupt enable
	 * [6] - STSREN = 0: status read interrupt disable
	 * [5] - EOWEN  = 0: End-of-Data for Write Transaction Interrupt Enable
	 * [4] - EOREN  = 1: End-of-Data for Read Transaction Interrupt Enable
	 * [3] - IBHFEN = 1: Input Buffer Half Full Interrupt Enable
	 * [2] - IBFEN  = 1: Input Buffer Full Interrupt Enable
	 * [1] - OBHEEN = 0: Output Buffer Half Empty Interrupt Enable
	 * [0] - OBEEN  = 0: Output Buffer Empty Interrupt Enable
	 */
	inst->EVENABLE = BIT(NPCX_EVENABLE_EOREN) | IBF_IBHF_EN_MASK;

	/*
	 * EVENABLE2 (Event Enable 2) setting
	 * [2] - CSNFEEN = 1: SHI_CS Falling Edge Interrupt Enable
	 * [1] - CSNREEN = 1: SHI_CS Rising Edge Interrupt Enable
	 * [0] - IBHF2EN = 0: Input Buffer Half Full 2 Interrupt Enable
	 */
	inst->EVENABLE2 = BIT(NPCX_EVENABLE2_CSNREEN) | BIT(NPCX_EVENABLE2_CSNFEEN);

	/* Clear SHI events status register */
	inst->EVSTAT = 0xff;

	if (IS_ENABLED(CONFIG_EC_HOST_CMD_BACKEND_SHI_NPCX_ENHANCED_BUF_MODE)) {
		inst->SHICFG6 |= BIT(NPCX_SHICFG6_EBUFMD);
	}

	npcx_miwu_interrupt_configure(&config->shi_cs_wui, NPCX_MIWU_MODE_EDGE, NPCX_MIWU_TRIG_LOW);

	/* SHI interrupt installation */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), shi_npcx_isr, DEVICE_DT_INST_GET(0),
		    0);

	shi_npcx_enable(dev);

	return ret;
}

static int shi_npcx_init(const struct device *dev)
{
	int ret;

	ret = shi_npcx_init_registers(dev);
	if (ret) {
		return ret;
	}
	pm_device_init_suspended(dev);

	return pm_device_runtime_enable(dev);
}

static int shi_npcx_backend_init(const struct ec_host_cmd_backend *backend,
				 struct ec_host_cmd_rx_ctx *rx_ctx, struct ec_host_cmd_tx_buf *tx)
{
	struct ec_host_cmd_shi_npcx_ctx *hc_shi = (struct ec_host_cmd_shi_npcx_ctx *)backend->ctx;
	struct shi_npcx_data *data;

	hc_shi->dev = DEVICE_DT_INST_GET(0);
	if (!device_is_ready(hc_shi->dev)) {
		return -ENODEV;
	}

	data = hc_shi->dev->data;
	data->rx_ctx = rx_ctx;
	data->tx = tx;

	rx_ctx->buf = data->in_msg;
	rx_ctx->len_max = CONFIG_EC_HOST_CMD_BACKEND_SHI_MAX_REQUEST;
	tx->buf = data->out_msg_padded + SHI_OUT_START_PAD;
	tx->len_max = CONFIG_EC_HOST_CMD_BACKEND_SHI_MAX_RESPONSE;

	return 0;
}

static int shi_npcx_backend_send(const struct ec_host_cmd_backend *backend)
{
	struct ec_host_cmd_shi_npcx_ctx *hc_shi = (struct ec_host_cmd_shi_npcx_ctx *)backend->ctx;
	struct shi_npcx_data *data = hc_shi->dev->data;
	uint8_t *out_buf = data->out_msg + EC_SHI_FRAME_START_LENGTH;

	if (!IS_ENABLED(CONFIG_EC_HOST_CMD_BACKEND_SHI_NPCX_ENHANCED_BUF_MODE)) {
		/*
		 * Disable interrupts. This routine is not called from interrupt context and buffer
		 * underrun will likely occur if it is preempted after writing its initial reply
		 * byte. Also, we must be sure our state doesn't unexpectedly change, in case we're
		 * expected to take RESP_NOT_RDY actions.
		 */
		__disable_irq();
	}

	if (data->state == SHI_STATE_PROCESSING) {
		/* Append our past-end byte, which we reserved space for. */
		((uint8_t *)out_buf)[data->tx->len] = EC_SHI_PAST_END;

		/* Computing sending bytes of response */
		data->sz_response = data->tx->len + EC_SHI_PROTO3_OVERHEAD;

		/* Start to fill output buffer with msg buffer */
		shi_npcx_write_first_pkg_outbuf(hc_shi->dev, data->sz_response);

		/* Transmit the reply */
		data->state = SHI_STATE_SENDING;
		if (IS_ENABLED(CONFIG_EC_HOST_CMD_BACKEND_SHI_NPCX_ENHANCED_BUF_MODE)) {
			struct shi_reg *const inst = HAL_INSTANCE(hc_shi->dev);

			/*
			 * Enable output buffer half/full empty interrupt and
			 * switch output mode from the repeated single byte mode
			 * to FIFO mode.
			 */
			inst->EVENABLE |= BIT(NPCX_EVENABLE_OBEEN) | BIT(NPCX_EVENABLE_OBHEEN);
			inst->SHICFG6 |= BIT(NPCX_SHICFG6_OBUF_SL);
		}
		LOG_DBG("SND-");
	} else if (data->state == SHI_STATE_CNL_RESP_NOT_RDY) {
		/*
		 * If we're not processing, then the AP has already terminated
		 * the transaction, and won't be listening for a response.
		 * Reset state machine for next transaction.
		 */
		shi_npcx_reset_prepare(hc_shi->dev);
		LOG_DBG("END\n");
	} else {
		LOG_ERR("Unexpected state %d in response handler", data->state);
	}

	if (!IS_ENABLED(CONFIG_EC_HOST_CMD_BACKEND_SHI_NPCX_ENHANCED_BUF_MODE)) {
		__enable_irq();
	}

	return 0;
}

static const struct ec_host_cmd_backend_api ec_host_cmd_api = {
	.init = shi_npcx_backend_init,
	.send = shi_npcx_backend_send,
};

#ifdef CONFIG_PM_DEVICE
static int shi_npcx_pm_cb(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		shi_npcx_disable(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		shi_npcx_enable(dev);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif

/* Assume only one peripheral */
PM_DEVICE_DT_INST_DEFINE(0, shi_npcx_pm_cb);

PINCTRL_DT_INST_DEFINE(0);
static const struct shi_npcx_config shi_cfg = {
	.base = DT_INST_REG_ADDR(0),
	.clk_cfg = NPCX_DT_CLK_CFG_ITEM(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.irq = DT_INST_IRQN(0),
	.shi_cs_wui = NPCX_DT_WUI_ITEM_BY_NAME(0, shi_cs_wui),
};

static struct shi_npcx_data shi_data = {
	.state = SHI_STATE_DISABLED,
	.last_error_state = SHI_STATE_NONE,
	.out_msg = shi_data.out_msg_padded + SHI_OUT_START_PAD - EC_SHI_FRAME_START_LENGTH,
};

DEVICE_DT_INST_DEFINE(0, shi_npcx_init, PM_DEVICE_DT_INST_GET(0), &shi_data, &shi_cfg, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &ec_host_cmd_api);

EC_HOST_CMD_SHI_NPCX_DEFINE(ec_host_cmd_shi_npcx);

struct ec_host_cmd_backend *ec_host_cmd_backend_get_shi_npcx(void)
{
	return &ec_host_cmd_shi_npcx;
}

#if DT_NODE_EXISTS(DT_CHOSEN(zephyr_host_cmd_shi_backend)) &&                                      \
	defined(CONFIG_EC_HOST_CMD_INITIALIZE_AT_BOOT)
static int host_cmd_init(void)
{
	ec_host_cmd_init(ec_host_cmd_backend_get_shi_npcx());
	return 0;
}
SYS_INIT(host_cmd_init, POST_KERNEL, CONFIG_EC_HOST_CMD_INIT_PRIORITY);
#endif
