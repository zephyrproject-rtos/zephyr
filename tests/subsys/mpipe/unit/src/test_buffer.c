/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Buffers and buffer pools, mpipe_buffer.c.
 *
 * Most of this is the pool negotiation contract: what a pool requires of its
 * own accord is kept apart from what a negotiation settled on, and a pool that
 * stops forgets the latter. Without that, one run's demands become the next
 * run's floor - the JPEG parser adds a buffer for its partial frame on every
 * negotiation, so the count would climb by one per replay.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_structure.h>

struct mpipe_buffer_api_fixture {
	struct mpipe_buffer_pool pool;
};

static void *buffer_suite_setup(void)
{
	static struct mpipe_buffer_api_fixture fixture;

	return &fixture;
}

static void buffer_before(void *f)
{
	struct mpipe_buffer_api_fixture *fix = f;
	const struct mpipe_buffer_pool_config req = {
		.size = 1024,
		.align = 4,
		.min_buffers = 2,
		.max_buffers = 8,
	};

	memset(fix, 0, sizeof(*fix));

	mpipe_buffer_pool_init(&fix->pool);
	zassert_false(fix->pool.started, "pool.started != false after init");
	zassert_ok(mpipe_buffer_pool_set_req_config(&fix->pool, &req),
		   "stating the requirement failed");
}

ZTEST_SUITE(mpipe_buffer_api, NULL, buffer_suite_setup, buffer_before, NULL, NULL);

/* A pool with no hook has nothing to do, which is a state and not an error */
ZTEST_F(mpipe_buffer_api, test_a_pool_without_hooks_reports_enosys)
{
	struct mpipe_structure config;

	zassert_ok(mpipe_structure_init(&config, MPIPE_MEDIA_AUDIO_PCM), "config init failed");

	zassert_equal(mpipe_buffer_pool_configure(&fixture->pool, &config), -ENOSYS,
		      "configure without a hook did not report -ENOSYS");
	zassert_equal(mpipe_buffer_pool_start(&fixture->pool), -ENOSYS,
		      "start without a hook did not report -ENOSYS");
	zassert_equal(mpipe_buffer_pool_stop(&fixture->pool), -ENOSYS,
		      "stop without a hook did not report -ENOSYS");
}

/* The requirement is the floor a run starts from, so init seeds both configs */
ZTEST_F(mpipe_buffer_api, test_req_config_seeds_the_live_config)
{
	zassert_equal(fixture->pool.config.min_buffers, 2, "live config did not take the floor");
	zassert_equal(fixture->pool.config.align, 4, "live align did not take the floor");
	zassert_equal(fixture->pool.req_config.min_buffers, 2, "requirement not stored");
}

/*
 * The heart of it: a run's negotiated demands must not survive into the next
 * one. This is the JPEG parser's extra buffer, applied twice.
 */
ZTEST_F(mpipe_buffer_api, test_stop_forgets_what_the_run_negotiated)
{
	struct mpipe_buffer_pool_config negotiated;

	for (int run = 0; run < 3; run++) {
		/* Every run starts from the requirement, never from the last run */
		zassert_equal(fixture->pool.config.min_buffers, 2,
			      "run %d started from the previous run's demand", run);

		negotiated = fixture->pool.config;
		negotiated.min_buffers += 1;
		zassert_ok(mpipe_buffer_pool_set_config(&fixture->pool, &negotiated),
			   "applying the negotiated config failed");
		zassert_equal(fixture->pool.config.min_buffers, 3, "the demand did not reach it");

		zassert_ok(mpipe_buffer_pool_stop(&fixture->pool), "stop failed");
	}
}

/*
 * A pool can be proposed upstream, have a demand written into it, and then be
 * turned down without ever starting. It still has to forget the demand, which
 * is why the reset sits before the not-started early return.
 */
ZTEST_F(mpipe_buffer_api, test_stop_resets_a_pool_that_never_started)
{
	struct mpipe_buffer_pool_config negotiated = fixture->pool.config;

	negotiated.min_buffers = 7;
	zassert_ok(mpipe_buffer_pool_set_config(&fixture->pool, &negotiated), "set_config failed");
	zassert_equal(fixture->pool.config.min_buffers, 7, "the demand did not reach it");

	zassert_false(fixture->pool.started, "the pool should not be started");
	zassert_ok(mpipe_buffer_pool_stop(&fixture->pool), "stop of an unstarted pool failed");
	zassert_equal(fixture->pool.config.min_buffers, 2, "an unstarted pool kept the demand");
}

/* A started pool cannot be reconfigured under the elements already using it */
ZTEST_F(mpipe_buffer_api, test_a_started_pool_refuses_reconfiguration)
{
	struct mpipe_buffer_pool_config negotiated = fixture->pool.config;

	fixture->pool.started = true;
	negotiated.min_buffers = 5;

	zassert_equal(mpipe_buffer_pool_set_config(&fixture->pool, &negotiated), -EBUSY,
		      "a started pool accepted a new config");
	zassert_equal(mpipe_buffer_pool_set_req_config(&fixture->pool, &negotiated), -EBUSY,
		      "a started pool accepted a new requirement");
	zassert_equal(fixture->pool.config.min_buffers, 2, "the refused config was applied anyway");
}

static int refusing_set_config(struct mpipe_buffer_pool *pool,
			       const struct mpipe_buffer_pool_config *cfg)
{
	ARG_UNUSED(pool);
	ARG_UNUSED(cfg);

	return -ENOSPC;
}

static int clamping_set_config(struct mpipe_buffer_pool *pool,
			       const struct mpipe_buffer_pool_config *cfg)
{
	pool->config = *cfg;
	pool->config.min_buffers = MIN(cfg->min_buffers, 4);

	return 0;
}

/*
 * The demand reaches a pool only through its owner, which may accept it, clamp
 * it, or refuse. A refusal must leave the pool as it was: that is what lets an
 * adopter fall back to its own pool, as the JPEG parser does.
 */
ZTEST_F(mpipe_buffer_api, test_the_owner_decides_what_it_accepts)
{
	struct mpipe_buffer_pool_config negotiated = fixture->pool.config;

	negotiated.min_buffers = 6;

	fixture->pool.set_config = refusing_set_config;
	zassert_equal(mpipe_buffer_pool_set_config(&fixture->pool, &negotiated), -ENOSPC,
		      "the pool's refusal was not reported");
	zassert_equal(fixture->pool.config.min_buffers, 2, "a refused config was applied anyway");

	fixture->pool.set_config = clamping_set_config;
	zassert_ok(mpipe_buffer_pool_set_config(&fixture->pool, &negotiated), "set_config failed");
	zassert_equal(fixture->pool.config.min_buffers, 4, "the pool's clamp was not honoured");

	/* Stopping still restores the requirement, whatever the owner accepted */
	zassert_ok(mpipe_buffer_pool_stop(&fixture->pool), "stop failed");
	zassert_equal(fixture->pool.config.min_buffers, 2, "stop did not restore the requirement");
}
