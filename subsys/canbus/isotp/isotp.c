/*
 * Copyright (c) 2019 Alexander Wachter
 * Copyright (c) 2023 Enphase Energy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "isotp_internal.h"
#include <zephyr/net/buf.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(isotp, CONFIG_ISOTP_LOG_LEVEL);

#ifdef CONFIG_ISOTP_ENABLE_CONTEXT_BUFFERS
K_MEM_SLAB_DEFINE(ctx_slab, sizeof(struct isotp_send_ctx),
		  CONFIG_ISOTP_TX_CONTEXT_BUF_COUNT, 4);
#endif

static void receive_pool_free(struct net_buf *buf);
static void receive_ff_sf_pool_free(struct net_buf *buf);

NET_BUF_POOL_DEFINE(isotp_rx_pool, CONFIG_ISOTP_RX_BUF_COUNT,
		    CONFIG_ISOTP_RX_BUF_SIZE, sizeof(uint32_t),
		    receive_pool_free);

NET_BUF_POOL_DEFINE(isotp_rx_sf_ff_pool, CONFIG_ISOTP_RX_SF_FF_BUF_COUNT,
		    CAN_MAX_DLEN, sizeof(uint32_t), receive_ff_sf_pool_free);

static struct isotp_global_ctx global_ctx = {
	.alloc_list = SYS_SLIST_STATIC_INIT(&global_ctx.alloc_list),
	.ff_sf_alloc_list = SYS_SLIST_STATIC_INIT(&global_ctx.ff_sf_alloc_list)
};

#ifdef CONFIG_ISOTP_USE_TX_BUF
NET_BUF_POOL_VAR_DEFINE(isotp_tx_pool, CONFIG_ISOTP_TX_BUF_COUNT,
			CONFIG_ISOTP_BUF_TX_DATA_POOL_SIZE, 0, NULL);
#endif

static void receive_state_machine(struct isotp_recv_ctx *rctx);

static inline void prepare_frame(struct can_frame *frame, struct isotp_msg_id *addr)
{
	frame->id = addr->ext_id;
	frame->flags = ((addr->flags & ISOTP_MSG_IDE) != 0 ? CAN_FRAME_IDE : 0) |
		       ((addr->flags & ISOTP_MSG_FDF) != 0 ? CAN_FRAME_FDF : 0) |
		       ((addr->flags & ISOTP_MSG_BRS) != 0 ? CAN_FRAME_BRS : 0);
}

static inline void prepare_filter(struct can_filter *filter, struct isotp_msg_id *addr,
				  uint32_t mask)
{
	filter->id = addr->ext_id;
	filter->mask = mask;
	filter->flags = (addr->flags & ISOTP_MSG_IDE) != 0 ? CAN_FILTER_IDE : 0;
}

/*
 * Wake every context that is waiting for a buffer
 */
static void receive_pool_free(struct net_buf *buf)
{
	struct isotp_recv_ctx *rctx;
	sys_snode_t *rctx_node;

	net_buf_destroy(buf);

	SYS_SLIST_FOR_EACH_NODE(&global_ctx.alloc_list, rctx_node) {
		rctx = CONTAINER_OF(rctx_node, struct isotp_recv_ctx, alloc_node);
		k_work_submit(&rctx->work);
	}
}

static void receive_ff_sf_pool_free(struct net_buf *buf)
{
	struct isotp_recv_ctx *rctx;
	sys_snode_t *rctx_node;

	net_buf_destroy(buf);

	SYS_SLIST_FOR_EACH_NODE(&global_ctx.ff_sf_alloc_list, rctx_node) {
		rctx = CONTAINER_OF(rctx_node, struct isotp_recv_ctx, alloc_node);
		k_work_submit(&rctx->work);
	}
}

static inline void receive_report_error(struct isotp_recv_ctx *rctx, int err)
{
	rctx->state = ISOTP_RX_STATE_ERR;
	rctx->error_nr = err;
}

static void receive_can_tx(const struct device *dev, int error, void *arg)
{
	struct isotp_recv_ctx *rctx = (struct isotp_recv_ctx *)arg;

	ARG_UNUSED(dev);

	if (error != 0) {
		LOG_ERR("Error sending FC frame (%d)", error);
		receive_report_error(rctx, ISOTP_N_ERROR);
		k_work_submit(&rctx->work);
	}
}

static inline uint32_t receive_get_ff_length(struct net_buf *buf)
{
	uint32_t len;
	uint8_t pci = net_buf_pull_u8(buf);

	len = ((pci & ISOTP_PCI_FF_DL_UPPER_MASK) << 8) | net_buf_pull_u8(buf);

	/* Jumbo packet (32 bit length)*/
	if (!len) {
		len = net_buf_pull_be32(buf);
	}

	return len;
}

static inline uint32_t receive_get_sf_length(struct net_buf *buf, bool fdf)
{
	uint8_t len = net_buf_pull_u8(buf) & ISOTP_PCI_SF_DL_MASK;

	/* Single frames > 8 bytes (CAN FD only) */
	if (IS_ENABLED(CONFIG_CAN_FD_MODE) && fdf && !len) {
		len = net_buf_pull_u8(buf);
	}

	return len;
}

static void receive_send_fc(struct isotp_recv_ctx *rctx, uint8_t fs)
{
	struct can_frame frame;
	uint8_t *data = frame.data;
	uint8_t payload_len;
	int ret;

	__ASSERT_NO_MSG(!(fs & ISOTP_PCI_TYPE_MASK));

	prepare_frame(&frame, &rctx->tx_addr);

	if ((rctx->tx_addr.flags & ISOTP_MSG_EXT_ADDR) != 0) {
		*data++ = rctx->tx_addr.ext_addr;
	}

	*data++ = ISOTP_PCI_TYPE_FC | fs;
	*data++ = rctx->opts.bs;
	*data++ = rctx->opts.stmin;
	payload_len = data - frame.data;

#ifdef CONFIG_ISOTP_ENABLE_TX_PADDING
	/* AUTOSAR requirement SWS_CanTp_00347 */
	memset(&frame.data[payload_len], ISOTP_PAD_BYTE,
	       ISOTP_PADDED_FRAME_DL_MIN - payload_len);
	frame.dlc = can_bytes_to_dlc(ISOTP_PADDED_FRAME_DL_MIN);
#else
	frame.dlc = can_bytes_to_dlc(payload_len);
#endif

	ret = can_send(rctx->can_dev, &frame, K_MSEC(ISOTP_A_TIMEOUT_MS), receive_can_tx, rctx);
	if (ret) {
		LOG_ERR("Can't send FC, (%d)", ret);
		receive_report_error(rctx, ISOTP_N_TIMEOUT_A);
		receive_state_machine(rctx);
	}
}

static inline struct net_buf *receive_alloc_buffer_chain(uint32_t len)
{
	struct net_buf *buf, *frag, *last;
	uint32_t remaining_len;

	LOG_DBG("Allocate %d bytes ", len);
	buf = net_buf_alloc_fixed(&isotp_rx_pool, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	if (len <= CONFIG_ISOTP_RX_BUF_SIZE) {
		return buf;
	}

	remaining_len = len - CONFIG_ISOTP_RX_BUF_SIZE;
	last = buf;
	while (remaining_len) {
		frag = net_buf_alloc_fixed(&isotp_rx_pool, K_NO_WAIT);
		if (!frag) {
			net_buf_unref(buf);
			return NULL;
		}

		net_buf_frag_insert(last, frag);
		last = frag;
		remaining_len = remaining_len > CONFIG_ISOTP_RX_BUF_SIZE ?
				remaining_len - CONFIG_ISOTP_RX_BUF_SIZE : 0;
	}

	return buf;
}

static void receive_timeout_handler(struct k_timer *timer)
{
	struct isotp_recv_ctx *rctx = CONTAINER_OF(timer, struct isotp_recv_ctx, timer);

	switch (rctx->state) {
	case ISOTP_RX_STATE_WAIT_CF:
		LOG_ERR("Timeout while waiting for CF");
		receive_report_error(rctx, ISOTP_N_TIMEOUT_CR);
		break;

	case ISOTP_RX_STATE_TRY_ALLOC:
		rctx->state = ISOTP_RX_STATE_SEND_WAIT;
		break;
	}

	k_work_submit(&rctx->work);
}

static int receive_alloc_buffer(struct isotp_recv_ctx *rctx)
{
	struct net_buf *buf = NULL;

	if (rctx->opts.bs == 0) {
		/* Alloc all buffers because we can't wait during reception */
		buf = receive_alloc_buffer_chain(rctx->length);
	} else {
		/* Alloc the minimum of the remaining length and bytes of one block */
		uint32_t len = MIN(rctx->length, rctx->opts.bs * (rctx->rx_addr.dl - 1));

		buf = receive_alloc_buffer_chain(len);
	}

	if (!buf) {
		k_timer_start(&rctx->timer, K_MSEC(ISOTP_ALLOC_TIMEOUT_MS), K_NO_WAIT);

		if (rctx->wft == ISOTP_WFT_FIRST) {
			LOG_DBG("Allocation failed. Append to alloc list");
			rctx->wft = 0;
			sys_slist_append(&global_ctx.alloc_list, &rctx->alloc_node);
		} else {
			LOG_DBG("Allocation failed. Send WAIT frame");
			rctx->state = ISOTP_RX_STATE_SEND_WAIT;
			receive_state_machine(rctx);
		}

		return -1;
	}

	if (rctx->state == ISOTP_RX_STATE_TRY_ALLOC) {
		k_timer_stop(&rctx->timer);
		rctx->wft = ISOTP_WFT_FIRST;
		sys_slist_find_and_remove(&global_ctx.alloc_list, &rctx->alloc_node);
	}

	if (rctx->opts.bs != 0) {
		rctx->buf = buf;
	} else {
		net_buf_frag_insert(rctx->buf, buf);
	}

	rctx->act_frag = buf;
	return 0;
}

static void receive_state_machine(struct isotp_recv_ctx *rctx)
{
	int ret;
	uint32_t *ud_rem_len;

	switch (rctx->state) {
	case ISOTP_RX_STATE_PROCESS_SF:
		rctx->length = receive_get_sf_length(rctx->buf,
						    (rctx->rx_addr.flags & ISOTP_MSG_FDF) != 0);
		ud_rem_len = net_buf_user_data(rctx->buf);
		*ud_rem_len = 0;
		LOG_DBG("SM process SF of length %d", rctx->length);
		net_buf_put(&rctx->fifo, rctx->buf);
		rctx->state = ISOTP_RX_STATE_RECYCLE;
		receive_state_machine(rctx);
		break;

	case ISOTP_RX_STATE_PROCESS_FF:
		rctx->length = receive_get_ff_length(rctx->buf);
		LOG_DBG("SM process FF. Length: %d", rctx->length);
		rctx->length -= rctx->buf->len;
		if (rctx->opts.bs == 0 &&
		    rctx->length > CONFIG_ISOTP_RX_BUF_COUNT * CONFIG_ISOTP_RX_BUF_SIZE) {
			LOG_ERR("Pkt length is %d but buffer has only %d bytes", rctx->length,
				CONFIG_ISOTP_RX_BUF_COUNT * CONFIG_ISOTP_RX_BUF_SIZE);
			receive_report_error(rctx, ISOTP_N_BUFFER_OVERFLW);
			receive_state_machine(rctx);
			break;
		}

		if (rctx->opts.bs) {
			rctx->bs = rctx->opts.bs;
			ud_rem_len = net_buf_user_data(rctx->buf);
			*ud_rem_len = rctx->length;
			net_buf_put(&rctx->fifo, rctx->buf);
		}

		rctx->wft = ISOTP_WFT_FIRST;
		rctx->state = ISOTP_RX_STATE_TRY_ALLOC;
		__fallthrough;
	case ISOTP_RX_STATE_TRY_ALLOC:
		LOG_DBG("SM try to allocate");
		k_timer_stop(&rctx->timer);
		ret = receive_alloc_buffer(rctx);
		if (ret) {
			LOG_DBG("SM allocation failed. Wait for free buffer");
			break;
		}

		rctx->state = ISOTP_RX_STATE_SEND_FC;
		__fallthrough;
	case ISOTP_RX_STATE_SEND_FC:
		LOG_DBG("SM send CTS FC frame");
		receive_send_fc(rctx, ISOTP_PCI_FS_CTS);
		k_timer_start(&rctx->timer, K_MSEC(ISOTP_CR_TIMEOUT_MS), K_NO_WAIT);
		rctx->state = ISOTP_RX_STATE_WAIT_CF;
		break;

	case ISOTP_RX_STATE_SEND_WAIT:
		if (++rctx->wft < CONFIG_ISOTP_WFTMAX) {
			LOG_DBG("Send wait frame number %d", rctx->wft);
			receive_send_fc(rctx, ISOTP_PCI_FS_WAIT);
			k_timer_start(&rctx->timer, K_MSEC(ISOTP_ALLOC_TIMEOUT_MS), K_NO_WAIT);
			rctx->state = ISOTP_RX_STATE_TRY_ALLOC;
			break;
		}

		sys_slist_find_and_remove(&global_ctx.alloc_list, &rctx->alloc_node);
		LOG_ERR("Sent %d wait frames. Giving up to alloc now", rctx->wft);
		receive_report_error(rctx, ISOTP_N_BUFFER_OVERFLW);
		__fallthrough;
	case ISOTP_RX_STATE_ERR:
		LOG_DBG("SM ERR state. err nr: %d", rctx->error_nr);
		k_timer_stop(&rctx->timer);

		if (rctx->error_nr == ISOTP_N_BUFFER_OVERFLW) {
			receive_send_fc(rctx, ISOTP_PCI_FS_OVFLW);
		}

		k_fifo_cancel_wait(&rctx->fifo);
		net_buf_unref(rctx->buf);
		rctx->buf = NULL;
		rctx->state = ISOTP_RX_STATE_RECYCLE;
		__fallthrough;
	case ISOTP_RX_STATE_RECYCLE:
		LOG_DBG("SM recycle context for next message");
		rctx->buf = net_buf_alloc_fixed(&isotp_rx_sf_ff_pool, K_NO_WAIT);
		if (!rctx->buf) {
			LOG_DBG("No free context. Append to waiters list");
			sys_slist_append(&global_ctx.ff_sf_alloc_list, &rctx->alloc_node);
			break;
		}

		sys_slist_find_and_remove(&global_ctx.ff_sf_alloc_list, &rctx->alloc_node);
		rctx->state = ISOTP_RX_STATE_WAIT_FF_SF;
		__fallthrough;
	case ISOTP_RX_STATE_UNBOUND:
		break;

	default:
		break;
	}
}

static void receive_work_handler(struct k_work *item)
{
	struct isotp_recv_ctx *rctx = CONTAINER_OF(item, struct isotp_recv_ctx, work);

	receive_state_machine(rctx);
}

static void process_ff_sf(struct isotp_recv_ctx *rctx, struct can_frame *frame)
{
	int index = 0;
	uint8_t sf_len;
	uint8_t payload_len;
	uint32_t rx_sa;		/* ISO-TP fixed source address (if used) */
	uint8_t can_dl = can_dlc_to_bytes(frame->dlc);

	if ((rctx->rx_addr.flags & ISOTP_MSG_EXT_ADDR) != 0) {
		if (frame->data[index++] != rctx->rx_addr.ext_addr) {
			return;
		}
	}

	if ((rctx->rx_addr.flags & ISOTP_MSG_FIXED_ADDR) != 0) {
		/* store actual CAN ID used by the sender */
		rctx->rx_addr.ext_id = frame->id;
		/* replace TX target address with RX source address */
		rx_sa = (frame->id & ISOTP_FIXED_ADDR_SA_MASK) >>
		     ISOTP_FIXED_ADDR_SA_POS;
		rctx->tx_addr.ext_id &= ~(ISOTP_FIXED_ADDR_TA_MASK);
		rctx->tx_addr.ext_id |= rx_sa << ISOTP_FIXED_ADDR_TA_POS;
		/* use same priority for TX as in received message */
		if (ISOTP_FIXED_ADDR_PRIO_MASK) {
			rctx->tx_addr.ext_id &= ~(ISOTP_FIXED_ADDR_PRIO_MASK);
			rctx->tx_addr.ext_id |= frame->id & ISOTP_FIXED_ADDR_PRIO_MASK;
		}
	}

	switch (frame->data[index] & ISOTP_PCI_TYPE_MASK) {
	case ISOTP_PCI_TYPE_FF:
		LOG_DBG("Got FF IRQ");
		if (can_dl < ISOTP_FF_DL_MIN) {
			LOG_INF("FF DLC invalid. Ignore");
			return;
		}

		payload_len = can_dl;
		rctx->state = ISOTP_RX_STATE_PROCESS_FF;
		rctx->rx_addr.dl = can_dl;
		rctx->sn_expected = 1;
		break;

	case ISOTP_PCI_TYPE_SF:
		LOG_DBG("Got SF IRQ");
#ifdef CONFIG_ISOTP_REQUIRE_RX_PADDING
		/* AUTOSAR requirement SWS_CanTp_00345 */
		if (can_dl < ISOTP_PADDED_FRAME_DL_MIN) {
			LOG_INF("SF DLC invalid. Ignore");
			return;
		}
#endif
		sf_len = frame->data[index] & ISOTP_PCI_SF_DL_MASK;

		/* Single frames > 8 bytes (CAN FD only) */
		if (IS_ENABLED(CONFIG_CAN_FD_MODE) && (rctx->rx_addr.flags & ISOTP_MSG_FDF) != 0 &&
		    can_dl > ISOTP_4BIT_SF_MAX_CAN_DL) {
			if (sf_len != 0) {
				LOG_INF("SF DL invalid. Ignore");
				return;
			}
			sf_len = frame->data[index + 1];
			payload_len = index + 2 + sf_len;
		} else {
			payload_len = index + 1 + sf_len;
		}

		if (payload_len > can_dl) {
			LOG_INF("SF DL does not fit. Ignore");
			return;
		}

		rctx->state = ISOTP_RX_STATE_PROCESS_SF;
		break;

	default:
		LOG_INF("Got unexpected frame. Ignore");
		return;
	}

	net_buf_add_mem(rctx->buf, &frame->data[index], payload_len - index);
}

static inline void receive_add_mem(struct isotp_recv_ctx *rctx, uint8_t *data, size_t len)
{
	size_t tailroom = net_buf_tailroom(rctx->act_frag);

	if (tailroom >= len) {
		net_buf_add_mem(rctx->act_frag, data, len);
		return;
	}

	/* Use next fragment that is already allocated*/
	net_buf_add_mem(rctx->act_frag, data, tailroom);
	rctx->act_frag = rctx->act_frag->frags;
	if (!rctx->act_frag) {
		LOG_ERR("No fragment left to append data");
		receive_report_error(rctx, ISOTP_N_BUFFER_OVERFLW);
		return;
	}

	net_buf_add_mem(rctx->act_frag, data + tailroom, len - tailroom);
}

static void process_cf(struct isotp_recv_ctx *rctx, struct can_frame *frame)
{
	uint32_t *ud_rem_len = (uint32_t *)net_buf_user_data(rctx->buf);
	int index = 0;
	uint32_t data_len;
	uint8_t can_dl = can_dlc_to_bytes(frame->dlc);

	if ((rctx->rx_addr.flags & ISOTP_MSG_EXT_ADDR) != 0) {
		if (frame->data[index++] != rctx->rx_addr.ext_addr) {
			return;
		}
	}

	if ((frame->data[index] & ISOTP_PCI_TYPE_MASK) != ISOTP_PCI_TYPE_CF) {
		LOG_DBG("Waiting for CF but got something else (%d)",
			frame->data[index] >> ISOTP_PCI_TYPE_POS);
		receive_report_error(rctx, ISOTP_N_UNEXP_PDU);
		k_work_submit(&rctx->work);
		return;
	}

	k_timer_start(&rctx->timer, K_MSEC(ISOTP_CR_TIMEOUT_MS), K_NO_WAIT);

	if ((frame->data[index++] & ISOTP_PCI_SN_MASK) != rctx->sn_expected++) {
		LOG_ERR("Sequence number mismatch");
		receive_report_error(rctx, ISOTP_N_WRONG_SN);
		k_work_submit(&rctx->work);
		return;
	}

#ifdef CONFIG_ISOTP_REQUIRE_RX_PADDING
	/* AUTOSAR requirement SWS_CanTp_00346 */
	if (can_dl < ISOTP_PADDED_FRAME_DL_MIN) {
		LOG_ERR("CF DL invalid");
		receive_report_error(rctx, ISOTP_N_ERROR);
		return;
	}
#endif

	/* First frame defines the RX data length, consecutive frames
	 * must have the same length (except the last frame)
	 */
	if (can_dl != rctx->rx_addr.dl && rctx->length > can_dl - index) {
		LOG_ERR("CF DL invalid");
		receive_report_error(rctx, ISOTP_N_ERROR);
		return;
	}

	LOG_DBG("Got CF irq. Appending data");
	data_len = MIN(rctx->length, can_dl - index);
	receive_add_mem(rctx, &frame->data[index], data_len);
	rctx->length -= data_len;
	LOG_DBG("%d bytes remaining", rctx->length);

	if (rctx->length == 0) {
		rctx->state = ISOTP_RX_STATE_RECYCLE;
		*ud_rem_len = 0;
		net_buf_put(&rctx->fifo, rctx->buf);
		return;
	}

	if (rctx->opts.bs && !--rctx->bs) {
		LOG_DBG("Block is complete. Allocate new buffer");
		rctx->bs = rctx->opts.bs;
		*ud_rem_len = rctx->length;
		net_buf_put(&rctx->fifo, rctx->buf);
		rctx->state = ISOTP_RX_STATE_TRY_ALLOC;
	}
}

static void receive_can_rx(const struct device *dev, struct can_frame *frame, void *arg)
{
	struct isotp_recv_ctx *rctx = (struct isotp_recv_ctx *)arg;

	ARG_UNUSED(dev);

	if (IS_ENABLED(CONFIG_CAN_ACCEPT_RTR) && (frame->flags & CAN_FRAME_RTR) != 0U) {
		return;
	}

	switch (rctx->state) {
	case ISOTP_RX_STATE_WAIT_FF_SF:
		__ASSERT_NO_MSG(rctx->buf);
		process_ff_sf(rctx, frame);
		break;

	case ISOTP_RX_STATE_WAIT_CF:
		process_cf(rctx, frame);
		/* still waiting for more CF */
		if (rctx->state == ISOTP_RX_STATE_WAIT_CF) {
			return;
		}

		break;

	case ISOTP_RX_STATE_RECYCLE:
		LOG_ERR("Got a frame but was not yet ready for a new one");
		receive_report_error(rctx, ISOTP_N_BUFFER_OVERFLW);
		break;

	default:
		LOG_INF("Got a frame in a state where it is unexpected.");
	}

	k_work_submit(&rctx->work);
}

static inline int add_ff_sf_filter(struct isotp_recv_ctx *rctx)
{
	struct can_filter filter;
	uint32_t mask;

	if ((rctx->rx_addr.flags & ISOTP_MSG_FIXED_ADDR) != 0) {
		mask = ISOTP_FIXED_ADDR_RX_MASK;
	} else {
		mask = CAN_EXT_ID_MASK;
	}

	prepare_filter(&filter, &rctx->rx_addr, mask);

	rctx->filter_id = can_add_rx_filter(rctx->can_dev, receive_can_rx, rctx, &filter);
	if (rctx->filter_id < 0) {
		LOG_ERR("Error adding FF filter [%d]", rctx->filter_id);
		return ISOTP_NO_FREE_FILTER;
	}

	return 0;
}

int isotp_bind(struct isotp_recv_ctx *rctx, const struct device *can_dev,
	       const struct isotp_msg_id *rx_addr,
	       const struct isotp_msg_id *tx_addr,
	       const struct isotp_fc_opts *opts,
	       k_timeout_t timeout)
{
	can_mode_t cap;
	int ret;

	__ASSERT(rctx, "rctx is NULL");
	__ASSERT(can_dev, "CAN device is NULL");
	__ASSERT(rx_addr && tx_addr, "RX or TX addr is NULL");
	__ASSERT(opts, "OPTS is NULL");

	rctx->can_dev = can_dev;
	rctx->rx_addr = *rx_addr;
	rctx->tx_addr = *tx_addr;
	k_fifo_init(&rctx->fifo);

	__ASSERT(opts->stmin < ISOTP_STMIN_MAX, "STmin limit");
	__ASSERT(opts->stmin <= ISOTP_STMIN_MS_MAX ||
		 opts->stmin >= ISOTP_STMIN_US_BEGIN, "STmin reserved");

	rctx->opts = *opts;
	rctx->state = ISOTP_RX_STATE_WAIT_FF_SF;

	if ((rx_addr->flags & ISOTP_MSG_FDF) != 0 || (tx_addr->flags & ISOTP_MSG_FDF) != 0) {
		ret = can_get_capabilities(can_dev, &cap);
		if (ret != 0 || (cap & CAN_MODE_FD) == 0) {
			LOG_ERR("CAN controller does not support FD mode");
			return ISOTP_N_ERROR;
		}
	}

	LOG_DBG("Binding to addr: 0x%x. Responding on 0x%x",
		rctx->rx_addr.ext_id, rctx->tx_addr.ext_id);

	rctx->buf = net_buf_alloc_fixed(&isotp_rx_sf_ff_pool, timeout);
	if (!rctx->buf) {
		LOG_ERR("No buffer for FF left");
		return ISOTP_NO_NET_BUF_LEFT;
	}

	ret = add_ff_sf_filter(rctx);
	if (ret) {
		LOG_ERR("Can't add filter for binding");
		net_buf_unref(rctx->buf);
		rctx->buf = NULL;
		return ret;
	}

	k_work_init(&rctx->work, receive_work_handler);
	k_timer_init(&rctx->timer, receive_timeout_handler, NULL);

	return ISOTP_N_OK;
}

void isotp_unbind(struct isotp_recv_ctx *rctx)
{
	struct net_buf *buf;

	if (rctx->filter_id >= 0 && rctx->can_dev) {
		can_remove_rx_filter(rctx->can_dev, rctx->filter_id);
	}

	k_timer_stop(&rctx->timer);

	sys_slist_find_and_remove(&global_ctx.ff_sf_alloc_list, &rctx->alloc_node);
	sys_slist_find_and_remove(&global_ctx.alloc_list, &rctx->alloc_node);

	rctx->state = ISOTP_RX_STATE_UNBOUND;

	while ((buf = net_buf_get(&rctx->fifo, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	k_fifo_cancel_wait(&rctx->fifo);

	if (rctx->buf) {
		net_buf_unref(rctx->buf);
	}

	LOG_DBG("Unbound");
}

int isotp_recv_net(struct isotp_recv_ctx *rctx, struct net_buf **buffer, k_timeout_t timeout)
{
	struct net_buf *buf;
	int ret;

	buf = net_buf_get(&rctx->fifo, timeout);
	if (!buf) {
		ret = rctx->error_nr ? rctx->error_nr : ISOTP_RECV_TIMEOUT;
		rctx->error_nr = 0;

		return ret;
	}

	*buffer = buf;

	return *(uint32_t *)net_buf_user_data(buf);
}

int isotp_recv(struct isotp_recv_ctx *rctx, uint8_t *data, size_t len, k_timeout_t timeout)
{
	size_t copied, to_copy;
	int err;

	if (!rctx->recv_buf) {
		rctx->recv_buf = net_buf_get(&rctx->fifo, timeout);
		if (!rctx->recv_buf) {
			err = rctx->error_nr ? rctx->error_nr : ISOTP_RECV_TIMEOUT;
			rctx->error_nr = 0;

			return err;
		}
	}

	/* traverse fragments and delete them after copying the data */
	copied = 0;
	while (rctx->recv_buf && copied < len) {
		to_copy = MIN(len - copied, rctx->recv_buf->len);
		memcpy((uint8_t *)data + copied, rctx->recv_buf->data, to_copy);

		if (rctx->recv_buf->len == to_copy) {
			/* point recv_buf to next frag */
			rctx->recv_buf = net_buf_frag_del(NULL, rctx->recv_buf);
		} else {
			/* pull received data from remaining frag(s) */
			net_buf_pull(rctx->recv_buf, to_copy);
		}

		copied += to_copy;
	}

	return copied;
}

static inline void send_report_error(struct isotp_send_ctx *sctx, uint32_t err)
{
	sctx->state = ISOTP_TX_ERR;
	sctx->error_nr = err;
}

static void send_can_tx_cb(const struct device *dev, int error, void *arg)
{
	struct isotp_send_ctx *sctx = (struct isotp_send_ctx *)arg;

	ARG_UNUSED(dev);

	sctx->tx_backlog--;
	k_sem_give(&sctx->tx_sem);

	if (sctx->state == ISOTP_TX_WAIT_BACKLOG) {
		if (sctx->tx_backlog > 0) {
			return;
		}

		sctx->state = ISOTP_TX_WAIT_FIN;
	}

	k_work_submit(&sctx->work);
}

static void send_timeout_handler(struct k_timer *timer)
{
	struct isotp_send_ctx *sctx = CONTAINER_OF(timer, struct isotp_send_ctx, timer);

	if (sctx->state != ISOTP_TX_SEND_CF) {
		send_report_error(sctx, ISOTP_N_TIMEOUT_BS);
		LOG_ERR("Reception of next FC has timed out");
	}

	k_work_submit(&sctx->work);
}

static void send_process_fc(struct isotp_send_ctx *sctx, struct can_frame *frame)
{
	uint8_t *data = frame->data;

	if ((sctx->rx_addr.flags & ISOTP_MSG_EXT_ADDR) != 0) {
		if (sctx->rx_addr.ext_addr != *data++) {
			return;
		}
	}

	if ((*data & ISOTP_PCI_TYPE_MASK) != ISOTP_PCI_TYPE_FC) {
		LOG_ERR("Got unexpected PDU expected FC");
		send_report_error(sctx, ISOTP_N_UNEXP_PDU);
		return;
	}

#ifdef CONFIG_ISOTP_REQUIRE_RX_PADDING
	/* AUTOSAR requirement SWS_CanTp_00349 */
	if (frame->dlc < ISOTP_PADDED_FRAME_DL_MIN) {
		LOG_ERR("FC DL invalid. Ignore");
		send_report_error(sctx, ISOTP_N_ERROR);
		return;
	}
#endif

	switch (*data++ & ISOTP_PCI_FS_MASK) {
	case ISOTP_PCI_FS_CTS:
		sctx->state = ISOTP_TX_SEND_CF;
		sctx->wft = 0;
		sctx->tx_backlog = 0;
		k_sem_reset(&sctx->tx_sem);
		sctx->opts.bs = *data++;
		sctx->opts.stmin = *data++;
		sctx->bs = sctx->opts.bs;
		LOG_DBG("Got CTS. BS: %d, STmin: %d", sctx->opts.bs,
			sctx->opts.stmin);
		break;

	case ISOTP_PCI_FS_WAIT:
		LOG_DBG("Got WAIT frame");
		k_timer_start(&sctx->timer, K_MSEC(ISOTP_BS_TIMEOUT_MS), K_NO_WAIT);
		if (sctx->wft >= CONFIG_ISOTP_WFTMAX) {
			LOG_INF("Got to many wait frames");
			send_report_error(sctx, ISOTP_N_WFT_OVRN);
		}

		sctx->wft++;
		break;

	case ISOTP_PCI_FS_OVFLW:
		LOG_ERR("Got overflow FC frame");
		send_report_error(sctx, ISOTP_N_BUFFER_OVERFLW);
		break;

	default:
		send_report_error(sctx, ISOTP_N_INVALID_FS);
	}
}

static void send_can_rx_cb(const struct device *dev, struct can_frame *frame, void *arg)
{
	struct isotp_send_ctx *sctx = (struct isotp_send_ctx *)arg;

	ARG_UNUSED(dev);

	if (IS_ENABLED(CONFIG_CAN_ACCEPT_RTR) && (frame->flags & CAN_FRAME_RTR) != 0U) {
		return;
	}

	if (sctx->state == ISOTP_TX_WAIT_FC) {
		k_timer_stop(&sctx->timer);
		send_process_fc(sctx, frame);
	} else {
		LOG_ERR("Got unexpected PDU");
		send_report_error(sctx, ISOTP_N_UNEXP_PDU);
	}

	k_work_submit(&sctx->work);
}

static size_t get_send_ctx_data_len(struct isotp_send_ctx *sctx)
{
	return sctx->is_net_buf ? net_buf_frags_len(sctx->buf) : sctx->len;
}

static const uint8_t *get_send_ctx_data(struct isotp_send_ctx *sctx)
{
	if (sctx->is_net_buf) {
		return sctx->buf->data;
	} else {
		return sctx->data;
	}
}

static void pull_send_ctx_data(struct isotp_send_ctx *sctx, size_t len)
{
	if (sctx->is_net_buf) {
		net_buf_pull_mem(sctx->buf, len);
	} else {
		sctx->data += len;
		sctx->len -= len;
	}
}

static inline int send_sf(struct isotp_send_ctx *sctx)
{
	struct can_frame frame;
	size_t len = get_send_ctx_data_len(sctx);
	int index = 0;
	int ret;
	const uint8_t *data;

	prepare_frame(&frame, &sctx->tx_addr);

	data = get_send_ctx_data(sctx);
	pull_send_ctx_data(sctx, len);

	if ((sctx->tx_addr.flags & ISOTP_MSG_EXT_ADDR) != 0) {
		frame.data[index++] = sctx->tx_addr.ext_addr;
	}

	if (IS_ENABLED(CONFIG_CAN_FD_MODE) && (sctx->tx_addr.flags & ISOTP_MSG_FDF) != 0 &&
	    len > ISOTP_4BIT_SF_MAX_CAN_DL - 1 - index) {
		frame.data[index++] = ISOTP_PCI_TYPE_SF;
		frame.data[index++] = len;
	} else {
		frame.data[index++] = ISOTP_PCI_TYPE_SF | len;
	}

	if (len > sctx->tx_addr.dl - index) {
		LOG_ERR("SF len does not fit DL");
		return -ENOSPC;
	}

	memcpy(&frame.data[index], data, len);

	if (IS_ENABLED(CONFIG_ISOTP_ENABLE_TX_PADDING) ||
	    (IS_ENABLED(CONFIG_CAN_FD_MODE) && (sctx->tx_addr.flags & ISOTP_MSG_FDF) != 0 &&
	     len + index > ISOTP_PADDED_FRAME_DL_MIN)) {
		/* AUTOSAR requirements SWS_CanTp_00348 / SWS_CanTp_00351.
		 * Mandatory for ISO-TP CAN FD frames > 8 bytes.
		 */
		frame.dlc = can_bytes_to_dlc(
			MAX(ISOTP_PADDED_FRAME_DL_MIN, len + index));
		memset(&frame.data[index + len], ISOTP_PAD_BYTE,
		       can_dlc_to_bytes(frame.dlc) - len - index);
	} else {
		frame.dlc = can_bytes_to_dlc(len + index);
	}

	sctx->state = ISOTP_TX_SEND_SF;
	ret = can_send(sctx->can_dev, &frame, K_MSEC(ISOTP_A_TIMEOUT_MS), send_can_tx_cb, sctx);
	return ret;
}

static inline int send_ff(struct isotp_send_ctx *sctx)
{
	struct can_frame frame;
	int index = 0;
	size_t len = get_send_ctx_data_len(sctx);
	int ret;
	const uint8_t *data;

	prepare_frame(&frame, &sctx->tx_addr);

	frame.dlc = can_bytes_to_dlc(sctx->tx_addr.dl);

	if ((sctx->tx_addr.flags & ISOTP_MSG_EXT_ADDR) != 0) {
		frame.data[index++] = sctx->tx_addr.ext_addr;
	}

	if (len > 0xFFF) {
		frame.data[index++] = ISOTP_PCI_TYPE_FF;
		frame.data[index++] = 0;
		frame.data[index++] = (len >> 3 * 8) & 0xFF;
		frame.data[index++] = (len >> 2 * 8) & 0xFF;
		frame.data[index++] = (len >>   8) & 0xFF;
		frame.data[index++] = len & 0xFF;
	} else {
		frame.data[index++] = ISOTP_PCI_TYPE_FF | (len >> 8);
		frame.data[index++] = len & 0xFF;
	}

	/* According to ISO FF has sn 0 and is incremented to one
	 * although it's not part of the FF frame
	 */
	sctx->sn = 1;
	data = get_send_ctx_data(sctx);
	pull_send_ctx_data(sctx, sctx->tx_addr.dl - index);
	memcpy(&frame.data[index], data, sctx->tx_addr.dl - index);

	ret = can_send(sctx->can_dev, &frame, K_MSEC(ISOTP_A_TIMEOUT_MS), send_can_tx_cb, sctx);
	return ret;
}

static inline int send_cf(struct isotp_send_ctx *sctx)
{
	struct can_frame frame;
	int index = 0;
	int ret;
	int len;
	int rem_len;
	const uint8_t *data;

	prepare_frame(&frame, &sctx->tx_addr);

	if ((sctx->tx_addr.flags & ISOTP_MSG_EXT_ADDR) != 0) {
		frame.data[index++] = sctx->tx_addr.ext_addr;
	}

	/*sn wraps around at 0xF automatically because it has a 4 bit size*/
	frame.data[index++] = ISOTP_PCI_TYPE_CF | sctx->sn;

	rem_len = get_send_ctx_data_len(sctx);
	len = MIN(rem_len, sctx->tx_addr.dl - index);
	rem_len -= len;
	data = get_send_ctx_data(sctx);
	memcpy(&frame.data[index], data, len);

	if (IS_ENABLED(CONFIG_ISOTP_ENABLE_TX_PADDING) ||
	    (IS_ENABLED(CONFIG_CAN_FD_MODE) && (sctx->tx_addr.flags & ISOTP_MSG_FDF) != 0 &&
	     len + index > ISOTP_PADDED_FRAME_DL_MIN)) {
		/* AUTOSAR requirements SWS_CanTp_00348 / SWS_CanTp_00351.
		 * Mandatory for ISO-TP CAN FD frames > 8 bytes.
		 */
		frame.dlc = can_bytes_to_dlc(
			MAX(ISOTP_PADDED_FRAME_DL_MIN, len + index));
		memset(&frame.data[index + len], ISOTP_PAD_BYTE,
		       can_dlc_to_bytes(frame.dlc) - len - index);
	} else {
		frame.dlc = can_bytes_to_dlc(len + index);
	}

	ret = can_send(sctx->can_dev, &frame, K_MSEC(ISOTP_A_TIMEOUT_MS), send_can_tx_cb, sctx);
	if (ret == 0) {
		sctx->sn++;
		pull_send_ctx_data(sctx, len);
		sctx->bs--;
		sctx->tx_backlog++;
	}

	ret = ret ? ret : rem_len;
	return ret;
}

#ifdef CONFIG_ISOTP_ENABLE_CONTEXT_BUFFERS
static inline void free_send_ctx(struct isotp_send_ctx **sctx)
{
	if ((*sctx)->is_net_buf) {
		net_buf_unref((*sctx)->buf);
		(*sctx)->buf = NULL;
	}

	if ((*sctx)->is_ctx_slab) {
		k_mem_slab_free(&ctx_slab, (void *)*sctx);
	}
}

static int alloc_send_ctx(struct isotp_send_ctx **sctx, k_timeout_t timeout)
{
	int ret;

	ret = k_mem_slab_alloc(&ctx_slab, (void **)sctx, timeout);
	if (ret) {
		return ISOTP_NO_CTX_LEFT;
	}

	(*sctx)->is_ctx_slab = 1;

	return 0;
}
#else
#define free_send_ctx(x)
#endif /*CONFIG_ISOTP_ENABLE_CONTEXT_BUFFERS*/

static k_timeout_t stmin_to_timeout(uint8_t stmin)
{
	/* According to ISO 15765-2 stmin should be 127ms if value is corrupt */
	if (stmin > ISOTP_STMIN_MAX ||
	    (stmin > ISOTP_STMIN_MS_MAX && stmin < ISOTP_STMIN_US_BEGIN)) {
		return K_MSEC(ISOTP_STMIN_MS_MAX);
	}

	if (stmin >= ISOTP_STMIN_US_BEGIN) {
		return K_USEC((stmin + 1 - ISOTP_STMIN_US_BEGIN) * 100U);
	}

	return K_MSEC(stmin);
}

static void send_state_machine(struct isotp_send_ctx *sctx)
{
	int ret;

	switch (sctx->state) {

	case ISOTP_TX_SEND_FF:
		send_ff(sctx);
		k_timer_start(&sctx->timer, K_MSEC(ISOTP_BS_TIMEOUT_MS), K_NO_WAIT);
		sctx->state = ISOTP_TX_WAIT_FC;
		LOG_DBG("SM send FF");
		break;

	case ISOTP_TX_SEND_CF:
		LOG_DBG("SM send CF");
		k_timer_stop(&sctx->timer);
		do {
			ret = send_cf(sctx);
			if (!ret) {
				sctx->state = ISOTP_TX_WAIT_BACKLOG;
				break;
			}

			if (ret < 0) {
				LOG_ERR("Failed to send CF");
				send_report_error(sctx, ret == -EAGAIN ?
						ISOTP_N_TIMEOUT_A :
						ISOTP_N_ERROR);
				break;
			}

			if (sctx->opts.bs && !sctx->bs) {
				k_timer_start(&sctx->timer, K_MSEC(ISOTP_BS_TIMEOUT_MS), K_NO_WAIT);
				sctx->state = ISOTP_TX_WAIT_FC;
				LOG_DBG("BS reached. Wait for FC again");
				break;
			} else if (sctx->opts.stmin) {
				sctx->state = ISOTP_TX_WAIT_ST;
				break;
			}

			/* Ensure FIFO style transmission of CF */
			k_sem_take(&sctx->tx_sem, K_FOREVER);
		} while (ret > 0);

		break;

	case ISOTP_TX_WAIT_ST:
		k_timer_start(&sctx->timer, stmin_to_timeout(sctx->opts.stmin), K_NO_WAIT);
		sctx->state = ISOTP_TX_SEND_CF;
		LOG_DBG("SM wait ST");
		break;

	case ISOTP_TX_ERR:
		LOG_DBG("SM error");
		__fallthrough;
	case ISOTP_TX_SEND_SF:
		__fallthrough;
	case ISOTP_TX_WAIT_FIN:
		if (sctx->filter_id >= 0) {
			can_remove_rx_filter(sctx->can_dev, sctx->filter_id);
		}

		LOG_DBG("SM finish");
		k_timer_stop(&sctx->timer);

		if (sctx->has_callback) {
			sctx->fin_cb.cb(sctx->error_nr, sctx->fin_cb.arg);
			free_send_ctx(&sctx);
		} else {
			k_sem_give(&sctx->fin_sem);
		}

		sctx->state = ISOTP_TX_STATE_RESET;
		break;

	default:
		break;
	}
}

static void send_work_handler(struct k_work *item)
{
	struct isotp_send_ctx *sctx = CONTAINER_OF(item, struct isotp_send_ctx, work);

	send_state_machine(sctx);
}

static inline int add_fc_filter(struct isotp_send_ctx *sctx)
{
	struct can_filter filter;

	prepare_filter(&filter, &sctx->rx_addr, CAN_EXT_ID_MASK);

	sctx->filter_id = can_add_rx_filter(sctx->can_dev, send_can_rx_cb, sctx,
					   &filter);
	if (sctx->filter_id < 0) {
		LOG_ERR("Error adding FC filter [%d]", sctx->filter_id);
		return ISOTP_NO_FREE_FILTER;
	}

	return 0;
}

static int send(struct isotp_send_ctx *sctx, const struct device *can_dev,
		const struct isotp_msg_id *tx_addr,
		const struct isotp_msg_id *rx_addr,
		isotp_tx_callback_t complete_cb, void *cb_arg)
{
	can_mode_t cap;
	size_t len;
	int ret;

	__ASSERT_NO_MSG(sctx);
	__ASSERT_NO_MSG(can_dev);
	__ASSERT_NO_MSG(rx_addr && tx_addr);

	if ((rx_addr->flags & ISOTP_MSG_FDF) != 0 || (tx_addr->flags & ISOTP_MSG_FDF) != 0) {
		ret = can_get_capabilities(can_dev, &cap);
		if (ret != 0 || (cap & CAN_MODE_FD) == 0) {
			LOG_ERR("CAN controller does not support FD mode");
			return ISOTP_N_ERROR;
		}
	}

	if (complete_cb) {
		sctx->fin_cb.cb = complete_cb;
		sctx->fin_cb.arg = cb_arg;
		sctx->has_callback = 1;
	} else {
		k_sem_init(&sctx->fin_sem, 0, 1);
		sctx->has_callback = 0;
	}

	k_sem_init(&sctx->tx_sem, 0, 1);
	sctx->can_dev = can_dev;
	sctx->tx_addr = *tx_addr;
	sctx->rx_addr = *rx_addr;
	sctx->error_nr = ISOTP_N_OK;
	sctx->wft = 0;
	k_work_init(&sctx->work, send_work_handler);
	k_timer_init(&sctx->timer, send_timeout_handler, NULL);

	switch (sctx->tx_addr.dl) {
	case 0:
		if ((sctx->tx_addr.flags & ISOTP_MSG_FDF) == 0) {
			sctx->tx_addr.dl = 8;
		} else {
			sctx->tx_addr.dl = 64;
		}
		__fallthrough;
	case 8:
		break;
	case 12:
	case 16:
	case 20:
	case 24:
	case 32:
	case 48:
	case 64:
		if ((sctx->tx_addr.flags & ISOTP_MSG_FDF) == 0) {
			LOG_ERR("TX_DL > 8 only supported with FD mode");
			return ISOTP_N_ERROR;
		}
		break;
	default:
		LOG_ERR("Invalid TX_DL: %u", sctx->tx_addr.dl);
		return ISOTP_N_ERROR;
	}

	len = get_send_ctx_data_len(sctx);
	LOG_DBG("Send %zu bytes to addr 0x%x and listen on 0x%x", len,
		sctx->tx_addr.ext_id, sctx->rx_addr.ext_id);
	/* Single frames > 8 bytes use an additional byte for length (CAN FD only) */
	if (len > sctx->tx_addr.dl - (((tx_addr->flags & ISOTP_MSG_EXT_ADDR) != 0) ? 2 : 1) -
			  ((sctx->tx_addr.dl > ISOTP_4BIT_SF_MAX_CAN_DL) ? 1 : 0)) {
		ret = add_fc_filter(sctx);
		if (ret) {
			LOG_ERR("Can't add fc filter: %d", ret);
			free_send_ctx(&sctx);
			return ret;
		}

		LOG_DBG("Starting work to send FF");
		sctx->state = ISOTP_TX_SEND_FF;
		k_work_submit(&sctx->work);
	} else {
		LOG_DBG("Sending single frame");
		sctx->filter_id = -1;
		ret = send_sf(sctx);
		if (ret) {
			free_send_ctx(&sctx);
			return ret == -EAGAIN ?
			       ISOTP_N_TIMEOUT_A : ISOTP_N_ERROR;
		}
	}

	if (!complete_cb) {
		k_sem_take(&sctx->fin_sem, K_FOREVER);
		ret = sctx->error_nr;
		free_send_ctx(&sctx);
		return ret;
	}

	return ISOTP_N_OK;
}

int isotp_send(struct isotp_send_ctx *sctx, const struct device *can_dev,
	       const uint8_t *data, size_t len,
	       const struct isotp_msg_id *tx_addr,
	       const struct isotp_msg_id *rx_addr,
	       isotp_tx_callback_t complete_cb, void *cb_arg)
{
	sctx->data = data;
	sctx->len = len;
	sctx->is_ctx_slab = 0;
	sctx->is_net_buf = 0;

	return send(sctx, can_dev, tx_addr, rx_addr, complete_cb, cb_arg);
}

#ifdef CONFIG_ISOTP_ENABLE_CONTEXT_BUFFERS

int isotp_send_ctx_buf(const struct device *can_dev,
		       const uint8_t *data, size_t len,
		       const struct isotp_msg_id *tx_addr,
		       const struct isotp_msg_id *rx_addr,
		       isotp_tx_callback_t complete_cb, void *cb_arg,
		       k_timeout_t timeout)
{
	struct isotp_send_ctx *sctx;
	int ret;

	__ASSERT_NO_MSG(data);

	ret = alloc_send_ctx(&sctx, timeout);
	if (ret) {
		return ret;
	}

	sctx->data = data;
	sctx->len = len;
	sctx->is_net_buf = 0;

	return send(sctx, can_dev, tx_addr, rx_addr, complete_cb, cb_arg);
}

int isotp_send_net_ctx_buf(const struct device *can_dev,
			   struct net_buf *data,
			   const struct isotp_msg_id *tx_addr,
			   const struct isotp_msg_id *rx_addr,
			   isotp_tx_callback_t complete_cb, void *cb_arg,
			   k_timeout_t timeout)
{
	struct isotp_send_ctx *sctx;
	int ret;

	__ASSERT_NO_MSG(data);

	ret = alloc_send_ctx(&sctx, timeout);
	if (ret) {
		return ret;
	}

	sctx->is_net_buf = 1;
	sctx->buf = data;

	return send(sctx, can_dev, tx_addr, rx_addr, complete_cb, cb_arg);
}

#ifdef CONFIG_ISOTP_USE_TX_BUF
int isotp_send_buf(const struct device *can_dev,
		   const uint8_t *data, size_t len,
		   const struct isotp_msg_id *tx_addr,
		   const struct isotp_msg_id *rx_addr,
		   isotp_tx_callback_t complete_cb, void *cb_arg,
		   k_timeout_t timeout)
{
	struct isotp_send_ctx *sctx;
	struct net_buf *buf;
	int ret;

	__ASSERT_NO_MSG(data);

	ret = alloc_send_ctx(&sctx, timeout);
	if (ret) {
		return ret;
	}

	buf = net_buf_alloc_len(&isotp_tx_pool, len, timeout);
	if (!buf) {
		k_mem_slab_free(&ctx_slab, (void *)sctx);
		return ISOTP_NO_BUF_DATA_LEFT;
	}

	net_buf_add_mem(buf, data, len);

	sctx->is_net_buf = 1;
	sctx->buf = buf;

	return send(sctx, can_dev, tx_addr, rx_addr, complete_cb, cb_arg);
}
#endif  /*CONFIG_ISOTP_USE_TX_BUF*/
#endif  /*CONFIG_ISOTP_ENABLE_CONTEXT_BUFFERS*/
