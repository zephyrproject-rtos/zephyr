/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <zephyr/arch/posix/posix_trace.h>
#include "posix_board_if.h"
#include "zephyr/types.h"
#include "cmdline_common.h"

/**
 * Check if <arg> is the option <option>
 * The accepted syntax is:
 *   * For options without a value following:
 *       [-[-]]<option>
 *   * For options with value:
 *       [-[-]]<option>{:|=}<value>
 *
 * Returns 0 if it is not, or a number > 0 if it is.
 * The returned number is the number of characters it went through
 * to find the end of the option including the ':' or '=' character in case of
 * options with value
 */
int cmd_is_option(const char *arg, const char *option, int with_value)
{
	int of = 0;
	size_t to_match_len = strlen(option);

	if (arg[of] == '-') {
		of++;
	}
	if (arg[of] == '-') {
		of++;
	}

	if (!with_value) {
		if (strcmp(&arg[of], option) != 0) {
			return 0;
		} else {
			return of + to_match_len;
		}
	}

	while (!(arg[of] == 0 && *option == 0)) {
		if (*option == 0) {
			if ((arg[of] == ':') || (arg[of] == '=')) {
				of++;
				break;
			}
			return 0;
		}
		if (arg[of] != *option) {
			return 0;
		}
		of++;
		option++;
	}

	if (arg[of] == 0) { /* we need a value to follow */
		posix_print_error_and_exit("Incorrect option syntax '%s'. The "
					   "value should follow the options. "
					   "For example --ratio=3\n",
					   arg);
	}
	return of;
}

/**
 * Return 1 if <arg> matches an accepted help option.
 * 0 otherwise
 *
 * Valid help options are [-[-]]{?|h|help}
 * with the h or help in any case combination
 */
int cmd_is_help_option(const char *arg)
{
	if (arg[0] == '-') {
		arg++;
	}
	if (arg[0] == '-') {
		arg++;
	}
	if ((strcasecmp(arg, "?") == 0) ||
	    (strcasecmp(arg, "h") == 0) ||
	    (strcasecmp(arg, "help") == 0)) {
		return 1;
	} else {
		return 0;
	}
}

#define CMD_TYPE_ERROR "Coding error: type %c not understood"
#define CMD_ERR_BOOL_SWI "Programming error: I only know how to "\
	"automatically read boolean switches\n"

/**
 * Read out a the value following an option from str, and store it into
 * <dest>
 * <type> indicates the type of parameter (and type of dest pointer)
 *   'b' : boolean
 *   's' : string (char *)
 *   'u' : 32 bit unsigned integer
 *   'U' : 64 bit unsigned integer
 *   'i' : 32 bit signed integer
 *   'I' : 64 bit signed integer
 *   'd' : *double* float
 *
 * Note: list type ('l') cannot be handled by this function and must always be
 *       manual
 *
 *  <long_d> is the long name of the option
 */
void cmd_read_option_value(const char *str, void *dest, const char type,
			   const char *option)
{
	int error = 0;
	char *endptr = NULL;

	switch (type) {
	case 'b':
		if (strcasecmp(str, "false") == 0) {
			*(bool *)dest = false;
			endptr = (char *)str + 5;
		} else if (strcmp(str, "0") == 0) {
			*(bool *)dest = false;
			endptr = (char *)str + 1;
		} else if (strcasecmp(str, "true") == 0) {
			*(bool *)dest = true;
			endptr = (char *)str + 4;
		} else if (strcmp(str, "1") == 0) {
			*(bool *)dest = true;
			endptr = (char *)str + 1;
		} else {
			error = 1;
		}
		break;
	case 's':
		*(char **)dest = (char *)str;
		endptr = (char *)str + strlen(str);
		break;
	case 'u':
		*(uint32_t *)dest = strtoul(str, &endptr, 0);
		break;
	case 'U':
		*(uint64_t *)dest = strtoull(str, &endptr, 0);
		break;
	case 'i':
		*(int32_t *)dest = strtol(str, &endptr, 0);
		break;
	case 'I':
		*(int64_t *)dest = strtoll(str, &endptr, 0);
		break;
	case 'd':
		*(double *)dest = strtod(str, &endptr);
		break;
	default:
		posix_print_error_and_exit(CMD_TYPE_ERROR, type);
		/* Unreachable */
		break;
	}

	if (!error && endptr && *endptr != 0) {
		error = 1;
	}

	if (error) {
		posix_print_error_and_exit("Error reading value of %s '%s'. Use"
					   " --help for usage information\n",
					   option, str);
	}
}

/**
 * Initialize existing dest* to defaults based on type
 */
void cmd_args_set_defaults(struct args_struct_t args_struct[])
{
	int count = 0;

	while (args_struct[count].option != NULL) {

		if (args_struct[count].dest == NULL) {
			count++;
			continue;
		}

		switch (args_struct[count].type) {
		case 0: /* does not have storage */
			break;
		case 'b':
			*(bool *)args_struct[count].dest = false;
			break;
		case 's':
			*(char **)args_struct[count].dest = NULL;
			break;
		case 'u':
			*(uint32_t *)args_struct[count].dest = UINT32_MAX;
			break;
		case 'U':
			*(uint64_t *)args_struct[count].dest = UINT64_MAX;
			break;
		case 'i':
			*(int32_t *)args_struct[count].dest = INT32_MAX;
			break;
		case 'I':
			*(int64_t *)args_struct[count].dest = INT64_MAX;
			break;
		case 'd':
			*(double *)args_struct[count].dest = (double)NAN;
			break;
		default:
			posix_print_error_and_exit(CMD_TYPE_ERROR,
						   args_struct[count].type);
			break;
		}
		count++;
	}
}

/**
 * For the help messages:
 * Generate a string containing how the option described by <args_s_el>
 * should be used
 *
 * The string is saved in <buf> which has been allocated <size> bytes by the
 * caller
 */
static void cmd_gen_switch_syntax(char *buf, int size,
				  struct args_struct_t *args_s_el)
{
	int ret = 0;

	if (size <= 0) {
		return;
	}

	if (args_s_el->is_mandatory == false) {
		*buf++ = '[';
		size--;
	}

	if (args_s_el->is_switch == true) {
		ret = snprintf(buf, size, "-%s", args_s_el->option);
	} else {
		if (args_s_el->type != 'l') {
			ret = snprintf(buf, size, "-%s=<%s>",
					args_s_el->option, args_s_el->name);
		} else {
			ret = snprintf(buf, size, "-%s <%s>...",
					args_s_el->option, args_s_el->name);
		}
	}

	if (ret < 0) {
		posix_print_error_and_exit("Unexpected error in %s %i\n",
					   __FILE__, __LINE__);
	}
	if (size - ret < 0) {
		/*
		 * If we run out of space we can just stop,
		 * this is not critical
		 */
		return;
	}
	buf += ret;
	size -= ret;

	if (args_s_el->is_mandatory == false) {
		snprintf(buf, size, "] ");
	} else {
		snprintf(buf, size, " ");
	}
}

/**
 * Print short list of available switches
 */
void cmd_print_switches_help(struct args_struct_t args_struct[])
{
	int count = 0;
	int printed_in_line = strlen(_HELP_SWITCH) + 1;

	fprintf(stdout, "%s ", _HELP_SWITCH);

	while (args_struct[count].option != NULL) {
		char stringy[_MAX_STRINGY_LEN];

		cmd_gen_switch_syntax(stringy, _MAX_STRINGY_LEN,
				      &args_struct[count]);

		if (printed_in_line + strlen(stringy) > _MAX_LINE_WIDTH) {
			fprintf(stdout, "\n");
			printed_in_line = 0;
		}

		fprintf(stdout, "%s", stringy);
		printed_in_line += strlen(stringy);
		count++;
	}

	fprintf(stdout, "\n");
}

/**
 * Print the long help message of the program
 */
void cmd_print_long_help(struct args_struct_t args_struct[])
{
	int ret;
	int count = 0;
	int printed_in_line = 0;
	char stringy[_MAX_STRINGY_LEN];

	cmd_print_switches_help(args_struct);

	fprintf(stdout, "\n %-*s:%s\n", _LONG_HELP_ALIGN-1,
		_HELP_SWITCH, _HELP_DESCR);

	while (args_struct[count].option != NULL) {
		int printed_right;
		char *toprint;
		int total_to_print;

		cmd_gen_switch_syntax(stringy, _MAX_STRINGY_LEN,
				      &args_struct[count]);

		ret = fprintf(stdout, " %-*s:", _LONG_HELP_ALIGN-1, stringy);
		printed_in_line = ret;
		printed_right = 0;
		toprint = args_struct[count].descript;
		total_to_print = strlen(toprint);
		ret = fprintf(stdout, "%.*s\n",
				_MAX_LINE_WIDTH - printed_in_line,
				&toprint[printed_right]);
		printed_right += ret - 1;

		while (printed_right < total_to_print) {
			fprintf(stdout, "%*s", _LONG_HELP_ALIGN, "");
			ret = fprintf(stdout, "%.*s\n",
				      _MAX_LINE_WIDTH - _LONG_HELP_ALIGN,
				      &toprint[printed_right]);
			printed_right += ret - 1;
		}
		count++;
	}
	fprintf(stdout, "\n");
	fprintf(stdout, "Note that which options are available depends on the "
		"enabled features/drivers\n\n");
}

/*
 * <argv> matched the argument described in <arg_element>
 *
 * If arg_element->dest points to a place to store a possible value, read it
 * If there is a callback registered, call it after
 */
static void cmd_handle_this_matched_arg(char *argv, int offset,
					struct args_struct_t *arg_element)
{
	if (arg_element->dest != NULL) {
		if (arg_element->is_switch) {
			if (arg_element->type == 'b') {
				*(bool *)arg_element->dest = true;
			} else {
				posix_print_error_and_exit(CMD_ERR_BOOL_SWI);
			}
		} else { /* if not a switch we need to read its value */
			cmd_read_option_value(&argv[offset],
					      arg_element->dest,
					      arg_element->type,
					      arg_element->option);
		}
	}

	if (arg_element->call_when_found) {
		arg_element->call_when_found(argv, offset);
	}
}

/**
 * Try to find if this argument is in the list (and it is not manual)
 * if it does, try to parse it, set its dest accordingly, and return true
 * if it is not found, return false
 */
bool cmd_parse_one_arg(char *argv, struct args_struct_t args_struct[])
{
	int count = 0;
	int ret;

	if (cmd_is_help_option(argv)) {
		cmd_print_long_help(args_struct);
		posix_exit(0);
	}

	while (args_struct[count].option != NULL) {
		if (args_struct[count].manual) {
			count++;
			continue;
		}
		ret = cmd_is_option(argv, args_struct[count].option,
				    !args_struct[count].is_switch);
		if (ret) {
			cmd_handle_this_matched_arg(argv,
						    ret,
						    &args_struct[count]);
			return true;
		}
		count++;
	}
	return false;
}
