/*
 * Copyright (c) 2022 Meta Platforms, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Native RTIO submit path for the Cadence I3C controller.
 *
 * Owns BOTH the i3c-RTIO and i2c-on-i3c-RTIO data paths. Each iodev_submit
 * entry pushes into its respective RTIO queue (i3c_rtio or i2c_rtio); the
 * controller-level state in cdns_i3c_rtio_state guarantees only one
 * transaction drives the bus at a time.
 *
 * The active transaction's "kind" is implicit in active_head->sqe.iodev->api:
 *   - &i3c_iodev_api -> i3c data sqe (RTIO_OP_RX/TX/TINY_TX) or CCC sqe
 *   - &i2c_iodev_api -> i2c-on-i3c sqe
 *
 * Scope: SDR controller mode plus HDR-DDR. Ops handled natively are
 * RTIO_OP_TX, RTIO_OP_RX, RTIO_OP_TINY_TX, and RTIO_OP_I3C_CCC. HDR-DDR
 * streams via TX_THR / RX_THR thresholds. HDR-TS, HDR-BT, TXRX,
 * I3C_CONFIGURE, and I3C_RECOVER fall back to -ENOTSUP from this path.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/rtio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "i3c_cdns.h"

LOG_MODULE_DECLARE(I3C_CADENCE, CONFIG_I3C_CADENCE_LOG_LEVEL);

static void cdns_i3c_rtio_complete_txn(const struct device *dev, int status);

static inline bool sqe_is_i3c(const struct rtio_iodev_sqe *iodev_sqe)
{
	return iodev_sqe != NULL && iodev_sqe->sqe.iodev != NULL &&
	       iodev_sqe->sqe.iodev->api == &i3c_iodev_api;
}

static inline bool sqe_is_i2c(const struct rtio_iodev_sqe *iodev_sqe)
{
	return iodev_sqe != NULL && iodev_sqe->sqe.iodev != NULL &&
	       iodev_sqe->sqe.iodev->api == &i2c_iodev_api;
}

/*
 * Resolve i3c_device_desc for an i3c data sqe. CCC sqes do not need a desc.
 */
static struct i3c_device_desc *cdns_i3c_rtio_resolve_i3c_desc(const struct device *dev,
							      struct rtio_iodev_sqe *iodev_sqe)
{
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_rtio_state *state = data->rtio;
	const struct i3c_iodev_data *iodev_data;

	if (!sqe_is_i3c(iodev_sqe)) {
		return NULL;
	}

	if (iodev_sqe->sqe.iodev == &state->i3c_ctx->iodev) {
		return state->i3c_ctx->i3c_desc;
	}

	if (iodev_sqe->sqe.iodev->data == NULL) {
		return NULL;
	}

	iodev_data = (const struct i3c_iodev_data *)iodev_sqe->sqe.iodev->data;
	if (iodev_data->bus != dev) {
		return NULL;
	}

	return i3c_device_find(dev, &iodev_data->dev_id);
}

static const struct i2c_dt_spec *cdns_i3c_rtio_resolve_i2c_spec(struct rtio_iodev_sqe *iodev_sqe)
{
	if (!sqe_is_i2c(iodev_sqe) || iodev_sqe->sqe.iodev->data == NULL) {
		return NULL;
	}

	return (const struct i2c_dt_spec *)iodev_sqe->sqe.iodev->data;
}

static bool cdns_i3c_rtio_sqe_tx_buf(const struct rtio_iodev_sqe *iodev_sqe,
				     const uint8_t **out_buf, uint32_t *out_len);

/*
 * Pull the next outbound source from the TX cursor, advancing past sqes
 * that have nothing more to send. For DDR cmds, "more to send" includes
 * the header phase (before any payload) and CRC phase (after all payload
 * bytes are consumed) — not just tx_off < len.
 *
 * Returns true when the cursor is positioned on a cmd with TX work
 * remaining; false when the entire transaction has been pushed.
 */
static bool cdns_i3c_rtio_advance_tx_cursor(struct cdns_i3c_rtio_state *state)
{
	while (state->tx_sqe != NULL) {
		struct cdns_i3c_rtio_cmd *cmd =
			(state->tx_cmd_idx < state->num_cmds) ? &state->cmds[state->tx_cmd_idx]
							      : NULL;
		bool more = false;

		if (cmd != NULL && cmd->is_ddr) {
			if (cmd->is_read) {
				/* DDR reads only push the header on the TX side. */
				more = (state->ddr_tx_phase == DDR_TX_HEADER);
			} else {
				more = (state->ddr_tx_phase != DDR_TX_DONE);
			}
		} else {
			uint32_t tx_len = 0;

			switch (state->tx_sqe->sqe.op) {
			case RTIO_OP_TX:
				tx_len = state->tx_sqe->sqe.tx.buf_len;
				break;
			case RTIO_OP_TINY_TX:
				tx_len = state->tx_sqe->sqe.tiny_tx.buf_len;
				break;
			default:
				break;
			}
			more = (tx_len > state->tx_off);
		}

		if (more) {
			return true;
		}

		state->tx_sqe = rtio_txn_next(state->tx_sqe);
		state->tx_off = 0;
		state->tx_cmd_idx++;
		state->ddr_tx_phase = DDR_TX_HEADER;
	}

	return false;
}

static bool cdns_i3c_rtio_advance_rx_cursor(struct cdns_i3c_rtio_state *state)
{
	while (state->rx_sqe != NULL) {
		struct cdns_i3c_rtio_cmd *cmd =
			(state->rx_cmd_idx < state->num_cmds) ? &state->cmds[state->rx_cmd_idx]
							      : NULL;

		if (cmd != NULL && cmd->is_ddr && cmd->is_read) {
			/* DDR reads always have a CRC token after the data
			 * words; "more" includes the still-pending CRC. The
			 * decoder loop checks rx_off vs len to know whether the
			 * next FIFO word is data or CRC.
			 */
			if (state->rx_sqe->sqe.op == RTIO_OP_RX) {
				return true;
			}
		} else if (state->rx_sqe->sqe.op == RTIO_OP_RX &&
			   state->rx_sqe->sqe.rx.buf_len > state->rx_off) {
			return true;
		}

		state->rx_sqe = rtio_txn_next(state->rx_sqe);
		state->rx_off = 0;
		state->rx_cmd_idx++;
	}

	return false;
}

static void cdns_i3c_rtio_drain_tx(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_rtio_state *state = data->rtio;

	while (cdns_i3c_rtio_advance_tx_cursor(state)) {
		struct cdns_i3c_rtio_cmd *cmd = &state->cmds[state->tx_cmd_idx];

		if (cdns_i3c_tx_fifo_full(config)) {
			return;
		}

		if (cmd->is_ddr) {
			const uint8_t *src;
			uint32_t src_len;
			uint16_t pl;
			uint32_t preamble;
			uint32_t word;

			switch (state->ddr_tx_phase) {
			case DDR_TX_HEADER:
				sys_write32(cmd->ddr_header, config->base + TX_FIFO);
				/* Reads have no data/CRC on TX side. */
				state->ddr_tx_phase = cmd->is_read ? DDR_TX_DONE : DDR_TX_DATA;
				break;
			case DDR_TX_DATA:
				if (!cdns_i3c_rtio_sqe_tx_buf(state->tx_sqe, &src, &src_len)) {
					/* Shouldn't happen — walk validated this. */
					state->ddr_tx_phase = DDR_TX_DONE;
					break;
				}
				pl = sys_get_be16(&src[state->tx_off]);
				preamble = (state->tx_off == 0) ? DDR_PREAMBLE_DATA_ABORT
								: DDR_PREAMBLE_DATA_ABORT_ALT;
				word = preamble | prepare_ddr_word(pl);
				sys_write32(word, config->base + TX_FIFO);
				state->tx_off += 2;
				if (state->tx_off >= cmd->len) {
					state->ddr_tx_phase = DDR_TX_CRC;
				}
				break;
			case DDR_TX_CRC:
				sys_write32(cmd->ddr_crc_word, config->base + TX_FIFO);
				state->ddr_tx_phase = DDR_TX_DONE;
				break;
			case DDR_TX_DONE:
			default:
				/* Shouldn't reach here — advance_tx_cursor would
				 * have moved on. Defensive: fall through to next
				 * loop iteration which will advance.
				 */
				break;
			}
		} else {
			const uint8_t *src;
			uint32_t total;

			if (state->tx_sqe->sqe.op == RTIO_OP_TX) {
				src = state->tx_sqe->sqe.tx.buf;
				total = state->tx_sqe->sqe.tx.buf_len;
			} else {
				src = state->tx_sqe->sqe.tiny_tx.buf;
				total = state->tx_sqe->sqe.tiny_tx.buf_len;
			}

			uint32_t remaining = total - state->tx_off;
			uint32_t chunk = MIN(remaining, sizeof(uint32_t));

			cdns_i3c_write_tx_fifo(config, &src[state->tx_off], chunk);
			state->tx_off += chunk;
		}
	}
}

static void cdns_i3c_rtio_drain_rx(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_rtio_state *state = data->rtio;

	while (cdns_i3c_rtio_advance_rx_cursor(state)) {
		struct cdns_i3c_rtio_cmd *cmd = &state->cmds[state->rx_cmd_idx];
		uint8_t *dst = state->rx_sqe->sqe.rx.buf;

		if (cdns_i3c_rx_fifo_empty(config)) {
			return;
		}

		if (cmd->is_ddr) {
			uint32_t val = sys_read32(config->base + RX_FIFO);
			uint32_t preamble = val & DDR_PREAMBLE_MASK;

			if (preamble == DDR_PREAMBLE_DATA_ABORT ||
			    preamble == DDR_PREAMBLE_DATA_ABORT_ALT) {
				uint16_t pl = (uint16_t)DDR_DATA(val);

				if (state->rx_off + 2 <= cmd->len) {
					sys_put_be16(pl, &dst[state->rx_off]);
					state->rx_off += 2;
				}
				cmd->ddr_crc = i3c_cdns_crc5(cmd->ddr_crc, pl);
			} else if (preamble == DDR_PREAMBLE_CMD_CRC &&
				   (val & DDR_CRC_TOKEN_MASK) == DDR_CRC_TOKEN) {
				uint8_t crc = (uint8_t)DDR_CRC(val);

				if (cmd->ddr_crc != crc) {
					LOG_ERR("DDR RX crc error");
					if (state->xfer_status == 0) {
						state->xfer_status = -EIO;
					}
				}
				/* Force advance to next sqe — CRC token marks
				 * end of this cmd's RX stream, even if some
				 * data words were dropped above (rx_off < len).
				 */
				state->rx_off = cmd->len;
			}
			/* Other preambles silently ignored (legacy parity). */
		} else {
			uint32_t total = state->rx_sqe->sqe.rx.buf_len;
			uint32_t remaining = total - state->rx_off;
			uint32_t chunk = MIN(remaining, sizeof(uint32_t));

			if (cdns_i3c_read_rx_fifo(config, &dst[state->rx_off], chunk) != 0) {
				return;
			}
			state->rx_off += chunk;
		}
	}
}

static int cdns_i3c_rtio_translate_cmdr_error(uint32_t err)
{
	switch (err) {
	case CMDR_NO_ERROR:
		return 0;
	case CMDR_DDR_RX_FIFO_OVF:
	case CMDR_DDR_TX_FIFO_UNF:
		return -ENOSPC;
	case CMDR_NACK_RESP:
	case CMDR_INVALID_DA:
		return -ENXIO;
	default:
		return -EIO;
	}
}

static enum i3c_sdr_controller_error_types cdns_i3c_rtio_cmdr_sdr_error(uint32_t err)
{
	switch (err) {
	case CMDR_NO_ERROR:
	case CMDR_MST_ABORT:
		return I3C_ERROR_CE_NONE;
	case CMDR_M0_ERROR:
		return I3C_ERROR_CE0;
	case CMDR_M1_ERROR:
		return I3C_ERROR_CE1;
	case CMDR_M2_ERROR:
		return I3C_ERROR_CE2;
	default:
		return I3C_ERROR_CE_UNKNOWN;
	}
}

/*
 * Walk an i3c data transaction starting at txn_head. Validates ops, fills
 * state->cmds[], returns the number of commands or a negative errno.
 */
static int cdns_i3c_rtio_walk_i3c_txn(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_rtio_state *state = data->rtio;
	struct rtio_iodev_sqe *cur = state->active_head;
	bool ddr_supported =
		(data->common.ctrl_config.supported_hdr & I3C_MSG_HDR_DDR) != 0;
	uint8_t n = 0;

	while (cur != NULL) {
		uint32_t buf_len;
		uint16_t plen;
		bool is_read;
		bool is_ddr = false;

		if (n >= I3C_MAX_MSGS || n >= data->hw_cfg.cmd_mem_depth ||
		    n >= data->hw_cfg.cmdr_mem_depth) {
			return -ENOMEM;
		}

		switch (cur->sqe.op) {
		case RTIO_OP_RX:
			buf_len = cur->sqe.rx.buf_len;
			is_read = true;
			break;
		case RTIO_OP_TX:
			buf_len = cur->sqe.tx.buf_len;
			is_read = false;
			break;
		case RTIO_OP_TINY_TX:
			buf_len = cur->sqe.tiny_tx.buf_len;
			is_read = false;
			break;
		default:
			return -ENOTSUP;
		}

		if (cur->sqe.iodev_flags & RTIO_IODEV_I3C_HDR) {
			uint32_t mode = RTIO_IODEV_I3C_HDR_MODE_GET(cur->sqe.iodev_flags);

			if (mode != I3C_MSG_HDR_DDR) {
				/* Only HDR-DDR is wired up in v1; other HDR modes
				 * (TS, BT) fall through to the fallback.
				 */
				return -ENOTSUP;
			}
			if (!ddr_supported) {
				return -ENOTSUP;
			}
			/* DDR sends data 16b at a time; payload must be even. */
			if ((buf_len % 2) != 0) {
				return -EINVAL;
			}
			/*
			 * DDR PL_LEN counts 16-bit words: (len/2) data words + 1 cmd
			 * header word + 1 CRC word. Must fit in the 12-bit PL_LEN field.
			 */
			if (((buf_len / 2) + 2) > CMD0_FIFO_PL_LEN_MAX) {
				return -EMSGSIZE;
			}
			is_ddr = true;
		} else {
			/* SDR PL_LEN field is 12 bits → byte count must fit. */
			if (buf_len > CMD0_FIFO_PL_LEN_MAX) {
				return -EMSGSIZE;
			}
		}

		plen = (uint16_t)buf_len;

		state->cmds[n].sqe = cur;
		state->cmds[n].len = plen;
		state->cmds[n].is_read = is_read;
		state->cmds[n].is_ccc_target = false;
		state->cmds[n].is_ddr = is_ddr;

		n++;
		cur = rtio_txn_next(cur);
	}

	return n;
}

/*
 * Walk an i2c-on-i3c data transaction. Same shape as i3c walk but no HDR
 * check and addressing comes from i2c_dt_spec; we only validate ops here.
 */
static int cdns_i3c_rtio_walk_i2c_txn(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_rtio_state *state = data->rtio;
	struct rtio_iodev_sqe *cur = state->active_head;
	uint8_t n = 0;

	while (cur != NULL) {
		uint32_t buf_len;
		uint16_t plen;
		bool is_read;

		if (n >= I3C_MAX_MSGS || n >= data->hw_cfg.cmd_mem_depth ||
		    n >= data->hw_cfg.cmdr_mem_depth) {
			return -ENOMEM;
		}

		switch (cur->sqe.op) {
		case RTIO_OP_RX:
			buf_len = cur->sqe.rx.buf_len;
			is_read = true;
			break;
		case RTIO_OP_TX:
			buf_len = cur->sqe.tx.buf_len;
			is_read = false;
			break;
		case RTIO_OP_TINY_TX:
			buf_len = cur->sqe.tiny_tx.buf_len;
			is_read = false;
			break;
		default:
			return -ENOTSUP;
		}

		/* PL_LEN field is 12 bits → byte count must fit. */
		if (buf_len > CMD0_FIFO_PL_LEN_MAX) {
			return -EMSGSIZE;
		}

		plen = (uint16_t)buf_len;

		state->cmds[n].sqe = cur;
		state->cmds[n].len = plen;
		state->cmds[n].is_read = is_read;
		state->cmds[n].is_ccc_target = false;
		state->cmds[n].is_ddr = false;

		n++;
		cur = rtio_txn_next(cur);
	}

	return n;
}

static int cdns_i3c_rtio_build_ccc(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_rtio_state *state = data->rtio;
	struct i3c_ccc_payload *payload = iodev_sqe->sqe.ccc_payload;
	bool broadcast;

	if (payload == NULL) {
		return -EINVAL;
	}

	broadcast = i3c_ccc_is_payload_broadcast(payload);

	if (broadcast) {
		uint32_t cmd0 = CMD0_FIFO_IS_CCC | CMD0_FIFO_BCH;
		uint32_t cmd1 = CMD1_FIFO_CCC(payload->ccc.id) | CMD1_FIFO_CMDID(0);

		if (payload->ccc.data_len > CMD0_FIFO_PL_LEN_MAX ||
		    payload->ccc.data_len > data->hw_cfg.tx_mem_depth) {
			return -EMSGSIZE;
		}

		state->cmds[0].sqe = iodev_sqe;
		state->cmds[0].len = payload->ccc.data_len;
		state->cmds[0].is_read = false;
		state->cmds[0].is_ccc_target = false;
		state->cmds[0].is_ddr = false;

		if (payload->ccc.data_len > 0) {
			cmd0 |= CMD0_FIFO_PL_LEN(payload->ccc.data_len);
			cdns_i3c_write_tx_fifo(config, payload->ccc.data, payload->ccc.data_len);
		}

		sys_write32(cmd1, config->base + CMD1_FIFO);
		sys_write32(cmd0, config->base + CMD0_FIFO);

		return 1;
	}

	if (payload->targets.num_targets == 0 || payload->targets.num_targets > I3C_MAX_MSGS) {
		return -EINVAL;
	}

	if (payload->ccc.data_len == 1) {
		if (REV_ID_REV(data->hw_cfg.rev_id) < REV_ID_VERSION(1, 7)) {
			LOG_ERR("%s: Defining Byte with Direct CCC not supported with rev %lup%lu",
				dev->name, REV_ID_REV_MAJOR(data->hw_cfg.rev_id),
				REV_ID_REV_MINOR(data->hw_cfg.rev_id));
			return -ENOTSUP;
		}
	} else if (payload->ccc.data_len > 1) {
		LOG_ERR("%s: Defining Byte length greater than 1", dev->name);
		return -EINVAL;
	}

	/*
	 * Pre-flight: sum direct-CCC per-target writes/reads and reject any that
	 * would overrun TX/RX FIFOs or exceed the PL_LEN field max. Native RTIO
	 * CCC writes/reads aren't streamed via *_THR IRQs (legacy parity), so
	 * the controller must hold the full payload up-front.
	 */
	{
		uint32_t txsum = 0;
		uint32_t rxsum = 0;

		for (uint8_t i = 0; i < payload->targets.num_targets; i++) {
			size_t plen = payload->targets.payloads[i].data_len;

			if (plen > CMD0_FIFO_PL_LEN_MAX) {
				return -EMSGSIZE;
			}
			if (payload->targets.payloads[i].rnw) {
				rxsum += plen;
			} else {
				txsum += plen;
			}
		}
		if (txsum > data->hw_cfg.tx_mem_depth || rxsum > data->hw_cfg.rx_mem_depth) {
			return -EMSGSIZE;
		}
	}

	for (uint8_t i = 0; i < payload->targets.num_targets; i++) {
		uint32_t cmd0 = CMD0_FIFO_IS_CCC;
		uint32_t cmd1 = CMD1_FIFO_CCC(payload->ccc.id) | CMD1_FIFO_CMDID(i);
		uint8_t addr = payload->targets.payloads[i].addr;
		size_t plen = payload->targets.payloads[i].data_len;

		if (i == 0) {
			cmd0 |= CMD0_FIFO_BCH;
			if (payload->ccc.data_len == 1 &&
			    REV_ID_REV(data->hw_cfg.rev_id) >= REV_ID_VERSION(1, 7)) {
				cmd0 |= CMD0_FIFO_IS_DB;
				cmd1 |= CMD1_FIFO_DB(payload->ccc.data[0]);
			}
		}
		if (i < (payload->targets.num_targets - 1)) {
			cmd0 |= CMD0_FIFO_RSBC;
		}

		cmd0 |= CMD0_FIFO_DEV_ADDR(addr) | CMD0_FIFO_PL_LEN(plen);

		state->cmds[i].sqe = iodev_sqe;
		state->cmds[i].len = plen;
		state->cmds[i].is_read = payload->targets.payloads[i].rnw;
		state->cmds[i].is_ccc_target = true;
		state->cmds[i].is_ddr = false;
		state->cmds[i].ccc_target_idx = i;

		if (payload->targets.payloads[i].rnw) {
			cmd0 |= CMD0_FIFO_RNW;
		} else if (plen > 0) {
			cdns_i3c_write_tx_fifo(config, payload->targets.payloads[i].data, plen);
		}

		sys_write32(cmd1, config->base + CMD1_FIFO);
		sys_write32(cmd0, config->base + CMD0_FIFO);
	}

	return (int)payload->targets.num_targets;
}

/*
 * Resolve TX source bytes for a sqe (RTIO_OP_TX or RTIO_OP_TINY_TX).
 * Sets *out_buf and *out_len. Returns false for non-TX-bearing ops.
 */
static bool cdns_i3c_rtio_sqe_tx_buf(const struct rtio_iodev_sqe *iodev_sqe,
				     const uint8_t **out_buf, uint32_t *out_len)
{
	switch (iodev_sqe->sqe.op) {
	case RTIO_OP_TX:
		*out_buf = iodev_sqe->sqe.tx.buf;
		*out_len = iodev_sqe->sqe.tx.buf_len;
		return true;
	case RTIO_OP_TINY_TX:
		*out_buf = iodev_sqe->sqe.tiny_tx.buf;
		*out_len = iodev_sqe->sqe.tiny_tx.buf_len;
		return true;
	default:
		return false;
	}
}

/*
 * Pre-build the DDR header + CRC for a DDR cmd. For writes, the entire
 * source buffer is folded into the running CRC so the final CRC token is
 * known before streaming begins. For reads, only the header bits seed the
 * CRC; subsequent words come in via RX and the decoder continues folding.
 */
static int cdns_i3c_rtio_ddr_prep(struct cdns_i3c_rtio_cmd *cmd, uint16_t dyn_addr)
{
	uint8_t hdr_cmd_code = RTIO_IODEV_I3C_HDR_CMD_CODE_GET(cmd->sqe->sqe.iodev_flags);
	uint8_t crc5 = 0x1F;
	uint16_t header;

	if (cmd->is_read) {
		header = HDR_CMD_RD | HDR_CMD_CODE(hdr_cmd_code) | (dyn_addr << 1);
		header = prepare_ddr_cmd_parity_adjustment_bit(header);
		cmd->ddr_header = DDR_PREAMBLE_CMD_CRC | prepare_ddr_word(header);
		crc5 = i3c_cdns_crc5(crc5, header);
		cmd->ddr_crc = crc5;
	} else {
		const uint8_t *src;
		uint32_t src_len;

		if (!cdns_i3c_rtio_sqe_tx_buf(cmd->sqe, &src, &src_len)) {
			return -ENOTSUP;
		}
		if (src_len < cmd->len) {
			return -EINVAL;
		}

		header = HDR_CMD_CODE(hdr_cmd_code) | (dyn_addr << 1);
		cmd->ddr_header = DDR_PREAMBLE_CMD_CRC | prepare_ddr_word(header);

		crc5 = i3c_cdns_crc5(crc5, header);
		for (uint32_t j = 0; j < cmd->len; j += 2) {
			crc5 = i3c_cdns_crc5(crc5, sys_get_be16(&src[j]));
		}
		cmd->ddr_crc_word = DDR_PREAMBLE_CMD_CRC | DDR_CRC_TOKEN |
				    ((uint32_t)crc5 << 9) | DDR_CRC_WR_SETUP;
	}

	return 0;
}

/*
 * Push CMD0/CMD1 pairs for a previously-walked data transaction. For SDR
 * cmds: a single (CMD1, CMD0) pair. For DDR cmds: a CCC ENTHDR0 frame
 * followed by a CMD0_FIFO_IS_DDR data frame (matches legacy).
 *
 * target_addr supplies dynamic_addr (i3c) or i2c_dt_spec.addr (i2c).
 * is_i3c controls BCH/NBCH semantics. is_10b applies only to i2c.
 */
static void cdns_i3c_rtio_push_data_cmds(const struct device *dev, uint8_t n,
					 uint16_t target_addr, bool is_i3c, bool is_10b)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_rtio_state *state = data->rtio;

	for (uint8_t i = 0; i < n; i++) {
		struct cdns_i3c_rtio_cmd *cmd = &state->cmds[i];
		struct rtio_iodev_sqe *s = cmd->sqe;
		uint32_t iodev_flags = s->sqe.iodev_flags;
		bool is_last = (i == n - 1);
		bool stop;

		if (is_i3c) {
			stop = !!(iodev_flags & RTIO_IODEV_I3C_STOP);
		} else {
			stop = !!(iodev_flags & RTIO_IODEV_I2C_STOP);
		}

		if (cmd->is_ddr) {
			/* CCC frame: ENTHDR0 announces the HDR-DDR transfer. */
			uint32_t ccc_cmd0 = CMD0_FIFO_IS_CCC;
			uint32_t ccc_cmd1 = I3C_CCC_ENTHDR0 | CMD1_FIFO_CMDID(i);

			sys_write32(ccc_cmd1, config->base + CMD1_FIFO);
			sys_write32(ccc_cmd0, config->base + CMD0_FIFO);

			/* IS_DDR data frame. CMDID intentionally omitted (the
			 * paired CCC above carries the response ID). PL_LEN is
			 * in DDR words: header + data + crc = (pl/2) + 2 for
			 * writes; reads use 1 (legacy quirk).
			 */
			uint32_t ddr_cmd0 = CMD0_FIFO_IS_DDR;

			if (cmd->is_read) {
				ddr_cmd0 |= CMD0_FIFO_PL_LEN(1);
			} else {
				ddr_cmd0 |= CMD0_FIFO_PL_LEN((cmd->len / 2) + 2);
			}

			sys_write32(0x00, config->base + CMD1_FIFO);
			sys_write32(ddr_cmd0, config->base + CMD0_FIFO);
		} else {
			uint32_t cmd0 = CMD0_FIFO_PRIV_XMIT_MODE(XMIT_BURST_WITHOUT_SUBADDR);
			uint32_t cmd1 = CMD1_FIFO_CMDID(i);

			cmd0 |= CMD0_FIFO_DEV_ADDR(target_addr);
			cmd0 |= CMD0_FIFO_PL_LEN(cmd->len);
			if (cmd->is_read) {
				cmd0 |= CMD0_FIFO_RNW;
			}
			if (is_10b) {
				cmd0 |= CMD0_FIFO_IS_10B;
			}

			if (is_i3c && i == 0 && !(iodev_flags & RTIO_IODEV_I3C_NBCH)) {
				cmd0 |= CMD0_FIFO_BCH;
			}

			if (!is_last && !stop) {
				cmd0 |= CMD0_FIFO_RSBC;
			}

			sys_write32(cmd1, config->base + CMD1_FIFO);
			sys_write32(cmd0, config->base + CMD0_FIFO);
		}
	}
}

/*
 * Program the controller for the transaction at active_head and kick the
 * bus. Caller has already set state->active_head and ensured no other
 * transaction is in flight.
 */
static void cdns_i3c_rtio_start(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_rtio_state *state = data->rtio;
	struct rtio_iodev_sqe *head = state->active_head;
	int n;

	if (head == NULL) {
		return;
	}

	if (!(sys_read32(config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE)) {
		cdns_i3c_rtio_complete_txn(dev, -EACCES);
		return;
	}

	if (cdns_i3c_wait_for_idle(dev) != 0) {
		cdns_i3c_rtio_complete_txn(dev, -EAGAIN);
		return;
	}

	while (!cdns_i3c_rx_fifo_empty(config)) {
		(void)sys_read32(config->base + RX_FIFO);
	}
	while (!cdns_i3c_cmd_rsp_fifo_empty(config)) {
		(void)sys_read32(config->base + CMDR);
	}

	state->tx_sqe = head;
	state->tx_off = 0;
	state->rx_sqe = head;
	state->rx_off = 0;
	state->tx_cmd_idx = 0;
	state->rx_cmd_idx = 0;
	state->ddr_tx_phase = DDR_TX_HEADER;
	state->xfer_status = 0;

	if (head->sqe.op == RTIO_OP_I3C_CCC) {
		n = cdns_i3c_rtio_build_ccc(dev, head);
	} else if (sqe_is_i3c(head)) {
		struct i3c_device_desc *desc = cdns_i3c_rtio_resolve_i3c_desc(dev, head);

		if (desc == NULL) {
			cdns_i3c_rtio_complete_txn(dev, -ENODEV);
			return;
		}
		n = cdns_i3c_rtio_walk_i3c_txn(dev);
		if (n < 0) {
			cdns_i3c_rtio_complete_txn(dev, n);
			return;
		}
		/* Pre-build header/CRC for any DDR cmds before pushing CMDs and
		 * starting the bus.
		 */
		for (uint8_t i = 0; i < (uint8_t)n; i++) {
			if (state->cmds[i].is_ddr) {
				int rc = cdns_i3c_rtio_ddr_prep(&state->cmds[i],
								desc->dynamic_addr);

				if (rc < 0) {
					cdns_i3c_rtio_complete_txn(dev, rc);
					return;
				}
			}
		}
		cdns_i3c_rtio_push_data_cmds(dev, (uint8_t)n, desc->dynamic_addr, true, false);
	} else if (sqe_is_i2c(head)) {
		const struct i2c_dt_spec *spec = cdns_i3c_rtio_resolve_i2c_spec(head);
		bool is_10b;

		if (spec == NULL) {
			cdns_i3c_rtio_complete_txn(dev, -ENODEV);
			return;
		}
		n = cdns_i3c_rtio_walk_i2c_txn(dev);
		if (n < 0) {
			cdns_i3c_rtio_complete_txn(dev, n);
			return;
		}
		is_10b = (head->sqe.iodev_flags & RTIO_IODEV_I2C_10_BITS) != 0;
		cdns_i3c_rtio_push_data_cmds(dev, (uint8_t)n, spec->addr, false, is_10b);
	} else {
		cdns_i3c_rtio_complete_txn(dev, -ENOTSUP);
		return;
	}

	if (n <= 0) {
		cdns_i3c_rtio_complete_txn(dev, (n == 0) ? -EINVAL : n);
		return;
	}

	state->num_cmds = (uint8_t)n;

	if (head->sqe.op != RTIO_OP_I3C_CCC) {
		cdns_i3c_rtio_drain_tx(dev);
	}

	sys_write32(MST_INT_CMDD_EMP | MST_INT_TX_THR | MST_INT_RX_THR,
		    config->base + MST_IER);
	sys_write32(CTRL_MCS | sys_read32(config->base + CTRL), config->base + CTRL);
}

/*
 * Pop the next queued transaction from the unified mpsc queue. Returns a
 * head sqe (and atomically claims the bus by setting state->active_head)
 * or NULL if the queue is empty.
 *
 * Time-ordered across i3c and i2c sqes — they all flow into the same queue
 * from submit, so popping FIFO preserves submit-time ordering.
 *
 * Called from ISR or from submit thread; uses state->slock for the
 * active_head transition.
 */
static struct rtio_iodev_sqe *cdns_i3c_rtio_pop_next(struct cdns_i3c_rtio_state *state)
{
	struct mpsc_node *node;
	struct rtio_iodev_sqe *head;
	k_spinlock_key_t key = k_spin_lock(&state->slock);

	if (state->active_head != NULL) {
		k_spin_unlock(&state->slock, key);
		return NULL;
	}

	node = mpsc_pop(&state->unified_q);
	if (node == NULL) {
		k_spin_unlock(&state->slock, key);
		return NULL;
	}

	head = CONTAINER_OF(node, struct rtio_iodev_sqe, q);
	state->active_head = head;
	k_spin_unlock(&state->slock, key);
	return head;
}

/*
 * Complete the active transaction. status applies to the whole batch
 * (sticky error from CMDR drain). We signal completion on the head sqe
 * directly via the RTIO executor — it walks the transaction chain (via
 * RTIO_SQE_TRANSACTION flag) and posts a CQE per sqe. No per-ctx queue
 * bookkeeping needed because we don't use the i*c_rtio ctx queues.
 */
static void cdns_i3c_rtio_complete_txn(const struct device *dev, int status)
{
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_rtio_state *state = data->rtio;
	struct rtio_iodev_sqe *head = state->active_head;

	/* Release bus ownership before invoking completion (which can recurse
	 * back into start via the next-txn pop).
	 */
	{
		k_spinlock_key_t key = k_spin_lock(&state->slock);

		state->active_head = NULL;
		k_spin_unlock(&state->slock, key);
	}

	if (head == NULL) {
		return;
	}

	if (status < 0) {
		rtio_iodev_sqe_err(head, status);
	} else {
		rtio_iodev_sqe_ok(head, status);
	}

	if (cdns_i3c_rtio_pop_next(state) != NULL) {
		cdns_i3c_rtio_start(dev);
	}
}

void cdns_i3c_iodev_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_rtio_state *state = data->rtio;

	mpsc_push(&state->unified_q, &iodev_sqe->q);

	if (cdns_i3c_rtio_pop_next(state) != NULL) {
		cdns_i3c_rtio_start(dev);
	}
}

void cdns_i3c_i2c_iodev_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_rtio_state *state = data->rtio;

	mpsc_push(&state->unified_q, &iodev_sqe->q);

	if (cdns_i3c_rtio_pop_next(state) != NULL) {
		cdns_i3c_rtio_start(dev);
	}
}

int cdns_i3c_rtio_init(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_rtio_state *state = data->rtio;

	if (state == NULL || state->i3c_ctx == NULL || state->i2c_ctx == NULL) {
		return -EINVAL;
	}

	mpsc_init(&state->unified_q);

	i3c_rtio_init(state->i3c_ctx, dev);
	i2c_rtio_init(state->i2c_ctx, dev);

	return 0;
}

void cdns_i3c_rtio_irq_tx_thr(const struct device *dev)
{
	cdns_i3c_rtio_drain_tx(dev);
}

void cdns_i3c_rtio_irq_rx_thr(const struct device *dev)
{
	cdns_i3c_rtio_drain_rx(dev);
}

void cdns_i3c_rtio_irq_cmdd_emp(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_rtio_state *state = data->rtio;
	int sticky = 0;
	bool was_full;

	sys_write32(MST_INT_CMDD_EMP | MST_INT_TX_THR | MST_INT_RX_THR,
		    config->base + MST_IDR);

	/* Snapshot before draining — used to distinguish real RX overflow
	 * from a benign controller abort (no EoD from target).
	 */
	was_full = cdns_i3c_rx_fifo_full(config);

	for (uint32_t status0 = sys_read32(config->base + MST_STATUS0);
	     !(status0 & MST_STATUS0_CMDR_EMP);
	     status0 = sys_read32(config->base + MST_STATUS0)) {
		uint32_t cmdr = sys_read32(config->base + CMDR);
		uint8_t id = CMDR_CMDID(cmdr);
		uint32_t err = CMDR_ERROR(cmdr);
		uint32_t bytes = CMDR_XFER_BYTES(cmdr);
		int rc;

		if (id == CMDR_CMDID_HJACK_DISEC || id == CMDR_CMDID_HJACK_ENTDAA ||
		    id >= state->num_cmds) {
			continue;
		}

		/* For CCC sqes, plumb num_xfer + RX bytes back into the payload. */
		if (state->cmds[id].is_ccc_target) {
			struct rtio_iodev_sqe *ccc_sqe = state->cmds[id].sqe;
			struct i3c_ccc_payload *p = ccc_sqe->sqe.ccc_payload;
			uint8_t t = state->cmds[id].ccc_target_idx;
			uint16_t got = MIN(bytes, state->cmds[id].len);

			p->targets.payloads[t].num_xfer = got;
			p->targets.payloads[t].err = cdns_i3c_rtio_cmdr_sdr_error(err);
			if (state->cmds[id].is_read && got > 0) {
				(void)cdns_i3c_read_rx_fifo(config,
							    p->targets.payloads[t].data, got);
			}
		} else if (sqe_is_i3c(state->cmds[id].sqe) &&
			   state->cmds[id].sqe->sqe.op == RTIO_OP_I3C_CCC) {
			/* Broadcast CCC: single command, num_xfer in ccc.num_xfer. */
			struct i3c_ccc_payload *p = state->cmds[id].sqe->sqe.ccc_payload;

			p->ccc.num_xfer = MIN(bytes, state->cmds[id].len);
			p->ccc.err = cdns_i3c_rtio_cmdr_sdr_error(err);
		} else if (state->cmds[id].sqe->sqe.iodev == &state->i3c_ctx->iodev &&
			   state->cmds[id].sqe->sqe.userdata != NULL) {
			struct i3c_msg *msg = state->cmds[id].sqe->sqe.userdata;

			msg->num_xfer = MIN(bytes, state->cmds[id].len);
			msg->err = cdns_i3c_rtio_cmdr_sdr_error(err);
		}

		rc = cdns_i3c_rtio_translate_cmdr_error(err);

		/*
		 * M0 error on variable-length CCCs (GETMXDS, GETCAPS) is
		 * expected -- the IP reports M0 when fewer bytes are returned
		 * than PL_LEN requested, which is normal for these CCCs.
		 * Suppress the error if the byte count is within spec.
		 */
		if (rc != 0 && err == CMDR_M0_ERROR && state->cmds[id].is_ccc_target) {
			struct rtio_iodev_sqe *ccc_sqe = state->cmds[id].sqe;

			if (ccc_sqe->sqe.op == RTIO_OP_I3C_CCC) {
				struct i3c_ccc_payload *p = ccc_sqe->sqe.ccc_payload;
				uint8_t ccc = p->ccc.id;

				if (ccc == I3C_CCC_GETMXDS) {
					if (bytes == SIZEOF_FIELD(union i3c_ccc_getmxds, fmt1) ||
					    bytes == SIZEOF_FIELD(union i3c_ccc_getmxds, fmt2)) {
						rc = 0;
					}
				} else if (ccc == I3C_CCC_GETCAPS) {
					if (bytes <= sizeof(union i3c_ccc_getcaps)) {
						rc = 0;
					}
				}
			}
		}

		/*
		 * Controller abort is forced when the RX FIFO fills up.
		 * If the FIFO was full and the requested length exceeds
		 * what was transferred, it is a real overflow. Otherwise
		 * the abort was caused by the read length being met while
		 * the target did not give an End of Data -- not an error.
		 */
		if (rc != 0 && err == CMDR_MST_ABORT) {
			if (was_full && state->cmds[id].len > bytes) {
				rc = -ENOSPC;
			} else {
				rc = 0;
			}
		}

		if (rc != 0 && sticky == 0) {
			sticky = rc;
		}
	}

	cdns_i3c_rtio_drain_rx(dev);

	cdns_i3c_rtio_complete_txn(dev, sticky);
}
