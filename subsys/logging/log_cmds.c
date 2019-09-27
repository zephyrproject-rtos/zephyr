/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>
#include <string.h>
#include <logging/log_link.h>
LOG_MODULE_REGISTER(log_cmds);

typedef int (*log_backend_cmd_t)(const struct shell *shell,
				 const struct log_backend *backend,
				 size_t argc,
				 char **argv);

static const char * const severity_lvls[] = {
	"none",
	"err",
	"wrn",
	"inf",
	"dbg",
};

static const char * const severity_lvls_sorted[] = {
	"dbg",
	"err",
	"inf",
	"none",
	"wrn",
};

/**
 * @brief Function for finding backend instance with given name.
 *
 * @param p_name Name of the backend instance.
 *
 * @return Pointer to the instance or NULL.
 *
 */
static const struct log_backend *backend_find(char const *name)
{
	const struct log_backend *backend;
	size_t slen = strlen(name);

	for (int i = 0; i < log_backend_count_get(); i++) {
		backend = log_backend_get(i);
		if (strncmp(name, backend->name, slen) == 0) {
			return backend;
		}
	}

	return NULL;
}

static bool shell_state_precheck(const struct shell *shell)
{
	if (shell->log_backend->control_block->state
				== SHELL_LOG_BACKEND_UNINIT) {
		shell_error(shell, "Shell log backend not initialized.");
		return false;
	}

	return true;
}

/**
 * @brief Function for executing command on given backend.
 */
static int shell_backend_cmd_execute(const struct shell *shell,
				     size_t argc,
				     char **argv,
				     log_backend_cmd_t func)
{
	/* Based on the structure of backend commands, name of the backend can
	 * be found at -1 (log backend <name> command).
	 */
	char const *name = argv[-1];
	const struct log_backend *backend = backend_find(name);

	if (backend != NULL) {
		func(shell, backend, argc, argv);
	} else {
		shell_error(shell, "Invalid backend: %s", name);
		return -ENOEXEC;
	}
	return 0;
}

static char * get_domain_source_name(char * buf, int len, u8_t abs_domain_id,
				     u16_t source_id)
{
	const char *dname;
	int dname_len;
	int sname_maxlen;

	if (z_log_is_local_domain(abs_domain_id)) {
		return (char *)log_source_name_get(NULL, 0, abs_domain_id,
							source_id);
	}

	dname = log_domain_name_get(abs_domain_id);
	dname_len = strlen(dname);
	sname_maxlen = len - (dname_len + 1);
	memcpy(buf, dname, dname_len);
	buf[dname_len++] = '/';
	log_source_name_get(&buf[dname_len], sname_maxlen,
			    abs_domain_id, source_id);
	return buf;
}

static void print_source_status(const struct shell *shell,
				const struct log_backend *backend,
				u8_t domain_id, u16_t source_id)
{
	u32_t dynamic_lvl = log_filter_get(backend, domain_id, source_id, true);
	u32_t static_lvl = log_filter_get(backend, domain_id, source_id, false);
	char buf[96];
	const char *src_name;

	src_name = get_domain_source_name(buf, sizeof(buf),
					  domain_id, source_id);

	shell_fprintf(shell, SHELL_NORMAL, "%-40s | %-7s | %s\r\n", src_name,
			severity_lvls[dynamic_lvl], severity_lvls[static_lvl]);
}

typedef bool (*on_each_source_fn_t)(u8_t domain_id, u16_t source_id,
		void *arg0, void *arg1, void *arg2);

/* Helper macro for iterating over all sources */
#define LOG_FOR_EACH_SOURCE(code) \
	for (u8_t d = 0; d < log_domains_count(); d++) { \
		for (u16_t s = 0; s < log_sources_count(d); s++) { \
			code; \
		} \
	}

static int log_status(const struct shell *shell,
		      const struct log_backend *backend,
		      size_t argc, char **argv)
{
	if (!log_backend_is_active(backend)) {
		shell_warn(shell, "Logs are halted!");
	}

	shell_fprintf(shell, SHELL_NORMAL, "%-40s | current | built-in \r\n",
					   "module_name");
	shell_fprintf(shell, SHELL_NORMAL,
	      "----------------------------------------------------------\r\n");

	LOG_FOR_EACH_SOURCE(print_source_status(shell, backend, d, s));

	return 0;
}


static int cmd_log_self_status(const struct shell *shell,
			       size_t argc, char **argv)
{
	if (!shell_state_precheck(shell)) {
		return 0;
	}

	log_status(shell, shell->log_backend->backend, argc, argv);
	return 0;
}

static int cmd_log_backend_status(const struct shell *shell,
				  size_t argc, char **argv)
{
	shell_backend_cmd_execute(shell, argc, argv, log_status);
	return 0;
}

static int source_find(const char *name, u8_t *abs_domain_id,
			u16_t *src_domain_id)
{
	char buf[96];

	LOG_FOR_EACH_SOURCE(
		if (strcmp(name,
			get_domain_source_name(buf, sizeof(buf), d, s)) == 0) {
			*abs_domain_id = d;
			*src_domain_id = s;
			return 0;
		}
	)

	return -EINVAL;
}

static void source_set_level(const struct shell *shell,
			     const struct log_backend *backend,
			     u8_t abs_domain_id, u16_t source_id, u32_t level)
{
	u32_t set_lvl;

	set_lvl = log_filter_set(backend, abs_domain_id, source_id, level);
	if (set_lvl != level) {
		char buf[96];
		const char *name;

		name = log_source_name_get(buf, sizeof(buf),
					   abs_domain_id, source_id);
		shell_warn(shell, "%s: level set to %s.",
			   buf, severity_lvls[set_lvl]);
	}
}

static void named_source_set_level(const struct shell *shell,
				   const struct log_backend *backend,
				   const char *name, u32_t level)
{
	u8_t abs_domain_id = 0;
	u16_t source_id = 0;
	int err;

	err = source_find(name, &abs_domain_id, &source_id);
	if (err != 0) {
		shell_error(shell, "%s: unknown source name.", name);
		return;
	}

	source_set_level(shell, backend, abs_domain_id, source_id, level);
}

static void filters_set(const struct shell *shell,
			const struct log_backend *backend,
			size_t argc, char **argv, u32_t level)
{
	if (!backend->cb->active) {
		shell_warn(shell, "Backend not active.");
	}

	if (argc == 0) {
		/* Set level for all modules if no module name specified. */
		LOG_FOR_EACH_SOURCE(source_set_level(shell, backend, d, s, level));
	} else {
		for (size_t i = 0; i < argc; i++) {
			named_source_set_level(shell, backend, argv[i], level);
		}
	}
}

static int severity_level_get(const char *str)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(severity_lvls); i++) {
		if (strncmp(str, severity_lvls[i], 4) == 0) {
			return i;
		}
	}

	return -1;
}
static int log_enable(const struct shell *shell,
		      const struct log_backend *backend,
		      size_t argc,
		      char **argv)
{
	int severity_level;

	severity_level = severity_level_get(argv[1]);

	if (severity_level < 0) {
		shell_error(shell, "Invalid severity: %s", argv[1]);
		return -ENOEXEC;
	}

	/* Arguments following severity level are interpreted as module names.*/
	filters_set(shell, backend, argc - 2, &argv[2], severity_level);
	return 0;
}

static int cmd_log_self_enable(const struct shell *shell,
			       size_t argc, char **argv)
{
	if (!shell_state_precheck(shell)) {
		return 0;
	}

	return log_enable(shell, shell->log_backend->backend, argc, argv);
}

static int cmd_log_backend_enable(const struct shell *shell,
				  size_t argc, char **argv)
{
	return shell_backend_cmd_execute(shell, argc, argv, log_enable);
}

static int log_disable(const struct shell *shell,
		       const struct log_backend *backend,
		       size_t argc,
		       char **argv)
{
	filters_set(shell, backend, argc - 1, &argv[1], LOG_LEVEL_NONE);
	return 0;
}

static int cmd_log_self_disable(const struct shell *shell,
				 size_t argc, char **argv)
{
	if (!shell_state_precheck(shell)) {
		return 0;
	}

	return log_disable(shell, shell->log_backend->backend, argc, argv);
}

static int cmd_log_backend_disable(const struct shell *shell,
				   size_t argc, char **argv)
{
	return shell_backend_cmd_execute(shell, argc, argv, log_disable);
}

static void module_name_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dsub_module_name, module_name_get);

static void idx_to_domain_source(size_t idx, u8_t *domain_id, u16_t *source_id)
{
	for(u8_t d = 0; d < log_domains_count(); d++) {
		u16_t cnt = log_sources_count(d);
		if (idx < cnt) {
			*domain_id = d;
			*source_id = idx;
			return;
		}
		idx -= cnt;
	}
}

static void module_name_get(size_t idx, struct shell_static_entry *entry)
{
	u8_t d = 0;
	u16_t s = idx;
	/* Note that if name is from remote domain then function is not
	 * re-entrant and it will lead to invalid data being printed on one
	 * shell if two shell instances execute autocompletion of log command
	 * at once. It is rather unlikely thus risk is taken.
	 */
	static char buf[96];

	idx_to_domain_source(idx, &d, &s);
	entry->handler = NULL;
	entry->help  = NULL;
	entry->subcmd = &dsub_module_name;
	entry->syntax = get_domain_source_name(buf, sizeof(buf), d, s);
}

static void severity_lvl_get(size_t idx, struct shell_static_entry *entry)
{
	entry->handler = NULL;
	entry->help  = NULL;
	entry->subcmd = &dsub_module_name;
	entry->syntax = (idx < ARRAY_SIZE(severity_lvls_sorted)) ?
					severity_lvls_sorted[idx] : NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_severity_lvl, severity_lvl_get);

static int log_halt(const struct shell *shell,
		    const struct log_backend *backend,
		    size_t argc,
		    char **argv)
{
	log_backend_deactivate(backend);
	return 0;
}


static int cmd_log_self_halt(const struct shell *shell,
			      size_t argc, char **argv)
{
	if (!shell_state_precheck(shell)) {
		return 0;
	}

	return log_halt(shell, shell->log_backend->backend, argc, argv);
}

static int cmd_log_backend_halt(const struct shell *shell,
				size_t argc, char **argv)
{
	return shell_backend_cmd_execute(shell, argc, argv, log_halt);
}

static int log_go(const struct shell *shell,
		  const struct log_backend *backend,
		  size_t argc,
		  char **argv)
{
	log_backend_activate(backend, backend->cb->ctx);
	return 0;
}


static int cmd_log_self_go(const struct shell *shell,
			   size_t argc, char **argv)
{
	if (!shell_state_precheck(shell)) {
		return 0;
	}

	return log_go(shell, shell->log_backend->backend, argc, argv);
}

static int cmd_log_backend_go(const struct shell *shell,
			      size_t argc, char **argv)
{
	return shell_backend_cmd_execute(shell, argc, argv, log_go);
}


static int cmd_log_backends_list(const struct shell *shell,
				 size_t argc, char **argv)
{
	int backend_count;

	backend_count = log_backend_count_get();

	for (int i = 0; i < backend_count; i++) {
		const struct log_backend *backend = log_backend_get(i);

		shell_fprintf(shell, SHELL_NORMAL,
			      "%s\r\n"
			      "\t- Status: %s\r\n"
			      "\t- ID: %d\r\n\r\n",
			      backend->name,
			      backend->cb->active ? "enabled" : "disabled",
			      backend->cb->id);

	}
	return 0;
}

static int cmd_log_strdup_utilization(const struct shell *shell,
				      size_t argc, char **argv)
{

	/* Defines needed when string duplication is disabled (LOG_IMMEDIATE is
	 * on). In that case, this function is not compiled in.
	 */
	#ifndef CONFIG_LOG_STRDUP_BUF_COUNT
	#define CONFIG_LOG_STRDUP_BUF_COUNT 0
	#endif

	#ifndef CONFIG_LOG_STRDUP_MAX_STRING
	#define CONFIG_LOG_STRDUP_MAX_STRING 0
	#endif

	u32_t buf_cnt = log_get_strdup_pool_utilization();
	u32_t buf_size = log_get_strdup_longest_string();
	u32_t percent = CONFIG_LOG_STRDUP_BUF_COUNT ?
			100 * buf_cnt / CONFIG_LOG_STRDUP_BUF_COUNT : 0;

	shell_print(shell,
		"Maximal utilization of the buffer pool: %d / %d (%d %%).",
		buf_cnt, CONFIG_LOG_STRDUP_BUF_COUNT, percent);
	if (buf_cnt == CONFIG_LOG_STRDUP_BUF_COUNT) {
		shell_warn(shell, "Buffer count too small.");
	}

	shell_print(shell,
		"Longest duplicated string: %d, buffer capacity: %d.",
		buf_size, CONFIG_LOG_STRDUP_MAX_STRING);
	if (buf_size > CONFIG_LOG_STRDUP_MAX_STRING) {
		shell_warn(shell, "Buffer size too small.");

	}

	return 0;
}


SHELL_STATIC_SUBCMD_SET_CREATE(sub_log_backend,
	SHELL_CMD_ARG(disable, &dsub_module_name,
		  "'log disable <module_0> .. <module_n>' disables logs in "
		  "specified modules (all if no modules specified).",
		  cmd_log_backend_disable, 2, 255),
	SHELL_CMD_ARG(enable, &dsub_severity_lvl,
		  "'log enable <level> <module_0> ...  <module_n>' enables logs"
		  " up to given level in specified modules (all if no modules "
		  "specified).",
		  cmd_log_backend_enable, 2, 255),
	SHELL_CMD(go, NULL, "Resume logging", cmd_log_backend_go),
	SHELL_CMD(halt, NULL, "Halt logging", cmd_log_backend_halt),
	SHELL_CMD(status, NULL, "Logger status", cmd_log_backend_status),
	SHELL_SUBCMD_SET_END
);

static void backend_name_get(size_t idx, struct shell_static_entry *entry)
{
	entry->handler = NULL;
	entry->help  = NULL;
	entry->subcmd = &sub_log_backend;
	entry->syntax  = NULL;

	if (idx < log_backend_count_get()) {
		const struct log_backend *backend = log_backend_get(idx);

		entry->syntax = backend->name;
	}
}

SHELL_DYNAMIC_CMD_CREATE(dsub_backend_name_dynamic, backend_name_get);


SHELL_STATIC_SUBCMD_SET_CREATE(sub_log_stat,
	SHELL_CMD(backend, &dsub_backend_name_dynamic,
			"Logger backends commands.", NULL),
	SHELL_CMD_ARG(disable, &dsub_module_name,
	"'log disable <module_0> .. <module_n>' disables logs in specified "
	"modules (all if no modules specified).",
	cmd_log_self_disable, 2, 255),
	SHELL_CMD_ARG(enable, &dsub_severity_lvl,
	"'log enable <level> <module_0> ...  <module_n>' enables logs up to"
	" given level in specified modules (all if no modules specified).",
	cmd_log_self_enable, 2, 255),
	SHELL_CMD(go, NULL, "Resume logging", cmd_log_self_go),
	SHELL_CMD(halt, NULL, "Halt logging", cmd_log_self_halt),
	SHELL_CMD_ARG(list_backends, NULL, "Lists logger backends.",
		      cmd_log_backends_list, 1, 0),
	SHELL_CMD(status, NULL, "Logger status", cmd_log_self_status),
	SHELL_COND_CMD_ARG(CONFIG_LOG_STRDUP_POOL_PROFILING, strdup_utilization,
			NULL, "Get utilization of string duplicates pool",
			cmd_log_strdup_utilization, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(log, &sub_log_stat, "Commands for controlling logger",
		   NULL);
