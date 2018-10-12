/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SETTINGS_SETTINGS_H_
#define ZEPHYR_INCLUDE_SETTINGS_SETTINGS_H_

#include <misc/util.h>
#include <misc/slist.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup settings Settings subsystem
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
 * Type of settings value.
 */
enum settings_type {
	SETTINGS_NONE = 0,
	SETTINGS_INT8,
	SETTINGS_INT16,
	SETTINGS_INT32,
	SETTINGS_INT64,
	SETTINGS_STRING,
	SETTINGS_BYTES,
	SETTINGS_FLOAT,
	SETTINGS_DOUBLE,
	SETTINGS_BOOL,
} __attribute__((__packed__));

/**
 * Parameter to commit handler describing where data is going to.
 */
enum settings_export_tgt {
	SETTINGS_EXPORT_PERSIST,        /* Value is to be persisted. */
	SETTINGS_EXPORT_SHOW            /* Value is to be displayed. */
};

/**
 * @struct settings_handler
 * Config handlers for subtree implement a set of handler functions.
 * These are registered using a call to settings_register().
 *
 * @param settings_handler::node Linked list node info for module internal usage.
 *
 * @param settings_handler::name Name of subtree.
 *
 * @param settings_handler::h_get Get values handler of settings items
 * identified by keyword names.Parameters:
 *  - argc - count of item in argv.
 *  - argv - array of pointers to keyword names.
 *  - val - buffer for a value.
 *  - val_len_max - size of that buffer.
 *
 * @param settings_handler::h_set Set value handler of settings items
 * identified by keyword names. Parameters:
 *  - argc - count of item in argv, argv - array of pointers to keyword names.
 *  - val- pointer to value to be set.
 *
 * @param settings_handler::h_commit This handler gets called after settings
 * has been loaded in full. User might use it to apply setting to
 * the application.
 *
 * @param settings_handler::h_export This gets called to dump all current
 * settings items.
 * This happens when settings_save() tries to save the settings. Parameters:
 *  - tgt: indicates where data is going to.
 *  - Export_function: the pointer to the internal function which appends
 *   a single key-value pair to persisted settings. Don't store duplicated
 *   value. The name is subtree/key string, val is the string with
 *   value.
 *
 * @remarks The User might limit a implementations of handler to serving only
 * one keyword at one call - what will impose limit to get/set values using full
 * subtree/key name.
 */
struct settings_handler {
	sys_snode_t node;
	char *name;
	char *(*h_get)(int argc, char **argv, char *val, int val_len_max);
	int (*h_set)(int argc, char **argv, char *val);
	int (*h_commit)(void);
	int (*h_export)(int (*export_func)(const char *name, char *val),
			enum settings_export_tgt tgt);
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
 * @param var Value of the settings item.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_save_one(const char *name, char *var);

/**
 * Set settings item identified by @p name to be value @p val_str.
 * This finds the settings handler for this subtree and calls it's
 * set handler.
 *
 * @param name Name/key of the settings item.
 * @param val_str Value of the settings item.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_set_value(char *name, char *val_str);

/**
 * Get value of settings item identified by @p name.
 * This calls the settings handler h_get for the subtree.
 *
 * Configuration handler can copy the string to @p buf, the maximum
 * number of bytes it will copy is limited by @p buf_len.
 *
 * @param name Name/key of the settings item.
 *
 * @param buf buffer for value of the settings item.
 * If value is not string, the value will be filled in *buf.
 *
 * @param buf_len size of buf.
 *
 * @return value will be pointer to beginning of the buf,
 * except for string it will pointer to beginning of string source.
 */
char *settings_get_value(char *name, char *buf, int buf_len);

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
 * Convenience routine for converting value passed as a string to native
 * data type.
 *
 * @param val_str Value of the settings item as string.
 * @param type Type of the value to convert to.
 * @param vp Pointer to variable to fill with the decoded value.
 * @param maxlen the vp buffer size.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_value_from_str(char *val_str, enum settings_type type, void *vp,
			    int maxlen);

/**
 * Convenience routine for converting byte array passed as a base64
 * encoded string.
 *
 * @param val_str Value of the settings item as string.
 * @param vp Pointer to variable to fill with the decoded value.
 * @param len Size of that variable. On return the number of bytes in the array.
 *
 * @return 0 on success, non-zero on failure.
 */
int settings_bytes_from_str(char *val_str, void *vp, int *len);

/**
 * Convenience routine for converting native data type to a string.
 *
 * @param type Type of the value to convert from.
 * @param vp Pointer to variable to convert.
 * @param buf Buffer where string value will be stored.
 * @param buf_len Size of the buffer.
 *
 * @return 0 on success, non-zero on failure.
 */
char *settings_str_from_value(enum settings_type type, void *vp, char *buf,
			      int buf_len);
#define SETTINGS_STR_FROM_BYTES_LEN(len) (((len) * 4 / 3) + 4)

/**
 * Convenience routine for converting byte array into a base64
 * encoded string.
 *
 * @param vp Pointer to variable to convert.
 * @param vp_len Number of bytes to convert.
 * @param buf Buffer where string value will be stored.
 * @param buf_len Size of the buffer.
 *
 * @return 0 on success, non-zero on failure.
 */
char *settings_str_from_bytes(void *vp, int vp_len, char *buf, int buf_len);

#define SETTINGS_VALUE_SET(str, type, val)                                  \
	settings_value_from_str((str), (type), &(val), sizeof(val))

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
