/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_PSA_CRYPTO_TESTS_OVERRIDES_H
#define ZEPHYR_PSA_CRYPTO_TESTS_OVERRIDES_H

#include <zephyr/sys/minmax.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <malloc.h>

#include <stdio.h>

/* Helper function to dump content easily portable in C data arrays */
static inline void dump(const char *str, const void *ptr, size_t size)
{
	const uint8_t *p = ptr;
	size_t cnt;

	if (str) {
		printf("Dump %zu bytes: %s\n", size, str);
	}

	for (cnt = 0; cnt < size; cnt++) {
		if (cnt && !(cnt % 7))
			printf("\n");

		printf("0x%02x, ", *p++);

		if (cnt % 8 == 7)
			printf("\n");
	}
}

/*
 * Override FILE, fprintf() and a few more to use pseudo ASCII files that actually
 * are char arrays.
 */
#define fprintf(stream, ...)	printf(__VA_ARGS__)
#define mbedtls_fprintf		fprintf

struct pseudo_file {
	const char *start;
	const char *end;
	const char *ptr;
	int line_num;
};

#define FILE		struct pseudo_file
#define fclose(file)	free(file)
#undef feof
#define feof(file)	((file)->ptr[0] == '\0')
#define fopen		pseudo_file_fopen
#define fgets		pseudo_file_fgets
#define fflush(foo)

static inline FILE *pseudo_file_fopen(const char *name, char *rights)
{
	FILE *file = malloc(sizeof(FILE));

	if (file) {
		size_t len = strlen(name);

		if (name[len - 1] != '\n') {
			printf("Error in pseudo ASCII file. It shall end with a line-return before null byte\n");
			free(file);
			return NULL;
		}
		file->start = name;
		file->ptr = name;
		file->end = name + len;
		file->line_num = -1;
	} else {
		printf("Failed to allocate pseudo-file structure (a few bytes)\n");
	}

	return file;
}


static inline char *pseudo_file_fgets(char *buf, size_t size, FILE *file)
{
	char *e;

	if (file->ptr == file->end) {
		/* End of data reached */
		return NULL;
	}

	e = strchr(file->ptr, '\n');

	if (e && buf) {
		memcpy(buf, file->ptr, min(e - file->ptr, size - 1));
		buf[e - file->ptr] = '\0';
		file->ptr = e;
		if (e < file->end) {
			file->ptr++;
		}
		file->line_num++;
		return buf;
	}

	return NULL;
}

/* FIXME: likely no more needed */
#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH		ARRAY_SIZE
#endif

/*
 * This is the scope in which we test the PSA Crypto implementation
 */
#define MBEDTLS_PSA_CRYPTO_CLIENT
#define MBEDTLS_PSA_CRYPTO_C

/*  We need to access private fields in PSA crypto and mbedtls structures */
#define MBEDTLS_ALLOW_PRIVATE_ACCESS

/* FIXME: these includes are likely no more needed here, already included
 * from the ts-psa-crypto and mbedtls-framework source files.
 */
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#endif /* ZEPHYR_PSA_CRYPTO_TESTS_OVERRIDES_H */

