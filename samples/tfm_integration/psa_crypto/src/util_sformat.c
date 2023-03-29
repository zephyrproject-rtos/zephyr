/*
 * Copyright (c) 2019,2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <stdio.h>
#include "util_sformat.h"

static void sf_hex_ascii(unsigned char *data, size_t len, unsigned char nonvis)
{
	uint32_t idx;

	/* Render printable characters. */
	idx = 0;
	while (len) {
		printf("%c", isprint(data[idx]) ? data[idx] : nonvis);
		idx++;
		len--;
	}
}

void sf_hex_tabulate_16(struct sf_hex_tbl_fmt *fmt, unsigned char *data,
			size_t len)
{
	uint32_t idx;
	uint32_t cpos;     /* Current position. */
	uint32_t ca;       /* Current address. */
	uint32_t ea;       /* End address. */

	if (!len) {
		return;
	}

	/* Set the end address (since we modify len in the write loop). */
	ea = fmt->addr + len;

	/* Check if we need to render the top address bar. */
	if (fmt->addr_label) {
		/* Render the top address bar. */
		printf("\n");
		printf("          ");
		printf("0  1  2  3  4  5  6  7  8  9  ");
		printf("A  B  C  D  E  F\n");
		printf("%08X ", fmt->addr - (fmt->addr % 16));
	}

	/* Insert offset padding for first row if necessary. */
	cpos = fmt->addr % 16;
	if (cpos != 0) {
		for (idx = 0; idx < cpos; idx++) {
			printf("   ");
		}
	}

	/* Print data row by row. */
	idx = 0;
	ca = fmt->addr;
	while (len) {
		/* Print the current value. */
		printf("%02X ", data[idx++]);
		cpos++;
		ca++;

		/* Wrap around to the next line if necessary. */
		if (cpos == 16 || ca == ea) {
			/* Render ASCII equiv at end of row if requested. */
			if (fmt->ascii) {
				if (ca == ea) {
					/* Handle last/single row. */
					if (ca % 16) {
						/* PARTIAL row (< 16 vals). */
						printf("%.*s",
						       (16 - ca % 16) * 3,
						       "                "
						       "                "
						       "                ");
						sf_hex_ascii(
							&data[idx - (ca % 16)],
							ca - fmt->addr < 16 ?
							idx % 16 : ca % 16,
							'.');
					} else {
						/* FULL row. */
						sf_hex_ascii(
							&data[idx - 16],
							16, '.');
					}
				} else if (ca < fmt->addr + 15) {
					/* Handle first row. */
					printf("%.*s", fmt->addr % 16,
					       "                ");
					sf_hex_ascii(data,
						     16 - fmt->addr % 16, '.');
				} else {
					/* Full row. */
					sf_hex_ascii(&data[idx - 16], 16, '.');
				}
			}

			/* Wrap around if this isn't the last row. */
			printf("\n");
			if (ca != ea) {
				/* Render the next base row addr. */
				if (fmt->addr_label) {
					printf("%08X ", ca);
				}
			}
			cpos = 0;
		}
		len--;
	}
	printf("\n");
}
