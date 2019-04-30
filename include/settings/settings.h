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
#define SETTINGS_NAME_SEPARATOR	'/'

/* pleace for settings additions:
 * up to 7 separators, '=', '\0'
 */
#define SETTINGS_EXTRA_LEN ((SETTINGS_MAX_DIR_DEPTH - 1) + 2)

typedef ssize_t (*settings_read_cb)(void *cb_arg, void *data, size_t len);

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

	int (*h_get)(const char *key, char *val, int val_len_max);
	/**< Get values handler of settings items identified by keyword names.
	 *
	 * Parameters:
	 *  - key - the name with skipped first element that was used
	 *          in component registration.
	 *  - val - buffer for a value.
	 *  - val_len_max - size of that buffer.
	 */

	int (*h_set)(const char *key, size_t len,
		     settings_read_cb read_cb, void *cb_arg);

	/**< Set value handler of settings items identified by keyword names.
	 *
	 * Parameters:
	 *  - key - the name with skipped first element that was used
	 *          in component registration.
	 *  - len - the size of the data found in the backend.
	 *  - read_cb - function provided to read the data from the backend.
	 *  - cb_arg - arguments for the read function provided by the backend.
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
 * Call commit for all settings handler. This should apply all
 * settings which has been set, but not applied yet.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_commit(void);

/**
 * @} settings
 */


/*
 * API for config storage
 */

struct settings_store_itf;

/**
 * @struct settings_store
 * Backend handler node for storage handling.
 */
struct settings_store {
	sys_snode_t cs_next;
	/**< Linked list node info for internal usage. */

	const struct settings_store_itf *cs_itf;
	/**< Backend handler structure. */
};

/**
 * @struct settings_store_itf
 * Backend handler functions.
 * Sources are registered using a call to @ref settings_src_register.
 * Destinations are registered using a call to @ref settings_dst_register.
 */
struct settings_store_itf {
	int (*csi_load)(struct settings_store *cs);
	/**< Loads all values from storage.
	 *
	 * Parameters:
	 *  - cs - Corresponding backend handler node
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
 * @brief Splits given name to given pattern and next part
 *
 * This function checks if in the beginning of the provided name the
 * given key may be found. The key is considered equal only if it ends with '\0'
 * character or @ref SETTINGS_NAME_SEPARATOR.
 *
 * It means this would match:
 * @code
   settings_key_cmp("my_key/other/stuff", "my_key", NULL);
   settings_key_cmp("my_key", "my_key", NULL);
 * @endcode
 *
 * And below would not match:
 * @code
   settings_name_split("my_key/other/stuff", "my", NULL);
   settings_name_split("my_key/other/stuff", "other", NULL);
   settings_name_split("my_key/other/stuff", "my_keys", NULL);
 * @endcode
 *
 * It is also possible to match bigger part of the patch:
 * @code
   settings_name_split("my_key/other/stuff", "my_key/other", NULL);
   settings_name_split("my_key/other/stuff", "my_key/other/stuff", NULL);
 * @endcode
 *
 * @param[in]  name The name to compare in a form of setting path
 * @param[in]  key  The keyword to search for
 * @param[out] next The beginning of the next name part to search
 *                  or NULL if there is nothing more to search.
 *                  Effectively it would point to the character after
 *                  the @ref SETTINGS_NAME_SEPARATOR following the key found.
 *
 * @retval true  The comparison matched.
 * @retval false The comparison does not matched.
 */
bool settings_name_split(const char *name, const char *key, const char **next);

/**
 * @brief Skip single name patch element
 *
 * Function skips single element.
 * For "a/b/c" the pointer to "b/c" string would be returned.
 *
 * @param name The base name
 *
 * @retval NULL if there is no elements to be skipped in the given patch.
 * @retval !NULL the pointer to the name part after the separator.
 */
const char *settings_name_skip(const char *name);

/**
 * @brief Get the length of the first element in patch
 *
 * @param name The patch
 *
 * @return The length of the first element in patch.
 */
size_t settings_name_elen(const char *name);

/**
 * @brief Check if the key matches the expression
 *
 * Simple expression is supported where single asterisk ('*') replaces
 * any single name element between @ref SETTINGS_NAME_SEPARATOR.
 * Double asterisk ('**') can replace one or more elements.
 *
 * The examples of matched patterns using single asterisk:
 * @code
   settings_name_cmp("my_key/other/stuff", "my_key/&ast;/stuff");
   settings_name_cmp("my_key/other/stuff", "&ast;/other/stuff");
   settings_name_cmp("my_key/other/stuff", "my_key/other/&ast;");
 * @endcode
 *
 * The examples of matched patterns using double asterisk:
 * @code
   settings_name_cmp("my_key/other/stuff", "**");
   settings_name_cmp("my_key/other/stuff", "&ast;&ast;/other/stuff");
   settings_name_cmp("my_key/other/stuff", "&ast;&ast;/stuff");
   settings_name_cmp("my_key/other/stuff", "my_key/&ast;&ast;");
 * @endcode
 *
 * Note: single asterisk always means to skip one patch element.
 * Note: double asterisk means to skip any number of elements, also 0.
 *
 * It means that the first line below would fail while in the second
 * true would be returned.
 * @code
   settings_name_cmp("it/is/going", "it/&ast;/is/going");
   settings_name_cmp("it/is/going", "it/&ast;&ast;/is/going");
 * @endcode
 *
 * Some useful patterns:
 *
 * Check if there is exactly one element in given name:
 * @code
   settings_name_cmp("element", "&ast;"); // true
   settings_name_cmp("element/other", "&ast;"); // false
   settings_name_cmp("", "&ast;"); // false
 * @endcode
 *
 * Check if there is at least one element in given name:
 * @code
   settings_name_cmp("element", "&ast;/&ast;&ast;"); // true
   settings_name_cmp("element/other", ""&ast;/&ast;&ast;"); // true
   settings_name_cmp("", "ast;/&ast;&ast;"); // false
 * @endcode
 *
 * @param name The name to compare in a form of setting path
 * @param patt The comparison pattern
 *
 * @retval true  The comparison matched.
 * @retval false The comparison does not matched.
 */
bool settings_name_cmp(const char *name, const char *patt);

/**
 * Parses a key to an array of elements and locate corresponding module handler.
 *
 * @param name Key in string format
 * @param key  Key after first name to be used in the returned settings handler.
 *
 * @return settings_handler node on success, NULL on failure.
 */
struct settings_handler *settings_parse_and_lookup(const char *name,
						   const char **key);


/*
 * API for runtime settings
 */

#ifdef CONFIG_SETTINGS_RUNTIME

/**
 * Set a value with a specific key to a module handler.
 *
 * @param name Key in string format.
 * @param data Binary value.
 * @param len Value length in bytes.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_runtime_set(const char *name, void *data, size_t len);

/**
 * Get a value corresponding to a key from a module handler.
 *
 * @param name Key in string format.
 * @param data Returned binary value.
 * @param len Returned value length in bytes.
 *
 * @return 0 on success, non-zero on failure.
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

#endif /* CONFIG_SETTINGS_RUNTIME */


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SETTINGS_SETTINGS_H_ */
