/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_frontend_stmesp.h>
#include <zephyr/logging/log_frontend_stmesp_demux.h>
#include <zephyr/debug/coresight/cs_trace_defmt.h>
#include <zephyr/debug/mipi_stp_decoder.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/drivers/misc/coresight/nrf_etr.h>
#include <zephyr/sys/printk.h>
#include <nrfx_tbm.h>
#include <stdio.h>
LOG_MODULE_REGISTER(cs_etr_tbm);

#define UART_NODE DT_CHOSEN(zephyr_console)

#define ETR_BUFFER_NODE DT_NODELABEL(etr_buffer)

#define DROP_CHECK_PERIOD                            \
	COND_CODE_1(CONFIG_NRF_ETR_DECODE, \
		    (CONFIG_NRF_ETR_DECODE_DROP_PERIOD), (0))

#define MIN_DATA (2 * CORESIGHT_TRACE_FRAME_SIZE32)

#define MEMORY_SECTION(node)                                                                  \
	COND_CODE_1(DT_NODE_HAS_PROP(node, memory_regions),                                   \
		    (__attribute__((__section__(                                              \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(node, memory_regions)))))), \
		    ())

/* Since ETR debug is a part of logging infrastructure, logging cannot be used
 * for debugging. Printk is used (assuming CONFIG_LOG_PRINTK=n)
 */
#define DBG(...) IF_ENABLED(CONFIG_NRF_ETR_DEBUG, (printk(__VA_ARGS__)))

/** @brief Macro for dumping debug data.
 *
 * @param _data Data variable.
 * @param _nlen Number of nibbles in @p _data to print.
 */
#define DBG_DATA(_data, _nlen, _marked)                   \
	do {                                              \
		char *fmt;                                \
		switch (_nlen) {                          \
		case 2:                                   \
			fmt = "D%s\t%02x ";               \
			break;                            \
		case 4:                                   \
			fmt = "D%s\t%04x ";               \
			break;                            \
		case 8:                                   \
			fmt = "D%s\t%08x ";               \
			break;                            \
		default:                                  \
			fmt = "D%s\t%016x ";              \
			break;                            \
		}                                         \
		DBG(fmt, _marked ? "M" : "", _data);      \
		for (int i = 0; i < _nlen / 2; i++) {     \
			DBG("%c ", ((char *)&_data)[i]);  \
		}                                         \
		DBG("\n");                                \
	} while (0)

static const uint32_t wsize_mask = DT_REG_SIZE(ETR_BUFFER_NODE) / sizeof(int) - 1;
static const uint32_t wsize_inc = DT_REG_SIZE(ETR_BUFFER_NODE) / sizeof(int) - 1;

static bool in_sync;
static int oosync_cnt;
static volatile bool tbm_full;
static volatile uint32_t base_wr_idx;
static uint32_t etr_rd_idx;
/* Counts number of new messages completed in the current formatter frame decoding. */
static uint32_t new_msg_cnt;

static bool volatile use_async_uart;

static struct k_sem uart_sem;
static const struct device *uart_dev = DEVICE_DT_GET(UART_NODE);
static uint32_t frame_buf0[CORESIGHT_TRACE_FRAME_SIZE32] MEMORY_SECTION(UART_NODE);
static uint32_t frame_buf1[CORESIGHT_TRACE_FRAME_SIZE32] MEMORY_SECTION(UART_NODE);
static uint32_t frame_buf_decode[CORESIGHT_TRACE_FRAME_SIZE32];
static uint32_t *frame_buf = IS_ENABLED(CONFIG_NRF_ETR_DECODE) ?
				frame_buf_decode : frame_buf0;

K_KERNEL_STACK_DEFINE(etr_stack, CONFIG_NRF_ETR_STACK_SIZE);
static struct k_thread etr_thread;

BUILD_ASSERT((DT_REG_SIZE(ETR_BUFFER_NODE) % CONFIG_DCACHE_LINE_SIZE) == 0);
BUILD_ASSERT((DT_REG_ADDR(ETR_BUFFER_NODE) % CONFIG_DCACHE_LINE_SIZE) == 0);

/* Domain details and prefixes. */
static const uint16_t stm_m_id[] = {0x21, 0x22, 0x23, 0x2c, 0x2d, 0x2e, 0x24, 0x80};
static const char *const stm_m_name[] = {"sec", "app", "rad", "sys", "flpr", "ppr", "mod", "hw"};
static const char *const hw_evts[] = {
	"CTI211_0",  /* 0 CTI211 triger out 1 */
	"CTI211_1",  /* 1 CTI211 triger out 1 inverted */
	"CTI211_2",  /* 2 CTI211 triger out 2 */
	"CTI211_3",  /* 3 CTI211 triger out 2 inverted*/
	"Sec up",    /* 4 Secure Domain up */
	"Sec down",  /* 5 Secure Domain down */
	"App up",    /* 6 Application Domain up */
	"App down",  /* 7 Application Domain down */
	"Rad up",    /* 8 Radio Domain up */
	"Rad down",  /* 9 Radio Domain down */
	"Radf up",   /* 10 Radio fast up */
	"Radf down", /* 11 Radio fast down */
	NULL, /* Reserved */
	NULL, /* Reserved */
	NULL, /* Reserved */
	NULL, /* Reserved */
	NULL, /* Reserved */
	NULL, /* Reserved */
	NULL, /* Reserved */
	NULL, /* Reserved */
	NULL, /* Reserved */
	NULL, /* Reserved */
	NULL, /* Reserved */
	NULL, /* Reserved */
	NULL, /* Reserved */
	NULL, /* Reserved */
	"GD LL up",    /* 26 Global domain low leakage up */
	"GD LL down",  /* 27 Global domain low leakage down */
	"GD1 HS up",   /* 28 Global domain high speed 1 up */
	"GD1 HS up",   /* 29 Global domain high speed 1 up */
	"GD0 HS down", /* 30 Global domain high speed 0 down */
	"GD0 HS down", /* 31 Global domain high speed 0 down */
};

static int log_output_func(uint8_t *buf, size_t size, void *ctx)
{
	if (use_async_uart) {
		int err;
		static uint8_t *tx_buf = (uint8_t *)frame_buf0;

		err = k_sem_take(&uart_sem, K_FOREVER);
		__ASSERT_NO_MSG(err >= 0);

		memcpy(tx_buf, buf, size);

		err = uart_tx(uart_dev, tx_buf, size, SYS_FOREVER_US);
		__ASSERT_NO_MSG(err >= 0);

		tx_buf = (tx_buf == (uint8_t *)frame_buf0) ?
			(uint8_t *)frame_buf1 : (uint8_t *)frame_buf0;
	} else {
		for (int i = 0; i < size; i++) {
			uart_poll_out(uart_dev, buf[i]);
		}
	}

	return size;
}

static uint8_t log_output_buf[CORESIGHT_TRACE_FRAME_SIZE];
LOG_OUTPUT_DEFINE(log_output, log_output_func, log_output_buf, sizeof(log_output_buf));

/** @brief Process a log message. */
static void log_message_process(struct log_frontend_stmesp_demux_log *packet)
{
	uint32_t flags = LOG_OUTPUT_FLAG_COLORS | LOG_OUTPUT_FLAG_LEVEL |
			 LOG_OUTPUT_FLAG_TIMESTAMP | LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	uint64_t ts = packet->timestamp;
	uint8_t level = packet->hdr.level;
	uint16_t plen = packet->hdr.package_len;
	const char *dname = stm_m_name[packet->hdr.major];
	const uint8_t *package = packet->data;
	const char *sname = &packet->data[plen];
	size_t sname_len = strlen(sname) + 1;
	uint16_t dlen = packet->hdr.total_len - (plen + sname_len);
	uint8_t *data = dlen ? &packet->data[plen + sname_len] : NULL;

	log_output_process(&log_output, ts, dname, sname, NULL, level, package, data, dlen, flags);
}

/** @brief Process a trace point message. */
static void trace_point_process(struct log_frontend_stmesp_demux_trace_point *packet)
{
	static const uint32_t flags = LOG_OUTPUT_FLAG_TIMESTAMP | LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	static const char *tp = "%d";
	static const char *tp_d32 = "%d %08x";
	const char *dname = stm_m_name[packet->major];
	static const char *sname = "tp";

	if (packet->has_data) {
		static const union cbprintf_package_hdr desc = {
			.desc = {.len = 4 /* hdr + fmt + id + data */}};
		uint32_t tp_d32_p[] = {(uint32_t)desc.raw, (uint32_t)tp_d32, packet->id,
				       packet->data};

		log_output_process(&log_output, packet->timestamp, dname, sname, NULL, 1,
				   (const uint8_t *)tp_d32_p, NULL, 0, flags);
		return;
	}

	static const union cbprintf_package_hdr desc = {.desc = {.len = 3 /* hdr + fmt + id */}};
	uint32_t tp_p[] = {(uint32_t)desc.raw, (uint32_t)tp, packet->id};

	log_output_process(&log_output, packet->timestamp, dname, sname, NULL,
			   1, (const uint8_t *)tp_p, NULL, 0, flags);
}

/** @brief Process a HW event message. */
static void hw_event_process(struct log_frontend_stmesp_demux_hw_event *packet)
{
	static const uint32_t flags = LOG_OUTPUT_FLAG_TIMESTAMP | LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	static const char *tp = "%s";
	static const char *dname = "hw";
	static const char *sname = "event";
	const char *evt_name = packet->evt < ARRAY_SIZE(hw_evts) ? hw_evts[packet->evt] : "invalid";
	static const union cbprintf_package_hdr desc = {.desc = {.len = 3 /* hdr + fmt + id */}};
	uint32_t tp_p[] = {(uint32_t)desc.raw, (uint32_t)tp, (uint32_t)evt_name};

	log_output_process(&log_output, packet->timestamp, dname, sname, NULL,
			   1, (const uint8_t *)tp_p, NULL, 0, flags);
}

static void message_process(union log_frontend_stmesp_demux_packet packet)
{
	switch (packet.generic_packet->type) {
	case LOG_FRONTEND_STMESP_DEMUX_TYPE_TRACE_POINT:
		trace_point_process(packet.trace_point);
		break;
	case LOG_FRONTEND_STMESP_DEMUX_TYPE_HW_EVENT:
		hw_event_process(packet.hw_event);
		break;
	default:
		log_message_process(packet.log);
		break;
	}
}

/** @brief Function called when potential STPv2 stream data drop is detected.
 *
 * When that occurs all active messages in the demultiplexer are marked as invalid and
 * stp_decoder is switching to re-synchronization mode where data is decoded in
 * search for ASYNC opcode.
 */
static void sync_loss(void)
{
	if (IS_ENABLED(CONFIG_NRF_ETR_DECODE)) {
		mipi_stp_decoder_sync_loss();
		log_frontend_stmesp_demux_reset();
		oosync_cnt++;
		in_sync = false;
	}
}

/** @brief Indicate that STPv2 decoder is synchronized.
 *
 * That occurs when ASYNC opcode is found.
 */
static void on_resync(void)
{
	if (IS_ENABLED(CONFIG_NRF_ETR_DECODE)) {
		in_sync = true;
	}
}

static void decoder_cb_debug(enum mipi_stp_decoder_ctrl_type type,
			     union mipi_stp_decoder_data data,
			     uint64_t *ts, bool marked)
{
	switch (type) {
	case STP_DECODER_MAJOR:
		DBG("M%04x\n", data.id);
		break;
	case STP_DECODER_CHANNEL:
		DBG("C%04x\n", data.id);
		break;
	case STP_DATA8:
		DBG_DATA(data.data, 2, marked);
		if (ts) {
			DBG("TS:%lld\n", *ts);
		}
		break;
	case STP_DATA16:
		DBG_DATA(data.data, 4, marked);
		break;
	case STP_DATA32:
		DBG_DATA(data.data, 8, marked);
		if (ts) {
			DBG("TS:%lld\n", *ts);
		}
		break;
	case STP_DATA64:
		DBG_DATA(data.data, 16, marked);
		break;
	case STP_DECODER_FLAG:
		DBG("F%s\n", ts ? "TS" : "");
		break;
	case STP_DECODER_NULL:
		DBG("NULL\n");
		break;
	case STP_DECODER_MERROR:
		DBG("MERR\n");
		break;
	case STP_DECODER_VERSION:
		DBG("VER\n");
		break;
	case STP_DECODER_FREQ: {
		DBG("FREQ%s %d\n", ts ? "TS" : "", (int)data.freq);
		break;
	}
	case STP_DECODER_GERROR:
		DBG("GERR\n");
		break;
	case STP_DECODER_ASYNC:
		DBG("ASYNC\n");
		break;
	case STP_DECODER_NOT_SUPPORTED:
		DBG("NOTSUP\n");
		break;
	default:
		DBG("OTHER\n");
		break;
	}
}

static void decoder_cb(enum mipi_stp_decoder_ctrl_type type,
		       union mipi_stp_decoder_data data, uint64_t *ts,
		       bool marked)
{
	int rv = 0;

	decoder_cb_debug(type, data, ts, marked);

	if (!IS_ENABLED(CONFIG_NRF_ETR_DECODE)) {
		return;
	}

	switch (type) {
	case STP_DECODER_ASYNC:
		on_resync();
		break;
	case STP_DECODER_MAJOR:
		log_frontend_stmesp_demux_major(data.id);
		break;
	case STP_DECODER_CHANNEL:
		log_frontend_stmesp_demux_channel(data.id);
		break;
	case STP_DATA8:
		if (marked) {
			rv = log_frontend_stmesp_demux_packet_start((uint32_t *)&data.data, ts);
			new_msg_cnt += rv;
		} else {
			log_frontend_stmesp_demux_data((char *)&data.data, 1);
		}
		break;
	case STP_DATA16:
		log_frontend_stmesp_demux_data((char *)&data.data, 2);
		break;
	case STP_DATA32:
		if (marked) {
			rv = log_frontend_stmesp_demux_packet_start((uint32_t *)&data.data, ts);
			new_msg_cnt += rv;
		} else {
			log_frontend_stmesp_demux_data((char *)&data.data, 4);
			if (ts) {
				log_frontend_stmesp_demux_timestamp(*ts);
			}
		}
		break;
	case STP_DATA64:
		log_frontend_stmesp_demux_data((char *)&data.data, 8);
		break;
	case STP_DECODER_FLAG:
		if (ts) {
			log_frontend_stmesp_demux_packet_start(NULL, ts);
		} else {
			log_frontend_stmesp_demux_packet_end();
		}
		new_msg_cnt++;
		break;
	case STP_DECODER_FREQ: {
		static uint32_t freq;
		/* Avoid calling log_output function multiple times as frequency
		 * is sent periodically.
		 */
		if (freq != (uint32_t)data.freq) {
			freq = (uint32_t)data.freq;
			log_output_timestamp_freq_set(freq);
		}
		break;
	}
	case STP_DECODER_MERROR: {
		sync_loss();
		break;
	}
	default:
		break;
	}

	/* Only -ENOMEM is accepted failure. */
	__ASSERT_NO_MSG((rv >= 0) || (rv == -ENOMEM));
}

static void deformatter_cb(uint32_t id, const uint8_t *data, size_t len)
{
	mipi_stp_decoder_decode(data, len);
}

/** @brief Get write index.
 *
 * It is a non-wrapping 32 bit write index. To get actual index in the ETR buffer
 * result must be masked by ETR buffer size mask.
 */
static uint32_t get_wr_idx(void)
{
	uint32_t cnt = nrfx_tbm_count_get();

	if (tbm_full && (cnt < wsize_mask)) {
		/* TBM full event is generated when max value is reached and not when
		 * overflow occurs. We cannot increment base_wr_idx just after the
		 * event but only when counter actually wraps.
		 */
		base_wr_idx += wsize_inc;
		tbm_full = false;
	}

	return cnt + base_wr_idx;
}

/** @brief Get amount of pending data in ETR buffer. */
static uint32_t pending_data(void)
{
	return get_wr_idx() - etr_rd_idx;
}

/** @brief Get current read index.
 *
 * Read index is not exact index in the ETR buffer. It does not wrap (32 bit word).
 * So ETR read index is derived by masking the value by the ETR buffer size mask.
 */
static void rd_idx_inc(void)
{
	etr_rd_idx += CORESIGHT_TRACE_FRAME_SIZE32;
}

/** @brief Process frame. */
static void process_frame(uint8_t *buf, uint32_t pending)
{
	DBG("%d (wr:%d): ", pending, get_wr_idx() & wsize_mask);
	for (int j = 0; j < CORESIGHT_TRACE_FRAME_SIZE; j++) {
		DBG("%02x ", ((uint8_t *)buf)[j]);
	}
	DBG("\n");
	cs_trace_defmt_process((uint8_t *)buf, CORESIGHT_TRACE_FRAME_SIZE);
	DBG("\n");
}

static void process_messages(void)
{
	static union log_frontend_stmesp_demux_packet curr_msg;

	/* Process any new messages. curr_msg remains the same if panic
	 * interrupts currently ongoing processing (curr_msg is not NULL then).
	 * In such a case it is processed once again, which may lead to
	 * a partial repetition of that message on the output.
	 */
	while (new_msg_cnt || curr_msg.generic_packet) {
		if (!curr_msg.generic_packet) {
			curr_msg = log_frontend_stmesp_demux_claim();
		}
		if (curr_msg.generic_packet) {
			message_process(curr_msg);
			log_frontend_stmesp_demux_free(curr_msg);
			curr_msg.generic_packet = NULL;
		} else {
			break;
		}
	}
	new_msg_cnt = 0;
}

/** @brief Dump frame over UART (using polling or async API). */
static void dump_frame(uint8_t *buf)
{
	int err;

	if (use_async_uart) {
		err = k_sem_take(&uart_sem, K_FOREVER);
		__ASSERT_NO_MSG(err >= 0);

		err = uart_tx(uart_dev, buf, CORESIGHT_TRACE_FRAME_SIZE, SYS_FOREVER_US);
		__ASSERT_NO_MSG(err >= 0);
	} else {
		for (int i = 0; i < CORESIGHT_TRACE_FRAME_SIZE; i++) {
			uart_poll_out(uart_dev, buf[i]);
		}
	}
}

/** @brief Attempt to process data pending in the ETR circular buffer.
 *
 * Data is processed in 16 bytes packages. Each package is a STPv2 frame which
 * contain data generated by STM stimulus ports.
 *
 */
static void process(void)
{
	static const uint32_t *const etr_buf = (uint32_t *)(DT_REG_ADDR(ETR_BUFFER_NODE));
	static uint32_t sync_cnt;
	uint32_t pending;

	/* If function is called in panic mode then it may interrupt ongoing
	 * processing. This must be carefully handled as function decodes data
	 * that must be synchronized. Losing synchronization results in failure.
	 *
	 * Special measures are taken to ensure proper synchronization when
	 * processing is preempted by panic.
	 *
	 */
	while ((pending = pending_data()) >= MIN_DATA) {
		/* Do not read the data that has already been read but not yet processed. */
		if (sync_cnt || (CONFIG_NRF_ETR_SYNC_PERIOD == 0)) {
			sync_cnt--;
			sys_cache_data_invd_range((void *)&etr_buf[etr_rd_idx & wsize_mask],
						  CORESIGHT_TRACE_FRAME_SIZE);
			for (int i = 0; i < CORESIGHT_TRACE_FRAME_SIZE32; i++) {
				frame_buf[i] = etr_buf[(etr_rd_idx + i) & wsize_mask];
			}
			rd_idx_inc();
			__sync_synchronize();
		} else {
			sync_cnt = CONFIG_NRF_ETR_SYNC_PERIOD;
			memset(frame_buf, 0xff, CORESIGHT_TRACE_FRAME_SIZE);
		}

		if (IS_ENABLED(CONFIG_NRF_ETR_DECODE) || IS_ENABLED(CONFIG_NRF_ETR_DEBUG)) {
			if ((pending >= (wsize_mask - MIN_DATA)) ||
			    (pending_data() >= (wsize_mask - MIN_DATA))) {
				/* If before or after reading the frame it is close to full
				 * then assume overwrite and sync loss.
				 */
				sync_loss();
			}

			process_frame((uint8_t *)frame_buf, pending);
			if (IS_ENABLED(CONFIG_NRF_ETR_DECODE)) {
				process_messages();
			}
		} else {
			dump_frame((uint8_t *)frame_buf);
			frame_buf = (use_async_uart && (frame_buf == frame_buf0)) ?
						frame_buf1 : frame_buf0;
		}
	}

	/* Fill the buffer to ensure that all logs are processed on time. */
	if (pending < MIN_DATA) {
		log_frontend_stmesp_dummy_write();
	}
}

static int decoder_init(void)
{
	int err;
	static bool once;

	if (once) {
		return -EALREADY;
	}

	once = true;
	if (IS_ENABLED(CONFIG_NRF_ETR_DECODE)) {
		static const struct log_frontend_stmesp_demux_config config = {.m_ids = stm_m_id,
							       .m_ids_cnt = ARRAY_SIZE(stm_m_id)};

		err = log_frontend_stmesp_demux_init(&config);
		if (err < 0) {
			return err;
		}
	}

	static const struct mipi_stp_decoder_config stp_decoder_cfg = {.cb = decoder_cb,
								  .start_out_of_sync = true};

	mipi_stp_decoder_init(&stp_decoder_cfg);

	cs_trace_defmt_init(deformatter_cb);

	return 0;
}

void nrf_etr_flush(void)
{
	int cnt = 4;

	if (IS_ENABLED(CONFIG_NRF_ETR_DECODE) ||
	    IS_ENABLED(CONFIG_NRF_ETR_DEBUG)) {
		(void)decoder_init();
	}

	/* Set flag which forces uart to use blocking polling out instead of
	 * asynchronous API.
	 */
	use_async_uart = false;
	uint32_t k = irq_lock();

	/* Repeat arbitrary number of times to ensure that all that is flushed. */
	while (cnt--) {
		process();
	}

	irq_unlock(k);
}

static void etr_thread_func(void *dummy1, void *dummy2, void *dummy3)
{
	uint64_t checkpoint = 0;

	if (IS_ENABLED(CONFIG_NRF_ETR_DECODE) ||
	    IS_ENABLED(CONFIG_NRF_ETR_DEBUG)) {
		int err;

		err = decoder_init();
		if (err < 0) {
			return;
		}
	}

	while (1) {
		process();

		uint64_t now = k_uptime_get();

		if (DROP_CHECK_PERIOD && (now - checkpoint) > DROP_CHECK_PERIOD) {
			uint32_t cnt = log_frontend_stmesp_demux_get_dropped();

			checkpoint = now;
			if (cnt || oosync_cnt) {
				oosync_cnt = 0;
				LOG_WRN("Too many log messages, some dropped");
			}
		}

		k_sleep(K_MSEC(CONFIG_NRF_ETR_BACKOFF));
	}
}

static void uart_event_handler(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);

	switch (evt->type) {
	case UART_TX_ABORTED:
		/* An intentional fall-through to UART_TX_DONE. */
	case UART_TX_DONE:
		k_sem_give(&uart_sem);
		break;
	default:
		__ASSERT_NO_MSG(0);
	}
}

static void tbm_event_handler(nrf_tbm_event_t event)
{
	ARG_UNUSED(event);

	if (event == NRF_TBM_EVENT_FULL) {
		tbm_full = true;
	}

	k_wakeup(&etr_thread);
}

int etr_process_init(void)
{
	int err;

	k_sem_init(&uart_sem, 1, 1);

	err = uart_callback_set(uart_dev, uart_event_handler, NULL);
	use_async_uart = (err == 0);

	static const nrfx_tbm_config_t config = {.size = wsize_mask};

	nrfx_tbm_init(&config, tbm_event_handler);

	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(tbm)), DT_IRQ(DT_NODELABEL(tbm), priority),
			    nrfx_isr, nrfx_tbm_irq_handler, 0);
	irq_enable(DT_IRQN(DT_NODELABEL(tbm)));

	k_thread_create(&etr_thread, etr_stack, K_KERNEL_STACK_SIZEOF(etr_stack),
			etr_thread_func, NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0,
			K_NO_WAIT);
	k_thread_name_set(&etr_thread, "etr_process");

	return 0;
}

SYS_INIT(etr_process_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
