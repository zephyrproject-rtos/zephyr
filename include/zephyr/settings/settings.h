/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SETTINGS_SETTINGS_H_
#define ZEPHYR_INCLUDE_SETTINGS_SETTINGS_H_

#include <sys/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/iterable_sections.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @defgroup file_system_storage File System Storage
 * @ingroup os_services
 * @{
 * @}
 */

/**
 * @defgroup settings Settings
 * @since 1.12
 * @version 1.0.0
 * @ingroup file_system_storage
 * @{
 */

#define SETTINGS_MAX_DIR_DEPTH	8	/* max depth of settings tree */
#define SETTINGS_MAX_NAME_LEN	(8 * SETTINGS_MAX_DIR_DEPTH)
#define SETTINGS_MAX_VAL_LEN	256
#define SETTINGS_NAME_SEPARATOR	'/'
#define SETTINGS_NAME_END '='

/* place for settings additions:
 * up to 7 separators, '=', '\0'
 */
#define SETTINGS_EXTRA_LEN ((SETTINGS_MAX_DIR_DEPTH - 1) + 2)

/**
 * Function used to read the data from the settings storage in
 * h_set handler implementations.
 *
 * @param[in] cb_arg  arguments for the read function. Appropriate cb_arg is
 *                    transferred to h_set handler implementation by
 *                    the backend.
 * @param[out] data  the destination buffer
 * @param[in] len    length of read
 *
 * @return positive: Number of bytes read, 0: key-value pair is deleted.
 *                   On error returns -ERRNO code.
 */
typedef ssize_t (*settings_read_cb)(void *cb_arg, void *data, size_t len);

/**
 * @struct settings_handler
 * Config handlers for subtree implement a set of handler functions.
 * These are registered using a call to @ref settings_register.
 */
struct settings_handler {

	const char *name;
	/**< Name of subtree. */

	int cprio;
	/**< Priority of commit, lower value is higher priority */

	int (*h_get)(const char *key, char *val, int val_len_max);
	/**< Get values handler of settings items identified by keyword names.
	 *
	 * Parameters:
	 *  - key[in] the name with skipped part that was used as name in
	 *    handler registration
	 *  - val[out] buffer to receive value.
	 *  - val_len_max[in] size of that buffer.
	 *
	 * Return: length of data read on success, negative on failure.
	 */

	int (*h_set)(const char *key, size_t len, settings_read_cb read_cb,
		     void *cb_arg);
	/**< Set value handler of settings items identified by keyword names.
	 *
	 * Parameters:
	 *  - key[in] the name with skipped part that was used as name in
	 *    handler registration
	 *  - len[in] the size of the data found in the backend.
	 *  - read_cb[in] function provided to read the data from the backend.
	 *  - cb_arg[in] arguments for the read function provided by the
	 *    backend.
	 *
	 *  Return: 0 on success, non-zero on failure.
	 */

	int (*h_commit)(void);
	/**< This handler gets called after settings has been loaded in full.
	 * User might use it to apply setting to the application.
	 *
	 * Return: 0 on success, non-zero on failure.
	 */

	int (*h_export)(int (*export_func)(const char *name, const void *val,
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
	 *
	 * Return: 0 on success, non-zero on failure.
	 */

	sys_snode_t node;
	/**< Linked list node info for module internal usage. */
};

/**
 * @struct settings_handler_static
 * Config handlers without the node element, used for static handlers.
 * These are registered using a call to SETTINGS_STATIC_HANDLER_DEFINE().
 */
struct settings_handler_static {

	const char *name;
	/**< Name of subtree. */

	int cprio;
	/**< Priority of commit, lower value is higher priority */

	int (*h_get)(const char *key, char *val, int val_len_max);
	/**< Get values handler of settings items identified by keyword names.
	 *
	 * Parameters:
	 *  - key[in] the name with skipped part that was used as name in
	 *    handler registration
	 *  - val[out] buffer to receive value.
	 *  - val_len_max[in] size of that buffer.
	 *
	 * Return: length of data read on success, negative on failure.
	 */

	int (*h_set)(const char *key, size_t len, settings_read_cb read_cb,
		     void *cb_arg);
	/**< Set value handler of settings items identified by keyword names.
	 *
	 * Parameters:
	 *  - key[in] the name with skipped part that was used as name in
	 *    handler registration
	 *  - len[in] the size of the data found in the backend.
	 *  - read_cb[in] function provided to read the data from the backend.
	 *  - cb_arg[in] arguments for the read function provided by the
	 *    backend.
	 *
	 * Return: 0 on success, non-zero on failure.
	 */

	int (*h_commit)(void);
	/**< This handler gets called after settings has been loaded in full.
	 * User might use it to apply setting to the application.
	 */

	int (*h_export)(int (*export_func)(const char *name, const void *val,
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
	 *
	 * Return: 0 on success, non-zero on failure.
	 */
};

/**
 * Define a static handler for settings items
 *
 * @param _hname handler name
 * @param _tree subtree name
 * @param _get get routine (can be NULL)
 * @param _set set routine (can be NULL)
 * @param _commit commit routine (can be NULL)
 * @param _export export routine (can be NULL)
 * @param _cprio commit priority (lower value is higher priority)
 *
 * This creates a variable _hname prepended by settings_handler_.
 *
 */

#define SETTINGS_STATIC_HANDLER_DEFINE_WITH_CPRIO(_hname, _tree, _get, _set, \
						  _commit, _export, _cprio)  \
	const STRUCT_SECTION_ITERABLE(settings_handler_static,		     \
				      settings_handler_ ## _hname) = {       \
		.name = _tree,						     \
		.cprio = _cprio,					     \
		.h_get = _get,						     \
		.h_set = _set,						     \
		.h_commit = _commit,					     \
		.h_export = _export,					     \
	}

/* Handlers without commit priority are set to priority O */
#define SETTINGS_STATIC_HANDLER_DEFINE(_hname, _tree, _get, _set, _commit,   \
				       _export)				     \
	SETTINGS_STATIC_HANDLER_DEFINE_WITH_CPRIO(_hname, _tree, _get, _set, \
		_commit, _export, 0)

/**
 * Initialization of settings and backend
 *
 * Can be called at application startup.
 * In case the backend is a FS Remember to call it after the FS was mounted.
 * For FCB backend it can be called without such a restriction.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_subsys_init(void);

/**
 * Register a handler for settings items stored in RAM with
 * commit priority.
 *
 * @param cf   Structure containing registration info.
 * @param cprio Commit priority (lower value is higher priority).
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_register_with_cprio(struct settings_handler *cf,
				 int cprio);

/**
 * Register a handler for settings items stored in RAM with
 * commit priority set to default.
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
 * Load limited set of serialized items from registered persistence sources.
 * Handlers for serialized item subtrees registered earlier will be called for
 * encountered values that belong to the subtree.
 *
 * @param[in] subtree name of the subtree to be loaded.
 * @return 0 on success, non-zero on failure.
 */
int settings_load_subtree(const char *subtree);

/**
 * Callback function used for direct loading.
 * Used by @ref settings_load_subtree_direct function.
 *
 * @param[in]     key     the name with skipped part that was used as name in
 *                        handler registration
 * @param[in]     len     the size of the data found in the backend.
 * @param[in]     read_cb function provided to read the data from the backend.
 * @param[in,out] cb_arg  arguments for the read function provided by the
 *                        backend.
 * @param[in,out] param   parameter given to the
 *                        @ref settings_load_subtree_direct function.
 *
 * @return When nonzero value is returned, further subtree searching is stopped.
 */
typedef int (*settings_load_direct_cb)(
	const char      *key,
	size_t           len,
	settings_read_cb read_cb,
	void            *cb_arg,
	void            *param);

/**
 * Load limited set of serialized items using given callback.
 *
 * This function bypasses the normal data workflow in settings module.
 * All the settings values that are found are passed to the given callback.
 *
 * @note
 * This function does not call commit function.
 * It works as a blocking function, so it is up to the user to call
 * any kind of commit function when this operation ends.
 *
 * @param[in]     subtree subtree name of the subtree to be loaded.
 * @param[in]     cb      pointer to the callback function.
 * @param[in,out] param   parameter to be passed when callback
 *                        function is called.
 * @return 0 on success, non-zero on failure.
 */
int settings_load_subtree_direct(
	const char             *subtree,
	settings_load_direct_cb cb,
	void                   *param);

/**
 * Save currently running serialized items. All serialized items which are
 * different from currently persisted values will be saved.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_save(void);

/**
 * Save limited set of currently running serialized items. All serialized items
 * that belong to subtree and which are different from currently persisted
 * values will be saved.
 *
 * @param[in] subtree name of the subtree to be loaded.
 * @return 0 on success, non-zero on failure.
 */
int settings_save_subtree(const char *subtree);

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
int settings_save_one(const char *name, const void *value, size_t val_len);

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
 * Call commit for all settings handler. This should apply all
 * settings which has been set, but not applied yet.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_commit(void);

/**
 * Call commit for settings handler that belong to subtree.
 * This should apply all settings which has been set, but not applied yet.
 *
 * @param[in] subtree name of the subtree to be committed.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_commit_subtree(const char *subtree);

/**
 * @} settings
 */


/**
 * @defgroup settings_backend Settings backend interface
 * @ingroup settings
 * @{
 */

/*
 * API for config storage
 */

struct settings_store_itf;

/**
 * Backend handler node for storage handling.
 */
struct settings_store {
	sys_snode_t cs_next;
	/**< Linked list node info for internal usage. */

	const struct settings_store_itf *cs_itf;
	/**< Backend handler structure. */
};

/**
 * Arguments for data loading.
 * Holds all parameters that changes the way data should be loaded from backend.
 */
struct settings_load_arg {
	/**
	 * @brief Name of the subtree to be loaded
	 *
	 * If NULL, all values would be loaded.
	 */
	const char *subtree;
	/**
	 * @brief Pointer to the callback function.
	 *
	 * If NULL then matching registered function would be used.
	 */
	settings_load_direct_cb cb;
	/**
	 * @brief Parameter for callback function
	 *
	 * Parameter to be passed to the callback function.
	 */
	void *param;
};

/**
 * Backend handler functions.
 * Sources are registered using a call to @ref settings_src_register.
 * Destinations are registered using a call to @ref settings_dst_register.
 */
struct settings_store_itf {
	int (*csi_load)(struct settings_store *cs,
			const struct settings_load_arg *arg);
	/**< Loads values from storage limited to subtree defined by subtree.
	 *
	 * Parameters:
	 *  - cs - Corresponding backend handler node,
	 *  - arg - Structure that holds additional data for data loading.
	 *
	 * @note
	 * Backend is expected not to provide duplicates of the entities.
	 * It means that if the backend does not contain any functionality to
	 * really delete old keys, it has to filter out old entities and call
	 * load callback only on the final entity.
	 */

	int (*csi_save_start)(struct settings_store *cs);
	/**< Handler called before an export operation.
	 *
	 * Parameters:
	 *  - cs - Corresponding backend handler node
	 */

	int (*csi_save)(struct settings_store *cs, const char *name,
			const char *value, size_t val_len);
	/**< Save a single key-value pair to storage.
	 *
	 * Parameters:
	 *  - cs - Corresponding backend handler node
	 *  - name - Key in string format
	 *  - value - Binary value
	 *  - val_len - Length of value in bytes.
	 */

	int (*csi_save_end)(struct settings_store *cs);
	/**< Handler called after an export operation.
	 *
	 * Parameters:
	 *  - cs - Corresponding backend handler node
	 */

	/**< Get pointer to the storage instance used by the backend.
	 *
	 * Parameters:
	 *  - cs - Corresponding backend handler node
	 */
	void *(*csi_storage_get)(struct settings_store *cs);
};

/**
 * Register a backend handler acting as source.
 *
 * @param cs Backend handler node containing handler information.
 *
 */
void settings_src_register(struct settings_store *cs);

/**
 * Register a backend handler acting as destination.
 *
 * @param cs Backend handler node containing handler information.
 *
 */
void settings_dst_register(struct settings_store *cs);


/*
 * API for handler lookup
 */

/**
 * Parses a key to an array of elements and locate corresponding module handler.
 *
 * @param[in] name in string format
 * @param[out] next remaining of name after matched handler
 *
 * @return settings_handler_static on success, NULL on failure.
 */
struct settings_handler_static *settings_parse_and_lookup(const char *name,
							  const char **next);

/**
 * Calls settings handler.
 *
 * @param[in]     name        The name of the data found in the backend.
 * @param[in]     len         The size of the data found in the backend.
 * @param[in]     read_cb     Function provided to read the data from
 *                            the backend.
 * @param[in,out] read_cb_arg Arguments for the read function provided by
 *                            the backend.
 * @param[in,out] load_arg    Arguments for data loading.
 *
 * @return 0 or negative error code
 */
int settings_call_set_handler(const char *name,
			      size_t len,
			      settings_read_cb read_cb,
			      void *read_cb_arg,
			      const struct settings_load_arg *load_arg);
/**
 * @}
 */

/**
 * @defgroup settings_name_proc Settings name processing
 * @brief API for const name processing
 * @ingroup settings
 * @{
 */

/**
 * Compares the start of name with a key
 *
 * @param[in] name in string format
 * @param[in] key comparison string
 * @param[out] next pointer to remaining of name, when the remaining part
 *             starts with a separator the separator is removed from next
 *
 * Some examples:
 * settings_name_steq("bt/btmesh/iv", "b", &next) returns 1, next="t/btmesh/iv"
 * settings_name_steq("bt/btmesh/iv", "bt", &next) returns 1, next="btmesh/iv"
 * settings_name_steq("bt/btmesh/iv", "bt/", &next) returns 0, next=NULL
 * settings_name_steq("bt/btmesh/iv", "bta", &next) returns 0, next=NULL
 *
 * REMARK: This routine could be simplified if the settings_handler names
 * would include a separator at the end.
 *
 * @return 0: no match
 *         1: match, next can be used to check if match is full
 */
int settings_name_steq(const char *name, const char *key, const char **next);

/**
 * determine the number of characters before the first separator
 *
 * @param[in] name in string format
 * @param[out] next pointer to remaining of name (excluding separator)
 *
 * @return index of the first separator, in case no separator was found this
 * is the size of name
 *
 */
int settings_name_next(const char *name, const char **next);
/**
 * @}
 */

#ifdef CONFIG_SETTINGS_RUNTIME

/**
 * @defgroup settings_rt Settings subsystem runtime
 * @brief API for runtime settings
 * @ingroup settings
 * @{
 */

/**
 * Set a value with a specific key to a module handler.
 *
 * @param name Key in string format.
 * @param data Binary value.
 * @param len Value length in bytes.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_runtime_set(const char *name, const void *data, size_t len);

/**
 * Get a value corresponding to a key from a module handler.
 *
 * @param name Key in string format.
 * @param data Returned binary value.
 * @param len requested value length in bytes.
 *
 * @return length of data read on success, negative on failure.
 */
int settings_runtime_get(const char *name, void *data, size_t len);

/**
 * Apply settings in a module handler.
 *
 * @param name Key in string format.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_runtime_commit(const char *name);
/**
 * @}
 */

#endif /* CONFIG_SETTINGS_RUNTIME */

/**
 * Get the storage instance used by zephyr.
 *
 * The type of storage object instance depends on the settings backend used.
 * It might pointer to: `struct nvs_fs`, `struct fcb` or string witch file name
 * depends on settings backend type used.
 *
 * @retval Pointer to which reference to the storage object can be stored.
 *
 * @retval 0 on success, negative error code on failure.
 */
int settings_storage_get(void **storage);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SETTINGS_SETTINGS_H_ */
