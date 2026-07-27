/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Media-player-like control layer for a pipeline.
 */

#ifndef ZEPHYR_INCLUDE_MP_UTILS_MP_PLAYER_H_
#define ZEPHYR_INCLUDE_MP_UTILS_MP_PLAYER_H_

/**
 * @defgroup mp_player Player
 * @ingroup mp
 * @brief High-level, reusable control layer over a pipeline.
 *
 * The player is a thin helper that owns the run-time lifecycle of a pipeline:
 * play, pause, stop, replay and quit.
 *
 * All state transitions are serialized on a dedicated worker thread so that:
 *  - the caller (e.g. an interactive console loop) never blocks on a pipeline
 *    teardown, and
 *  - end-of-stream / error notifications coming from a pipeline thread never
 *    trigger a state change in that same thread's context (which would attempt
 *    to join the thread that is running it).
 *
 * The caller enqueues commands with the mp_player_*() API; the worker applies
 * them one at a time.
 *
 * @{
 */

#include <zephyr/kernel.h>

#include <zephyr/mp/mp_bus.h>
#include <zephyr/mp/mp_pipeline.h>

/**
 * @brief Observable player state.
 */
enum mp_player_state {
	/** Pipeline is in READY: no thread is streaming. */
	MP_PLAYER_STOPPED = 0,
	/** Pipeline is in PLAYING: data is flowing. */
	MP_PLAYER_PLAYING,
	/** Pipeline is in PAUSED: streaming is suspended, data is preserved. */
	MP_PLAYER_PAUSED,
};

/**
 * @brief Player instance.
 *
 * All fields are for internal use; treat the struct as opaque and only pass a
 * pointer to the mp_player_*() API.
 */
struct mp_player {
	/** Controlled pipeline. */
	struct mp_pipeline *pipeline;
	/** Cached pipeline bus (read by the worker to observe EOS / ERROR). */
	struct mp_bus *bus;
	/** Command queue feeding the worker thread. */
	struct k_msgq cmd_q;
	/** Backing buffer for the command queue. Each cmd is a char (keystroke). */
	char cmd_buf[CONFIG_MP_PLAYER_CMD_QUEUE_DEPTH * sizeof(uint8_t)];
	/** Worker thread control block. */
	struct k_thread worker;
	/** Worker thread id. */
	k_tid_t worker_tid;
	/** Signaled by the worker just before it exits. */
	struct k_sem exited;
	/** Current player state (only written by the worker). */
	enum mp_player_state state;
};

/**
 * @brief Initialize a player and start its worker thread.
 *
 * Registers a bus listener that auto-stops the pipeline on end-of-stream or
 * error. The pipeline must already be built and linked, but should be in the
 * READY state (not yet playing).
 *
 * @note Only a single player instance can be active at a time.
 *
 * @param player   Pointer to an uninitialized @ref mp_player.
 * @param pipeline Pointer to the pipeline to control.
 * @retval 0 on success
 * @retval -EINVAL if @p player or @p pipeline is NULL
 * @retval -EBUSY if another player instance is already active
 */
int mp_player_init(struct mp_player *player, struct mp_pipeline *pipeline);

/**
 * @brief Request PLAYING.
 *
 * From STOPPED this starts the pipeline; from PAUSED it resumes without data
 * loss. No effect if already PLAYING.
 *
 * @param player Pointer to the player.
 * @retval 0 on success, negative errno on failure.
 */
int mp_player_play(struct mp_player *player);

/**
 * @brief Request PAUSED.
 *
 * Suspends streaming while preserving queued data. No effect unless PLAYING.
 *
 * @param player Pointer to the player.
 * @retval 0 on success, negative errno on failure.
 */
int mp_player_pause(struct mp_player *player);

/**
 * @brief Toggle between PLAYING and PAUSED.
 *
 * If PLAYING, pauses; otherwise plays (resuming from PAUSED or starting from
 * STOPPED).
 *
 * @param player Pointer to the player.
 * @retval 0 on success, negative errno on failure.
 */
int mp_player_toggle(struct mp_player *player);

/**
 * @brief Request STOPPED (pipeline to READY).
 *
 * Flushes any queued data and joins all streaming threads.
 *
 * @param player Pointer to the player.
 * @retval 0 on success, negative errno on failure.
 */
int mp_player_stop(struct mp_player *player);

/**
 * @brief Restart playback from the beginning.
 *
 * Equivalent to a stop followed by a play.
 *
 * @param player Pointer to the player.
 * @retval 0 on success, negative errno on failure.
 */
int mp_player_replay(struct mp_player *player);

/**
 * @brief Ask the worker to stop the pipeline and exit.
 *
 * Returns immediately; use @ref mp_player_deinit to wait for the worker to
 * finish and release its resources.
 *
 * @param player Pointer to the player.
 * @retval 0 on success, negative errno on failure.
 */
int mp_player_quit(struct mp_player *player);

/**
 * @brief Block until the player worker exits.
 *
 * Returns once the pipeline has been stopped and the worker thread is about to
 * exit, i.e. after a quit has been requested (typically via the "q" / "player
 * quit" shell command or @ref mp_player_quit). This lets an application's main
 * thread wait for the user to finish while the shell drives the player.
 *
 * @note This only waits; call @ref mp_player_deinit afterwards to join the
 *       worker and release the player's resources.
 *
 * @param player Pointer to the player.
 * @retval 0 on success
 * @retval -EINVAL if @p player is NULL
 */
int mp_player_wait_quit(struct mp_player *player);

/**
 * @brief Wait for the worker to exit and release the player's resources.
 *
 * If the worker has not been asked to quit yet, this requests it first. After
 * this call the player must be re-initialized before reuse.
 *
 * @param player Pointer to the player.
 * @retval 0 on success
 * @retval -EINVAL if @p player is NULL
 */
int mp_player_deinit(struct mp_player *player);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_UTILS_MP_PLAYER_H_ */
