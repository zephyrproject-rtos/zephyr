/*
 * Copyright (c) 2025 Martin Schröder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * COBS driver using UART async API with COBS framing.
 * This driver provides a network interface over a UART using COBS encoding.
 */

#include <zephyr/logging/log.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/dummy.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/random/random.h>
#include <zephyr/data/cobs.h>

LOG_MODULE_REGISTER(cobs_uart_async, CONFIG_COBS_UART_ASYNC_LOG_LEVEL);

/* COBS frame delimiter */
#define COBS_DELIMITER COBS_DEFAULT_DELIMITER

#define UART_BUF_LEN CONFIG_COBS_UART_ASYNC_RX_BUF_LEN
#define UART_TX_BUF_LEN CONFIG_COBS_UART_ASYNC_TX_BUF_LEN

/* Net buf pool for RX decoded data
 * Each buffer can hold CONFIG_NET_BUF_DATA_SIZE bytes of decoded data.
 * We need multiple buffers to handle fragmentation and pipelining.
 */
NET_BUF_POOL_DEFINE(rx_buf_pool, CONFIG_COBS_UART_ASYNC_RX_BUF_COUNT,
		    CONFIG_NET_BUF_DATA_SIZE, 0, NULL);

struct cobs_uart_async_config {
	const struct device *uart_dev;
};

struct cobs_uart_async_context {
	const struct device *dev;
	struct net_if *iface;

	/* COBS decoder state */
	struct cobs_decode_state decoder;
	struct net_buf *rx_buf_head;  /* Head of net_buf chain being assembled */
	struct net_buf *rx_buf_tail;  /* Tail for efficient appending */
	struct k_spinlock rx_lock;    /* Protects decoder and rx_buf chain */

	/* RX buffers for UART async API */
	uint8_t rx_buf1[UART_BUF_LEN];
	uint8_t rx_buf2[UART_BUF_LEN];
	uint8_t *rx_next_buf;

	/* Ring buffer for incoming UART data */
	struct ring_buf rx_ringbuf;
	uint8_t rx_ringbuf_data[CONFIG_COBS_UART_ASYNC_RINGBUF_SIZE];

	/* TX buffer */
	uint8_t tx_buf[UART_TX_BUF_LEN];
	struct k_sem tx_sem;

	/* Worker for processing received data */
	struct k_work rx_work;
	struct k_work_q rx_workq;
	K_KERNEL_STACK_MEMBER(rx_stack, CONFIG_COBS_UART_ASYNC_RX_STACK_SIZE);

	/* MAC address */
	uint8_t mac_addr[6];
	struct net_linkaddr ll_addr;

	/* Flags */
	atomic_t tx_busy;
	bool init_done;
};

static void uart_async_callback(const struct device *dev,
				struct uart_event *evt,
				void *user_data)
{
	struct cobs_uart_async_context *ctx = user_data;
	int ret;

	switch (evt->type) {
	case UART_TX_DONE:
		LOG_DBG("TX done: %zu bytes", evt->data.tx.len);
		atomic_clear(&ctx->tx_busy);
		k_sem_give(&ctx->tx_sem);
		break;

	case UART_TX_ABORTED:
		LOG_WRN("TX aborted: %zu bytes sent", evt->data.tx.len);
		atomic_clear(&ctx->tx_busy);
		k_sem_give(&ctx->tx_sem);
		break;

	case UART_RX_RDY:
		/* Data received, push to ring buffer */
		ret = ring_buf_put(&ctx->rx_ringbuf,
				   evt->data.rx.buf + evt->data.rx.offset,
				   evt->data.rx.len);
		if (ret < evt->data.rx.len) {
			LOG_ERR("!!!!! RX RING BUFFER OVERFLOW: dropped %d bytes - PACKET LOSS !!!!!",
				evt->data.rx.len - ret);
		}
		/* Schedule worker to process data */
		k_work_submit_to_queue(&ctx->rx_workq, &ctx->rx_work);
		break;

	case UART_RX_BUF_REQUEST:
		/* Provide next buffer */
		LOG_DBG("RX buf request, next_buf=%p", (void *)ctx->rx_next_buf);
		if (ctx->rx_next_buf) {
			ret = uart_rx_buf_rsp(dev, ctx->rx_next_buf, UART_BUF_LEN);
			if (ret < 0) {
				LOG_ERR("uart_rx_buf_rsp failed: %d", ret);
			} else {
				LOG_DBG("Provided buffer %p", (void *)ctx->rx_next_buf);
				/* Clear to avoid double-use */
				ctx->rx_next_buf = NULL;
			}
		} else {
			LOG_ERR("No buffer available for RX! RX will stop.");
		}
		break;

	case UART_RX_BUF_RELEASED:
		/* Buffer released, make it available again */
		LOG_DBG("RX buf released: %p", (void *)evt->data.rx_buf.buf);
		ctx->rx_next_buf = evt->data.rx_buf.buf;
		break;

	case UART_RX_DISABLED:
		LOG_DBG("RX disabled");
		break;

	case UART_RX_STOPPED:
		/* RX can stop due to errors (overrun/framing) during startup noise.
		 * The UART driver will continue with the next buffer automatically.
		 * Only log at debug level to avoid noise.
		 */
		LOG_DBG("RX stopped: reason %d (expected during startup)", evt->data.rx_stop.reason);
		break;

	default:
		break;
	}
}

static struct net_buf *alloc_rx_net_buf(void)
{
	/* Allocate net_buf for decoded data - can use K_NO_WAIT in worker context
	 * but not in ISR. We use a reasonable size for network packets.
	 */
	struct net_buf *buf = net_buf_alloc(&rx_buf_pool, K_NO_WAIT);

	if (buf) {
		net_buf_reserve(buf, 0);
	}
	return buf;
}

static void cobs_rx_work_handler(struct k_work *work)
{
	struct cobs_uart_async_context *ctx =
		CONTAINER_OF(work, struct cobs_uart_async_context, rx_work);
	uint8_t chunk[64];  /* Process in chunks for efficiency */
	size_t chunk_len;
	int ret;

	/* Process all available data in ring buffer */
	while ((chunk_len = ring_buf_get(&ctx->rx_ringbuf, chunk, sizeof(chunk))) > 0) {
		size_t offset = 0;

		while (offset < chunk_len) {
			k_spinlock_key_t key = k_spin_lock(&ctx->rx_lock);

			/* Ensure we have a net_buf to decode into */
			if (!ctx->rx_buf_tail) {
				struct net_buf *buf = alloc_rx_net_buf();

				if (!buf) {
					k_spin_unlock(&ctx->rx_lock, key);
					LOG_ERR("!!!!! RX net_buf allocation failed - DROPPING FRAME !!!!!");
					/* Drop data until we can allocate */
					break;
				}

				ctx->rx_buf_head = buf;
				ctx->rx_buf_tail = buf;
				/* Note: allocated new RX buffer chain */
			}

			/* Decode stream fragment into current buffer */
			ret = cobs_decode_stream(&ctx->decoder,
						 chunk + offset,
						 chunk_len - offset,
						 ctx->rx_buf_tail);

			if (ret > 0) {
				/* Processed some bytes */
				offset += ret;
				
				/* Check decoder flag to see if frame completed */
				if (ctx->decoder.frame_complete) {
					/* Check if we have a complete frame with data */
					if (ctx->rx_buf_head && ctx->rx_buf_head->len > 0) {
						struct net_pkt *pkt;
						size_t frame_len;

						/* Calculate frame length before creating packet */
						struct net_buf *tmp = ctx->rx_buf_head;
						frame_len = 0;
						while (tmp) {
							frame_len += tmp->len;
							tmp = tmp->frags;
						}

						/* Create net_pkt from assembled net_buf chain */
						pkt = net_pkt_rx_alloc_on_iface(ctx->iface, K_NO_WAIT);
						if (!pkt) {
							net_buf_unref(ctx->rx_buf_head);
							ctx->rx_buf_head = NULL;
							ctx->rx_buf_tail = NULL;
							k_spin_unlock(&ctx->rx_lock, key);
							LOG_ERR("!!!!! net_pkt allocation failed for %zu byte frame - DROPPING !!!!!", frame_len);
							continue;
						}

						/* Attach net_buf chain to packet */
						net_pkt_append_buffer(pkt, ctx->rx_buf_head);

						/* Reset for next frame */
						ctx->rx_buf_head = NULL;
						ctx->rx_buf_tail = NULL;
						cobs_decode_reset(&ctx->decoder);

						k_spin_unlock(&ctx->rx_lock, key);

						LOG_DBG("Complete frame: %zu bytes", net_pkt_get_len(pkt));

						/* Send to network stack */
						ret = net_recv_data(ctx->iface, pkt);
						if (ret < 0) {
							LOG_ERR("!!!!! net_recv_data REJECTED packet: %d !!!!!", ret);
							net_pkt_unref(pkt);
						}
					} else {
						/* Empty frame or just delimiter */
						if (ctx->rx_buf_head) {
							net_buf_unref(ctx->rx_buf_head);
							ctx->rx_buf_head = NULL;
							ctx->rx_buf_tail = NULL;
						}
						cobs_decode_reset(&ctx->decoder);
						k_spin_unlock(&ctx->rx_lock, key);
					}
				} else {
					/* Frame continues, just processed all available input */
					k_spin_unlock(&ctx->rx_lock, key);
				}
			} else if (ret == -ENOMEM) {
				/* Current buffer full, allocate a new one and chain it */
				struct net_buf *new_buf = alloc_rx_net_buf();

				if (!new_buf) {
					/* Drop this frame */
					if (ctx->rx_buf_head) {
						net_buf_unref(ctx->rx_buf_head);
						ctx->rx_buf_head = NULL;
						ctx->rx_buf_tail = NULL;
					}
					cobs_decode_reset(&ctx->decoder);
					k_spin_unlock(&ctx->rx_lock, key);
					LOG_ERR("!!!!! Chained net_buf allocation failed - DROPPING FRAME !!!!!");
					break;
				}

				/* Chain new buffer */
				net_buf_frag_add(ctx->rx_buf_tail, new_buf);
				ctx->rx_buf_tail = new_buf;
				k_spin_unlock(&ctx->rx_lock, key);
				/* Continue processing without advancing offset */
			} else {
				/* Decode error - reset decoder and continue with next byte
				 * The COBS decoder will resync automatically when it sees
				 * a valid code byte or delimiter.
				 */
				size_t error_offset = offset;
				
				/* Discard partial frame */
				if (ctx->rx_buf_head) {
					net_buf_unref(ctx->rx_buf_head);
					ctx->rx_buf_head = NULL;
					ctx->rx_buf_tail = NULL;
				}
				cobs_decode_reset(&ctx->decoder);
				k_spin_unlock(&ctx->rx_lock, key);
				
				LOG_WRN("COBS decode error: %d at offset %zu/%zu, resetting decoder", 
					ret, error_offset, chunk_len);
				LOG_HEXDUMP_DBG(chunk + error_offset, MIN(chunk_len - error_offset, 32), "RX error data:");
				
				/* Skip the bad byte and continue */
				offset++;
			}
		}
	}
}

static int cobs_uart_async_send(const struct device *dev, struct net_pkt *pkt)
{
	struct cobs_uart_async_context *ctx = dev->data;
	const struct cobs_uart_async_config *config = dev->config;
	const struct device *uart_dev = config->uart_dev;
	struct cobs_encode_state encoder;
	size_t total_len;
	size_t encoded_len;
	int ret;

	if (!pkt || !pkt->buffer) {
		return -ENODATA;
	}

	/* Check if TX is busy */
	if (atomic_test_and_set_bit(&ctx->tx_busy, 0)) {
		return -EBUSY;
	}

	/* Calculate total packet length */
	total_len = net_pkt_get_len(pkt);

	/* Check if encoded data will fit in TX buffer */
	size_t max_encoded = cobs_max_encoded_len(total_len, 0) + 1;

	if (max_encoded > UART_TX_BUF_LEN) {
		LOG_ERR("Packet too large: %zu encoded > %d", max_encoded, UART_TX_BUF_LEN);
		atomic_clear(&ctx->tx_busy);
		return -EMSGSIZE;
	}

	/* Use non-destructive streaming COBS encoder */
	cobs_encode_init(&encoder);

	/* Encode into tx_buf (should complete in one call since buffer is large enough) */
	encoded_len = UART_TX_BUF_LEN;
	ret = cobs_encode_stream(&encoder, pkt->buffer, ctx->tx_buf, &encoded_len);
	if (ret < 0) {
		LOG_ERR("COBS encode stream failed: %d", ret);
		atomic_clear(&ctx->tx_busy);
		return ret;
	}

	/* Finalize encoding (should add nothing since we encoded everything) */
	size_t final_len = UART_TX_BUF_LEN - encoded_len;
	ret = cobs_encode_finalize(&encoder, ctx->tx_buf + encoded_len, &final_len);
	if (ret < 0) {
		LOG_ERR("COBS encode finalize failed: %d", ret);
		atomic_clear(&ctx->tx_busy);
		return ret;
	}
	encoded_len += final_len;

	/* Add frame delimiter */
	if (encoded_len >= UART_TX_BUF_LEN) {
		LOG_ERR("No space for delimiter");
		atomic_clear(&ctx->tx_busy);
		return -ENOMEM;
	}
	ctx->tx_buf[encoded_len++] = COBS_DELIMITER;

	LOG_DBG("TX: Sending %zu bytes (encoded from %zu)", encoded_len, total_len);
	LOG_HEXDUMP_DBG(ctx->tx_buf, MIN(encoded_len, 64), "TX data:");

	/* Send via UART async */
	ret = uart_tx(uart_dev, ctx->tx_buf, encoded_len,
		      CONFIG_COBS_UART_ASYNC_TX_TIMEOUT_MS * 1000);
	if (ret < 0) {
		LOG_ERR("uart_tx failed: %d", ret);
		atomic_clear(&ctx->tx_busy);
		return ret;
	}

	/* Wait for TX to complete */
	ret = k_sem_take(&ctx->tx_sem, K_MSEC(CONFIG_COBS_UART_ASYNC_TX_TIMEOUT_MS + 100));
	if (ret < 0) {
		LOG_ERR("TX timeout");
		atomic_clear(&ctx->tx_busy);
		return ret;
	}

	return 0;
}

static int cobs_uart_async_init(const struct device *dev)
{
	struct cobs_uart_async_context *ctx = dev->data;
	const struct cobs_uart_async_config *config = dev->config;
	const struct device *uart_dev = config->uart_dev;
	int ret;

	LOG_DBG("Initializing COBS UART async driver for %s", dev->name);

	/* Check UART device readiness first */
	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device %s not ready", uart_dev->name);
		return -ENODEV;
	}

	ctx->dev = dev;

	/* Initialize COBS decoder */
	cobs_decode_init(&ctx->decoder);

	/* Initialize RX buffer chain */
	ctx->rx_buf_head = NULL;
	ctx->rx_buf_tail = NULL;

	ring_buf_init(&ctx->rx_ringbuf, sizeof(ctx->rx_ringbuf_data),
		      ctx->rx_ringbuf_data);

	k_sem_init(&ctx->tx_sem, 0, 1);
	atomic_clear(&ctx->tx_busy);

	k_work_init(&ctx->rx_work, cobs_rx_work_handler);

	/* Start RX worker queue immediately */
	k_work_queue_start(&ctx->rx_workq, ctx->rx_stack,
			   K_KERNEL_STACK_SIZEOF(ctx->rx_stack),
			   CONFIG_COBS_UART_ASYNC_RX_PRIORITY, NULL);
	k_thread_name_set(&ctx->rx_workq.thread, dev->name);

	/* Configure UART async callback */
	ret = uart_callback_set(uart_dev, uart_async_callback, ctx);
	if (ret < 0) {
		LOG_ERR("Failed to set UART callback: %d", ret);
		return ret;
	}

	/* Enable RX immediately so we're ready to receive */
	ctx->rx_next_buf = ctx->rx_buf2;
	ret = uart_rx_enable(uart_dev, ctx->rx_buf1, UART_BUF_LEN,
			     CONFIG_COBS_UART_ASYNC_RX_TIMEOUT_US);
	if (ret < 0) {
		LOG_ERR("Failed to enable UART RX: %d", ret);
		return ret;
	}

	/* Give initial TX semaphore */
	k_sem_give(&ctx->tx_sem);

	LOG_INF("COBS UART async driver %s initialized on %s (RX enabled)",
		dev->name, uart_dev->name);

	return 0;
}

static void cobs_uart_async_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct cobs_uart_async_context *ctx = dev->data;

	LOG_DBG("Initializing interface for dev %s", dev->name);

	if (ctx->init_done) {
		return;
	}

	ctx->init_done = true;
	ctx->iface = iface;

	/* Generate MAC address (00-00-5E-00-53-xx per RFC 7042) */
	ctx->mac_addr[0] = 0x00;
	ctx->mac_addr[1] = 0x00;
	ctx->mac_addr[2] = 0x5E;
	ctx->mac_addr[3] = 0x00;
	ctx->mac_addr[4] = 0x53;
	ctx->mac_addr[5] = sys_rand8_get();

	net_linkaddr_set(&ctx->ll_addr, ctx->mac_addr,
			 sizeof(ctx->mac_addr));
	net_if_set_link_addr(iface, ctx->ll_addr.addr, ctx->ll_addr.len,
			     NET_LINK_DUMMY);

	/* Set point-to-point flag */
	net_if_flag_set(iface, NET_IF_POINTOPOINT);

	LOG_DBG("Interface %s ready (UART RX already enabled)", dev->name);
}

static const struct dummy_api cobs_uart_async_api = {
	.iface_api.init = cobs_uart_async_iface_init,
	.send = cobs_uart_async_send,
};

/* Multi-instance support via devicetree */
#define DT_DRV_COMPAT zephyr_cobs_uart_async

#define COBS_UART_ASYNC_DEVICE_INIT(inst)					\
	static const struct cobs_uart_async_config				\
		cobs_uart_async_config_##inst = {				\
		.uart_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, uart)),	\
	};									\
										\
	static struct cobs_uart_async_context					\
		cobs_uart_async_context_##inst;				\
										\
	NET_DEVICE_DT_INST_DEFINE(inst,					\
				  cobs_uart_async_init,			\
				  NULL,					\
				  &cobs_uart_async_context_##inst,	\
				  &cobs_uart_async_config_##inst,	\
				  CONFIG_COBS_UART_ASYNC_INIT_PRIORITY,	\
				  &cobs_uart_async_api,			\
				  COBS_SERIAL_L2,			\
				  NET_L2_GET_CTX_TYPE(COBS_SERIAL_L2),	\
				  CONFIG_COBS_UART_ASYNC_MTU);

DT_INST_FOREACH_STATUS_OKAY(COBS_UART_ASYNC_DEVICE_INIT)

