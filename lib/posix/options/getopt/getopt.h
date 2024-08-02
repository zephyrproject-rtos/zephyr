/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _GETOPT_H__
#define _GETOPT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>

struct getopt_state {
	int opterr;	/* if error message should be printed */
	int optind;	/* index into parent argv vector */
	int optopt;	/* character checked for validity */
	int optreset;	/* reset getopt */
	char *optarg;	/* argument associated with option */

	char *place;	/* option letter processing */

#if CONFIG_GETOPT_LONG
	int nonopt_start;
	int nonopt_end;
#endif
};

extern int optreset;	/* reset getopt */
extern char *optarg;
extern int opterr;
extern int optind;
extern int optopt;

#define no_argument        0
#define required_argument  1
#define optional_argument  2

struct option {
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
void getopt_init(void);

/* Function returns getopt_state structure for the current thread. */
struct getopt_state *getopt_state_get(void);

/**
 * @brief Parses the command-line arguments.
 *
 * The getopt_long() function works like @ref getopt() except
 * it also accepts long options, started with two dashes.
 *
 * @note This function is based on FreeBSD implementation but it does not
 * support environment variable: POSIXLY_CORRECT.
 *
 * @param[in] argc	   Arguments count.
 * @param[in] argv	   Arguments.
 * @param[in] options	   String containing the legitimate option characters.
 * @param[in] long_options Pointer to the first element of an array of
 *			   @a struct z_option.
 * @param[in] long_idx	   If long_idx is not NULL, it points to a variable
 *			   which is set to the index of the long option relative
 *			   to @p long_options.
 *
 * @return		If an option was successfully found, function returns
 *			the option character.
 */
int getopt_long(int nargc, char *const *nargv,
		const char *options, const struct option *long_options,
		int *idx);

/**
 * @brief Parses the command-line arguments.
 *
 * The getopt_long_only() function works like @ref getopt_long(),
 * but '-' as well as "--" can indicate a long option. If an option that starts
 * with '-' (not "--") doesn't match a long option, but does match a short
 * option, it is parsed as a short option instead.
 *
 * @note This function is based on FreeBSD implementation but it does not
 * support environment variable: POSIXLY_CORRECT.
 *
 * @param[in] argc	   Arguments count.
 * @param[in] argv	   Arguments.
 * @param[in] options	   String containing the legitimate option characters.
 * @param[in] long_options Pointer to the first element of an array of
 *			   @a struct option.
 * @param[in] long_idx	   If long_idx is not NULL, it points to a variable
 *			   which is set to the index of the long option relative
 *			   to @p long_options.
 *
 * @return		If an option was successfully found, function returns
 *			the option character.
 */
int getopt_long_only(int nargc, char *const *nargv,
		     const char *options, const struct option *long_options,
		     int *idx);

#ifdef __cplusplus
}
#endif

#endif /* _GETOPT_H__ */
