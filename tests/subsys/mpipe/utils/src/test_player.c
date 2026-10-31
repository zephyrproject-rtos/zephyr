/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/mpipe_fake_src.h>
#include <zephyr/mpipe/mpipe_sink.h>
#include <zephyr/mpipe/utils/mpipe_player.h>

/* Element ids: a pipeline's id is what names its player, so they are distinct */
enum {
	PLAYER_PIPE_A_ID = 10,
	PLAYER_SRC_A_ID,
	PLAYER_SINK_A_ID,
	PLAYER_PIPE_B_ID = 20,
	PLAYER_SRC_B_ID,
	PLAYER_SINK_B_ID,
	PLAYER_PIPE_C_ID = 30,
};

/* Number of buffers each source produces before EOS */
#define TEST_BUFS_NUM 10

/* How long a run may take: a fake source finishes in simulated microseconds */
#define TEST_RUN_TIMEOUT K_SECONDS(2)

/* One pipeline the player drives: a fake source feeding a sink */
struct test_player_pipe {
	struct mpipe pipeline;
	struct mpipe_fake_src fake_src;
	struct mpipe_sink sink;
	struct mpipe_player player;
};

struct test_player_fixture {
	struct test_player_pipe a;
	struct test_player_pipe b;
	/* Only ever initialized as a pipeline, to ask for one player too many */
	struct mpipe pipeline_c;
	struct mpipe_player player_c;
};

static enum mpipe_state test_player_state(struct test_player_pipe *p)
{
	return ((struct mpipe_element *)&p->pipeline)->current_state;
}

/* Wait for the worker to bring the pipeline to a state, since commands are asynchronous */
static bool test_player_wait_state(struct test_player_pipe *p, enum mpipe_state state,
				   k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);

	while (test_player_state(p) != state) {
		if (sys_timepoint_expired(end)) {
			return false;
		}

		k_sleep(K_MSEC(1));
	}

	return true;
}

static void test_player_pipe_init(struct test_player_pipe *p, uint8_t pipe_id, uint8_t src_id,
				  uint8_t sink_id)
{
	zassert_ok(mpipe_pipeline_init(&p->pipeline, pipe_id));
	zassert_ok(mpipe_fake_src_init(&p->fake_src, src_id));
	zassert_ok(mpipe_sink_init(&p->sink, sink_id));

	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&p->fake_src,
					       MPIPE_PROP_SRC_NUM_BUFS, TEST_BUFS_NUM,
					       MPIPE_PROP_LIST_END),
		   "Failed to set fake_src MPIPE_PROP_SRC_NUM_BUFS");

	zassert_ok(mpipe_bin_add((struct mpipe_bin *)&p->pipeline,
				 (struct mpipe_element *)&p->fake_src,
				 (struct mpipe_element *)&p->sink, NULL),
		   "Failed to add elements");
	zassert_ok(mpipe_element_link((struct mpipe_element *)&p->fake_src,
				      (struct mpipe_element *)&p->sink, NULL),
		   "Failed to link elements");

	zassert_ok(mpipe_player_init(&p->player, &p->pipeline), "Failed to init the player");
}

static void *player_suite_setup(void)
{
	static struct test_player_fixture fixture;

	return &fixture;
}

static void player_before(void *f)
{
	struct test_player_fixture *fix = f;

	memset(fix, 0, sizeof(*fix));

	test_player_pipe_init(&fix->a, PLAYER_PIPE_A_ID, PLAYER_SRC_A_ID, PLAYER_SINK_A_ID);
	test_player_pipe_init(&fix->b, PLAYER_PIPE_B_ID, PLAYER_SRC_B_ID, PLAYER_SINK_B_ID);
}

/* Deinit stops the pipeline and frees the slot, so every test leaves the table empty */
static void player_after(void *f)
{
	struct test_player_fixture *fix = f;

	zassert_ok(mpipe_player_deinit(&fix->a.player));
	zassert_ok(mpipe_player_deinit(&fix->b.player));
}

ZTEST_SUITE(test_player, NULL, player_suite_setup, player_before, player_after, NULL);

/* Two players drive two pipelines, and each one's end-of-stream stops only its own */
ZTEST_F(test_player, test_player_two_pipelines_run_independently)
{
	zassert_ok(mpipe_player_play(&fixture->a.player));
	zassert_ok(mpipe_player_play(&fixture->b.player));

	/* Both run to EOS, on which their player brings them back to READY */
	zassert_true(test_player_wait_state(&fixture->a, MPIPE_STATE_READY, TEST_RUN_TIMEOUT),
		     "Pipeline A did not run to EOS and stop");
	zassert_true(test_player_wait_state(&fixture->b, MPIPE_STATE_READY, TEST_RUN_TIMEOUT),
		     "Pipeline B did not run to EOS and stop");

	/* A replay of one is a run of one */
	zassert_ok(mpipe_player_replay(&fixture->a.player));
	zassert_true(test_player_wait_state(&fixture->a, MPIPE_STATE_READY, TEST_RUN_TIMEOUT),
		     "Pipeline A did not replay to EOS and stop");
	zassert_equal(test_player_state(&fixture->b), MPIPE_STATE_READY,
		      "Replaying A moved pipeline B");
}

/*
 * An end-of-stream reaches the player of the pipeline that posted it. Pipeline
 * B is parked at PAUSED, which no command of its own player would leave it at
 * once a stop from A's end-of-stream reached it.
 */
ZTEST_F(test_player, test_player_eos_reaches_its_own_player)
{
	zassert_ok(mpipe_element_set_state((struct mpipe_element *)&fixture->b.pipeline,
					   MPIPE_STATE_PAUSED),
		   "Pipeline B failed to reach PAUSED");

	zassert_ok(mpipe_player_play(&fixture->a.player));
	zassert_true(test_player_wait_state(&fixture->a, MPIPE_STATE_READY, TEST_RUN_TIMEOUT),
		     "Pipeline A did not run to EOS and stop");

	zassert_equal(test_player_state(&fixture->b), MPIPE_STATE_PAUSED,
		      "The end of stream of A stopped B");

	/* B still works under its own player from where it was left */
	zassert_ok(mpipe_player_play(&fixture->b.player));
	zassert_true(test_player_wait_state(&fixture->b, MPIPE_STATE_READY, TEST_RUN_TIMEOUT),
		     "Pipeline B did not run to EOS and stop");
}

/* The table has CONFIG_MPIPE_PLAYER_NUM slots, given back on deinit */
ZTEST_F(test_player, test_player_slots_are_bounded)
{
	BUILD_ASSERT(CONFIG_MPIPE_PLAYER_NUM == 2, "The test asks for one player too many");

	zassert_ok(mpipe_pipeline_init(&fixture->pipeline_c, PLAYER_PIPE_C_ID));

	zassert_equal(mpipe_player_init(&fixture->player_c, &fixture->pipeline_c), -ENOMEM,
		      "A third player was accepted with two slots");

	/* A pipeline gets one player, whether or not a slot is free */
	zassert_ok(mpipe_player_deinit(&fixture->b.player));
	zassert_equal(mpipe_player_init(&fixture->player_c, &fixture->a.pipeline), -EBUSY,
		      "A second player on pipeline A was accepted");
	zassert_ok(mpipe_player_init(&fixture->b.player, &fixture->b.pipeline));

	zassert_ok(mpipe_player_deinit(&fixture->b.player));
	zassert_ok(mpipe_player_init(&fixture->player_c, &fixture->pipeline_c),
		   "The slot freed by B was not reused");

	/* Leave the table as player_after() expects it: A and B initialized */
	zassert_ok(mpipe_player_deinit(&fixture->player_c));
	zassert_ok(mpipe_player_init(&fixture->b.player, &fixture->b.pipeline));
}
