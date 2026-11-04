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
 *
 * With CONFIG_AUDIO_INPUT_TIMED the USB SOF times and the SDU transmission times share the
 * controller time base. The feedback value is then the measured number of samples per SOF
 * period at the ISO stream rate, and a slow proportional loop holds the buffer level at the
 * nominal encode time of each SDU, interpolated from the SOF timeline, on the target. This
 * level is free of USB packet and thread scheduling jitter, so the buffer stays at its target
 * without dropped or repeated samples.
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

#if defined(CONFIG_AUDIO_INPUT_TIMED)
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#endif /* CONFIG_AUDIO_INPUT_TIMED */

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

/* SOF timeline: one entry per SOF, (time, samples written). The consumer reads back at most
 * TL_DEPTH entries so that the producer does not overwrite an entry being read.
 */
#define TL_SIZE  32U
#define TL_DEPTH (TL_SIZE - 4U)

#define SOF_PERIOD_US 1000U
/* A longer SOF gap (bus suspend, host stall) restarts the SOF rate measurement */
#define SOF_GAP_US    100000U

/* Rate measured over the last RATE_WINDOW..2*RATE_WINDOW SOF periods, ~0.5 ppm resolution with
 * 1 us time stamps; a first estimate is used after RATE_MIN_FRAMES.
 */
#define RATE_WINDOW_FRAMES 2048U
#define RATE_MIN_FRAMES    128U

/* The buffer level is evaluated this long before the average encode time, so that it is
 * interpolated between SOF time stamps already received.
 */
#define PHASE_GUARD_US     2000
/* Encode lead time average: 1/16 weight per SDU */
#define LEAD_EMA_WEIGHT    0.0625f
/* Phase loop gain, error in 48 kHz samples, output in Q10.14 LSB: ~4 s time constant */
#define PHASE_KP           4.0f
#define PHASE_LIMIT_PPM    200

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
/* Samples per channel written to the ring buffer, wrapping */
static uint32_t written;

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
/* Samples per channel removed from the ring buffer, wrapping */
static uint32_t removed;

#if defined(CONFIG_AUDIO_INPUT_TIMED)
struct tl_entry {
	uint32_t t;
	uint32_t s;
};

/* Producer timed state */
static struct tl_entry tl[TL_SIZE];
static atomic_t tl_head;
static bool sof_valid;
static uint32_t sof_last_t;
static uint32_t sof_frames;
static uint32_t rate_a_t;
static uint32_t rate_a_k;
static uint32_t rate_b_t;
static uint32_t rate_b_k;
static atomic_t fb_rate;
static atomic_t sof_skipped;
static atomic_t sof_restarts;

/* Consumer timed state */
static bool lead_valid;
static float lead_us;
static int32_t lead_ref;
static bool aligned;
static float phase_sum;
static uint32_t phase_n;
static float phase_min = (float)INT32_MAX;
static float phase_max = (float)INT32_MIN;
static int32_t lead_min = INT32_MAX;
static uint32_t late;
static uint32_t no_timeline;
static uint32_t aligns;
#endif /* CONFIG_AUDIO_INPUT_TIMED */

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
		written += (uint32_t)n;
	}
}

#if defined(CONFIG_AUDIO_INPUT_TIMED)
static void sof_restart(uint32_t t_us)
{
	sof_valid = true;
	sof_last_t = t_us;
	sof_frames = 0U;
	rate_a_t = t_us;
	rate_a_k = 0U;
	rate_b_t = t_us;
	rate_b_k = 0U;
	atomic_inc(&sof_restarts);
}

void audio_in_sof(uint32_t t_us)
{
	const uint32_t dt = t_us - sof_last_t;
	uint32_t frames;
	uint32_t dk;

	if (!sof_valid || (dt > SOF_GAP_US)) {
		sof_restart(t_us);
		return;
	}

	if (dt < (SOF_PERIOD_US / 2U)) {
		return;
	}

	/* The capture may be read late or be from a later SOF than the one being handled;
	 * the frame count follows from the time stamps.
	 */
	dk = (dt + (SOF_PERIOD_US / 2U)) / SOF_PERIOD_US;
	sof_frames += dk;
	sof_last_t = t_us;

	if (dk == 1U) {
		const atomic_val_t head = atomic_get(&tl_head);

		tl[(uint32_t)head % TL_SIZE] = (struct tl_entry){.t = t_us, .s = written};
		(void)atomic_set(&tl_head, head + 1);
	} else {
		/* Samples written are not known at the captured SOF; use it for rate only */
		atomic_inc(&sof_skipped);
	}

	if ((sof_frames - rate_b_k) >= RATE_WINDOW_FRAMES) {
		rate_a_t = rate_b_t;
		rate_a_k = rate_b_k;
		rate_b_t = t_us;
		rate_b_k = sof_frames;
	}

	frames = sof_frames - rate_a_k;
	if (frames >= RATE_MIN_FRAMES) {
		/* Samples per SOF period at exactly AUDIO_IN_RATE_HZ in the controller time base */
		const uint64_t num = (uint64_t)AUDIO_IN_FB_NOMINAL * (t_us - rate_a_t);
		const uint64_t den = (uint64_t)frames * SOF_PERIOD_US;

		(void)atomic_set(&fb_rate, (atomic_val_t)((num + (den / 2U)) / den));
	}
}

/* Buffer level at time t_us for the samples removed so far, interpolated on the SOF timeline */
static bool level_at(uint32_t t_us, float *level)
{
	const atomic_val_t head = atomic_get(&tl_head);
	const uint32_t n = MIN((uint32_t)head, TL_DEPTH);
	struct tl_entry a;
	struct tl_entry b;
	bool found = false;

	if (n < 2U) {
		return false;
	}

	b = tl[(uint32_t)(head - 1) % TL_SIZE];
	if ((int32_t)(t_us - b.t) >= 0) {
		/* Later than the last SOF: no extrapolation */
		*level = (float)(int32_t)(b.s - removed);
		found = true;
	}

	for (uint32_t i = 2U; !found && (i <= n); i++) {
		a = tl[(uint32_t)(head - (atomic_val_t)i) % TL_SIZE];
		if ((int32_t)(t_us - a.t) >= 0) {
			const float frac = (float)(int32_t)(t_us - a.t) /
					   (float)(int32_t)(b.t - a.t);

			*level = (float)(int32_t)(a.s - removed) +
				 ((float)(int32_t)(b.s - a.s) * frac);
			found = true;
		}
		b = a;
	}

	/* Entries read may have been overwritten meanwhile */
	if ((uint32_t)(atomic_get(&tl_head) - head) > (TL_SIZE - TL_DEPTH)) {
		return false;
	}

	return found;
}
#else
void audio_in_sof(uint32_t t_us)
{
	ARG_UNUSED(t_us);
}
#endif /* CONFIG_AUDIO_INPUT_TIMED */

static uint32_t fill_get(void)
{
	return MIN(ring_buf_size_get(&rings[0]), ring_buf_size_get(&rings[1])) / sizeof(int16_t);
}

static void discard(uint32_t samples)
{
	samples = MIN(samples, fill_get());

	for (size_t ch = 0U; ch < AUDIO_IN_CHANNELS; ch++) {
		ring_buf_consume(&rings[ch], samples * sizeof(int16_t));
	}

	removed += samples;
}

#if defined(CONFIG_AUDIO_INPUT_TIMED)
static bool control_update_timed(void)
{
	const float fb_limit = (float)AUDIO_IN_FB_NOMINAL * FB_LIMIT_PPM / 1000000.0f;
	const float phase_limit = (float)AUDIO_IN_FB_NOMINAL * PHASE_LIMIT_PPM / 1000000.0f;
	const int32_t rate = (int32_t)atomic_get(&fb_rate);
	float err;
	float fb;

	if ((phase_n == 0U) || (rate == 0)) {
		phase_sum = 0.0f;
		phase_n = 0U;
		return false;
	}

	/* Error in 48 kHz samples */
	err = (phase_sum / (float)phase_n) * (float)ratio;
	phase_sum = 0.0f;
	phase_n = 0U;

	fb = (float)(rate - (int32_t)AUDIO_IN_FB_NOMINAL) +
	     CLAMP(-(PHASE_KP * err), -phase_limit, phase_limit);
	fb = CLAMP(fb, -fb_limit, fb_limit);

	atomic_set(&feedback, (atomic_val_t)((int32_t)AUDIO_IN_FB_NOMINAL + (int32_t)fb));

	/* Continue from the same value if the time stamps are lost */
	pi_integral = -fb;

	return true;
}
#endif /* CONFIG_AUDIO_INPUT_TIMED */

static void control_update(void)
{
	const float fb_limit = (float)AUDIO_IN_FB_NOMINAL * FB_LIMIT_PPM / 1000000.0f;
	float err;
	float fb;

#if defined(CONFIG_AUDIO_INPUT_TIMED)
	if (primed && control_update_timed()) {
		fill_sum = 0U;
		fill_sum_n = 0U;
		return;
	}
#endif /* CONFIG_AUDIO_INPUT_TIMED */

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

#if defined(CONFIG_AUDIO_INPUT_TIMED)
	const int32_t rate = (int32_t)atomic_get(&fb_rate);

	if (rate != 0) {
		printk("Audio in: rate %d (%d ppm) level err %d..%d lead %d us ref %d us min %d us "
		       "late %u align %u no timeline %u sof skip %u restart %u\n",
		       rate,
		       ((rate - (int32_t)AUDIO_IN_FB_NOMINAL) * 1000000) /
			       (int32_t)AUDIO_IN_FB_NOMINAL,
		       (int32_t)phase_min, (int32_t)phase_max, (int32_t)lead_us, lead_ref, lead_min,
		       late, aligns, no_timeline, (uint32_t)atomic_get(&sof_skipped),
		       (uint32_t)atomic_get(&sof_restarts));
	}

	phase_min = (float)INT32_MAX;
	phase_max = (float)INT32_MIN;
	lead_min = INT32_MAX;
#endif /* CONFIG_AUDIO_INPUT_TIMED */
}

#if defined(CONFIG_AUDIO_INPUT_TIMED)
static void phase_update(float err, uint32_t *fill)
{
	if (!aligned) {
		/* One time phase alignment after priming, before the loop takes over */
		if (err > (float)(frame_n / 4U)) {
			const uint32_t excess = MIN((uint32_t)err, *fill - frame_n);

			discard(excess);
			*fill -= excess;
			err -= (float)excess;
			aligns++;
		}
		aligned = true;
	}

	phase_sum += err;
	phase_n++;
	phase_min = MIN(phase_min, err);
	phase_max = MAX(phase_max, err);
}
#endif /* CONFIG_AUDIO_INPUT_TIMED */

static void get_frame(int16_t *left, int16_t *right, bool timed, uint32_t t_level)
{
	int16_t *out[AUDIO_IN_CHANNELS] = {left, right};
	uint32_t fill;
	size_t n = frame_n;

	if (atomic_cas(&flush_req, 1, 0)) {
		discard(fill_get());
		primed = false;
	}

#if defined(CONFIG_AUDIO_INPUT_TIMED)
	if (!primed) {
		aligned = false;
	}
#endif /* CONFIG_AUDIO_INPUT_TIMED */

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

#if defined(CONFIG_AUDIO_INPUT_TIMED)
	if (timed) {
		float level;

		if (level_at(t_level, &level)) {
			phase_update(level - (float)target, &fill);
		} else {
			no_timeline++;
		}
	}
#else
	ARG_UNUSED(timed);
	ARG_UNUSED(t_level);
#endif /* CONFIG_AUDIO_INPUT_TIMED */

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
	removed += (uint32_t)n;

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

void audio_in_get(int16_t *left, int16_t *right)
{
	get_frame(left, right, false, 0U);
}

void audio_in_get_timed(int16_t *left, int16_t *right, uint32_t t_tx_us)
{
#if defined(CONFIG_AUDIO_INPUT_TIMED)
	const uint32_t now = (uint32_t)z_nrf_grtc_timer_read();
	const int32_t lead = (int32_t)(t_tx_us - now);

	/* Encode lead time before the SDU is sent on air */
	if (!lead_valid) {
		lead_us = (float)lead;
		lead_valid = true;
	} else {
		lead_us += ((float)lead - lead_us) * LEAD_EMA_WEIGHT;
	}

	lead_min = MIN(lead_min, lead);
	if (lead <= 0) {
		late++;
	}

	/* Freeze the reference once aligned so the encode wakeup jitter does not move the
	 * point where the level is measured.
	 */
	if (!aligned) {
		lead_ref = (int32_t)lead_us;
	}

	/* Level at a fixed lead before the SDU air time, independent of this wakeup's latency */
	get_frame(left, right, true, t_tx_us - (uint32_t)(lead_ref + PHASE_GUARD_US));
#else
	ARG_UNUSED(t_tx_us);

	get_frame(left, right, false, 0U);
#endif /* CONFIG_AUDIO_INPUT_TIMED */
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
static uint64_t emu_t0;
static uint64_t emu_frame;
static uint32_t emu_acc;

static uint64_t emu_now_us(void)
{
#if defined(CONFIG_AUDIO_INPUT_TIMED)
	return z_nrf_grtc_timer_read();
#else
	return k_ticks_to_us_floor64((uint64_t)k_uptime_ticks());
#endif /* CONFIG_AUDIO_INPUT_TIMED */
}

static void emu_timer_handler(struct k_timer *timer)
{
	int16_t pcm[PUT_CHUNK_MAX * AUDIO_IN_CHANNELS];
	const uint64_t now = emu_now_us();

	ARG_UNUSED(timer);

	/* Emulate an asynchronous UAC2 host: each host frame, of 1 ms at the host clock, carries
	 * the samples of the accumulated feedback value.
	 */
	while (true) {
		const uint64_t host_scale = (uint64_t)(1000000 + CONFIG_AUDIO_INPUT_EMULATED_PPM);
		const uint64_t t_sof = emu_t0 +
				       ((emu_frame * USEC_PER_MSEC * 1000000ULL) / host_scale);
		uint32_t frames;

		if (t_sof > now) {
			break;
		}

		emu_acc += IS_ENABLED(CONFIG_AUDIO_INPUT_EMULATED_FEEDBACK) ?
				   audio_in_feedback_get() : AUDIO_IN_FB_NOMINAL;
		frames = MIN(emu_acc >> AUDIO_IN_FB_SHIFT, PUT_CHUNK_MAX);
		emu_acc &= BIT_MASK(AUDIO_IN_FB_SHIFT);

		for (uint32_t i = 0U; i < frames; i++) {
			pcm[i * AUDIO_IN_CHANNELS] = emu_tone[emu_phase];
			pcm[(i * AUDIO_IN_CHANNELS) + 1U] = emu_tone[emu_phase];
			emu_phase = (emu_phase + 1U) % EMU_TONE_PERIOD;
		}

		audio_in_sof((uint32_t)t_sof);
		audio_in_put(pcm, frames);
		emu_frame++;
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

	emu_t0 = emu_now_us();
	k_timer_start(&emu_timer, K_MSEC(1), K_MSEC(1));
}
#else
void audio_in_emulated_start(void)
{
}
#endif /* CONFIG_AUDIO_INPUT_EMULATED */
