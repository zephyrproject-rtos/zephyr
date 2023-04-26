/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "isotp_internal.h"
#include <zephyr/net/buf.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/timeout_q.h>

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
		    ISOTP_CAN_DL, sizeof(uint32_t), receive_ff_sf_pool_free);

static struct isotp_global_ctx global_ctx = {
	.alloc_list = SYS_SLIST_STATIC_INIT(&global_ctx.alloc_list),
	.ff_sf_alloc_list = SYS_SLIST_STATIC_INIT(&global_ctx.ff_sf_alloc_list)
};

#ifdef CONFIG_ISOTP_USE_TX_BUF
NET_BUF_POOL_VAR_DEFINE(isotp_tx_pool, CONFIG_ISOTP_TX_BUF_COUNT,
			CONFIG_ISOTP_BUF_TX_DATA_POOL_SIZE, 0, NULL);
#endif

static void receive_state_machine(struct isotp_recv_ctx *ctx);

/*
 * Wake every context that is waiting for a buffer
 */
static void receive_pool_free(struct net_buf *buf)
{
	struct isotp_recv_ctx *ctx;
	sys_snode_t *ctx_node;

	net_buf_destroy(buf);

	SYS_SLIST_FOR_EACH_NODE(&global_ctx.alloc_list, ctx_node) {
		ctx = CONTAINER_OF(ctx_node, struct isotp_recv_ctx, alloc_node);
		k_work_submit(&ctx->work);
	}
}

static void receive_ff_sf_pool_free(struct net_buf *buf)
{
	struct isotp_recv_ctx *ctx;
	sys_snode_t *ctx_node;

	net_buf_destroy(buf);

	SYS_SLIST_FOR_EACH_NODE(&global_ctx.ff_sf_alloc_list, ctx_node) {
		ctx = CONTAINER_OF(ctx_node, struct isotp_recv_ctx, alloc_node);
		k_work_submit(&ctx->work);
	}
}

static inline void receive_report_error(struct isotp_recv_ctx *ctx, int err)
{
	ctx->state = ISOTP_RX_STATE_ERR;
	ctx->error_nr = err;
}

static void receive_can_tx(const struct device *dev, int error, void *arg)
{
	struct isotp_recv_ctx *ctx = (struct isotp_recv_ctx *)arg;

	ARG_UNUSED(dev);

	if (error != 0) {
		LOG_ERR("Error sending FC frame (%d)", error);
		receive_report_error(ctx, ISOTP_N_ERROR);
		k_work_submit(&ctx->work);
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

static inline uint32_t receive_get_sf_length(struct net_buf *buf)
{
	uint8_t len = net_buf_pull_u8(buf) & ISOTP_PCI_SF_DL_MASK;

	/* Single frames > 16 bytes (CAN-FD only) */
	if (IS_ENABLED(ISOTP_USE_CAN_FD) && !len) {
		len = net_buf_pull_u8(buf);
	}

	return len;
}

static void receive_send_fc(struct isotp_recv_ctx *ctx, uint8_t fs)
{
	struct can_frame frame = {
		.flags = (ctx->tx_addr.ide != 0 ? CAN_FRAME_IDE : 0) | (IS_ENABLED(ISOTP_USE_CAN_FD) ? CAN_FRAME_FDF : 0),
		.id = ctx->tx_addr.ext_id
	};
	uint8_t *data = frame.data;
	uint8_t payload_len;
	int ret;

	__ASSERT_NO_MSG(!(fs & ISOTP_PCI_TYPE_MASK));

	if (ctx->tx_addr.use_ext_addr) {
		*data++ = ctx->tx_addr.ext_addr;
	}

	*data++ = ISOTP_PCI_TYPE_FC | fs;
	*data++ = ctx->opts.bs;
	*data++ = ctx->opts.stmin;
	payload_len = data - frame.data;

#if defined(CONFIG_ISOTP_REQUIRE_RX_PADDING) || \
				defined(CONFIG_ISOTP_ENABLE_TX_PADDING)
	/* AUTOSAR requirement SWS_CanTp_00347 */
	memset(&frame.data[payload_len], 0xCC, ISOTP_CAN_DL - payload_len);
	frame.dlc = can_bytes_to_dlc(ISOTP_CAN_DL);
#else
	frame.dlc = can_bytes_to_dlc(payload_len);
#endif

	ret = can_send(ctx->can_dev, &frame, K_MSEC(ISOTP_A),
		       receive_can_tx, ctx);
	if (ret) {
		LOG_ERR("Can't send FC, (%d)", ret);
		receive_report_error(ctx, ISOTP_N_TIMEOUT_A);
		receive_state_machine(ctx);
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

static void receive_timeout_handler(struct _timeout *to)
{
	struct isotp_recv_ctx *ctx = CONTAINER_OF(to, struct isotp_recv_ctx,
						  timeout);

	switch (ctx->state) {
	case ISOTP_RX_STATE_WAIT_CF:
		LOG_ERR("Timeout while waiting for CF");
		receive_report_error(ctx, ISOTP_N_TIMEOUT_CR);
		break;

	case ISOTP_RX_STATE_TRY_ALLOC:
		ctx->state = ISOTP_RX_STATE_SEND_WAIT;
		break;
	}

	k_work_submit(&ctx->work);
}

static int receive_alloc_buffer(struct isotp_recv_ctx *ctx)
{
	struct net_buf *buf = NULL;

	if (ctx->opts.bs == 0) {
		/* Alloc all buffers because we can't wait during reception */
		buf = receive_alloc_buffer_chain(ctx->length);
	} else {
		buf = receive_alloc_buffer_chain(ctx->opts.bs *
						 (ISOTP_CAN_DL - 1));
	}

	if (!buf) {
		z_add_timeout(&ctx->timeout, receive_timeout_handler,
			      K_MSEC(ISOTP_ALLOC_TIMEOUT));

		if (ctx->wft == ISOTP_WFT_FIRST) {
			LOG_DBG("Allocation failed. Append to alloc list");
			ctx->wft = 0;
			sys_slist_append(&global_ctx.alloc_list,
					 &ctx->alloc_node);
		} else {
			LOG_DBG("Allocation failed. Send WAIT frame");
			ctx->state = ISOTP_RX_STATE_SEND_WAIT;
			receive_state_machine(ctx);
		}

		return -1;
	}

	if (ctx->state == ISOTP_RX_STATE_TRY_ALLOC) {
		z_abort_timeout(&ctx->timeout);
		ctx->wft = ISOTP_WFT_FIRST;
		sys_slist_find_and_remove(&global_ctx.alloc_list,
					  &ctx->alloc_node);
	}

	if (ctx->opts.bs != 0) {
		ctx->buf = buf;
	} else {
		net_buf_frag_insert(ctx->buf, buf);
	}

	ctx->act_frag = buf;
	return 0;
}

static void receive_state_machine(struct isotp_recv_ctx *ctx)
{
	int ret;
	uint32_t *ud_rem_len;

	switch (ctx->state) {
	case ISOTP_RX_STATE_PROCESS_SF:
		ctx->length = receive_get_sf_length(ctx->buf);
		ud_rem_len = net_buf_user_data(ctx->buf);
		*ud_rem_len = 0;
		LOG_DBG("SM process SF of length %d", ctx->length);
		net_buf_put(&ctx->fifo, ctx->buf);
		ctx->state = ISOTP_RX_STATE_RECYCLE;
		receive_state_machine(ctx);
		break;

	case ISOTP_RX_STATE_PROCESS_FF:
		ctx->length = receive_get_ff_length(ctx->buf);
		LOG_DBG("SM process FF. Length: %d", ctx->length);
		ctx->length -= ctx->buf->len;
		if (ctx->opts.bs == 0 &&
		    ctx->length > CONFIG_ISOTP_RX_BUF_COUNT *
		    CONFIG_ISOTP_RX_BUF_SIZE) {
			LOG_ERR("Pkt length is %d but buffer has only %d bytes",
				ctx->length,
				CONFIG_ISOTP_RX_BUF_COUNT *
				CONFIG_ISOTP_RX_BUF_SIZE);
			receive_report_error(ctx, ISOTP_N_BUFFER_OVERFLW);
			receive_state_machine(ctx);
			break;
		}

		if (ctx->opts.bs) {
			ctx->bs = ctx->opts.bs;
			ud_rem_len = net_buf_user_data(ctx->buf);
			*ud_rem_len = ctx->length;
			net_buf_put(&ctx->fifo, ctx->buf);
		}

		ctx->wft = ISOTP_WFT_FIRST;
		ctx->state = ISOTP_RX_STATE_TRY_ALLOC;
		__fallthrough;
	case ISOTP_RX_STATE_TRY_ALLOC:
		LOG_DBG("SM try to allocate");
		z_abort_timeout(&ctx->timeout);
		ret = receive_alloc_buffer(ctx);
		if (ret) {
			LOG_DBG("SM allocation failed. Wait for free buffer");
			break;
		}

		ctx->state = ISOTP_RX_STATE_SEND_FC;
		__fallthrough;
	case ISOTP_RX_STATE_SEND_FC:
		LOG_DBG("SM send CTS FC frame");
		receive_send_fc(ctx, ISOTP_PCI_FS_CTS);
		z_add_timeout(&ctx->timeout, receive_timeout_handler,
			      K_MSEC(ISOTP_CR));
		ctx->state = ISOTP_RX_STATE_WAIT_CF;
		break;

	case ISOTP_RX_STATE_SEND_WAIT:
		if (++ctx->wft < CONFIG_ISOTP_WFTMAX) {
			LOG_DBG("Send wait frame number %d", ctx->wft);
			receive_send_fc(ctx, ISOTP_PCI_FS_WAIT);
			z_add_timeout(&ctx->timeout, receive_timeout_handler,
				      K_MSEC(ISOTP_ALLOC_TIMEOUT));
			ctx->state = ISOTP_RX_STATE_TRY_ALLOC;
			break;
		}

		sys_slist_find_and_remove(&global_ctx.alloc_list,
					  &ctx->alloc_node);
		LOG_ERR("Sent %d wait frames. Giving up to alloc now",
			ctx->wft);
		receive_report_error(ctx, ISOTP_N_BUFFER_OVERFLW);
		__fallthrough;
	case ISOTP_RX_STATE_ERR:
		LOG_DBG("SM ERR state. err nr: %d", ctx->error_nr);
		z_abort_timeout(&ctx->timeout);

		if (ctx->error_nr == ISOTP_N_BUFFER_OVERFLW) {
			receive_send_fc(ctx, ISOTP_PCI_FS_OVFLW);
		}

		k_fifo_cancel_wait(&ctx->fifo);
		net_buf_unref(ctx->buf);
		ctx->buf = NULL;
		ctx->state = ISOTP_RX_STATE_RECYCLE;
		__fallthrough;
	case ISOTP_RX_STATE_RECYCLE:
		LOG_DBG("SM recycle context for next message");
		ctx->buf = net_buf_alloc_fixed(&isotp_rx_sf_ff_pool, K_NO_WAIT);
		if (!ctx->buf) {
			LOG_DBG("No free context. Append to waiters list");
			sys_slist_append(&global_ctx.ff_sf_alloc_list,
					 &ctx->alloc_node);
			break;
		}

		sys_slist_find_and_remove(&global_ctx.ff_sf_alloc_list,
					  &ctx->alloc_node);
		ctx->state = ISOTP_RX_STATE_WAIT_FF_SF;
		__fallthrough;
	case ISOTP_RX_STATE_UNBOUND:
		break;

	default:
		break;
	}
}

static void receive_work_handler(struct k_work *item)
{
	struct isotp_recv_ctx *ctx = CONTAINER_OF(item, struct isotp_recv_ctx,
						  work);

	receive_state_machine(ctx);
}

static void process_ff_sf(struct isotp_recv_ctx *ctx, struct can_frame *frame)
{
	int index = 0;
	uint8_t payload_len;
	uint32_t rx_sa;		/* ISO-TP fixed source address (if used) */

	if (ctx->rx_addr.use_ext_addr) {
		if (frame->data[index++] != ctx->rx_addr.ext_addr) {
			return;
		}
	}

	if (ctx->rx_addr.use_fixed_addr) {
		/* store actual CAN ID used by the sender */
		ctx->rx_addr.ext_id = frame->id;
		/* replace TX target address with RX source address */
		rx_sa = (frame->id & ISOTP_FIXED_ADDR_SA_MASK) >>
		     ISOTP_FIXED_ADDR_SA_POS;
		ctx->tx_addr.ext_id &= ~(ISOTP_FIXED_ADDR_TA_MASK);
		ctx->tx_addr.ext_id |= rx_sa << ISOTP_FIXED_ADDR_TA_POS;
		/* use same priority for TX as in received message */
		if (ISOTP_FIXED_ADDR_PRIO_MASK) {
			ctx->tx_addr.ext_id &= ~(ISOTP_FIXED_ADDR_PRIO_MASK);
			ctx->tx_addr.ext_id |= frame->id & ISOTP_FIXED_ADDR_PRIO_MASK;
		}
	}

	switch (frame->data[index] & ISOTP_PCI_TYPE_MASK) {
	case ISOTP_PCI_TYPE_FF:
		LOG_DBG("Got FF IRQ");
		if (can_dlc_to_bytes(frame->dlc) != ISOTP_CAN_DL) {
			LOG_INF("FF DLC invalid. Ignore");
			return;
		}

		payload_len = ISOTP_CAN_DL;
		ctx->state = ISOTP_RX_STATE_PROCESS_FF;
		ctx->sn_expected = 1;
		break;

	case ISOTP_PCI_TYPE_SF:
		LOG_DBG("Got SF IRQ");
#ifdef CONFIG_ISOTP_REQUIRE_RX_PADDING
		/* AUTOSAR requirement SWS_CanTp_00345 */
		if (can_dlc_to_bytes(frame->dlc) != ISOTP_CAN_DL) {
			LOG_INF("SF DLC invalid. Ignore");
			return;
		}
#endif

		payload_len = index + 1 + (frame->data[index] &
						ISOTP_PCI_SF_DL_MASK);

		if (payload_len > can_dlc_to_bytes(frame->dlc)) {
			LOG_INF("SF DL does not fit. Ignore");
			return;
		}

		ctx->state = ISOTP_RX_STATE_PROCESS_SF;
		break;

	default:
		LOG_INF("Got unexpected frame. Ignore");
		return;
	}

	net_buf_add_mem(ctx->buf, &frame->data[index], payload_len - index);
}

static inline void receive_add_mem(struct isotp_recv_ctx *ctx, uint8_t *data,
				   size_t len)
{
	size_t tailroom = net_buf_tailroom(ctx->act_frag);

	if (tailroom >= len) {
		net_buf_add_mem(ctx->act_frag, data, len);
		return;
	}

	/* Use next fragment that is already allocated*/
	net_buf_add_mem(ctx->act_frag, data, tailroom);
	ctx->act_frag = ctx->act_frag->frags;
	if (!ctx->act_frag) {
		LOG_ERR("No fragment left to append data");
		receive_report_error(ctx, ISOTP_N_BUFFER_OVERFLW);
		return;
	}

	net_buf_add_mem(ctx->act_frag, data + tailroom, len - tailroom);
}

static void process_cf(struct isotp_recv_ctx *ctx, struct can_frame *frame)
{
	uint32_t *ud_rem_len = (uint32_t *)net_buf_user_data(ctx->buf);
	int index = 0;
	uint32_t data_len;

	if (ctx->rx_addr.use_ext_addr) {
		if (frame->data[index++] != ctx->rx_addr.ext_addr) {
			return;
		}
	}

	if ((frame->data[index] & ISOTP_PCI_TYPE_MASK) != ISOTP_PCI_TYPE_CF) {
		LOG_DBG("Waiting for CF but got something else (%d)",
			frame->data[index] >> ISOTP_PCI_TYPE_POS);
		receive_report_error(ctx, ISOTP_N_UNEXP_PDU);
		k_work_submit(&ctx->work);
		return;
	}

	z_abort_timeout(&ctx->timeout);
	z_add_timeout(&ctx->timeout, receive_timeout_handler,
		      K_MSEC(ISOTP_CR));

	if ((frame->data[index++] & ISOTP_PCI_SN_MASK) != ctx->sn_expected++) {
		LOG_ERR("Sequence number mismatch");
		receive_report_error(ctx, ISOTP_N_WRONG_SN);
		k_work_submit(&ctx->work);
		return;
	}

#ifdef CONFIG_ISOTP_REQUIRE_RX_PADDING
	/* AUTOSAR requirement SWS_CanTp_00346 */
	if (can_dlc_to_bytesframe->dlc) != ISOTP_CAN_DL) {
		LOG_ERR("CF DL invalid");
		receive_report_error(ctx, ISOTP_N_ERROR);
		return;
	}
#endif

	LOG_DBG("Got CF irq. Appending data");
	data_len = (ctx->length > can_dlc_to_bytes(frame->dlc) - index) ? can_dlc_to_bytes(frame->dlc) - index :
		ctx->length;
	receive_add_mem(ctx, &frame->data[index], data_len);
	ctx->length -= data_len;
	LOG_DBG("%d bytes remaining", ctx->length);

	if (ctx->length == 0) {
		ctx->state = ISOTP_RX_STATE_RECYCLE;
		*ud_rem_len = 0;
		net_buf_put(&ctx->fifo, ctx->buf);
		return;
	}

	if (ctx->opts.bs && !--ctx->bs) {
		LOG_DBG("Block is complete. Allocate new buffer");
		ctx->bs = ctx->opts.bs;
		*ud_rem_len = ctx->length;
		net_buf_put(&ctx->fifo, ctx->buf);
		ctx->state = ISOTP_RX_STATE_TRY_ALLOC;
	}
}

static void receive_can_rx(const struct device *dev, struct can_frame *frame, void *arg)
{
	struct isotp_recv_ctx *ctx = (struct isotp_recv_ctx *)arg;

	ARG_UNUSED(dev);

	switch (ctx->state) {
	case ISOTP_RX_STATE_WAIT_FF_SF:
		__ASSERT_NO_MSG(ctx->buf);
		process_ff_sf(ctx, frame);
		break;

	case ISOTP_RX_STATE_WAIT_CF:
		process_cf(ctx, frame);
		/* still waiting for more CF */
		if (ctx->state == ISOTP_RX_STATE_WAIT_CF) {
			return;
		}

		break;

	case ISOTP_RX_STATE_RECYCLE:
		LOG_ERR("Got a frame but was not yet ready for a new one");
		receive_report_error(ctx, ISOTP_N_BUFFER_OVERFLW);
		break;

	default:
		LOG_INF("Got a frame in a state where it is unexpected.");
	}

	k_work_submit(&ctx->work);
}

static inline int attach_ff_filter(struct isotp_recv_ctx *ctx)
{
	uint32_t mask;

	if (ctx->rx_addr.use_fixed_addr) {
		mask = ISOTP_FIXED_ADDR_RX_MASK;
	} else {
		mask = CAN_EXT_ID_MASK;
	}

	struct can_filter filter = {
           .flags =  (IS_ENABLED(ISOTP_USE_CAN_FD) ? CAN_FILTER_FDF : 0) | CAN_FILTER_DATA | ((ctx->rx_addr.ide != 0) ? CAN_FILTER_IDE : 0),
		.id = ctx->rx_addr.ext_id,
		.mask = mask
	};

	ctx->filter_id = can_add_rx_filter(ctx->can_dev, receive_can_rx, ctx,
					   &filter);
	if (ctx->filter_id < 0) {
		LOG_ERR("Error attaching FF filter [%d]", ctx->filter_id);
		return ISOTP_NO_FREE_FILTER;
	}

	return 0;
}

int isotp_bind(struct isotp_recv_ctx *ctx, const struct device *can_dev,
	       const struct isotp_msg_id *rx_addr,
	       const struct isotp_msg_id *tx_addr,
	       const struct isotp_fc_opts *opts,
	       k_timeout_t timeout)
{
	int ret;

	__ASSERT(ctx, "ctx is NULL");
	__ASSERT(can_dev, "CAN device is NULL");
	__ASSERT(rx_addr && tx_addr, "RX or TX addr is NULL");
	__ASSERT(opts, "OPTS is NULL");

	ctx->can_dev = can_dev;
	ctx->rx_addr = *rx_addr;
	ctx->tx_addr = *tx_addr;
	k_fifo_init(&ctx->fifo);

	__ASSERT(opts->stmin < ISOTP_STMIN_MAX, "STmin limit");
	__ASSERT(opts->stmin <= ISOTP_STMIN_MS_MAX ||
		 opts->stmin >= ISOTP_STMIN_US_BEGIN, "STmin reserved");

	ctx->opts = *opts;
	ctx->state = ISOTP_RX_STATE_WAIT_FF_SF;

	LOG_DBG("Binding to addr: 0x%x. Responding on 0x%x",
		ctx->rx_addr.ext_id, ctx->tx_addr.ext_id);

	ctx->buf = net_buf_alloc_fixed(&isotp_rx_sf_ff_pool, timeout);
	if (!ctx->buf) {
		LOG_ERR("No buffer for FF left");
		return ISOTP_NO_NET_BUF_LEFT;
	}

	ret = attach_ff_filter(ctx);
	if (ret) {
		LOG_ERR("Can't attach filter for binding");
		net_buf_unref(ctx->buf);
		ctx->buf = NULL;
		return ret;
	}

	k_work_init(&ctx->work, receive_work_handler);
	z_init_timeout(&ctx->timeout);

	return ISOTP_N_OK;
}

void isotp_unbind(struct isotp_recv_ctx *ctx)
{
	struct net_buf *buf;

	if (ctx->filter_id >= 0 && ctx->can_dev) {
		can_remove_rx_filter(ctx->can_dev, ctx->filter_id);
	}

	z_abort_timeout(&ctx->timeout);

	sys_slist_find_and_remove(&global_ctx.ff_sf_alloc_list,
				  &ctx->alloc_node);
	sys_slist_find_and_remove(&global_ctx.alloc_list,
				  &ctx->alloc_node);

	ctx->state = ISOTP_RX_STATE_UNBOUND;

	while ((buf = net_buf_get(&ctx->fifo, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	k_fifo_cancel_wait(&ctx->fifo);

	if (ctx->buf) {
		net_buf_unref(ctx->buf);
	}

	LOG_DBG("Unbound");
}

int isotp_recv_net(struct isotp_recv_ctx *ctx, struct net_buf **buffer,
		   k_timeout_t timeout)
{
	struct net_buf *buf;
	int ret;

	buf = net_buf_get(&ctx->fifo, timeout);
	if (!buf) {
		ret = ctx->error_nr ? ctx->error_nr : ISOTP_RECV_TIMEOUT;
		ctx->error_nr = 0;

		return ret;
	}

	*buffer = buf;

	return *(uint32_t *)net_buf_user_data(buf);
}

int isotp_recv(struct isotp_recv_ctx *ctx, uint8_t *data, size_t len,
	       k_timeout_t timeout)
{
	size_t copied, to_copy;
	int err;

	if (!ctx->recv_buf) {
		ctx->recv_buf = net_buf_get(&ctx->fifo, timeout);
		if (!ctx->recv_buf) {
			err = ctx->error_nr ? ctx->error_nr : ISOTP_RECV_TIMEOUT;
			ctx->error_nr = 0;

			return err;
		}
	}

	/* traverse fragments and delete them after copying the data */
	copied = 0;
	while (ctx->recv_buf && copied < len) {
		to_copy = MIN(len - copied, ctx->recv_buf->len);
		memcpy((uint8_t *)data + copied, ctx->recv_buf->data, to_copy);

		if (ctx->recv_buf->len == to_copy) {
			/* point recv_buf to next frag */
			ctx->recv_buf = net_buf_frag_del(NULL, ctx->recv_buf);
		} else {
			/* pull received data from remaining frag(s) */
			net_buf_pull(ctx->recv_buf, to_copy);
		}

		copied += to_copy;
	}

	return copied;
}

static inline void send_report_error(struct isotp_send_ctx *ctx, uint32_t err)
{
	ctx->state = ISOTP_TX_ERR;
	ctx->error_nr = err;
}

static void send_can_tx_cb(const struct device *dev, int error, void *arg)
{
	struct isotp_send_ctx *ctx = (struct isotp_send_ctx *)arg;

	ARG_UNUSED(dev);

	ctx->tx_backlog--;
	k_sem_give(&ctx->tx_sem);

	if (ctx->state == ISOTP_TX_WAIT_BACKLOG) {
		if (ctx->tx_backlog > 0) {
			return;
		}

		ctx->state = ISOTP_TX_WAIT_FIN;
	}

	k_work_submit(&ctx->work);
}

static void send_timeout_handler(struct _timeout *to)
{
	struct isotp_send_ctx *ctx = CONTAINER_OF(to, struct isotp_send_ctx,
						  timeout);

	if (ctx->state != ISOTP_TX_SEND_CF) {
		send_report_error(ctx, ISOTP_N_TIMEOUT_BS);
		LOG_ERR("Reception of next FC has timed out");
	}

	k_work_submit(&ctx->work);
}

static void send_process_fc(struct isotp_send_ctx *ctx,
			    struct can_frame *frame)
{
	uint8_t *data = frame->data;

	if (ctx->rx_addr.use_ext_addr) {
		if (ctx->rx_addr.ext_addr != *data++) {
			return;
		}
	}

	if ((*data & ISOTP_PCI_TYPE_MASK) != ISOTP_PCI_TYPE_FC) {
		LOG_ERR("Got unexpected PDU expected FC");
		send_report_error(ctx, ISOTP_N_UNEXP_PDU);
		return;
	}

#ifdef CONFIG_ISOTP_ENABLE_TX_PADDING
	/* AUTOSAR requirement SWS_CanTp_00349 */
	if (frame->dlc != ISOTP_CAN_DL) {
		LOG_ERR("FC DL invalid. Ignore");
		send_report_error(ctx, ISOTP_N_ERROR);
		return;
	}
#endif

	switch (*data++ & ISOTP_PCI_FS_MASK) {
	case ISOTP_PCI_FS_CTS:
		ctx->state = ISOTP_TX_SEND_CF;
		ctx->wft = 0;
		ctx->tx_backlog = 0;
		k_sem_reset(&ctx->tx_sem);
		ctx->opts.bs = *data++;
		ctx->opts.stmin = *data++;
		ctx->bs = ctx->opts.bs;
		LOG_DBG("Got CTS. BS: %d, STmin: %d", ctx->opts.bs,
			ctx->opts.stmin);
		break;

	case ISOTP_PCI_FS_WAIT:
		LOG_DBG("Got WAIT frame");
		z_abort_timeout(&ctx->timeout);
		z_add_timeout(&ctx->timeout, send_timeout_handler,
			      K_MSEC(ISOTP_BS));
		if (ctx->wft >= CONFIG_ISOTP_WFTMAX) {
			LOG_INF("Got to many wait frames");
			send_report_error(ctx, ISOTP_N_WFT_OVRN);
		}

		ctx->wft++;
		break;

	case ISOTP_PCI_FS_OVFLW:
		LOG_ERR("Got overflow FC frame");
		send_report_error(ctx, ISOTP_N_BUFFER_OVERFLW);
		break;

	default:
		send_report_error(ctx, ISOTP_N_INVALID_FS);
	}
}

static void send_can_rx_cb(const struct device *dev, struct can_frame *frame, void *arg)
{
	struct isotp_send_ctx *ctx = (struct isotp_send_ctx *)arg;

	ARG_UNUSED(dev);

	if (ctx->state == ISOTP_TX_WAIT_FC) {
		z_abort_timeout(&ctx->timeout);
		send_process_fc(ctx, frame);
	} else {
		LOG_ERR("Got unexpected PDU");
		send_report_error(ctx, ISOTP_N_UNEXP_PDU);
	}

	k_work_submit(&ctx->work);
}

static size_t get_ctx_data_length(struct isotp_send_ctx *ctx)
{
	return ctx->is_net_buf ? net_buf_frags_len(ctx->buf) : ctx->len;
}

static const uint8_t *get_data_ctx(struct isotp_send_ctx *ctx)
{
	if (ctx->is_net_buf) {
		return ctx->buf->data;
	} else {
		return ctx->data;
	}
}

static void pull_data_ctx(struct isotp_send_ctx *ctx, size_t len)
{
	if (ctx->is_net_buf) {
		net_buf_pull_mem(ctx->buf, len);
	} else {
		ctx->data += len;
		ctx->len -= len;
	}
}

static inline int send_sf(struct isotp_send_ctx *ctx)
{
	struct can_frame frame = {
		.flags = (ctx->tx_addr.ide != 0 ? CAN_FRAME_IDE : 0) | (IS_ENABLED(ISOTP_USE_CAN_FD) ? CAN_FRAME_FDF : 0),
		.id = ctx->tx_addr.ext_id
	};
	size_t len = get_ctx_data_length(ctx);
	int index = 0;
	int ret;
	const uint8_t *data;

	data = get_data_ctx(ctx);
	pull_data_ctx(ctx, len);

	if (ctx->tx_addr.use_ext_addr) {
		frame.data[index++] = ctx->tx_addr.ext_addr;
	}

	frame.data[index++] = ISOTP_PCI_TYPE_SF | len;

	__ASSERT_NO_MSG(len <= ISOTP_CAN_DL - index);
	memcpy(&frame.data[index], data, len);

#ifdef CONFIG_ISOTP_ENABLE_TX_PADDING
	/* AUTOSAR requirement SWS_CanTp_00348 */
	memset(&frame.data[index + len], 0xCC, ISOTP_CAN_DL - len - index);
	frame.dlc = can_bytes_to_dlc(ISOTP_CAN_DL);
#else
	frame.dlc = can_bytes_to_dlc(len + index);
#endif

	ctx->state = ISOTP_TX_SEND_SF;
	ret = can_send(ctx->can_dev, &frame, K_MSEC(ISOTP_A),
		       send_can_tx_cb, ctx);
	return ret;
}

static inline int send_ff(struct isotp_send_ctx *ctx)
{
	struct can_frame frame = {
		.flags = (ctx->tx_addr.ide != 0 ? CAN_FRAME_IDE : 0) | (IS_ENABLED(ISOTP_USE_CAN_FD) ? CAN_FRAME_FDF : 0),
		.id = ctx->tx_addr.ext_id,
		.dlc = can_bytes_to_dlc(ISOTP_CAN_DL)
	};
	int index = 0;
	size_t len = get_ctx_data_length(ctx);
	int ret;
	const uint8_t *data;

	if (ctx->tx_addr.use_ext_addr) {
		frame.data[index++] = ctx->tx_addr.ext_addr;
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
	ctx->sn = 1;
	data = get_data_ctx(ctx);
	pull_data_ctx(ctx, ISOTP_CAN_DL - index);
	memcpy(&frame.data[index], data, ISOTP_CAN_DL - index);

	ret = can_send(ctx->can_dev, &frame, K_MSEC(ISOTP_A),
		       send_can_tx_cb, ctx);
	return ret;
}

static inline int send_cf(struct isotp_send_ctx *ctx)
{
	struct can_frame frame = {
		.flags = (ctx->tx_addr.ide != 0 ? CAN_FRAME_IDE : 0) | (IS_ENABLED(ISOTP_USE_CAN_FD) ? CAN_FRAME_FDF : 0),
		.id = ctx->tx_addr.ext_id,
	};
	int index = 0;
	int ret;
	int len;
	int rem_len;
	const uint8_t *data;

	if (ctx->tx_addr.use_ext_addr) {
		frame.data[index++] = ctx->tx_addr.ext_addr;
	}

	/*sn wraps around at 0xF automatically because it has a 4 bit size*/
	frame.data[index++] = ISOTP_PCI_TYPE_CF | ctx->sn;

	rem_len = get_ctx_data_length(ctx);
	len = MIN(rem_len, ISOTP_CAN_DL - index);
	rem_len -= len;
	data = get_data_ctx(ctx);
	memcpy(&frame.data[index], data, len);

#ifdef CONFIG_ISOTP_ENABLE_TX_PADDING
	/* AUTOSAR requirement SWS_CanTp_00348 */
	memset(&frame.data[index + len], 0xCC, ISOTP_CAN_DL - len - index);
	frame.dlc = can_bytes_to_dlc(ISOTP_CAN_DL);
#else
	frame.dlc = can_bytes_to_dlc(len + index);
#endif

	ret = can_send(ctx->can_dev, &frame, K_MSEC(ISOTP_A),
		       send_can_tx_cb, ctx);
	if (ret == 0) {
		ctx->sn++;
		pull_data_ctx(ctx, len);
		ctx->bs--;
		ctx->tx_backlog++;
	}

	ret = ret ? ret : rem_len;
	return ret;
}

#ifdef CONFIG_ISOTP_ENABLE_CONTEXT_BUFFERS
static inline void free_send_ctx(struct isotp_send_ctx **ctx)
{
	if ((*ctx)->is_net_buf) {
		net_buf_unref((*ctx)->buf);
		(*ctx)->buf = NULL;
	}

	if ((*ctx)->is_ctx_slab) {
		k_mem_slab_free(&ctx_slab, (void **)ctx);
	}
}

static int alloc_ctx(struct isotp_send_ctx **ctx, k_timeout_t timeout)
{
	int ret;

	ret = k_mem_slab_alloc(&ctx_slab, (void **)ctx, timeout);
	if (ret) {
		return ISOTP_NO_CTX_LEFT;
	}

	(*ctx)->is_ctx_slab = 1;

	return 0;
}
#else
#define free_send_ctx(x)
#endif /*CONFIG_ISOTP_ENABLE_CONTEXT_BUFFERS*/

static k_timeout_t stmin_to_ticks(uint8_t stmin)
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

static void send_state_machine(struct isotp_send_ctx *ctx)
{
	int ret;

	switch (ctx->state) {

	case ISOTP_TX_SEND_FF:
		send_ff(ctx);
		z_add_timeout(&ctx->timeout, send_timeout_handler,
			      K_MSEC(ISOTP_BS));
		ctx->state = ISOTP_TX_WAIT_FC;
		LOG_DBG("SM send FF");
		break;

	case ISOTP_TX_SEND_CF:
		LOG_DBG("SM send CF");
		z_abort_timeout(&ctx->timeout);
		do {
			ret = send_cf(ctx);
			if (!ret) {
				ctx->state = ISOTP_TX_WAIT_BACKLOG;
				break;
			}

			if (ret < 0) {
				LOG_ERR("Failed to send CF");
				send_report_error(ctx, ret == -EAGAIN ?
						ISOTP_N_TIMEOUT_A :
						ISOTP_N_ERROR);
				break;
			}

			if (ctx->opts.bs && !ctx->bs) {
				z_add_timeout(&ctx->timeout,
					      send_timeout_handler,
					      K_MSEC(ISOTP_BS));
				ctx->state = ISOTP_TX_WAIT_FC;
				LOG_DBG("BS reached. Wait for FC again");
				break;
			} else if (ctx->opts.stmin) {
				ctx->state = ISOTP_TX_WAIT_ST;
				break;
			}

			/* Ensure FIFO style transmission of CF */
			k_sem_take(&ctx->tx_sem, K_FOREVER);
		} while (ret > 0);

		break;

	case ISOTP_TX_WAIT_ST:
		z_add_timeout(&ctx->timeout, send_timeout_handler,
			      stmin_to_ticks(ctx->opts.stmin));
		ctx->state = ISOTP_TX_SEND_CF;
		LOG_DBG("SM wait ST");
		break;

        case ISOTP_TX_ERR:
		LOG_DBG("SM error");
		__fallthrough;
        case ISOTP_TX_SEND_SF:
		__fallthrough;
	case ISOTP_TX_WAIT_FIN:
		if (ctx->filter_id >= 0) {
			can_remove_rx_filter(ctx->can_dev, ctx->filter_id);
		}

		LOG_DBG("SM finish");
		z_abort_timeout(&ctx->timeout);

		if (ctx->has_callback) {
			ctx->fin_cb.cb(ctx->error_nr, ctx->fin_cb.arg);
			free_send_ctx(&ctx);
		} else {
			k_sem_give(&ctx->fin_sem);
		}

		ctx->state = ISOTP_TX_STATE_RESET;
		break;

	default:
		break;
	}
}

static void send_work_handler(struct k_work *item)
{
	struct isotp_send_ctx *ctx = CONTAINER_OF(item, struct isotp_send_ctx,
						  work);

	send_state_machine(ctx);
}

static inline int attach_fc_filter(struct isotp_send_ctx *ctx)
{
	struct can_filter filter = {
		.flags =  (IS_ENABLED(ISOTP_USE_CAN_FD) ? CAN_FILTER_FDF : 0) | CAN_FILTER_DATA | ((ctx->rx_addr.ide != 0) ? CAN_FILTER_IDE : 0),
		.id = ctx->rx_addr.ext_id,
		.mask = CAN_EXT_ID_MASK
	};

	ctx->filter_id = can_add_rx_filter(ctx->can_dev, send_can_rx_cb, ctx,
					   &filter);
	if (ctx->filter_id < 0) {
		LOG_ERR("Error attaching FC filter [%d]", ctx->filter_id);
		return ISOTP_NO_FREE_FILTER;
	}

	return 0;
}

static int send(struct isotp_send_ctx *ctx, const struct device *can_dev,
		const struct isotp_msg_id *tx_addr,
		const struct isotp_msg_id *rx_addr,
		isotp_tx_callback_t complete_cb, void *cb_arg)
{
	size_t len;
	int ret;

	__ASSERT_NO_MSG(ctx);
	__ASSERT_NO_MSG(can_dev);
	__ASSERT_NO_MSG(rx_addr && tx_addr);

	if (complete_cb) {
		ctx->fin_cb.cb = complete_cb;
		ctx->fin_cb.arg = cb_arg;
		ctx->has_callback = 1;
	} else {
		k_sem_init(&ctx->fin_sem, 0, 1);
		ctx->has_callback = 0;
	}

	k_sem_init(&ctx->tx_sem, 0, 1);
	ctx->can_dev = can_dev;
	ctx->tx_addr = *tx_addr;
	ctx->rx_addr = *rx_addr;
	ctx->error_nr = ISOTP_N_OK;
	ctx->wft = 0;
	k_work_init(&ctx->work, send_work_handler);
	z_init_timeout(&ctx->timeout);

	len = get_ctx_data_length(ctx);
	LOG_DBG("Send %zu bytes to addr 0x%x and listen on 0x%x", len,
		ctx->tx_addr.ext_id, ctx->rx_addr.ext_id);
	if (len > ISOTP_CAN_DL - (tx_addr->use_ext_addr ? 2 : 1)) {
		ret = attach_fc_filter(ctx);
		if (ret) {
			LOG_ERR("Can't attach fc filter: %d", ret);
			return ret;
		}

		LOG_DBG("Starting work to send FF");
		ctx->state = ISOTP_TX_SEND_FF;
		k_work_submit(&ctx->work);
	} else {
		LOG_DBG("Sending single frame");
		ctx->filter_id = -1;
		ret = send_sf(ctx);
                if (ret) {
			return ret == -EAGAIN ?
			       ISOTP_N_TIMEOUT_A : ISOTP_N_ERROR;
		}
	}

	if (!complete_cb) {
		k_sem_take(&ctx->fin_sem, K_FOREVER);
		ret = ctx->error_nr;
		free_send_ctx(&ctx);
		return ret;
	}

	return ISOTP_N_OK;
}

int isotp_send(struct isotp_send_ctx *ctx, const struct device *can_dev,
	       const uint8_t *data, size_t len,
	       const struct isotp_msg_id *tx_addr,
	       const struct isotp_msg_id *rx_addr,
	       isotp_tx_callback_t complete_cb, void *cb_arg)
{
	ctx->data = data;
	ctx->len = len;
	ctx->is_ctx_slab = 0;
	ctx->is_net_buf = 0;

	return send(ctx, can_dev, tx_addr, rx_addr, complete_cb, cb_arg);
}

#ifdef CONFIG_ISOTP_ENABLE_CONTEXT_BUFFERS

int isotp_send_ctx_buf(const struct device *can_dev,
		       const uint8_t *data, size_t len,
		       const struct isotp_msg_id *tx_addr,
		       const struct isotp_msg_id *rx_addr,
		       isotp_tx_callback_t complete_cb, void *cb_arg,
		       k_timeout_t timeout)
{
	struct isotp_send_ctx *ctx;
	int ret;

	__ASSERT_NO_MSG(data);

	ret = alloc_ctx(&ctx, timeout);
	if (ret) {
		return ret;
	}

	ctx->data = data;
	ctx->len = len;
	ctx->is_net_buf = 0;

	return send(ctx, can_dev, tx_addr, rx_addr, complete_cb, cb_arg);
}

int isotp_send_net_ctx_buf(const struct device *can_dev,
			   struct net_buf *data,
			   const struct isotp_msg_id *tx_addr,
			   const struct isotp_msg_id *rx_addr,
			   isotp_tx_callback_t complete_cb, void *cb_arg,
			   k_timeout_t timeout)
{
	struct isotp_send_ctx *ctx;
	int ret;

	__ASSERT_NO_MSG(data);

	ret = alloc_ctx(&ctx, timeout);
	if (ret) {
		return ret;
	}

	ctx->is_net_buf = 1;
	ctx->buf = data;

	return send(ctx, can_dev, tx_addr, rx_addr, complete_cb, cb_arg);
}

#ifdef CONFIG_ISOTP_USE_TX_BUF
int isotp_send_buf(const struct device *can_dev,
		   const uint8_t *data, size_t len,
		   const struct isotp_msg_id *tx_addr,
		   const struct isotp_msg_id *rx_addr,
		   isotp_tx_callback_t complete_cb, void *cb_arg,
		   k_timeout_t timeout)
{
	struct isotp_send_ctx *ctx;
	struct net_buf *buf;
	int ret;

	__ASSERT_NO_MSG(data);

	ret = alloc_ctx(&ctx, timeout);
	if (ret) {
		return ret;
	}

	buf = net_buf_alloc_len(&isotp_tx_pool, len, timeout);
	if (!buf) {
		k_mem_slab_free(&ctx_slab, (void **)&ctx);
		return ISOTP_NO_BUF_DATA_LEFT;
	}

	net_buf_add_mem(buf, data, len);

	ctx->is_net_buf = 1;
	ctx->buf = buf;

	return send(ctx, can_dev, tx_addr, rx_addr, complete_cb, cb_arg);
}
#endif  /*CONFIG_ISOTP_USE_TX_BUF*/
#endif  /*CONFIG_ISOTP_ENABLE_CONTEXT_BUFFERS*/
