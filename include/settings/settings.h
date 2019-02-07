/*
 * Copyright (c) 2019 Laczen
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

#ifdef CONFIG_SETTINGS_NVS
#include <nvs/nvs.h>
#endif

#ifdef CONFIG_SETTINGS_FCB
#include <fcb.h>
#endif

#ifdef CONFIG_SETTINGS_FS
#include <fs.h>
#endif

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

typedef ssize_t (*settings_read_fn)(void *back_end, void *data, size_t len);

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

	bool disabled;
	/**< Status of subtree. A disabled subtree will not be loaded. */

	int (*h_get)(int argc, char **argv, void *val, int val_len_max);
	/**< Get values handler of settings items identified by keyword names.
	 *
	 * Parameters:
	 *  - argc - count of item in argv.
	 *  - argv - array of pointers to keyword names.
	 *  - val - buffer for a value.
	 *  - val_len_max - size of that buffer.
	 */

	int (*h_set)(int argc, char **argv, size_t len, settings_read_fn read,
		     void *back_end);
	/**< Set value handler of settings items identified by keyword names.
	 *
	 * Parameters:
	 *  - argc - count of item in argv, argv - array of pointers to keyword
	 *   names.
	 *  - len - the size of the data found in the backend
	 *  - read - function provided to read the data from the backend
	 *  - cb_arg - arguments for the read function provided by the backend
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
 * Save currently running serialized items. All serialized items which are
 * different from currently persisted values will be saved.
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
 * Config storage
 */

struct settings_store_itf;
struct settings_store {
	sys_snode_t cs_next;
	const struct settings_store_itf *cs_itf;
};

struct settings_store_itf {
	int (*csi_load)(struct settings_store *cs);
	int (*csi_save_start)(struct settings_store *cs);
	int (*csi_save)(struct settings_store *cs, const char *name,
			const char *value, size_t val_len);
	int (*csi_save_end)(struct settings_store *cs);
};

/*
 * API for handler lookup
 */

struct settings_handler *settings_parse_and_lookup(char *name, int *name_argc,
						   char *name_argv[]);

/*
 * API for config storage.
 */

void settings_src_register(struct settings_store *cs);
void settings_dst_register(struct settings_store *cs);

#ifdef CONFIG_SETTINGS_RUNTIME

int settings_runtime_set(const char *name, void *data, size_t len);

int settings_runtime_get(const char *name, void *data, size_t len);

int settings_runtime_commit(const char *name);

#endif /* CONFIG_SETTINGS_RUNTIME */

#ifdef CONFIG_SETTINGS_FCB

#define SETTINGS_FCB_VERS		1

struct settings_fcb {
	struct settings_store cf_store;
	struct fcb cf_fcb;
	int fa_id; /* flash area id */
};
/* register fcb to be a source of settings */
int settings_fcb_src(struct settings_fcb *cf);

/* register fcb to be the destination of settings */
int settings_fcb_dst(struct settings_fcb *cf);

/* initialize a settings fcb  backend */
int settings_fcb_backend_init(struct settings_fcb *cf);
#endif /* CONFIG_SETTINGS_FCB */

#ifdef CONFIG_SETTINGS_FS

#define SETTINGS_FILE_NAME_MAX 32 /* max length for settings filename */

struct settings_file {
	struct settings_store cf_store;
	const char *cf_name;	/* filename */
	int cf_maxlines;	/* max # of lines before compressing */
	int cf_lines;		/* private */
	u8_t toggle;		/* private 0 or 1 used to extend the filename */
};

/* register file to be a source of settings */
int settings_file_src(struct settings_file *cf);

/* settings file to be the destination of settings */
int settings_file_dst(struct settings_file *cf);

/* initialize a settings file  backend */
int settings_file_backend_init(struct settings_file *cf);

#endif /* CONFIG_SETTINGS_FS */


#ifdef CONFIG_SETTINGS_NVS

/* In nvs the settings data is stored as:
 *	a. name at id starting from NVS_NAMECNT_ID + 1
 *	b. value at id of name + NVS_NAME_ID_OFFSET
 * Deleted records will not be found, only the last record will be
 * read. The entry ad NVS_NAMECNT_ID is used to store the largest
 * name id in use.
 */
#define NVS_NAMECNT_ID 0x8000
#define NVS_NAME_ID_OFFSET 0x4000

struct settings_nvs {
	struct settings_store cf_store;
	struct nvs_fs cf_nvs;
	u16_t last_name_id;
};

/* register nvs to be a source of settings */
int settings_nvs_src(struct settings_nvs *cf);

/* register nvs to be the destination of settings */
int settings_nvs_dst(struct settings_nvs *cf);

/* Initialize a nvs backend. */
int settings_nvs_backend_init(struct settings_nvs *cf);

#endif /* CONFIG_SETTINGS_NVS */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SETTINGS_SETTINGS_H_ */
