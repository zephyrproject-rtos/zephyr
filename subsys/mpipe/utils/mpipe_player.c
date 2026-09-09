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

/* Only a single player instance is supported at a time. */
static atomic_ptr_t active_player = ATOMIC_PTR_INIT(NULL);

static K_THREAD_STACK_DEFINE(mpipe_player_worker_stack, CONFIG_MPIPE_PLAYER_WORKER_STACK_SIZE);

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

static const char *mpipe_player_state_str(enum mpipe_player_state state)
{
	switch (state) {
	case MPIPE_PLAYER_STOPPED:
		return "STOPPED";
	case MPIPE_PLAYER_PLAYING:
		return "PLAYING";
	case MPIPE_PLAYER_PAUSED:
		return "PAUSED";
	default:
		return "?";
	}
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
	(void)mpipe_dump_bin((struct mpipe_bin *)player->pipeline, NULL);
}

static void mpipe_player_dump(struct mpipe_player *player, const char *what)
{
	printk("--- mpipe_dump: %s ---\n", what);
	(void)mpipe_dump_bin((struct mpipe_bin *)player->pipeline, NULL);
}

#else
#define mpipe_player_dump_transition(player, from, to, ok) ((void)0)
#define mpipe_player_dump(player, what)                    ((void)0)
#endif /* CONFIG_MPIPE_PLAYER_DUMP_ON_STATE_CHANGE */

/*
 * Drive the pipeline to a target mpipe_state and update the observable state.
 *
 * One transition at a time rather than asking for the target directly. This is
 * the same work in the same order - mpipe_element_set_state_func() runs one
 * transition per iteration of its own loop either way - and it lets the player
 * see each step, which is what makes a dump per transition possible and what
 * names the transition a failure happened in.
 */
static void mpipe_player_set_state(struct mpipe_player *player, enum mpipe_state target,
				   enum mpipe_player_state new_state)
{
	struct mpipe_element *pipe = (struct mpipe_element *)player->pipeline;

	while (pipe->current_state != target) {
		enum mpipe_state from = pipe->current_state;
		enum mpipe_state next = MPIPE_STATE_GET_NEXT(from, target);

		/* Anything but SUCCESS leaves current_state in place: looping on would spin */
		if (mpipe_element_set_state(pipe, next) != MPIPE_STATE_CHANGE_SUCCESS) {
			LOG_ERR("Failed to set pipeline to %s", mpipe_player_state_str(new_state));
			mpipe_player_dump_transition(player, from, next, false);
			return;
		}

		mpipe_player_dump_transition(player, from, next, true);
	}

	player->state = new_state;
	LOG_INF("Player state: %s", mpipe_player_state_str(new_state));
}

static void mpipe_player_do_play(struct mpipe_player *player)
{
	if (player->state == MPIPE_PLAYER_PLAYING) {
		return;
	}

	/* A resume continues the run it was paused in; a start from STOPPED
	 * begins a new one, which retires any end-of-run still queued.
	 */
	if (player->state == MPIPE_PLAYER_STOPPED) {
		player->run_id++;
	}

	/* Both STOPPED->PLAYING and PAUSED->PLAYING are handled by set_state
	 * stepping through the intermediate states. Resume from PAUSED keeps
	 * queued data (no flush).
	 */
	mpipe_player_set_state(player, MPIPE_STATE_PLAYING, MPIPE_PLAYER_PLAYING);
}

static void mpipe_player_do_pause(struct mpipe_player *player)
{
	if (player->state != MPIPE_PLAYER_PLAYING) {
		return;
	}

	mpipe_player_set_state(player, MPIPE_STATE_PAUSED, MPIPE_PLAYER_PAUSED);
}

static void mpipe_player_do_stop(struct mpipe_player *player)
{
	if (player->state == MPIPE_PLAYER_STOPPED) {
		return;
	}

	mpipe_player_set_state(player, MPIPE_STATE_READY, MPIPE_PLAYER_STOPPED);
}

static void mpipe_player_do_replay(struct mpipe_player *player)
{
	mpipe_player_do_stop(player);
	mpipe_player_do_play(player);
}

/*
 * Say which element failed and what it was doing. Without both, a failure on
 * the pipeline thread reads as a stall with an unexplained log line somewhere
 * above it.
 */
static void mpipe_player_report_error(struct mpipe_player *player, const struct mpipe_message *msg)
{
	LOG_ERR("Pipeline error: element #%u in %s (%d)",
		msg->origin != NULL ? msg->origin->object.id : UINT8_MAX,
		mpipe_player_domain_str(msg->domain), msg->code);

	/*
	 * Only worth a graph when the error arrived while streaming: nothing is
	 * torn down until the stop below, so this is the live graph at the point
	 * it broke. An error raised during a transition leaves the player's state
	 * untouched, and the failed transition has already dumped the same graph.
	 */
	if (player->state == MPIPE_PLAYER_PLAYING) {
		mpipe_player_dump(player, "ERROR");
	}
}

/*
 * Apply a single command. Returns true when the worker should exit (QUIT).
 */
static bool mpipe_player_handle_cmd(struct mpipe_player *player,
				    const struct mpipe_player_cmd_msg *msg)
{
	uint8_t cmd = msg->cmd;

	/*
	 * An end-of-run describes the run it was posted from. A replay racing
	 * the end of the previous run leaves one queued behind the replay, and
	 * acting on it would stop the run that has just started. Drop it: the
	 * run it speaks for no longer exists.
	 */
	if (cmd == MPIPE_PLAYER_CMD_END_OF_RUN || cmd == MPIPE_PLAYER_CMD_RUN_ERROR) {
		if (msg->run_id != player->run_id) {
			LOG_DBG("Dropping end-of-run from run %u, now on run %u", msg->run_id,
				player->run_id);
			return false;
		}

		/* Reported here, not in the listener: this is where it is worth
		 * saying, and where the player state the report reads is settled.
		 */
		if (cmd == MPIPE_PLAYER_CMD_RUN_ERROR) {
			mpipe_player_report_error(player, &player->last_error);
		} else {
			LOG_INF("End of stream");
		}

		cmd = MPIPE_PLAYER_CMD_STOP;
	}

	switch (cmd) {
	case MPIPE_PLAYER_CMD_PLAY:
		mpipe_player_do_play(player);
		break;
	case MPIPE_PLAYER_CMD_PAUSE:
		mpipe_player_do_pause(player);
		break;
	case MPIPE_PLAYER_CMD_TOGGLE:
		if (player->state == MPIPE_PLAYER_PLAYING) {
			mpipe_player_do_pause(player);
		} else {
			mpipe_player_do_play(player);
		}
		break;
	case MPIPE_PLAYER_CMD_STOP:
		mpipe_player_do_stop(player);
		break;
	case MPIPE_PLAYER_CMD_REPLAY:
		mpipe_player_do_replay(player);
		break;
	case MPIPE_PLAYER_CMD_QUIT:
		mpipe_player_do_stop(player);
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

static void mpipe_player_msg_cb(const struct zbus_channel *chan)
{
	const struct mpipe_message *m = zbus_chan_const_msg(chan);
	struct mpipe_player *player = atomic_ptr_get(&active_player);
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

int mpipe_player_init(struct mpipe_player *player, struct mpipe *pipeline)
{
	__ASSERT_NO_MSG(player != NULL);
	__ASSERT_NO_MSG(pipeline != NULL);

	if (atomic_ptr_get(&active_player) != NULL) {
		return -EBUSY;
	}

	player->pipeline = pipeline;
	player->state = MPIPE_PLAYER_STOPPED;
	player->run_id = 0;

	k_msgq_init(&player->cmd_q, player->cmd_buf, sizeof(struct mpipe_player_cmd_msg),
		    CONFIG_MPIPE_PLAYER_CMD_QUEUE_DEPTH);
	k_sem_init(&player->exited, 0, 1);

	struct zbus_channel *bus = mpipe_element_get_bus_chan((struct mpipe_element *)pipeline);

	/* Attach the listener to the pipeline's message channel */
	if (zbus_chan_add_obs(bus, &mpipe_player_listener, K_FOREVER) != 0) {
		LOG_ERR("Failed to attach player to pipeline channel");
		return -EIO;
	}

	player->worker_tid = k_thread_create(&player->worker, mpipe_player_worker_stack,
					     K_THREAD_STACK_SIZEOF(mpipe_player_worker_stack),
					     mpipe_player_worker, player, NULL, NULL,
					     CONFIG_MPIPE_PLAYER_WORKER_PRIORITY, 0, K_NO_WAIT);
	if (player->worker_tid == NULL) {
		LOG_ERR("Failed to create player worker thread");
		(void)zbus_chan_rm_obs(bus, &mpipe_player_listener, K_FOREVER);

		return -EIO;
	}

	atomic_ptr_set(&active_player, player);

	k_thread_name_set(player->worker_tid, "mpipe_player");

#if defined(CONFIG_SHELL)
	/* Advertise the interactive controls so the user does not have to guess
	 * them (they are also discoverable via shell tab-completion and "help").
	 */
	LOG_INF("Player shell ready. Interactive controls:");
	LOG_INF("  p = play/pause toggle, s = stop, r = replay, q = quit");
	IF_ENABLED(CONFIG_MPIPE_DUMP,
		   (LOG_INF("  d = dump the pipeline as a Graphviz graph");))
	LOG_INF("  or: player play|pause|stop|replay|quit|status");
#endif

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

	atomic_ptr_set(&active_player, NULL);

	/* Detach the listener. */
	err = zbus_chan_rm_obs(mpipe_element_get_bus_chan((struct mpipe_element *)player->pipeline),
			       &mpipe_player_listener, K_FOREVER);

	return err;
}

#if defined(CONFIG_SHELL)

/*
 * Interactive shell control for the active player.
 *
 * Two ways to drive the player are registered:
 *  - single-letter top-level shortcuts for fast, one-key control:
 *      p (play/pause toggle), s (stop), r (replay), q (quit)
 *  - a grouped "player" command for discoverability and tab-completion:
 *      player play|pause|stop|replay|status
 */

static struct mpipe_player *mpipe_shell_active(const struct shell *sh)
{
	struct mpipe_player *player = atomic_ptr_get(&active_player);

	if (player == NULL) {
		shell_error(sh, "No active player");
	}

	return player;
}

static int cmd_player_play(const struct shell *sh, size_t argc, char **argv)
{
	struct mpipe_player *player = mpipe_shell_active(sh);

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (player == NULL) {
		return -ENODEV;
	}

	return mpipe_player_play(player);
}

static int cmd_player_pause(const struct shell *sh, size_t argc, char **argv)
{
	struct mpipe_player *player = mpipe_shell_active(sh);

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (player == NULL) {
		return -ENODEV;
	}

	return mpipe_player_pause(player);
}

static int cmd_player_toggle(const struct shell *sh, size_t argc, char **argv)
{
	struct mpipe_player *player = mpipe_shell_active(sh);

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (player == NULL) {
		return -ENODEV;
	}

	return mpipe_player_toggle(player);
}

static int cmd_player_stop(const struct shell *sh, size_t argc, char **argv)
{
	struct mpipe_player *player = mpipe_shell_active(sh);

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (player == NULL) {
		return -ENODEV;
	}

	return mpipe_player_stop(player);
}

static int cmd_player_replay(const struct shell *sh, size_t argc, char **argv)
{
	struct mpipe_player *player = mpipe_shell_active(sh);

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (player == NULL) {
		return -ENODEV;
	}

	return mpipe_player_replay(player);
}

static int cmd_player_quit(const struct shell *sh, size_t argc, char **argv)
{
	struct mpipe_player *player = mpipe_shell_active(sh);

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (player == NULL) {
		return -ENODEV;
	}

	return mpipe_player_quit(player);
}

static int cmd_player_status(const struct shell *sh, size_t argc, char **argv)
{
	struct mpipe_player *player = mpipe_shell_active(sh);

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (player == NULL) {
		return -ENODEV;
	}

	shell_print(sh, "Player state: %s", mpipe_player_state_str(player->state));

	return 0;
}

#if defined(CONFIG_MPIPE_DUMP)

/*
 * Write a dump to a shell instance. Going through the shell rather than the log
 * keeps prefixes and timestamps out of the graph, which is what lets the DOT
 * rendering be piped straight into dot(1).
 */
static void mpipe_player_dump_vprint(void *ctx, const char *fmt, va_list ap)
{
	shell_vfprintf((const struct shell *)ctx, SHELL_NORMAL, fmt, ap);
}

static int cmd_player_dump(const struct shell *sh, size_t argc, char **argv)
{
	struct mpipe_dump_sink sink = {
		.vprint = mpipe_player_dump_vprint,
		.ctx = (void *)sh,
	};

	struct mpipe_player *player = mpipe_shell_active(sh);

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (player == NULL) {
		return -ENODEV;
	}

	return mpipe_dump_bin((struct mpipe_bin *)player->pipeline, &sink);
}

#endif /* CONFIG_MPIPE_DUMP */

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(
	mpipe_player_subcmds, SHELL_CMD(play, NULL, "Start or resume playback", cmd_player_play),
	SHELL_CMD(pause, NULL, "Pause playback", cmd_player_pause),
	SHELL_CMD(stop, NULL, "Stop playback (pipeline to READY)", cmd_player_stop),
	SHELL_CMD(replay, NULL, "Restart playback from the beginning", cmd_player_replay),
	SHELL_CMD(quit, NULL, "Stop the pipeline and exit the player", cmd_player_quit),
	SHELL_CMD(status, NULL, "Print the current player state", cmd_player_status),
	IF_ENABLED(CONFIG_MPIPE_DUMP,
		   (SHELL_CMD(dump, NULL,
			      "Print the pipeline topology and negotiated caps as a "
			      "Graphviz graph",
			      cmd_player_dump),))
	SHELL_SUBCMD_SET_END);
/* clang-format on */

SHELL_CMD_REGISTER(player, &mpipe_player_subcmds, "Multimedia Pipeline player control", NULL);

/* Single-letter top-level shortcuts for fast, one-key control. */
SHELL_CMD_REGISTER(p, NULL, "Player: play/pause toggle", cmd_player_toggle);
SHELL_CMD_REGISTER(s, NULL, "Player: stop", cmd_player_stop);
SHELL_CMD_REGISTER(r, NULL, "Player: replay from the beginning", cmd_player_replay);
SHELL_CMD_REGISTER(q, NULL, "Player: quit", cmd_player_quit);

#if defined(CONFIG_MPIPE_DUMP)
/*
 * Worth a shortcut of its own: over a serial line a graph is then one keystroke
 * rather than a whole command, which matters when the console is the only way
 * in.
 */
SHELL_CMD_REGISTER(d, NULL, "Player: dump the pipeline as a Graphviz graph", cmd_player_dump);
#endif

#endif /* CONFIG_SHELL */
