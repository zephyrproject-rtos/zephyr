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

#ifdef __need_getopt_newlib
struct getopt_data {
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

/*
 * The GETOPT_DATA_INITIALIZER macro is used to initialize a statically-
 * allocated variable of type struct getopt_data.
 */

#define GETOPT_DATA_INITIALIZER	{ 0 }

#endif

#if defined(__need_getopt_legacy) || !defined(__need_getopt_newlib)
/* Applications using the newlib-style APIs "shouldn't" be using these variables */
extern int optreset; /* reset getopt */
extern char *optarg;
extern int opterr;
extern int optind;
extern int optopt;
#endif

#define no_argument       0
#define required_argument 1
#define optional_argument 2

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
int getopt_long(int nargc, char *const *nargv, const char *options,
		const struct option *long_options, int *idx);

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
int getopt_long_only(int nargc, char *const *nargv, const char *options,
		     const struct option *long_options, int *idx);

/* Re-entrant forms of the above functions. These follow the newlib/picolibc API */

#ifdef __need_getopt_newlib

/* These #defines are to make accessing the reentrant functions easier.  */
#define getopt_r		__getopt_r
#define getopt_long_r		__getopt_long_r
#define getopt_long_only_r	__getopt_long_only_r

int __getopt_r(int __argc, char *const __argv[], const char *__optstring,
	       struct getopt_data *__data);

int __getopt_long_r(int __argc, char *const __argv[], const char *__shortopts,
		    const struct option *__longopts, int *__longind,
		    struct getopt_data *__data);

int __getopt_long_only_r(int __argc, char *const __argv[], const char *__shortopts,
			 const struct option *__longopts, int *__longind,
			 struct getopt_data *__data);

#endif /* __need_getopt_newlib */

#ifdef __cplusplus
}
#endif

#endif /* _GETOPT_H__ */
