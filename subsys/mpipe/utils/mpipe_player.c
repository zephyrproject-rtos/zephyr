/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/zbus/zbus.h>

#include <zephyr/mpipe/mpipe_bin.h>
#include <zephyr/mpipe/mpipe_message.h>
#include <zephyr/mpipe/mpipe_pipeline.h>
#include <zephyr/mpipe/utils/mpipe_player.h>

#if defined(CONFIG_MPIPE_DUMP)
#include <zephyr/mpipe/utils/mpipe_dump.h>
#endif

LOG_MODULE_REGISTER(mpipe_player, CONFIG_MPIPE_LOG_LEVEL);

/*
 * Commands enqueued by the public API and applied one at a time by the worker
 * thread. Serializing every state change on a single thread keeps the caller
 * (e.g. a console loop) from blocking on a teardown and prevents a pipeline
 * thread from ever joining itself when it delivers EOS/ERROR.
 */
enum mpipe_player_cmd {
	MPIPE_PLAYER_CMD_PLAY = 0,
	MPIPE_PLAYER_CMD_PAUSE,
	MPIPE_PLAYER_CMD_TOGGLE,
	MPIPE_PLAYER_CMD_STOP,
	MPIPE_PLAYER_CMD_REPLAY,
	MPIPE_PLAYER_CMD_QUIT,
	/* Stop asked for by the bus, on end-of-stream. */
	MPIPE_PLAYER_CMD_END_OF_RUN,
	/* Stop asked for by the bus, on a fatal error kept in last_error. */
	MPIPE_PLAYER_CMD_RUN_ERROR,
};

static K_THREAD_STACK_ARRAY_DEFINE(mpipe_player_stacks, CONFIG_MPIPE_PLAYER_NUM,
				   CONFIG_MPIPE_PLAYER_WORKER_STACK_SIZE);
static atomic_ptr_t mpipe_players[CONFIG_MPIPE_PLAYER_NUM];
/* Held while claiming a slot, so a pipeline cannot get two players at once */
static struct k_spinlock mpipe_players_lock;

static void mpipe_player_msg_cb(const struct zbus_channel *chan);

/*
 * zbus runs the listener inline in the posting thread, holding the channel
 * lock, so it only records the message and queues a command for the worker.
 */
ZBUS_LISTENER_DEFINE(mpipe_player_listener, mpipe_player_msg_cb);

/* clang-format off */
static const char *const mpipe_player_domain_names[] = {
	[MPIPE_ERROR_CAPS] = "capability negotiation",
	[MPIPE_ERROR_BUFFER_POOL] = "buffer negotiation",
	[MPIPE_ERROR_FLOW] = "buffer flow",
	[MPIPE_ERROR_RESOURCE] = "resource",
	[MPIPE_ERROR_FAILED] = "failure",
};
/* clang-format on */
BUILD_ASSERT(ARRAY_SIZE(mpipe_player_domain_names) == MPIPE_ERROR_DOMAIN_END,
	     "An error domain has no name in mpipe_player_domain_names");

static const char *mpipe_player_domain_str(uint8_t domain)
{
	if (domain >= ARRAY_SIZE(mpipe_player_domain_names) ||
	    mpipe_player_domain_names[domain] == NULL) {
		return "?";
	}

	return mpipe_player_domain_names[domain];
}

static const char *mpipe_player_state_str(enum mpipe_state state)
{
	switch (state) {
	case MPIPE_STATE_READY:
		/* READY is what a stop leaves behind */
		return "STOPPED";
	case MPIPE_STATE_PLAYING:
		return "PLAYING";
	case MPIPE_STATE_PAUSED:
		return "PAUSED";
	default:
		return "?";
	}
}

static enum mpipe_state mpipe_player_get_state(const struct mpipe_player *player)
{
	return ((const struct mpipe_element *)player->pipeline)->current_state;
}

static uint8_t mpipe_player_id(const struct mpipe_player *player)
{
	return ((const struct mpipe_object *)player->pipeline)->id;
}

#if defined(CONFIG_MPIPE_PLAYER_DUMP_ON_STATE_CHANGE)

static void mpipe_player_dump_transition(struct mpipe_player *player, enum mpipe_state from,
					 enum mpipe_state to, bool ok)
{
	printk("--- mpipe_dump: %s%s -> %s ---\n", ok ? "" : "FAILED at ",
	       mpipe_dump_state_str(from), mpipe_dump_state_str(to));
	(void)mpipe_dump_bin((struct mpipe_bin *)player->pipeline, NULL, NULL);
}

static void mpipe_player_dump(struct mpipe_player *player, const char *what)
{
	printk("--- mpipe_dump: %s ---\n", what);
	(void)mpipe_dump_bin((struct mpipe_bin *)player->pipeline, NULL, NULL);
}

#else
#define mpipe_player_dump_transition(player, from, to, ok) ((void)0)
#define mpipe_player_dump(player, what)                    ((void)0)
#endif /* CONFIG_MPIPE_PLAYER_DUMP_ON_STATE_CHANGE */

/*
 * Drive the pipeline to a target state, one transition at a time so that each
 * step can be dumped and a failure names the transition it happened in.
 */
static void mpipe_player_set_state(struct mpipe_player *player, enum mpipe_state target)
{
	struct mpipe_element *pipe = (struct mpipe_element *)player->pipeline;

	if (pipe->current_state == target) {
		return;
	}

	while (pipe->current_state != target) {
		enum mpipe_state from = pipe->current_state;
		enum mpipe_state next = MPIPE_STATE_GET_NEXT(from, target);

		/* Anything but 0 leaves current_state in place: looping on would spin */
		if (mpipe_element_set_state(pipe, next) != 0) {
			LOG_ERR("Player #%u: failed to reach %s", mpipe_player_id(player),
				mpipe_player_state_str(target));
			mpipe_player_dump_transition(player, from, next, false);
			return;
		}

		mpipe_player_dump_transition(player, from, next, true);
	}

	LOG_INF("Player #%u state: %s", mpipe_player_id(player), mpipe_player_state_str(target));
}

static void mpipe_player_do_play(struct mpipe_player *player)
{
	/* A start from READY begins a new run, which retires any queued end-of-run */
	if (mpipe_player_get_state(player) == MPIPE_STATE_READY) {
		player->run_id++;
	}

	mpipe_player_set_state(player, MPIPE_STATE_PLAYING);
}

static void mpipe_player_report_error(struct mpipe_player *player, const struct mpipe_message *msg)
{
	LOG_ERR("Player #%u: error from element #%u in %s (%d)", mpipe_player_id(player),
		msg->origin != NULL ? msg->origin->object.id : UINT8_MAX,
		mpipe_player_domain_str(msg->domain), msg->code);

	/* Only while streaming: a failed transition has already dumped its graph */
	if (mpipe_player_get_state(player) == MPIPE_STATE_PLAYING) {
		mpipe_player_dump(player, "ERROR");
	}
}

static bool mpipe_player_handle_cmd(struct mpipe_player *player,
				    const struct mpipe_player_cmd_msg *msg)
{
	switch (msg->cmd) {
	case MPIPE_PLAYER_CMD_PLAY:
		mpipe_player_do_play(player);
		break;
	case MPIPE_PLAYER_CMD_PAUSE:
		/* Only from PLAYING: from READY it would preroll, which is a start */
		if (mpipe_player_get_state(player) == MPIPE_STATE_PLAYING) {
			mpipe_player_set_state(player, MPIPE_STATE_PAUSED);
		}
		break;
	case MPIPE_PLAYER_CMD_TOGGLE:
		if (mpipe_player_get_state(player) == MPIPE_STATE_PLAYING) {
			mpipe_player_set_state(player, MPIPE_STATE_PAUSED);
		} else {
			mpipe_player_do_play(player);
		}
		break;
	case MPIPE_PLAYER_CMD_STOP:
		mpipe_player_set_state(player, MPIPE_STATE_READY);
		break;
	case MPIPE_PLAYER_CMD_END_OF_RUN:
	case MPIPE_PLAYER_CMD_RUN_ERROR:
		/*
		 * A replay racing the end of the previous run leaves that run's
		 * end-of-run queued behind it; acting on it would stop the new run.
		 */
		if (msg->run_id != player->run_id) {
			LOG_DBG("Dropping end-of-run from run %u, now on run %u", msg->run_id,
				player->run_id);
			break;
		}

		/* Reported here, not in the listener, where the pipeline state is settled */
		if (msg->cmd == MPIPE_PLAYER_CMD_RUN_ERROR) {
			mpipe_player_report_error(player, &player->last_error);
		} else {
			LOG_INF("Player #%u: end of stream", mpipe_player_id(player));
		}

		mpipe_player_set_state(player, MPIPE_STATE_READY);
		break;
	case MPIPE_PLAYER_CMD_REPLAY:
		mpipe_player_set_state(player, MPIPE_STATE_READY);
		mpipe_player_do_play(player);
		break;
	case MPIPE_PLAYER_CMD_QUIT:
		mpipe_player_set_state(player, MPIPE_STATE_READY);
		LOG_DBG("Player worker exiting");
		k_sem_give(&player->exited);
		return true;
	default:
		break;
	}

	return false;
}

/* The worker owns every state transition: none runs in a pipeline thread's context */
static void mpipe_player_worker(void *p1, void *p2, void *p3)
{
	struct mpipe_player *player = p1;
	struct mpipe_player_cmd_msg msg;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (k_msgq_get(&player->cmd_q, &msg, K_FOREVER) == 0) {
		if (mpipe_player_handle_cmd(player, &msg)) {
			return;
		}
	}
}

static int mpipe_player_post(struct mpipe_player *player, enum mpipe_player_cmd cmd)
{
	struct mpipe_player_cmd_msg msg;

	__ASSERT_NO_MSG(player != NULL);

	msg.cmd = (uint8_t)cmd;
	msg.run_id = player->run_id;

	return k_msgq_put(&player->cmd_q, &msg, K_NO_WAIT);
}

static struct mpipe_player *mpipe_player_from_chan(const struct zbus_channel *chan)
{
	const struct mpipe *pipeline = zbus_chan_user_data(chan);

	for (int i = 0; i < CONFIG_MPIPE_PLAYER_NUM; i++) {
		struct mpipe_player *player = atomic_ptr_get(&mpipe_players[i]);

		if (player != NULL && player->pipeline == pipeline) {
			return player;
		}
	}

	return NULL;
}

static void mpipe_player_msg_cb(const struct zbus_channel *chan)
{
	const struct mpipe_message *m = zbus_chan_const_msg(chan);
	struct mpipe_player *player = mpipe_player_from_chan(chan);
	int ret = 0;

	if (player == NULL) {
		return;
	}

	switch (m->type) {
	case MPIPE_MESSAGE_ERROR:
		/* Keep the detail for the worker: it is gone once we return. */
		player->last_error = *m;
		ret = mpipe_player_post(player, MPIPE_PLAYER_CMD_RUN_ERROR);
		break;
	case MPIPE_MESSAGE_EOS:
		ret = mpipe_player_post(player, MPIPE_PLAYER_CMD_END_OF_RUN);
		break;
	default:
		break;
	}

	if (ret != 0) {
		LOG_ERR("Failed to post command to the player command queue");
	}
}

static int mpipe_player_claim_slot(struct mpipe_player *player)
{
	k_spinlock_key_t key = k_spin_lock(&mpipe_players_lock);
	int slot = -ENOMEM;

	for (int i = 0; i < CONFIG_MPIPE_PLAYER_NUM; i++) {
		const struct mpipe_player *other = atomic_ptr_get(&mpipe_players[i]);

		if (other == NULL) {
			if (slot < 0) {
				slot = i;
			}
		} else if (other->pipeline == player->pipeline) {
			slot = -EBUSY;
			break;
		}
	}

	if (slot >= 0) {
		atomic_ptr_set(&mpipe_players[slot], player);
	}

	k_spin_unlock(&mpipe_players_lock, key);

	return slot;
}

int mpipe_player_init(struct mpipe_player *player, struct mpipe *pipeline)
{
	struct zbus_channel *bus;
	k_tid_t tid;
	int slot;

	__ASSERT_NO_MSG(player != NULL);
	__ASSERT_NO_MSG(pipeline != NULL);

	player->pipeline = pipeline;
	player->run_id = 0;

	k_msgq_init(&player->cmd_q, player->cmd_buf, sizeof(struct mpipe_player_cmd_msg),
		    CONFIG_MPIPE_PLAYER_CMD_QUEUE_DEPTH);
	k_sem_init(&player->exited, 0, 1);

	slot = mpipe_player_claim_slot(player);
	if (slot < 0) {
		return slot;
	}

	player->slot = (uint8_t)slot;

	/* Attach the listener to the pipeline's message channel */
	bus = mpipe_element_get_bus_chan((struct mpipe_element *)pipeline);
	if (zbus_chan_add_obs(bus, &mpipe_player_listener, K_FOREVER) != 0) {
		LOG_ERR("Failed to attach player to pipeline channel");
		atomic_ptr_set(&mpipe_players[slot], NULL);
		return -EIO;
	}

	tid = k_thread_create(&player->worker, mpipe_player_stacks[slot],
			      K_THREAD_STACK_SIZEOF(mpipe_player_stacks[slot]), mpipe_player_worker,
			      player, NULL, NULL, CONFIG_MPIPE_PLAYER_WORKER_PRIORITY, 0,
			      K_NO_WAIT);
	k_thread_name_set(tid, "mpipe_player");

	/* Advertise the interactive controls once, with the first player */
	if (IS_ENABLED(CONFIG_SHELL) && slot == 0) {
		LOG_INF("Player shell ready. Interactive controls:");
		LOG_INF("  p = play/pause toggle, s = stop, r = replay, q = quit");
		IF_ENABLED(CONFIG_MPIPE_DUMP,
			   (LOG_INF("  d = dump the pipeline as a Graphviz graph");))
		LOG_INF("  or: player play|pause|stop|replay|quit|status");
		LOG_INF("  commands take a pipeline id; without one they act on every player");
	}

	LOG_INF("Player #%u ready", mpipe_player_id(player));

	return 0;
}

int mpipe_player_play(struct mpipe_player *player)
{
	return mpipe_player_post(player, MPIPE_PLAYER_CMD_PLAY);
}

int mpipe_player_pause(struct mpipe_player *player)
{
	return mpipe_player_post(player, MPIPE_PLAYER_CMD_PAUSE);
}

int mpipe_player_toggle(struct mpipe_player *player)
{
	return mpipe_player_post(player, MPIPE_PLAYER_CMD_TOGGLE);
}

int mpipe_player_stop(struct mpipe_player *player)
{
	return mpipe_player_post(player, MPIPE_PLAYER_CMD_STOP);
}

int mpipe_player_replay(struct mpipe_player *player)
{
	return mpipe_player_post(player, MPIPE_PLAYER_CMD_REPLAY);
}

int mpipe_player_quit(struct mpipe_player *player)
{
	return mpipe_player_post(player, MPIPE_PLAYER_CMD_QUIT);
}

int mpipe_player_wait_quit(struct mpipe_player *player)
{
	__ASSERT_NO_MSG(player != NULL);

	k_sem_take(&player->exited, K_FOREVER);

	return 0;
}

int mpipe_player_deinit(struct mpipe_player *player)
{
	int err;

	__ASSERT_NO_MSG(player != NULL);

	if (player->pipeline == NULL) {
		return -EINVAL;
	}

	/* In case the caller never asked for it */
	(void)mpipe_player_post(player, MPIPE_PLAYER_CMD_QUIT);

	/* Join rather than take exited: mpipe_player_wait_quit() may have consumed it */
	(void)k_thread_join(&player->worker, K_FOREVER);

	/* Unregister first: a message arriving before the listener is detached is ignored */
	atomic_ptr_set(&mpipe_players[player->slot], NULL);

	err = zbus_chan_rm_obs(mpipe_element_get_bus_chan((struct mpipe_element *)player->pipeline),
			       &mpipe_player_listener, K_FOREVER);

	return err;
}

#if defined(CONFIG_SHELL)

/*
 * Interactive control of the players: single-letter shortcuts p (play/pause
 * toggle), s (stop), r (replay), q (quit), and the "player" command group.
 * Every command takes the id of the pipeline to act on and acts on every
 * player without one.
 */

/*
 * The players a command acts on: the one whose pipeline argv[1] names, or
 * every player without an argument. Returns how many, -ENODEV when none.
 */
static int mpipe_shell_players(const struct shell *sh, size_t argc, char **argv,
			       struct mpipe_player *players[])
{
	unsigned long id = 0;
	int num = 0;
	int err = 0;

	if (argc > 1) {
		id = shell_strtoul(argv[1], 0, &err);
		if (err != 0) {
			shell_error(sh, "Invalid player id: %s", argv[1]);
			return -EINVAL;
		}
	}

	for (int i = 0; i < CONFIG_MPIPE_PLAYER_NUM; i++) {
		struct mpipe_player *player = atomic_ptr_get(&mpipe_players[i]);

		if (player != NULL && (argc == 1 || mpipe_player_id(player) == id)) {
			players[num] = player;
			num++;
		}
	}

	if (num == 0) {
		if (argc > 1) {
			shell_error(sh, "No player #%lu", id);
		} else {
			shell_error(sh, "No player");
		}

		return -ENODEV;
	}

	return num;
}

/* Apply a player function to each selected player. Returns the first error. */
static int mpipe_shell_for_each(const struct shell *sh, size_t argc, char **argv,
				int (*fn)(struct mpipe_player *player))
{
	struct mpipe_player *players[CONFIG_MPIPE_PLAYER_NUM];
	int num = mpipe_shell_players(sh, argc, argv, players);
	int first_err = 0;

	if (num < 0) {
		return num;
	}

	for (int i = 0; i < num; i++) {
		int err = fn(players[i]);

		if (err != 0 && first_err == 0) {
			first_err = err;
		}
	}

	return first_err;
}

static int cmd_player_play(const struct shell *sh, size_t argc, char **argv)
{
	return mpipe_shell_for_each(sh, argc, argv, mpipe_player_play);
}

static int cmd_player_pause(const struct shell *sh, size_t argc, char **argv)
{
	return mpipe_shell_for_each(sh, argc, argv, mpipe_player_pause);
}

static int cmd_player_toggle(const struct shell *sh, size_t argc, char **argv)
{
	return mpipe_shell_for_each(sh, argc, argv, mpipe_player_toggle);
}

static int cmd_player_stop(const struct shell *sh, size_t argc, char **argv)
{
	return mpipe_shell_for_each(sh, argc, argv, mpipe_player_stop);
}

static int cmd_player_replay(const struct shell *sh, size_t argc, char **argv)
{
	return mpipe_shell_for_each(sh, argc, argv, mpipe_player_replay);
}

static int cmd_player_quit(const struct shell *sh, size_t argc, char **argv)
{
	return mpipe_shell_for_each(sh, argc, argv, mpipe_player_quit);
}

static int cmd_player_status(const struct shell *sh, size_t argc, char **argv)
{
	struct mpipe_player *players[CONFIG_MPIPE_PLAYER_NUM];
	int num = mpipe_shell_players(sh, argc, argv, players);

	if (num < 0) {
		return num;
	}

	for (int i = 0; i < num; i++) {
		shell_print(sh, "Player #%u: %s", mpipe_player_id(players[i]),
			    mpipe_player_state_str(mpipe_player_get_state(players[i])));
	}

	return 0;
}

#if defined(CONFIG_MPIPE_DUMP)

static void mpipe_player_dump_print(void *ctx, const char *str)
{
	shell_fprintf((const struct shell *)ctx, SHELL_NORMAL, "%s", str);
}

static int cmd_player_dump(const struct shell *sh, size_t argc, char **argv)
{
	struct mpipe_player *players[CONFIG_MPIPE_PLAYER_NUM];
	int num = mpipe_shell_players(sh, argc, argv, players);

	if (num < 0) {
		return num;
	}

	for (int i = 0; i < num; i++) {
		int err = mpipe_dump_bin((struct mpipe_bin *)players[i]->pipeline,
					 mpipe_player_dump_print, (void *)sh);

		if (err != 0) {
			return err;
		}
	}

	return 0;
}

#endif /* CONFIG_MPIPE_DUMP */

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(
	mpipe_player_subcmds,
	SHELL_CMD_ARG(play, NULL, "Start or resume playback [id]", cmd_player_play, 1, 1),
	SHELL_CMD_ARG(pause, NULL, "Pause playback [id]", cmd_player_pause, 1, 1),
	SHELL_CMD_ARG(stop, NULL, "Stop playback (pipeline to READY) [id]", cmd_player_stop, 1, 1),
	SHELL_CMD_ARG(replay, NULL, "Restart playback from the beginning [id]", cmd_player_replay,
		      1, 1),
	SHELL_CMD_ARG(quit, NULL, "Stop the pipeline and exit the player [id]", cmd_player_quit,
		      1, 1),
	SHELL_CMD_ARG(status, NULL, "Print the player state [id]", cmd_player_status, 1, 1),
	IF_ENABLED(CONFIG_MPIPE_DUMP,
		   (SHELL_CMD_ARG(dump, NULL,
				  "Print the pipeline topology and negotiated caps as a "
				  "Graphviz graph [id]",
				  cmd_player_dump, 1, 1),))
	SHELL_SUBCMD_SET_END);
/* clang-format on */

SHELL_CMD_REGISTER(player, &mpipe_player_subcmds, "Multimedia Pipeline player control", NULL);

/* Single-letter top-level shortcuts for fast, one-key control. */
SHELL_CMD_ARG_REGISTER(p, NULL, "Player: play/pause toggle [id]", cmd_player_toggle, 1, 1);
SHELL_CMD_ARG_REGISTER(s, NULL, "Player: stop [id]", cmd_player_stop, 1, 1);
SHELL_CMD_ARG_REGISTER(r, NULL, "Player: replay from the beginning [id]", cmd_player_replay, 1, 1);
SHELL_CMD_ARG_REGISTER(q, NULL, "Player: quit [id]", cmd_player_quit, 1, 1);
#if defined(CONFIG_MPIPE_DUMP)
SHELL_CMD_ARG_REGISTER(d, NULL, "Player: dump the pipeline as a Graphviz graph [id]",
		       cmd_player_dump, 1, 1);
#endif

#endif /* CONFIG_SHELL */
