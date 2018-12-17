/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SETTINGS_SETTINGS_H_
#define ZEPHYR_INCLUDE_SETTINGS_SETTINGS_H_

#include <sys/types.h>
#include <misc/util.h>
#include <misc/slist.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup settings Settings subsystem
 * @ingroup file_system_storage
 * @{
 */

#define SETTINGS_MAX_DIR_DEPTH	8	/* max depth of settings tree */
#define SETTINGS_MAX_NAME_LEN	(8 * SETTINGS_MAX_DIR_DEPTH)
#define SETTINGS_MAX_VAL_LEN	256
#define SETTINGS_NAME_SEPARATOR	"/"

/* pleace for settings additions:
 * up to 7 separators, '=', '\0'
 */
#define SETTINGS_EXTRA_LEN ((SETTINGS_MAX_DIR_DEPTH - 1) + 2)

#define SETTINGS_NMGR_OP		0

/**
 * @struct settings_handler
 * Config handlers for subtree implement a set of handler functions.
 * These are registered using a call to @ref settings_register.
 */
struct settings_handler {
	sys_snode_t node;
	/**< Linked list node info for module internal usage. */

	char *name;
	/**< Name of subtree. */

	int (*h_get)(int argc, char **argv, char *val, int val_len_max);
	/**< Get values handler of settings items identified by keyword names.
	 *
	 * Parameters:
	 *  - argc - count of item in argv.
	 *  - argv - array of pointers to keyword names.
	 *  - val - buffer for a value.
	 *  - val_len_max - size of that buffer.
	 */

	int (*h_set)(int argc, char **argv, void *value_ctx);
	/**< Set value handler of settings items identified by keyword names.
	 *
	 * Parameters:
	 *  - argc - count of item in argv, argv - array of pointers to keyword
	 *   names.
	 *  - value_ctx - pointer to the value context which is used parameter
	 *   for data extracting routine (@ref settings_val_read_cb).
	 */

	int (*h_commit)(void);
	/**< This handler gets called after settings has been loaded in full.
	 * User might use it to apply setting to the application.
	 */

	int (*h_export)(int (*export_func)(const char *name, void *val,
					   size_t val_len));
	/**< This gets called to dump all current settings items.
	 *
	 * This happens when @ref settings_save tries to save the settings.
	 * Parameters:
	 *  - export_func: the pointer to the internal function which appends
	 *   a single key-value pair to persisted settings. Don't store
	 *   duplicated value. The name is subtree/key string, val is the string
	 *   with value.
	 *
	 * @remarks The User might limit a implementations of handler to serving
	 * only one keyword at one call - what will impose limit to get/set
	 * values using full subtree/key name.
	 */
};

/**
 * Initialization of settings and backend
 *
 * Can be called at application startup.
 * In case the backend is NFFS Remember to call it after FS was mounted.
 * For FCB backend it can be called without such a restriction.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_subsys_init(void);

/**
 * Register a handler for settings items.
 *
 * @param cf Structure containing registration info.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_register(struct settings_handler *cf);

/**
 * Load serialized items from registered persistence sources. Handlers for
 * serialized item subtrees registered earlier will be called for encountered
 * values.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_load(void);

/**
 * Save currently running serialized items. All serialized items which are different
 * from currently persisted values will be saved.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_save(void);

/**
 * Write a single serialized value to persisted storage (if it has
 * changed value).
 *
 * @param name Name/key of the settings item.
 * @param value Pointer to the value of the settings item. This value will
 * be transferred to the @ref settings_handler::h_export handler implementation.
 * @param val_len Length of the value.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_save_one(const char *name, void *value, size_t val_len);

/**
 * Delete a single serialized in persisted storage.
 *
 * Deleting an existing key-value pair in the settings mean
 * to set its value to NULL.
 *
 * @param name Name/key of the settings item.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_delete(const char *name);

/**
 * Set settings item identified by @p name to be value @p value.
 * This finds the settings handler for this subtree and calls it's
 * set handler.
 *
 * @param name Name/key of the settings item.
 * @param value Pointer to the value of the settings item. This value will
 * be transferred to the @ref settings_handler::h_set handler implementation.
 * @param len Length of value string.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_set_value(char *name, void *value, size_t len);

/**
 * Get value of settings item identified by @p name.
 * This calls the settings handler h_get for the subtree.
 *
 * Configuration handler should copy the string to @p buf, the maximum
 * number of bytes it will copy is limited by @p buf_len.
 *
 * @param name Name/key of the settings item.
 *
 * @param buf buffer for value of the settings item.
 * If value is not string, the value will be filled in *buf.
 *
 * @param buf_len size of buf.
 *
 * @return Positive: Length of copied dat. Negative: -ERCODE
 */
int settings_get_value(char *name, char *buf, int buf_len);

/**
 * Call commit for all settings handler. This should apply all
 * settings which has been set, but not applied yet.
 *
 * @param name Name of the settings subtree, or NULL to commit everything.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_commit(char *name);

/**
 * Persistent data extracting routine.
 *
 * This function read and decode data from non-volatile storage to user buffer
 * This function should be used inside set handler in order to read the settings
 * data from backend storage.
 *
 * @param[in] value_ctx Data context provided by the h_set handler.
 * @param[out] buf Buffer for data read.
 * @param[in] len Length of @p buf.
 *
 * @retval Negative value on failure. 0 and positive: Length of data loaded to
 * the @p buf.
 */
int settings_val_read_cb(void *value_ctx, void *buf, size_t len);

/**
 * This function fetch length of decode data.
 * This function should be used inside set handler in order to detect the
 * settings data length.
 *
 * @param[in] value_ctx Data context provided by the h_set handler.
 *
 * @retval length of data.
 */
size_t settings_val_get_len_cb(void *value_ctx);

/**
 * @} settings
 */

/*
 * Config storage
 */
struct settings_store_itf;
struct settings_store {
	sys_snode_t cs_next;
	const struct settings_store_itf *cs_itf;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SETTINGS_SETTINGS_H_ */
