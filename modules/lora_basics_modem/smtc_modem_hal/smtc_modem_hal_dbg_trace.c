/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <smtc_modem_hal_dbg_trace.h>

#include <zephyr/logging/log.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(smtc_modem, CONFIG_LORA_BASICS_MODEM_LOG_LEVEL);

#define PRINT_BUFFER_SIZE 255

#ifdef CONFIG_LOG

/**
 * @brief Check if string is "useless"
 *
 * This is a very opinionated filter for Semtech's logging, since we do not require prints such as
 *
 * *****************************
 * *  TX DONE                 *
 * *****************************
 *
 * I would prefer just a simple
 * TX DONE
 *
 * This function returns true for all strings that only contain spaces, starts and new lines
 *
 * @param[in] text The text to check
 * @return true If string is useless and should not be printed
 * @return false If string i not useless and should be printed
 */
static bool prv_string_is_useless(char *text)
{
	for (int i = 0; i < strlen(text); i++) {
		if (text[i] != '\n' && text[i] != ' ' && text[i] != '*') {
			return false;
		}
	}
	return true;
}

/**
 * @brief Trims a string
 *
 * This is a very opinionated filter for Semtech's logging (see above)
 *
 * This function "removes" spaces at the beginning and end of strings.
 *
 * The returned string is just a pointer to a substring of the original string. A NULL
 * terminator is also inserted early into the string if spaces at the end should be trimmed.
 *
 * @param[in] text The text to trim
 * @return char* The trimmed text
 */
static char *prv_string_trim(char *text)
{
	char *end;

	/* Trim leading space */
	while (isspace((unsigned char)*text)) {
		text++;
	}

	/* All spaces? */
	if (*text == 0) {
		return text;
	}

	/* Trim trailing space */
	end = text + strlen(text) - 1;
	while (end > text && isspace((unsigned char)*end)) {
		end--;
	}

	/* Write new null terminator character */
	end[1] = '\0';

	return text;
}
#endif /* CONFIG_LOG */

void smtc_modem_hal_print_trace(const char *fmt, ...)
{
#ifdef CONFIG_LOG
	char text[PRINT_BUFFER_SIZE];
	va_list args;

	va_start(args, fmt);
	vsprintf(text, fmt, args);
	if (!prv_string_is_useless(text)) {
		char *ttext = prv_string_trim(text);

		LOG_INF("%s", ttext);
	}
	va_end(args);
#endif /* CONFIG_LOG */
}

void smtc_modem_hal_print_trace_inf(const char *fmt, ...)
{
#ifdef CONFIG_LOG
	char text[PRINT_BUFFER_SIZE];
	va_list args;

	va_start(args, fmt);
	vsprintf(text, fmt, args);
	if (!prv_string_is_useless(text)) {
		char *ttext = prv_string_trim(text);

		LOG_INF("%s", ttext);
	}
	va_end(args);
#endif /* CONFIG_LOG */
}

void smtc_modem_hal_print_trace_dbg(const char *fmt, ...)
{
#ifdef CONFIG_LOG
	char text[PRINT_BUFFER_SIZE];
	va_list args;

	va_start(args, fmt);
	vsprintf(text, fmt, args);
	if (!prv_string_is_useless(text)) {
		char *ttext = prv_string_trim(text);

		LOG_DBG("%s", ttext);
	}
	va_end(args);
#endif /* CONFIG_LOG */
}

void smtc_modem_hal_print_trace_err(const char *fmt, ...)
{
#ifdef CONFIG_LOG
	char text[PRINT_BUFFER_SIZE];
	va_list args;

	va_start(args, fmt);
	vsprintf(text, fmt, args);
	if (!prv_string_is_useless(text)) {
		char *ttext = prv_string_trim(text);

		LOG_ERR("%s", ttext);
	}
	va_end(args);
#endif /* CONFIG_LOG */
}

void smtc_modem_hal_print_trace_wrn(const char *fmt, ...)
{
#ifdef CONFIG_LOG
	char text[PRINT_BUFFER_SIZE];
	va_list args;

	va_start(args, fmt);
	vsprintf(text, fmt, args);
	if (!prv_string_is_useless(text)) {
		char *ttext = prv_string_trim(text);

		LOG_WRN("%s", ttext);
	}
	va_end(args);
#endif /* CONFIG_LOG */
}

void smtc_modem_hal_print_trace_array_inf(char *msg, uint8_t *array, uint32_t len)
{
#ifdef CONFIG_LOG
	LOG_HEXDUMP_INF(array, len, msg);
#endif /* CONFIG_LOG */
}

void smtc_modem_hal_print_trace_array_dbg(char *msg, uint8_t *array, uint32_t len)
{
#ifdef CONFIG_LOG
	LOG_HEXDUMP_DBG(array, len, msg);
#endif /* CONFIG_LOG */
}
