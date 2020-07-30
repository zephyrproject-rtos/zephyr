/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "config_parser.h"

typedef int (*config_parser_func)(char *, void **);

struct config_line {
	int ncols;
	char line[MAX_LINE_LEN];
	char *cols[MAX_COLS_IN_FILE];
	int col_lens[MAX_COLS_IN_FILE];
};

static int parse_string(char *s, void **pvstring);
static int parse_boolean(char *s, void **pvboolean);
static int parse_bytestream(char *s, void **bytes);

static FILE *fd;
static config_parser_func config_parsers[] = { parse_bytestream, parse_string,
					       parse_boolean };
static int config_lines_number;

static int count_config_lines_in_file(void)
{
	int nlines;
	char line[MAX_LINE_LEN];

	for (nlines = 0; fgets(line, sizeof(line), fd);) {
		if (line[0] != IGNORE_LINE_CHAR) {
			nlines++;
		}
	}

	return nlines;
}

int get_config_lines_number(void)
{
	return config_lines_number;
}

bool config_openfile(const char *filename)
{
	bool ret;

	fd = fopen(filename, "r");
	ret = (bool)fd;

	if (ret) {
		config_lines_number = count_config_lines_in_file();
		fseek(fd, 0, SEEK_SET);
	} else {
		config_lines_number = -1;
	}

	return ret;
}

void config_closefile(void)
{
	fclose(fd);
}

static void strreplace(char *str, char old, char new)
{
	for (int i = 0; i < strlen(str); i++) {
		if (str[i] == old) {
			str[i] = new;
		}
	}
}

static bool config_getline(struct config_line *config_line)
{
	if (!fd) {
		printf("File is not open\n");
		return false;
	}

	do {
		if (!fgets(config_line->line, sizeof(config_line->line), fd)) {
			return false;
		}
	} while (config_line->line[0] == IGNORE_LINE_CHAR);

	/* Trim the string */
	strreplace(config_line->line, '\n', 0);
	config_line->ncols = 0;

	char *prev_col = config_line->line;

	for (int i = 0; i <= strlen(config_line->line); i++) {
		if (config_line->line[i] == SEPARATOR_CHAR ||
		    i == strlen(config_line->line)) {
			char *curr_col = config_line->line + i + 1;

			config_line->cols[config_line->ncols] = prev_col;
			config_line->col_lens[config_line->ncols] =
				(curr_col - 1) - prev_col;
			config_line->ncols++;
			prev_col = curr_col;
		}
	}

	return true;
}

/** Parses NULL-terminated string of bytes in hex format to bytes
 *
 * Arguments:
 * s             -   Null-terminated string containing bytestream in hex notation
 * pvbytes [out] -   Buffer that will be written, bytes[0] contains the bytestream len.
 *                 The buffer is allocated inside function and needs to be freed after.
 *
 * Returns:
 * Number of bytes parsed   -   On success
 * Negative                 -   On failure
 */
static int parse_bytestream(char *s, void **pvbytes)
{
	int nchars;
	int nbytes;
	uint8_t **bytes = (uint8_t **)pvbytes;

	*bytes = malloc((strlen(s) / 2) + 1);
	if (!(*bytes)) {
		return -ENOMEM;
	}

	uint8_t *bytes_out = (*bytes) + 1;
	uint8_t *bytes_len = (*bytes);

	for (nbytes = 0, nchars = 0; nchars < strlen(s);
	     nchars += 2, nbytes++) {
		char number[] = { s[nchars], s[nchars + 1], 0 };

		bytes_out[nbytes] = strtol(number, NULL, 16);
	}

	*bytes_len = nbytes;
	return nbytes;
}

/** Parses NULL-terminated string of boolean value to bool
 *
 * Arguments:
 * s             -   Null-terminated string containing "false", "true", "1","0"
 *                   Case sensitive.
 * pvboolean [out] -   Buffer that will be written with parsed value
 *              	   The buffer is allocated inside function and needs
 * 					   to be freed after.
 * Returns:
 * 0          -   On success
 * Negative   -   On failure
 */
static int parse_boolean(char *s, void **pvboolean)
{
	int ret = -EINVAL;
	bool **boolean = (bool **)pvboolean;
	*boolean = malloc(sizeof(bool));

	if (strcmp(s, "true") == 0) {
		**boolean = true;
		ret = 0;
	} else if (strcmp(s, "false") == 0) {
		**boolean = false;
		ret = 0;
	} else if (strcmp(s, "1") == 0) {
		**boolean = true;
		ret = 0;
	} else if (strcmp(s, "0") == 0) {
		**boolean = false;
		ret = 0;
	}

	return ret;
}

/**
 * Parses NULL-terminated string of characters to string
 *
 * Arguments:
 * s             -   Null-terminated string containing "false", "true","1","0"
 *                   Case sensitive.
 * pvstring [out] -   Buffer that will be written with parsed value
 *                 The buffer is allocated inside function and needs
 * 				   to be freed after.
 *
 * Returns:
 * String len          -   On success
 * Negative            -   On failure
 */
static int parse_string(char *s, void **pvstring)
{
	int len = strlen(s);
	char **str = (char **)pvstring;

	*str = malloc(len + 1);
	memcpy(*str, s, len);
	(*str)[len] = 0;
	return len;
}

/**
 * Parse next line found in open file.
 *
 * Arguments:
 * parser	-	Points to parser used to parse the line
 *
 * Returns:
 * false	-	Failed to parse next line (EOL or line format doesn't match
 *											parser)
 * true		-	Successfully parsed the line
 */
bool config_parse_line(struct config_parser *parser)
{
	struct config_line line;

	if (!config_getline(&line)) {
		return false;
	}

	if (parser->ncols != line.ncols) {
		return false;
	}

	for (int col = 0; col < parser->ncols; col++) {
		char column[MAX_COLUMN_LEN] = "";
		int ret;

		memcpy(column, line.cols[col], line.col_lens[col]);
		column[line.col_lens[col]] = 0;
		config_parser_func config_parser_f =
			config_parsers[parser->datatypes[col]];
		ret = config_parser_f(column, &parser->dest[col].any);
		if (ret < 0) {
			return false;
		}
	}

	return true;
}

/**
 * Clean the data allocated during parsing the line
 *
 * Arguments:
 * parser	-	Points to parser that will be cleaned up
 */
void config_parser_clean(struct config_parser *parser)
{
	for (int col = 0; col < parser->ncols; col++) {
		free(parser->dest[col].any);
		parser->dest[col].any = NULL;
	}
}
