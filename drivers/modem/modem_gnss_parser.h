/*
 * Copyright (C) 2022, Sensorfy B.V
 *
 * @brief modem gnss parser
 */

#ifndef MODEM_GNSS_PARSER_H
#define MODEM_GNSS_PARSER_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/**
 * Get the next parameter from the gnss phrase.
 *
 * @param src The source string supported on first call.
 * @param delim The delimiter of the parameter list.
 * @param saveptr Pointer for subsequent parses.
 * @return On success a pointer to the parameter. On failure
 *         or end of string NULL is returned.
 *
 * This function is used instead of strtok because strtok would
 * skip empty parameters, which is not desired. The modem may
 * omit parameters which could lead to a incorrect parse.
 */
char *gnss_get_next_param(char *src, const char *delim, char **saveptr);

/**
 * Skip the next parameter from the gnss phrase.
 *
 * @param saveptr Pointer for subsequent parses.
 */
void gnss_skip_param(char **saveptr);

/**
 * Splits float parameters of the CGNSINF response on '.'
 *
 * @param src Null terminated string containing the float.
 * @param f1 Resulting number part of the float.
 * @param f2 Resulting fraction part of the float.
 * @return 0 if parsing was successful. Otherwise <0 is returned.
 *
 * If the number part of the float is negative f1 and f2 will be
 * negative too.
 */
int gnss_split_on_dot(const char *src, int32_t *f1, int32_t *f2);

#endif /* MODEM_GNSS_PARSER_H */
