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

/*
 * The players in existence, one slot per worker stack. Claiming a slot reserves
 * its stack and registers the player for the bus listener and the shell in one
 * step. Readers take one atomic load per slot and need no lock.
 */
static K_THREAD_STACK_ARRAY_DEFINE(mpipe_player_stacks, CONFIG_MPIPE_PLAYER_NUM,
				   CONFIG_MPIPE_PLAYER_WORKER_STACK_SIZE);
static atomic_ptr_t mpipe_players[CONFIG_MPIPE_PLAYER_NUM];
/* Held while claiming a slot, so a pipeline cannot get two players at once */
static struct k_spinlock mpipe_players_lock;

static void mpipe_player_msg_cb(const struct zbus_channel *chan);

/*
 * Listener for the messages the pipeline posts on its bus. zbus runs it inline
 * in the posting thread, holding the channel lock, so it only ever records the
 * message and queues a command; the worker does the rest.
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

/* A NULL entry is a mid-enum hole the size assertion cannot see */
static const char *mpipe_player_domain_str(uint8_t domain)
{
	if (domain >= ARRAY_SIZE(mpipe_player_domain_names) ||
	    mpipe_player_domain_names[domain] == NULL) {
		return "?";
	}

	return mpipe_player_domain_names[domain];
}

/* The pipeline's state in the player's words: READY is what a stop leaves behind */
static const char *mpipe_player_state_str(enum mpipe_state state)
{
	switch (state) {
	case MPIPE_STATE_READY:
		return "STOPPED";
	case MPIPE_STATE_PLAYING:
		return "PLAYING";
	case MPIPE_STATE_PAUSED:
		return "PAUSED";
	default:
		return "?";
	}
}

/* Only the worker changes it; a reader on the worker or the shell sees a settled value */
static enum mpipe_state mpipe_player_get_state(const struct mpipe_player *player)
{
	return ((const struct mpipe_element *)player->pipeline)->current_state;
}

/* A player is named after its pipeline, the number a dump shows as "pipeline #N" */
static uint8_t mpipe_player_id(const struct mpipe_player *player)
{
	return ((const struct mpipe_object *)player->pipeline)->id;
}

#if defined(CONFIG_MPIPE_PLAYER_DUMP_ON_STATE_CHANGE)

/*
 * Render the graph to the console, headed by the transition that produced it.
 *
 * Through printk rather than the shell: no shell instance exists here, and a
 * dump asked for at the shell goes through that instead. A failed transition is
 * worth a graph of its own - nothing unwinds one, so what is rendered is the
 * state the pipeline broke in.
 */
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
 * Drive the pipeline to a target state. Already there is a no-op.
 *
 * One transition at a time rather than asking for the target directly. This is
 * the same work in the same order - mpipe_element_set_state_func() runs one
 * transition per iteration of its own loop either way - and it lets the player
 * see each step, which is what makes a dump per transition possible and what
 * names the transition a failure happened in.
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
	/* A resume continues the run it was paused in; a start from READY
	 * begins a new one, which retires any end-of-run still queued.
	 */
	if (mpipe_player_get_state(player) == MPIPE_STATE_READY) {
		player->run_id++;
	}

	mpipe_player_set_state(player, MPIPE_STATE_PLAYING);
}

/*
 * Say which element failed and what it was doing. Without both, a failure on
 * the pipeline thread reads as a stall with an unexplained log line somewhere
 * above it.
 */
static void mpipe_player_report_error(struct mpipe_player *player, const struct mpipe_message *msg)
{
	LOG_ERR("Player #%u: error from element #%u in %s (%d)", mpipe_player_id(player),
		msg->origin != NULL ? msg->origin->object.id : UINT8_MAX,
		mpipe_player_domain_str(msg->domain), msg->code);

	/*
	 * Only worth a graph when the error arrived while streaming: nothing is
	 * torn down until the stop below, so this is the live graph at the point
	 * it broke. An error raised during a transition finds the pipeline short
	 * of PLAYING, and the failed transition has already dumped the same graph.
	 */
	if (mpipe_player_get_state(player) == MPIPE_STATE_PLAYING) {
		mpipe_player_dump(player, "ERROR");
	}
}

/*
 * Apply a single command. Returns true when the worker should exit (QUIT).
 */
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
		 * An end-of-run describes the run it was posted from. A replay
		 * racing the end of the previous run leaves one queued behind the
		 * replay, and acting on it would stop the run that has just
		 * started. Drop it: the run it speaks for no longer exists.
		 */
		if (msg->run_id != player->run_id) {
			LOG_DBG("Dropping end-of-run from run %u, now on run %u", msg->run_id,
				player->run_id);
			break;
		}

		/* Reported here, not in the listener: this is where it is worth
		 * saying, and where the pipeline state the report reads is settled.
		 */
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

/*
 * Player worker thread. It is the sole owner of every state transition, so no
 * state change ever runs in a pipeline thread's context (which would risk a
 * thread joining itself on teardown) nor on the system work queue.
 */
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

/*
 * The player of the pipeline a channel belongs to. The pipeline core sets the
 * pipeline as the channel's user data, and the one listener is attached to
 * every player's channel, so this is how a message finds its player.
 */
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

/*
 * Register a player in the first free slot. A pipeline with a player already
 * is refused: two workers driving one pipeline would fight over its state.
 */
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

/* Number of players in existence */
static int mpipe_player_count(void)
{
	int num = 0;

	for (int i = 0; i < CONFIG_MPIPE_PLAYER_NUM; i++) {
		if (atomic_ptr_get(&mpipe_players[i]) != NULL) {
			num++;
		}
	}

	return num;
}

int mpipe_player_init(struct mpipe_player *player, struct mpipe *pipeline)
{
	struct zbus_channel *bus;
	k_tid_t tid;
	int slot;
#ifdef CONFIG_THREAD_NAME
	char name[CONFIG_THREAD_MAX_NAME_LEN];
#endif

	__ASSERT_NO_MSG(player != NULL);
	__ASSERT_NO_MSG(pipeline != NULL);

	player->pipeline = pipeline;
	player->run_id = 0;

	k_msgq_init(&player->cmd_q, player->cmd_buf, sizeof(struct mpipe_player_cmd_msg),
		    CONFIG_MPIPE_PLAYER_CMD_QUEUE_DEPTH);
	k_sem_init(&player->exited, 0, 1);

	/* Registered from here on: everything the listener and the shell read is set */
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
#ifdef CONFIG_THREAD_NAME
	(void)snprintk(name, sizeof(name), "mpipe_player_%d", slot);
	k_thread_name_set(tid, name);
#else
	ARG_UNUSED(tid);
#endif

	/* Advertise the interactive controls once, with the first player, so the
	 * user does not have to guess them (they are also discoverable via shell
	 * tab-completion and "help").
	 */
	if (IS_ENABLED(CONFIG_SHELL) && mpipe_player_count() == 1) {
		LOG_INF("Player shell ready. Interactive controls:");
		LOG_INF("  p = play/pause toggle, s = stop, r = replay, q = quit");
		IF_ENABLED(CONFIG_MPIPE_DUMP,
			   (LOG_INF("  d = dump the pipeline as a Graphviz graph");))
		LOG_INF("  or: player play|pause|stop|replay|quit|status");
		LOG_INF("  each takes a pipeline id; without one it applies to every player");
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

	/* Post QUIT here too so cleanup works even if the caller never called
	 * mpipe_player_quit().
	 */
	(void)mpipe_player_post(player, MPIPE_PLAYER_CMD_QUIT);

	/* Wait via k_thread_join(), not k_sem_take(&exited): the worker signals
	 * "exited" only once, and mpipe_player_wait_quit() may have already consumed
	 * it, so a second take could block forever. join() returns once the
	 * worker function has returned, regardless of who consumed the semaphore.
	 */
	(void)k_thread_join(&player->worker, K_FOREVER);

	/* Unregister first: a message arriving before the listener is detached is ignored */
	atomic_ptr_set(&mpipe_players[player->slot], NULL);

	err = zbus_chan_rm_obs(mpipe_element_get_bus_chan((struct mpipe_element *)player->pipeline),
			       &mpipe_player_listener, K_FOREVER);

	return err;
}

#if defined(CONFIG_SHELL)

/*
 * Interactive shell control of the players.
 *
 * Two ways to drive the players are registered:
 *  - single-letter top-level shortcuts for fast, one-key control:
 *      p (play/pause toggle), s (stop), r (replay), q (quit)
 *  - a grouped "player" command for discoverability and tab-completion:
 *      player play|pause|stop|replay|quit|status
 *
 * Each takes the id of the pipeline to act on; without one it acts on every
 * player, which is what a single-player application types.
 */

typedef int (*mpipe_shell_action_t)(const struct shell *sh, struct mpipe_player *player);

/*
 * Apply an action to the player of the pipeline named by the argument, or to
 * every player without one. Returns the first error, -ENODEV when no player
 * matched.
 */
static int mpipe_shell_for_each(const struct shell *sh, size_t argc, char **argv,
				mpipe_shell_action_t action)
{
	unsigned long id = 0;
	int first_err = 0;
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

		if (player == NULL || (argc > 1 && mpipe_player_id(player) != id)) {
			continue;
		}

		num++;

		err = action(sh, player);
		if (err != 0 && first_err == 0) {
			first_err = err;
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

	return first_err;
}

static int mpipe_shell_play(const struct shell *sh, struct mpipe_player *player)
{
	ARG_UNUSED(sh);

	return mpipe_player_play(player);
}

static int mpipe_shell_pause(const struct shell *sh, struct mpipe_player *player)
{
	ARG_UNUSED(sh);

	return mpipe_player_pause(player);
}

static int mpipe_shell_toggle(const struct shell *sh, struct mpipe_player *player)
{
	ARG_UNUSED(sh);

	return mpipe_player_toggle(player);
}

static int mpipe_shell_stop(const struct shell *sh, struct mpipe_player *player)
{
	ARG_UNUSED(sh);

	return mpipe_player_stop(player);
}

static int mpipe_shell_replay(const struct shell *sh, struct mpipe_player *player)
{
	ARG_UNUSED(sh);

	return mpipe_player_replay(player);
}

static int mpipe_shell_quit(const struct shell *sh, struct mpipe_player *player)
{
	ARG_UNUSED(sh);

	return mpipe_player_quit(player);
}

/* One line per player: this is also how the user learns the ids */
static int mpipe_shell_status(const struct shell *sh, struct mpipe_player *player)
{
	shell_print(sh, "Player #%u: %s", mpipe_player_id(player),
		    mpipe_player_state_str(mpipe_player_get_state(player)));

	return 0;
}

static int cmd_player_play(const struct shell *sh, size_t argc, char **argv)
{
	return mpipe_shell_for_each(sh, argc, argv, mpipe_shell_play);
}

static int cmd_player_pause(const struct shell *sh, size_t argc, char **argv)
{
	return mpipe_shell_for_each(sh, argc, argv, mpipe_shell_pause);
}

static int cmd_player_toggle(const struct shell *sh, size_t argc, char **argv)
{
	return mpipe_shell_for_each(sh, argc, argv, mpipe_shell_toggle);
}

static int cmd_player_stop(const struct shell *sh, size_t argc, char **argv)
{
	return mpipe_shell_for_each(sh, argc, argv, mpipe_shell_stop);
}

static int cmd_player_replay(const struct shell *sh, size_t argc, char **argv)
{
	return mpipe_shell_for_each(sh, argc, argv, mpipe_shell_replay);
}

static int cmd_player_quit(const struct shell *sh, size_t argc, char **argv)
{
	return mpipe_shell_for_each(sh, argc, argv, mpipe_shell_quit);
}

static int cmd_player_status(const struct shell *sh, size_t argc, char **argv)
{
	return mpipe_shell_for_each(sh, argc, argv, mpipe_shell_status);
}

#if defined(CONFIG_MPIPE_DUMP)

/*
 * Write a dump to a shell instance. Going through the shell rather than the log
 * keeps prefixes and timestamps out of the graph, which is what lets the DOT
 * rendering be piped straight into dot(1).
 */
static void mpipe_player_dump_print(void *ctx, const char *str)
{
	shell_fprintf((const struct shell *)ctx, SHELL_NORMAL, "%s", str);
}

/* Each pipeline is a digraph of its own, so several players render in turn */
static int mpipe_shell_dump(const struct shell *sh, struct mpipe_player *player)
{
	return mpipe_dump_bin((struct mpipe_bin *)player->pipeline, mpipe_player_dump_print,
			      (void *)sh);
}

static int cmd_player_dump(const struct shell *sh, size_t argc, char **argv)
{
	return mpipe_shell_for_each(sh, argc, argv, mpipe_shell_dump);
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

SHELL_CMD_REGISTER(player, &mpipe_player_subcmds,
		   "Multimedia Pipeline player control; [id] names a pipeline, every player "
		   "without one",
		   NULL);

/* Single-letter top-level shortcuts for fast, one-key control. */
SHELL_CMD_ARG_REGISTER(p, NULL, "Player: play/pause toggle [id]", cmd_player_toggle, 1, 1);
SHELL_CMD_ARG_REGISTER(s, NULL, "Player: stop [id]", cmd_player_stop, 1, 1);
SHELL_CMD_ARG_REGISTER(r, NULL, "Player: replay from the beginning [id]", cmd_player_replay, 1, 1);
SHELL_CMD_ARG_REGISTER(q, NULL, "Player: quit [id]", cmd_player_quit, 1, 1);

#if defined(CONFIG_MPIPE_DUMP)
/*
 * Worth a shortcut of its own: over a serial line a graph is then one keystroke
 * rather than a whole command, which matters when the console is the only way
 * in.
 */
SHELL_CMD_ARG_REGISTER(d, NULL, "Player: dump the pipeline as a Graphviz graph [id]",
		       cmd_player_dump, 1, 1);
#endif

#endif /* CONFIG_SHELL */
