/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Audio input path between an asynchronous 48 kHz stereo producer (USB host) and the ISO
 * stream consumer. The ISO stream is the clock master: the consumer takes one SDU frame per
 * channel each time the controller completes an SDU. The buffer fill level, sampled just
 * before each take, integrates the rate difference between producer and consumer and drives
 * a PI loop that sets the UAC2 explicit feedback value. If the host does not follow the
 * feedback, a single sample is dropped or repeated per SDU to keep the buffer within bounds.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>

#include "audio_in.h"

#define FRAME_SAMPLES_MAX 480U /* 10 ms at 48 kHz */
#define RING_FRAMES       6U
#define RING_BYTES        (FRAME_SAMPLES_MAX * RING_FRAMES * sizeof(int16_t))
#define PUT_CHUNK_MAX     64U

/* Fill level (per channel, in SDU frames) before a take: 2 frames leaves one frame of margin
 * for producer delivery jitter after the take. Samples are dropped above 3.5 frames, which
 * tolerates late consumer wakeups, and repeated below 1.5 frames.
 */
#define TARGET_FRAMES 2U

/* PI gains, error in 48 kHz samples, output in Q10.14 LSB (1 LSB ~ 1.27 ppm).
 * Kp gives ~4 s time constant, Ki gives a damping ratio of ~0.7.
 */
#define PI_KP        4.0f
#define PI_KI        0.5f
#define FB_LIMIT_PPM 1000

#define STATS_PERIOD_S 10U

static struct ring_buf rings[AUDIO_IN_CHANNELS];
static uint8_t ring_mem[AUDIO_IN_CHANNELS][RING_BYTES];

static uint32_t ratio;
static size_t frame_n;
static uint32_t sdus_per_sec;
static uint32_t target;
static uint32_t slip_above;
static uint32_t repeat_below;

static atomic_t feedback = ATOMIC_INIT(AUDIO_IN_FB_NOMINAL);
static atomic_t flush_req;

/* Producer state */
static int32_t dec_acc[AUDIO_IN_CHANNELS];
static uint32_t dec_cnt;
static atomic_t overruns;

/* Consumer state */
static bool primed;
static float pi_integral;
static uint32_t period_cnt;
static uint32_t sec_cnt;
static uint64_t fill_sum;
static uint32_t fill_sum_n;
static uint32_t fill_min = UINT32_MAX;
static uint32_t fill_max;
static uint32_t underruns;
static uint32_t slips;
static uint32_t repeats;
static uint32_t primes;

int audio_in_init(uint32_t rate_hz, size_t frame_samples)
{
	if ((rate_hz == 0U) || ((AUDIO_IN_RATE_HZ % rate_hz) != 0U) || (frame_samples < 2U) ||
	    (frame_samples > FRAME_SAMPLES_MAX) || ((rate_hz % frame_samples) != 0U)) {
		return -EINVAL;
	}

	ratio = AUDIO_IN_RATE_HZ / rate_hz;
	frame_n = frame_samples;
	sdus_per_sec = rate_hz / frame_samples;
	target = TARGET_FRAMES * frame_samples;
	slip_above = target + frame_samples + (frame_samples / 2U);
	repeat_below = target - (frame_samples / 2U);

	for (size_t i = 0U; i < AUDIO_IN_CHANNELS; i++) {
		ring_buf_init(&rings[i], sizeof(ring_mem[i]), ring_mem[i]);
	}

	printk("Audio in: ratio %u, frame %zu, target %u, capacity %u samples\n", ratio, frame_n,
	       target, ring_buf_capacity_get(&rings[0]) / (uint32_t)sizeof(int16_t));

	return 0;
}

void audio_in_put(const int16_t *pcm, size_t frames)
{
	int16_t out[AUDIO_IN_CHANNELS][PUT_CHUNK_MAX];

	while (frames > 0U) {
		const size_t chunk = MIN(frames, PUT_CHUNK_MAX);
		uint32_t bytes;
		size_t n = 0U;

		/* Phase continuous boxcar decimation; chunks need not be a multiple of ratio */
		for (size_t i = 0U; i < chunk; i++) {
			for (size_t ch = 0U; ch < AUDIO_IN_CHANNELS; ch++) {
				dec_acc[ch] += pcm[(i * AUDIO_IN_CHANNELS) + ch];
			}

			dec_cnt++;
			if (dec_cnt == ratio) {
				for (size_t ch = 0U; ch < AUDIO_IN_CHANNELS; ch++) {
					out[ch][n] = (int16_t)(dec_acc[ch] / (int32_t)ratio);
					dec_acc[ch] = 0;
				}
				dec_cnt = 0U;
				n++;
			}
		}

		pcm += chunk * AUDIO_IN_CHANNELS;
		frames -= chunk;

		if (n == 0U) {
			continue;
		}

		/* Keep channels aligned: write all or nothing */
		bytes = n * sizeof(int16_t);
		if ((ring_buf_space_get(&rings[0]) < bytes) ||
		    (ring_buf_space_get(&rings[1]) < bytes)) {
			atomic_inc(&overruns);
			continue;
		}

		for (size_t ch = 0U; ch < AUDIO_IN_CHANNELS; ch++) {
			(void)ring_buf_put(&rings[ch], (uint8_t *)out[ch], bytes);
		}
	}
}

static uint32_t fill_get(void)
{
	return MIN(ring_buf_size_get(&rings[0]), ring_buf_size_get(&rings[1])) / sizeof(int16_t);
}

static void discard(uint32_t samples)
{
	for (size_t ch = 0U; ch < AUDIO_IN_CHANNELS; ch++) {
		ring_buf_consume(&rings[ch], MIN(samples * sizeof(int16_t),
						 ring_buf_size_get(&rings[ch])));
	}
}

static void control_update(void)
{
	const float fb_limit = (float)AUDIO_IN_FB_NOMINAL * FB_LIMIT_PPM / 1000000.0f;
	float err;
	float fb;

	if ((fill_sum_n == 0U) || !primed) {
		fill_sum = 0U;
		fill_sum_n = 0U;
		return;
	}

	/* Error in 48 kHz samples */
	err = (((float)fill_sum / (float)fill_sum_n) - (float)target) * (float)ratio;
	fill_sum = 0U;
	fill_sum_n = 0U;

	pi_integral = CLAMP(pi_integral + (PI_KI * err), -fb_limit, fb_limit);
	fb = CLAMP(-(PI_KP * err) - pi_integral, -fb_limit, fb_limit);

	atomic_set(&feedback, (atomic_val_t)((int32_t)AUDIO_IN_FB_NOMINAL + (int32_t)fb));
}

static void stats_print(void)
{
	const int32_t fb = (int32_t)atomic_get(&feedback);
	const int32_t ppm = ((fb - (int32_t)AUDIO_IN_FB_NOMINAL) * 1000000) /
			    (int32_t)AUDIO_IN_FB_NOMINAL;

	printk("Audio in: fb %d (%d ppm) fill %u..%u target %u primed %u underrun %u "
	       "overrun %u slip %u repeat %u\n",
	       fb, ppm, fill_min, fill_max, target, primes, underruns,
	       (uint32_t)atomic_get(&overruns), slips, repeats);

	fill_min = UINT32_MAX;
	fill_max = 0U;
}

void audio_in_get(int16_t *left, int16_t *right)
{
	int16_t *out[AUDIO_IN_CHANNELS] = {left, right};
	uint32_t fill;
	size_t n = frame_n;

	if (atomic_cas(&flush_req, 1, 0)) {
		discard(fill_get());
		primed = false;
	}

	fill = fill_get();

	if (!primed && (fill >= target)) {
		/* The producer may run before the ISO stream starts and the first takes are not
		 * yet at their steady phase; start in the middle of the slip/repeat band and let
		 * the control loop settle on the target.
		 */
		const uint32_t start = (slip_above + repeat_below) / 2U;

		if (fill > start) {
			discard(fill - start);
			fill = start;
		}
		primed = true;
		primes++;
	}

	fill_min = MIN(fill_min, fill);
	fill_max = MAX(fill_max, fill);

	if (!primed) {
		goto silence;
	}

	if (fill < frame_n) {
		underruns++;
		primed = false;
		goto silence;
	}

	fill_sum += fill;
	fill_sum_n++;

	if (fill > slip_above) {
		discard(1U);
		slips++;
	} else if (fill < repeat_below) {
		n--;
		repeats++;
	}

	for (size_t ch = 0U; ch < AUDIO_IN_CHANNELS; ch++) {
		(void)ring_buf_get(&rings[ch], (uint8_t *)out[ch], n * sizeof(int16_t));
		if (n < frame_n) {
			out[ch][n] = out[ch][n - 1U];
		}
	}

	goto account;

silence:
	(void)memset(left, 0, frame_n * sizeof(int16_t));
	(void)memset(right, 0, frame_n * sizeof(int16_t));

account:
	period_cnt++;
	if (period_cnt >= sdus_per_sec) {
		period_cnt = 0U;
		control_update();

		sec_cnt++;
		if (sec_cnt >= STATS_PERIOD_S) {
			sec_cnt = 0U;
			stats_print();
		}
	}
}

void audio_in_flush(void)
{
	(void)atomic_set(&flush_req, 1);
}

uint32_t audio_in_feedback_get(void)
{
	return (uint32_t)atomic_get(&feedback);
}

#if defined(CONFIG_AUDIO_INPUT_EMULATED)
#include <math.h>

/* 400 Hz tone at 48 kHz has a period of exactly 120 samples */
#define EMU_TONE_PERIOD 120U
#define EMU_TONE_AMP    8000.0f

static int16_t emu_tone[EMU_TONE_PERIOD];
static uint32_t emu_phase;
static int64_t emu_last_ticks;
static int64_t emu_acc;

static void emu_timer_handler(struct k_timer *timer)
{
	/* frames = (dticks / ticks_per_s) * 1000 * (fb / 2^14) * (1e6 + ppm) / 1e6, accumulated
	 * with the remainder carried so that tick rounding does not bias the rate.
	 */
	const int64_t den = (int64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC * BIT(AUDIO_IN_FB_SHIFT);
	int16_t pcm[PUT_CHUNK_MAX * AUDIO_IN_CHANNELS];
	const int64_t now = k_uptime_ticks();
	uint32_t fb;
	int64_t frames;

	ARG_UNUSED(timer);

	fb = IS_ENABLED(CONFIG_AUDIO_INPUT_EMULATED_FEEDBACK) ? audio_in_feedback_get()
							       : AUDIO_IN_FB_NOMINAL;

	emu_acc += (now - emu_last_ticks) * (int64_t)fb *
		   (1000000LL + CONFIG_AUDIO_INPUT_EMULATED_PPM) / 1000LL;
	emu_last_ticks = now;

	frames = emu_acc / den;
	emu_acc -= frames * den;

	while (frames > 0) {
		const size_t chunk = MIN((size_t)frames, PUT_CHUNK_MAX);

		for (size_t i = 0U; i < chunk; i++) {
			pcm[i * AUDIO_IN_CHANNELS] = emu_tone[emu_phase];
			pcm[(i * AUDIO_IN_CHANNELS) + 1U] = emu_tone[emu_phase];
			emu_phase = (emu_phase + 1U) % EMU_TONE_PERIOD;
		}

		audio_in_put(pcm, chunk);
		frames -= (int64_t)chunk;
	}
}

static K_TIMER_DEFINE(emu_timer, emu_timer_handler, NULL);

void audio_in_emulated_start(void)
{
	for (size_t i = 0U; i < EMU_TONE_PERIOD; i++) {
		emu_tone[i] = (int16_t)(EMU_TONE_AMP * sinf(2.0f * 3.14159265f * (float)i /
							    (float)EMU_TONE_PERIOD));
	}

	printk("Emulated USB host: %d ppm, feedback %s\n", CONFIG_AUDIO_INPUT_EMULATED_PPM,
	       IS_ENABLED(CONFIG_AUDIO_INPUT_EMULATED_FEEDBACK) ? "on" : "off");

	emu_last_ticks = k_uptime_ticks();
	k_timer_start(&emu_timer, K_MSEC(1), K_MSEC(1));
}
#else
void audio_in_emulated_start(void)
{
}
#endif /* CONFIG_AUDIO_INPUT_EMULATED */
