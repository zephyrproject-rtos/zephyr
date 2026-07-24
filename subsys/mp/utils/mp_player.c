/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/mp/mp_element.h>
#include <zephyr/mp/mp_message.h>
#include <zephyr/mp/utils/mp_player.h>

LOG_MODULE_REGISTER(mp_player, CONFIG_MP_LOG_LEVEL);

/*
 * Commands enqueued by the public API and applied one at a time by the worker
 * thread. Serializing every state change on a single thread keeps the caller
 * (e.g. a console loop) from blocking on a teardown and prevents a pipeline
 * thread from ever joining itself when it delivers EOS/ERROR.
 */
enum mp_player_cmd {
	MP_PLAYER_CMD_PLAY = 0,
	MP_PLAYER_CMD_PAUSE,
	MP_PLAYER_CMD_TOGGLE,
	MP_PLAYER_CMD_STOP,
	MP_PLAYER_CMD_REPLAY,
	MP_PLAYER_CMD_QUIT,
};

/* Only a single player instance is supported at a time. */
static struct mp_player *active_player;

static K_THREAD_STACK_DEFINE(mp_player_worker_stack, CONFIG_MP_PLAYER_WORKER_STACK_SIZE);

static const char *mp_player_state_str(enum mp_player_state state)
{
	switch (state) {
	case MP_PLAYER_STOPPED:
		return "STOPPED";
	case MP_PLAYER_PLAYING:
		return "PLAYING";
	case MP_PLAYER_PAUSED:
		return "PAUSED";
	default:
		return "?";
	}
}

/* Drive the pipeline to a target mp_state and update the observable state. */
static void mp_player_set_state(struct mp_player *player, enum mp_state target,
				enum mp_player_state new_state)
{
	enum mp_state_change_return ret;

	ret = mp_element_set_state((struct mp_element *)player->pipeline, target);
	if (ret == MP_STATE_CHANGE_FAILURE) {
		LOG_ERR("Failed to set pipeline to %s", mp_player_state_str(new_state));
		return;
	}

	player->state = new_state;
	LOG_INF("Player state: %s", mp_player_state_str(new_state));
}

static void mp_player_do_play(struct mp_player *player)
{
	if (player->state == MP_PLAYER_PLAYING) {
		return;
	}

	/* Both STOPPED->PLAYING and PAUSED->PLAYING are handled by set_state
	 * stepping through the intermediate states. Resume from PAUSED keeps
	 * queued data (no flush).
	 */
	mp_player_set_state(player, MP_STATE_PLAYING, MP_PLAYER_PLAYING);
}

static void mp_player_do_pause(struct mp_player *player)
{
	if (player->state != MP_PLAYER_PLAYING) {
		return;
	}

	mp_player_set_state(player, MP_STATE_PAUSED, MP_PLAYER_PAUSED);
}

static void mp_player_do_stop(struct mp_player *player)
{
	if (player->state == MP_PLAYER_STOPPED) {
		return;
	}

	mp_player_set_state(player, MP_STATE_READY, MP_PLAYER_STOPPED);
}

static void mp_player_do_replay(struct mp_player *player)
{
	mp_player_do_stop(player);
	mp_player_do_play(player);
}

/*
 * Apply a single command. Returns true when the worker should exit (QUIT).
 */
static bool mp_player_handle_cmd(struct mp_player *player, uint8_t cmd)
{
	switch (cmd) {
	case MP_PLAYER_CMD_PLAY:
		mp_player_do_play(player);
		break;
	case MP_PLAYER_CMD_PAUSE:
		mp_player_do_pause(player);
		break;
	case MP_PLAYER_CMD_TOGGLE:
		if (player->state == MP_PLAYER_PLAYING) {
			mp_player_do_pause(player);
		} else {
			mp_player_do_play(player);
		}
		break;
	case MP_PLAYER_CMD_STOP:
		mp_player_do_stop(player);
		break;
	case MP_PLAYER_CMD_REPLAY:
		mp_player_do_replay(player);
		break;
	case MP_PLAYER_CMD_QUIT:
		mp_player_do_stop(player);
		LOG_DBG("Player worker exiting");
		k_sem_give(&player->exited);
		return true;
	default:
		break;
	}

	return false;
}

/*
 * Player worker thread. This is the pipeline's sole bus consumer AND the sole
 * owner of every state transition, so nothing here ever runs in a pipeline
 * thread's context (which would risk a thread joining itself on teardown).
 *
 * It waits on two sources at once with k_poll():
 *  - the command queue, fed by the mp_player_*() API (play/pause/stop/quit...),
 *  - the pipeline bus, on which the pipeline posts an EOS (or an ERROR) once
 *   every sink has finished.
 */
static void mp_player_worker(void *p1, void *p2, void *p3)
{
	struct mp_player *player = p1;
	uint8_t cmd;
	struct mp_message msg;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/*
	 * Without a bus we can only observe commands. This keeps the worker
	 * usable in minimal setups (e.g. unit tests) where no bus is wired up.
	 */
	if (player->bus == NULL) {
		for (;;) {
			if (k_msgq_get(&player->cmd_q, &cmd, K_FOREVER) != 0) {
				continue;
			}
			if (mp_player_handle_cmd(player, cmd)) {
				return;
			}
		}
	}

	struct k_poll_event events[2];

	k_poll_event_init(&events[0], K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
			  &player->cmd_q);
	k_poll_event_init(&events[1], K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
			  &player->bus->msgq);

	for (;;) {
		if (k_poll(events, ARRAY_SIZE(events), K_FOREVER) != 0) {
			continue;
		}

		/* Drain all pending commands first so a QUIT is honored promptly. */
		if (events[0].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
			while (k_msgq_get(&player->cmd_q, &cmd, K_NO_WAIT) == 0) {
				if (mp_player_handle_cmd(player, cmd)) {
					return;
				}
			}
		}

		/*
		 * Bus messages: the pipeline delivers a single aggregated EOS (or
		 * an ERROR) once every sink has finished. Return to STOPPED so a
		 * subsequent play/replay starts cleanly.
		 */
		if (events[1].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
			while (mp_bus_pop_msg(player->bus, MP_MESSAGE_EOS | MP_MESSAGE_ERROR,
					      &msg) == 0) {
				LOG_INF("End of stream");
				mp_player_do_stop(player);
				if (k_msgq_num_used_get(&player->bus->msgq) == 0) {
					break;
				}
			}
		}

		events[0].state = K_POLL_STATE_NOT_READY;
		events[1].state = K_POLL_STATE_NOT_READY;
	}
}

static int mp_player_post(struct mp_player *player, enum mp_player_cmd cmd)
{
	uint8_t c = (uint8_t)cmd;

	if (player == NULL) {
		return -EINVAL;
	}

	return k_msgq_put(&player->cmd_q, &c, K_FOREVER);
}

int mp_player_init(struct mp_player *player, struct mp_pipeline *pipeline)
{
	if (player == NULL || pipeline == NULL) {
		return -EINVAL;
	}

	if (active_player != NULL) {
		return -EBUSY;
	}

	player->pipeline = pipeline;
	player->bus = mp_element_get_bus((struct mp_element *)pipeline);
	player->state = MP_PLAYER_STOPPED;

	k_msgq_init(&player->cmd_q, player->cmd_buf, sizeof(uint8_t),
		    CONFIG_MP_PLAYER_CMD_QUEUE_DEPTH);
	k_sem_init(&player->exited, 0, 1);

	active_player = player;

	player->worker_tid =
		k_thread_create(&player->worker, mp_player_worker_stack,
				K_THREAD_STACK_SIZEOF(mp_player_worker_stack), mp_player_worker,
				player, NULL, NULL, CONFIG_MP_PLAYER_WORKER_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(player->worker_tid, "mp_player");

#if defined(CONFIG_SHELL)
	/* Advertise the interactive controls so the user does not have to guess
	 * them (they are also discoverable via shell tab-completion and "help").
	 */
	LOG_INF("Player shell ready. Interactive controls:");
	LOG_INF("  p = play/pause toggle, s = stop, r = replay, q = quit");
	LOG_INF("  or: player play|pause|stop|replay|quit|status");
#endif

	return 0;
}

int mp_player_play(struct mp_player *player)
{
	return mp_player_post(player, MP_PLAYER_CMD_PLAY);
}

int mp_player_pause(struct mp_player *player)
{
	return mp_player_post(player, MP_PLAYER_CMD_PAUSE);
}

int mp_player_toggle(struct mp_player *player)
{
	return mp_player_post(player, MP_PLAYER_CMD_TOGGLE);
}

int mp_player_stop(struct mp_player *player)
{
	return mp_player_post(player, MP_PLAYER_CMD_STOP);
}

int mp_player_replay(struct mp_player *player)
{
	return mp_player_post(player, MP_PLAYER_CMD_REPLAY);
}

int mp_player_quit(struct mp_player *player)
{
	return mp_player_post(player, MP_PLAYER_CMD_QUIT);
}

int mp_player_wait_quit(struct mp_player *player)
{
	if (player == NULL) {
		return -EINVAL;
	}

	k_sem_take(&player->exited, K_FOREVER);

	return 0;
}

int mp_player_deinit(struct mp_player *player)
{
	if (player == NULL) {
		return -EINVAL;
	}

	/* Post QUIT here too so cleanup works even if the caller never called
	 * mp_player_quit().
	 */
	(void)mp_player_post(player, MP_PLAYER_CMD_QUIT);

	/* Wait via k_thread_join(), not k_sem_take(&exited): the worker signals
	 * "exited" only once, and mp_player_wait_quit() may have already consumed
	 * it, so a second take could block forever. join() returns once the
	 * worker function has returned, regardless of who consumed the semaphore.
	 */
	(void)k_thread_join(&player->worker, K_FOREVER);

	active_player = NULL;

	return 0;
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

static int mp_shell_check_active(const struct shell *sh)
{
	if (active_player == NULL) {
		shell_error(sh, "No active player");
		return -ENODEV;
	}

	return 0;
}

static int cmd_player_play(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (mp_shell_check_active(sh) != 0) {
		return -ENODEV;
	}

	return mp_player_play(active_player);
}

static int cmd_player_pause(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (mp_shell_check_active(sh) != 0) {
		return -ENODEV;
	}

	return mp_player_pause(active_player);
}

static int cmd_player_toggle(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (mp_shell_check_active(sh) != 0) {
		return -ENODEV;
	}

	return mp_player_toggle(active_player);
}

static int cmd_player_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (mp_shell_check_active(sh) != 0) {
		return -ENODEV;
	}

	return mp_player_stop(active_player);
}

static int cmd_player_replay(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (mp_shell_check_active(sh) != 0) {
		return -ENODEV;
	}

	return mp_player_replay(active_player);
}

static int cmd_player_quit(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (mp_shell_check_active(sh) != 0) {
		return -ENODEV;
	}

	return mp_player_quit(active_player);
}

static int cmd_player_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (mp_shell_check_active(sh) != 0) {
		return -ENODEV;
	}

	shell_print(sh, "Player state: %s", mp_player_state_str(active_player->state));

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	mp_player_subcmds, SHELL_CMD(play, NULL, "Start or resume playback", cmd_player_play),
	SHELL_CMD(pause, NULL, "Pause playback", cmd_player_pause),
	SHELL_CMD(stop, NULL, "Stop playback (pipeline to READY)", cmd_player_stop),
	SHELL_CMD(replay, NULL, "Restart playback from the beginning", cmd_player_replay),
	SHELL_CMD(quit, NULL, "Stop the pipeline and exit the player", cmd_player_quit),
	SHELL_CMD(status, NULL, "Print the current player state", cmd_player_status),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(player, &mp_player_subcmds, "MultimediaPipe player control", NULL);

/* Single-letter top-level shortcuts for fast, one-key control. */
SHELL_CMD_REGISTER(p, NULL, "Player: play/pause toggle", cmd_player_toggle);
SHELL_CMD_REGISTER(s, NULL, "Player: stop", cmd_player_stop);
SHELL_CMD_REGISTER(r, NULL, "Player: replay from the beginning", cmd_player_replay);
SHELL_CMD_REGISTER(q, NULL, "Player: quit", cmd_player_quit);

#endif /* CONFIG_SHELL */
