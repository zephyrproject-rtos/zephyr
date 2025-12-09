/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_GETOPT_H_
#define ZEPHYR_INCLUDE_SYS_GETOPT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>

struct sys_getopt_state {
	int opterr;   /* if error message should be printed */
	int optind;   /* index into parent argv vector */
	int optopt;   /* character checked for validity */
	int optreset; /* reset getopt */
	char *optarg; /* argument associated with option */

	char *place; /* option letter processing */

#if CONFIG_GETOPT_LONG
	int nonopt_start;
	int nonopt_end;
#endif
};

extern int sys_getopt_optreset; /* reset getopt */
extern char *sys_getopt_optarg;
extern int sys_getopt_opterr;
extern int sys_getopt_optind;
extern int sys_getopt_optopt;

#define sys_getopt_no_argument       0
#define sys_getopt_required_argument 1
#define sys_getopt_optional_argument 2

struct sys_getopt_option {
	/* name of long option */
	const char *name;
	/*
	 * one of no_argument, required_argument, and optional_argument:
	 * whether option takes an argument
	 */
	int has_arg;
	/* if not NULL, set *flag to val when option found */
	int *flag;
	/* if flag not NULL, value to set *flag to; else return value */
	int val;
};

/* Function initializes getopt_state structure for current thread */
void sys_getopt_init(void);

/* Function returns getopt_state structure for the current thread. */
struct sys_getopt_state *sys_getopt_state_get(void);

/**
 * @brief Parses the command-line arguments.
 *
 * @note This function is based on FreeBSD implementation but it does not
 * support environment variable: POSIXLY_CORRECT.
 *
 * @param[in] nargc	   Arguments count.
 * @param[in] nargv	   Arguments.
 * @param[in] ostr	   String containing the legitimate option characters.
 *
 * @return		If an option was successfully found, function returns
 *			the option character.
 */
int sys_getopt(int nargc, char *const nargv[], const char *ostr);

/**
 * @brief Parses the command-line arguments.
 *
 * The sys_getopt_long() function works like @ref sys_getopt() except
 * it also accepts long options, started with two dashes.
 *
 * @note This function is based on FreeBSD implementation but it does not
 * support environment variable: POSIXLY_CORRECT.
 *
 * @param[in] nargc	   Arguments count.
 * @param[in] nargv	   Arguments.
 * @param[in] options	   String containing the legitimate option characters.
 * @param[in] long_options Pointer to the first element of an array of
 *			   @a struct sys_getopt_option.
 * @param[in] idx	   If idx is not NULL, it points to a variable
 *			   which is set to the index of the long option relative
 *			   to @p long_options.
 *
 * @return		If an option was successfully found, function returns
 *			the option character.
 */
int sys_getopt_long(int nargc, char *const *nargv, const char *options,
		    const struct sys_getopt_option *long_options, int *idx);

/**
 * @brief Parses the command-line arguments.
 *
 * The sys_getopt_long_only() function works like @ref sys_getopt_long(),
 * but '-' as well as "--" can indicate a long option. If an option that starts
 * with '-' (not "--") doesn't match a long option, but does match a short
 * option, it is parsed as a short option instead.
 *
 * @note This function is based on FreeBSD implementation but it does not
 * support environment variable: POSIXLY_CORRECT.
 *
 * @param[in] nargc	   Arguments count.
 * @param[in] nargv	   Arguments.
 * @param[in] options	   String containing the legitimate option characters.
 * @param[in] long_options Pointer to the first element of an array of
 *			   @a struct sys_getopt_option.
 * @param[in] idx	   If idx is not NULL, it points to a variable
 *			   which is set to the index of the long option relative
 *			   to @p long_options.
 *
 * @return		If an option was successfully found, function returns
 *			the option character.
 */
int sys_getopt_long_only(int nargc, char *const *nargv, const char *options,
			 const struct sys_getopt_option *long_options, int *idx);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_GETOPT_H_ */
