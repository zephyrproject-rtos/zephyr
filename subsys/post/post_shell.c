/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief POST shell commands
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/post/post.h>
#include <stdlib.h>

/* Helper to compute test ID from test pointer */
static inline uint32_t post_compute_test_id(const struct post_test *test)
{
	extern struct post_test _post_test_list_start[];
	return (uint32_t)(test - _post_test_list_start);
}

static int cmd_post_list(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "%-4s %-24s %-8s %-12s %-20s",
		    "ID", "Name", "Cat", "Level", "Flags");
	shell_print(sh, "---- ------------------------ -------- "
		    "------------ --------------------");

	STRUCT_SECTION_FOREACH(post_test, test) {
		const char *level_str;

		switch (test->init_level) {
		case POST_LEVEL_EARLY:
			level_str = "EARLY";
			break;
		case POST_LEVEL_PRE_KERNEL_1:
			level_str = "PRE_KERNEL_1";
			break;
		case POST_LEVEL_PRE_KERNEL_2:
			level_str = "PRE_KERNEL_2";
			break;
		case POST_LEVEL_POST_KERNEL:
			level_str = "POST_KERNEL";
			break;
		case POST_LEVEL_APPLICATION:
			level_str = "APPLICATION";
			break;
		default:
			level_str = "UNKNOWN";
			break;
		}

		char flags_str[32] = "";

		if (test->flags & POST_FLAG_RUNTIME_OK) {
			strcat(flags_str, "RT ");
		}
		if (test->flags & POST_FLAG_USERSPACE_OK) {
			strcat(flags_str, "US ");
		}
		if (test->flags & POST_FLAG_CRITICAL) {
			strcat(flags_str, "CRIT ");
		}
		if (test->flags & POST_FLAG_DESTRUCTIVE) {
			strcat(flags_str, "DESTR ");
		}

		uint32_t test_id = post_compute_test_id(test);
		shell_print(sh, "%04x %-24s %04x     %-12s %-20s",
			    test_id, test->name, test->category,
			    level_str, flags_str);
	}

	return 0;
}

static int cmd_post_run(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(sh, "Usage: post run <test_id_hex> | --all | --category <cat>");
		return -EINVAL;
	}

	if (strcmp(argv[1], "--all") == 0) {
		int failures = 0;

		shell_print(sh, "Running all runtime-safe tests...");
		STRUCT_SECTION_FOREACH(post_test, test) {
			if (test->flags & POST_FLAG_RUNTIME_OK) {
				uint32_t test_id = post_compute_test_id(test);
				enum post_result result = post_run_test(test_id);

				if (result == POST_RESULT_FAIL ||
				    result == POST_RESULT_ERROR) {
					failures++;
				}
			}
		}
		shell_print(sh, "Complete. %d failures.", failures);
		return 0;
	}

	if (strcmp(argv[1], "--category") == 0) {
		if (argc < 3) {
			shell_error(sh, "Category required (cpu, ram, stack, flash)");
			return -EINVAL;
		}

		uint32_t category;

		if (strcmp(argv[2], "cpu") == 0) {
			category = POST_CAT_CPU;
		} else if (strcmp(argv[2], "ram") == 0) {
			category = POST_CAT_RAM;
		} else if (strcmp(argv[2], "stack") == 0) {
			category = POST_CAT_STACK;
		} else if (strcmp(argv[2], "flash") == 0) {
			category = POST_CAT_FLASH;
		} else {
			shell_error(sh, "Unknown category: %s", argv[2]);
			return -EINVAL;
		}

		int failures = post_run_category(category);

		shell_print(sh, "Category tests complete. %d failures.", failures);
		return 0;
	}

	/* Parse test ID as hex */
	char *endptr;
	unsigned long test_id = strtoul(argv[1], &endptr, 16);

	if (*endptr != '\0') {
		shell_error(sh, "Invalid test ID: %s", argv[1]);
		return -EINVAL;
	}

	const struct post_test *test = post_get_test((uint32_t)test_id);

	if (test == NULL) {
		shell_error(sh, "Test ID 0x%04lx not found", test_id);
		return -ENOENT;
	}

	if (!(test->flags & POST_FLAG_RUNTIME_OK)) {
		shell_error(sh, "Test '%s' is not safe for runtime execution",
			    test->name);
		return -EPERM;
	}

	shell_print(sh, "Running test: %s (0x%04lx)...", test->name, test_id);

	enum post_result result = post_run_test((uint32_t)test_id);

	switch (result) {
	case POST_RESULT_PASS:
		shell_print(sh, "Result: PASS");
		break;
	case POST_RESULT_FAIL:
		shell_error(sh, "Result: FAIL");
		break;
	case POST_RESULT_SKIP:
		shell_warn(sh, "Result: SKIP");
		break;
	case POST_RESULT_ERROR:
		shell_error(sh, "Result: ERROR");
		break;
	default:
		shell_error(sh, "Result: UNKNOWN");
		break;
	}

	return 0;
}

static int cmd_post_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	uint32_t passed, failed, skipped;
	int total = post_get_summary(&passed, &failed, &skipped);

	shell_print(sh, "POST Summary:");
	shell_print(sh, "  Total:   %d", total);
	shell_print(sh, "  Passed:  %u", passed);
	shell_print(sh, "  Failed:  %u", failed);
	shell_print(sh, "  Skipped: %u", skipped);

	if (failed > 0) {
		shell_error(sh, "  STATUS: FAIL");
		shell_print(sh, "\nFailed Tests:");
		shell_print(sh, "%-4s %-24s %-10s %-10s", "ID", "Name", "ErrCode", "ErrData");
		STRUCT_SECTION_FOREACH(post_test, test) {
			uint32_t test_id = post_compute_test_id(test);
			struct post_result_record record;
			if (post_get_result(test_id, &record) == 0) {
				if (record.result == POST_RESULT_FAIL || 
				    record.result == POST_RESULT_ERROR) {
					shell_error(sh, "%04x %-24s 0x%08x 0x%08x", 
						    test_id, test->name, 
						    record.error_code, record.error_data);
				}
			}
		}
	} else {
		shell_print(sh, "  STATUS: OK");
	}

	if (skipped > 0) {
		shell_print(sh, "\nSkipped Tests:");
		STRUCT_SECTION_FOREACH(post_test, test) {
			uint32_t test_id = post_compute_test_id(test);
			struct post_result_record record;
			if (post_get_result(test_id, &record) == 0 && 
			    record.result == POST_RESULT_SKIP) {
				shell_warn(sh, "%04x %-24s", test_id, test->name);
			}
		}
	}

	if (passed > 0) {
		shell_print(sh, "\nPassed Tests:");
		STRUCT_SECTION_FOREACH(post_test, test) {
			uint32_t test_id = post_compute_test_id(test);
			struct post_result_record record;
			if (post_get_result(test_id, &record) == 0 && 
			    record.result == POST_RESULT_PASS) {
				shell_print(sh, "%04x %-24s", test_id, test->name);
			}
		}
	}

	return 0;
}

static int cmd_post_results(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "%-4s %-24s %-8s %-10s",
		    "ID", "Name", "Result", "Time (us)");
	shell_print(sh, "---- ------------------------ -------- ----------");

	STRUCT_SECTION_FOREACH(post_test, test) {
		uint32_t test_id = post_compute_test_id(test);
		struct post_result_record record;
		int ret = post_get_result(test_id, &record);
		const char *result_str;

		if (ret != 0) {
			result_str = "NOT_RUN";
		} else {
			switch (record.result) {
			case POST_RESULT_PASS:
				result_str = "PASS";
				break;
			case POST_RESULT_FAIL:
				result_str = "FAIL";
				break;
			case POST_RESULT_SKIP:
				result_str = "SKIP";
				break;
			case POST_RESULT_ERROR:
				result_str = "ERROR";
				break;
			default:
				result_str = "UNKNOWN";
				break;
			}
		}

		if (ret == 0) {
			shell_print(sh, "%04x %-24s %-8s %llu",
				    test_id, test->name, result_str,
				    record.duration_us);
		} else {
			shell_print(sh, "%04x %-24s %-8s -",
				    test_id, test->name, result_str);
		}
	}

	return 0;
}

/* Shell command hierarchy */
SHELL_STATIC_SUBCMD_SET_CREATE(post_cmds,
	SHELL_CMD(list, NULL, "List all registered POST tests", cmd_post_list),
	SHELL_CMD(run, NULL, "Run a POST test: <id> | --all | --category <cat>",
		  cmd_post_run),
	SHELL_CMD(status, NULL, "Show POST summary status", cmd_post_status),
	SHELL_CMD(results, NULL, "Show detailed POST results", cmd_post_results),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(post, &post_cmds, "POST subsystem commands", NULL);
