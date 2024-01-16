/*
 * Copyright (c) 2023 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <stdlib.h>
#include "heatshrink_encoder.h"
#include "heatshrink_decoder.h"

static heatshrink_encoder hse;
static heatshrink_decoder hsd;

int main(void)
{
	static char *src = "Lorem ipsum dolor sit amet, consectetur "
			   "adipiscing elit. Quisque sodales lorem lorem, sed congue enim "
			   "vehicula a. Sed finibus diam sed odio ultrices pharetra. Nullam "
			   "dictum arcu ultricies turpis congue,vel venenatis turpis venenatis. "
			   "Nam tempus arcu eros, ac congue libero tristique congue. Proin velit "
			   "lectus, euismod sit amet quam in, maximus condimentum urna. Cras vel "
			   "erat luctus, mattis orci ut, varius urna. Nam eu lobortis velit."
			   "\n"
			   "Nullam sit amet diam vel odio sodales cursus vehicula eu arcu. Proin "
			   "fringilla, enim nec consectetur mollis, lorem orci interdum nisi, "
			   "vitae suscipit nisi mauris eu mi. Proin diam enim, mollis ac rhoncus "
			   "vitae, placerat et eros. Suspendisse convallis, ipsum nec rhoncus "
			   "aliquam, ex augue ultrices nisl, id aliquet mi diam quis ante. "
			   "Pellentesque venenatis ornare ultrices. Quisque et porttitor lectus. "
			   "Ut venenatis nunc et urna imperdiet porttitor non laoreet massa."
			   "Donec eleifend eros in mi sagittis egestas. Sed et mi nunc. Nunc "
			   "vulputate,mauris non ullamcorper viverra, lorem nulla vulputate "
			   "diam, et congue dui velit non erat. Duis interdum leo et ipsum "
			   "tempor consequat. In faucibus enim quis purus vulputate nullam."
			   "\n";

	heatshrink_encoder_reset(&hse);
	heatshrink_decoder_reset(&hsd);

	const uint32_t src_size = (strlen(src) + 1);
	const uint32_t max_dst_size = src_size;
	uint8_t *compressed_data = malloc(max_dst_size);

	if (compressed_data == NULL) {
		printf("Failed to allocate memory for compressed data\n");
		return -EFAULT;
	}

	size_t count = 0;
	uint32_t sunk = 0;
	uint32_t compressed_size = 0;

	while (sunk < src_size) {
		if (heatshrink_encoder_sink(&hse, &src[sunk], src_size - sunk, &count) < 0) {
			printk("Failed to sink data into encoder");
		}

		sunk += count;
		if (sunk == src_size) {
			if (heatshrink_encoder_finish(&hse) != HSER_FINISH_MORE) {
				break;
			}
		}

		HSE_poll_res pres;

		do { /* "turn the crank" */
			pres = heatshrink_encoder_poll(&hse, &compressed_data[compressed_size],
						       max_dst_size - compressed_size, &count);
			if (pres < 0) {
				printk("Failed to poll data from encoder");
			}
			compressed_size += count;

		} while (pres == HSER_POLL_MORE);

		if (pres != HSER_POLL_EMPTY) {
			printk("Inconsistent encoder state");
		}
		if (compressed_size >= max_dst_size) {
			printk("Compression should never expand that much");
			break;
		}
		if (sunk == src_size) {
			if (heatshrink_encoder_finish(&hse) != HSER_FINISH_DONE) {
				break;
			}
		}
	}

	if (compressed_size <= 0) {
		printk("Failed to compress data\n");
		return -ENOMSG;
	}

	printk("Original Data size: %u\n", src_size);
	printk("Compressed Data size : %u\n", compressed_size);

	compressed_data = (char *)realloc(compressed_data, compressed_size);
	if (compressed_data == NULL) {
		printk("Failed to re-alloc memory for compressed data\n");
		return -EFAULT;
	}

	/* Decompression part */
	const uint32_t decompress_buf_size = src_size + 1;
	uint8_t *decompressed_data = malloc(decompress_buf_size);

	if (decompressed_data == NULL) {
		printk("Failed to allocate memory to decompress data\n");
		return -EFAULT;
	}

	sunk = 0;
	uint32_t decompressed_size = 0;

	while (sunk < compressed_size) {
		if (heatshrink_decoder_sink(&hsd, &compressed_data[sunk], compressed_size - sunk,
					    &count) < 0) {
			printk("Failed to sink data into decoder");
			break;
		}
		sunk += count;

		if (sunk == compressed_size) {
			if (heatshrink_decoder_finish(&hsd) != HSDR_FINISH_MORE) {
				break;
			}
		}

		HSD_poll_res pres;

		do {
			pres = heatshrink_decoder_poll(&hsd, &decompressed_data[decompressed_size],
						       decompress_buf_size - decompressed_size,
						       &count);
			if (pres < 0) {
				printk("Failed to poll data from decoder");
			}
			decompressed_size += count;

		} while (pres == HSDR_POLL_MORE);

		if (pres != HSDR_POLL_EMPTY) {
			printk("Inconsistent decoder state");
			break;
		}
		if (sunk == compressed_size) {
			HSD_finish_res fres = heatshrink_decoder_finish(&hsd);

			if (fres != HSDR_FINISH_DONE) {
				break;
			}
		}
	}

	free(compressed_data);

	if (decompressed_size >= 0) {
		printk("Successfully decompressed some data\n");
	}

	if (decompressed_size != src_size) {
		printk("Decompressed data is different from original\n");
		return -EILSEQ;
	}

	if (memcmp(src, decompressed_data, src_size) != 0) {
		printk("Validation failed.\n");
		printk("*src and *new_src are not identical\n");
		return -EILSEQ;
	}

	printk("Validation done. The string we ended up with is:\n%s\n", decompressed_data);

	free(decompressed_data);
	return 0;
}
