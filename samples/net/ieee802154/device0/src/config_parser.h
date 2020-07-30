/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CONFIG_PARSER_H
#define _CONFIG_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_COLS_IN_FILE (10)
#define MAX_COLUMN_LEN (260)
#define IGNORE_LINE_CHAR '#'
#define MAX_LINE_LEN (500)
#define SEPARATOR_CHAR ','

enum csv_datatypes {
	CSV_DATATYPE_BYTESTREAM,
	CSV_DATATYPE_STRING,
	CSV_DATATYPE_BOOLEAN
};

/* Describes the line of comma separated columns */
struct config_parser {
	/** Required number of columns in the line */
	int ncols;

	/** Datatype of each column */
	enum csv_datatypes datatypes[MAX_COLS_IN_FILE];

	/** Stores successfully parsed data */
	union {
		void *any;
		uint8_t *bytestream;
		bool *boolean;
		char *str;
	} dest[MAX_COLS_IN_FILE];
};

#ifdef __cplusplus
extern "C" {
#endif

bool config_openfile(const char *filename);
void config_closefile(void);
bool config_parse_line(struct config_parser *parser);
void config_parser_clean(struct config_parser *parser);
int get_config_lines_number(void);

#ifdef __cplusplus
}
#endif

#endif /* _CONFIG_PARSER_H */
