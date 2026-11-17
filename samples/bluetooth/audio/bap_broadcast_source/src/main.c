/*
 * Copyright (c) 2022-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/usb/usb_buf.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/clock.h>
#include <zephyr/toolchain.h>
#include <zephyr/usb/class/usbd_uac2.h>
#include <zephyr/usb/usbd.h>

#include "chan_map.h"
#include "vs_fatal.h"

#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#define HFCLK_REQUESTED_FREQUENCY MHZ(128)
#endif /* CONFIG_SOC_NRF5340_CPUAPP */

BUILD_ASSERT(strlen(CONFIG_BROADCAST_CODE) <= BT_ISO_BROADCAST_CODE_SIZE, "Invalid broadcast code");
BUILD_ASSERT(IN_RANGE(strlen(CONFIG_BROADCAST_NAME), BT_AUDIO_BROADCAST_NAME_LEN_MIN,
		      BT_AUDIO_BROADCAST_NAME_LEN_MAX),
	     "Invalid broadcast name");

/* When BROADCAST_ENQUEUE_COUNT > 1 we can enqueue enough buffers to ensure that
 * the controller is never idle
 */
#define BROADCAST_ENQUEUE_COUNT 3U
#define TOTAL_BUF_NEEDED        (BROADCAST_ENQUEUE_COUNT * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT)

BUILD_ASSERT(CONFIG_BT_ISO_TX_BUF_COUNT >= TOTAL_BUF_NEEDED,
	     "CONFIG_BT_ISO_TX_BUF_COUNT should be at least "
	     "BROADCAST_ENQUEUE_COUNT * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT");

#if defined(CONFIG_BAP_BROADCAST_8_2_1)

static struct bt_bap_lc3_preset preset_active = BT_BAP_LC3_BROADCAST_PRESET_8_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
	BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

#define BROADCAST_SAMPLE_RATE 8000

#elif defined(CONFIG_BAP_BROADCAST_16_2_1)

static struct bt_bap_lc3_preset preset_active = BT_BAP_LC3_BROADCAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
	BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

#define BROADCAST_SAMPLE_RATE 16000U

#elif defined(CONFIG_BAP_BROADCAST_24_2_1)

static struct bt_bap_lc3_preset preset_active = BT_BAP_LC3_BROADCAST_PRESET_24_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
	BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

#define BROADCAST_SAMPLE_RATE 24000U

#elif defined(CONFIG_BAP_BROADCAST_48_2_1)

static struct bt_bap_lc3_preset preset_active = BT_BAP_LC3_BROADCAST_PRESET_48_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
	BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

#define BROADCAST_SAMPLE_RATE 48000U

#elif defined(CONFIG_BAP_BROADCAST_48_6_1)

static struct bt_bap_lc3_preset preset_active = BT_BAP_LC3_BROADCAST_PRESET_48_6_1(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
	BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

#define BROADCAST_SAMPLE_RATE 48000

#endif

#if defined(CONFIG_BAP_BROADCAST_8_2_1)
#define MAX_SAMPLE_RATE 8000U
#elif defined(CONFIG_BAP_BROADCAST_16_2_1)
#define MAX_SAMPLE_RATE 16000U
#elif defined(CONFIG_BAP_BROADCAST_24_2_1)
#define MAX_SAMPLE_RATE 24000U
#elif defined(CONFIG_BAP_BROADCAST_48_2_1) || defined(CONFIG_BAP_BROADCAST_48_6_1)
#define MAX_SAMPLE_RATE 48000U
#endif
#define MAX_FRAME_DURATION_US 10000U
#define MAX_NUM_SAMPLES       ((MAX_FRAME_DURATION_US * MAX_SAMPLE_RATE) / USEC_PER_SEC)

#if defined(CONFIG_LIBLC3)
#include "lc3.h"

#if defined(CONFIG_AUDIO_INPUT_STREAM)
#include "audio_in.h"
#if defined(CONFIG_AUDIO_INPUT_TIMED) && defined(CONFIG_USE_USB_AUDIO_INPUT)
#include "usb_sof.h"
#endif /* CONFIG_AUDIO_INPUT_TIMED && CONFIG_USE_USB_AUDIO_INPUT */
#endif /* CONFIG_AUDIO_INPUT_STREAM */

#if defined(CONFIG_USE_USB_AUDIO_INPUT)
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_audio.h>

#include <sample_usbd.h>

/* USB Audio Data is downsampled from 48kHz to match broadcast preset when receiving data */
#define USB_SAMPLE_RATE           AUDIO_IN_RATE_HZ
#define USB_FRAME_DURATION_US     1000U
#define USB_SAMPLE_CNT            ((USB_FRAME_DURATION_US * USB_SAMPLE_RATE) / USEC_PER_SEC)
#define USB_BYTES_PER_SAMPLE      2U
#define USB_CHANNELS              2U
#define USB_MONO_FRAME_SIZE       (USB_SAMPLE_CNT * USB_BYTES_PER_SAMPLE)
/* The number of samples received may be USB_SAMPLE_CNT -+ 1 */
#define USB_MAX_MONO_FRAME_SIZE   ((USB_SAMPLE_CNT + 1) * USB_BYTES_PER_SAMPLE)
#define USB_STEREO_FRAME_SIZE     (USB_MONO_FRAME_SIZE * USB_CHANNELS)
#define USB_MAX_STEREO_FRAME_SIZE (USB_MAX_MONO_FRAME_SIZE * USB_CHANNELS)
#endif /* defined(CONFIG_USE_USB_AUDIO_INPUT) */

#if !defined(CONFIG_AUDIO_INPUT_STREAM)

#include <math.h>

#define AUDIO_VOLUME            (INT16_MAX - 3000) /* codec does clipping above INT16_MAX - 3000 */
#define AUDIO_TONE_FREQUENCY_HZ 400U

/**
 * Use the math lib to generate a sine-wave using 16 bit samples into a buffer.
 *
 * @param buf Destination buffer
 * @param length_us Length of the buffer in microseconds
 * @param frequency_hz frequency in Hz
 * @param sample_rate_hz sample-rate in Hz.
 */
static void fill_audio_buf_sin(int16_t *buf, int length_us, int frequency_hz, int sample_rate_hz)
{
	const int sine_period_samples = sample_rate_hz / frequency_hz;
	const unsigned int num_samples = (length_us * sample_rate_hz) / USEC_PER_SEC;
	const float step = 2 * 3.1415f / sine_period_samples;

	for (unsigned int i = 0U; i < num_samples; i++) {
		const float sample = sinf(i * step);

		buf[i] = (int16_t)(AUDIO_VOLUME * sample);
	}
}
#endif /* !defined(CONFIG_AUDIO_INPUT_STREAM) */
#endif /* defined(CONFIG_LIBLC3) */

#define SDU_METRICS_INTERVAL_MS ((int64_t)CONFIG_SDU_METRICS_PRINT_INTERVAL * MSEC_PER_SEC)

/* SDU transmission metrics of one stream over a metrics interval */
struct sdu_tx_metrics {
	/* Sending context */
	uint32_t sent;
	uint32_t bytes;
	uint32_t send_err;
	int32_t in_flight_min;
	int32_t in_flight_max;
	atomic_val_t queue_empty_ref;
	/* Sent callback context; cleared there when cb_clear is set */
	atomic_t cb_clear;
	bool cb_valid;
	uint32_t cb_last;
	uint32_t cb_n;
	uint32_t cb_sum;
	uint32_t cb_min;
	uint32_t cb_max;
};

static struct broadcast_source_stream {
	struct bt_bap_stream stream;
	uint16_t seq_num;
	uint8_t bis_number;
	size_t sent_cnt;
	/* SDUs queued to the controller and not yet completed */
	atomic_t in_flight;
	/* Completions that left the controller queue empty: possible on-air gap */
	atomic_t queue_empty_cnt;
	struct sdu_tx_metrics metrics;

#if defined(CONFIG_LIBLC3)
	lc3_encoder_t lc3_encoder;
#if defined(CONFIG_BAP_BROADCAST_8_2_1)
	lc3_encoder_mem_16k_t lc3_encoder_mem;
#elif defined(CONFIG_BAP_BROADCAST_16_2_1)
	lc3_encoder_mem_16k_t lc3_encoder_mem;
#elif defined(CONFIG_BAP_BROADCAST_24_2_1)
	lc3_encoder_mem_48k_t lc3_encoder_mem;
#elif defined(CONFIG_BAP_BROADCAST_48_2_1) || defined(CONFIG_BAP_BROADCAST_48_6_1)
	lc3_encoder_mem_48k_t lc3_encoder_mem;
#endif
#endif /* defined(CONFIG_LIBLC3) */
} streams[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
static struct bt_bap_broadcast_source *broadcast_source;

NET_BUF_POOL_FIXED_DEFINE(tx_pool, TOTAL_BUF_NEEDED, BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static int16_t send_pcm_data[MAX_NUM_SAMPLES];
static uint16_t seq_num;
static bool stopping;

static K_SEM_DEFINE(sem_started, 0U, 1U);
static K_SEM_DEFINE(sem_stopped, 0U, 1U);

static volatile bool big_active;
static volatile bool conn_adv_active;
static volatile uint8_t acl_conn_count;
static volatile bool scanning;

/* Red blinks while scanning, blue while the BIG is active, green while connected and led3 while
 * connectable advertising is active
 */
#if DT_NODE_EXISTS(DT_ALIAS(led_red))
#define LED_RED_NODE DT_ALIAS(led_red)
#else
#define LED_RED_NODE DT_ALIAS(led0)
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led_green))
#define LED_GREEN_NODE DT_ALIAS(led_green)
#else
#define LED_GREEN_NODE DT_ALIAS(led1)
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led_blue))
#define LED_BLUE_NODE DT_ALIAS(led_blue)
#else
#define LED_BLUE_NODE DT_ALIAS(led2)
#endif
#define LED_ADV_NODE DT_ALIAS(led3)

#define LED_BLINK_INTERVAL K_MSEC(250)

enum led_id {
	LED_RED,
	LED_GREEN,
	LED_BLUE,
	LED_ADV,
	LED_COUNT,
};

static const struct gpio_dt_spec leds[LED_COUNT] = {
	[LED_RED] = GPIO_DT_SPEC_GET_OR(LED_RED_NODE, gpios, {0}),
	[LED_GREEN] = GPIO_DT_SPEC_GET_OR(LED_GREEN_NODE, gpios, {0}),
	[LED_BLUE] = GPIO_DT_SPEC_GET_OR(LED_BLUE_NODE, gpios, {0}),
	[LED_ADV] = GPIO_DT_SPEC_GET_OR(LED_ADV_NODE, gpios, {0}),
};
static bool led_ready[LED_COUNT];
static volatile bool led_blink[LED_COUNT];
static bool led_phase_on;

static void led_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(led_work, led_work_handler);

static void led_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	led_phase_on = !led_phase_on;

	for (size_t i = 0U; i < LED_COUNT; i++) {
		if (led_ready[i]) {
			(void)gpio_pin_set_dt(&leds[i], (int)(led_phase_on && led_blink[i]));
		}
	}

	(void)k_work_reschedule(&led_work, LED_BLINK_INTERVAL);
}

static void leds_update(void)
{
	led_blink[LED_RED] = scanning;
	led_blink[LED_GREEN] = (acl_conn_count > 0U);
	led_blink[LED_BLUE] = big_active;
	led_blink[LED_ADV] = conn_adv_active;
}

static void leds_init(void)
{
	for (size_t i = 0U; i < LED_COUNT; i++) {
		if (leds[i].port != NULL && gpio_is_ready_dt(&leds[i]) &&
		    gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE) == 0) {
			led_ready[i] = true;
		}
	}

	leds_update();
	(void)k_work_reschedule(&led_work, K_NO_WAIT);
}

#define BROADCAST_SOURCE_LIFETIME 120U /* seconds */

#if defined(CONFIG_LIBLC3)
static int freq_hz;
static int frame_duration_us;
static int frames_per_sdu;
static int octets_per_frame;

static K_SEM_DEFINE(lc3_encoder_sem, 0U, TOTAL_BUF_NEEDED);

#if defined(CONFIG_AUDIO_INPUT_STREAM)
/* SDUs that fill the controller queue at start are silence; they are encoded back to back and
 * would otherwise drain the input buffer below its target level.
 */
static atomic_t queue_fill_cnt;
#endif /* CONFIG_AUDIO_INPUT_STREAM */
#endif

static struct net_buf *encode_data(struct broadcast_source_stream *source_stream,
				   const int16_t *pcm)
{
	struct bt_bap_stream *stream = &source_stream->stream;
	struct net_buf *buf;

	if (stopping) {
		return NULL;
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (buf == NULL) {
		printk("Could not allocate buffer when sending on %p\n", stream);
		return NULL;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
#if defined(CONFIG_LIBLC3)
	uint8_t lc3_encoded_buffer[preset_active.qos.sdu];
	int ret;

	if (source_stream->lc3_encoder == NULL) {
		printk("LC3 encoder not setup, cannot encode data.\n");
		net_buf_unref(buf);
		return NULL;
	}

	ret = lc3_encode(source_stream->lc3_encoder, LC3_PCM_FORMAT_S16, pcm, 1,
			 octets_per_frame, lc3_encoded_buffer);
	if (ret == -1) {
		printk("LC3 encoder failed - wrong parameters?: %d", ret);
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, lc3_encoded_buffer, preset_active.qos.sdu);
#else
	net_buf_add_mem(buf, pcm, preset_active.qos.sdu);
#endif /* defined(CONFIG_LIBLC3) */

	return buf;
}

static void send_buf(struct broadcast_source_stream *source_stream, struct net_buf *buf)
{
	struct bt_bap_stream *stream = &source_stream->stream;
	int32_t in_flight;
	int ret;

	ret = bt_bap_stream_send(stream, buf, source_stream->seq_num++);
	if (ret < 0) {
		/* This will end broadcasting on this stream. */
		printk("Unable to broadcast data on %p: %d\n", stream, ret);
		net_buf_unref(buf);
		source_stream->metrics.send_err++;
		return;
	}

	/* SDUs queued in the controller, including this one */
	in_flight = (int32_t)atomic_inc(&source_stream->in_flight) + 1;
	source_stream->sent_cnt++;
	source_stream->metrics.sent++;
	source_stream->metrics.bytes += preset_active.qos.sdu;
	source_stream->metrics.in_flight_min = MIN(source_stream->metrics.in_flight_min, in_flight);
	source_stream->metrics.in_flight_max = MAX(source_stream->metrics.in_flight_max, in_flight);
}

#if defined(CONFIG_LIBLC3)
/* Encode the SDUs of all streams before sending any, so that the SDUs of one SDU interval
 * reach the controller back to back.
 */
static void send_data_all(const int16_t *pcm[])
{
	struct net_buf *bufs[ARRAY_SIZE(streams)];

	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		bufs[i] = encode_data(&streams[i], pcm[i]);
	}

	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		if (bufs[i] != NULL) {
			send_buf(&streams[i], bufs[i]);
		}
	}
}
#else /* !CONFIG_LIBLC3 */
static void send_data(struct broadcast_source_stream *source_stream, const int16_t *pcm)
{
	struct net_buf *buf;

	buf = encode_data(source_stream, pcm);
	if (buf != NULL) {
		send_buf(source_stream, buf);
	}
}
#endif /* !CONFIG_LIBLC3 */

#if defined(CONFIG_AUDIO_INPUT_TIMED)
/* LE Read ISO TX Sync returns the sequence number and BIG anchor point of the last SDU given
 * to the controller. The controller numbers SDUs from 1 and the host from 0, and keeps each SDU
 * on the SDU interval grid, so one anchor gives the air time of every SDU. It is refreshed
 * periodically to keep the sequence number difference small and to check its consistency.
 */
#define TX_SYNC_REFRESH_SDUS 100U
#define TX_SYNC_RETRY_SDUS   10U

static struct tx_sync {
	bool valid;
	uint16_t sn;
	uint32_t ts;
	uint16_t next_seq;
} tx_sync;
static atomic_t tx_sync_reset;

/* TX sync metrics over a metrics interval, encoder context */
static struct tx_sync_metrics {
	uint32_t reads;
	uint32_t fails;
	int32_t dev_min;
	int32_t dev_max;
	int64_t dev_sum;
	uint32_t dev_sdus;
} tx_sync_m = {.dev_min = INT32_MAX, .dev_max = INT32_MIN};

/* Air time, in controller microseconds, of the SDU with host sequence number seq */
static uint32_t tx_sync_time(uint16_t seq)
{
	const int16_t sdus = (int16_t)(uint16_t)(seq + 1U - tx_sync.sn);

	return tx_sync.ts + (uint32_t)((int32_t)sdus * (int32_t)preset_active.qos.interval);
}

/* Encoder thread: the anchor is invalid after the stream (re)starts */
static bool tx_sync_valid(void)
{
	if (atomic_cas(&tx_sync_reset, 1, 0)) {
		tx_sync.valid = false;
		tx_sync.next_seq = 0U;
	}

	return tx_sync.valid;
}

/* Encoder thread, with the host sequence number of the next SDU */
static void tx_sync_update(uint16_t seq)
{
	struct bt_iso_tx_info info;
	int err;

	if ((int16_t)(uint16_t)(seq - tx_sync.next_seq) < 0) {
		return;
	}

	err = bt_bap_stream_get_tx_sync(&streams[0].stream, &info);
	if (err != 0) {
		tx_sync.next_seq = seq + TX_SYNC_RETRY_SDUS;
		tx_sync_m.fails++;
		return;
	}

	tx_sync_m.reads++;
	if (tx_sync.valid) {
		/* Air time against the one predicted from the previous anchor */
		const int32_t dev = (int32_t)(info.ts - tx_sync_time(info.seq_num - 1U));

		tx_sync_m.dev_min = MIN(tx_sync_m.dev_min, dev);
		tx_sync_m.dev_max = MAX(tx_sync_m.dev_max, dev);
		tx_sync_m.dev_sum += dev;
		tx_sync_m.dev_sdus += (uint16_t)(info.seq_num - tx_sync.sn);
	} else {
		printk("TX sync: seq %u ts %u\n", info.seq_num, info.ts);
	}

	tx_sync.valid = true;
	tx_sync.sn = info.seq_num;
	tx_sync.ts = info.ts;
	tx_sync.next_seq = seq + TX_SYNC_REFRESH_SDUS;
}

static void tx_sync_metrics_print(void)
{
	char s1[24];

	if (!tx_sync.valid) {
		printk("  ISO TX sync: no anchor, %u failed\n", tx_sync_m.fails);
	} else if (tx_sync_m.dev_sdus == 0U) {
		printk("  ISO TX sync: anchor seq %u ts %u us, %u reads, %u failed\n", tx_sync.sn,
		       tx_sync.ts, tx_sync_m.reads, tx_sync_m.fails);
	} else {
		/* SDU air times against the nominal SDU interval in the local clock */
		const int64_t cppm = (tx_sync_m.dev_sum * 100000000LL) /
				     ((int64_t)tx_sync_m.dev_sdus * preset_active.qos.interval);

		printk("  ISO TX sync: anchor seq %u ts %u us, %u reads, %u failed, deviation "
		       "%d..%d us over %u SDUs, SDU interval %s ppm\n",
		       tx_sync.sn, tx_sync.ts, tx_sync_m.reads, tx_sync_m.fails, tx_sync_m.dev_min,
		       tx_sync_m.dev_max, tx_sync_m.dev_sdus,
		       audio_in_centi_str(s1, sizeof(s1), cppm));
	}

	tx_sync_m = (struct tx_sync_metrics){.dev_min = INT32_MAX, .dev_max = INT32_MIN};
}
#endif /* CONFIG_AUDIO_INPUT_TIMED */

#if defined(CONFIG_AUDIO_INPUT_TIMED) && defined(CONFIG_USE_USB_AUDIO_INPUT)
/* USB SOF callbacks, and those without a captured SOF time stamp */
static atomic_t usb_sof_cnt;
static atomic_t usb_sof_no_ts_cnt;
#endif /* CONFIG_AUDIO_INPUT_TIMED && CONFIG_USE_USB_AUDIO_INPUT */

static int64_t sdu_metrics_next_report_ms;
static atomic_t sdu_metrics_restart;

static void sdu_metrics_clear(struct sdu_tx_metrics *m)
{
	m->sent = 0U;
	m->bytes = 0U;
	m->send_err = 0U;
	m->in_flight_min = INT32_MAX;
	m->in_flight_max = 0;
}

/* Sent callback context, for each SDU completed by the controller */
static void sdu_metrics_sent(struct sdu_tx_metrics *m)
{
	const uint32_t now = k_cycle_get_32();

	if (atomic_cas(&m->cb_clear, 1, 0)) {
		m->cb_n = 0U;
		m->cb_sum = 0U;
		m->cb_min = UINT32_MAX;
		m->cb_max = 0U;
	}

	if (m->cb_valid) {
		const uint32_t dt = k_cyc_to_us_near32(now - m->cb_last);

		m->cb_n++;
		m->cb_sum += dt;
		m->cb_min = MIN(m->cb_min, dt);
		m->cb_max = MAX(m->cb_max, dt);
	}

	m->cb_valid = true;
	m->cb_last = now;
}

/* Sending context */
static void sdu_metrics_maybe_log(void)
{
	const int64_t now_ms = k_uptime_get();

	if (atomic_cas(&sdu_metrics_restart, 1, 0)) {
		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			struct sdu_tx_metrics *m = &streams[i].metrics;

			sdu_metrics_clear(m);
			m->queue_empty_ref = atomic_get(&streams[i].queue_empty_cnt);
		}

		sdu_metrics_next_report_ms = now_ms + SDU_METRICS_INTERVAL_MS;
		return;
	}

	if (now_ms < sdu_metrics_next_report_ms) {
		return;
	}

	sdu_metrics_next_report_ms = now_ms + SDU_METRICS_INTERVAL_MS;

	printk("SDU transmission metrics (%d s interval)\n", CONFIG_SDU_METRICS_PRINT_INTERVAL);
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		const atomic_val_t queue_empty = atomic_get(&streams[i].queue_empty_cnt);
		struct sdu_tx_metrics *m = &streams[i].metrics;
		/* Read before the sent callback clears them */
		const uint32_t cb_sum = m->cb_sum;
		const uint32_t cb_min = m->cb_min;
		const uint32_t cb_max = m->cb_max;
		const uint32_t cb_n = m->cb_n;
		const int32_t cb_avg = (cb_n > 0U) ? (int32_t)(cb_sum / cb_n) : 0;

		(void)atomic_set(&m->cb_clear, 1);

		printk("  stream[%zu] BIS %u: tx=%u total=%u seq=%u bytes=%u err=%u "
		       "queue_empty=%ld in_flight=%d..%d\n",
		       i, streams[i].bis_number, m->sent, streams[i].sent_cnt, streams[i].seq_num,
		       m->bytes, m->send_err, (long)(queue_empty - m->queue_empty_ref),
		       (m->sent > 0U) ? m->in_flight_min : 0, m->in_flight_max);

		if (cb_n > 0U) {
			printk("  stream[%zu] sent cb: interval %d us, %u..%u us (jitter %d..%+d "
			       "us)\n",
			       i, cb_avg, cb_min, cb_max, (int32_t)cb_min - cb_avg,
			       (int32_t)cb_max - cb_avg);
		}

		m->queue_empty_ref = queue_empty;
		sdu_metrics_clear(m);
	}

#if defined(CONFIG_AUDIO_INPUT_TIMED)
	tx_sync_metrics_print();
#if defined(CONFIG_USE_USB_AUDIO_INPUT)
	printk("  USB SOF capture: %ld SOF, %ld without time stamp\n",
	       (long)atomic_clear(&usb_sof_cnt), (long)atomic_clear(&usb_sof_no_ts_cnt));
#endif /* CONFIG_USE_USB_AUDIO_INPUT */
#endif /* CONFIG_AUDIO_INPUT_TIMED */

#if defined(CONFIG_AUDIO_INPUT_STREAM)
	audio_in_metrics_print();
#endif /* CONFIG_AUDIO_INPUT_STREAM */
}

#if defined(CONFIG_LIBLC3)
static void init_lc3_thread(void *arg1, void *arg2, void *arg3)
{
	const struct bt_audio_codec_cfg *codec_cfg = &preset_active.codec_cfg;
	int ret;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	ret = bt_audio_codec_cfg_get_freq(codec_cfg);
	if (ret > 0) {
		freq_hz = bt_audio_codec_cfg_freq_to_freq_hz(ret);
	} else {
		return;
	}

	ret = bt_audio_codec_cfg_get_frame_dur(codec_cfg);
	if (ret >= 0) {
		frame_duration_us = bt_audio_codec_cfg_frame_dur_to_frame_dur_us(ret);
	} else {
		printk("Error: Frame duration not set, cannot start codec: %d", ret);
		return;
	}

	octets_per_frame = bt_audio_codec_cfg_get_octets_per_frame(codec_cfg);
	frames_per_sdu = bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, true);

	if (freq_hz < 0) {
		printk("Error: Codec frequency not set, cannot start codec.");
		return;
	}

	if (frame_duration_us < 0) {
		printk("Error: Frame duration not set, cannot start codec.");
		return;
	}

	if (octets_per_frame < 0) {
		printk("Error: Octets per frame not set, cannot start codec.");
		return;
	}

#if defined(CONFIG_AUDIO_INPUT_STREAM)
	if ((frame_duration_us * freq_hz) != (MAX_NUM_SAMPLES * USEC_PER_SEC)) {
		printk("Error: Audio input frame size mismatch\n");
		return;
	}
#else
	/* If USB is not used as a sound source, generate a sine wave */
	fill_audio_buf_sin(send_pcm_data, frame_duration_us, AUDIO_TONE_FREQUENCY_HZ, freq_hz);
#endif

	/* Create the encoder instance. This shall complete before stream_started() is called. */
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		printk("Initializing lc3 encoder for stream %zu\n", i);
		streams[i].lc3_encoder = lc3_setup_encoder(frame_duration_us, freq_hz, 0,
							   &streams[i].lc3_encoder_mem);

		if (streams[i].lc3_encoder == NULL) {
			printk("ERROR: Failed to setup LC3 encoder - wrong parameters?\n");
		}
	}

	while (true) {
		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			__maybe_unused int err = k_sem_take(&lc3_encoder_sem, K_FOREVER);

			__ASSERT_NO_MSG(err == 0);
		}
#if defined(CONFIG_AUDIO_INPUT_STREAM)
		static int16_t in_pcm[AUDIO_IN_CHANNELS][MAX_NUM_SAMPLES];

		if ((atomic_get(&queue_fill_cnt) > 0) && (atomic_dec(&queue_fill_cnt) > 0)) {
			(void)memset(in_pcm, 0, sizeof(in_pcm));
		} else {
#if defined(CONFIG_AUDIO_INPUT_TIMED)
			if (tx_sync_valid()) {
				audio_in_get_timed(in_pcm[0], in_pcm[1],
						   tx_sync_time(streams[0].seq_num));
			} else {
				audio_in_get(in_pcm[0], in_pcm[1]);
			}
#else
			audio_in_get(in_pcm[0], in_pcm[1]);
#endif /* CONFIG_AUDIO_INPUT_TIMED */
		}
		const int16_t *pcm[ARRAY_SIZE(streams)];

		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			pcm[i] = in_pcm[MIN(i, AUDIO_IN_CHANNELS - 1U)];
		}
		send_data_all(pcm);
#if defined(CONFIG_AUDIO_INPUT_TIMED)
		/* After sending, so that the HCI command does not delay the SDU */
		tx_sync_update(streams[0].seq_num);
#endif /* CONFIG_AUDIO_INPUT_TIMED */
#else
		const int16_t *pcm[ARRAY_SIZE(streams)];

		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			pcm[i] = send_pcm_data;
		}
		send_data_all(pcm);
#endif /* CONFIG_AUDIO_INPUT_STREAM */
		sdu_metrics_maybe_log();
	}
}

#define LC3_ENCODER_STACK_SIZE (4U * 4096U)
#define LC3_ENCODER_PRIORITY   5

K_THREAD_DEFINE(encoder, LC3_ENCODER_STACK_SIZE, init_lc3_thread, NULL, NULL, NULL,
		LC3_ENCODER_PRIORITY, 0U, -1);

#if defined(CONFIG_USE_USB_AUDIO_INPUT)
/* Allocate 3: 1 for USB to receive data to and 2 additional buffers to prevent out of memory
 * errors when USB host decides to perform rapid terminal enable/disable cycles.
 */
K_MEM_SLAB_DEFINE_STATIC(usb_out_buf_pool, ROUND_UP(USB_MAX_STEREO_FRAME_SIZE, UDC_BUF_ALIGN), 3,
			 UDC_BUF_ALIGN);
static bool terminal_enabled;

static void terminal_update_cb(const struct device *dev, uint8_t terminal, bool enabled,
			       bool microframes, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(terminal);
	ARG_UNUSED(microframes);
	ARG_UNUSED(user_data);

	terminal_enabled = enabled;
	audio_in_flush();
}

static void uac2_sof_cb(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

#if defined(CONFIG_AUDIO_INPUT_TIMED)
	uint32_t t_us;

	atomic_inc(&usb_sof_cnt);
	if (usb_sof_read(&t_us) == 0) {
		audio_in_sof(t_us);
	} else {
		atomic_inc(&usb_sof_no_ts_cnt);
	}
#endif /* CONFIG_AUDIO_INPUT_TIMED */
}

static uint32_t feedback_cb(const struct device *dev, uint8_t terminal, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(terminal);
	ARG_UNUSED(user_data);

	return audio_in_feedback_get();
}

static void *get_recv_buf_cb(const struct device *dev, uint8_t terminal, uint16_t size,
			     void *user_data)
{
	void *buf = NULL;
	int ret;

	ARG_UNUSED(dev);
	ARG_UNUSED(terminal);
	ARG_UNUSED(user_data);

	if (!terminal_enabled) {
		return NULL;
	}

	__ASSERT(size <= USB_MAX_STEREO_FRAME_SIZE, "%u was not <= %d", size,
		 USB_MAX_STEREO_FRAME_SIZE);

	ret = k_mem_slab_alloc(&usb_out_buf_pool, &buf, K_NO_WAIT);
	if (ret != 0) {
		printk("Failed to allocate buffer: %d\n", ret);
	}

	return buf;
}

static void data_recv_cb(const struct device *dev, uint8_t terminal, void *buf, uint16_t size,
			 void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(terminal);
	ARG_UNUSED(user_data);

	if (!terminal_enabled || buf == NULL || size == 0U) {
		k_mem_slab_free(&usb_out_buf_pool, buf);
		return;
	}

	/* 'size' is in bytes, containing 1 ms of 48 kHz stereo 16-bit samples; the sample count
	 * varies around 48 as the host follows the feedback value.
	 */
	audio_in_put((const int16_t *)buf, size / (USB_BYTES_PER_SAMPLE * USB_CHANNELS));

	k_mem_slab_free(&usb_out_buf_pool, buf);
}
#endif /* defined(CONFIG_USE_USB_AUDIO_INPUT) */
#endif /* defined(CONFIG_LIBLC3) */

static void stream_started_cb(struct bt_bap_stream *stream)
{
	struct broadcast_source_stream *source_stream =
		CONTAINER_OF(stream, struct broadcast_source_stream, stream);
	struct bt_iso_info info;
	int err;

	err = bt_iso_chan_get_info(stream->iso, &info);
	__ASSERT(err == 0, "Failed to get ISO chan info: %d", err);

	printk("Stream %p started with BIG_Handle %u and BIS_Number %u\n", stream,
	       info.broadcaster.big_handle, info.broadcaster.bis_number);

	source_stream->seq_num = 0U;
	source_stream->sent_cnt = 0U;
	atomic_clear(&source_stream->in_flight);
	atomic_clear(&source_stream->queue_empty_cnt);
	source_stream->metrics.cb_valid = false;
	(void)atomic_set(&source_stream->metrics.cb_clear, 1);
	(void)atomic_set(&sdu_metrics_restart, 1);
	/* Store the BIS number for logging purposes */
	source_stream->bis_number = info.broadcaster.bis_number;

#if defined(CONFIG_AUDIO_INPUT_TIMED)
	(void)atomic_set(&tx_sync_reset, 1);
#endif /* CONFIG_AUDIO_INPUT_TIMED */
}

static void stream_sent_cb(struct bt_bap_stream *stream)
{
	struct broadcast_source_stream *source_stream =
		CONTAINER_OF(stream, struct broadcast_source_stream, stream);

	/* Also called directly to prime the queue, when nothing is in flight */
	if (atomic_get(&source_stream->in_flight) > 0) {
		sdu_metrics_sent(&source_stream->metrics);

		if ((atomic_dec(&source_stream->in_flight) == 1) && !stopping) {
			atomic_inc(&source_stream->queue_empty_cnt);
		}
	}

#if defined(CONFIG_LIBLC3)
	k_sem_give(&lc3_encoder_sem);
#else
	/* If no LC3 encoder is used, just send mock data directly */
	send_data(source_stream, send_pcm_data);
	sdu_metrics_maybe_log();
#endif
}

static struct bt_bap_stream_ops stream_ops = {
	.started = stream_started_cb,
	.sent = stream_sent_cb,
};

static int setup_broadcast_source(struct bt_bap_broadcast_source **source)
{
	struct bt_bap_broadcast_source_stream_param
		stream_params[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
	struct bt_bap_broadcast_source_subgroup_param
		subgroup_param[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];
	struct bt_bap_broadcast_source_param create_param = {0};
	const size_t streams_per_subgroup = ARRAY_SIZE(stream_params) / ARRAY_SIZE(subgroup_param);
	uint8_t left[] = {BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
					      BT_BYTES_LIST_LE32(BT_AUDIO_LOCATION_FRONT_LEFT))};
	uint8_t right[] = {BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
					       BT_BYTES_LIST_LE32(BT_AUDIO_LOCATION_FRONT_RIGHT))};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(subgroup_param); i++) {
		subgroup_param[i].params_count = streams_per_subgroup;
		subgroup_param[i].params = stream_params + i * streams_per_subgroup;
		subgroup_param[i].codec_cfg = &preset_active.codec_cfg;
	}

	for (size_t j = 0U; j < ARRAY_SIZE(stream_params); j++) {
		stream_params[j].stream = &streams[j].stream;
		stream_params[j].data = j == 0U ? left : right;
		stream_params[j].data_len = j == 0U ? sizeof(left) : sizeof(right);
		bt_bap_stream_cb_register(stream_params[j].stream, &stream_ops);
	}

	create_param.params_count = ARRAY_SIZE(subgroup_param);
	create_param.params = subgroup_param;
	create_param.qos = &preset_active.qos;
	create_param.encryption = strlen(CONFIG_BROADCAST_CODE) > 0;
	create_param.packing = (IS_ENABLED(CONFIG_ISO_PACKING_INTERLEAVED) ?
				BT_ISO_PACKING_INTERLEAVED :
				BT_ISO_PACKING_SEQUENTIAL);

	if (create_param.encryption) {
		memcpy(create_param.broadcast_code, CONFIG_BROADCAST_CODE,
		       strlen(CONFIG_BROADCAST_CODE));
	}

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       ARRAY_SIZE(subgroup_param), ARRAY_SIZE(subgroup_param) * streams_per_subgroup);

	err = bt_bap_broadcast_source_create(&create_param, source);
	if (err != 0) {
		printk("Unable to create broadcast source: %d\n", err);
		return err;
	}

	return 0;
}

static void source_started_cb(struct bt_bap_broadcast_source *source)
{
	printk("Broadcast source %p started\n", source);

	big_active = true;
	leds_update();

	k_sem_give(&sem_started);
}

static void source_stopped_cb(struct bt_bap_broadcast_source *source, uint8_t reason)
{
	printk("Broadcast source %p stopped with reason 0x%02X\n", source, reason);

	big_active = false;
	leds_update();

	k_sem_give(&sem_stopped);
}

#if defined(CONFIG_BT_PERIPHERAL)
/* Two additional advertising sets run in parallel with the LE Audio
 * broadcast so that a remote central can connect/disconnect repeatedly
 * without interrupting the broadcast stream:
 *   - conn_ext_adv:    connectable extended advertising
 *   - conn_legacy_adv: connectable legacy advertising
 * Each advertising set uses a distinct device name in its advertising
 * data so the peer can distinguish between them.
 */
static struct bt_le_ext_adv *conn_ext_adv;
static struct bt_le_ext_adv *conn_legacy_adv;

#define CONN_EXT_ADV_NAME    "BAP Ext Connectable"
#define CONN_LEGACY_ADV_NAME "BAP Legacy Connectable"

static const struct bt_data conn_ext_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONN_EXT_ADV_NAME, sizeof(CONN_EXT_ADV_NAME) - 1),
};

static const struct bt_data conn_legacy_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONN_LEGACY_ADV_NAME,
		sizeof(CONN_LEGACY_ADV_NAME) - 1),
};

/* Extended connectable advertising parameters (undirected, extended PDUs). */
#define BT_LE_ADV_CONN_EXT                                                                         \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_EXT_ADV,                                \
			BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL)

/* Connectable advertising is enabled or disabled by sw0, it is off at boot. While enabled it runs
 * only when there is no ACL connection, and resumes on disconnect.
 */
static volatile bool conn_adv_enabled;

static int conn_adv_set_update(struct bt_le_ext_adv *adv, bool start, const char *name)
{
	int err;

	if (adv == NULL) {
		return -EINVAL;
	}

	if (!start) {
		err = bt_le_ext_adv_stop(adv);
		if (err != 0) {
			printk("Failed to stop connectable %s advertising (err %d)\n", name, err);
		}

		return err;
	}

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err == -EALREADY) {
		return 0;
	}

	if (err != 0) {
		printk("Failed to start connectable %s advertising (err %d)\n", name, err);
	} else {
		printk("Connectable %s advertising started\n", name);
	}

	return err;
}

/* The advertising sets are created on first enable and started right after their data is
 * set, the Controller releases superseded advertising PDUs only while a set is advertising.
 */
static int conn_adv_create(void)
{
	int err;

	/* Connectable extended advertising set. */
	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_EXT, NULL, &conn_ext_adv);
	if (err != 0) {
		printk("Failed to create connectable extended adv set (err %d)\n", err);
		return err;
	}

	err = bt_le_ext_adv_set_data(conn_ext_adv, conn_ext_ad, ARRAY_SIZE(conn_ext_ad),
				     NULL, 0);
	if (err != 0) {
		printk("Failed to set connectable extended adv data (err %d)\n", err);
		return err;
	}

	/* Connectable legacy advertising set. */
	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_FAST_1, NULL, &conn_legacy_adv);
	if (err != 0) {
		printk("Failed to create connectable legacy adv set (err %d)\n", err);
		return err;
	}

	err = bt_le_ext_adv_set_data(conn_legacy_adv, conn_legacy_ad,
				     ARRAY_SIZE(conn_legacy_ad), NULL, 0);
	if (err != 0) {
		printk("Failed to set connectable legacy adv data (err %d)\n", err);
		return err;
	}

	return 0;
}

static void conn_adv_update_work_handler(struct k_work *work)
{
	const bool connected = (acl_conn_count > 0U);
	const bool start = (conn_adv_enabled && !connected);
	int err_legacy;
	int err_ext;

	ARG_UNUSED(work);

	if (start && (conn_ext_adv == NULL) && (conn_legacy_adv == NULL)) {
		(void)conn_adv_create();
	}

	err_ext = conn_adv_set_update(conn_ext_adv, start, "extended");
	err_legacy = conn_adv_set_update(conn_legacy_adv, start, "legacy");

	conn_adv_active = (start && ((err_ext == 0) || (err_legacy == 0)));
	leds_update();
}

static K_WORK_DEFINE(conn_adv_update_work, conn_adv_update_work_handler);

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	if (err != 0U) {
		printk("Failed to connect to %s %u %s\n", bt_conn_dst_str(conn),
		       err, bt_hci_err_to_str(err));
		/* Advertising sets that failed to establish a connection stop;
		 * re-start them so the central can retry.
		 */
		(void)k_work_submit(&conn_adv_update_work);
		return;
	}

	printk("Connected: %s\n", bt_conn_dst_str(conn));
	acl_conn_count++;

	/* Stop the advertising set that did not produce the connection */
	(void)k_work_submit(&conn_adv_update_work);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected: %s, reason 0x%02x %s\n", bt_conn_dst_str(conn),
	       reason, bt_hci_err_to_str(reason));
	if (acl_conn_count > 0U) {
		acl_conn_count--;
	}

	/* Resume connectable advertising, if enabled, so the central can reconnect */
	(void)k_work_submit(&conn_adv_update_work);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_OBSERVER)
/* Continuous active scanning, the 1M and the Coded PHY scan windows together cover the scan
 * interval. Auxiliary channel receptions provide the Controller channel metrics used to update the
 * Broadcast ISO channel map. No duplicate filtering, so that every auxiliary PDU is received.
 */
static volatile bool scan_user = IS_ENABLED(CONFIG_SCAN_BACKGROUND);

#define NAME_LEN 128U

static bool data_name_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
	case BT_DATA_BROADCAST_NAME:
		len = MIN(data->data_len, NAME_LEN - 1);
		(void)memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case BT_GAP_LE_PHY_NONE: return "No packets";
	case BT_GAP_LE_PHY_1M: return "LE 1M";
	case BT_GAP_LE_PHY_2M: return "LE 2M";
	case BT_GAP_LE_PHY_CODED: return "LE Coded";
	default: return "Unknown";
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct net_buf_simple buf;
	char name[NAME_LEN];
	uint8_t data_status;
	uint16_t data_len;

	(void)memset(name, 0, sizeof(name));

	data_len = ad->len;
	net_buf_simple_clone(ad, &buf);
	bt_data_parse(&buf, data_name_cb, name);

	data_status = BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info->adv_props);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	printk("[DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i "
	       "Data status: %u, AD data len: %u Name: %s "
	       "C:%u S:%u D:%u SR:%u E:%u Pri PHY: %s, Sec PHY: %s, "
	       "Interval: 0x%04x (%u ms), SID: %u\n",
	       le_addr, info->adv_type, info->tx_power, info->rssi,
	       data_status, data_len, name,
	       (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0,
	       phy2str(info->primary_phy), phy2str(info->secondary_phy),
	       info->interval, info->interval * 5 / 4, info->sid);
}

static struct bt_le_scan_cb scan_cb = {
	.recv = scan_recv,
};

static int scan_start(void)
{
	struct bt_le_scan_param scan_param = {
		.type           = BT_LE_SCAN_TYPE_ACTIVE,
		.options        = BT_LE_SCAN_OPT_CODED,
		.interval       = BT_GAP_SCAN_FAST_INTERVAL_MIN,
		.window         = BT_GAP_SCAN_FAST_WINDOW,
		.interval_coded = BT_GAP_SCAN_FAST_INTERVAL_MIN,
		.window_coded   = BT_GAP_SCAN_FAST_WINDOW,
	};
	int err;

	if (scanning) {
		return 0;
	}

	err = bt_le_scan_start(&scan_param, NULL);
	if (err != 0 && err != -EALREADY) {
		printk("Unable to start scan: %d\n", err);
		return err;
	}

	scanning = true;
	leds_update();

	return 0;
}

static int scan_stop(void)
{
	int err;

	err = bt_le_scan_stop();
	if (err != 0 && err != -EALREADY) {
		return err;
	}

	scanning = false;
	leds_update();

	return 0;
}

/* Called from the system workqueue to apply the user scan state */
static void scan_update(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	if (scan_user) {
		if (!scanning) {
			err = scan_start();
			if (err == 0) {
				printk("Scanning started\n");
			}
		}
	} else if (scanning) {
		err = scan_stop();
		if (err != 0) {
			printk("Failed to stop scanning: %d\n", err);
		} else {
			printk("Scanning stopped\n");
		}
	}
}

static K_WORK_DEFINE(scan_update_work, scan_update);

/* Called from any thread, the system workqueue starts or stops scanning */
static void scan_user_set(bool enable)
{
	scan_user = enable;

	if (enable) {
		printk("Scanning enabled\n");
	} else {
		printk("Scanning disabled\n");
	}

	(void)k_work_submit(&scan_update_work);
}
#endif /* CONFIG_BT_OBSERVER */

/* sw0 enables or disables connectable advertising, sw2 enables or disables scanning */
#define BUTTON_DEBOUNCE K_MSEC(30)

enum button_id {
	BUTTON_ADV,
	BUTTON_SCAN,
	BUTTON_COUNT,
};

static const struct gpio_dt_spec buttons[BUTTON_COUNT] = {
	[BUTTON_ADV] = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0}),
	[BUTTON_SCAN] = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw2), gpios, {0}),
};
static struct gpio_callback button_cb_data[BUTTON_COUNT];
static struct k_work_delayable button_work[BUTTON_COUNT];

static void button_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	const size_t id = (size_t)(dwork - &button_work[0]);

	if (gpio_pin_get_dt(&buttons[id]) <= 0) {
		return;
	}

	switch (id) {
#if defined(CONFIG_BT_PERIPHERAL)
	case BUTTON_ADV:
		conn_adv_enabled = !conn_adv_enabled;
		printk("Connectable advertising %s\n",
		       conn_adv_enabled ? "enabled" : "disabled");
		(void)k_work_submit(&conn_adv_update_work);
		break;
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_OBSERVER)
	case BUTTON_SCAN:
		scan_user_set(!scanning);
		break;
#endif /* CONFIG_BT_OBSERVER */
	default:
		break;
	}
}

static void button_isr(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	const size_t id = (size_t)(cb - &button_cb_data[0]);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	(void)k_work_reschedule(&button_work[id], BUTTON_DEBOUNCE);
}

static void button_init(enum button_id id)
{
	const struct gpio_dt_spec *btn = &buttons[id];
	int err;

	if (btn->port == NULL || !gpio_is_ready_dt(btn)) {
		return;
	}

	err = gpio_pin_configure_dt(btn, GPIO_INPUT);
	if (err != 0) {
		printk("Failed to configure button %d: %d\n", id, err);
		return;
	}

	k_work_init_delayable(&button_work[id], button_work_handler);
	gpio_init_callback(&button_cb_data[id], button_isr, BIT(btn->pin));

	err = gpio_add_callback_dt(btn, &button_cb_data[id]);
	if (err != 0) {
		printk("Failed to add button %d callback: %d\n", id, err);
		return;
	}

	err = gpio_pin_interrupt_configure_dt(btn, GPIO_INT_EDGE_TO_ACTIVE);
	if (err != 0) {
		printk("Failed to configure button %d interrupt: %d\n", id, err);
	}
}

static void buttons_init(void)
{
	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		printk("Press button 0 to toggle connectable advertising\n");
		button_init(BUTTON_ADV);
	}

	if (IS_ENABLED(CONFIG_BT_OBSERVER)) {
		printk("Press button 2 to toggle scanning\n");
		button_init(BUTTON_SCAN);
	}
}

#if defined(CONFIG_BT_HCI_VS_EVT_USER)
/* Delegate the vendor-specific events to their handlers, each handler is given the event from
 * its start.
 */
static bool vnd_evt_cb(struct net_buf_simple *buf)
{
	struct net_buf_simple_state state;

	net_buf_simple_save(buf, &state);

	if (IS_ENABLED(CONFIG_CHAN_MAP_UPDATE) && chan_map_vnd_evt(buf)) {
		return true;
	}
	net_buf_simple_restore(buf, &state);

	if (IS_ENABLED(CONFIG_VS_FATAL_PRINT) && vs_fatal_vnd_evt(buf)) {
		return true;
	}
	net_buf_simple_restore(buf, &state);

	return false;
}
#endif /* CONFIG_BT_HCI_VS_EVT_USER */

int main(void)
{
	static struct bt_bap_broadcast_source_cb broadcast_source_cb = {
		.started = source_started_cb,
		.stopped = source_stopped_cb,
	};
	struct bt_le_ext_adv *adv;
	int err;

#if defined(CONFIG_SOC_NRF5340_CPUAPP)
	/* Use this to turn on 128 MHz clock for the nRF5340 cpu_app */
	err = clock_control_set_rate(DEVICE_DT_GET_ONE(nordic_nrf_clock_hfclk), NULL,
				     (clock_control_subsys_rate_t)HFCLK_REQUESTED_FREQUENCY);
	if (err != 0) {
		printk("Failed to set 128 MHz (err %d)\n", err);
	}
#endif /* CONFIG_SOC_NRF5340_CPUAPP */

	leds_init();

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}
	printk("Bluetooth initialized\n");

	err = bt_bap_broadcast_source_register_cb(&broadcast_source_cb);
	if (err != 0) {
		printk("Failed to register broadcast source callbacks (err %d)\n", err);
		return 0;
	}

#if defined(CONFIG_BT_PERIPHERAL)
	/* Connectable advertising sets run alongside the LE Audio broadcast,
	 * so a remote central can connect and disconnect repeatedly while the
	 * broadcast continues.
	 */
	printk("Connectable advertising as \"%s\" and \"%s\" is off\n", CONN_EXT_ADV_NAME,
	       CONN_LEGACY_ADV_NAME);

	if (IS_ENABLED(CONFIG_CHAN_MAP_UPDATE)) {
		chan_map_init();
	}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_HCI_VS_EVT_USER)
	err = bt_hci_register_vnd_evt_cb(vnd_evt_cb);
	if (err != 0) {
		printk("Vendor-specific event callback register failed (err %d)\n", err);
		return 0;
	}
#endif /* CONFIG_BT_HCI_VS_EVT_USER */

	buttons_init();

#if defined(CONFIG_BT_OBSERVER)
	bt_le_scan_cb_register(&scan_cb);

	if (scan_user) {
		(void)k_work_submit(&scan_update_work);
	}
#endif /* CONFIG_BT_OBSERVER */

	for (size_t i = 0U; i < ARRAY_SIZE(send_pcm_data); i++) {
		/* Initialize mock data */
		send_pcm_data[i] = i;
	}

#if defined(CONFIG_LIBLC3)
#if defined(CONFIG_USE_USB_AUDIO_INPUT)
	const struct device *broadcaster_dev = DEVICE_DT_GET(DT_NODELABEL(uac2_broadcaster));
	static struct uac2_ops usb_audio_ops = {
		.terminal_update_cb = terminal_update_cb,
		.get_recv_buf = get_recv_buf_cb,
		.data_recv_cb = data_recv_cb,
		.sof_cb = uac2_sof_cb,
		.feedback_cb = feedback_cb,
	};
	struct usbd_context *sample_usbd;

	if (!device_is_ready(broadcaster_dev)) {
		printk("Cannot get USB Broadcaster Device\n");
		return -EIO;
	}

	printk("Found USB Headset Device\n");

	(void)memset(streams, 0, sizeof(streams));

	err = audio_in_init(BROADCAST_SAMPLE_RATE, MAX_NUM_SAMPLES);
	if (err != 0) {
		printk("Failed to init audio input: %d\n", err);
		return err;
	}

#if defined(CONFIG_AUDIO_INPUT_TIMED)
	err = usb_sof_init();
	if (err != 0) {
		printk("USB SOF time stamps unavailable (err %d)\n", err);
	}
#endif /* CONFIG_AUDIO_INPUT_TIMED */

	usbd_uac2_set_ops(broadcaster_dev, &usb_audio_ops, NULL);

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		printk("Failed to init USBD device: %d\n", err);
		return -ENODEV;
	}

	err = usbd_enable(sample_usbd);
	if (err != 0) {
		printk("Failed to enable USBD: %d\n", err);
		return err;
	}

	printk("USB initialized\n");

#elif defined(CONFIG_AUDIO_INPUT_EMULATED)
	err = audio_in_init(BROADCAST_SAMPLE_RATE, MAX_NUM_SAMPLES);
	if (err != 0) {
		printk("Failed to init audio input: %d\n", err);
		return err;
	}

	audio_in_emulated_start();
#endif /* defined(CONFIG_USE_USB_AUDIO_INPUT) */
	k_thread_start(encoder);
#endif /* defined(CONFIG_LIBLC3) */

	while (true) {
		/* Broadcast Audio Streaming Endpoint advertising data */
		NET_BUF_SIMPLE_DEFINE(ad_buf, BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
		NET_BUF_SIMPLE_DEFINE(base_buf, 128);
		struct bt_data ext_ad[3];
		struct bt_data per_ad;
		uint32_t broadcast_id;

		/* Create a connectable advertising set */
		err = bt_le_ext_adv_create(BT_BAP_ADV_PARAM_BROADCAST_FAST, NULL, &adv);
		if (err != 0) {
			printk("Unable to create extended advertising set: %d\n", err);
			return 0;
		}

		/* Set periodic advertising parameters */
		err = bt_le_per_adv_set_param(adv, BT_BAP_PER_ADV_PARAM_BROADCAST_FAST);
		if (err != 0) {
			printk("Failed to set periodic advertising parameters (err %d)\n", err);
			return 0;
		}

		printk("Creating broadcast source\n");
		err = setup_broadcast_source(&broadcast_source);
		if (err != 0) {
			printk("Unable to setup broadcast source: %d\n", err);
			return 0;
		}

#if defined(CONFIG_STATIC_BROADCAST_ID)
		broadcast_id = CONFIG_BROADCAST_ID;
#else
		err = bt_rand(&broadcast_id, BT_AUDIO_BROADCAST_ID_SIZE);
		if (err != 0) {
			printk("Unable to generate broadcast ID: %d\n", err);
			return err;
		}
#endif /* CONFIG_STATIC_BROADCAST_ID */

		/* Setup extended advertising data */
		net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
		net_buf_simple_add_le24(&ad_buf, broadcast_id);
		ext_ad[0].type = BT_DATA_SVC_DATA16;
		ext_ad[0].data_len = ad_buf.len;
		ext_ad[0].data = ad_buf.data;
		ext_ad[1] = (struct bt_data)BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
						    sizeof(CONFIG_BT_DEVICE_NAME) - 1);
		/* Broadcast name used for scanning device that displays information on the */
		/* available broadcast sources. */
		ext_ad[2] = (struct bt_data)BT_DATA(BT_DATA_BROADCAST_NAME, CONFIG_BROADCAST_NAME,
						    sizeof(CONFIG_BROADCAST_NAME) - 1);
		err = bt_le_ext_adv_set_data(adv, ext_ad, 3, NULL, 0);
		if (err != 0) {
			printk("Failed to set extended advertising data: %d\n", err);
			return 0;
		}

		/* Setup periodic advertising data */
		err = bt_bap_broadcast_source_get_base(broadcast_source, &base_buf);
		if (err != 0) {
			printk("Failed to get encoded BASE: %d\n", err);
			return 0;
		}

		per_ad.type = BT_DATA_SVC_DATA16;
		per_ad.data_len = base_buf.len;
		per_ad.data = base_buf.data;
		err = bt_le_per_adv_set_data(adv, &per_ad, 1);
		if (err != 0) {
			printk("Failed to set periodic advertising data: %d\n", err);
			return 0;
		}

		/* Start extended advertising */
		err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
		if (err != 0) {
			printk("Failed to start extended advertising: %d\n", err);
			return 0;
		}

		/* Enable Periodic Advertising */
		err = bt_le_per_adv_start(adv);
		if (err != 0) {
			printk("Failed to enable periodic advertising: %d\n", err);
			return 0;
		}

		printk("Starting broadcast source\n");
		stopping = false;
		err = bt_bap_broadcast_source_start(broadcast_source, adv);
		if (err != 0) {
			printk("Unable to start broadcast source: %d\n", err);
			return 0;
		}

		/* Wait for broadcast source to be started */
		err = k_sem_take(&sem_started, K_FOREVER);
		__ASSERT_NO_MSG(err == 0);

		printk("Broadcast source started\n");

		/* Initialize sending */
#if defined(CONFIG_AUDIO_INPUT_STREAM)
		atomic_set(&queue_fill_cnt, BROADCAST_ENQUEUE_COUNT);
#endif /* CONFIG_AUDIO_INPUT_STREAM */
		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			for (unsigned int j = 0U; j < BROADCAST_ENQUEUE_COUNT; j++) {
				stream_sent_cb(&streams[i].stream);
			}
		}

#if defined(CONFIG_LIBLC3) && defined(CONFIG_AUDIO_INPUT_STREAM)
		/* Never stop streaming when using USB Audio as input */
		k_sleep(K_FOREVER);
#endif /* defined(CONFIG_LIBLC3) && defined(CONFIG_AUDIO_INPUT_STREAM) */
		printk("Waiting %u seconds before stopped\n", BROADCAST_SOURCE_LIFETIME);
		k_sleep(K_SECONDS(BROADCAST_SOURCE_LIFETIME));
		printk("Stopping broadcast source\n");
		stopping = true;
		err = bt_bap_broadcast_source_stop(broadcast_source);
		if (err != 0) {
			printk("Unable to stop broadcast source: %d\n", err);
			return 0;
		}

		/* Wait for broadcast source to be stopped */
		err = k_sem_take(&sem_stopped, K_FOREVER);
		__ASSERT_NO_MSG(err == 0);

		printk("Broadcast source stopped\n");

		printk("Deleting broadcast source\n");
		err = bt_bap_broadcast_source_delete(broadcast_source);
		if (err != 0) {
			printk("Unable to delete broadcast source: %d\n", err);
			return 0;
		}
		printk("Broadcast source deleted\n");
		broadcast_source = NULL;
		seq_num = 0U;

		err = bt_le_per_adv_stop(adv);
		if (err != 0) {
			printk("Failed to stop periodic advertising (err %d)\n", err);
			return 0;
		}

		err = bt_le_ext_adv_stop(adv);
		if (err != 0) {
			printk("Failed to stop extended advertising (err %d)\n", err);
			return 0;
		}

		err = bt_le_ext_adv_delete(adv);
		if (err != 0) {
			printk("Failed to delete extended advertising (err %d)\n", err);
			return 0;
		}
	}
	return 0;
}
