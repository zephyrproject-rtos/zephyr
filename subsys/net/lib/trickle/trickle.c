/** @file
 * @brief Trickle timer library
 *
 * This implements Trickle timer as specified in RFC 6206
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_trickle, CONFIG_NET_TRICKLE_LOG_LEVEL);

#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/random/random.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/trickle.h>

#define TICK_MAX ~0

static void trickle_timeout(struct k_work *work);

static inline bool is_suppression_disabled(struct net_trickle *trickle)
{
	return trickle->k == NET_TRICKLE_INFINITE_REDUNDANCY;
}

static inline bool is_tx_allowed(struct net_trickle *trickle)
{
	return is_suppression_disabled(trickle) ||
		(trickle->c < trickle->k);
}

static inline uint32_t get_end(struct net_trickle *trickle)
{
	return trickle->Istart + trickle->I;
}

/* Returns a random time point t in [I/2 , I) */
static uint32_t get_t(uint32_t I)
{
	I >>= 1;

	NET_DBG("[%d, %d)", I, I << 1);

	return I + (sys_rand32_get() % I);
}

static void double_interval_timeout(struct net_trickle *trickle)
{
	uint32_t rand_time;
	uint32_t last_end = get_end(trickle);

	trickle->c = 0U;

	NET_DBG("now %u (was at %u)", k_uptime_get_32(), last_end);

	/* Check if we need to double the interval */
	if (trickle->I <= (trickle->Imax_abs >> 1)) {
		/* Double if I <= Imax/2 */
		trickle->I <<= 1;

		NET_DBG("double I %u", trickle->I);
	} else {
		trickle->I = trickle->Imax_abs;

		NET_DBG("I %u", trickle->I);
	}

	/* Random t in [I/2, I) */
	rand_time = get_t(trickle->I);

	NET_DBG("doubling time %u", rand_time);

	trickle->Istart = k_uptime_get_32() + rand_time;
	trickle->double_to = false;

	k_work_reschedule(&trickle->timer, K_MSEC(rand_time));

	NET_DBG("last end %u new end %u for %u I %u",
		last_end, get_end(trickle), trickle->Istart, trickle->I);
}

static inline void reschedule(struct net_trickle *trickle)
{
	uint32_t now = k_uptime_get_32();
	uint32_t diff = get_end(trickle) - now;

	NET_DBG("now %d end in %d", now, diff);

	/* Did the clock wrap */
	if ((int32_t)diff < 0) {
		diff = 0U;
		NET_DBG("Clock wrap");
	}

	trickle->double_to = true;

	k_work_reschedule(&trickle->timer, K_MSEC(diff));
}

static void inteval_timeout(struct net_trickle *trickle)
{
	NET_DBG("Trickle timeout at %d", k_uptime_get_32());

	if (trickle->cb) {
		NET_DBG("TX ok %d c(%u) < k(%u)",
			is_tx_allowed(trickle), trickle->c, trickle->k);

		trickle->cb(trickle, is_tx_allowed(trickle),
			    trickle->user_data);
	}

	if (net_trickle_is_running(trickle)) {
		reschedule(trickle);
	}
}

static void trickle_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct net_trickle *trickle = CONTAINER_OF(dwork,
						   struct net_trickle,
						   timer);

	if (trickle->double_to) {
		double_interval_timeout(trickle);
	} else {
		inteval_timeout(trickle);
	}
}

static void setup_new_interval(struct net_trickle *trickle)
{
	uint32_t t;

	trickle->c = 0U;

	t = get_t(trickle->I);

	trickle->Istart = k_uptime_get_32();

	k_work_reschedule(&trickle->timer, K_MSEC(t));

	NET_DBG("new interval at %d ends %d t %d I %d",
		trickle->Istart,
		get_end(trickle),
		t,
		trickle->I);
}

#define CHECK_IMIN(Imin) \
	((Imin < 2) || (Imin > (TICK_MAX >> 1)))

int net_trickle_create(struct net_trickle *trickle,
		       uint32_t Imin,
		       uint8_t Imax,
		       uint8_t k)
{
	NET_ASSERT(trickle && Imax > 0 && k > 0 && !CHECK_IMIN(Imin));

	(void)memset(trickle, 0, sizeof(struct net_trickle));

	trickle->Imin = Imin;
	trickle->Imax = Imax;
	trickle->Imax_abs = Imin << Imax;
	trickle->k = k;

	NET_ASSERT(trickle->Imax_abs);

	NET_DBG("Imin %d Imax %u k %u Imax_abs %u",
		trickle->Imin, trickle->Imax, trickle->k,
		trickle->Imax_abs);

	k_work_init_delayable(&trickle->timer, trickle_timeout);

	return 0;
}

int net_trickle_start(struct net_trickle *trickle,
		      net_trickle_cb_t cb,
		      void *user_data)
{
	NET_ASSERT(trickle && cb);

	trickle->cb = cb;
	trickle->user_data = user_data;
	trickle->double_to = false;

	/* Random I in [Imin , Imax] */
	trickle->I = trickle->Imin +
		(sys_rand32_get() % (trickle->Imax_abs - trickle->Imin + 1));

	setup_new_interval(trickle);

	NET_DBG("start %d end %d in [%d , %d)",
		trickle->Istart, get_end(trickle),
		trickle->I >> 1, trickle->I);

	return 0;
}

int net_trickle_stop(struct net_trickle *trickle)
{
	NET_ASSERT(trickle);

	k_work_cancel_delayable(&trickle->timer);

	trickle->I = 0U;

	return 0;
}

void net_trickle_consistency(struct net_trickle *trickle)
{
	NET_ASSERT(trickle);

	if (trickle->c < 0xFF) {
		trickle->c++;
	}

	NET_DBG("consistency %u", trickle->c);
}

void net_trickle_inconsistency(struct net_trickle *trickle)
{
	NET_ASSERT(trickle);

	if (trickle->I != trickle->Imin) {
		NET_DBG("inconsistency");

		trickle->I = trickle->Imin;
	}

	setup_new_interval(trickle);
}
