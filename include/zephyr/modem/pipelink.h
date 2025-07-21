/*
 * Copyright (c) 2024 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/modem/pipe.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#ifndef ZEPHYR_MODEM_PIPELINK_
#define ZEPHYR_MODEM_PIPELINK_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Modem pipelink
 * @defgroup modem_pipelink Modem pipelink
 * @since 3.7
 * @version 1.0.0
 * @ingroup modem
 * @{
 */

/** Pipelink event */
enum modem_pipelink_event {
	/** Modem pipe has been connected and can be opened */
	MODEM_PIPELINK_EVENT_CONNECTED = 0,
	/** Modem pipe has been disconnected and can't be opened */
	MODEM_PIPELINK_EVENT_DISCONNECTED,
};

/** @cond INTERNAL_HIDDEN */

/** Forward declaration */
struct modem_pipelink;

/** @endcond */

/**
 * @brief Pipelink callback definition
 * @param link Modem pipelink instance
 * @param event Modem pipelink event
 * @param user_data User data passed to modem_pipelink_attach()
 */
typedef void (*modem_pipelink_callback)(struct modem_pipelink *link,
					enum modem_pipelink_event event,
					void *user_data);

/** @cond INTERNAL_HIDDEN */

/** Pipelink structure */
struct modem_pipelink {
	struct modem_pipe *pipe;
	modem_pipelink_callback callback;
	void *user_data;
	bool connected;
	struct k_spinlock spinlock;
};

/** @endcond */

/**
 * @brief Attach callback to pipelink
 * @param link Pipelink instance
 * @param callback Pipelink callback
 * @param user_data User data passed to pipelink callback
 */
void modem_pipelink_attach(struct modem_pipelink *link,
			   modem_pipelink_callback callback,
			   void *user_data);

/**
 * @brief Check whether pipelink pipe is connected
 * @param link Pipelink instance
 * @retval true if pipe is connected
 * @retval false if pipe is not connected
 */
bool modem_pipelink_is_connected(struct modem_pipelink *link);

/**
 * @brief Get pipe from pipelink
 * @param link Pipelink instance
 * @retval Pointer to pipe if pipelink has been initialized
 * @retval NULL if pipelink has not been initialized
 */
struct modem_pipe *modem_pipelink_get_pipe(struct modem_pipelink *link);

/**
 * @brief Clear callback
 * @param link Pipelink instance
 */
void modem_pipelink_release(struct modem_pipelink *link);

/** @cond INTERNAL_HIDDEN */

/** Initialize modem pipelink */
void modem_pipelink_init(struct modem_pipelink *link, struct modem_pipe *pipe);

/** Notify user of pipelink that pipe has been connected */
void modem_pipelink_notify_connected(struct modem_pipelink *link);

/** Notify user of pipelink that pipe has been disconnected */
void modem_pipelink_notify_disconnected(struct modem_pipelink *link);

/** @endcond */

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Synthesize pipelink symbol from devicetree node identifier and name
 * @param node_id Devicetree node identifier
 * @param name Pipelink name
 */
#define MODEM_PIPELINK_DT_SYM(node_id, name) \
	_CONCAT_4(__modem_pipelink_, DT_DEP_ORD(node_id), _, name)

/** @endcond */

/**
 * @brief Declare pipelink from devicetree node identifier and name
 * @param node_id Devicetree node identifier
 * @param name Pipelink name
 */
#define MODEM_PIPELINK_DT_DECLARE(node_id, name) \
	extern struct modem_pipelink MODEM_PIPELINK_DT_SYM(node_id, name)

/**
 * @brief Define pipelink from devicetree node identifier and name
 * @param node_id Devicetree node identifier
 * @param name Pipelink name
 */
#define MODEM_PIPELINK_DT_DEFINE(node_id, name) \
	struct modem_pipelink MODEM_PIPELINK_DT_SYM(node_id, name)

/**
 * @brief Get pointer to pipelink from devicetree node identifier and name
 * @param node_id Devicetree node identifier
 * @param name Pipelink name
 */
#define MODEM_PIPELINK_DT_GET(node_id, name) \
	(&MODEM_PIPELINK_DT_SYM(node_id, name))

/**
 * @brief Device driver instance variants of MODEM_PIPELINK_DT macros
 * @name MODEM_PIPELINK_DT_INST macros
 * @anchor MODEM_PIPELINK_DT_INST
 * @{
 */

#define MODEM_PIPELINK_DT_INST_DECLARE(inst, name) \
	MODEM_PIPELINK_DT_DECLARE(DT_DRV_INST(inst), name)

#define MODEM_PIPELINK_DT_INST_DEFINE(inst, name) \
	MODEM_PIPELINK_DT_DEFINE(DT_DRV_INST(inst), name)

#define MODEM_PIPELINK_DT_INST_GET(inst, name) \
	MODEM_PIPELINK_DT_GET(DT_DRV_INST(inst), name)

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODEM_PIPELINK_ */
