/*
 * Copyright (C) 2022, Sensorfy B.V
 *
 * @brief modem gnss parser
 */

#include "modem_gnss_parser.h"

char *gnss_get_next_param(char *src, const char *delim, char **saveptr)
{
	char *start, *del;

	if (src) {
		start = src;
	} else {
		start = *saveptr;
	}

	/* Illegal start string. */
	if (!start) {
		return NULL;
	}

	/* End of string reached. */
	if (*start == '\0' || *start == '\r') {
		return NULL;
	}

	del = strstr(start, delim);
	if (!del) {
		return NULL;
	}

	*del = '\0';
	*saveptr = del + 1;

	if (del == start) {
		return NULL;
	}

	return start;
}

void gnss_skip_param(char **saveptr)
{
	gnss_get_next_param(NULL, ",", saveptr);
}

int gnss_split_on_dot(const char *src, int32_t *f1, int32_t *f2)
{
	char *dot = strchr(src, '.');

	if (!dot) {
		return -1;
	}

	*dot = '\0';

	*f1 = (int32_t)strtol(src, NULL, 10);
	*f2 = (int32_t)strtol(dot + 1, NULL, 10);

	if (*f1 < 0) {
		*f2 = -*f2;
	}

	return 0;
}
