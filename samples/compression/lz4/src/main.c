/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/misc/lorem_ipsum.h>
#include <string.h>
#include <stdlib.h>
#include "lz4.h"

int main(void)
{
	static const char *src = LOREM_IPSUM;

	const int src_size = (int)(strlen(src) + 1);
	const int max_dst_size = LZ4_compressBound(src_size);

	char *compressed_data = malloc((size_t)max_dst_size);

	if (compressed_data == NULL) {
		printk("Failed to allocate memory for compressed data\n");
		return 0;
	}

	const int compressed_data_size = LZ4_compress_default(src,
					compressed_data, src_size,
					max_dst_size);
	if (compressed_data_size <= 0) {
		printk("Failed to compress the data\n");
		return 0;
	}

	printk("Original Data size: %d\n", src_size);
	printk("Compressed Data size : %d\n", compressed_data_size);

	compressed_data = (char *)realloc(compressed_data,
					  (size_t)compressed_data_size);
	if (compressed_data == NULL) {
		printk("Failed to re-alloc memory for compressed data\n");
		return 0;
	}

	char * const decompressed_data = malloc(src_size);

	if (decompressed_data == NULL) {
		printk("Failed to allocate memory to decompress data\n");
		return 0;
	}

	const int decompressed_size = LZ4_decompress_safe(compressed_data,
				      decompressed_data, compressed_data_size,
				      src_size);
	if (decompressed_size < 0) {
		printk("Failed to decompress the data\n");
		return 0;
	}

	free(compressed_data);

	if (decompressed_size >= 0) {
		printk("Successfully decompressed some data\n");
	}

	if (decompressed_size != src_size) {
		printk("Decompressed data is different from original\n");
		return 0;
	}

	if (memcmp(src, decompressed_data, src_size) != 0) {
		printk("Validation failed.\n");
		printk("*src and *new_src are not identical\n");
		return 0;
	}

	printk("Validation done. The string we ended up with is:\n%s\n",
			decompressed_data);
	return 0;
}
